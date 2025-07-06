#include <stdint.h>
#include <stdbool.h>
#include "hardware_structs/resets.h"
#include "hardware_structs/io_bank0.h"
#include "hardware_structs/sio.h"
#include "hardware/uart.h"
#include "hardware/spi.h"
#include "SD.h"
#include "hexutils.h"

// Declare usSleep function
extern void usSleep(uint64_t us);

static void blink_forever(void) {
    io_bank0_hw->gpio[25].CTRL = 5;
    sio_hw->OE_SET |= 1 << 25; // Set output enable for GPIO 25 in SIO

    while (1)
    {
        usSleep(500000); // Wait for 0.5sec
        //uartTx('A');
        sio_hw->OUT_XOR |= 1 << 25;  // Flip output for GPIO 25
    }
}

static void resetSubsys(void) {
    resets_hw->RESET &= ~(1 << 5); // Bring IO_BANK0 out of reset state
    while (!(resets_hw->RESET_DONE & (1 << 5))); // Wait for peripheral to respond

    resets_hw->RESET &= ~(1 << 8); // pads bank
    while (!(resets_hw->RESET_DONE & (1 << 8))); // Wait for peripheral to respond

    resets_hw->RESET &= ~(1 << 22); //UART0
    while (!(resets_hw->RESET_DONE & (1 << 22))); // Wait for peripheral to respond

    resets_hw->RESET &= ~(1 << 16); //SPI0
    while (!(resets_hw->RESET_DONE & (1 << 16))); // Wait for peripheral to respond
}

static int strlen(char *str) {
    int i = 0;
    while(str[i] != '\0') {
        i++;
    }
    return i;
}

static void kernel_main(void) {
    resetSubsys();
    uart_init(115200);
    uartTxStr("\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n"); //clear screen
    uartTxStr("freeGPOS starting\r\n");
    uartTxStr("initializing SPI\r\n");
    spi_init();

    if(!SDInit()) {
        uartTxStr("Failed to initialize SD card\r\n");
        uartTxStr("(rebooting a few times can fix the issue)\r\n");
        uartTxStr("Cannot continue booting\r\n");
        return;
    }

    uint8_t csd_buf[16];

    SDReadCSD(csd_buf);

    uint8_t block_data[512];

    if(sdReadBlock(0, block_data)) {
        char hex[3];
        uartTxStr("Block 0 read sucessfully\r\n");
        byteToStr(hex, block_data[510]);
        uartTxStr(hex);
        uartTxStr("\r\n");
        byteToStr(hex, block_data[511]);
        uartTxStr(hex);
    } else {
        uartTxStr("Failed to read block 0\r\n");
    }

    while (true) {}
}

// Main entry point
int main(void)
{
    kernel_main();
    
    return 0;
}