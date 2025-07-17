#include "elf/elf.h"
#include "fs/FAT32.h"
#include "util/hexutils.h"
#include <libc/string.h>
#include <libc/stdlib.h>
#include "uart/uart.h"

typedef struct {
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_phentsize;
    uint16_t e_phnum;

    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr; // This is actually our physical address
    //uint32_t p_addr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    //uint32_t p_flags;
    //uint32_t p_align;
} elf_data_t;

uint32_t loadELF(const char *path) {
    uint32_t file_id = fat32_open(path, 0);
    uint32_t size = fat32_get_size(file_id);
    uint8_t *data = malloc(sizeof(uint8_t) * size);
    if (data == NULL) return 2;

    fat32_read(file_id, data, size);
    if (data[0x00] != 0x7F || data[1] != 0x45 || data[2] != 0x4C || data[3] != 0x46) {
        return ELF_INVALID_EXECUTABLE; // Not a elf executable
    } else if (data[0x04] == 2) { //EI_CLASS
        return ELF_INVALID_EXECUTABLE; // 64 bit executable
    } else if (data[0x05] == 2) { //EI_DATA
        return ELF_INVALID_EXECUTABLE; // Only support little endian
    } else if (data[0x10] != 0x02) { // Check if the ELF is of executable type
        return ELF_INVALID_EXECUTABLE;
    } else if (data[0x12] != 0x28) {
        return ELF_INVALID_EXECUTABLE; // not arm
    }

    uint32_t e_entry = read_le32(data, 0x18);
    uint32_t e_phoff = read_le32(data, 0x1C);
    uint32_t e_phentsize = read_le16(data, 0x2A);
    uint16_t e_phnum = read_le16(data, 0x2C);
    
    // Read program data table
    for(uint16_t i = 0; i < e_phnum; i++) {
        uint32_t offset = e_phoff + i * e_phentsize;
        
        uint32_t p_type = read_le32(data, offset);
        if (p_type != 0x00000001) { // PT_LOAD
            continue; // We only care about PT_LOAD for staticly linked binaries
        }

        uint32_t p_offset = read_le32(data, offset + 0x04);
        uint32_t p_vaddr = read_le32(data, offset + 0x08); // This is actually our physical address
        // uint32_t p_addr = read_le32(data, offset + 0x0C);
        uint32_t p_filesz = read_le32(data, offset + 0x10);
        uint32_t p_memsz = read_le32(data, offset + 0x14);
        // uint32_t p_flags = read_le32(data, offset + 0x18);
        // uint32_t p_align = read_le32(data, offset + 0x1C);

        // Load the binary
        // Since the EI_OSABI is set to System V, i think i can assume
        // that p_vaddr will be in ascending order

        // void* destination_addr = ;
        

        // Copy into memory
        memcpy((void*)p_vaddr, (const void*)data + p_offset, p_filesz);

        // Zero initialize the BSS
        if (p_memsz > p_filesz) {
            memset((void*)((uintptr_t)(void*)p_vaddr + p_filesz), 0, p_memsz - p_filesz);
        } 
    }
    free(data);
    return e_entry;
}