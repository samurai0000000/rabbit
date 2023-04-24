/*
 * adc.cxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <sys/time.h>
#include <bsd/sys/time.h>
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
    : _handle(-1),
      _config(0),
      _running(false),
      _v()
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
        DR_128_SPS |
        COMP_MODE_TRAD |
        COMP_POL_LO |
        COMP_LAT_NO |
        COMP_QUE_DIS;
    ret = writeReg(CONFIG_REG, _config);
    if (ret != 0) {
        goto done;
    }

done:

    _running = true;
    pthread_mutex_init(&_mutex, NULL);
    pthread_cond_init(&_cond, NULL);
    pthread_create(&_thread, NULL, ADC::thread_func, this);
}

ADC::~ADC()
{
    _running = false;
    pthread_cond_broadcast(&_cond);
    pthread_join(_thread, NULL);
    pthread_mutex_destroy(&_mutex);
    pthread_cond_destroy(&_cond);

    if (_handle >= 0) {
        i2cClose(_handle);
        _handle = -1;
    }
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

void *ADC::thread_func(void *args)
{
    ADC *adc = (ADC *) args;

    adc->run();

    return NULL;
}

void ADC::run(void)
{
    struct timespec now, interval, next;

    interval.tv_sec = 0;
    interval.tv_nsec = 250000000;

    while (_running) {
        clock_gettime(CLOCK_REALTIME, &now);
        timespecadd(&now, &interval, &next);

        convert(0);
        convert(1);

        pthread_cond_timedwait(&_cond, &_mutex, &next);
        pthread_mutex_unlock(&_mutex);
    }
}

void ADC::convert(unsigned int chan)
{
    float v = 0.0;
    int ret;
    uint16_t config;
    int16_t conv;

    if (_handle < 0) {
        return;
    }

    config = _config & ~0x7000;
    config |= OS;
    switch (chan) {
    case 0: config |= MUX_AIN0_GND; break;
    case 1: config |= MUX_AIN1_GND; break;
    case 2: config |= MUX_AIN2_GND; break;
    case 3: config |= MUX_AIN3_GND; break;
    default:
        return;
    }

    /* Setup CONFIG register to perform a single shot sampling */
    ret = writeReg(CONFIG_REG, config);
    if (ret != 0) {
        return;
    }

    /* Conversion delay */
    usleep(80000);

    /* Read result */
    ret = readReg(CONVERSION_REG, (uint16_t *) &conv);
    if (ret != 0) {
        return;
    }


    v = (float) conv;
    v = v * 6.144 / 32768.0;
    _v[chan] = v;
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
