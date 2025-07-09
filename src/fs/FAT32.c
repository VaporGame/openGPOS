#include "fs/FAT32.h"
#include <stdbool.h>
#include <stdint.h>
#include "SD.h"
#include "hardware/uart.h"
#include <stddef.h>
#include <libc/string.h>

uint32_t fat32_volume_start_lba = 0;
uint32_t fat32_volume_sector_count = 0;

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

//fat_directory_iterator_t g_root_dir_iterator; // REMOVE

static bool read_sectors(uint32_t lba, uint32_t count, uint8_t *buffer) {
    for (uint32_t i = 0; i < count; i++) {
        uint32_t current_lba = lba + i;
        uint8_t *current_buffer_pos = buffer + (i * 512);

        bool result = sdReadBlock(current_lba, current_buffer_pos);

        if (result == false) {
            return false;
        }
    }
    return true;
}

static bool write_sectors(uint32_t lba, uint32_t count, uint8_t *buffer) {
    for (uint32_t i = 0; i < count; i++) {
        uint32_t current_lba = lba + i;
        uint8_t *current_buffer_pos = buffer + (i * 512);

        bool result = sdWriteBlock(current_lba, current_buffer_pos);

        if (!result) {
            return false;
        }
    }
    return true;
}

// TODO: IMPLEMENT MULTI BLOCK READING SD COMMANDS

static uint32_t read_le32(const uint8_t *data, uint16_t offset) {
    return data[offset]         |
           (data[offset + 1] << 8)  |
           (data[offset + 2] << 16) |
           (data[offset + 3] << 24);
}

static uint16_t read_le16(const uint8_t *data, uint16_t offset) {
    return data[offset]         |
           (data[offset + 1] << 8);
}

static bool parseMBR(void) {
    uint8_t block_data[512];

    if (!sdReadBlock(0, block_data)) {
        uartTxStr("Failed to read MBR block 0.\r\n");
        return false;
    }

    // Read MBR signature
    //uint16_t mbr_signature = block_data[510] | (block_data[511] << 8);
    uint16_t mbr_signature = read_le16(block_data, 510);
    if (mbr_signature != 0xAA55) {
        uartTxStr("Card not formatted with MBR (bad signature)");
        return false;
    }

    // Read the four partition table entries
    // First partition entry is at offset 446 (0x1BE)
    // Each entry is 16 bytes
    for (uint8_t i = 0; i < 4; i++) {
        uint16_t offset = 0x1BE + (i * 16);

        // Read bootable flag (first byte)
        bool bootable = (block_data[offset] & 0x80);
        // MSB signifies if the partition is bootable

        // Read partition type
        // Should be 0x0B or 0x0C for FAT32
        uint8_t type = block_data[offset + 4];
        bool isFAT32 = false;
        if(type == 0x0B || type == 0x0C) {
            isFAT32 = true;
        }

        // Read LBA of first sector
        // It is in little endian
        uint32_t first_sector = read_le32(block_data, offset + 8);

        // Read number of sectors
        uint32_t sector_count = read_le32(block_data, offset + 12);
        if(bootable && isFAT32) {
            fat32_volume_sector_count = sector_count;
            fat32_volume_start_lba = first_sector;
            // uartTxStr("Found bootable partition: ");
            // uartTxDec(i + 1);
            // uartTxStr("\r\n");
            return true;
        }
    }
    uartTxStr("No bootable FAT32 partition found\r\n");
    return false;
}

