#ifndef SPI_H
#define SPI_H
#include <stdint.h>

void spi_send_byte(uint8_t data);
uint8_t spi_read_byte(void);

void spi_rw(char *data, unsigned int len);
void spi_rw_blocking(char *data, unsigned int len);
uint8_t spi_rw_byte(uint8_t byte);

void spi_init(void);
void spi_set_speed(uint8_t divisor);

#endif //SPI_H