/*
 * onboard_temp.c
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <pico/stdlib.h>
#include <hardware/adc.h>
#include "rabbit_mcu.h"

/* 12-bit conversion, assume max value == ADC_VREF == 3.3 V */
static const float conversionFactor = 3.3f / (1 << 12);

float temperature_c = 0.0;

void onboard_temp_init(void)
{
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4);
}

float onboard_temp_refresh(void)
{
    float adc = (float) adc_read() * conversionFactor;

    temperature_c = 27.0f - (adc - 0.706f) / 0.001721f;

    return temperature_c;
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
