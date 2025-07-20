#include "sd/SD.h"
#include <stdbool.h>
#include <stdint.h>
#include "uart/uart.h"
#include "spi/spi.h"
#include "hardware_structs/sio.h"
#include "util/hexutils.h"
#include <libc/unistd.h>

static void deassertCS() {
    sio_hw->OUT_SET |= 1 << 9; // Deassert CS
    spi_send_byte(0xFF); // Dummy clock
}

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

static bool read_data_block(uint8_t *buffer, const uint16_t timeout_val) {
    uint16_t timeout = timeout_val;
    uint8_t token = 0xFF;

    //wait for data start token (0xFE)
    do {
        token = spi_read_byte();
        spi_send_byte(0xFF);
        timeout--;
    } while (token == 0xFF && timeout > 0);

    if (timeout == 0) {
        return false;
    }
    if (token != 0xFE) {
        return false;
    }

    for (uint16_t i = 0; i < SD_BLOCK_SIZE; i++) {
        *buffer = spi_read_byte();
        buffer++;
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
    usleep(10000); //10ms, let sd card stabilize

    sio_hw->OUT_SET |= 1 << 9;
    for(uint8_t i = 0; i < 10; i++) {
        spi_send_byte(0xFF);
    }

    SDSendCommand(0x40, 0x00000000, 0x95);
    uint8_t cmd0_r1 = read_r1_response(0xFF);
    
    if (cmd0_r1 != 0x01) {
        deassertCS();
        return false;
    }

    // check if card is SDv2+
    // CMD8

    SDSendCommand(0x48, 0x000001AA, 0x87);
    uint8_t CMD8_r1 = read_r1_response(0xFF);
    uint8_t r7_data[4];

    if (CMD8_r1 == 0x1) {
        read_bytes(r7_data, 4);
        
        deassertCS();

        if(r7_data[2] == 0x01 && r7_data[3] == 0xAA) {
        } else {
            return false;
        }

    } else if (CMD8_r1 == 0x05) {
        uartTxStr("Unsupported card\r\n");
        deassertCS();
        return false;
    } else {
        deassertCS();
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
        deassertCS();

        if (cmd55_r1 != 0x01) {
            return false;
        }

        //i only support HC/HX cards for now
        SDSendCommand(0x69, 0x40000000, 0x77);
        acmd41_r1 = read_r1_response(0xFF);
        deassertCS();

        if (acmd41_r1 == 0x00) {
            break;
        }

        acmd41_timeout--;
        usleep(1000);
    } while (acmd41_timeout > 0);

    if (acmd41_timeout == 0) {
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
        
        if (!(ocr_data[0] & 0x40)) {
            deassertCS();
            return false;
        }
    } else {
        deassertCS();
        return false;
    }
    spi_set_speed(8); // Can maybe go to 4 (50MHz)
    return true;
}

// uint32_t SDReadCSD(uint8_t *csd_buffer) {
//     //uartTxStr("Reading CSD register...\r\n");

//     // CMD9 (SEND_CSD)
//     SDSendCommand(0x49, 0x00000000, 0x01);

//     uint8_t cmd9_r1 = read_r1_response(0xFF);
//     if (cmd9_r1 != 0x00) {
//         // uartTxStr("CMD9 FAIL\r\n");
//         deassertCS();
//         return 0;
//     }

//     // Read 16-byte CSD data block
//     if (!read_data_block(csd_buffer, 16, 0xFFFF)) {
//         // uartTxStr("Failed to read CSD data block\r\n");
//         deassertCS();
//         return 0;
//     }

//     deassertCS();

//     //Parse CSD to get card capacity
//     if (((csd_buffer[0] >> 6) & 0x03) == 0x1) { // CSD_STRUCTURE == 1 (CSD Version 2.0)
//         uint32_t c_size = ((csd_buffer[7] & 0x3F) << 16) | (csd_buffer[8] << 8) | csd_buffer[9];

//         uint32_t capacity_bytes = (c_size + 1) / 2; // 512KB blocks
//         return capacity_bytes;

//     }

//     return 0;
// }

// TODO: IMPLEMENT MULTI BLOCK READING SD COMMANDS

bool sdReadBlock(uint32_t block_address, uint8_t *data_buffer) {
    // CMD17 (READ_SINGLE_BLOCK)
    // for SDHC/SDXC, block address is already in 512 byte units
    SDSendCommand(0x51, block_address, 0x01); //dummy crc

    uint8_t cmd17_r1 = read_r1_response(0xFF);
    if (cmd17_r1 != 0x00) {
        deassertCS();
        return false;
    }

    if (!read_data_block(data_buffer, 0xFFFF)) {
        deassertCS();
        return false;
    }

    deassertCS();
    return true;
}

static bool sdWaitForNotBusy(uint32_t timeout_ms) {
    uint32_t timeout_counter = 0; // Or use actual time if available
    const uint32_t MAX_TIMEOUT_COUNT = 1000000; // Adjust as needed for your clock speed

    while (spi_read_byte() == 0x00) { // SD card holds DO low (0x00) when busy
        timeout_counter++;
        if (timeout_counter > MAX_TIMEOUT_COUNT) {
            return false; // Timeout occurred
        }
    }
    return true; // SD card is no longer busy
}

bool sdWriteBlock(uint32_t block_address, uint8_t *data_buffer) {
    // CMD24 (READ_SINGLE_BLOCK)
    // for SDHC/SDXC, block address is already in 512 byte units
    SDSendCommand(0x58, block_address, 0x01); //dummy crc

    uint8_t cmd24_r1 = read_r1_response(0xFF);
    if (cmd24_r1 != 0x00) {
        deassertCS();
        return false;
    }

    // Send start block token
    spi_send_byte(0xFE);

    for (uint16_t i = 0; i < SD_BLOCK_SIZE; i++) {
        spi_send_byte(data_buffer[i]);
    }

    spi_send_byte(0xFF); //MSB of dummy CRC
    spi_send_byte(0xFF); //LSB of dummy CRC

    uint8_t data_response;

    for(uint16_t i = 0; i < 1000; i++) {
        data_response = spi_read_byte();
        if (data_response != 0xFF) break;
    }

    if ((data_response & 0x1F) != 0x05) { // Check for data accepted (0b00000101)
        deassertCS();
        return false;
    }

    if (!sdWaitForNotBusy(500)) {
        deassertCS();
        return false;
    }

    deassertCS();

    return true;
}

bool SDShutdown(void) {
    SDSendCommand(0x40, 0x00000000, 0x95);
    uint8_t cmd0_r1 = read_r1_response(0xFF);
    
    if (cmd0_r1 == 0x01) {
        deassertCS();
        return true;
    } else {
        return false;
    }
}

static bool read_data_block_partial(uint16_t start_offset, uint8_t *buffer, uint16_t count, uint16_t timeout_val) {
    uint16_t timeout = timeout_val;
    uint8_t token = 0xFF;

    //wait for data start token (0xFE)
    do {
        token = spi_read_byte();
        spi_send_byte(0xFF);
        timeout--;
    } while (token == 0xFF && timeout > 0);

    if (timeout == 0 || token != 0xFE) {
        return false;
    }

    // Read and discard bytes until start_offset
    for (uint16_t i = 0; i < start_offset; i++) {
        spi_read_byte();
        spi_send_byte(0xFF);
    }

    // Read requested bytes into buffer
    for (uint16_t i = 0; i < count; i++) {
        buffer[i] = spi_read_byte();
        spi_send_byte(0xFF);
    }

    // Read and discard remaining bytes in the 512-byte block + CRC
    // Total bytes to discard = 512 - start_offset - count + 2 (for CRC)
    uint16_t bytes_to_discard = (SD_BLOCK_SIZE - start_offset - count) + 2; // +2 for CRC
    for (uint16_t i = 0; i < bytes_to_discard; i++) {
        spi_read_byte();
        spi_send_byte(0xFF);
    }
    
    // CRC validation can be put here if needed, if you read them into variables
    // For this example, they are just discarded.

    return true;
}

bool sdReadPartialBlock(uint32_t block_number, uint16_t offset, uint8_t* buffer, uint16_t length) {
    // CMD17 (READ_SINGLE_BLOCK)
    SDSendCommand(0x51, block_number, 0x01); //dummy crc

    uint8_t cmd17_r1 = read_r1_response(0xFF);
    if (cmd17_r1 != 0x00) {
        deassertCS();
        return false;
    }

    // Use the modified read_data_block_partial
    if (!read_data_block_partial(offset, buffer, length, 0xFFFF)) {
        deassertCS();
        return false;
    }

    deassertCS();
    return true;
}