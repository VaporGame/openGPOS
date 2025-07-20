#include "fs/FAT32.h"
#include <stdbool.h>
#include <stdint.h>
#include "sd/SD.h"
#include "uart/uart.h"
#include <stddef.h>
#include <libc/string.h>
#include "util/hexutils.h"

#define MAX_OPEN_FILES 16
file_handle_t open_file_handles[MAX_OPEN_FILES];

typedef struct {
    uint32_t volume_start_lba;
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sector_count;
    uint8_t num_fats;
    uint32_t fat_size_sectors;
    uint32_t root_dir_cluster;
    uint32_t total_sectors;
    uint32_t total_clusters;
    uint32_t first_fat_lba;
    uint32_t data_start_lba;
    bool is_initialized;
    uint16_t fsinfo_sector_lba;
    uint16_t backup_boot_sector_lba;
    uint32_t bytes_per_cluster;
} fat32_volume_info_t;

fat32_volume_info_t g_fat32_volume_info;

#define FAT_ATTR_READ_ONLY  0x01
#define FAT_ATTR_HIDDEN     0x02
#define FAT_ATTR_SYSTEM     0x04
#define FAT_ATTR_VOLUME_ID  0x08 // Volume label entry
#define FAT_ATTR_DIRECTORY  0x10
#define FAT_ATTR_ARCHIVE    0x20
#define FAT_ATTR_LFN        (FAT_ATTR_READ_ONLY | FAT_ATTR_HIDDEN | FAT_ATTR_SYSTEM | FAT_ATTR_VOLUME_ID) // 0x0F

typedef struct {
    uint8_t  DIR_Name[11];      // 8.3 filename (padded with spaces)
    uint8_t  DIR_Attr;          // File attributes
    uint8_t  DIR_NTRes;         // Reserved, always 0
    uint8_t  DIR_CrtTimeTenth;  // Creation time (tenths of a second)
    uint16_t DIR_CrtTime;       // Creation time
    uint16_t DIR_CrtDate;       // Creation date
    uint16_t DIR_LstAccDate;    // Last access date
    uint16_t DIR_FstClusHI;     // High 16 bits of first cluster (for FAT32)
    uint16_t DIR_WrtTime;       // Last write time
    uint16_t DIR_WrtDate;       // Last write date
    uint16_t DIR_FstClusLO;     // Low 16 bits of first cluster
    uint32_t DIR_FileSize;      // File size in bytes
} __attribute__((packed)) fat_sfn_dir_entry_t;

typedef struct {
    uint8_t  LDIR_Ord;          // Order of entry in sequence, plus 0x40 if last
    uint16_t LDIR_Name1[5];     // Characters 1-5 of LFN (UTF-16LE)
    uint8_t  LDIR_Attr;         // Attributes (must be 0x0F for LFN)
    uint8_t  LDIR_Type;         // Must be 0
    uint8_t  LDIR_Chksum;       // Checksum of SFN
    uint16_t LDIR_Name2[6];     // Characters 6-11 of LFN (UTF-16LE)
    uint16_t LDIR_FstClusLO;    // Must be 0 (LFN has no data, only SFN does)
    uint16_t LDIR_Name3[2];     // Characters 12-13 of LFN (UTF-16LE)
} __attribute__((packed)) fat_lfn_dir_entry_t;

// A union to easily access either type of entry
typedef union {
    fat_sfn_dir_entry_t sfn;
    fat_lfn_dir_entry_t lfn;
    uint8_t raw[32]; // For general byte access
} fat_dir_entry_t;

static inline bool read_sectors(uint32_t lba, uint32_t count, uint8_t *buffer) {
    for (uint32_t i = 0; i < count; i++) {
        if (!sdReadBlock(lba + i, buffer + (i * 512))) {
            return false;
        }
    }
    return true;
}

static inline bool write_sectors(uint32_t lba, uint32_t count, uint8_t *buffer) {
    for (uint32_t i = 0; i < count; i++) {
        if (!sdWriteBlock(lba + i, buffer + (i * 512))) {
            return false;
        }
    }
    return true;
}

static inline int min(int a, int b) {
    return (a < b) ? a : b;
}

// TODO: IMPLEMENT MULTI BLOCK READING SD COMMANDS

