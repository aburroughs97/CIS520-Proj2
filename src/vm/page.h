#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <stdbool.h>
#include <hash.h>
#include "filesys/file.h"

struct spte
{
	struct file * file;
	unsigned int offset;
	bool in_memory;
	bool zero;
	unsigned int length;
	int *pte;
	struct hash_elem elem;
	bool writable;
	int swap_index;
};

void register_frame(void * kpage, void * upage);
void * vm_get_page(bool zero);
void vm_free_page(void * page);
bool vm_install_page(void * upage, struct file * file, unsigned int offset, unsigned int length, bool zero, bool writable);
void vm_init();
unsigned int spte_hash_func(const struct hash_elem *e, void *aux);
bool spte_hash_less(const struct hash_elem *a, const struct hash_elem *b, void *aux);
void clear_frame(void *kpage);
void free_swap_page(int page);

#endif
