/*
 * servos.cxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <math.h>
#include <pigpio.h>
#include "rabbit.hxx"
#include "pca9685_regs.h"

/*
 * PCA9685
 * https://www.mouser.com/datasheet/2/737/PCA9685-932827.pdf
 * https://cdn-shop.adafruit.com/datasheets/PCA9685.pdf
 */
#define PWM_I2C_BUS    1
#define PWM_I2C_ADDR   0x40

Servos::Servos()
    : _handle(-1),
      _freq(50)
{
    unsigned int i;
    uint8_t v;
    float prescale;

    _handle = i2cOpen(PWM_I2C_BUS, PWM_I2C_ADDR, 0x0);
    if (_handle < 0) {
        cerr << "Open PCA9685 failed" << endl;
        return;
    }

    for (i = 0; i < SERVO_CHANNELS; i++) {
        _lo[i] = 450;
        _hi[i] = 2500;
    }

    writeReg(MODE2_REG, MODE2_OUTDRV);
    writeReg(MODE1_REG, MODE1_SLEEP | MODE1_ALLCALL);

    prescale = 25000000;
    prescale /= 4096.0;
    prescale /= (float) _freq;
    prescale -= 1.0;
    v = floor(prescale + 0.5);
    writeReg(PRE_SCALE_REG, v);
    writeReg(MODE1_REG, MODE1_RESTART | MODE1_ALLCALL);
}

Servos::~Servos()
{
    if (_handle >= 0) {
        i2cWriteByteData(_handle, MODE1_REG, MODE1_SLEEP | MODE1_ALLCALL);
        i2cClose(_handle);
        _handle = -1;
    }
}

int Servos::readReg(uint8_t reg, uint8_t *val) const
{
    int ret = 0;

    if (_handle < 0) {
        return _handle;
    }

    ret = i2cReadByteData(_handle, reg);
    if (ret < 0) {
        fprintf(stderr, "%s: i2cReadByteData 0x%.2x failed!\n", __func__, reg);
    } else {
        *val = ret;
        ret = 0;
    }

    return ret;
}

int Servos::writeReg(uint8_t reg, uint8_t val) const
{
    int ret = 0;

    if (_handle < 0) {
        return _handle;
    }

    ret = i2cWriteByteData(_handle, reg, val);
    if (ret != 0) {
        fprintf(stderr, "%s: i2cWriteByteData 0x%.2x failed!\n", __func__, reg);
    }

    return ret;
}

void Servos::setPwm(unsigned int chan, unsigned int on, unsigned int off)
{
    if (_handle < 0) {
        return;
    }
    writeReg(LED_ON_L_REG(chan), on & 0xff);
    writeReg(LED_ON_H_REG(chan), on >> 8);
    writeReg(LED_OFF_L_REG(chan), off & 0xff);
    writeReg(LED_OFF_H_REG(chan), off >> 8);
}

void Servos::setRange(unsigned int chan, unsigned int lo, unsigned int hi)
{
    if (chan > SERVO_CHANNELS) {
        return;
    } else if (_lo >= _hi) {
        return;
    }

    _lo[chan] = lo;
    _hi[chan] = hi;
}

void Servos::setPulse(unsigned int chan, unsigned int pulse, bool ignoreRange)
{
    unsigned int on, off;

    if (chan > SERVO_CHANNELS) {
        return;
    }

    if (ignoreRange == false) {
        if (pulse < _lo[chan]) {
            pulse = _lo[chan];
        }

        if (pulse > _hi[chan]) {
            pulse = _hi[chan];
        }
    }

    on = 0;
    off = pulse * 4096 / (1000000 / _freq);
    if (off <= 4096) {
        setPwm(chan, on, off);
        _pulse[chan] = pulse;
    }
}

void Servos::setPct(unsigned int chan, unsigned int pct)
{
    unsigned int pulse;

    if (pct > 100) {
        pct = 100;
    }

    pulse = ((_hi[chan] - _lo[chan]) * pct / 100) + _lo[chan];
    setPulse(chan, pulse, false);
}

void Servos::center(unsigned int chan)
{
    unsigned int pulse;

    if (chan > SERVO_CHANNELS) {
        return;
    }

    pulse = (_lo[chan] + _hi[chan]) / 2;
    setPulse(chan, pulse);
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