// This function will only be ran internally after parsing the MBR, its not static for testing
static bool parseBPB(void) {
    uint8_t bpb_data[512];

    if(!sdReadBlock(fat32_volume_start_lba, bpb_data)) {
        uartTxStr("failed to read BPB\r\n");
        return false;
    }

    g_fat32_volume_info.volume_start_lba = fat32_volume_start_lba;
    g_fat32_volume_info.bytes_per_sector = read_le16(bpb_data, 0x0B);
    g_fat32_volume_info.sectors_per_cluster = bpb_data[0x0D];
    g_fat32_volume_info.reserved_sector_count = read_le16(bpb_data, 0x0E);
    g_fat32_volume_info.num_fats = bpb_data[0x10];
    g_fat32_volume_info.fat_size_sectors = read_le32(bpb_data, 0x24);
    g_fat32_volume_info.root_dir_cluster = read_le32(bpb_data, 0x2c);
    g_fat32_volume_info.total_sectors = read_le32(bpb_data, 0x20);
    g_fat32_volume_info.fsinfo_sector_lba = read_le16(bpb_data, 0x30);
    g_fat32_volume_info.backup_boot_sector_lba = read_le16(bpb_data, 0x32);

    g_fat32_volume_info.first_fat_lba = g_fat32_volume_info.volume_start_lba + g_fat32_volume_info.reserved_sector_count;
    g_fat32_volume_info.data_start_lba =    g_fat32_volume_info.first_fat_lba +
                                            (g_fat32_volume_info.num_fats * g_fat32_volume_info.fat_size_sectors);

    uint32_t data_sectors = g_fat32_volume_info.total_sectors - (g_fat32_volume_info.reserved_sector_count +
                                                                (g_fat32_volume_info.num_fats * g_fat32_volume_info.fat_size_sectors));
    g_fat32_volume_info.total_clusters = data_sectors / g_fat32_volume_info.sectors_per_cluster;

    g_fat32_volume_info.is_initialized = true;
    return true;
}

bool fat32_init(void) {
    if (!parseMBR()) return false;
    if (!parseBPB()) return false;
    return true;
}

// REMEMBER TO IMPLEMENT CACHING!!!!!!!!!

#define FAT32_EOC_MARKER_START 0x0FFFFFF8
#define FAT32_BAD_CLUSTER      0x0FFFFFF7
#define FAT32_LAST_CLUSTER_VALID 0x0FFFFFFF // EOC marker valid up to this value

uint32_t getNextCluster(uint32_t current_cluster, uint32_t *next_cluster_out) {
    if (!g_fat32_volume_info.is_initialized) {
        uartTxStr("Not inited\r\n");
        return FAT_ERROR_NOT_INITIALIZED;
    }

    if (current_cluster < 2 || current_cluster >= g_fat32_volume_info.total_clusters + 2) {
        uartTxStr("Error: Invalid current_cluster: ");
        uartTxHex(current_cluster);
        uartTxStr("\r\n");
        return FAT_ERROR_INVALID_CLUSTER;
    }

    uint8_t bytes_per_entry = 4;
    uint32_t fat_entry_offset = current_cluster * bytes_per_entry;
    uint32_t fat_sector_offset = fat_entry_offset / g_fat32_volume_info.bytes_per_sector;
    uint16_t fat_byte_in_sector_offset = fat_entry_offset % g_fat32_volume_info.bytes_per_sector;
    uint32_t fat_sector_lba = g_fat32_volume_info.first_fat_lba + fat_sector_offset;

    uint8_t fat_sector_buffer[g_fat32_volume_info.bytes_per_sector];

    // Read the FAT sector
    bool result = sdReadBlock(fat_sector_lba, fat_sector_buffer);
    if (result == false) {
        uartTxStr("Failed to read FAT sector at LBA ");
        uartTxHex(fat_sector_lba);
        uartTxStr("\r\n");
        return FAT_ERROR_READ_FAIL;
    }

    uint32_t fat_entry_value = read_le32(fat_sector_buffer, fat_byte_in_sector_offset);

    fat_entry_value &= 0x0FFFFFFF; // Clear bits 28-31

    if (fat_entry_value >= FAT32_EOC_MARKER_START && fat_entry_value <= FAT32_LAST_CLUSTER_VALID) {
        *next_cluster_out = 0;
        return FAT_ERROR_END_OF_CHAIN;
    } else if (fat_entry_value == FAT32_BAD_CLUSTER) {
        *next_cluster_out = 0;
        return FAT32_BAD_CLUSTER;
    } else if (fat_entry_value == 0) {
        uartTxStr("Cluster "); uartTxHex(current_cluster); uartTxStr(" points to a free cluster\r\n");
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
        uartTxStr("Attempted to read invalid cluster\r\n");
        return FAT_ERROR_INVALID_CLUSTER;
    }
    uint32_t sectors_to_read = g_fat32_volume_info.sectors_per_cluster;
    bool res = read_sectors(lba, sectors_to_read, buffer);
    if (res != 0) {
        uartTxStr("Failed to read cluster\r\n");
        return FAT_ERROR_READ_FAIL;
    }
    return FAT_SUCCESS;
}

