#ifndef DMA_H
#define DMA_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// transfer request signals
#define DMA_DREQ_PIO0_TX0   0
#define DMA_DREQ_PIO0_TX1   1
#define DMA_DREQ_PIO0_TX2   2
#define DMA_DREQ_PIO0_TX3   3
#define DMA_DREQ_PIO0_RX0   4
#define DMA_DREQ_PIO0_RX1   5
#define DMA_DREQ_PIO0_RX2   6
#define DMA_DREQ_PIO0_RX3   7
#define DMA_DREQ_PIO1_TX0   8
#define DMA_DREQ_PIO1_TX1   9
#define DMA_DREQ_PIO1_TX2   10
#define DMA_DREQ_PIO1_TX3   11
#define DMA_DREQ_PIO1_RX0   12
#define DMA_DREQ_PIO1_RX1   13
#define DMA_DREQ_PIO1_RX2   14
#define DMA_DREQ_PIO1_RX3   15
#define DMA_DREQ_SPI0_TX    16
#define DMA_DREQ_SPI0_RX    17
#define DMA_DREQ_SPI1_TX    18
#define DMA_DREQ_SPI1_RX    19
#define DMA_DREQ_UART0_TX   20
#define DMA_DREQ_UART0_RX   21
#define DMA_DREQ_UART1_TX   22
#define DMA_DREQ_UART1_RX   23
#define DMA_DREQ_PWM_WRAP0  24
#define DMA_DREQ_PWM_WRAP1  25
#define DMA_DREQ_PWM_WRAP2  26
#define DMA_DREQ_PWM_WRAP3  27
#define DMA_DREQ_PWM_WRAP4  28
#define DMA_DREQ_PWM_WRAP5  29
#define DMA_DREQ_PWM_WRAP6  30
#define DMA_DREQ_PWM_WRAP7  31
#define DMA_DREQ_I2C0_TX    32
#define DMA_DREQ_I2C0_RX    33
#define DMA_DREQ_I2C1_TX    34
#define DMA_DREQ_I2C1_RX    35
#define DMA_DREQ_ADC        36
#define DMA_DREQ_XIP_STREAM 37
#define DMA_DREQ_XIP_SSITX  38
#define DMA_DREQ_XIP_SSIRX  39
#define DMA_TREQ_TIMER0     0x3B
#define DMA_TREQ_TIMER1     0x3C
#define DMA_TREQ_TIMER2     0x3D
#define DMA_TREQ_TIMER3     0x3E
#define DMA_TREQ_PERMANENT  0x3F // use this if you want an unpaced transfer

// dma data size
#define DMA_SIZE_8BIT 0
#define DMA_SIZE_16BIT 1
#define DMA_SIZE_32BIT 2



typedef uint32_t dma_channel_config_t;

// simple interface for now, TODO expand later
dma_channel_config_t dma_get_default_config(uint8_t channel);
void dma_config_set_data_size(dma_channel_config_t *config, uint8_t size);
void dma_config_set_read_increment(dma_channel_config_t *config, bool set);
void dma_config_set_write_increment(dma_channel_config_t *config, bool set);
void dma_config_set_chain(dma_channel_config_t *config, uint8_t channel);
void dma_config_set_treq(dma_channel_config_t *config, uint8_t treq);
void dma_config_set_ring(dma_channel_config_t *config, bool write, uint8_t exponent); // the actual size is 2^exponent, 0 for no ring

void dma_channel_configure(uint8_t channel, dma_channel_config_t config, const void *src, void *dst, size_t n, bool start);
bool dma_channel_busy(uint8_t channel);

void dma_transfer_start(const void *src, void *dst, size_t n, uint8_t channel);
void dma_transfer_await(uint8_t channel);

#endif //DMA_H