/*
 * rs232.c
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <stdio.h>
#include <stdarg.h>
#include <pico/stdlib.h>
#include <hardware/uart.h>
#include "rabbit_mcu.h"

#define UART_TX_PIN          0
#define UART_RX_PIN          1
#define UART_ID          uart0
#define UART_BAUD_RATE  115200
#define UART_DATA_BITS       8
#define UART_STOP_BITS       1
#define UART_PARITY     UART_PARITY_NONE
#define UART_IRQ        UART0_IRQ

unsigned int rs232_rx_chars = 0;

unsigned int rs232_rx_lines = 0;

static void rs232_interrupt_handler(void)
{
    char c;

    while (uart_is_readable(UART_ID)) {
        c = uart_getc(UART_ID);
        rs232_rx_chars++;
        if (c == '\r') {
            rs232_rx_lines++;
        }
    }
}

void rs232_init(void)
{
    uart_init(UART_ID, UART_BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_hw_flow(UART_ID, false, false);
    uart_set_format(UART_ID, UART_DATA_BITS, UART_STOP_BITS, UART_PARITY);
    irq_set_exclusive_handler(UART_IRQ, rs232_interrupt_handler);
    irq_set_enabled(UART_IRQ, true);
    uart_set_irq_enables(UART_ID, true, false);
}

int rs232_printf(const char *fmt, ...)
{
    int ret;
    va_list ap;
    char buf[256];
    int i;

    va_start(ap, fmt);
    ret = vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
    va_end(ap);

    for (i = 0; i < ret; i++) {
        if (buf[i] == '\n' && i > 0 && buf[i] != '\r') {
            uart_putc_raw(uart0, '\r');
        }
        uart_putc_raw(uart0, buf[i]);
    }

    return ret;
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