static void fat_sfn_to_string(uint8_t *sfn_name, char *out_name) {
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
    for (int i = 11; i > 0; i--) {
        sum = ((sum & 1) ? 0x80 : 0) + (sum >> 1) + *sfn_name++;
    }
    return sum;
}

static void fat_lfn_to_utf8(uint8_t *utf16_name_part, size_t copy_len, char *out_buffer, size_t max_len) {
    uint16_t buffer[copy_len];
    memcpy(buffer, utf16_name_part, copy_len);

    size_t current_len = strlen(out_buffer);
    unsigned int i;
    for (i = 0; i < copy_len / 2; i++) {
            uint16_t wc = buffer[i];
        if (wc == 0x0000 || wc == 0xFFFF) {
            break;
        }
        if (wc < 0x80) {
            out_buffer[current_len++] = (char)wc;
        } else {
            out_buffer[current_len++] = '?';
        }
    }
    out_buffer[current_len] = '\0';
}

fat_error_t fat_init_root_dir_iterator(fat_directory_iterator_t *iter, uint32_t start_cluster) {
    if (!g_fat32_volume_info.is_initialized) {
        uartTxStr("Error: FAT32 volume not initialized before root dir iterator.\r\n");
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

    iter->assembled_lfn_buffer[0] = '\0';
    iter->lfn_parts_found = 0;
    iter->lfn_checksum = 0;
    return FAT_SUCCESS;
}

fat_error_t fat_read_next_dir_entry(fat_directory_iterator_t *iter, fat_file_info_t *file_info_out) {
    if (!g_fat32_volume_info.is_initialized) {
        return FAT_ERROR_NOT_INITIALIZED;
    }
    if (file_info_out == NULL) {
        uartTxStr("file info is NULL\r\n");
        return FAT_ERROR_BAD_FAT_ENTRY;
    }
    if (iter == NULL) {
        uartTxStr("iter is NULL\r\n");
        return FAT_ERROR_BAD_FAT_ENTRY;
    }

    // uint32_t cluster_size_bytes = g_fat32_volume_info.bytes_per_sector * g_fat32_volume_info.sectors_per_cluster;
    uint32_t entries_per_sector = g_fat32_volume_info.bytes_per_sector / 32;

    while (true) {
        // Check if we need to load a new sector
        if (iter->sector_buffer_valid == false || iter->current_entry_in_sector >= entries_per_sector) {
            // Advance to the next sector in cluster
            iter->current_sector_in_cluster++;
            iter->current_entry_in_sector = 0;

            // Check if we need to move to the next cluster
            if (iter->current_sector_in_cluster >= g_fat32_volume_info.sectors_per_cluster) {
                uint32_t next_cluster;
                fat_error_t fat_res = getNextCluster(iter->current_cluster, &next_cluster);
                
                if (fat_res == FAT_ERROR_END_OF_CHAIN) {
                    uartTxStr("End of dir chain\r\n");
                    return FAT_ERROR_NO_MORE_ENTRIES;
                } else if (fat_res != FAT_SUCCESS) {
                    uartTxStr("Failed to get next cluster in dir\r\n");
                    return fat_res;
                }

                iter->current_cluster = next_cluster;
                iter->current_sector_in_cluster = 0;
            }

            // Read new sector
            uint32_t sector_lba = fatClusterToLba(iter->current_cluster) + iter->current_sector_in_cluster;

            if (sdReadBlock(sector_lba, iter->sector_buffer) == false) {
                uartTxStr("Failed to read dir sector\r\n");
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
            uartTxStr("End of dir marker found\r\n");
            return FAT_ERROR_NO_MORE_ENTRIES;
        }

        // Check for deleted entry
        if (first_byte == 0xE5) {
            continue; // Skip
        }

        // Check for LFN entry
        if ((entry->sfn.DIR_Attr & FAT_ATTR_LFN) == FAT_ATTR_LFN) {
            fat_lfn_dir_entry_t *lfn_entry = &entry->lfn;
            uint8_t order = lfn_entry->LDIR_Ord & ~0x40;
            
            if (order == 0 || order > MAX_LFN_PARTS) {
                iter->lfn_parts_found = 0;
                iter->lfn_checksum = 0;
                iter->assembled_lfn_buffer[0] = '\0';
                // Clear segment buffers
                for (uint8_t i = 0; i < MAX_LFN_PARTS; i++) iter->lfn_buffer[i][0] = '\0';
                uartTxStr("Invalid lfn order");
                continue;
            }

            // // Check if this is the last LFN entry
            if (lfn_entry->LDIR_Ord & 0x40) {
                iter->lfn_checksum =lfn_entry->LDIR_Chksum;
                // Clear segment buffers
                for (uint8_t i = 0; i < MAX_LFN_PARTS; i++) iter->lfn_buffer[i][0] = '\0';
            } else if (iter->lfn_checksum == 0) {
                uartTxStr("Out of order lfn part\r\n");
                continue;
            }

            char *segment_dest = iter->lfn_buffer[order - 1];
            segment_dest[0] = '\0';

            fat_lfn_to_utf8((uint8_t *)lfn_entry->LDIR_Name1, 10, segment_dest, MAX_FILENAME_LEN);
            fat_lfn_to_utf8((uint8_t *)lfn_entry->LDIR_Name2, 12, segment_dest, MAX_FILENAME_LEN);
            fat_lfn_to_utf8((uint8_t *)lfn_entry->LDIR_Name3, 4, segment_dest, MAX_FILENAME_LEN);

            iter->lfn_parts_found++;

            // idfk know anymore its 1am
            continue;
        }

        // If we reached here, it's an SFN entry
        fat_sfn_dir_entry_t *sfn_entry = &entry->sfn;
        // Validate LFN checksum if an LFN sequence was being processed
        if (iter->lfn_parts_found > 0) {
            uint8_t sfn_checksum = fat_sfn_checksum(sfn_entry->DIR_Name);

            if (sfn_checksum == iter->lfn_checksum) {
                iter->assembled_lfn_buffer[0] = '\0';
                for (uint8_t i = 0; i < iter->lfn_parts_found; i++) {
                    strncat(iter->assembled_lfn_buffer, iter->lfn_buffer[i], MAX_FILENAME_LEN - 1 - strlen(iter->assembled_lfn_buffer));
                }
                iter->assembled_lfn_buffer[MAX_FILENAME_LEN-1] = '\0';

                strncpy(file_info_out->filename, iter->assembled_lfn_buffer, MAX_FILENAME_LEN - 1);
                file_info_out->filename[MAX_FILENAME_LEN - 1] = '\0';
            } else {
                // Checksum mismatch, fallback to sfn
                uartTxStr("LFN checksum mismtach, falling back to sfn\r\n");
                fat_sfn_to_string(sfn_entry->DIR_Name, file_info_out->filename);
            }
            
            // Reset LFN state after matching SFN
            iter->lfn_parts_found = 0;
            iter->assembled_lfn_buffer[0] = '0';
            iter->lfn_checksum = 0;
            for (uint8_t i = 0; i < MAX_LFN_PARTS; i++) iter->lfn_buffer[i][0] = '\0';
        } else {
            // No LFN, use SFN
            fat_sfn_to_string(sfn_entry->DIR_Name, file_info_out->filename);
        }

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

fat_error_t fat_read_file(const fat_file_info_t *file_info, uint8_t *buffer, uint32_t bytes_to_read, uint32_t offset_in_file) {
    if (!g_fat32_volume_info.is_initialized) {
        return FAT_ERROR_NOT_INITIALIZED;
    }
    if (file_info == NULL || buffer == NULL) {
        return FAT_ERROR_BAD_FAT_ENTRY;
    }

    // Ensure we dont read beyong the size or requested bytes
    if (offset_in_file >= file_info->file_size) {
        return FAT_SUCCESS;
    }

    uint32_t actual_bytes_to_read = bytes_to_read;
    if (offset_in_file + actual_bytes_to_read > file_info->file_size) {
        actual_bytes_to_read = file_info->file_size - offset_in_file;
    }
    if (actual_bytes_to_read == 0) {
        return FAT_SUCCESS; // Nothing to read
    }

    uint32_t bytes_read_total = 0;
    uint32_t current_cluster = file_info->first_cluster;

    uint32_t sectors_per_cluster = g_fat32_volume_info.sectors_per_cluster;
    uint32_t bytes_per_sector = g_fat32_volume_info.bytes_per_sector;
    uint16_t bytes_per_cluster = sectors_per_cluster * bytes_per_sector;

    uint8_t sector_buffer[bytes_per_sector]; // Temp buffer for reading sectors

    // Seek to the starting cluster and offset within that cluster
    uint32_t cluster_offset_bytes = offset_in_file;
    uint32_t num_clusters_to_skip = cluster_offset_bytes / bytes_per_cluster;

    for (uint32_t i = 0; i < num_clusters_to_skip; i++) {
        uint32_t next_cluster;
        fat_error_t res = getNextCluster(current_cluster, &next_cluster);
        if (res ==  FAT_ERROR_END_OF_CHAIN) {
            uartTxStr("File truncated or corrupted during seek\r\n");
            return FAT_ERROR_READ_FAIL;
        } else if (res != FAT_SUCCESS) {
            uartTxStr("Error seeking to file offset. Cluster read fail.\r\n");
            return res;
        }
        current_cluster = next_cluster;
    }

    // Now the current_cluster is the cluster where the read should start
    // Calculate the byte offset withing the starting cluster.
    uint32_t byte_offset_in_start_cluster = cluster_offset_bytes % bytes_per_cluster;
    uint32_t start_sector_in_cluster = byte_offset_in_start_cluster / bytes_per_sector;
    uint32_t start_byte_in_sector = byte_offset_in_start_cluster % bytes_per_sector;

    // Loop to read data
    while (bytes_read_total < actual_bytes_to_read) {
        uint32_t current_sector_lba = fatClusterToLba(current_cluster) + start_sector_in_cluster;

        // uartTxStr("Reading file data from LBA: "); uartTxHex(current_sector_lba); uartTxStr("\r\n");

        if (!sdReadBlock(current_sector_lba, sector_buffer)) {
            uartTxStr("Error: Failed to read file data sector.\r\n");
            return FAT_ERROR_READ_FAIL;
        }
        
        // Calculate how much data to copy from this sector
        uint32_t bytes_to_copy_from_sector = bytes_per_sector - start_byte_in_sector;
        if (bytes_to_copy_from_sector > (actual_bytes_to_read - bytes_read_total)) {
            bytes_to_copy_from_sector = actual_bytes_to_read - bytes_read_total;
        }

        memcpy(buffer + bytes_read_total, sector_buffer + start_byte_in_sector, bytes_to_copy_from_sector);
        bytes_read_total += bytes_to_copy_from_sector;

        // Prepare for next read
        start_byte_in_sector = 0;
        start_sector_in_cluster++;

        if (start_sector_in_cluster >= sectors_per_cluster) {
            uint32_t next_cluster;
            fat_error_t res = getNextCluster(current_cluster, &next_cluster);
            if (res == FAT_ERROR_END_OF_CHAIN) {
                // Reached end of file (or chain)
                break; // Exit loop, even if actual_bytes_to_read hasn't been met (file was shorter)
            } else if (res != FAT_SUCCESS) {
                uartTxStr("Error getting next cluster for file data.\r\n");
                return res;
            }
            current_cluster = next_cluster;
            start_sector_in_cluster = 0; // Reset sector counter for new cluster
        }

    }

    return FAT_SUCCESS;
}