static bool parseMBR(void) {
    static uint8_t mbr_buffer[66];

    if (!sdReadPartialBlock(0, 446, mbr_buffer, 66)) {
        return false;
    }

    uint16_t mbr_signature = read_le16(mbr_buffer, 64); // Read from the start of the 2-byte buffer
    if (mbr_signature != 0xAA55) {
        return false;
    }

    // Process the partition table entries from the smaller buffer
    for (uint8_t i = 0; i < 4; i++) {
        const uint16_t offset = i * 16;

        if ((mbr_buffer[offset] & 0x80) && (mbr_buffer[offset + 4] == 0x0B || mbr_buffer[offset + 4] == 0x0C)) {
            g_fat32_volume_info.total_sectors = read_le32(mbr_buffer, offset + 12);
            g_fat32_volume_info.volume_start_lba = read_le32(mbr_buffer, offset + 8);
            return true;
        }
    }
    return false;
}

// This function will only be ran internally after parsing the MBR, its not static for testing
static bool parseBPB(void) {
    static uint8_t bpb_data[41];

    // if(!sdReadBlock(g_fat32_volume_info.volume_start_lba, bpb_data)) {
    if (!sdReadPartialBlock(g_fat32_volume_info.volume_start_lba, 0x0B, bpb_data, 41)) {
        return false;
    }

    // 18 bytes
    // g_fat32_volume_info.bytes_per_sector = read_le16(bpb_data, 0x0B);
    // g_fat32_volume_info.sectors_per_cluster = bpb_data[0x0D];
    // g_fat32_volume_info.reserved_sector_count = read_le16(bpb_data, 0x0E);
    // g_fat32_volume_info.num_fats = bpb_data[0x10];
    // g_fat32_volume_info.fat_size_sectors = read_le32(bpb_data, 0x24);
    // g_fat32_volume_info.root_dir_cluster = read_le32(bpb_data, 0x2c);
    // //g_fat32_volume_info.total_sectors = read_le32(bpb_data, 0x20);
    // g_fat32_volume_info.fsinfo_sector_lba = read_le16(bpb_data, 0x30);
    // g_fat32_volume_info.backup_boot_sector_lba = read_le16(bpb_data, 0x32);

    g_fat32_volume_info.bytes_per_sector = read_le16(bpb_data, 0);
    g_fat32_volume_info.sectors_per_cluster = bpb_data[0x02];
    g_fat32_volume_info.reserved_sector_count = read_le16(bpb_data, 0x03);
    g_fat32_volume_info.num_fats = bpb_data[0x05];
    g_fat32_volume_info.fat_size_sectors = read_le32(bpb_data, 25);
    g_fat32_volume_info.root_dir_cluster = read_le32(bpb_data, 33);
    //g_fat32_volume_info.total_sectors = read_le32(bpb_data, 0x20);
    g_fat32_volume_info.fsinfo_sector_lba = read_le16(bpb_data, 37);
    g_fat32_volume_info.backup_boot_sector_lba = read_le16(bpb_data, 39);

    g_fat32_volume_info.first_fat_lba = g_fat32_volume_info.volume_start_lba + g_fat32_volume_info.reserved_sector_count;
    g_fat32_volume_info.data_start_lba =    g_fat32_volume_info.first_fat_lba +
                                            (g_fat32_volume_info.num_fats * g_fat32_volume_info.fat_size_sectors);

    g_fat32_volume_info.bytes_per_cluster = g_fat32_volume_info.sectors_per_cluster * g_fat32_volume_info.bytes_per_sector;

    uint32_t data_sectors = g_fat32_volume_info.total_sectors - (g_fat32_volume_info.reserved_sector_count +
                                                                (g_fat32_volume_info.num_fats * g_fat32_volume_info.fat_size_sectors));
    g_fat32_volume_info.total_clusters = data_sectors / g_fat32_volume_info.sectors_per_cluster;

    g_fat32_volume_info.is_initialized = true;
    return true;
}

bool fat32_init(void) {
    if (!parseMBR() || !parseBPB()) return false;

    // init handles
    for (uint32_t i = 0; i < MAX_OPEN_FILES; i++) {
        open_file_handles[i].is_free = true;
    }

    return true;
}

// REMEMBER TO IMPLEMENT CACHING!!!!!!!!!

