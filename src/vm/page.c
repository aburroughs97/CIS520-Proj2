#include "vm/page.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/pte.h"
#include <stdbool.h>

static void ***frame_table;

int fd_no(void * addr)
{
	return ((unsigned int)addr & 0xFFC00000) >> (32 - 10);
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

}

void vm_free_page(void * page)
{
	palloc_free_page(page);
}

bool vm_install_page(void *page, void * addr)
{
	struct thread * t = thread_current();
	frame_table[fd_no(page)][ft_no(page)] = &((void**)t->pagedir)[pd_no(addr)][pt_no(addr)];
	return true;
}

void vm_init()
{
	frame_table = palloc_get_page(0);
	for (int i = 256; i < 1024; i++)
	{
		frame_table[i] = palloc_get_page(0);
		for (int j = 0; j < 1024; j++)
		{
			frame_table[i][j] = NULL; //no pte currently using
		}
	}
}