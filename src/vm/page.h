#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <stdbool.h>
#include <hash.h>

void * vm_get_page(bool zero);
void vm_free_page(void * page);
bool vm_install_page(void *kpage, void * upage);
void vm_init();
unsigned int spte_hash_func(const struct hash_elem *e, void *aux);
bool spte_hash_less(const struct hash_elem *a, const struct hash_elem *b, void *aux);

#endif
