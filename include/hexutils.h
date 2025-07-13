#ifndef HEXUTILS_H
#define HEXUTILS_H
#include <stdint.h>

void hexToStr(char *str, uint32_t n);
void byteToStr(char *str, uint8_t n);

void byteToDec(char *str, uint8_t n);
void intToDec(char *str, uint32_t n);

uint32_t read_le32(const uint8_t *data, uint16_t offset);
uint16_t read_le16(const uint8_t *data, uint16_t offset);
#endif //HEXUTILS_H