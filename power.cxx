/*
 * power.cxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include "rabbit.hxx"

#define VOLTAGE_ADC_CHAN 0
#define CURRENT_ADC_CHAN 1

Power::Power()
{

}

Power::~Power()
{

}

unsigned int Power::voltage(void) const
{
    return adc->val(VOLTAGE_ADC_CHAN);
}

unsigned int Power::current(void) const
{
    return adc->val(CURRENT_ADC_CHAN);
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
