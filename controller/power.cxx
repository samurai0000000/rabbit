/*
 * power.cxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include "rabbit.hxx"

/*
 * Voltage divider (by 5)
 */
#define VOLTAGE_ADC_CHAN 0

/*
 * ACS712
 * https://www.sparkfun.com/datasheets/BreakoutBoards/0712.pdf
 */
#define CURRENT_ADC_CHAN 1

static unsigned int instance = 0;

Power::Power()
{
    if (instance != 0) {
        fprintf(stderr, "Power can be instantiated only once!\n");
        exit(EXIT_FAILURE);
    } else {
        instance++;
    }

    printf("Power is online\n");
}

Power::~Power()
{
    instance--;
    printf("Power is offline\n");
}

float Power::voltage(void) const
{
    float v;

    v = adc->v(VOLTAGE_ADC_CHAN);
    v = v * 5.0;

    return v;
}

float Power::current(void) const
{
    float v;

    v = adc->v(CURRENT_ADC_CHAN);
    v = (v - 2.5) * 5.0;

    return v;
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
