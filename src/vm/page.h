#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <stdbool.h>

void * vm_get_page(bool zero);
void vm_free_page(void * page);
bool vm_install_page(void *page, void * addr);
void vm_init();

#endif
