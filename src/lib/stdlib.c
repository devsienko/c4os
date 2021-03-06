#include "stdlib.h"

void memset(void *mem, char value, size_t count) {
	asm("movl %0, %%eax \n movl %1, %%edi \n movl %2, %%ecx \n rep stosl"
		::"a"((uint32)value | ((uint32)value << 8) | ((uint32)value << 16) | ((uint32)value << 24)),"b"(mem),"c"(count >> 2));
	asm("movb %b0, %%al \n movl %1, %%ecx \n rep stosb"::"a"(value),"b"(count & 3));
}

void memset_word(void *mem, uint16 value, size_t count) {
	asm("movl %0, %%eax \n movl %1, %%edi \n movl %2, %%ecx \n rep stosl"
		::"a"((uint32)value | ((uint32)value << 16)),"b"(mem),"c"(count >> 1));
}

void *memcpy (void *dest, void *src, size_t len) {
  char *d = dest;
  const char *s = src;
  while (len--)
    *d++ = *s++;
  return dest;
}

int memcmp(void *mem1, void *mem2, size_t count) {
	char above, below;
	asm("movl %0, %%esi \n movl %1, %%edi \n movl %2, %%ecx \n repe cmpsb"::"a"(mem1),"b"(mem2),"c"(count));
	asm("seta %0 \n setb %1":"=a"(above),"=b"(below));
	return above - below;
}

void *memchr(void *mem, char value, size_t count) {
	void *result;
	asm("movb %b0, %%al \n movl %1, %%edi \n movl %2, %%ecx \n repe cmpsb"::"a"(value),"b"(mem),"c"(count));
	asm("movl %%edi, %0":"=a"(result));
	if (result < mem + count) {
		return result;
	} else {
		return NULL;
	}
}

size_t strlen(char *str) {
	return (char*)memchr(str, '\0', -1) - str;
}

void strcpy(char *dest, char *src) {
	memcpy(dest, src, strlen(src) + 1);
}

void strncpy(char *dest, char *src, size_t max_count) {
	size_t len = min(max_count - 1, strlen(src));
	memcpy(dest, src, len);
	dest[len] = '\0';
}

int strcmp(char *str1, char *str2) {
  	char c1, c2;
  	do {
      	c1 = *str1++;
      	c2 = *str2++;
      	if (c1 == '\0')
        	return c1 - c2;
    }
  	while (c1 == c2);
  	return c1 - c2;
}

char *strchr(char *str, char value) {
	return memchr(str, value, strlen(str));
}

bool mutex_get(Mutex *mutex, bool wait) {
	bool old_value = true;
	do {
		asm("xchg (,%1,), %0":"=a"(old_value):"b"(mutex),"a"(old_value));
	} while (old_value && wait);
	return !old_value;
}

void mutex_release(Mutex *mutex) {
	*mutex = false;
}

int atoi(const char *s)
{
	int n=0, neg=0;
	while (isspace(*s)) s++;
	switch (*s) {
	case '-': neg=1;
	case '+': s++;
	}
	/* Compute n as a negative number to avoid overflow on INT_MIN */
	while (isdigit(*s))
		n = 10*n - (*s++ - '0');
	return neg ? n : -n;
}

int isspace(int c)
{
	return ((c == '\t') || (c == '\n') ||
	    (c == '\v') || (c == '\f') || (c == '\r') || (c == ' '));
}

int isdigit(int c)
{
  	return ((c >= '0') && (c <= '9'));
}

void list_init(List *list) {
	list->first = NULL;
	list->count = 0;
	list->mutex = false;
}

void list_append(List *list, ListItem *item) {
	if (item->list == NULL) {
		mutex_get(&(list->mutex), true);
		if (list->first) {
			item->list = list;
			item->next = list->first;
			item->prev = list->first->prev;
			item->prev->next = item;
			item->next->prev = item;
		} else {
			item->next = item;
			item->prev = item;
			list->first = item;
		}
		list->count++;
		mutex_release(&(list->mutex));
	}
}

void list_remove(ListItem *item) {
	mutex_get(&(item->list->mutex), true);
	if (item->list->first == item) {
		item->list->first = item->next;
		if (item->list->first == item) {
			item->list->first = NULL;
		}
	}
	item->next->prev = item->prev;
	item->prev->next = item->next;
	item->list->count--;
	mutex_release(&(item->list->mutex));
}