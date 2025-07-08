#include <libc/string.h>

void *memcpy(void *destination, const void *source, size_t num) {
    char *d = (char *)destination;
    const char *s = (const char *)source;

    for (size_t i = 0; i < num ; i++) {
        d[i] = s[i];
    }

    return destination;
}