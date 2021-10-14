#ifndef LISTFS_H
# define LISTFS_H

#include <stdlib.h>
#include <memory_manager.h>

typedef struct {
	char jump[4];
	uint32 magic;
	uint32 version;
	uint32 flags;
	uint64 base;
	uint64 size;
	uint64 map_base;
	uint64 map_size;
	uint64 first_file;
	uint64 uid;
	uint32 block_size;
} __attribute((packed)) listfs_header;

typedef struct {
	char name[256];
	uint64 next;
	uint64 prev;
	uint64 parent;
	uint64 flags;
	uint64 data;
	uint64 size;
	uint64 create_time;
	uint64 modify_time;
	uint64 access_time;
} __attribute((packed)) listfs_file_header;

#define LISTFS_COMMON_INDICATOR -1
#define LISTFS_EMPTY 0

listfs_file_header* get_file_info_by(uint32 sector_number);

#endif