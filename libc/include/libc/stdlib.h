#ifndef STDLIB_H
#define STDLIB_H
#include <stddef.h>

void init_malloc();
void* malloc(size_t size) __attribute__((malloc)) __attribute__((alloc_size(1)));
void* realloc(void* ptr, size_t size) __attribute__((alloc_size(2)));
void free(void* ptr) __attribute__((nonnull(1)));

// void dumpHeap();

#endif //STDLIB_H