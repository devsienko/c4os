#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include "stdlib.h"

#define PAGE_SIZE 0x1000
#define PAGE_OFFSET_BITS 12
#define PAGE_OFFSET_MASK 0xFFF
#define PAGE_TABLE_INDEX_BITS 10
#define PAGE_TABLE_INDEX_MASK 0x3FF

#define PHYADDR_BITS 32

#define PAGE_VALID 1
#define PAGE_WRITABLE 2
#define PAGE_USER 4

typedef size_t phyaddr;

#define KERNEL_BASE 0xFFC00000
#define KERNEL_PAGE_TABLE 0xFFFFE000
#define TEMP_PAGE 0xFFFFF000
#define TEMP_PAGE_INFO (KERNEL_PAGE_TABLE + ((TEMP_PAGE >> PAGE_OFFSET_BITS) & PAGE_TABLE_INDEX_MASK) * sizeof(phyaddr))

phyaddr kernel_page_dir;
size_t memory_size;

void init_memory_manager(void *memory_map);

void temp_map_page(phyaddr addr);
bool map_pages(phyaddr page_dir, void *vaddr, phyaddr paddr, size_t count, unsigned int flags);
phyaddr get_page_info(phyaddr page_dir, void *vaddr);

size_t get_free_memory_size();
phyaddr alloc_phys_pages(size_t count);
void free_phys_pages(phyaddr base, size_t count);

#endif