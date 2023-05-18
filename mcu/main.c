/*
 * main.c
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <stdio.h>
#include <pico/stdlib.h>
#include "rabbit_mcu.h"

int main(void)
{
    unsigned int usid = 0;

    ir_init();
    ultrasound_init();
    led_init();

    while (true) {
        ir_refresh();
        ultrasound_trigger(usid);
        usid++;
        usid %= ULTRASOUND_DEVICES;

        if (ir_triggered()) {
            led_set(true);
        } else {
            led_set(false);
        }

        sleep_ms(5);
    }

    return 0;
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
