#include "vm/page.h"
#include "threads/palloc.h"
#include "userprog/pagedir.h"
#include "threads/thread.h"
#include "threads/pte.h"
#include <stdbool.h>
#include <hash.h>
#include "devices/block.h"
#include "threads/vaddr.h"
#include <debug.h>

struct fte
{
	unsigned int *pte;
	struct hash * spt;
};

static struct fte *frame_table[32];

unsigned int spte_hash_func(const struct hash_elem *e, void *aux UNUSED)
{
	return hash_entry(e, struct spte, elem)->pte;
}

bool spte_hash_less(const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED)
{
	return hash_entry(a, struct spte, elem)->pte < hash_entry(b, struct spte, elem)->pte;
}

int fd_no(void * addr)
{
	return ((unsigned int)vtop(addr) >> (12 + 9)) & 0x1F;
}

int ft_no(void *addr)
{
	return ((unsigned int)vtop(addr) >> 12) & 0x1FF;
}

struct swap_page
{
	bool taken;
};

struct swap_page * swap_table = NULL;

int get_swap_page()
{
	struct block * swap_block = block_get_role(BLOCK_SWAP);
	unsigned int swap_size = block_size(swap_block)/8;
	if (swap_table == NULL)
	{
		swap_table = malloc(sizeof(struct swap_page)*swap_size);
		for (int i = 0; i < swap_size; i++)
		{
			swap_table[i].taken = false;
		}
	}
	int swap_page = -1;
	for (int i = 0; i < swap_size; i++)
	{
		if (!swap_table[i].taken)
		{
			swap_page = i;
			break;
		}
	}
	if (swap_page == -1) PANIC("No more swap disk space");
	swap_table[swap_page].taken = true;
	return swap_page;
}

void free_swap_page(int page)
{
	swap_table[page].taken = false;
}

static unsigned int cur_fte_index = 0;

struct fte * find_page()
{
	struct fte * categories[3] = { NULL, NULL, NULL };
	int searched_pages = 0, s = 0;
	while (searched_pages < 256)
	{
		struct fte * fd = frame_table[cur_fte_index >> 9];
		if (fd == NULL)
		{
			cur_fte_index = (cur_fte_index&(~0x1FF)) + 512;
			if (cur_fte_index >= 16384) cur_fte_index = 0;
			continue;
		}
		struct fte * fte = &fd[cur_fte_index & 0x1FF];
		if (fte->pte != NULL)
		{
			searched_pages++;
			if (*fte->pte&PTE_A)
			{
				if (*fte->pte&PTE_D)
				{
					if (categories[0] == NULL) categories[0] = fte;
				}
				else {
					if (categories[1] == NULL) categories[1] = fte;
				}
			}
			else {
				if (*fte->pte&PTE_D)
				{
					if (categories[2] == NULL) categories[2] = fte;
				}
				else {
					cur_fte_index++;
					if (cur_fte_index >= 16384) cur_fte_index = 0;
					return fte;
				}
			}
		}
		cur_fte_index++;
		if (cur_fte_index >= 16384) cur_fte_index = 0;
	}
	for (int i = 2; i >= 0; i--)
	{
		if (categories[i] != NULL) return categories[i];
	}
	return NULL;
}

