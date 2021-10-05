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
Mutex phys_memory_mutex;

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
	
	map_pages(kernel_page_dir, KERNEL_CODE_BASE, get_page_info(kernel_page_dir, KERNEL_CODE_BASE),
		((size_t)KERNEL_DATA_BASE - (size_t)KERNEL_CODE_BASE) >> PAGE_OFFSET_BITS, PAGE_PRESENT | PAGE_GLOBAL | PAGE_USER);
	map_pages(kernel_page_dir, KERNEL_DATA_BASE, get_page_info(kernel_page_dir, KERNEL_DATA_BASE),
		((size_t)KERNEL_END - (size_t)KERNEL_DATA_BASE) >> PAGE_OFFSET_BITS, PAGE_PRESENT | PAGE_WRITABLE | PAGE_GLOBAL | PAGE_USER);
	map_pages(kernel_page_dir, KERNEL_PAGE_TABLE, get_page_info(kernel_page_dir, KERNEL_PAGE_TABLE), 1, PAGE_PRESENT | PAGE_WRITABLE | PAGE_GLOBAL);

	kernel_address_space.page_dir = kernel_page_dir;
	kernel_address_space.start = KERNEL_MEMORY_START;
	kernel_address_space.end = KERNEL_MEMORY_END;
	
	kernel_address_space.block_table_size = PAGE_SIZE / sizeof(VirtMemoryBlock);
	kernel_address_space.blocks = KERNEL_MEMORY_START;
	kernel_address_space.block_count = 1;
	
	map_pages(kernel_page_dir, kernel_address_space.blocks, alloc_phys_pages(1), 1, PAGE_PRESENT | PAGE_WRITABLE);
	
	kernel_address_space.blocks[0].type = VMB_RESERVED;
	kernel_address_space.blocks[0].base = kernel_address_space.blocks;
	kernel_address_space.blocks[0].length = PAGE_SIZE;
}

static inline void flush_page_cache(void *addr) {
	asm("invlpg (,%0,)"::"a"(addr));
} 

