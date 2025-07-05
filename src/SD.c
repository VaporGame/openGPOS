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

bool sdInit(void) {
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
    //repeat for 1 second or untill card responds with 0x00 (not in idle state)

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
    
    return true;
}