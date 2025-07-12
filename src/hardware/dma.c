#include <hardware/dma.h>
#include <hardware_structs/dma_channels.h>

void dma_transfer_start(void *src, void *dst, size_t n, uint8_t channel) {
    dma_transfer_await(channel); // wait for channel to become not busy
    // use alias 0
    dma_channels[channel].al0.READ_ADDR = (uint32_t)src;
    dma_channels[channel].al0.WRITE_ADDR = (uint32_t)dst;
    dma_channels[channel].al0.TRANS_COUNT = (uint32_t)n;
    dma_channels[channel].al0.CTRL_TRIG = // set up ctrl reg and start transfer
            DMA_CTRL_EN |
            DMA_CTRL_DATA_SIZE_1BYTE |
            DMA_CTRL_INCR_READ |
            DMA_CTRL_INCR_WRITE |
            DMA_CTRL_CHAIN_TO(channel) | // don't chain
            DMA_CTRL_TREQ_SEL(DMA_TREQ_PERMANENT); // do unpaced transfer
}

void dma_transfer_await(uint8_t channel) {
    // use alias 1 just to be safe
    while(dma_channels[channel].al1.CTRL & DMA_CTRL_BUSY);
}