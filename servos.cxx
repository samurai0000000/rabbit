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

#define READ_I2C(r, v)                                              \
    (v) = i2cReadByteData(_handle, (r));                            \
    if ((v) < 0) {                                                  \
        fprintf(stderr, "PCA9685 read reg 0x%.2x failed!\n", (r));  \
        goto done;                                                  \
    }

#define WRITE_I2C(r, v)                                             \
    ret = i2cWriteByteData(_handle, (r), (uint8_t) (v));            \
    if (ret != 0) {                                                 \
        fprintf(stderr, "PCA9685 write reg 0x%.2x failed!\n", (r)); \
        goto done;                                                  \
    }

Servos::Servos()
    : _handle(-1),
      _freq(50)
{
    int ret = 0;
    unsigned int i;
    uint8_t v;
    float prescale;

    _handle = i2cOpen(PWM_I2C_BUS, PWM_I2C_ADDR, 0x0);
    if (_handle < 0) {
        goto done;
    }

    for (i = 0; i < SERVO_CHANNELS; i++) {
        _lo[i] = 0;
        _hi[i] = 4800;
    }

    WRITE_I2C(ALL_LED_ON_L_REG, 0x0);
    WRITE_I2C(ALL_LED_ON_H_REG, 0x0);
    WRITE_I2C(ALL_LED_OFF_L_REG, 0x0);
    WRITE_I2C(ALL_LED_OFF_H_REG, 0x0);
    WRITE_I2C(MODE2_REG, MODE2_OUTDRV);
    WRITE_I2C(MODE1_REG, MODE1_SLEEP | MODE1_ALLCALL);

    prescale = 25000000;
    prescale /= 4096.0;
    prescale /= (float) _freq;
    prescale -= 1.0;
    v = floor(prescale + 0.5);
    WRITE_I2C(PRE_SCALE_REG, v);
    WRITE_I2C(MODE1_REG, MODE1_RESTART | MODE1_ALLCALL);

done:

    if (ret != 0) {
        cerr << "Open PCA9685 failed" << endl;
        if (_handle != -1) {
            i2cClose(_handle);
            _handle = -1;
        }
    }

    return;
}

Servos::~Servos()
{
    if (_handle >= 0) {
        i2cWriteByteData(_handle, MODE1_REG, MODE1_SLEEP | MODE1_ALLCALL);
        i2cClose(_handle);
    }
}

void Servos::setPwm(unsigned int chan, unsigned int on, unsigned int off)
{
    int ret;

    WRITE_I2C(LED_ON_L_REG(chan), on & 0xff);
    WRITE_I2C(LED_ON_H_REG(chan), on >> 8);
    WRITE_I2C(LED_OFF_L_REG(chan), off & 0xff);
    WRITE_I2C(LED_OFF_H_REG(chan), off >> 8);

done:

    return;
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

    setPwm(chan, 0, pulse * 4096 / (1000000 / _freq));
    _pulse[chan] = pulse;
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
