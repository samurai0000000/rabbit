/*
 * wheels.cxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <string.h>
#include <pigpio.h>
#include "wheels.hxx"

/*
 * https://lastminuteengineers.com/l298n-dc-stepper-driver-arduino-tutorial/
 */

#define L298N_IN1 23
#define L298N_IN2 24
#define L298N_IN3 17
#define L298N_IN4 27

Wheels::Wheels()
    : _state(0),
      _ts(),
      _fwd_ms(0),
      _bwd_ms(0),
      _ror_ms(0),
      _rol_ms(0)
{
    gpioSetMode(L298N_IN1, PI_OUTPUT);
    gpioSetMode(L298N_IN2, PI_OUTPUT);
    gpioSetMode(L298N_IN3, PI_OUTPUT);
    gpioSetMode(L298N_IN4, PI_OUTPUT);

    gpioSetMode(L298N_IN1, 0);
    gpioSetMode(L298N_IN2, 0);
    gpioSetMode(L298N_IN3, 0);
    gpioSetMode(L298N_IN4, 0);
}

Wheels::~Wheels()
{
    halt();
}

void Wheels::halt(void)
{
    change(0);

    gpioSetMode(L298N_IN1, 0);
    gpioSetMode(L298N_IN2, 0);
    gpioSetMode(L298N_IN3, 0);
    gpioSetMode(L298N_IN4, 0);
}

void Wheels::fwd(unsigned int ms)
{
    change(1);

    gpioSetMode(L298N_IN1, 0);
    gpioSetMode(L298N_IN2, 1);
    gpioSetMode(L298N_IN3, 0);
    gpioSetMode(L298N_IN4, 1);

    if (ms > 0) {

    }
}

void Wheels::bwd(unsigned int ms)
{
    change(2);

    gpioSetMode(L298N_IN1, 1);
    gpioSetMode(L298N_IN2, 0);
    gpioSetMode(L298N_IN3, 1);
    gpioSetMode(L298N_IN4, 0);

    if (ms > 0) {
    }
}

void Wheels::ror(unsigned int ms)
{
    change(3);

    gpioSetMode(L298N_IN1, 0);
    gpioSetMode(L298N_IN2, 1);
    gpioSetMode(L298N_IN3, 1);
    gpioSetMode(L298N_IN4, 0);

    if (ms > 0) {
    }
}

void Wheels::rol(unsigned int ms)
{
    change(4);

    gpioSetMode(L298N_IN1, 1);
    gpioSetMode(L298N_IN2, 0);
    gpioSetMode(L298N_IN3, 0);
    gpioSetMode(L298N_IN4, 1);

    if (ms > 0) {
    }
}

unsigned int Wheels::state(void) const
{
    return _state;
}

unsigned int Wheels::fwd_ms(void) const
{
    return _fwd_ms;
}

unsigned int Wheels::bwd_ms(void) const
{
    return _bwd_ms;
}

unsigned int Wheels::ror_ms(void) const
{
    return _ror_ms;
}

unsigned int Wheels::rol_ms(void) const
{
    return _rol_ms;
}

const char *Wheels::stateStr(void) const
{
    const char *s[6] = {
        "halt",
        "fwd",
        "bwd",
        "ror",
        "rol",
        "unknown",
    };

    if (_state <= 4) {
        return s[_state];
    }

    return s[5];
}

void Wheels::change(unsigned int state)
{
    struct timeval now, tdiff;
    unsigned int ms;

    gettimeofday(&now, 0);
    timersub(&now, &_ts, &tdiff);
    ms = (tdiff.tv_sec * 1000) + (tdiff.tv_usec / 1000);

    switch (_state) {
    case 0:
        break;
    case 1:
        _fwd_ms += ms;
        break;
    case 2:
        _bwd_ms += ms;
        break;
    case 3:
        _ror_ms += ms;
        break;
    case 4:
        _rol_ms += ms;
        break;
    default:
        break;
    }

    _state = state;
    memcpy(&_ts, &now, sizeof(struct timeval));
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
