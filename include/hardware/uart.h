#ifndef UART_H
#define UART_H
#include <stdint.h>

void uart_init(uint32_t baud_rate);

void uartTx( unsigned char x);
void uartTxStr(unsigned char *x);

char uartRx(void);
void uartRxStr(char *str);

#endif //UART_H