#include <libc/string.h>

char *strncat(char * destination, char *source, size_t num) {
    char *original_dest = destination;

    while (*destination != '\0') {
        destination++;
    }
    
    while (*source != '\0' && num > 0) {
        *destination = *source;
        destination++;
        source++;
        num--;
    }

    *destination = '\0';

    return original_dest;
}