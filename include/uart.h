#ifndef UART_H
#define UART_H

void uart_init(void);

void uartTx( unsigned char x);
void uartTxStr(unsigned char *x);

char uartRx(void);
void uartRxStr(char *str);

#endif //UART_H