#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

#define LISTFS_MAGIC 0x84837376

#define LISTFS_VERSION 0x0001

typedef unsigned char uint8;
typedef signed char int8;
typedef unsigned short uint16;
typedef signed short int16;
typedef unsigned int uint32;
typedef signed int int32;
typedef unsigned long long uint64;
typedef signed long long int64;

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

char *output_file_name = NULL;
char *boot_loader_file_name = NULL;
char *source_dir_name = NULL;
long block_size = 512;
long block_count = 0;
FILE *output_file;
listfs_header *fs_header;
char *disk_map;
long boot_loader_extra_blocks = 0;

bool get_commandline_options(int argc, char **argv) {
	int i;
	for (i = 1; i < argc; i++) {
		if (!strncmp(argv[i], "of=", 3)) {
			output_file_name = argv[i] + 3;
		} else if (!strncmp(argv[i], "bs=", 3)) {
			block_size = atoi(argv[i] + 3);
		} else if (!strncmp(argv[i], "size=", 5)) {
			block_count = atoi(argv[i] + 5);
		} else if (!strncmp(argv[i], "boot=", 5)) {
			boot_loader_file_name = argv[i] + 5;
		} else if (!strncmp(argv[i], "src=", 4)) {
			source_dir_name = argv[i] + 4;
		}
	}
	return (output_file_name && block_count);
}

void usage() {
	printf("Syntax:\n");
	printf("  make_listfs [options]\n");
	printf("Options:\n");
	printf("  of=XXX - output file name (required)\n");
	printf("  bs=XXX - size of disk image block in bytes (optional, default - 512)\n");
	printf("  size=XXX - size of disk image in blocks (required)\n");
	printf("  boot=XXX - name of boot loader image file (optional, need only for bootable image)\n");
	printf("  src=XXX - Dir, that contents will be copied on created disk image (optional)\n");
	printf("Author:\n");
	printf("  kiv (kiv.apple@gmail.com)\n");
	printf("\n");
}

bool check_options() {
	if ((block_size < 512) && !(block_size & 7)) {
		fprintf(stderr, "Error: Block size must be larger than 512 and be multiple of 8!\n");
		return false;
	}
	if (block_count < 2) {
		fprintf(stderr, "Error: Block count must be larger than 1!\n");
		return false;
	}
	return true;
}

bool open_output_file() {
	char *buf;
	output_file = fopen(output_file_name, "wb+");
	if (!output_file) {
		fprintf(stderr, "Error: Could not create output file: %s\n", strerror(errno));
		return false;
	}
	fseek(output_file, block_size * (block_count - 1), SEEK_SET);
	buf = calloc(1, block_size);
	if (fwrite(buf, block_size, 1, output_file) == 0) {
		fprintf(stderr, "Error: Could not create output file: %s\n", strerror(errno));
		fclose(output_file);
		remove(output_file_name);
		free(buf);
		return false;
	}
	free(buf);
	return true;
}

void close_output_file() {
	fclose(output_file);
}

void write_block(uint64 index, void *data, unsigned int count) {
	fseek(output_file, block_size * index, SEEK_SET);
	fwrite(data, block_size, count, output_file);
}

void init_listfs_header() {
	fs_header = calloc(block_size, 1); //this sector (512 bytes) of memory will contain the fs header (64 bytes) + bootloader data (the rest of space)
	if (boot_loader_file_name) {
		FILE *f;
		f = fopen(boot_loader_file_name, "rb");
		if (f) {
			fread(fs_header, block_size, 1, f);
			if (!feof(f)){
				char *buffer = calloc(block_size, 1);
				int i = 1;
				while (!feof(f)) {
					fread(buffer, block_size, 1, f);
					write_block(i, buffer, 1);
					i++;
					boot_loader_extra_blocks++; //we will use it to store the disk map
				}
			}
			fclose(f);
		} else
			fprintf(stderr, "Warning: Could not open boot loader file for reading. No boot loader will be installed.\n");
	}
	fs_header->magic = LISTFS_MAGIC;
	fs_header->version = LISTFS_VERSION;
	fs_header->base = 0;
	fs_header->size = block_count;
	fs_header->first_file = -1;
	fs_header->flags = 0;
	fs_header->block_size = block_size;
	fs_header->uid = (time(NULL) << 16) | (rand() & 0xFFFF);
}

void store_listfs_header() {
	write_block(0, fs_header, 1);
	free(fs_header);
}

