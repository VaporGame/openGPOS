#include <libc/string.h>
#include <stdint.h>

void *memcpy(void *destination, const void *source, size_t num) {
    uint32_t *d = (uint32_t *)destination;
    //const char *s = (const char *)source;
    const uint32_t *s = (const uint32_t *)source;

    while (num > 0 && ((uintptr_t)d % 4 != 0 || (uintptr_t)s % 4 != 0)) {
        *((char *)d) = *((const char *)s);
        d = (uint32_t *)((char *)d + 1);
        s = (const uint32_t *)((const char *)s + 1);
        num--;
    }

    // Copy 32-bit words
    size_t num_words = num / 4;
    for (size_t i = 0; i < num_words; i++) {
        d[i] = s[i];
    }

    // Handle trailing unaligned bytes
    size_t remaining_bytes = num % 4;
    char *d_byte = (char *)d + num_words * 4;
    const char *s_byte = (const char *)s + num_words * 4;
    for (size_t i = 0; i < remaining_bytes; i++) {
        d_byte[i] = s_byte[i];
    }

    return destination;
}