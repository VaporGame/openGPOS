#ifndef CLOCKS_H
#define CLOCKS_H
#include <stdint.h>
#include "hardware_structs/platform_defs.h"

#define CLOCKS_BASE     0x40008000

typedef struct {
    io_reg_32 GPOUT0_CTRL; //00
    io_reg_32 GPOUT0_DIV; //04
    io_reg_32 GPOUT0_SELECTED; //08
    io_reg_32 GPOUT1_CTRL; //0c
    io_reg_32 GPOUT1_DIV; //10
    io_reg_32 GPOUT1_SELECTED; //14
    io_reg_32 GPOUT2_CTRL; //18
    io_reg_32 GPOUT2_DIV; //1c
    io_reg_32 GPOUT2_SELECTED; //20
    io_reg_32 GPOUT3_CTRL; //24
    io_reg_32 GPOUT3_DIV; //28
    io_reg_32 GPOUT3_SELECTED; //2c

    io_reg_32 REF_CTRL; //30
    io_reg_32 REF_DIV; //34
    io_reg_32 REF_SELECTED; //38

    io_reg_32 SYS_CTRL; //3c
    io_reg_32 SYS_DIV; //40
    io_reg_32 SYS_SELECTED; //44

    io_reg_32 PERI_CTRL; //48
    io_reg_32 _gap;
    io_reg_32 PERI_DIV; //50
    io_reg_32 PERI_SELECTED; //54
} clocks_hw_t;
#define clocks_hw       ((clocks_hw_t *)CLOCKS_BASE)

#define XOSC_BASE       0x40024000
#define XOSC_HZ         12000000  // Crystal Oscillator Frequency

typedef struct {
    io_reg_32 CTRL;
    io_reg_32 STATUS;
    io_reg_32 DORMANT;
    io_reg_32 STARTUP;
    io_reg_32 COUNT;
} xosc_hw_t;
#define xosc_hw         ((xosc_hw_t *)XOSC_BASE)

#define ROSC_BASE       0x40060000

typedef struct {
    io_reg_32 CTRL;
    //This is the only field i really care about;
} rosc_hw_t;
#define rosc_hw         ((xosc_hw_t *)ROSC_BASE)

#define PLL_SYS_BASE    0x40028000

typedef struct {
    io_reg_32 CS;
    io_reg_32 PWR;
    io_reg_32 FBDIV_INT;
    io_reg_32 PRIM;
} pll_sys_hw_t;
#define pll_sys_hw      ((pll_sys_hw_t *)PLL_SYS_BASE)

#define WATCHDOG_BASE   0x40058000

typedef struct {
    io_reg_32 _reserved[11];
    io_reg_32 TICK;
    //I only care about tick
} watchdog_hw_t;
#define watchdog_hw     ((watchdog_hw_t *)WATCHDOG_BASE)

#define TIMER_BASE      0x40054000

typedef struct {
    io_reg_32 HW;
    io_reg_32 LW;
    io_reg_32 HR;
    io_reg_32 LR;
    //This is all i need
} timer_hw_t;
#define timer_hw       ((timer_hw_t *)TIMER_BASE)

#endif //CLOCKS_H