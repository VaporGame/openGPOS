#include <stdint.h>
#include <stdbool.h>

// Define necessary register addresses
#define RESETS_RESET                                    *(volatile uint32_t *) (0x4000c000)
#define RESETS_RESET_DONE                               *(volatile uint32_t *) (0x4000c008)
#define IO_BANK0_GPIO25_CTRL                            *(volatile uint32_t *) (0x400140cc)
#define SIO_GPIO_OE_SET                                 *(volatile uint32_t *) (0xd0000024)
#define SIO_GPIO_OUT_XOR                                *(volatile uint32_t *) (0xd000001c)

// Declare usSleep function
extern void usSleep(uint64_t us);

// Global variable counting how many times LED switched state
uint8_t blinkCnt;

void blink_forever(void) {
    RESETS_RESET &= ~(1 << 5); // Bring IO_BANK0 out of reset state
    while (!(RESETS_RESET_DONE & (1 << 5))); // Wait for peripheral to respond
    IO_BANK0_GPIO25_CTRL = 5; // Set GPIO 25 function to SIO
    SIO_GPIO_OE_SET |= 1 << 25; // Set output enable for GPIO 25 in SIO

    while (++blinkCnt < 21)
    //while (1)
    {
        usSleep(500000); // Wait for 0.5sec
        SIO_GPIO_OUT_XOR |= 1 << 25;  // Flip output for GPIO 25
    }
}

#define USBCTRL_BASE        0x50110000
#define USBCTRL_DPRAM_BASE  0x50100000

// USB Controller registers
#define USB_MAIN_CTRL       *(volatile uint32_t *)(USBCTRL_BASE + 0x40)
#define USB_SIE_CTRL        *(volatile uint32_t *)(USBCTRL_BASE + 0x4C)
#define USB_SIE_STATUS      *(volatile uint32_t *)(USBCTRL_BASE + 0x50)
#define USB_BUFF_STATUS     *(volatile uint32_t *)(USBCTRL_BASE + 0x58)
#define USB_ADDR_ENDP       *(volatile uint32_t *)(USBCTRL_BASE + 0x00)
#define USB_INTE            *(volatile uint32_t *)(USBCTRL_BASE + 0x90)
#define USB_INTS            *(volatile uint32_t *)(USBCTRL_BASE + 0x98)
#define USB_INTR            *(volatile uint32_t *)(USBCTRL_BASE + 0x8c)
#define USB_INTR_ENDP_BITS  0x1fffe

// DPRAM structures
typedef struct {
    volatile uint32_t addr;
    volatile uint32_t len;
    volatile uint32_t status;
    volatile uint32_t reserved;
} buffer_descriptor_t;

typedef struct {
    buffer_descriptor_t ep_out[16];
    buffer_descriptor_t ep_in[16];
    uint8_t setup_packet[8];
} __attribute__((packed)) usb_dpram_t;

volatile usb_dpram_t *dpram = (volatile usb_dpram_t *)USBCTRL_DPRAM_BASE;

// Descriptors (as byte arrays from the USB spec)
uint8_t device_descriptor[] = {
    18, // bLength
    0x01, // bDescriptorType (DEVICE)
    0x00, 0x02, // bcdUSB (2.00)
    0x02, // bDeviceClass (CDC)
    0x02, // bDeviceSubClass (ACM)
    0x00, // bDeviceProtocol
    0x40, // bMaxPacketSize0 (64 bytes)
    0x8A, 0x2E, // idVendor (set to a pico id)
    0x09, 0x00, // idProduct (set to a pico id)
    0x00, 0x01, // bcdDevice
    0x00, // iManufacturer
    0x00, // iProduct
    0x00, // iSerialNumber
    0x01, // bNumConfigurations
};

uint8_t configuration_descriptor[] = {
    // Configuration Descriptor
    8, // bLenght
    0x02, // bDescriptorType (CONFIGURATION)
    67, // wTotalLength
    0x01, // bNumInterfaces
    0x01, // bConfigurationValue
    0x00, // iConfiguration
    0x80, // bmAttributes (D7 must be 1)
    0xFA, // bMaxPower (500mA)
    // Interface Association Descriptor (IAD)
    0, // bLenght
    0x04, // bDescriptorType (INTERFACE)
    0x00, // bInterfaceNumber
    0x00, // bAlternateSetting
    0x03, // bNumEndpoints
};

void kernel_main(void) {
    blink_forever();
}

// Main entry point
int main(void)
{
    kernel_main();
    
    return 0;
}