#define FAT32_EOC_MARKER_START 0x0FFFFFF8
#define FAT32_BAD_CLUSTER      0x0FFFFFF7
#define FAT32_LAST_CLUSTER_VALID 0x0FFFFFFF // EOC marker valid up to this value

static uint32_t getNextCluster(uint32_t current_cluster, uint32_t *next_cluster_out) {
    if (!g_fat32_volume_info.is_initialized) {
        return FAT_ERROR_NOT_INITIALIZED;
    }

    if (current_cluster < 2 || current_cluster >= g_fat32_volume_info.total_clusters + 2) {
        return FAT_ERROR_INVALID_CLUSTER;
    }

    const uint32_t fat_entry_offset = current_cluster * 4;
    const uint32_t fat_sector_offset = fat_entry_offset / g_fat32_volume_info.bytes_per_sector;
    const uint16_t fat_byte_in_sector_offset = fat_entry_offset % g_fat32_volume_info.bytes_per_sector;
    const uint32_t fat_sector_lba = g_fat32_volume_info.first_fat_lba + fat_sector_offset;

    uint8_t fat_entry_buffer[4];

    // Read the FAT sector
    if (!sdReadPartialBlock(fat_sector_lba, fat_byte_in_sector_offset, fat_entry_buffer, 4)) {
        return FAT_ERROR_READ_FAIL;
    }

    uint32_t fat_entry_value = read_le32(fat_entry_buffer, 0);

    fat_entry_value &= 0x0FFFFFFF; // Clear bits 28-31

    if (fat_entry_value >= FAT32_EOC_MARKER_START && fat_entry_value <= FAT32_LAST_CLUSTER_VALID) {
        *next_cluster_out = 0;
        return FAT_ERROR_END_OF_CHAIN;
    } else if (fat_entry_value == FAT32_BAD_CLUSTER) {
        *next_cluster_out = 0;
        return FAT32_BAD_CLUSTER;
    } else if (fat_entry_value == 0) {
        *next_cluster_out = 0;
        return FAT_ERROR_INVALID_CLUSTER;
    } else {
        *next_cluster_out = fat_entry_value;
        return FAT_SUCCESS;
    }
}

static uint32_t fatClusterToLba(uint32_t cluster) {
    if (cluster < 2) return 0;

    return g_fat32_volume_info.data_start_lba + ((cluster - 2) * g_fat32_volume_info.sectors_per_cluster);
}

static uint8_t fat_read_cluster(uint32_t cluster, uint8_t *buffer) {
    uint32_t lba = fatClusterToLba(cluster);
    if (lba == 0) {
        return FAT_ERROR_INVALID_CLUSTER;
    }
    if (!read_sectors(lba, g_fat32_volume_info.sectors_per_cluster, buffer)) {
        return FAT_ERROR_READ_FAIL;
    }
    return FAT_SUCCESS;
}

static void fat_sfn_to_string(const uint8_t *sfn_name, char *out_name) {
    uint16_t i, j;
    for (i = 0; i < 8 && sfn_name[i] != ' '; i++) {
        out_name[i] = sfn_name[i];
    }
    // Add dot if extension exists
    if (sfn_name[8] != ' ') {
        out_name[i++] = '.';
        for( j = 0; j < 3 && sfn_name[8 + j] != ' '; j++) {
            out_name[i++] = sfn_name[8 + j];
        }
    }
    out_name[i] = '\0';
}

static uint8_t fat_sfn_checksum(const uint8_t *sfn_name) {
    uint8_t sum = 0;
    for (uint8_t i = 0; i < 11; i++) {
        sum = ((sum & 1) ? 0x80 : 0) + (sum >> 1) + sfn_name[i];
    }
    return sum;
}

static size_t fat_lfn_to_utf8(const uint8_t *utf16_name_part, size_t copy_len, char *out_buffer, size_t max_len, size_t current_out_len) {
    uint16_t buffer[copy_len/2];
    memcpy(buffer, utf16_name_part, copy_len);

    size_t original_out_len = current_out_len;
    for (unsigned int i = 0; i < copy_len / 2; i++) {
        uint16_t wc = buffer[i];

        if (wc == 0x0000 || wc == 0xFFFF) {
            break;
        }
        
        if (current_out_len < max_len - 1) {
            if (wc < 0x80) {
                out_buffer[current_out_len++] = (char)wc;
            } else {
                out_buffer[current_out_len++] = '?';
            }
        } else break;
    }
    out_buffer[current_out_len] = '\0';
    return current_out_len - original_out_len;
}

