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
      _running(false)
{
    int ret;
    unsigned int i;

    _handle = i2cOpen(ADC_I2C_BUS, ADC_I2C_ADDR, 0x0);
    if (_handle < 0) {
        fprintf(stderr, "Open ADS1115 failed!\n");
        goto done;
    }

    _config =
        MUX_AIN0_GND |
        PGA_FSR_6_144V |
        MODE_SING |
        DR_860_SPS |
        COMP_MODE_TRAD |
        COMP_POL_LO |
        COMP_LAT_NO |
        COMP_QUE_DIS;
    ret = writeReg(CONFIG_REG, _config);
    if (ret != 0) {
        goto done;
    }

    writeReg(LO_THRESH_REG, 0x8000);
    writeReg(HI_THRESH_REG, 0x7fff);

done:

    for (i = 0; i < ADC_CHANNELS; i++) {
        _hist[i] = new MedianFilter<float>(50);
    }

    _running = true;
    pthread_mutex_init(&_mutex, NULL);
    pthread_cond_init(&_cond, NULL);
    pthread_create(&_thread, NULL, ADC::thread_func, this);
}

ADC::~ADC()
{
    unsigned int i;

    _running = false;
    pthread_cond_broadcast(&_cond);
    pthread_join(_thread, NULL);
    pthread_mutex_destroy(&_mutex);
    pthread_cond_destroy(&_cond);

    if (_handle >= 0) {
        i2cClose(_handle);
        _handle = -1;
    }

    for (i = 0; i < ADC_CHANNELS; i++) {
        delete _hist[i];
        _hist[i] = NULL;
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
        fprintf(stderr, "ADC::readReg: i2cReadWordData 0x%.2x failed!\n", reg);
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
        fprintf(stderr, "ADC::writeReg i2cWriteWordData 0x%.2xfailed!\n", reg);
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
    struct timespec ts, tloop;

    clock_gettime(CLOCK_REALTIME, &ts);
    tloop.tv_sec = 0;
    tloop.tv_nsec = 100000000;

    while (_running) {
        timespecadd(&ts, &tloop, &ts);

        convert(0);
        convert(1);
        convert(2);
        convert(3);

        pthread_mutex_lock(&_mutex);
        pthread_cond_timedwait(&_cond, &_mutex, &ts);
        pthread_mutex_unlock(&_mutex);
    }
}

void ADC::convert(unsigned int chan)
{
    float v = 0.0;
    int ret;
    uint16_t config;
    int16_t conv;
    struct timespec now, interval, next;

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
    interval.tv_sec = 0;
    switch (config & DR_860_SPS) {
    case DR_8_SPS:   interval.tv_nsec = 125000000; break;
    case DR_16_SPS:  interval.tv_nsec =  62500000; break;
    case DR_32_SPS:  interval.tv_nsec =  31250000; break;
    case DR_64_SPS:  interval.tv_nsec =  15625000; break;
    case DR_128_SPS: interval.tv_nsec =   7812500; break;
    case DR_250_SPS: interval.tv_nsec =   4000000; break;
    case DR_475_SPS: interval.tv_nsec =   2105263; break;
    case DR_860_SPS: interval.tv_nsec =   1162791; break;
    default:         interval.tv_nsec = 125000000; break;
    }
    pthread_mutex_lock(&_mutex);
    clock_gettime(CLOCK_REALTIME, &now);
    timespecadd(&now, &interval, &next);
    pthread_cond_timedwait(&_cond, &_mutex, &next);
    pthread_mutex_unlock(&_mutex);

    /* Read result */
    ret = readReg(CONVERSION_REG, (uint16_t *) &conv);
    if (ret != 0) {
        return;
    }

    /* Apply Multiplier */
    v = (float) ((int16_t) conv);
    switch (config & (7 << 9)) {
    case PGA_FSR_6_144V:    v = v * 6.144 / 32768.0; break;
    case PGA_FSR_4_096V:    v = v * 4.096 / 32768.0; break;
    case PGA_FSR_2_048V:    v = v * 2.048 / 32768.0; break;
    case PGA_FSR_1_024V:    v = v * 1.024 / 32768.0; break;
    case PGA_FSR_0_512V:    v = v * 0.512 / 32768.0; break;
    case PGA_FSR_0_256V:    v = v * 0.256 / 32768.0; break;
    default:
        return;
    }

    //printf("chan%u: %.3f\n", chan, v);
    v = v * 2.20;
    _hist[chan]->addSample(v);
}

float ADC::v(unsigned int chan) const
{
    if (chan >= ADC_CHANNELS) {
        return 0.0;
    }

    return _hist[chan]->average();
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
