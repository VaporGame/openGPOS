#include <stdint.h>
#include <stdbool.h>
#include "hardware_structs/resets.h"
#include "hardware_structs/io_bank0.h"
#include "hardware_structs/sio.h"
#include "hardware_structs/dma_channels.h"
#include "uart/uart.h"
#include "spi/spi.h"
#include "sd/SD.h"
#include "util/hexutils.h"
#include "fs/FAT32.h"
#include <libc/stdlib.h>
#include "dma/dma.h"
#include "elf/elf.h"
#include <libc/unistd.h>

typedef void (*EntryFunction_t)(void);

// static void blink_forever(void) {
//     io_bank0_hw->gpio[25].CTRL = 5;
//     sio_hw->OE_SET |= 1 << 25; // Set output enable for GPIO 25 in SIO

//     while (1)
//     {
//         usleep(500000); // Wait for 0.5sec
//         sio_hw->OUT_XOR |= 1 << 25;  // Flip output for GPIO 25
//     }
// }

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
    // uartTxStr("initializing SPI\r\n");
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
    //const char *buf = "Hello from userland!";
    //write(1, buf, 20);
    // while(1);
    
    // uint32_t file_id = fat32_open("/dir1/dir2/file.txt", 0);
    // uint8_t *buffer1 = (uint8_t *)malloc(sizeof(uint8_t) * 15);
    // buffer1[14] = '\0';
    // bool result = fat32_read(file_id, buffer1, 14);

    // uartTxStr("\r\n");
    // if (result) {
    //     uartTxStr(buffer1);
    // } else {
    //     uartTxStr("unable to read file");
    // }
    // uartTxStr("\r\n");

    // free(buffer1);
    // bool res = fat32_close(file_id);

    

    //uartTxDec(result);

    // const char* message = "Hello, World!";
    // char* buf = malloc(14);
    // dma_transfer_start(message, buf, 14, 0);
    // dma_transfer_await(0);
    // uartTxStr(buf);
    // TODO: make memory manip functions use DMA

    // uartTxStr("Bringing SD card into idle state\r\n");
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