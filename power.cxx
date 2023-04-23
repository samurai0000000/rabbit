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

Power::Power()
{

}

Power::~Power()
{

}

float Power::voltage(void) const
{
    float v;

    v = adc->v(VOLTAGE_ADC_CHAN);
    v = v * 5.0;  /* Multiply by 5 to get the voltage */

    return v;
}

float Power::current(void) const
{
    float v;

    v = adc->v(CURRENT_ADC_CHAN);
    v = v * 5.0;  /* Multiply by 5 to get the amperage */

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
