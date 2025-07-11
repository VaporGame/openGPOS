#ifndef HEXUTILS_H
#define HEXUTILS_H
#include <stdint.h>

void hexToStr(char *str, uint32_t n);
void byteToStr(char *str, uint8_t n);

void byteToDec(char *str, uint8_t n);
void intToDec(char *str, uint32_t n);
#endif //HEXUTILS_H