fat_error_t fat_init_root_dir_iterator(fat_directory_iterator_t *iter, uint32_t start_cluster) {
    if (!g_fat32_volume_info.is_initialized) {
        return FAT_ERROR_NOT_INITIALIZED;
    }
    if (iter == NULL) {
        return FAT_ERROR_BAD_FAT_ENTRY;
    }
    
    memset(iter, 0, sizeof(fat_directory_iterator_t));
    iter->current_cluster = start_cluster;
    iter->current_sector_in_cluster = -1;
    iter->current_entry_in_sector = 0;
    iter->sector_buffer_valid = false;

    //iter->lfn_checksum = 0;
    return FAT_SUCCESS;
}

fat_error_t fat_read_next_dir_entry(fat_directory_iterator_t *iter, fat_file_info_t *file_info_out) {
    if (!g_fat32_volume_info.is_initialized) {
        return FAT_ERROR_NOT_INITIALIZED;
    }
    if (file_info_out == NULL || iter == NULL) {
        return FAT_ERROR_BAD_FAT_ENTRY;
    }

    const uint32_t entries_per_sector = g_fat32_volume_info.bytes_per_sector / 32;

    for (;;) {
        // Check if we need to load a new sector
        if (iter->sector_buffer_valid == false || iter->current_entry_in_sector >= entries_per_sector) {
            // Advance to the next sector in cluster
            iter->current_sector_in_cluster++;
            iter->current_entry_in_sector = 0;

            // Check if we need to move to the next cluster
            if (iter->current_sector_in_cluster >= g_fat32_volume_info.sectors_per_cluster) {
                uint32_t next_cluster;
                fat_error_t fat_res = getNextCluster(iter->current_cluster, &next_cluster);
                
                if (fat_res != FAT_SUCCESS) {
                    return fat_res;
                }

                iter->current_cluster = next_cluster;
                iter->current_sector_in_cluster = 0;
            }

            // Read new sector
            uint32_t sector_lba = fatClusterToLba(iter->current_cluster) + iter->current_sector_in_cluster;

            if (!sdReadBlock(sector_lba, iter->sector_buffer)) {
                return FAT_ERROR_READ_FAIL;
            }
            iter->sector_buffer_valid = true;
        } 

        // Process the current entry in the buffer
        fat_dir_entry_t *entry = (fat_dir_entry_t *)(iter->sector_buffer + (iter->current_entry_in_sector * 32));
        iter->current_entry_in_sector++; // Move to next entry for next call

        uint8_t first_byte = entry->raw[0];

        // Check for end of dir marker (0x00)
        if (first_byte == 0x00) {
            return FAT_ERROR_NO_MORE_ENTRIES;
        }

        // Check for deleted entry
        if (first_byte == 0xE5) continue; // Skip
        
        char lfn_buffer[MAX_LFN_PARTS][MAX_UTF8_CHARS_PER_LFN_PART];
        // for (uint8_t i)
        // char assembled_lfn_buffer[MAX_FILENAME_LEN];
        uint8_t lfn_parts_found;
        // Check for LFN entry
        if ((entry->sfn.DIR_Attr & FAT_ATTR_LFN) == FAT_ATTR_LFN) {
            fat_lfn_dir_entry_t *lfn_entry = &entry->lfn;
            const uint8_t order_flags = lfn_entry->LDIR_Ord;
            const uint8_t sequence_order = order_flags & ~0x40;
            
            if (sequence_order == 0 || sequence_order > MAX_LFN_PARTS) {
                lfn_parts_found = 0;
                iter->lfn_checksum = 0;
                continue;
            }

            // Check if this is the last LFN entry
            if (order_flags & 0x40) {
                iter->lfn_checksum =lfn_entry->LDIR_Chksum;
            } else if (iter->lfn_checksum == 0) {
                lfn_parts_found = 0;
                iter->lfn_checksum = 0;
                continue;
            }

            char *segment_dest = lfn_buffer[sequence_order - 1];;
            size_t current_segment_len = 0;

            current_segment_len += fat_lfn_to_utf8((uint8_t *)lfn_entry->LDIR_Name1, 10, segment_dest, MAX_FILENAME_LEN, current_segment_len);
            current_segment_len += fat_lfn_to_utf8((uint8_t *)lfn_entry->LDIR_Name2, 12, segment_dest, MAX_FILENAME_LEN, current_segment_len);
            fat_lfn_to_utf8((uint8_t *)lfn_entry->LDIR_Name3, 4, segment_dest, MAX_FILENAME_LEN, current_segment_len);

            lfn_parts_found++;
            continue;
        }

        // If we reached here, it's an SFN entry
        fat_sfn_dir_entry_t *sfn_entry = &entry->sfn;

        // Validate LFN checksum if an LFN sequence was being processed
        if (iter->lfn_checksum != 0 && fat_sfn_checksum(sfn_entry->DIR_Name) == iter->lfn_checksum) {
            file_info_out->filename[0] = '\0';
            for (uint8_t i = 0; i < lfn_parts_found; i++) {
                if (lfn_buffer[i][0] != '\0') {
                    strncat(file_info_out->filename, lfn_buffer[i], MAX_FILENAME_LEN - 1 - strlen(file_info_out->filename));
                }
            }
            file_info_out->filename[MAX_FILENAME_LEN - 1] = '\0';
        } else {
            // No LFN, use SFN
            fat_sfn_to_string(sfn_entry->DIR_Name, file_info_out->filename);
        }

        lfn_parts_found = 0;
        iter->lfn_checksum = 0;

        file_info_out->first_cluster = (read_le16((uint8_t*)sfn_entry, 20) << 16) | read_le16((uint8_t*)sfn_entry, 26);
        file_info_out->file_size = read_le32((uint8_t*)sfn_entry, 28);
        file_info_out->attributes = sfn_entry->DIR_Attr;
        file_info_out->is_directory = (sfn_entry->DIR_Attr & FAT_ATTR_DIRECTORY) != 0;

        // Skip volume ID entries (if they don't have LFNs and aren't actually directories)
        if ((file_info_out->attributes & FAT_ATTR_VOLUME_ID) && file_info_out->is_directory == false) {
            continue; // Skip this entry and try next
        }

        return FAT_SUCCESS;
    }
}

