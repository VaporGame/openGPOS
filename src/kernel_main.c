#include <stdint.h>
#include <stdbool.h>

typedef volatile uint32_t io_rw_32;

// Define necessary register addresses
#define RESETS_RESET                                    *(volatile uint32_t *) (0x4000c000)
#define RESETS_RESET_DONE                               *(volatile uint32_t *) (0x4000c008)
#define IO_BANK0_BASE                                   0x40014000
#define IO_BANK0_GPIO00_CTRL                            *(volatile uint32_t *) (IO_BANK0_BASE + 0x04)
#define IO_BANK0_GPIO01_CTRL                            *(volatile uint32_t *) (IO_BANK0_BASE + 0x0C)
#define IO_BANK0_GPIO04_CTRL                            *(volatile uint32_t *) (IO_BANK0_BASE + 0x24)
#define IO_BANK0_GPIO05_CTRL                            *(volatile uint32_t *) (IO_BANK0_BASE + 0x2c)
#define IO_BANK0_GPIO06_CTRL                            *(volatile uint32_t *) (IO_BANK0_BASE + 0x34)
#define IO_BANK0_GPIO07_CTRL                            *(volatile uint32_t *) (IO_BANK0_BASE + 0x3c)
#define IO_BANK0_GPIO09_CTRL                            *(volatile uint32_t *) (IO_BANK0_BASE + 0x4c)

#define IO_BANK0_GPIO16_CTRL                            *(volatile uint32_t *) (IO_BANK0_BASE + 0x84)
#define IO_BANK0_GPIO18_CTRL                            *(volatile uint32_t *) (IO_BANK0_BASE + 0x94)
#define IO_BANK0_GPIO19_CTRL                            *(volatile uint32_t *) (IO_BANK0_BASE + 0x9c)

#define IO_BANK0_GPIO25_CTRL                            *(volatile uint32_t *) (IO_BANK0_BASE + 0xCC)
#define SIO_GPIO_OE_SET                                 *(volatile uint32_t *) (0xd0000024)
#define SIO_GPIO_OUT_XOR                                *(volatile uint32_t *) (0xd000001c)
#define SIO_GPIO_OUT_SET                                *(volatile uint32_t *) (0xd0000014)
#define SIO_GPIO_OUT_CLR                                *(volatile uint32_t *) (0xd0000018)

// #define USB_BASE                                        0x50110000
// #define USB_MAIN_CTRL                                   *(volatile uint32_t *) (USB_BASE + 0x40)
// #define USB_MUXING                                      *(volatile uint32_t *) (USB_BASE + 0x74)
// #define USB_PWR                                         *(volatile uint32_t *) (USB_BASE + 0x78)
// #define USB_SIE_CTRL                                    *(volatile uint32_t *) (USB_BASE + 0x4c)
// #define USB_SIE_STATUS                                  *(volatile uint32_t *) (USB_BASE + 0x50)
// #define USB_INTE                                        *(volatile uint32_t *) (USB_BASE + 0x90)
// #define USB_INTS                                        *(volatile uint32_t *) (USB_BASE + 0x98)

// #define USB_NUM_ENDPOINTS 16

// #define USB_DPRAM                                       0x50100000
// #define USB_DPRAM_SIZE                                  4096 //bytes
// #define USB_SETUP_PACKET                                *(volatile uint32_t *) (USB_BASE) //8 bytes

// #define USB_EP_CTRL_OUT1                                *(volatile uint32_t *) (USB_BASE + 0x000c)
// #define USB_EP_CTRL_IN2                                 *(volatile uint32_t *) (USB_BASE + 0x0010)

// #define USB_EP_BUF_CTRL_OUT1                            *(volatile uint32_t *) (USB_BASE + 0x000c + ((USB_NUM_ENDPOINTS-1) * 8) + 0x0008)
// #define USB_EP_BUF_CTRL_IN2                             *(volatile uint32_t *) (USB_BASE + 0x0008 + ((USB_NUM_ENDPOINTS-1) * 8) + 0x0010)

// #define USB_EPX_DATA                                    *(volatile uint32_t *) (USB_BASE + 0x0140 + (USB_NUM_ENDPOINTS * 8))
// #define USB_EPX_DATA_OUT1                               *(volatile uint32_t *) (USB_EPX_DATA)
// #define USB_EPX_DATA_IN2                                *(volatile uint32_t *) (USB_EPX_DATA + 0x40)

