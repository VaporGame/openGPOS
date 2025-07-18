#ifndef UART_H
#define UART_H
#include <stdint.h>
#include <stddef.h>

void uart_init(uint32_t baud_rate);

void uartTx( unsigned char x);
void uartTxStr(unsigned const char *x);

char uartRx(void);
void uartRxStr(char *str);

void uartTxHexByte(uint8_t byte);
void uartTxDecByte(uint8_t byte);

void uartTxHex(uint32_t n);
void uartTxDec(uint32_t n);

void uart_puts(const char *str, size_t len);

#endif //UART_H