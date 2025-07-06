#include "SD.h"
#include <stdbool.h>
#include <stdint.h>
#include "hardware/uart.h"
#include "hardware/spi.h"
#include "hardware_structs/sio.h"
#include "hexutils.h"

extern void usSleep(uint64_t us);

static void SDSendCommand(uint8_t command, uint32_t args, uint8_t crc) {
    sio_hw->OUT_CLR |= 1 << 9;
    spi_send_byte(command);
    spi_send_byte((args >> 24) & 0xFF);
    spi_send_byte((args >> 16) & 0xFF);
    spi_send_byte((args >> 8) & 0xFF);
    spi_send_byte(args & 0xFF);
    spi_send_byte(crc);
    spi_send_byte(0xFF); // Dummy clock
}

static uint8_t read_r1_response(uint16_t timeout_val) {
    uint8_t res = 0xFF;
    uint16_t timeout = timeout_val;

    do {
        res = spi_read_byte();
        spi_send_byte(0xFF);
        timeout--;
    } while (res == 0xFF && timeout > 0);

    return res;
}

static void read_bytes(uint8_t *buffer, uint8_t count) {
    for (uint8_t i = 0; i < count; i++) {
        buffer[i] = spi_read_byte();
        spi_send_byte(0xFF);
    }
}

static bool read_data_block(uint8_t *buffer, uint16_t count, uint16_t timeout_val) {
    uint16_t timeout = timeout_val;
    uint8_t token = 0xFF;

    //wait for data start token (0xFE)
    do {
        token = spi_read_byte();
        spi_send_byte(0xFF);
        timeout--;
    } while (token == 0xFF && timeout > 0);

    if (timeout == 0) {
        uartTxStr("Data block read timeout (no start token)\r\n");
        return false;
    }
    if (token != 0xFE) {
        uartTxStr("Unexpected data start token\r\n");
        return false;
    }

    for (uint16_t i = 0; i < count; i++) {
        buffer[i] = spi_read_byte();
        spi_send_byte(0xFF);
    }

    uint8_t crc_low = spi_read_byte();
    spi_send_byte(0xFF);
    uint8_t crc_high = spi_read_byte();
    spi_send_byte(0xFF);

    // CRC validation can be put here if needed

    return true;
} 

bool SDInit(void) {
    uartTxStr("initializing SD card ");
    usSleep(10000); //10ms, let sd card stabilize

    sio_hw->OUT_SET |= 1 << 9;
    for(uint8_t i = 0; i < 10; i++) {
        spi_send_byte(0xFF);
    }

    SDSendCommand(0x40, 0x00000000, 0x95);
    uint8_t cmd0_r1 = read_r1_response(0xFF);
    
    if (cmd0_r1 == 0x01) {
        uartTxStr("[OK]\r\n");
    } else {
        sio_hw->OUT_SET |= 1 << 9;
        spi_send_byte(0xFF);

        uartTxStr("[FAIL]\r\n");
        return false;
    }

    // check if card is SDv2+
    // CMD8

    SDSendCommand(0x48, 0x000001AA, 0x87);
    uint8_t CMD8_r1 = read_r1_response(0xFF);
    uint8_t r7_data[4];

    if (CMD8_r1 == 0x1) {
        read_bytes(r7_data, 4);
        
        sio_hw->OUT_SET |= 1 << 9; // Deassert CS
        spi_send_byte(0xFF); // Dummy clock

        if(r7_data[2] == 0x01 && r7_data[3] == 0xAA) {
            uartTxStr("CMD8 OK\r\n");
        } else {
            uartTxStr("CMD8 R1 OK, but incorrect data pattern. Card unusable\r\n");
            sio_hw->OUT_SET |= 1 << 9; // Deassert CS
            spi_send_byte(0xFF); // Dummy clock
            return false;
        }

    } else if (CMD8_r1 == 0x05) {
        uartTxStr("Unsupported card\r\n");
        sio_hw->OUT_SET |= 1 << 9; // Deassert CS
        spi_send_byte(0xFF); // Dummy clock
        return false;
    } else {
        uartTxStr("CMD8 Unexpected R1 response\r\n");
        sio_hw->OUT_SET |= 1 << 9; // Deassert CS
        spi_send_byte(0xFF); // Dummy clock
        return false;
    }

    //send CMD55 with dummy crc
    //send ACMD41 with dummy crc
    //repeat for 5 seconds or untill card responds with 0x00 (not in idle state)

    uint16_t acmd41_timeout = 5000; //could be decreased to 1000
    uint8_t acmd41_r1 = 0xFF;

    do {
        SDSendCommand(0x77, 0x00000000, 0x65); //CMD55
        uint8_t cmd55_r1 = read_r1_response(0xFF);
        sio_hw->OUT_SET |= 1 << 9; // Deassert CS
        spi_send_byte(0xFF); // Dummy clock

        if (cmd55_r1 != 0x01) {
            uartTxStr("CMD55 unexpected R1 response\r\n");
            uartTxStr("Aborting ACMD41 loop\r\n");
            return false;
        }

        //i only support HC/HX cards for now
        SDSendCommand(0x69, 0x40000000, 0x77);
        acmd41_r1 = read_r1_response(0xFF);
        sio_hw->OUT_SET |= 1 << 9; // Deassert CS
        spi_send_byte(0xFF); // Dummy clock

        if (acmd41_r1 == 0x00) {
            uartTxStr("ACMD41 OK: Card is ready\r\n");
            break;
        }

        acmd41_timeout--;
        usSleep(1000);
    } while (acmd41_timeout > 0);

    if (acmd41_timeout == 0) {
        uartTxStr("ACMD41 FAIL (timeout)\r\n");
        return false;
    }
    
    // read operating conditions register to confirm card capacity status
    // CMD58

    SDSendCommand(0x7A, 0x00000000, 0x01);
    uint8_t cmd58_r1 = read_r1_response(0xFF);
    uint8_t ocr_data[4];

    if (cmd58_r1 == 0x00) {
        //read the 4 OCR bytes
        for (uint8_t i = 0; i < 4; i++) {
            ocr_data[i] = spi_read_byte();
            spi_send_byte(0xFF);
        }
        sio_hw->OUT_SET |= 1 << 9;
        spi_send_byte(0xFF);
        
        if (ocr_data[0] & 0x40) {
            uartTxStr("CMD58 OK: Card is SDHC/SDXC\r\n");
        } else {
            uartTxStr("CMD58 OK: Card is Standard Capacity, unsupported, this should never happen");
            return false;
        }
    } else {
        uartTxStr("CMD58 FAIL");
        return false;
    }
    return true;
}

