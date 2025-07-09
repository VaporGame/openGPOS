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
#include <libc/stdlib.h>

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

    init_malloc();

    fat32_init();
    fat_directory_iterator_t *root_dir_iter = malloc(sizeof(fat_directory_iterator_t));
    if (root_dir_iter == NULL) {
        uartTxStr("Failed to allocate root_dir_iter\r\n");
        return;
    }
    fat_init_root_dir_iterator(root_dir_iter, 2); // TODO: make this use actual root dir sector instead of assuming 2


    fat_file_info_t *my_file = malloc(sizeof(fat_file_info_t));
    if (my_file == NULL) {
        uartTxStr("Failed to allocate my_file\r\n");
        return;
    }

    fat_error_t result = FAT_SUCCESS;
    uartTxStr("\r\n");
    while (result != FAT_ERROR_NO_MORE_ENTRIES) {
        result = fat_read_next_dir_entry(root_dir_iter, my_file);

        if (result == FAT_SUCCESS) {
            if (my_file->is_directory) {
                uartTxStr("dir "); uartTxStr(my_file->filename); uartTxStr("\r\n");
                continue;
            } else {
                uartTxStr("file "); uartTxStr(my_file->filename); uartTx(' '); uartTxDec(my_file->file_size); uartTxStr("b\r\n"); 
            }

            uint32_t file_size = my_file->file_size;

            uint8_t file_content_buffer[file_size];
            uint32_t read_size = file_size;
            if (my_file->file_size < read_size) {
                read_size = my_file->file_size;
            }

            fat_error_t read_res = fat_read_file(my_file, file_content_buffer, read_size, 0);
            if (read_res == FAT_SUCCESS) {
                uartTxStr("content:\r\n");

                for (uint32_t i = 0; i < read_size; i++) {
                    if (file_content_buffer[i] >= 32 && file_content_buffer[i] <= 126) {
                        uartTx(file_content_buffer[i]);
                    } else {
                        uartTx('?');
                    }
                }
                uartTxStr("\r\n\r\n");
            } else {
                uartTxStr("Failed to read file\r\n");
            }

        } else if (result != FAT_ERROR_NO_MORE_ENTRIES) {
            uartTxDec(result);
        }
    }

    free(root_dir_iter);
    free(my_file);

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