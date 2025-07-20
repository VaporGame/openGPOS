#ifndef HEXUTILS_H
#define HEXUTILS_H
#include <stdint.h>

void hexToStr(char *str, uint32_t n);
void byteToStr(char *str, uint8_t n);

void byteToDec(char *str, uint8_t n);
void intToDec(char *str, uint32_t n);

inline uint32_t read_le32(const uint8_t *data, const uint16_t offset) {
    return data[offset]         |
           (data[offset + 1] << 8)  |
           (data[offset + 2] << 16) |
           (data[offset + 3] << 24);
}
inline uint16_t read_le16(const uint8_t *data, const uint16_t offset) {
    return data[offset]         |
           (data[offset + 1] << 8);
}
#endif //HEXUTILS_H