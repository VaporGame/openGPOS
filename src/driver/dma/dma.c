#include <dma/dma.h>
#include <hardware_structs/dma_channels.h>

extern void usSleep(uint64_t us);

dma_channel_config_t dma_get_default_config(uint8_t channel) {
    return  DMA_CTRL_DATA_SIZE_1BYTE |
            DMA_CTRL_INCR_READ |
            DMA_CTRL_INCR_WRITE |
            DMA_CTRL_CHAIN_TO(channel) | // don't chain
            DMA_CTRL_TREQ_SEL(DMA_TREQ_PERMANENT); // do unpaced transfer
};

void dma_config_set_data_size(dma_channel_config_t *config, uint8_t size) {
    *config = (*config & ~(0b11 << 2)) | (size & 0b11) << 2;
};

void dma_config_set_read_increment(dma_channel_config_t *config, bool set) {
    *config = (*config & ~DMA_CTRL_INCR_READ) | (set ? DMA_CTRL_INCR_READ : 0);
};

void dma_config_set_write_increment(dma_channel_config_t *config, bool set) {
    *config = (*config & ~DMA_CTRL_INCR_WRITE) | (set ? DMA_CTRL_INCR_WRITE : 0);
};

void dma_config_set_chain(dma_channel_config_t *config, uint8_t channel) {
    *config = (*config & ~(0b1111 << 11)) | (channel & 0b1111) << 11;
};

void dma_config_set_treq(dma_channel_config_t *config, uint8_t treq) {
    *config = (*config & ~(0b111111 << 15)) | (treq & 0b111111) << 15;
};

void dma_config_set_ring(dma_channel_config_t *config, bool write, uint8_t exponent) {
    *config = (*config & ~(0b1111 << 6)) | (exponent & 0b1111) << 6; // set exponent
    *config = (*config & ~DMA_CTRL_RING_SEL) | (write ? DMA_CTRL_RING_SEL : 0); // set ring selection
}

void dma_channel_configure(uint8_t channel, dma_channel_config_t config, const void *src, void *dst, size_t n, bool start) {
    dma_transfer_await(channel);

    dma_channels[channel].al0.READ_ADDR = (uint32_t)src;
    dma_channels[channel].al0.WRITE_ADDR = (uint32_t)dst;
    dma_channels[channel].al0.TRANS_COUNT = (uint32_t)n;
    if(start)
        dma_channels[channel].al0.CTRL_TRIG = config | DMA_CTRL_EN;
    else
        dma_channels[channel].al1.CTRL = config | DMA_CTRL_EN;
}

bool dma_channel_busy(uint8_t channel) {
    return dma_channels[channel].al1.CTRL & DMA_CTRL_BUSY;
};

void dma_channel_start(uint8_t channel) {
    *dma_chann_trig = 1 << (channel & 0b1111);
};

void dma_transfer_start(const void *src, void *dst, size_t n, uint8_t channel) {
    dma_transfer_await(channel); // wait for channel to become not busy

    dma_channel_config_t config =
            DMA_CTRL_DATA_SIZE_1BYTE |
            DMA_CTRL_INCR_READ |
            DMA_CTRL_INCR_WRITE |
            DMA_CTRL_CHAIN_TO(channel) | // don't chain
            DMA_CTRL_TREQ_SEL(DMA_TREQ_PERMANENT); // do unpaced transfer
    

    dma_channel_configure(channel, config, src, dst, n, true);
}

void dma_transfer_await(uint8_t channel) {
    // use alias 1 just to be safe
    while(dma_channel_busy(channel)) {usSleep(1000);}
}