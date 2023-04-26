/*
 * wheels.cxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <iostream>
#include "rabbit.hxx"

/*
 * https://lastminuteengineers.com/l298n-dc-stepper-driver-arduino-tutorial/
 * https://www.youtube.com/watch?v=_7COO5Xcff0
 *
 * rainbow wires       roomba wheel   pin
 *           red                red     A
 *        orange              black     B
 *        yellow               gray   GND
 *         green              white    5V
 *          blue              green   SIG
 *        purple              brown    DS
 *          gray              brown    DS
 */


#define RHS_SPEED_SERVO        14
#define RHS_SPEED_LO_PULSE      0
#define RHS_SPEED_HI_PULSE  19995
#define LHS_SPEED_SERVO        15
#define LHS_SPEED_LO_PULSE      0
#define LHS_SPEED_HI_PULSE  19995

#define L298N_IN1 23
#define L298N_IN2 24
#define L298N_IN3 17
#define L298N_IN4 27

using namespace std;

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

    gpioWrite(L298N_IN1, 0);
    gpioWrite(L298N_IN2, 0);
    gpioWrite(L298N_IN3, 0);
    gpioWrite(L298N_IN4, 0);

    servos->setRange(RHS_SPEED_SERVO, RHS_SPEED_LO_PULSE, RHS_SPEED_HI_PULSE);
    servos->setPct(RHS_SPEED_SERVO, 0);
    servos->setRange(LHS_SPEED_SERVO, LHS_SPEED_LO_PULSE, LHS_SPEED_HI_PULSE);
    servos->setPct(LHS_SPEED_SERVO, 0);
}

Wheels::~Wheels()
{
    if (_state != 0) {
        halt();
    }
}

void Wheels::halt(void)
{
    struct itimerval tv;

    if (_state == 0) {
        return;
    }

    tv.it_value.tv_sec = 0;
    tv.it_value.tv_usec = 0;
    tv.it_interval.tv_sec = 0;
    tv.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &tv, NULL);

    change(0);

    servos->setPct(RHS_SPEED_SERVO, 0);
    servos->setPct(LHS_SPEED_SERVO, 0);

    gpioWrite(L298N_IN1, 0);
    gpioWrite(L298N_IN2, 0);
    gpioWrite(L298N_IN3, 0);
    gpioWrite(L298N_IN4, 0);
}

static void my_setitimer(unsigned int ms)
{
    struct itimerval tv;

    tv.it_value.tv_sec = ms / 1000;
    tv.it_value.tv_usec = (ms - (ms / 1000)) * 1000;
    tv.it_interval.tv_sec = 0;
    tv.it_interval.tv_usec = 0;

    setitimer(ITIMER_REAL, &tv, NULL);
}

void Wheels::fwd(unsigned int ms)
{
    change(1);

    servos->setPct(RHS_SPEED_SERVO, 50);
    servos->setPct(LHS_SPEED_SERVO, 50);

    gpioWrite(L298N_IN1, 1);
    gpioWrite(L298N_IN2, 0);
    gpioWrite(L298N_IN3, 1);
    gpioWrite(L298N_IN4, 0);

    if (ms > 0) {
        my_setitimer(ms);
    }
}

void Wheels::bwd(unsigned int ms)
{
    change(2);

    servos->setPct(RHS_SPEED_SERVO, 50);
    servos->setPct(LHS_SPEED_SERVO, 50);

    gpioWrite(L298N_IN1, 0);
    gpioWrite(L298N_IN2, 1);
    gpioWrite(L298N_IN3, 0);
    gpioWrite(L298N_IN4, 1);

    if (ms > 0) {
        my_setitimer(ms);
    }
}

void Wheels::ror(unsigned int ms)
{
    change(3);

    servos->setPct(RHS_SPEED_SERVO, 30);
    servos->setPct(LHS_SPEED_SERVO, 30);

    gpioWrite(L298N_IN1, 0);
    gpioWrite(L298N_IN2, 1);
    gpioWrite(L298N_IN3, 1);
    gpioWrite(L298N_IN4, 0);

    if (ms > 0) {
        my_setitimer(ms);
    }
}

void Wheels::rol(unsigned int ms)
{
    change(4);

    servos->setPct(RHS_SPEED_SERVO, 30);
    servos->setPct(LHS_SPEED_SERVO, 30);

    gpioWrite(L298N_IN1, 1);
    gpioWrite(L298N_IN2, 0);
    gpioWrite(L298N_IN3, 0);
    gpioWrite(L298N_IN4, 1);

    if (ms > 0) {
        my_setitimer(ms);
    }
}

unsigned int Wheels::state(void) const
{
    return _state;
}

unsigned int Wheels::fwd_ms(void) const
{
    if (_state == 1) {
        return _fwd_ms + now_diff_ts_ms();
    }

    return _fwd_ms;
}

unsigned int Wheels::bwd_ms(void) const
{
    if (_state == 2) {
        return _bwd_ms + now_diff_ts_ms();
    }

    return _bwd_ms;
}

unsigned int Wheels::ror_ms(void) const
{
    if (_state == 3) {
        return _ror_ms + now_diff_ts_ms();
    }

    return _ror_ms;
}

unsigned int Wheels::rol_ms(void) const
{
    if (_state == 4) {
        return _rol_ms + now_diff_ts_ms();
    }

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

unsigned int Wheels::now_diff_ts_ms(void) const
{
    struct timeval now, tdiff;
    unsigned int ms;

    gettimeofday(&now, 0);
    timersub(&now, &_ts, &tdiff);
    ms = (tdiff.tv_sec * 1000) + (tdiff.tv_usec / 1000);

    return ms;
}

void Wheels::change(unsigned int state)
{
    unsigned int ms;

    switch (_state) {
    case 0:
        break;
    case 1:
        ms = now_diff_ts_ms();
        _fwd_ms += ms;
        break;
    case 2:
        ms = now_diff_ts_ms();
        _bwd_ms += ms;
        break;
    case 3:
        ms = now_diff_ts_ms();
        _ror_ms += ms;
        break;
    case 4:
        ms = now_diff_ts_ms();
        _rol_ms += ms;
        break;
    default:
        break;
    }

    _state = state;
    gettimeofday(&_ts, NULL);
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
