/*
 * main.c
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <stdio.h>
#include <pico/stdlib.h>

#define LED_PIN 25

int main(int argc, char **argv)
{
    setup_default_uart();
    printf("Hello, world!\n");

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    while (true) {
        gpio_put(LED_PIN, 1);
        sleep_ms(250);
        gpio_put(LED_PIN, 0);
        sleep_ms(500);
    }

    return 0;
}

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