void temp_map_page(phyaddr addr) {
	*((phyaddr*)TEMP_PAGE_INFO) = (addr & ~PAGE_OFFSET_MASK) | PAGE_PRESENT | PAGE_WRITABLE;
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
				size_t prev_page_table = page_table;
				page_table = ((phyaddr*)TEMP_PAGE)[index];
				if (!(page_table & PAGE_PRESENT)) {
					phyaddr addr = alloc_phys_pages(1);
					if (addr != -1) {
						temp_map_page(addr);
						memset((void*)TEMP_PAGE, 0, PAGE_SIZE);
						temp_map_page(prev_page_table);
						((phyaddr*)TEMP_PAGE)[index] = addr | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
						page_table = addr;
					} else {
						return false;
					}
				}
			} else {
				((phyaddr*)TEMP_PAGE)[index] = (paddr & ~PAGE_OFFSET_BITS) | flags;
				flush_page_cache(vaddr);
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
	if (!(pde & PAGE_PRESENT)) {
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

// count - count of pages
phyaddr alloc_phys_pages(size_t count) {
	if (free_page_count < count) 
		return -1;

	phyaddr result = -1;
	mutex_get(&phys_memory_mutex, true);
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
	mutex_release(&phys_memory_mutex);
	return result;
} 

void free_phys_pages(phyaddr base, size_t count) {
	mutex_get(&phys_memory_mutex, true);

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
				break;
			} else if ((cur_block > base) || (((PhysMemoryBlock*)TEMP_PAGE)->next == free_phys_memory_pointer)) {
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

	mutex_release(&phys_memory_mutex);
}

/* Highlevel virtual memory manager */

static inline bool is_blocks_overlapped(void *base1, size_t size1, void *base2, size_t size2) {
	return ((base1 >= base2) && (base1 < base2 + size2)) || ((base2 >= base1) && (base2 < base1 + size1));
}

void *alloc_virt_pages(AddressSpace *address_space, void *vaddr, phyaddr paddr, size_t count, unsigned int flags) {
	VirtMemoryBlockType type = VMB_IO_MEMORY;
	size_t i;
	
	mutex_get(&(address_space->mutex), true);

	if (vaddr == NULL) {
		vaddr = address_space->end - (count << PAGE_OFFSET_BITS) + 1;
		for (i = 0; i < address_space->block_count; i++) {
			if (is_blocks_overlapped(address_space->blocks[i].base, address_space->blocks[i].length, vaddr, count)) {
				vaddr = address_space->blocks[i].base - (count << PAGE_OFFSET_BITS);
			} else {
				break;
			}
		}
	} else {
		if ((vaddr >= address_space->start) && (vaddr + (count << PAGE_OFFSET_BITS) < address_space->end)) {
			for (i = 0; i < address_space->block_count; i++) {
				if (is_blocks_overlapped(address_space->blocks[i].base, address_space->blocks[i].length, vaddr, count << PAGE_OFFSET_BITS)) {
					vaddr = NULL;
					break;
				} else if (address_space->blocks[i].base < vaddr) {
					break;
				}
			}
		} else {
			vaddr = NULL;
		}
	}
	if (vaddr != NULL) {
		if (paddr == -1) {
			paddr = alloc_phys_pages(count);
			type = VMB_MEMORY;
		}
		if (paddr != -1) {
			if (map_pages(address_space->page_dir, vaddr, paddr, count, flags)) {
				if (address_space->block_count == address_space->block_table_size) {
					size_t new_size = address_space->block_table_size * sizeof(VirtMemoryBlock);
					new_size = (new_size + PAGE_OFFSET_MASK) & ~PAGE_OFFSET_MASK;
					new_size += PAGE_SIZE;
					new_size = new_size >> PAGE_OFFSET_BITS;
					if (&kernel_address_space != address_space) {
						VirtMemoryBlock *new_table = alloc_virt_pages(&kernel_address_space, NULL, -1, new_size,	
							PAGE_PRESENT | PAGE_WRITABLE);
						if (new_table) {
							memcpy(new_table, address_space->blocks, address_space->block_table_size * sizeof(VirtMemoryBlock));
							free_virt_pages(&kernel_address_space, address_space->blocks, 0);
							address_space->blocks = new_table;
						} else {
							goto fail;
						}
					} else {
						phyaddr new_page = alloc_phys_pages(1);
						if (new_page == -1) {
							goto fail;
						} else {
							VirtMemoryBlock *main_block = &(address_space->blocks[address_space->block_count - 1]);
							if (map_pages(address_space->page_dir, main_block->base + main_block->length, new_page, 1,
									PAGE_PRESENT | PAGE_WRITABLE)) {
								main_block->length += PAGE_SIZE;
							} else {
								free_phys_pages(new_page, 1);
							}
						}
					}
					address_space->block_table_size = (new_size << PAGE_OFFSET_BITS) / sizeof(VirtMemoryBlock);
				}
				memcpy(address_space->blocks + i + 1, address_space->blocks + i,
					(address_space->block_count - i) * sizeof(VirtMemoryBlock));
				address_space->block_count++;
				address_space->blocks[i].type = type;
				address_space->blocks[i].base = vaddr;
				address_space->blocks[i].length = count << PAGE_OFFSET_BITS;
			} else {
fail:
				map_pages(address_space->page_dir, vaddr, 0, count, 0);
				free_phys_pages(paddr, count);
				vaddr = NULL;
			}
		}
	}

	mutex_release(&(address_space->mutex));

	return vaddr;
}

bool free_virt_pages(AddressSpace *address_space, void *vaddr, unsigned int flags) {
	size_t i;

	mutex_get(&(address_space->mutex), true);

	for (i = 0; i < address_space->block_count; i++) {
		if ((address_space->blocks[i].base <= vaddr) && (address_space->blocks[i].base + address_space->blocks[i].length > vaddr)) {
			break;
		}
	}
	if (i < address_space->block_count) {
		if (address_space->blocks[i].type = VMB_MEMORY) {
			free_phys_pages(get_page_info(address_space->page_dir, vaddr) & ~PAGE_OFFSET_MASK, address_space->blocks[i].length >> PAGE_OFFSET_BITS);
		}
		address_space->block_count--;
		memcpy(address_space->blocks + i, address_space->blocks + i + 1, (address_space->block_count - i) * sizeof(VirtMemoryBlock));
		
		mutex_release(&(address_space->mutex));
		
		return true;
	} else {
		
		mutex_release(&(address_space->mutex));

		return false;
	}
}