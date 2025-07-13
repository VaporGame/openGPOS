#ifndef IO_BANK0_H
#define IO_BANK0_H
#include <stdint.h>
#include "hardware_structs/platform_defs.h"

#define IO_BANK0_BASE   0x40014000

typedef struct {
    struct {
        io_reg_32 STATUS;
        io_reg_32 CTRL;
    } gpio[30];

    // I'll add the rest when i need it
} io_bank0_hw_t;
#define io_bank0_hw     ((io_bank0_hw_t *)IO_BANK0_BASE)

#endif //IO_BANK0_H