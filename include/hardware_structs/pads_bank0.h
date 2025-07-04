#ifndef PADS_BANK0_H
#define PADS_BANK0_H
#include <stdint.h>
#include "hardware_structs/platform_defs.h"

#define PADS_BANK0_BASE   0x4001c000

typedef struct {
    io_reg_32 VOLATAGE_SELECT;
    io_reg_32 gpio[30];
    // I'll add the rest when i need it
} pads_bank0_hw_t;
#define pads_bank0_hw     ((pads_bank0_hw_t *)PADS_BANK0_BASE)

#endif //PADS_BANK0_H