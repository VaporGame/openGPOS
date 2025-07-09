#ifndef FAT32_H
#define FAT32_H
#include <stdbool.h>
#include "SD.h"

#define MAX_FILENAME_LEN 256 // Max LFN length (255 chars + null)

typedef enum {
    FAT_SUCCESS = 0,
    FAT_ERROR_READ_FAIL,
    FAT_ERROR_INVALID_CLUSTER,
    FAT_ERROR_BAD_FAT_ENTRY,
    FAT_ERROR_END_OF_CHAIN,
    FAT_ERROR_NOT_INITIALIZED,
    FAT_ERROR_NO_MORE_ENTRIES, // New error for readdir
} fat_error_t;

typedef struct {
    uint32_t  current_cluster;      // Current cluster being read
    uint32_t  current_sector_in_cluster; // Current sector within current_cluster
    uint16_t  current_entry_in_sector; // Current 32-byte entry within current_sector
    uint8_t   sector_buffer[SD_BLOCK_SIZE]; // Buffer for the current sector
    bool      sector_buffer_valid;  // True if buffer contains valid data

    // For LFN reconstruction
    char      lfn_buffer[MAX_FILENAME_LEN]; // Buffer to build LFN
    uint8_t   lfn_sequence_count;   // Expected number of LFN entries
    uint8_t   lfn_checksum;         // Checksum of the SFN to match LFNs
} fat_directory_iterator_t;

typedef struct {
    char     filename[MAX_FILENAME_LEN]; // Long filename if available, otherwise SFN
    uint32_t first_cluster;
    uint32_t file_size;
    uint8_t  attributes;
    bool     is_directory;
} fat_file_info_t;

fat_error_t fat_init_root_dir_iterator(fat_directory_iterator_t *iter, uint32_t start_cluster);
fat_error_t fat_read_next_dir_entry(fat_directory_iterator_t *iter, fat_file_info_t *file_info_out);

fat_error_t fat_read_file(const fat_file_info_t *file_info, uint8_t *buffer, uint32_t bytes_to_read, uint32_t offset_in_file);

bool fat32_init(void);

#endif //FAT32_H