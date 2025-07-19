#ifndef M0PLUS_H
#define M0PLUS_H
#include <stdint.h>
#include "hardware_structs/platform_defs.h"

// #define PPB_BASE     0xe0000000

typedef struct {
    io_reg_32 CSR;
    io_reg_32 RVR;
    io_reg_32 CVR;
    io_reg_32 CALIB;
} syst_hw_t;
#define SYST_BASE   0xe0000010
#define syst_hw     ((syst_hw_t *)SYST_BASE)

typedef struct {
    io_reg_32 ISER;
    io_reg_32 gap1[0x1F]; //0x104 - 0x180
    io_reg_32 ICER;
    io_reg_32 gap2[0x1F]; // 0x184 - 0x200
    io_reg_32 ISPR;
    io_reg_32 gap3[0x1F];
    io_reg_32 ICPR;
    io_reg_32 gap4[0x5F];
    io_reg_32 IPR[8];
} nvic_hw_t;
#define NVIC_BASE   0xe0000100
#define nvic_hw     ((nvic_hw_t *)NVIC_BASE)

typedef struct {
    io_reg_32 SHPR2;
    io_reg_32 SHPR3;
} scb_hw_t;
#define SCB_BASE    0xe000ED1C
#define scb_hw      ((scb_hw_t *)SCB_BASE)

#endif //M0PLUS_H