bool SDReadCSD(uint8_t *csd_buffer) {
    uartTxStr("Reading CSD register...\r\n");

    // CMD9 (SEND_CSD)
    SDSendCommand(0x49, 0x00000000, 0x01);

    uint8_t cmd9_r1 = read_r1_response(0xFF);
    if (cmd9_r1 != 0x00) {
        uartTxStr("CMD9 FAIL\r\n");
        sio_hw->OUT_SET |= 1 << 9;
        spi_send_byte(0xFF);
        return false;
    }

    // Read 16-byte CSD data block
    if (!read_data_block(csd_buffer, 16, 0xFFFF)) {
        uartTxStr("Failed to read CSD data block\r\n");
        sio_hw->OUT_SET |= 1 << 9;
        spi_send_byte(0xFF);
        return false;
    }

    sio_hw->OUT_SET |= 1 << 9;
    spi_send_byte(0xFF);

    // uartTxStr("CSD Data (16 bytes)\r\n");
    // char hex_buf[3];
    // for (uint8_t i = 0; i < 16; i++) {
    //     byteToStr(hex_buf, csd_buffer[i]);
    //     uartTxStr(hex_buf);
    //     uartTx(' ');
    // }
    // uartTxStr("\r\n");

    //Parse CSD to get card capacity
    if (((csd_buffer[0] >> 6) & 0x03) == 0x1) { // CSD_STRUCTURE == 1 (CSD Version 2.0)
        uint32_t c_size = ((csd_buffer[7] & 0x3F) << 16) | (csd_buffer[8] << 8) | csd_buffer[9];

        uint32_t capacity_bytes = (c_size + 1) / 2; // 512KB blocks
        uartTxStr("Calculated Capacity: ");
        char hex2[11];
        intToDec(hex2, capacity_bytes);
        uartTxStr(hex2);
        uartTxStr(" MB\r\n");
    } else {
        uartTxStr("CSD Version 1.0 detected (or unknown). Capacity calculation differs.\r\n");
    }

    return true;
}

bool sdReadBlock(uint32_t block_address, uint8_t *data_buffer) {
    uartTxStr("reading block");

    // CMD17 (READ_SINGLE_BLOCK)
    // for SDHC/SDXC, block address is already in 512 byte units
    SDSendCommand(0x51, block_address, 0x01); //dummy crc

    uint8_t cmd17_r1 = read_r1_response(0xFF);
    if (cmd17_r1 != 0x00) {
        uartTxStr("CMD17 FAIL\r\n");
        sio_hw->OUT_SET |= 1 << 9; // Deassert CS
        spi_send_byte(0xFF);
        return false;
    }

    if (!read_data_block(data_buffer, 512, 0xFFFF)) {
        uartTxStr("Failed to read data block\r\n");
        sio_hw->OUT_SET |= 1 << 9; // Deassert CS
        spi_send_byte(0xFF);
        return false;
    }

    sio_hw->OUT_SET |= 1 << 9; // Deassert CS
    spi_send_byte(0xFF);

    uartTxStr("Block data (first 16 bytes):\r\n");
    char hex_buf[3];
    for (uint8_t i = 0; i < 16; i++) {
        byteToStr(hex_buf, data_buffer[i]);
        uartTxStr(hex_buf);
        uartTx(' ');
    }
    uartTxStr("\r\n");

    return true;
}