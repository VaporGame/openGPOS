#include <libc/string.h>

size_t strlen(const char *str) {
    unsigned int i = 0;
    while(str[i] != '\0') {
        i++;
    }
    return i;
}