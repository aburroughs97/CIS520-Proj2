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

static unsigned int **frame_table[16];

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
	return ((unsigned int)vtop(addr) >> (32 - 10))&0x3FF;
}

int ft_no(void *addr)
{
	return ((unsigned int)vtop(addr) >> (32 - 20))&0x3FF;
}

struct swap_page
{
	bool taken;
};

struct swap_page * swap_table = NULL;

int get_swap_page()
{
	struct block * swap_block = block_get_role(BLOCK_SWAP);
	unsigned int swap_size = block_size(swap_block);
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
	return swap_page;
}

static unsigned int cur_fte_index = 0;

void * find_page()
{
	unsigned int * categories[3] = { NULL };
	int searched_pages = 0;
	while (searched_pages < 256)
	{
		unsigned int * pte = frame_table[cur_fte_index >> 10][cur_fte_index & 0x3FF];
		if (pte != NULL)
		{
			searched_pages++;
			if (*pte&PTE_A)
			{
				if (*pte&PTE_D)
				{
					if (categories[0] == NULL) categories[0] = pte;
				}
				else {
					if (categories[1] == NULL) categories[1] = pte;
				}
			}
			else {
				if (*pte&PTE_D)
				{
					if (categories[2] == NULL) categories[2] = pte;
				}
				else {
					return pte;
				}
			}
		}
		cur_fte_index++;
		if (cur_fte_index == 8192) cur_fte_index = 0;
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
		unsigned int * pte = find_page();
		if (*pte&PTE_D)
		{

		} else {
			*pte = *pte & (~PTE_P);
			return pte_get_page(*pte);
		}
		return NULL;
	}

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
	free(spte);
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
	for (int i = 0; i < 16; i++)
	{
		frame_table[i] = palloc_get_page(0);
		for (int j = 0; j < 1024; j++)
		{
			frame_table[i][j] = NULL; //no pte currently using
		}
	}
}

void register_frame(void * kpage, void * upage)
{
	void * pte = lookup_page(thread_current()->pagedir, upage, false);
	frame_table[fd_no(kpage)][ft_no(kpage)] = pte;
}

void clear_frame(void *kpage)
{
	frame_table[fd_no(kpage)][ft_no(kpage)] = NULL;
}
