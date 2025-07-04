#ifndef SIO_H
#define SIO_H
#include <stdint.h>
#include "hardware_structs/platform_defs.h"

#define SIO_BASE        0xd0000000

typedef struct {
    io_reg_32 _reserved[3]; //CPUID, GPIO_IN, GPIO_HI_IN
    io_reg_32 _gap; //gap

    io_reg_32 OUT;
    io_reg_32 OUT_SET;
    io_reg_32 OUT_CLR;
    io_reg_32 OUT_XOR;
    io_reg_32 OE;
    io_reg_32 OE_SET;
    io_reg_32 OE_CLR;
    io_reg_32 OE_XOR;

    // I'll add the rest when i need it
} sio_hw_t;
#define sio_hw          ((sio_hw_t *)SIO_BASE)

#endif //SIO_H