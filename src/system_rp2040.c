#include <stdint.h>
#include "hardware_structs/clocks.h"
#include "hardware_structs/resets.h"

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
    pll_sys_hw->PRIM = (6 << 16) | (2 << 12); // Set POSTDIV1 = 6 and POSTDIV2 = 2, thus 1.2GHz / 6 / 2 = 100MHz
    pll_sys_hw->PWR &= ~(1 << 3); // Turn on the post dividers

    // Setup clock generators
    // Setup clk_ref
    clocks_hw->REF_CTRL |= (2 << 0); // Switch clk_ref glitchless mux to XOSC_CLKSRC for the best accuracy possible
    while (!(clocks_hw->REF_SELECTED & (1 << 2)));// Make sure that the switch happened
    // Setup clk_sys
    clocks_hw->SYS_CTRL |= (1 << 0); // Switch clk_sys glitchless mux to CLKSRC_CLK_SYS_AUX and the aux defaults to CLKSRC_PLL_SYS
    while (!(clocks_hw->SYS_SELECTED & (1 << 1)));// Make sure that the switch happened
    //setup clk_peri
    clocks_hw->PERI_CTRL |= ((1 << 11) | (4 << 5));
    //while (!(CLOCKS_PERI_SELECTED & (4 << 5)));

    // Shut down ROSC
    rosc_hw->CTRL = (rosc_hw->CTRL & (~0x00fff000)) | (0xd1e << 12);

    // // Enable 64-bit Timer
    watchdog_hw->TICK |= (12 << 0); // Set appropriate value for TICK, 1 us = 12 cycles / 12MHz
    resets_hw->RESET &= ~(1 << 21); // Bring 64-bit Timer out of reset state
    while (!(resets_hw->RESET_DONE & (1 << 21))); // Wait for TIMER peripheral to respond
}

uint64_t readTime()
{
    uint32_t timeLR = timer_hw->LR;
    uint32_t timeHR = timer_hw->HR;
    return (((uint64_t)timeHR << 32) | timeLR);
}

void usSleep(uint64_t us)
{
    uint64_t timeOld = readTime(); // Get current timer value
    while ((readTime() - timeOld) < us); // Wait till desired time is passed
}