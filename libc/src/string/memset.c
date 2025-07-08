#include <libc/string.h>

void *memset(void *ptr, int value, size_t num) {
    unsigned char *p = (unsigned char*)ptr;
    unsigned char val = (unsigned char)value;

    for (size_t i = 0; i < num; i++) {
        p[i] = val;
    }

    return ptr;
};