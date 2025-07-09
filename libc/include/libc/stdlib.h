#ifndef STDLIB_H
#define STDLIB_H
#include <stddef.h>

void init_malloc();
void* malloc(size_t size);
void* realloc(void* ptr, size_t size);
void free(void* ptr);

#endif //STDLIB_H