#include "hexutils.h"
#include <stdint.h>

void hexToStr(char *str, uint8_t n) {
    int i, hb;

    for (i = 0; i < 8; i++) {
        hb = n >> (7 - i) * 4;
        hb &= 0x0F;

        if (hb > 9) {
            str[i] = (char)(hb + 'A' - 10);
        } else {
            str[i] = (char)(hb + '0');
        }
        str[8] = 0;
    }
}

void byteToStr(char *str, uint8_t n) {
    unsigned char nb;

    nb = (n >> 4) & 0x0F;
    if(nb < 10) {
        str[0] = nb + '0';
    } else {
        str[0] = nb - 10 + 'A';
    }

    nb = n & 0x0F;
    if(nb < 10) {
        str[1] = nb + '0';
    } else {
        str[1] = nb - 10 + 'A';
    }
    str[2] = '\0';
}