void * vm_get_page(bool zero)
{
	void * page = palloc_get_page(PAL_USER | (zero ? PAL_ZERO : 0));
	if (page == NULL)
	{
		//evict
		struct fte * fte = find_page();
		ASSERT(fte != NULL);
		ASSERT(fte->pte != NULL);
		ASSERT((*fte->pte&PTE_ADDR) < PHYS_BASE);
		if (*fte->pte&PTE_D)
		{
			int swap_page = get_swap_page();
			//write out to sectors
			struct block * swap_block = block_get_role(BLOCK_SWAP);
			ASSERT(swap_block != NULL);
			for (int i = 0; i < 8; i++)
			{
				block_write(swap_block, swap_page * 8 + i, pte_get_page(*fte->pte) + i * 512);
			}
			//get spte and tell it where its data is
			struct spte to_find;
			to_find.pte = fte->pte;
			struct hash_elem * hash_elem = hash_find(fte->spt, &to_find.elem);
			struct spte * spte = hash_entry(hash_elem, struct spte, elem);
			ASSERT(hash_elem != NULL);
			ASSERT(spte->swap_index == -1);
			spte->swap_index = swap_page;
			ASSERT(spte->pte == fte->pte);
			//invalidate page
			*fte->pte = *fte->pte & (~PTE_P);
			ASSERT(!(*fte->pte & PTE_P));
			unsigned int * pte = fte->pte;
			fte->pte = NULL;
			if (zero) memset(pte_get_page(*pte), 0, 4096);
			return pte_get_page(*pte);
		} else {
			*fte->pte = *fte->pte & (~PTE_P);
			unsigned int * pte = fte->pte;
			fte->pte = NULL;
			if (zero) memset(pte_get_page(*pte), 0, 4096);
			return pte_get_page(*pte);
		}
	}
	//ASSERT(frame_table[fd_no(page)][ft_no(page)].pte == NULL);
	return page;
}

void vm_free_page(void * page)
{
	//free spte
	struct thread * t = thread_current();
	struct spte to_find;
	to_find.pte = lookup_page(t->pagedir, page, false);
	struct spte * spte = hash_entry(hash_find(&t->spt, &to_find.elem), struct spte, elem);
	hash_delete(&t->spt, &spte->elem);
	clear_frame(pte_get_page(*to_find.pte));
	free(spte);
	*to_find.pte = *to_find.pte & ~PTE_P;
	palloc_free_page(page);
}

bool vm_install_page(void * upage, struct file * file, unsigned int offset, unsigned int length, bool zero, bool writable)
{
	struct thread * t = thread_current();
	void * page = lookup_page(t->pagedir, upage, true);
	if (page != NULL)
	{
		//frame_table[fd_no(kpage)][ft_no(kpage)] = page;
		//allocate and set up spte
		struct spte * spte = malloc(sizeof(struct spte));
		spte->pte = page;
		spte->file = file;
		spte->offset = offset;
		spte->in_memory = false;
		spte->length = length;
		spte->zero = zero;
		spte->writable = writable;
		spte->swap_index = -1;
		if (NULL != hash_find(&t->spt, &spte->elem))
		{
			free(spte);
			return false;
		}
		hash_insert(&t->spt, &spte->elem);
		return true;
	}
	return false;
}

void vm_init()
{
	ASSERT(sizeof(struct fte) == 8);
	for (int i = 0; i < 32; i++)
	{
		frame_table[i] = NULL;
		/*frame_table[i] = palloc_get_page(0);
		for (int j = 0; j < 512; j++)
		{
			frame_table[i][j].pte = NULL; //no pte currently using
			frame_table[i][j].spt = NULL;
		}*/
	}
}

void register_frame(void * kpage, void * upage)
{
	void * pte = lookup_page(thread_current()->pagedir, upage, false);
	int fd = fd_no(kpage);
	if (frame_table[fd] == NULL)
	{
		frame_table[fd] = palloc_get_page(0);
		for (int i = 0; i < 512; i++)
		{
			frame_table[fd][i].pte = NULL;
			frame_table[fd][i].spt = NULL;
		}
	}
	ASSERT(pte != NULL);
	ASSERT(frame_table[fd][ft_no(kpage)].pte == NULL);
	frame_table[fd][ft_no(kpage)].pte = pte;
	frame_table[fd][ft_no(kpage)].spt = &thread_current()->spt;
}

void clear_frame(void *kpage)
{
	int fd = fd_no(kpage);
	ASSERT(frame_table[fd] != NULL);
	frame_table[fd][ft_no(kpage)].pte = NULL;
}
