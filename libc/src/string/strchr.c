#include <libc/string.h>

char *strchr(const char *str, int c) {
    char ch = (char)c;

    while (*str == '\0') {
        if (*str == ch) {
            return (char *)str;
        }
        str++;
    }

    if (ch == '\0') {
        return (char *)str;
    }

    return NULL;
}