static fat_error_t fat_read_file(uint32_t file_id, uint8_t *buffer, uint32_t bytes_to_read) {
    if (!g_fat32_volume_info.is_initialized) {
        return FAT_ERROR_NOT_INITIALIZED;
    }
    if (buffer == NULL) {
        return FAT_ERROR_BAD_FAT_ENTRY;
    }
    if (file_id >= MAX_OPEN_FILES || open_file_handles[file_id].is_free) {
        return FAT_ERROR_BAD_FAT_ENTRY;
    }

    file_handle_t *file_handle = &open_file_handles[file_id];

    uint32_t actual_bytes_to_read = min(bytes_to_read, file_handle->file_size - file_handle->current_file_pos);
    if (actual_bytes_to_read == 0) {
        return FAT_SUCCESS; // Nothing to read
    }

    uint32_t bytes_read_total = 0;

    while (bytes_read_total < actual_bytes_to_read) {
        uint32_t current_sector_in_cluster = file_handle->current_offset / g_fat32_volume_info.bytes_per_sector;
        uint32_t current_byte_in_sector = file_handle->current_offset % g_fat32_volume_info.bytes_per_sector;

        uint32_t current_sector_lba = fatClusterToLba(file_handle->current_cluster) + current_sector_in_cluster;

        // Calculate how much data to copy from this sector
        uint32_t bytes_left_in_sector = g_fat32_volume_info.bytes_per_sector - current_byte_in_sector;
        uint32_t bytes_to_copy = min(actual_bytes_to_read - bytes_read_total, bytes_left_in_sector);

        if (!sdReadPartialBlock(current_sector_lba, current_byte_in_sector, buffer + bytes_read_total, bytes_to_copy)) {
            return FAT_ERROR_READ_FAIL;
        }

        file_handle->current_file_pos += bytes_to_copy;
        file_handle->current_offset += bytes_to_copy;
        bytes_read_total += bytes_to_copy;

        if (file_handle->current_offset >= g_fat32_volume_info.bytes_per_cluster) {
            uint32_t next_cluster;
            fat_error_t res = getNextCluster(file_handle->current_cluster, &next_cluster);

            if (res == FAT_ERROR_END_OF_CHAIN) {
                break; // Reached end of file
            } else if (res != FAT_SUCCESS) {
                return res;
            }
            file_handle->current_cluster = next_cluster;
            file_handle->current_offset = 0;
        }
    }

    return FAT_SUCCESS;
}

