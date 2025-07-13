#ifndef HARDWARE_STRUCTS_UART_H
#define HARDWARE_STRUCTS_UART_H
#include <stdint.h>
#include "hardware_structs/platform_defs.h"

#define UART_BASE   0x40034000

typedef struct {
    io_reg_32 DR; //00
    io_reg_32 RSR; //unused //04
    io_reg_32 _gap0[4];

    io_reg_32 FR; //18
    io_reg_32 _gap1;
    io_reg_32 ILPR; //unused //20
    io_reg_32 IBRD; //24
    io_reg_32 FBRD; //28
    io_reg_32 LCR_H; //2c
    io_reg_32 LCR; //30

    // I'll add the rest when i need it
} uart_hw_t;
#define uart_hw     ((uart_hw_t *)UART_BASE)

#endif //HARDWARE_STRUCTS_UART_H