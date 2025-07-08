#ifndef SD_H
#define SD_H
#include <stdbool.h>
#include <stdint.h>

#define SD_BLOCK_SIZE 512

bool SDInit(void);

bool SDReadCSD(uint8_t *csd_buffer);
bool sdReadBlock(uint32_t block_address, uint8_t *data_buffer);
bool sdWriteBlock(uint32_t block_address, uint8_t *data_buffer);

bool SDShutdown(void);

#endif //SD_H