static fat_error_t get_file(const char *path, fat_file_info_t *file) {
    if (path == NULL || file == NULL) {
        return FAT_ERROR_BAD_FAT_ENTRY;
    } 

    char name_buffer[MAX_FILENAME_LEN];
    fat_directory_iterator_t dir_iter;
    fat_file_info_t current_entry;

    fat_error_t init_res = fat_init_root_dir_iterator(&dir_iter, g_fat32_volume_info.root_dir_cluster);
    if(init_res != FAT_SUCCESS) {
        return init_res;
    } 

    path++;

    for (;;) {
        uint16_t i = 0;
        bool final_path_component = false;

        // get current dir/file name
        while(*path != '/' && *path != '\0') {
            if (i < MAX_FILENAME_LEN - 1) {
                name_buffer[i++] = *path;
            }
            path++;
        }
        name_buffer[i] = '\0'; 

        if (*path == '\0') {
            final_path_component = true;
        } else {
            path++;
        }

        if (name_buffer[0] == '\0' && !final_path_component) { // Handle empty component (e.g., "//")
            continue;
        }

        // find dir/file name in current directory
        fat_error_t result = FAT_ERROR_NO_MORE_ENTRIES;
        do {
            result = fat_read_next_dir_entry(&dir_iter, &current_entry);

            if (result == FAT_SUCCESS) {
                if (strcmp(name_buffer, current_entry.filename) == 0) {
                    if (current_entry.is_directory && !final_path_component) {
                        init_res = fat_init_root_dir_iterator(&dir_iter, current_entry.first_cluster);
                        if (init_res != FAT_SUCCESS) {
                            return init_res;
                        }
                        break;

                    } else if (final_path_component && !current_entry.is_directory) {
                        *file = current_entry;
                        return FAT_SUCCESS;

                    } else if (final_path_component && current_entry.is_directory) {
                        return FAT_ERROR_FILE_NOT_FOUND;
                    }
                }
            } else if (result == FAT_ERROR_NO_MORE_ENTRIES) {
                return FAT_ERROR_FILE_NOT_FOUND;
            } else {
                return result;
            }

        } while (result == FAT_SUCCESS);
    }
    return FAT_ERROR_FILE_NOT_FOUND;
}

uint32_t fat32_open(const char* path, const uint8_t mode) {
    fat_file_info_t file;
    if (get_file(path, &file) != FAT_SUCCESS) {
        return (uint32_t)-1;
    }

    for (uint32_t i = 0; i < MAX_OPEN_FILES; i++) {
        if(open_file_handles[i].is_free) {
            file_handle_t *handle = &open_file_handles[i];
            handle->cluster_start = file.first_cluster;
            handle->current_cluster = file.first_cluster;
            handle->current_offset = 0;
            handle->file_size = file.file_size;
            handle->current_file_pos = 0;
            handle->is_free = false;
            handle->mode = mode;
            return i;
        }
    }
    return (uint32_t)-1;
}

bool fat32_close(uint32_t file_id) {
    if (file_id >= MAX_OPEN_FILES) {
        return false;
    }

    open_file_handles[file_id].is_free = true;
    return true;
}

bool fat32_read(uint32_t file_id, uint8_t *buffer, uint32_t bytes_to_read) {
    if (file_id >= MAX_OPEN_FILES || buffer == NULL) {
        return false;
    }
    if (open_file_handles[file_id].is_free) {
        return false;
    }

    return fat_read_file(file_id, buffer, bytes_to_read) == FAT_SUCCESS;
}

uint32_t fat32_get_size(uint32_t file_id) {
    if (file_id >= MAX_OPEN_FILES) {
        return 0;
    }
    return open_file_handles[file_id].file_size;
}