// #define NVIC_BASE                                       0xe0000000
// #define NVIC_ISER                                       *(volatile uint32_t *) (NVIC_BASE + 0xe100)

// Declare usSleep function
extern void usSleep(uint64_t us);

// Global variable counting how many times LED switched state
uint8_t blinkCnt;

void blink_forever(void) {
    IO_BANK0_GPIO25_CTRL = 5; // Set GPIO 25 function to SIO
    SIO_GPIO_OE_SET |= 1 << 25; // Set output enable for GPIO 25 in SIO

    while (++blinkCnt < 21)
    //while (1)
    {
        usSleep(500000); // Wait for 0.5sec
        //uartTx('A');
        SIO_GPIO_OUT_XOR |= 1 << 25;  // Flip output for GPIO 25
    }
}

// void usb_device_init(void) {
//     //bring the USB peripheral out of reset
//     RESETS_RESET &= ~(1 << 24);

//     //clear dpram
//     volatile uint32_t *mem_ptr = (volatile uint32_t *) USB_DPRAM;
//     uint32_t num_words = USB_DPRAM_SIZE / sizeof(uint32_t);
//     for(uint32_t i = 0; i < num_words; i++) {
//         *mem_ptr = 0;
//     }

//     //enable usb interrupt at processor
//     NVIC_ISER = (1 << 5); //USBCTRL_IRQ

//     //mux the controller to the onboard usb phy
//     USB_MUXING = (1 << 0) | (1 << 3); //TO_PHY | SOFTCON

//     //force VBUS detect so the device thinks its plugged into a host (it always should be)
//     USB_PWR = (1 << 4) | (1 << 3); //VBUS_DETECT | VBUS_DETECT_OVVERIDE_EN
    
//     //enable the USB controller in device mode
//     USB_MAIN_CTRL = (1 << 0); //CONTROLLER_EN enable controller
    
//     //enable an interrupt per EP0 transaction
//     USB_SIE_CTRL = (1 << 29);

//     // Enable interrupts for when a buffer is done, when the bus is reset,
//     // and when a setup packet is received
//     USB_INTE = (1 << 4) | //BUFF_STATUS
//                 (1 << 12) | //BUS_RESET
//                 (1 << 16); //SETUP_REQ

//     // Set up endpoints (endpoint control registers)
//     // described by device configuration
//     uint32_t dpram_offset = USB_EPX_DATA_OUT1 ^ USB_DPRAM;
//     uint32_t reg =  (1 << 31) | //EP_CTRL_ENABLE_BITS
//                     (1 << 29) | //EP_CTRL_INTERRUPT_PER_BUFFER
//                     (0x2 << 26) | //USB_TRANSFER_TYPE_BULK << EP_CTRL_BUFFER_TYPE_LSB
//                     dpram_offset;
//     USB_EP_CTRL_OUT1 = reg;

//     dpram_offset = USB_EPX_DATA_IN2 ^ USB_DPRAM;
//     reg =   (1 << 31) | //EP_CTRL_ENABLE_BITS
//             (1 << 29) | //EP_CTRL_INTERRUPT_PER_BUFFER
//             (0x2 << 26) | //USB_TRANSFER_TYPE_BULK << EP_CTRL_BUFFER_TYPE_LSB
//             dpram_offset;
//     USB_EP_CTRL_IN2 = reg;

//     //preset fuill speed device by enabling pull up on DP
//     USB_SIE_CTRL = 0x00010000;
// }

//unfinished
// void isr_usbctrl(void) {
//     uint32_t status = USB_INTS;
//     uint32_t handled = 0;

//     if (status & 0x00010000) { //USB_INTS_SETUP_REQ_BITS
//         handled |= 0x00010000; //USB_INTS_SETUP_REQ_BITS
//         USB_SIE_STATUS = 0x00020000; //USB_SIE_STATUS_SETUP_REC_BITS
//         //usb_handle_setup_packet
//         uint8_t req_direction = USB_SETUP_PACKET & 0xFF;
//         uint8_t req = USB_SETUP_PACKET & 0xFF00;



//     } 
// }

#define UART_BASE                                       0x40034000
#define UART_DR                                         *(volatile uint32_t *) (UART_BASE)
#define UART_FR                                         *(volatile uint32_t *) (UART_BASE + 0x18)
#define UART_IBRD                                       *(volatile uint32_t *) (UART_BASE + 0x24)
#define UART_FBRD                                       *(volatile uint32_t *) (UART_BASE + 0x28)
#define UART_LCR_H                                      *(volatile uint32_t *) (UART_BASE + 0x2c)
#define UART_LCR                                        *(volatile uint32_t *) (UART_BASE + 0x30)

