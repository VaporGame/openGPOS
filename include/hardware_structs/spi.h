#ifndef HARDWARE_STRUCTS_SPI_H
#define HARDWARE_STRUCTS_SPI_H
#include <stdint.h>
#include "hardware_structs/platform_defs.h"

#define SPI0_BASE   0x4003c000

typedef struct {
    io_reg_32 CR0;
    io_reg_32 CR1;
    io_reg_32 DR;
    io_reg_32 SR;
    io_reg_32 CPSR;
    // I'll add the rest when i need it
} spi0_hw_t;
#define spi0_hw     ((spi0_hw_t *)SPI0_BASE)

#endif //HARDWARE_STRUCTS_SPI_H