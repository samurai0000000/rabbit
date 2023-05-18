/*
 * ir.c
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <stdio.h>
#include <pico/stdlib.h>
#include "rabbit_mcu.h"

static const unsigned int IR_PINS[IR_DEVICES] = {
    22, 21, 20, 19,
};

bool ir_state[IR_DEVICES] __attribute__((section(".data.bss")));

void ir_init(void)
{
    unsigned int i;

    for (i = 0; i < IR_DEVICES; i++) {
        gpio_init(IR_PINS[i]);
        gpio_set_dir(IR_PINS[i], GPIO_IN);
        gpio_disable_pulls(IR_PINS[i]);
    }
}

void ir_refresh(void)
{
    unsigned int i;

    for (i = 0; i < IR_DEVICES; i++) {
        ir_state[i] = gpio_get(IR_PINS[i]);
    }
}

bool ir_triggered(void)
{
    unsigned int i;

    for (i = 0; i < IR_DEVICES; i++) {
        if (ir_state[i]) {
            return true;
        }
    }

    return false;
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
