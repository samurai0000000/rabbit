/*
 * servos.cxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include "rabbit.hxx"

#define PWM_CHANNELS   16
#define PWM_RESOLUTION 4096

Servos::Servos()
    : _pos(NULL)
{
    _pos = new unsigned int[PWM_CHANNELS];
}

Servos::~Servos()
{
    delete [] _pos;
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
