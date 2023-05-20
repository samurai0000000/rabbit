/*
 * main.c
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <pico/stdlib.h>
#include <pico/util/queue.h>
#include "rabbit_mcu.h"

#define IR_SAMPLING_INTERVAL_US    500
#define US_SAMPLING_INTERVAL_US   5000

void rabbit_panic(const char *msg)
{
    printf("panic: %s\n", msg);
    printf("halt!\n");
    for (;;);
}

static bool sample_ir(repeating_timer_t *timer)
{
    (void)(timer);

    ir_refresh();
    if (ir_triggered()) {
        led_set(true);
    } else {
        led_set(false);
    }

    return true;
}

static bool sample_us(repeating_timer_t *timer)
{
    static unsigned int usid = 0;

    (void)(timer);

    ultrasound_trigger(usid);
    usid++;
    usid %= ULTRASOUND_DEVICES;

    onboard_temp_refresh();

    return true;
}

int main(void)
{
    repeating_timer_t timer1;
    repeating_timer_t timer2;
    unsigned int i;

    rs232_init();
    printf("Rabbit MCU\n");

    ir_init();
    ultrasound_init();
    led_init();
    onboard_temp_init();

    if (add_repeating_timer_us(-IR_SAMPLING_INTERVAL_US,
                               sample_ir, NULL, &timer1) ==
        false) {
        rabbit_panic("add_repeating_timer for infrared");
    }

    if (add_repeating_timer_us(-US_SAMPLING_INTERVAL_US,
                               sample_us, NULL, &timer2) ==
        false) {
        rabbit_panic("add_repeating_timer for ultrasound");
    }

    while (true) {
        for (i = 0; i < ULTRASOUND_DEVICES; i++) {
            if (ultrasound_d_mm[i] != 0) {
                printf("US%u: %u mm\n", i, ultrasound_d_mm[i]);
            }
        }

        sleep_ms(500);
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
