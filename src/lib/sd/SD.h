#ifndef SD_H
#define SD_H
#include <stdbool.h>
#include <stdint.h>

#define SD_BLOCK_SIZE 512

bool SDInit(void);

//uint32_t SDReadCSD(uint8_t *csd_buffer);
bool sdReadBlock(uint32_t block_address, uint8_t *data_buffer);
bool sdWriteBlock(uint32_t block_address, uint8_t *data_buffer);

bool SDShutdown(void);

bool sdReadPartialBlock(uint32_t block_number, uint16_t offset, uint8_t* buffer, uint16_t length);

#endif //SD_H