#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include "stdlib.h"

#define PAGE_SIZE 0x1000
#define PAGE_OFFSET_BITS 12
#define PAGE_OFFSET_MASK 0xFFF
#define PAGE_TABLE_INDEX_BITS 10
#define PAGE_TABLE_INDEX_MASK 0x3FF

#define PHYADDR_BITS 32

#define PAGE_PRESENT		(1 << 0)
#define PAGE_WRITABLE		(1 << 1)
#define PAGE_USER		    (1 << 2)
#define PAGE_WRITE_THROUGH	(1 << 3)
#define PAGE_CACHE_DISABLED	(1 << 4)
#define PAGE_ACCESSED		(1 << 5)

#define PAGE_MODIFIED		(1 << 6)
#define PAGE_GLOBAL		    (1 << 8)

void KERNEL_BASE();
void KERNEL_CODE_BASE();
void KERNEL_DATA_BASE();
void KERNEL_BSS_BASE();
void KERNEL_END();

#define KERNEL_PAGE_TABLE ((void*)0xFFFFE000)
#define TEMP_PAGE ((void*)0xFFFFF000)
#define TEMP_PAGE_INFO ((size_t)KERNEL_PAGE_TABLE + (((size_t)TEMP_PAGE >> PAGE_OFFSET_BITS) & PAGE_TABLE_INDEX_MASK) * sizeof(phyaddr))

#define USER_MEMORY_START ((void*)0)
#define USER_MEMORY_END ((void*)0x7FFFFFFF)
#define KERNEL_MEMORY_START ((void*)0x80000000)
#define KERNEL_MEMORY_END ((void*)(KERNEL_BASE - 1))

typedef size_t phyaddr;

typedef enum {
	VMB_RESERVED,
	VMB_MEMORY,
	VMB_IO_MEMORY
} VirtMemoryBlockType;

typedef struct {
	VirtMemoryBlockType type;
	void *base;
	size_t length;
} VirtMemoryBlock;

typedef struct {
	phyaddr page_dir;
	void *start;
	void *end;
	size_t block_table_size;
	size_t block_count;
	VirtMemoryBlock *blocks;
} AddressSpace;

phyaddr kernel_page_dir;
size_t memory_size;
AddressSpace kernel_address_space;

void init_memory_manager(void *memory_map);

void temp_map_page(phyaddr addr);
bool map_pages(phyaddr page_dir, void *vaddr, phyaddr paddr, size_t count, unsigned int flags);
phyaddr get_page_info(phyaddr page_dir, void *vaddr);

size_t get_free_memory_size();
phyaddr alloc_phys_pages(size_t count);
void free_phys_pages(phyaddr base, size_t count);

void *alloc_virt_pages(AddressSpace *address_space, void *vaddr, phyaddr paddr, size_t count, unsigned int flags);
bool free_virt_pages(AddressSpace *address_space, void *vaddr, unsigned int flags);

#endif