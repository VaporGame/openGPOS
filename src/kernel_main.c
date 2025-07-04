#include <stdint.h>
#include <stdbool.h>
#include "hardware_structs/resets.h"
#include "hardware_structs/io_bank0.h"
#include "hardware_structs/sio.h"
#include "hardware_structs/uart.h"
#include "hardware_structs/spi.h"
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

void uart_init(void) {
    uart_hw->IBRD = 78;
    uart_hw->FBRD = 8;
    uart_hw->LCR_H = (0x3 << 5) | (1 << 4);
    uart_hw->LCR = (1 << 9) | (1 << 8) | (1 << 0);    
    //set pins to function 2 (uart)
    io_bank0_hw->gpio[0].CTRL = 2;
    io_bank0_hw->gpio[1].CTRL = 2;
}

void uartTx( unsigned char x) {
    while ((uart_hw->FR & (1 << 5)) != 0);
    uart_hw->DR = x;
}

void uartTxStr(unsigned char *x) {
    while(*x != '\0') {
        uartTx(*x);
        x++;
    }
}

static char uartRx(void) {
    while ((uart_hw->FR & (1 << 4)) != 0);
    return uart_hw->DR;
}

static void *uartRxStr(char *str) { //this is bad
    unsigned int dat;
    int i = 0;
    while (dat != '\r') {
        dat = uartRx();
        str[i++] = dat;
    }
    str[--i] = '\0';
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

void spi_send_byte(uint8_t data) {
    while (!(spi0_hw->SR & (1 << 1))); //wait untill transmit fifo not full
    spi0_hw->DR = data;
}

void spi_rw(char *data, unsigned int len) {
    unsigned int i;
    char str[3];
    for (i = 0; i < len; i++) {
        while (!(spi0_hw->SR & (1 << 1))); //wait untill FIFO is not full
        spi0_hw->DR = data[i];

        while (spi0_hw->SR & (1 << 4)); //wait untill its not busy

        if (spi0_hw->SR & (1 << 2)) { //if receive FIFO is not empty
            data[i] = spi0_hw->DR;
        }
    }
}

void spi_rw_blocking(char *data, unsigned int len) {
    unsigned int i;
    for (i = 0; i < len; i++) {
        while (!(spi0_hw->SR & (1 << 1))); //wait untill FIFO is not full
        //SPI0_SSPDR = data[i];
        spi0_hw->DR = 0xFF;


        while (spi0_hw->SR & (1 << 4)); //wait untill its not busy

        if (spi0_hw->SR & (1 << 2)) { //if receive FIFO is not empty
        //while (!(SPI0_SSPSR & (1 << 2))) {} //if receive FIFO is not empty
            data[i] = spi0_hw->DR;
        }
    }
}

static char buff[256]; //idk

uint8_t spi_read_byte(void) {
    //spi_send_byte(0xFF); //send dummy byte to clock in data
    while (!(spi0_hw->SR & (1 << 2))); //wait untill receive fifo is not empty
    return spi0_hw->DR;
}

uint8_t spi_rw_byte(uint8_t byte) {
    spi0_hw->DR = byte;
    while (spi0_hw->SR & (1 << 4));
    return (uint8_t) spi0_hw->DR;
}

void spi_init(void) {
    //This may not be good, but it works
    spi0_hw->CR0 = (0x7 << 0); //set data size to 8bit
    //frame format is motorola SPI by default
    //assume SPI mode 0
    //SPI0_SSPCR0 = ((1 << 6) | (1 << 7)); //SPO = 0, SPH = 0
    spi0_hw->CPSR = 32;
    //we will want to change this afterwards to 2 to get a 6mhz clock
    //device is master by default
    spi0_hw->CR1 &= ~(1 << 2);
    
    //SOD is only relevant in slave mode
    
    //setup pins
    io_bank0_hw->gpio[16].CTRL = 1; //function 1 SPI MISO
    //IO_BANK0_GPIO05_CTRL = 1; //CS, doing software CS
    io_bank0_hw->gpio[18].CTRL = 1; //SCK
    io_bank0_hw->gpio[19].CTRL = 1; //MOSI
    //set direction for pins 6 and 7
    //SIO_GPIO_OE_SET = (1 << 18);
    //SIO_GPIO_OE_SET = (1 << 19);
    io_bank0_hw->gpio[9].CTRL = 5;
    sio_hw->OE_SET = (1 << 9); //gp9 is gonna be cs

    pads_bank0_hw->gpio[18] = (1 << 1) | (1 << 8); //disable input, output disable is 0 by default
    pads_bank0_hw->gpio[19] = (1 << 1) | (1 << 8);

    spi0_hw->CR1 |= (1 << 1); //enable SSP
}

bool sdInit(void) {
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
    //usb_device_init();
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

    //blink_forever();
}

// Main entry point
int main(void)
{
    kernel_main();
    
    return 0;
}