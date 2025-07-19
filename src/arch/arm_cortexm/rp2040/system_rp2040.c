#include <stdint.h>
#include "hardware_structs/clocks.h"
#include "hardware_structs/resets.h"
#include "hardware_structs/m0plus.h"

#define SYSTICK_RELOAD_VAL ((CLK_SYS_HZ / (1000 / 10)) - 1)

#define PRIORITY_0_HIGHEST  0x00UL // Mapped to 0x00 in the 8-bit field
#define PRIORITY_1          0x40UL // Mapped to 0x40 in the 8-bit field
#define PRIORITY_2          0x80UL // Mapped to 0x80 in the 8-bit field
#define PRIORITY_3_LOWEST   0xC0UL // Mapped to 0xC0 in the 8-bit field

void SystemInit()
{
    // Initialize XOSC
    xosc_hw->CTRL = 0xaa0; // This makes it work
    xosc_hw->CTRL |= (0xfab << 12); // Enable XOSC
    while (!(xosc_hw->STATUS & (1 << 31))); // Wait for XOSC to stabilize

    // Initialize System PLL
    resets_hw->RESET &= ~(1 << 12);
    while (!(resets_hw->RESET_DONE & (1 << 12))); // Wait for PLL peripheral to respond
    pll_sys_hw->FBDIV_INT = 100; // Set feedback clock div = 100, thus VCO clock = 12MHz * 100 = 1.2GHz
    pll_sys_hw->PWR &= ~((1 << 0) | (1 << 5)); // Turn on the main power and VCO
    while (!(pll_sys_hw->CS & (1 << 31))); // Wait for PLL to lock
    // pll_sys_hw->PRIM = (6 << 16) | (2 << 12); // Set POSTDIV1 = 6 and POSTDIV2 = 2, thus 1.2GHz / 6 / 2 = 100MHz
    pll_sys_hw->PRIM = (6 << 16) | (1 << 12); // Set POSTDIV1 = 6 and POSTDIV2 = 2, thus 1.2GHz / 6 / 1 = 200MHz
    pll_sys_hw->PWR &= ~(1 << 3); // Turn on the post dividers

    // Setup clock generators
    // Setup clk_ref
    clocks_hw->REF_CTRL |= (2 << 0); // Switch clk_ref glitchless mux to XOSC_CLKSRC for the best accuracy possible
    while (!(clocks_hw->REF_SELECTED & (1 << 2)));// Make sure that the switch happened
    // Setup clk_sys
    clocks_hw->SYS_CTRL |= (1 << 0); // Switch clk_sys glitchless mux to CLKSRC_CLK_SYS_AUX and the aux defaults to CLKSRC_PLL_SYS
    while (!(clocks_hw->SYS_SELECTED & (1 << 1)));// Make sure that the switch happened
    //setup clk_peri
    // clocks_hw->PERI_CTRL |= ((1 << 11) | (4 << 5));
    clocks_hw->PERI_CTRL = (1 << 11);
    //while (!(CLOCKS_PERI_SELECTED & (4 << 5)));

    // Shut down ROSC
    rosc_hw->CTRL = (rosc_hw->CTRL & (~0x00fff000)) | (0xd1e << 12);

    // Enable 64-bit Timer
    watchdog_hw->TICK |= (12 << 0); // Set appropriate value for TICK, 1 us = 12 cycles / 12MHz
    resets_hw->RESET &= ~(1 << 21); // Bring 64-bit Timer out of reset state
    while (!(resets_hw->RESET_DONE & (1 << 21))); // Wait for TIMER peripheral to respond

    // Set up NVIC
    // SVC
    scb_hw->SHPR2 = (scb_hw->SHPR2 & ~(0xFF << 0)) | (PRIORITY_0_HIGHEST << 0);
    // SysTick
    scb_hw->SHPR3 = (scb_hw->SHPR3 & ~(0xFF << 24)) | (PRIORITY_2 << 24);
    // PendSV
    scb_hw->SHPR3 = (scb_hw->SHPR3 & ~(0xFF << 16)) | (PRIORITY_3_LOWEST << 16);

    // Set up SysTick
    syst_hw->RVR = SYSTICK_RELOAD_VAL; // 10ms ticks
    syst_hw->CVR = 0;
    syst_hw->CSR |= (1 << 2) | (1 << 1) | (1 << 0); 
    // IMPORTANT: Enable global interrupts when ready to start processing interrupts.
}