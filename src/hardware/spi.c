#include "hardware/spi.h"
#include "hardware_structs/spi.h"
#include "hardware_structs/io_bank0.h"
#include "hardware_structs/pads_bank0.h"
#include "hardware_structs/sio.h"
//#include "hardware/uart.h"

void spi_send_byte(uint8_t data) {
    while (!(spi0_hw->SR & (1 << 1))); //wait untill transmit fifo not full
    spi0_hw->DR = data;
}

void spi_rw(char *data, unsigned int len) {
    unsigned int i;
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
    spi0_hw->CPSR = 254;
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

void spi_set_speed(uint8_t divisor) {
    spi0_hw->CPSR = divisor;
}