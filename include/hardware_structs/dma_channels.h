#ifndef DMA_CHANNELS_H
#define DMA_CHANNELS_H

#include <stdint.h>

#define DMA_BASE 0x50000000

typedef struct {
    uint32_t READ_ADDR;
    uint32_t WRITE_ADDR;
    uint32_t TRANS_COUNT;
    uint32_t CTRL_TRIG;
} dma_channel_al0_t;

typedef struct {
    uint32_t CTRL;
    uint32_t READ_ADDR;
    uint32_t WRITE_ADDR;
    uint32_t TRANS_COUNT_TRIG;
} dma_channel_al1_t;

typedef struct {
    uint32_t CTRL;
    uint32_t TRANS_COUNT;
    uint32_t READ_ADDR;
    uint32_t WRITE_ADDR_TRIG;
} dma_channel_al2_t;

typedef struct {
    uint32_t CTRL;
    uint32_t WRITE_ADDR;
    uint32_t TRANS_COUNT;
    uint32_t READ_ADD_TRIG;
} dma_channel_al3_t;

typedef struct {
    dma_channel_al0_t al0;
    dma_channel_al1_t al1;
    dma_channel_al2_t al2;
    dma_channel_al3_t al3;
} dma_channel_t;

#define dma_channel_num 12
#define dma_channels    ((dma_channel_t*)DMA_BASE)

#endif //DMA_CHANNELS_H