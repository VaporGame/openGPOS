#ifndef ELF_H
#define ELF_H
#include <stdint.h>

typedef enum {
    ELF_SUCCESS = 0,
    ELF_INVALID_EXECUTABLE
} elf_error_t;

uint32_t loadELF(const char *path);

#endif //ELF_H