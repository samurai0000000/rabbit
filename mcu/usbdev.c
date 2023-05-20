/*
 * usbdev.c
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <string.h>
#include <pico/stdlib.h>
#include <hardware/regs/usb.h>
#include <hardware/structs/usb.h>
#include <hardware/irq.h>
#include <hardware/resets.h>
#include "rabbit_mcu.h"


static void usb_setup_endpoints(void)
{
    //const struct usb_endpoint_configuration *endpoints = dev_config.endpoints;

    //for (int i = 0; i < USB_NUM_ENDPOINTS; i++) {
    //    if (endpoints[i].descriptor && endpoints[i].handler) {
    //        usb_setup_endpoint(&endpoints[i]);
    //    }
    //}
}

void usbdev_init(void)
{
    /* Reset USB controller */
    reset_block(RESETS_RESET_USBCTRL_BITS);
    unreset_block_wait(RESETS_RESET_USBCTRL_BITS);

    /* Clear any previous state in dpram just in case */
    memset(usb_dpram, 0x0, sizeof(*usb_dpram));

    /* Enable USB interrupt at processor */
    irq_set_enabled(USBCTRL_IRQ, true);

    /* Mux the conrroller ot the onboard USB PHY */
    usb_hw->muxing = USB_USB_MUXING_TO_PHY_BITS | USB_USB_MUXING_SOFTCON_BITS;

    /* Enable the USB controller in device mode */
    usb_hw->main_ctrl = USB_MAIN_CTRL_CONTROLLER_EN_BITS;

    /* Enable an interrupt per EP0 transaction */
    usb_hw->sie_ctrl = USB_SIE_CTRL_EP0_INT_1BUF_BITS;

    /* Enable interrupts for when a buffer is done, when the bus is reset, */
    /* and when a setup packet is received */
    usb_hw->inte =
        USB_INTS_BUFF_STATUS_BITS |
        USB_INTS_BUS_RESET_BITS |
        USB_INTS_SETUP_REQ_BITS;

    /* Set up endpoints described by device configuration */
    usb_setup_endpoints();

    /* Present full speed device by enabling pull up on DP */
    hw_set_alias(usb_hw)->sie_ctrl = USB_SIE_CTRL_PULLUP_EN_BITS;
}

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
