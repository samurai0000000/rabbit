/*
 * adc.cxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <pigpio.h>
#include "rabbit.hxx"

/*
 * ADS1115
 * https://www.ti.com/lit/ds/symlink/ads1115.pdf
 */
#define ADC0_I2C_BUS    1
#define ADC0_I2C_ADDR   0x49
#define ADC1_I2C_BUS    1
#define ADC1_I2C_ADDR   0x4a

#define NUM_ADC_DEVICES  2
#define NUM_ADC_CHANNELS 4
#define MIN_ADC_VALUE    0
#define MAX_ADC_VALUE    65536

struct adc_devinfo {
    unsigned int bus;
    unsigned int addr;
};

static const struct adc_devinfo adc_devinfo[NUM_ADC_DEVICES] = {
    { ADC0_I2C_BUS, ADC0_I2C_ADDR, },
    { ADC1_I2C_BUS, ADC1_I2C_ADDR, },
};

ADC::ADC()
    : _devs(NULL)
{
    unsigned int i;

    _devs = new int[NUM_ADC_DEVICES];
    for (i = 0; i < NUM_ADC_DEVICES; i++) {
        _devs[i] = i2cOpen(adc_devinfo[i].bus, adc_devinfo[i].addr, 0);
        if (_devs[i] < 0) {
            cerr << "Open ADS1115 failed" << endl;
        }
    }
}

ADC::~ADC()
{
    if (_devs) {
        delete [] _devs;
        _devs = NULL;
    }
}

unsigned int ADC::channels(void) const
{
    return NUM_ADC_CHANNELS * NUM_ADC_DEVICES;
}

unsigned int ADC::min(void) const
{
    return MIN_ADC_VALUE;
}

unsigned int ADC::max(void) const
{
    return MAX_ADC_VALUE;
}

unsigned int ADC::val(unsigned int chan) const
{
    int dev_id;
    int dev = -1;

    dev_id = chan / NUM_ADC_CHANNELS;
    if (dev_id < NUM_ADC_DEVICES) {
        dev = _devs[dev_id];
    }

    if (dev == -1) {
        return 0;
    }

    // TODO

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
