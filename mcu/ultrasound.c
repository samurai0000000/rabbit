/*
 * ultrasound.c
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <pico/stdlib.h>
#include "rabbit_mcu.h"

struct ultrasound_pin {
    unsigned int trigger;
    unsigned int echo;
};

struct ultrasound_state {
    bool active;
    uint64_t t_trigger;
    uint64_t t_rise;
    uint64_t t_fall;
};

struct ultrasound_pin ULTRASOUND_PINS[ULTRASOUND_DEVICES] = {
    {  2,  3, },
    {  4,  5, },
    {  6,  7, },
    {  8,  9, },
    { 10, 11, },
    { 12, 13, },
};

struct ultrasound_state ultrasound_state[ULTRASOUND_DEVICES]
__attribute__((section(".data.bss")));

unsigned int ultrasound_d_mm[ULTRASOUND_DEVICES]
__attribute__((section(".data.bss")));

static void ultrasound_gpio_interrupt(uint gpio, uint32_t mask)
{
    unsigned int i;
    unsigned int tdiff;

    for (i = 0; i < ULTRASOUND_DEVICES; i++) {
        if (gpio != ULTRASOUND_PINS[i].echo) {
            continue;
        }

        if (ultrasound_state[i].active == false) {
            continue;
        }

        if (mask & GPIO_IRQ_EDGE_RISE) {
            ultrasound_state[i].t_rise = time_us_64();
        } else if (mask & GPIO_IRQ_EDGE_FALL) {
            ultrasound_state[i].t_fall = time_us_64();
            if ((ultrasound_state[i].t_fall > ultrasound_state[i].t_rise) &&
                (ultrasound_state[i].t_rise != 0)) {
                tdiff =
                    ultrasound_state[i].t_fall -
                    ultrasound_state[i].t_rise;
                ultrasound_d_mm[i] = (tdiff * 331 / 1000) / 2;
            }
            ultrasound_state[i].active = false;
        }
    }
}

void ultrasound_init(void)
{
    unsigned int i;

    for (i = 0; i < ULTRASOUND_DEVICES; i++) {
        gpio_init(ULTRASOUND_PINS[i].trigger);
        gpio_set_dir(ULTRASOUND_PINS[i].trigger, GPIO_OUT);
        gpio_put(ULTRASOUND_PINS[i].trigger, false);
        gpio_init(ULTRASOUND_PINS[i].echo);
        gpio_set_dir(ULTRASOUND_PINS[i].echo, GPIO_IN);
        gpio_disable_pulls(ULTRASOUND_PINS[i].echo);
        gpio_set_irq_enabled_with_callback(
            ULTRASOUND_PINS[i].echo,
            GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
            true,
            ultrasound_gpio_interrupt);
    }
}

void ultrasound_trigger(unsigned int id)
{
    if (id >= ULTRASOUND_DEVICES) {
        return;
    }

    gpio_put(ULTRASOUND_PINS[id].trigger, true);
    ultrasound_state[id].active = 1;
    ultrasound_state[id].t_trigger = time_us_64();
    ultrasound_state[id].t_rise = 0;
    ultrasound_state[id].t_fall = 0;
    sleep_us(5);
    gpio_put(ULTRASOUND_PINS[id].trigger, false);
}

void ultrasound_clear(unsigned int id)
{
    if (id >= ULTRASOUND_DEVICES) {
        return;
    }

    ultrasound_state[id].active = 1;
    ultrasound_state[id].t_trigger = time_us_64();
    ultrasound_state[id].t_rise = 0;
    ultrasound_state[id].t_fall = 0;
    ultrasound_d_mm[id] = 0;
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
