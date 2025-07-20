#include "util/hexutils.h"
#include <stdint.h>
#include <stdbool.h>

// void hexToStr(char *str, uint32_t n) {
//     int i, hb;

//     for (i = 0; i < 8; i++) {
//         hb = n >> (7 - i) * 4;
//         hb &= 0x0F;

//         if (hb > 9) {
//             str[i] = (char)(hb + 'A' - 10);
//         } else {
//             str[i] = (char)(hb + '0');
//         }
//     }
//     str[8] = 0;
// }

static const char hex_digits[] = "0123456789ABCDEF";

void hexToStr(char *str, uint32_t n) {
    for (uint8_t i = 0; i < 8; i++) {
        str[i] = hex_digits[(n >> (28 - i * 4)) & 0xF];
    }
    str[8] = '\0';
}

// void byteToStr(char *str, uint8_t n) {
//     unsigned char nb;

//     nb = (n >> 4) & 0x0F;
//     if(nb < 10) {
//         str[0] = nb + '0';
//     } else {
//         str[0] = nb - 10 + 'A';
//     }

//     nb = n & 0x0F;
//     if(nb < 10) {
//         str[1] = nb + '0';
//     } else {
//         str[1] = nb - 10 + 'A';
//     }
//     str[2] = '\0';
// }

void byteToStr(char *str, uint8_t n) {
    str[0] = hex_digits[(n >> 4) & 0x0F];
    str[1] = hex_digits[n & 0x0F];
    str[2] = '\0';
}

void byteToDec(char *str, uint8_t n) {
    str[2] = n % 10 + '0';
    str[1] = (n / 10) % 10 + '0';
    str[0] = (n / 100) % 10 + '0';
    str[3] = '\0';
}

void intToDec(char* str, uint32_t n) { // Trims zeroes
    if (n == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }

    int8_t i = 0;
    char temp_str[12];

    while (n > 0) {
        temp_str[i++] = (n % 10) + '0';
        n /= 10;
    }

    uint8_t j = 0;
    while (i > 0) {
        str[j++] = temp_str[--i];
    }
    str[j] = '\0';
}

extern uint32_t read_le32(const uint8_t *data, const uint16_t offset);
extern uint16_t read_le16(const uint8_t *data, const uint16_t offset);