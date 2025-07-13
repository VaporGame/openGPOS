#ifndef DMA_CHANNELS_H
#define DMA_CHANNELS_H

#include <stdint.h>

#define DMA_BASE 0x50000000

// control register fields
#define DMA_CTRL_EN              (1 << 0)
#define DMA_CTRL_HIGH_PRIO       (1 << 1)
#define DMA_CTRL_DATA_SIZE_1BYTE (0 << 2)
#define DMA_CTRL_DATA_SIZE_2BYTE (1 << 2)
#define DMA_CTRL_DATA_SIZE_4BYTE (2 << 2)
#define DMA_CTRL_INCR_READ       (1 << 4)
#define DMA_CTRL_INCR_WRITE      (1 << 5)
#define DMA_CTRL_RING_SEL        (1 << 10)
#define DMA_CTRL_CHAIN_TO(x)     (x << 11)
#define DMA_CTRL_TREQ_SEL(x)     (x << 15)
#define DMA_CTRL_IRQ_QUIET       (1 << 21)
#define DMA_CTRL_BSWAP           (1 << 22)
#define DMA_CTRL_SNIFF_EN        (1 << 23)
#define DMA_CTRL_BUSY            (1 << 24)
#define DMA_CTRL_WRITE_ERROR     (1 << 29)
#define DMA_CTRL_READ_ERROR      (1 << 30)
#define DMA_CTRL_AHB_ERROR       (1 << 31)

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
#define dma_intr        ((uint32_t*)(DMA_BASE + 0x400))
#define dma_chann_trig  ((uint32_t*)(DMA_BASE + 0x430))

#endif //DMA_CHANNELS_H