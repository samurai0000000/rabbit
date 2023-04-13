/*
 * servos.cxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <pigpio.h>
#include "rabbit.hxx"

/*
 * PCA9685
 * https://www.mouser.com/datasheet/2/737/PCA9685-932827.pdf
 * https://cdn-shop.adafruit.com/datasheets/PCA9685.pdf
 */
#define PWM_I2C_BUS    1
#define PWM_I2C_ADDR   0x40
#define PWM_CHANNELS   16
#define PWM_RESOLUTION 4096

Servos::Servos()
    : _pos(NULL),
      _handle(-1)
{
    _pos = new unsigned int[PWM_CHANNELS];
    _handle = i2cOpen(PWM_I2C_BUS, PWM_I2C_ADDR, 0);
    if (_handle < 0) {
        cerr << "Open PCA9685 failed" << endl;
    }
}

Servos::~Servos()
{
    delete [] _pos;
    if (_handle >= 0) {
        i2cClose(_handle);
    }
}

unsigned int Servos::pos(unsigned int index) const
{
    if (index >= PWM_CHANNELS) {
        return 0;
    }

    return _pos[index];
}

unsigned int Servos::posPct(unsigned int index) const
{
    return pos(index) * 100 / PWM_CHANNELS;
}

void Servos::setPos(unsigned int index, unsigned int pos)
{
    if (index > PWM_CHANNELS) {
        return;
    }

    if (pos >= PWM_RESOLUTION) {
        pos = PWM_RESOLUTION;
    }

    _pos[index] = pos;
}

void Servos::setPosPct(unsigned int index, unsigned int pct)
{
    setPos(index, pct * PWM_CHANNELS / 100);
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
