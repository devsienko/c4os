#include "stdlib.h"
#include "memory_manager.h"

typedef struct {
	uint64 base;
	uint64 length;
	uint32 type;
	uint32 acpi_ext_attrs;
} __attribute__((packed)) MemoryMapEntry;

size_t free_page_count = 0;
phyaddr free_phys_memory_pointer = 0;

void init_memory_manager(void *memory_map) {
	asm("movl %%cr3, %0":"=a"(kernel_page_dir));
	memory_size = 0x100000;
	MemoryMapEntry *entry;
	for (entry = memory_map; entry->type; entry++) {
		if ((entry->type == 1) && (entry->base >= 0x100000)) {
			free_phys_pages(entry->base, entry->length >> PAGE_OFFSET_BITS);
			memory_size += entry->length;
		}
	}
}

static inline void flush_page_cache(void *addr) {
	asm("invlpg (,%0,)"::"a"(addr));
} 

void temp_map_page(phyaddr addr) {
	*((phyaddr*)TEMP_PAGE_INFO) = (addr & ~PAGE_OFFSET_MASK) | PAGE_VALID | PAGE_WRITABLE;
	flush_page_cache((void*)TEMP_PAGE);
}

bool map_pages(phyaddr page_dir, void *vaddr, phyaddr paddr, size_t count, unsigned int flags) {
	for (; count; count--) {
		phyaddr page_table = page_dir;
		char shift;
		for (shift = PHYADDR_BITS - PAGE_TABLE_INDEX_BITS; shift >= PAGE_OFFSET_BITS; shift -= PAGE_TABLE_INDEX_BITS) {
			unsigned int index = ((size_t)vaddr >> shift) & PAGE_TABLE_INDEX_MASK;
			temp_map_page(page_table);
			if (shift > PAGE_OFFSET_BITS) {
				page_table = ((phyaddr*)TEMP_PAGE)[index];
				if (!(page_table & PAGE_VALID)) {
					phyaddr addr = alloc_phys_pages(1);
					if (addr) {
						temp_map_page(paddr);
						memset((void*)TEMP_PAGE, 0, PAGE_SIZE);
						temp_map_page(page_table);
						((phyaddr*)TEMP_PAGE)[index] = addr | PAGE_VALID | PAGE_WRITABLE | PAGE_USER;
						page_table = addr;
					} else {
						return false;
					}
				}
			} else {
				((phyaddr*)TEMP_PAGE)[index] = (paddr & ~PAGE_OFFSET_BITS) | flags;
				asm("invlpg (,%0,)"::"a"(vaddr));
			}
		}
		vaddr += PAGE_SIZE;
		paddr += PAGE_SIZE;
	}
	return true;
}

unsigned int get_page_directory_index (void *vaddr) {
	char page_directory_shift = 22;
	unsigned int result = ((size_t)vaddr >> page_directory_shift) & PAGE_TABLE_INDEX_MASK;
	return result;
}

unsigned int get_page_table_index (void * vaddr) {
	char page_table_shift = 12;
	unsigned int result = ((size_t)vaddr >> page_table_shift) & PAGE_TABLE_INDEX_MASK;
	return result;
}

phyaddr get_page_info(phyaddr page_dir, void *vaddr) {
	unsigned int page_directory_index = get_page_directory_index(vaddr);
	temp_map_page(page_dir);
	phyaddr pde = ((phyaddr*)TEMP_PAGE)[page_directory_index];
	if (!(pde & PAGE_VALID)) {
		return 0;
	}

	unsigned int page_table_index = get_page_table_index(vaddr);
	phyaddr page_table_base =  pde & ~PAGE_OFFSET_MASK;
	temp_map_page(page_table_base);
	phyaddr pte = ((phyaddr*)TEMP_PAGE)[page_table_index];
	return pte;
}

size_t get_free_memory_size() {
	return free_page_count << PAGE_OFFSET_BITS;
}

phyaddr alloc_phys_pages(size_t count) {
	return 0;
}

void free_phys_pages(phyaddr base, size_t count) {
	
}