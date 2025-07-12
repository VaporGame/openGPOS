#include <libc/string.h>

char* strtok(char* restrict str, const char* restrict delim) {
    static char* lastToken = NULL;
    char* tokenStart = NULL;
    int i = 0;
    
    if (str != NULL) {
        lastToken = str;
    } else {
        if (lastToken == NULL || *lastToken == '\0') {
            return NULL;
        } 
    }

    while (*lastToken != '\0' && strchr(delim, *lastToken) == NULL) {
        lastToken++;
    }

    if (*lastToken == '\0') {
        return NULL;
    }

    tokenStart = lastToken;

    while (*lastToken != '\0' && strchr(delim, *lastToken) == NULL) {
        lastToken++;
    }

    if (*lastToken != '\0') {
        *lastToken = '\0';
        lastToken++;
    }
    
    return tokenStart;
}