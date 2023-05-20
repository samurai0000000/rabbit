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

#define UART_TX_PIN 0
#define UART_RX_PIN 1

void rs232_init(void)
{
    uart_init(uart0, 115200);
    gpio_set_function(0, GPIO_FUNC_UART);
    gpio_set_function(1, GPIO_FUNC_UART);
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
