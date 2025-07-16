#include "uart/uart.h"
#include <libc/stdlib.h>
#include "dma/dma.h"

int main(void) {
    const char *message = "Hello from RAM!";
    char *string = malloc(sizeof(char) * 26);
    dma_transfer_start(message, string, 26, 0);
    dma_transfer_await(0);
    uartTxStr(string);
    // uartTxStr("Test");
}