void resetSubsys(void) {
    RESETS_RESET &= ~(1 << 5); // Bring IO_BANK0 out of reset state
    while (!(RESETS_RESET_DONE & (1 << 5))); // Wait for peripheral to respond

    RESETS_RESET &= ~(1 << 8); // pads bank
    while (!(RESETS_RESET_DONE & (1 << 8))); // Wait for peripheral to respond

    RESETS_RESET &= ~(1 << 22); //UART0
    while (!(RESETS_RESET_DONE & (1 << 22))); // Wait for peripheral to respond

    RESETS_RESET &= ~(1 << 16); //SPI0
    while (!(RESETS_RESET_DONE & (1 << 16))); // Wait for peripheral to respond
}

void uart_init(void) {
    UART_IBRD = 78;
    UART_FBRD = 8;
    UART_LCR_H = (0x3 << 5) | (1 << 4);
    UART_LCR = (1 << 9) | (1 << 8) | (1 << 0);    
    //set pins to function 2 (uart)
    IO_BANK0_GPIO00_CTRL = 2;
    IO_BANK0_GPIO01_CTRL = 2;
}

void uartTx( unsigned char x) {
    while ((UART_FR & (1 << 5)) != 0);
    UART_DR = x;
}

void uartTxStr(unsigned char *x) {
    while(*x != '\0') {
        uartTx(*x);
        x++;
    }
}

static char uartRx(void) {
    while ((UART_FR & (1 << 4)) != 0);
    return UART_DR;
}

static void *uartRxStr(char *str) { //this is bad
    unsigned int dat;
    int i = 0;
    while (dat != '\r') {
        dat = uartRx();
        str[i++] = dat;
    }
    str[--i] = '\0';
}

#define SPI0_BASE                               0x4003c000
#define SPI0_SSPCR0                             *(volatile uint32_t *) (SPI0_BASE)
#define SPI0_SSPCR1                             *(volatile uint32_t *) (SPI0_BASE + 0x004)
#define SPI0_SSPSR                              *(volatile uint32_t *) (SPI0_BASE + 0x00c)
#define SPI0_SSPDR                              *(volatile uint32_t *) (SPI0_BASE + 0x008)
#define SPI0_SSCPSR                             *(volatile uint32_t *) (SPI0_BASE + 0x010)

#define PADS_BANK0_BASE                         0x4001c000
#define PADS_BANK0_GPIO06                       *(volatile uint32_t *) (PADS_BANK0_BASE + 0x1c)
#define PADS_BANK0_GPIO07                       *(volatile uint32_t *) (PADS_BANK0_BASE + 0x20)
#define PADS_BANK0_GPIO09                       *(volatile uint32_t *) (PADS_BANK0_BASE + 0x28)

#define PADS_BANK0_GPIO18                       *(volatile uint32_t *) (PADS_BANK0_BASE + 0x4c)
#define PADS_BANK0_GPIO19                       *(volatile uint32_t *) (PADS_BANK0_BASE + 0x50)