uint64 alloc_disk_block() {
	uint64 index = 0;
	while (disk_map[index >> 3] & (1 << (index & 7))) {
		index++;
		if (index >= fs_header->size) {
			fprintf(stderr, "Error: Disk image is full. Could not write more.\n");
			exit(-3);
		}
	}
	disk_map[index >> 3] |= 1 << (index & 7);
	return index;
}

void get_disk_block(uint64 index) {
	disk_map[index >> 3] |= 1 << (index & 7);
}

void init_disk_map() {
	int i;
	fs_header->map_base = /* 1 + */ boot_loader_extra_blocks;
	fs_header->map_size = block_count / 8;
	if (fs_header->map_size % block_size)
		fs_header->map_size += block_size;
	fs_header->map_size /= block_size;
	disk_map = calloc(block_size, fs_header->map_size);
	for (i = 0; i < fs_header->map_base + fs_header->map_size; i++) {
		get_disk_block(i);
	}
}

void store_disk_map() {
	write_block(fs_header->map_base, disk_map, fs_header->map_size);
	free(disk_map);
}

uint64 store_file_data(FILE *file) {
	if (feof(file)) 
		return -1;
	uint64 *block_list = malloc(block_size);
	uint64 block_list_index = alloc_disk_block();
	char *block_data = malloc(block_size);
	int i;
	for (i = 0; i < (block_size / 8 - 1); i++) {
		if (!feof(file)) {
			memset(block_data, 0, block_size);
			if (fread(block_data, 1, block_size, file) == 0) {
                		block_list[i] = -1;
                		continue;
			}
			uint64 block_index = alloc_disk_block();
			block_list[i] = block_index;
			write_block(block_index, block_data, 1);
		} else {
			block_list[i] = -1;
		}
	}
	block_list[block_size / 8 - 1] = store_file_data(file);
	write_block(block_list_index, block_list, 1);
	free(block_list);
	free(block_data);
	return block_list_index;
}

uint64 process_dir(uint64 parent, char *dir_name) {
	DIR *dir = opendir(dir_name);
	struct dirent *entry;
	uint64 prev_file = -1;
	uint64 cur_file;
	uint64 first_file = -1;
	char file_name[260];
	struct stat file_stat;
	listfs_file_header *file_header;
	if (!dir) {
		fprintf(stderr, "Warning: Could not read contents of directory '%s': %s\n", dir_name, strerror(errno));
		return -1;
	}
	file_header = malloc(block_size);
	while (entry = readdir(dir)) {
		if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) 
			continue;
		cur_file = alloc_disk_block();
		if (prev_file != -1) {
			file_header->next = cur_file;
			write_block(prev_file, file_header, 1);
		} else
			first_file = cur_file;
		memset(file_header, 0, block_size);
		file_header->prev = prev_file;
		file_header->next = -1;
		strncpy(file_header->name, entry->d_name, sizeof(file_header->name) - 1);
		file_header->name[sizeof(file_header->name) - 1] = 0;
		file_header->size = 0;
		sprintf((char*)&file_name, "%s/%s", dir_name, entry->d_name);
		stat(file_name, &file_stat);
		file_header->parent = parent;
		file_header->flags = 0;
		file_header->create_time = file_stat.st_ctime;
		file_header->modify_time = file_stat.st_mtime;
		file_header->access_time = file_stat.st_atime;
		if (S_ISDIR(file_stat.st_mode)) {
			file_header->flags = 1;
			file_header->data = process_dir(cur_file, file_name);
		} else if (S_ISREG(file_stat.st_mode)) {
			FILE *f = fopen(file_name, "rb");
			if (f) {
				file_header->size = file_stat.st_size;
				file_header->data = store_file_data(f);
				fclose(f);
			} else {
				fprintf(stderr, "Warning: Could not open file '%s' for reading!\n", file_name);
				file_header->data = -1;
			}
		} else {
			fprintf(stderr, "Warning: File '%s' is not directory or regular file.\n", file_name);
		}
		prev_file = cur_file;
	}
	write_block(prev_file, file_header, 1);
	free(file_header);
	closedir(dir);
	return first_file;
}

int main(int argc, char *argv[]) {
	if (!get_commandline_options(argc, argv)) {
		usage();
		return 0;
	}
	if (!check_options()) 
		return -1;
	if (!open_output_file()) 
		return -2;
	init_listfs_header();
	init_disk_map();
	if (source_dir_name) 
		fs_header->first_file = process_dir(-1, source_dir_name);
	store_disk_map();
	store_listfs_header();
	close_output_file();
	return 0;
}