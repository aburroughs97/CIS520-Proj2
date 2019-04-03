#include "vm/page.h"
#include "threads/palloc.h"
#include "userprog/pagedir.h"
#include "threads/thread.h"
#include "threads/pte.h"
#include <stdbool.h>
#include <hash.h>
#include "threads/vaddr.h"

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

void * vm_get_page(bool zero)
{
	void * page = palloc_get_page(PAL_USER | (zero ? PAL_ZERO : 0));
	if (page == NULL)
	{
		//evict
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

bool vm_install_page(void * upage, struct file * file, unsigned int offset, unsigned int length, bool zero)
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
		int size = hash_size(&t->spt);
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