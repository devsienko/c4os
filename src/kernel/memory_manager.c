#include "stdlib.h"
#include "memory_manager.h"

typedef struct {
	uint64 base;
	uint64 length;
	uint32 type;
	uint32 acpi_ext_attrs;
} __attribute__((packed)) MemoryMapEntry;

typedef struct {
	phyaddr next;
	phyaddr prev;
	size_t size;
} PhysMemoryBlock;

size_t free_page_count = 0;
phyaddr free_phys_memory_pointer = -1;

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
					if (addr != -1) {
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
	if (free_page_count < count) return -1;
	phyaddr result = -1;
	if (free_phys_memory_pointer != -1) {
		phyaddr cur_block = free_phys_memory_pointer;
		do {
			temp_map_page(cur_block);
			if (((PhysMemoryBlock*)TEMP_PAGE)->size == count) {
				phyaddr next = ((PhysMemoryBlock*)TEMP_PAGE)->next;
				phyaddr prev = ((PhysMemoryBlock*)TEMP_PAGE)->prev;
				temp_map_page(next);
				((PhysMemoryBlock*)TEMP_PAGE)->prev = prev;
				temp_map_page(prev);
				((PhysMemoryBlock*)TEMP_PAGE)->next = next;
				if (cur_block == free_phys_memory_pointer) {
					free_phys_memory_pointer = next;
					if (cur_block == free_phys_memory_pointer) {
						free_phys_memory_pointer = -1;
					}
				}
				result = cur_block;
				break;
			} else if (((PhysMemoryBlock*)TEMP_PAGE)->size > count) {
				((PhysMemoryBlock*)TEMP_PAGE)->size -= count;
				result = cur_block + (((PhysMemoryBlock*)TEMP_PAGE)->size << PAGE_OFFSET_BITS);
				break;
			}
			cur_block = ((PhysMemoryBlock*)TEMP_PAGE)->next;

		} while (cur_block != free_phys_memory_pointer);
		if (result != -1) {
			free_page_count -= count;
		} 
	}
	return result;
} 

void free_phys_pages(phyaddr base, size_t count) {
	if (free_phys_memory_pointer == -1) {
		temp_map_page(base);
		((PhysMemoryBlock*)TEMP_PAGE)->next = base;
		((PhysMemoryBlock*)TEMP_PAGE)->prev = base;
		((PhysMemoryBlock*)TEMP_PAGE)->size = count;
		free_phys_memory_pointer = base;
	} else {
		phyaddr cur_block = free_phys_memory_pointer;
		do {
			temp_map_page(cur_block);
			if (cur_block + (((PhysMemoryBlock*)TEMP_PAGE)->size << PAGE_OFFSET_BITS) == base) {
				((PhysMemoryBlock*)TEMP_PAGE)->size += count;
				if (((PhysMemoryBlock*)TEMP_PAGE)->next == base + (count << PAGE_OFFSET_BITS)) {
					phyaddr next1 = ((PhysMemoryBlock*)TEMP_PAGE)->next;
					temp_map_page(next1);
					phyaddr next2 = ((PhysMemoryBlock*)TEMP_PAGE)->next;
					size_t new_count = ((PhysMemoryBlock*)TEMP_PAGE)->size;
					temp_map_page(next2);
					((PhysMemoryBlock*)TEMP_PAGE)->prev = cur_block;
					temp_map_page(cur_block);
					((PhysMemoryBlock*)TEMP_PAGE)->next = next2;
					((PhysMemoryBlock*)TEMP_PAGE)->size += new_count;
				}
				break;
			} else if (base + (count << PAGE_OFFSET_BITS) == cur_block) {
				size_t old_count = ((PhysMemoryBlock*)TEMP_PAGE)->size;
				phyaddr next = ((PhysMemoryBlock*)TEMP_PAGE)->next;
				phyaddr prev = ((PhysMemoryBlock*)TEMP_PAGE)->prev;
				temp_map_page(next);
				((PhysMemoryBlock*)TEMP_PAGE)->prev = base;
				temp_map_page(prev);
				((PhysMemoryBlock*)TEMP_PAGE)->next = base;
				temp_map_page(base);
				((PhysMemoryBlock*)TEMP_PAGE)->next = next;
				((PhysMemoryBlock*)TEMP_PAGE)->prev = prev;
				((PhysMemoryBlock*)TEMP_PAGE)->size = count + old_count;
				break;} else if ((cur_block > base) || (((PhysMemoryBlock*)TEMP_PAGE)->next == free_phys_memory_pointer)) {
				phyaddr prev = ((PhysMemoryBlock*)TEMP_PAGE)->next;
				((PhysMemoryBlock*)TEMP_PAGE)->prev = base;
				temp_map_page(prev);
				((PhysMemoryBlock*)TEMP_PAGE)->next = base;
				temp_map_page(base);
				((PhysMemoryBlock*)TEMP_PAGE)->next = cur_block;
				((PhysMemoryBlock*)TEMP_PAGE)->prev = prev;
				((PhysMemoryBlock*)TEMP_PAGE)->size = count;
				break;
			}
			cur_block = ((PhysMemoryBlock*)TEMP_PAGE)->next;
		} while (cur_block != free_phys_memory_pointer);
		if (base < free_phys_memory_pointer) {
			free_phys_memory_pointer = base;
		}

	}
	free_page_count += count;
}