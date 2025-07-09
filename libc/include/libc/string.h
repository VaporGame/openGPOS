#ifndef STRING_H
#define STRING_H
#include <stddef.h>

void *memcpy(void *destination, const void *source, size_t num);
void *memset(void *ptr, int value, size_t num);

size_t strlen(const char *str);

char *strncpy(char *dest, const char *src, size_t count);
char *strncat(char * destination, char *source, size_t num);

#endif //STRING_H