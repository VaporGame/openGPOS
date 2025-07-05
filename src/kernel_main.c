#include <stdint.h>
#include <stdbool.h>
#include "hardware_structs/resets.h"
#include "hardware_structs/io_bank0.h"
#include "hardware_structs/sio.h"
#include "uart.h"

//TODO: make this compile

#include "spi.h"
#include "hardware_structs/pads_bank0.h"

// Declare usSleep function
extern void usSleep(uint64_t us);

// Global variable counting how many times LED switched state
uint8_t blinkCnt;

void blink_forever(void) {
    io_bank0_hw->gpio[25].CTRL = 5;
    sio_hw->OE_SET |= 1 << 25; // Set output enable for GPIO 25 in SIO

    while (++blinkCnt < 21)
    //while (1)
    {
        usSleep(500000); // Wait for 0.5sec
        //uartTx('A');
        sio_hw->OUT_XOR |= 1 << 25;  // Flip output for GPIO 25
    }
}

void resetSubsys(void) {
    resets_hw->RESET &= ~(1 << 5); // Bring IO_BANK0 out of reset state
    while (!(resets_hw->RESET_DONE & (1 << 5))); // Wait for peripheral to respond

    resets_hw->RESET &= ~(1 << 8); // pads bank
    while (!(resets_hw->RESET_DONE & (1 << 8))); // Wait for peripheral to respond

    resets_hw->RESET &= ~(1 << 22); //UART0
    while (!(resets_hw->RESET_DONE & (1 << 22))); // Wait for peripheral to respond

    resets_hw->RESET &= ~(1 << 16); //SPI0
    while (!(resets_hw->RESET_DONE & (1 << 16))); // Wait for peripheral to respond
}

static void hexToStr(char *str, int n) {
    int i, hb;

    for (i = 0; i < 8; i++) {
        hb = n >> (7 - i) * 4;
        hb &= 0x0F;

        if (hb > 9) {
            str[i] = (char)(hb + 'A' - 10);
        } else {
            str[i] = (char)(hb + '0');
        }
        str[8] = 0;
    }
}

static void byteToStr(char *str, int n) {
    unsigned char nb;

    nb = (n >> 4) & 0x0F;
    if(nb < 10) {
        str[0] = nb + '0';
    } else {
        str[0] = nb - 10 + 'A';
    }

    nb = n & 0x0F;
    if(nb < 10) {
        str[1] = nb + '0';
    } else {
        str[1] = nb - 10 + 'A';
    }
    str[2] = '\0';
}

bool sdInit(void) {
    static char buff[6]; //idk

    usSleep(10000); //10ms, let sd card stabilize

    sio_hw->OUT_SET |= 1 << 9;

    for(uint8_t i = 0; i < 10; i++) {
        spi_send_byte(0xFF);
    }

    //spi_rw_blocking(buff, 80);

    buff[0] = 0x40;
    buff[1] = 0x00;
    buff[2] = 0x00;
    buff[3] = 0x00;
    buff[4] = 0x00;
    buff[5] = 0x95;
    
    sio_hw->OUT_CLR |= 1 << 9;

    for(uint8_t i = 0; i < 6; i++) {
        spi_send_byte(buff[i]);
    }

    uint16_t timeout = 0xFFF;
    uint8_t res;

    do {
        res = spi_read_byte();
        // byteToStr(str, res);
        // uartTxStr(str);
        spi_send_byte(0xFF);
        timeout--;
    } while((res != 0x1) && timeout > 0);

    if (timeout > 0) {
        uartTxStr("[OK]\r\n");
        return true;
    }

    sio_hw->OUT_SET |= 1 << 9;
    spi_send_byte(0xFF);

    uartTxStr("[FAIL]\r\n");
    return true;
}

int strlen(char *str) {
    int i = 0;
    while(str[i] != '\0') {
        i++;
    }
    return i;
}

void kernel_main(void) {
    resetSubsys();
    uart_init();
    uartTxStr("\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n"); //clear screen
    uartTxStr("freeGPOS starting\r\n");
    uartTxStr("initializing SPI\r\n");
    spi_init();
    uartTxStr("initializing SD card ");
    sdInit();

    char str[100];
    char hex[3];

    while (true) {
        uartRxStr(str);
        //uartTxStr(str);
        uartTxStr("\r\n");
        int len = strlen(str);
        sio_hw->OUT_CLR |= 1 << 9;
        spi_rw_blocking(str, len);
        for(int i = 0; i < len; i++) {
            byteToStr(hex, str[i]);
            uartTxStr(hex);
        }
        sio_hw->OUT_SET |= 1 << 9;
        uartTxStr("\r\n");
    }
}

// Main entry point
int main(void)
{
    kernel_main();
    
    return 0;
}