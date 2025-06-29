#include <stdint.h>
#include <stdbool.h>

typedef volatile uint32_t io_rw_32;

// Define necessary register addresses
#define RESETS_RESET                                    *(volatile uint32_t *) (0x4000c000)
#define RESETS_RESET_DONE                               *(volatile uint32_t *) (0x4000c008)
#define IO_BANK0_GPIO25_CTRL                            *(volatile uint32_t *) (0x400140cc)
#define SIO_GPIO_OE_SET                                 *(volatile uint32_t *) (0xd0000024)
#define SIO_GPIO_OUT_XOR                                *(volatile uint32_t *) (0xd000001c)

#define USB_BASE                                        0x50110000
#define USB_MAIN_CTRL                                   *(volatile uint32_t *) (USB_BASE + 0x40)
#define USB_MUXING                                      *(volatile uint32_t *) (USB_BASE + 0x74)
#define USB_PWR                                         *(volatile uint32_t *) (USB_BASE + 0x78)
#define USB_SIE_CTRL                                    *(volatile uint32_t *) (USB_BASE + 0x4c)
#define USB_INTE                                        *(volatile uint32_t *) (USB_BASE + 0x90)

#define USB_NUM_ENDPOINTS 16

#define USB_DPRAM                                       0x50100000
#define USB_DPRAM_SIZE                                  4096 //bytes
#define USB_SETUP_PACKET                                *(volatile uint32_t *) (USB_BASE) //8 bytes

#define USB_EP_CTRL_OUT1                                *(volatile uint32_t *) (USB_BASE + 0x000c)
#define USB_EP_CTRL_IN2                                 *(volatile uint32_t *) (USB_BASE + 0x0010)

#define USB_EP_BUF_CTRL_OUT1                            *(volatile uint32_t *) (USB_BASE + 0x000c + ((USB_NUM_ENDPOINTS-1) * 8) + 0x0008)
#define USB_EP_BUF_CTRL_IN2                             *(volatile uint32_t *) (USB_BASE + 0x0008 + ((USB_NUM_ENDPOINTS-1) * 8) + 0x0010)

#define USB_EPX_DATA                                    *(volatile uint32_t *) (USB_BASE + 0x0140 + (USB_NUM_ENDPOINTS * 8))
#define USB_EPX_DATA_OUT1                               *(volatile uint32_t *) (USB_EPX_DATA)
#define USB_EPX_DATA_IN2                                *(volatile uint32_t *) (USB_EPX_DATA + 0x40)

#define NVIC_BASE                                       0xe0000000
#define NVIC_ISER                                       *(volatile uint32_t *) (NVIC_BASE + 0xe100)

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

void usb_device_init(void) {
    //bring the USB peripheral out of reset
    RESETS_RESET &= ~(1 << 24);

    //clear dpram
    volatile uint32_t *mem_ptr = (volatile uint32_t *) USB_DPRAM;
    uint32_t num_words = USB_DPRAM_SIZE / sizeof(uint32_t);
    for(uint32_t i = 0; i < num_words; i++) {
        *mem_ptr = 0;
    }

    //enable usb interrupt at processor
    NVIC_ISER |= (1 << 5); //USBCTRL_IRQ

    //mux the controller to the onboard usb phy
    USB_MUXING |= (1 << 0) | (1 << 3); //TO_PHY | SOFTCON

    //force VBUS detect so the device thinks its plugged into a host (it always should be)
    USB_PWR |= (1 << 4) | (1 << 3); //VBUS_DETECT | VBUS_DETECT_OVVERIDE_EN
    
    //enable the USB controller in device mode
    USB_MAIN_CTRL |= (1 << 0); //CONTROLLER_EN enable controller
    
    //enable an interrupt per EP0 transaction
    USB_SIE_CTRL |= (1 << 29);

    // Enable interrupts for when a buffer is done, when the bus is reset,
    // and when a setup packet is received
    USB_INTE |= (1 << 4) | //BUFF_STATUS
                (1 << 12) | //BUS_RESET
                (1 << 16); //SETUP_REQ

    // Set up endpoints (endpoint control registers)
    // described by device configuration

    //EP0 does not have endpoint control, so they are skipped
    //need to set up ep1_out and ep2_in

                //     {
                //         .descriptor = &ep1_out,
                //         .handler = &ep1_out_handler,
                //         // EP1 starts at offset 0 for endpoint control
                //         .endpoint_control = &usb_dpram->ep_ctrl[0].out,
                //         .buffer_control = &usb_dpram->ep_buf_ctrl[1].out,
                //         // First free EPX buffer
                //         .data_buffer = &usb_dpram->epx_data[0 * 64],
                // },
                // {
                //         .descriptor = &ep2_in,
                //         .handler = &ep2_in_handler,
                //         .endpoint_control = &usb_dpram->ep_ctrl[1].in,
                //         .buffer_control = &usb_dpram->ep_buf_ctrl[2].in,
                //         // Second free EPX buffer
                //         .data_buffer = &usb_dpram->epx_data[1 * 64],
                // }

    
    // Get the data buffer as an offset of the USB controller's DPRAM
    // uint32_t dpram_offset = usb_buffer_offset(ep->data_buffer); (return (uint32_t) buf ^ (uint32_t) usb_dpram;)
    // uint32_t reg = EP_CTRL_ENABLE_BITS
    //                | EP_CTRL_INTERRUPT_PER_BUFFER
    //                | (ep->descriptor->bmAttributes << EP_CTRL_BUFFER_TYPE_LSB)
    //                | dpram_offset;
    // *ep->endpoint_control = reg;
    //uint8_t epx_data[USB_DPRAM_MAX - 0x180]; 4096 - 384 = 0xE80
    //exp_data should be at offset 384 from the start of dpram

    uint32_t dpram_offset = USB_EPX_DATA_OUT1 ^ USB_DPRAM;
    uint32_t reg =  (1 << 31) | //EP_CTRL_ENABLE_BITS
                    (1 << 29) | //EP_CTRL_INTERRUPT_PER_BUFFER
                    (0x2 << 26) | //USB_TRANSFER_TYPE_BULK << EP_CTRL_BUFFER_TYPE_LSB
                    dpram_offset;
    USB_EP_CTRL_OUT1 = reg;

    dpram_offset = USB_EPX_DATA_IN2 ^ USB_DPRAM;
    reg =   (1 << 31) | //EP_CTRL_ENABLE_BITS
            (1 << 29) | //EP_CTRL_INTERRUPT_PER_BUFFER
            (0x2 << 26) | //USB_TRANSFER_TYPE_BULK << EP_CTRL_BUFFER_TYPE_LSB
            dpram_offset;
    USB_EP_CTRL_IN2 = reg;

    //preset fuill speed device by enabling pull up on DP
    USB_SIE_CTRL |= 0x00010000;
}

void kernel_main(void) {
    usb_device_init();

    blink_forever();
}

// Main entry point
int main(void)
{
    kernel_main();
    
    return 0;
}