static void hexToStr(char *str, int n) {
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

static void byteToStr(char *str, int n) {
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

void spi_send_byte(uint8_t data) {
    while (!(SPI0_SSPSR & (1 << 1))); //wait untill transmit fifo not full
    SPI0_SSPDR = data;
}

void spi_rw(char *data, unsigned int len) {
    unsigned int i;
    char str[3];
    for (i = 0; i < len; i++) {
        while (!(SPI0_SSPSR & (1 << 1))); //wait untill FIFO is not full
        SPI0_SSPDR = data[i];

        while (SPI0_SSPSR & (1 << 4)); //wait untill its not busy

        if (SPI0_SSPSR & (1 << 2)) { //if receive FIFO is not empty
            data[i] = SPI0_SSPDR;
        }
    }
}

void spi_rw_blocking(char *data, unsigned int len) {
    unsigned int i;
    for (i = 0; i < len; i++) {
        while (!(SPI0_SSPSR & (1 << 1))); //wait untill FIFO is not full
        //SPI0_SSPDR = data[i];
        SPI0_SSPDR = 0xFF;


        while (SPI0_SSPSR & (1 << 4)); //wait untill its not busy

        if (SPI0_SSPSR & (1 << 2)) { //if receive FIFO is not empty
        //while (!(SPI0_SSPSR & (1 << 2))) {} //if receive FIFO is not empty
            data[i] = SPI0_SSPDR;
        }
    }
}

static char buff[256]; //idk

uint8_t spi_read_byte(void) {
    //spi_send_byte(0xFF); //send dummy byte to clock in data
    while (!(SPI0_SSPSR & (1 << 2))); //wait untill receive fifo is not empty
    return SPI0_SSPDR;
}

uint8_t spi_rw_byte(uint8_t byte) {
    SPI0_SSPDR = byte;
    while (SPI0_SSPSR & (1 << 4));
    return (uint8_t) SPI0_SSPDR;
}

void spi_init(void) {
    //This may not be good, but it works
    SPI0_SSPCR0 = (0x7 << 0); //set data size to 8bit
    //frame format is motorola SPI by default
    //assume SPI mode 0
    //SPI0_SSPCR0 = ((1 << 6) | (1 << 7)); //SPO = 0, SPH = 0
    SPI0_SSCPSR = 32;
    //we will want to change this afterwards to 2 to get a 6mhz clock
    //device is master by default
    SPI0_SSPCR1 &= ~(1 << 2);
    
    //SOD is only relevant in slave mode
    
    //setup pins
    IO_BANK0_GPIO16_CTRL = 1; //function 1 SPI MISO
    //IO_BANK0_GPIO05_CTRL = 1; //CS, doing software CS
    IO_BANK0_GPIO18_CTRL = 1; //SCK
    IO_BANK0_GPIO19_CTRL = 1; //MOSI
    //set direction for pins 6 and 7
    //SIO_GPIO_OE_SET = (1 << 18);
    //SIO_GPIO_OE_SET = (1 << 19);
    IO_BANK0_GPIO09_CTRL = 5;
    SIO_GPIO_OE_SET = (1 << 9); //gp9 is gonna be cs

    PADS_BANK0_GPIO18 = (1 << 1) | (1 << 8); //disable input, output disable is 0 by default
    PADS_BANK0_GPIO19 = (1 << 1) | (1 << 8);

    SPI0_SSPCR1 |= (1 << 1); //enable SSP
}

bool sdInit(void) {
    usSleep(10000); //10ms, let sd card stabilize

    SIO_GPIO_OUT_SET |= 1 << 9;

    for(uint8_t i = 0; i < 10; i++) {
        spi_send_byte(0xFF);
    }

    //spi_rw_blocking(buff, 80);

    buff[0] = 0x40;
    buff[1] = 0x00;
    buff[2] = 0x00;
    buff[3] = 0x00;
    buff[4] = 0x00;
    buff[5] = 0x95;
    
    SIO_GPIO_OUT_CLR |= 1 << 9;

    for(uint8_t i = 0; i < 6; i++) {
        spi_send_byte(buff[i]);
    }

    uint16_t timeout = 0xFFF;
    uint8_t res;

    do {
        res = spi_read_byte();
        // byteToStr(str, res);
        // uartTxStr(str);
        spi_send_byte(0xFF);
        timeout--;
    } while((res != 0x1) && timeout > 0);

    if (timeout > 0) {
        uartTxStr("[OK]\r\n");
        return true;
    }

    SIO_GPIO_OUT_SET |= 1 << 9;
    spi_send_byte(0xFF);

    uartTxStr("[FAIL]\r\n");
    return true;
}

int strlen(char *str) {
    int i = 0;
    while(str[i] != '\0') {
        i++;
    }
    return i;
}

void kernel_main(void) {
    //usb_device_init();
    resetSubsys();
    uart_init();
    uartTxStr("\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n"); //clear screen
    uartTxStr("freeGPOS starting\r\n");
    uartTxStr("initializing SPI\r\n");
    spi_init();
    uartTxStr("initializing SD card ");
    sdInit();

    char str[100];
    char hex[3];

    while (true) {
        uartRxStr(str);
        //uartTxStr(str);
        uartTxStr("\r\n");
        int len = strlen(str);
        SIO_GPIO_OUT_CLR |= 1 << 9;
        spi_rw_blocking(str, len);
        for(int i = 0; i < len; i++) {
            byteToStr(hex, str[i]);
            uartTxStr(hex);
        }
        SIO_GPIO_OUT_SET |= 1 << 9;
        uartTxStr("\r\n");
    }

    //blink_forever();
}

// Main entry point
int main(void)
{
    kernel_main();
    
    return 0;
}