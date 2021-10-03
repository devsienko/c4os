#ifndef STDLIB_H
#define STDLIB_H

typedef enum {
	false = 0,
	true = 1
} bool;

#define NULL ((void*)0)

typedef unsigned char uint8;
typedef signed char int8;

typedef unsigned short uint16;
typedef signed short int16;

typedef unsigned int uint32;
typedef signed int int32;

typedef unsigned long long uint64;
typedef signed long long int64;

#ifdef __x86_64__
	typedef uint64 size_t;
#else
	typedef uint32 size_t;
#endif

typedef bool Mutex;

typedef struct _ListItem ListItem;

typedef struct {
	ListItem *first;
	size_t count;
	Mutex mutex;
} List;

struct _ListItem {
	ListItem *next;
	ListItem *prev;
	List *list;
};

bool mutex_get(Mutex *mutex, bool wait);
void mutex_release(Mutex *mutex);

#define min(a, b) (((a) > (b)) ? (b) : (a))
#define max(a, b) (((a) > (b)) ? (a) : (b))

#define outportb(port, value) asm("outb %b0, %w1"::"a"(value),"d"(port));
#define outportw(port, value) asm("outw %w0, %w1"::"a"(value),"d"(port));
#define outportl(port, value) asm("outl %0, %w1"::"a"(value),"d"(port));

#define inportb(port, out_value) asm("inb %w1, %b0":"=a"(out_value):"d"(port));
#define inportw(port, out_value) asm("inw %w1, %w0":"=a"(out_value):"d"(port));
#define inportl(port, out_value) asm("inl %w1, %0":"=a"(out_value):"d"(port));

void memset(void *mem, char value, size_t count);
void memset_word(void *mem, uint16 value, size_t count);
void memcpy(void *dest, void *src, size_t count);
int memcmp(void *mem1, void *mem2, size_t count);
void *memchr(void *mem, char value, size_t count);

size_t strlen(char *str);
void strcpy(char *dest, char *src);
void strncpy(char *dest, char*src, size_t max_count);
int strcmp(char *str1, char *str2);
char *strchr(char *str, char value);

int atoi(const char *s);
int isspace(int c);
int isdigit(int c);

struct memory_map_entry{
    unsigned long long base;
    unsigned long long length;
    unsigned long type;
    unsigned long acpi_attrs;
}; 

void list_init(List *list);
void list_append(List *list, ListItem *item);
void list_remove(ListItem *item);

#endif 