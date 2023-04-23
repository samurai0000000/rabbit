/*
 * adc.cxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <pigpio.h>
#include "rabbit.hxx"
#include "ads1115_defs.h"

/*
 * ADS1115
 * https://www.ti.com/lit/ds/symlink/ads1115.pdf
 */
#define ADC_I2C_BUS    1
#define ADC_I2C_ADDR   0x48

ADC::ADC()
    : _handle(-1)
{
    int ret;

    _handle = i2cOpen(ADC_I2C_BUS, ADC_I2C_ADDR, 0x0);
    if (_handle < 0) {
        fprintf(stderr, "Open ADS1115 failed!\n");
        goto done;
    }

    _config =
        MUX_AIN0_GND |
        PGA_FSR_6_144V |
        MODE_SING |
        DR_8_SPS |
        COMP_MODE_TRAD |
        COMP_POL_LO |
        COMP_LAT_NO |
        COMP_QUE_DIS;
    ret = writeReg(CONFIG_REG, _config);
    if (ret != 0) {
        goto done;
    }

done:

    pthread_mutex_init(&_mutex, NULL);
}

ADC::~ADC()
{
    pthread_mutex_lock(&_mutex);
    if (_handle >= 0) {
        i2cClose(_handle);
    }
    pthread_mutex_unlock(&_mutex);
    pthread_mutex_destroy(&_mutex);
}

int ADC::readReg(uint8_t reg, uint16_t *val) const
{
    int ret = 0;

    if (_handle < 0) {
        return _handle;
    }

    ret = i2cReadWordData(_handle, reg);
    if (ret < 0) {
        fprintf(stderr, "%s: i2cReadWordData 0x%.2x failed!\n", __func__, reg);
    } else {
        uint16_t v = ret;
        v = (v >> 8) | (v << 8);
        *val = v;
        ret = 0;
    }

    return ret;
}

int ADC::writeReg(uint8_t reg, uint16_t val) const
{
    int ret = 0;

    if (_handle < 0) {
        return _handle;
    }

    val = (val >> 8) | (val << 8);
    ret = i2cWriteWordData(_handle, reg, val);
    if (ret != 0) {
        fprintf(stderr, "%s: i2cWriteWordData failed!\n", __func__);
    }

    return ret;
}

float ADC::v(unsigned int chan)
{
    float v = 0.0;
    unsigned int retries;
    static const unsigned int retry_limit = 10;
    int ret;
    uint16_t config;
    uint16_t conv;

    if (_handle < 0) {
        return 0.0;
    }

    pthread_mutex_lock(&_mutex);

start:

    config = _config & ~0x7000;  /* Clear MUX */
    config |= OS;                /* Start a single conversion */
    switch (chan) {
    case 0: config |= MUX_AIN0_GND; break;
    case 1: config |= MUX_AIN1_GND; break;
    case 2: config |= MUX_AIN2_GND; break;
    case 3: config |= MUX_AIN3_GND; break;
    default:
        goto done;
    }

    ret = writeReg(CONFIG_REG, config);
    if (ret != 0) {
        goto done;
    }

    for (retries = 0; retries < retry_limit; retries++) {
        ret = readReg(CONFIG_REG, &config);
        if (ret != 0) {
            goto done;
        }

        if ((config & OS) == 0x0) {
            break;
        }

        usleep(1000);
    }

    if (retries >= retry_limit) {
        /* Reset ADS1115 if retried too many times */
        i2cClose(_handle);
        _handle = i2cOpen(ADC_I2C_BUS, ADC_I2C_ADDR, 0x0);
        if (_handle < 0) {
            fprintf(stderr, "Reopen ADS1115 failed!\n");
            goto done;
        } else {
            goto start;
        }
    }

    ret = readReg(CONVERSION_REG, &conv);
    if (ret != 0) {
        goto done;
    }

    v = (float) conv;
    v = v * 6.144 / 32768.0;

done:

    pthread_mutex_unlock(&_mutex);

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
