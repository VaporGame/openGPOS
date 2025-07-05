#include "hardware/uart.h"
#include "hardware_structs/uart.h"
#include "hardware_structs/io_bank0.h"
#include "hardware_structs/clocks.h"
#include <stdint.h>
#include "hexutils.h"

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

char uartRx(void) {
    while ((uart_hw->FR & (1 << 4)) != 0);
    return uart_hw->DR;
}

void uartRxStr(char *str) {
    unsigned int dat;
    int i = 0;
    while (dat != '\r') {
        dat = uartRx();
        str[i++] = dat;
    }
    str[--i] = '\0';
}

void uart_init(uint32_t baud_rate) {
    // taken from sdk
    // currently the peripheral clock runs from the sys clock, which runs at 200mhz

    uint32_t baud_rate_div = (8 * CLK_SYS_HZ / baud_rate);
    uint32_t ibrd = baud_rate_div >> 7; //divide by 128
    uint32_t fbrd;
    if(ibrd == 0) { //should not happen
        ibrd = 1;
        fbrd = 0;
    } else {
        fbrd = ((baud_rate_div & 0x7f) + 1) >> 1; // (remainder + 0.5 * 128) / 2 = remainder / 2 + 32
    }

    uart_hw->IBRD = ibrd;
    uart_hw->FBRD = fbrd;

    uart_hw->LCR_H = (0x3 << 5) | (1 << 4);
    uart_hw->LCR = (1 << 9) | (1 << 8) | (1 << 0);    
    //set pins to function 2 (uart)
    io_bank0_hw->gpio[0].CTRL = 2;
    io_bank0_hw->gpio[1].CTRL = 2;
}