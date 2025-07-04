#ifndef RESETS_H
#define RESETS_H
#include <stdint.h>
#include "hardware_structs/platform_defs.h"

#define RESETS_BASE     0x4000c000

typedef struct {
    io_reg_32 RESET;
    io_reg_32 WDSEL; //watchdog select
    io_reg_32 RESET_DONE;
} resets_hw_t;
#define resets_hw       ((resets_hw_t *)RESETS_BASE)

#endif //RESETS_H