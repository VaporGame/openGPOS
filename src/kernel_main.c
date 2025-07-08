#include <stdint.h>
#include <stdbool.h>
#include "hardware_structs/resets.h"
#include "hardware_structs/io_bank0.h"
#include "hardware_structs/sio.h"
#include "hardware/uart.h"
#include "hardware/spi.h"
#include "SD.h"
#include "hexutils.h"
#include "fs/FAT32.h"

// Declare usSleep function
extern void usSleep(uint64_t us);

static void blink_forever(void) {
    io_bank0_hw->gpio[25].CTRL = 5;
    sio_hw->OE_SET |= 1 << 25; // Set output enable for GPIO 25 in SIO

    while (1)
    {
        usSleep(500000); // Wait for 0.5sec
        //uartTx('A');
        sio_hw->OUT_XOR |= 1 << 25;  // Flip output for GPIO 25
    }
}

static void resetSubsys(void) {
    resets_hw->RESET &= ~(1 << 5); // Bring IO_BANK0 out of reset state
    while (!(resets_hw->RESET_DONE & (1 << 5))); // Wait for peripheral to respond

    resets_hw->RESET &= ~(1 << 8); // pads bank
    while (!(resets_hw->RESET_DONE & (1 << 8))); // Wait for peripheral to respond

    resets_hw->RESET &= ~(1 << 22); //UART0
    while (!(resets_hw->RESET_DONE & (1 << 22))); // Wait for peripheral to respond

    resets_hw->RESET &= ~(1 << 16); //SPI0
    while (!(resets_hw->RESET_DONE & (1 << 16))); // Wait for peripheral to respond
}

static void kernel_main(void) {
    resetSubsys();
    uart_init(115200);
    uartTxStr("\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n"); //clear screen
    uartTxStr("freeGPOS starting\r\n");
    uartTxStr("initializing SPI\r\n");
    spi_init();

    if(!SDInit()) {
        uartTxStr("Failed to initialize SD card\r\n");
        uartTxStr("(rebooting a few times can fix the issue)\r\n");
        uartTxStr("Cannot continue booting\r\n");
        return;
    }
    
    fat32_init();
    fat_directory_iterator_t root_dir_iter;
    fat_init_root_dir_iterator(&root_dir_iter, 2); // TODO: make this use actual root dir sector instead of assuming 2

    fat_file_info_t my_file;
    fat_error_t result = fat_read_next_dir_entry(&root_dir_iter, &my_file);

    if (result == FAT_SUCCESS) {
        uartTxStr("Found file: "); uartTxStr(my_file.filename);
        uartTx(' '); uartTxDec(my_file.file_size); uartTxStr(" bytes\r\n");

        uint8_t file_content_buffer[5];
        uint8_t read_size = 4;
        if (my_file.file_size < read_size) {
            read_size = my_file.file_size;
        }

        fat_error_t read_res = fat_read_file(&my_file, file_content_buffer, read_size, 0);
        if (read_res == FAT_SUCCESS) {
            uartTxStr("File content: ");

            for(uint32_t i = 0; i < read_size; i++) {
                if (file_content_buffer[i] >= 32 && file_content_buffer[i] <= 126) {
                    uartTx(file_content_buffer[i]);
                } else {
                    uartTx('?');
                }
            }
            uartTxStr("\r\n");
        } else {
            uartTxStr("Failed to read file\r\n");
        }

    }

    // while((result ) == FAT_SUCCESS) {
    //     uartTxStr(file_info.filename);
    //     uartTx(' ');
    //     uartTxStr(file_info.is_directory ? "DIR " : "FILE ");
    //     uartTxDec(file_info.file_size);
    //     uartTx(' ');
    //     uartTxHex(file_info.first_cluster);
    //     uartTxStr("\r\n");
    // }

    // Show file pls

    

    //uartTxDec(result);

    uartTxStr("Bringing SD card into idle state\r\n");
    while(!SDShutdown()) {
        usSleep(100000);
    }
    while (true) {}
}

// Main entry point
int main(void)
{
    kernel_main();
    
    return 0;
}