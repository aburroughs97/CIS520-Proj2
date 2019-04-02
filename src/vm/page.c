#include "vm/page.h"
#include "threads/palloc.h"
#include "userprog/pagedir.h"
#include "threads/thread.h"
#include "threads/pte.h"
#include <stdbool.h>
#include <hash.h>

static unsigned int **frame_table[16];

struct spte
{
	int diskaddr;
	bool in_memory;
	int *pte;
	struct hash_elem elem;
};

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
	return ((unsigned int)addr >> (32 - 10))&0x3FF;
}

int ft_no(void *addr)
{
	return ((unsigned int)addr >> (32 - 20))&0x3FF;
}

void * vm_get_page(bool zero)
{
	void * page = palloc_get_page(PAL_USER | (zero ? PAL_ZERO : 0));
	if (page == NULL)
	{
		//evict
		return NULL;
	}
	//allocate and set up spte
}

void vm_free_page(void * page)
{
	palloc_free_page(page);
	//free spte
}

bool vm_install_page(void *page, void * addr)
{
	struct thread * t = thread_current();
	frame_table[fd_no(page)][ft_no(page)] = lookup_page(t->pagedir,addr,false);
	return true;
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