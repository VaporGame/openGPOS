#include "uart.h"
#include "hardware_structs/uart.h"
#include "hardware_structs/io_bank0.h"
#include <stdint.h>

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