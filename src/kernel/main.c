#include <stdint.h>
#include <stdbool.h>
#include "hardware_structs/resets.h"
#include "uart/uart.h"
#include "spi/spi.h"
#include "sd/SD.h"
#include "fs/FAT32.h"
#include <libc/stdlib.h>
#include "elf/elf.h"
#include <libc/unistd.h>

typedef void (*EntryFunction_t)(void);

static void resetSubsys(void) {
    resets_hw->RESET &= ~(1 << 5); // Bring IO_BANK0 out of reset state
    while (!(resets_hw->RESET_DONE & (1 << 5))); // Wait for peripheral to respond

    resets_hw->RESET &= ~(1 << 8); // pads bank
    while (!(resets_hw->RESET_DONE & (1 << 8))); // Wait for peripheral to respond

    resets_hw->RESET &= ~(1 << 22); //UART0
    while (!(resets_hw->RESET_DONE & (1 << 22))); // Wait for peripheral to respond

    resets_hw->RESET &= ~(1 << 16); //SPI0
    while (!(resets_hw->RESET_DONE & (1 << 16))); // Wait for peripheral to respond

    resets_hw->RESET &= ~(1 << 2); //DMA
    while (!(resets_hw->RESET_DONE & (1 << 2))); // Wait for peripheral to respond
}

static void kernel_main(void) {
    resetSubsys();
    uart_init(115200);
    for(uint8_t i = 0; i < 80; i++) {
        uartTx('\n');
    }
    uartTxStr("\rfreeGPOS starting\r\n");
    spi_init();

    if(!SDInit()) {
        uartTxStr("Failed to initialize SD card\r\n");
        return;
    }
    init_malloc();
    fat32_init();

    uint32_t entry = loadELF("/bin/init.elf");
    EntryFunction_t start = (EntryFunction_t)(uintptr_t)entry;

    start();
    // const char *buf = "Hello from userland!";
    // write(1, buf, 20);
    
    while(!SDShutdown()) {
        usleep(100000);
    }
    for (;;);
}

// Main entry point
int main(void)
{
    kernel_main();
    
    return 0;
}