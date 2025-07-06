#ifndef SD_H
#define SD_H
#include <stdbool.h>
#include <stdint.h>

bool SDInit(void);

bool SDReadCSD(uint8_t *csd_buffer);
bool sdReadBlock(uint32_t block_address, uint8_t *data_buffer);

#endif //SD_H