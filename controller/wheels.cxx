/*
 * wheels.cxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <sys/time.h>
#include <bsd/sys/time.h>
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

static const char *state_string[10] = {
    "halt",
    "fwd",
    "bwd",
    "ror",
    "rol",
    "fwr",
    "fwl",
    "bwr",
    "bwl",
    "unknown",
};

static unsigned int instance = 0;

Wheels::Wheels()
{
    if (instance != 0) {
        fprintf(stderr, "Wheels can be instantiated only once!\n");
        exit(EXIT_FAILURE);
    } else {
        instance++;
    }

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

    _state = 0;
    _rpwr = 0;
    _rpwrTarget = 0;
    _lpwr = 0;
    _lpwrTarget = 0;

    _fwd_ms = 0;
    _bwd_ms = 0;
    _ror_ms = 0;
    _rol_ms = 0;
    _fwr_ms = 0;
    _fwl_ms = 0;
    _bwr_ms = 0;
    _bwl_ms = 0;

    _running = true;
    pthread_mutex_init(&_mutex, NULL);
    pthread_cond_init(&_cond, NULL);
    pthread_create(&_thread, NULL, Wheels::thread_func, this);
    pthread_setname_np(_thread, "R'Wheels");

    printf("Wheels is online\n");
}

Wheels::~Wheels()
{
    if (_state != 0) {
        halt();
    }

    _running = false;
    pthread_cond_broadcast(&_cond);
    pthread_join(_thread, NULL);
    pthread_mutex_destroy(&_mutex);
    pthread_cond_destroy(&_cond);

    instance--;
    printf("Wheels is offline\n");
}

void *Wheels::thread_func(void *args)
{
    Wheels *wheels = (Wheels *) args;

    wheels->run();

    return NULL;
}

void Wheels::run(void)
{
    struct timespec ts, tloop;

    tloop.tv_sec = 0;
    tloop.tv_nsec = 50000000;

    while (_running) {
        clock_gettime(CLOCK_REALTIME, &ts);
        timespecadd(&ts, &tloop, &ts);

        if (hasExpired()) {
            halt();
        }

        if (_rpwr != _rpwrTarget) {
            if (_rpwrTarget < _rpwr) {
                _rpwr -= 5;
                if (_rpwr > 100) {
                    _rpwr = 0;
                }
            } else {
                if (_rpwr < 30) {
                    _rpwr = 30;
                } else {
                    _rpwr += 1;
                }
                if (_rpwr > _rpwrTarget) {
                    _rpwr = _rpwrTarget;
                }
            }

            servos->setPct(RHS_SPEED_SERVO, _rpwr);
        }

        if (_lpwr != _lpwrTarget) {
            if (_lpwrTarget < _lpwr) {
                _lpwr -= 5;
                if (_lpwr > 100) {
                    _lpwr = 0;
                }
            } else {
                if (_lpwr < 30) {
                    _lpwr = 30;
                } else {
                    _lpwr += 1;
                }
                if (_lpwr > _lpwrTarget) {
                    _lpwr = _lpwrTarget;
                }
            }
            servos->setPct(LHS_SPEED_SERVO, _lpwr);
        }

        if (_rpwr == 0) {
            gpioWrite(L298N_IN1, 0);
            gpioWrite(L298N_IN2, 0);
        }

        if (_lpwr == 0) {
            gpioWrite(L298N_IN3, 0);
            gpioWrite(L298N_IN4, 0);
        }

        pthread_mutex_lock(&_mutex);
        pthread_cond_timedwait(&_cond, &_mutex, &ts);
        pthread_mutex_unlock(&_mutex);
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
    tv.it_interval.tv_sec = 70;
    tv.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &tv, NULL);

    change(0);

    _rpwrTarget = 0;
    _lpwrTarget = 0;
}

void Wheels::fwd(unsigned int ms)
{
    if (_state != 1) {
        change(1);

        _rpwrTarget = 75;
        _lpwrTarget = 75;

        gpioWrite(L298N_IN1, 1);
        gpioWrite(L298N_IN2, 0);
        gpioWrite(L298N_IN3, 1);
        gpioWrite(L298N_IN4, 0);
    }

    if (ms > 0) {
        setExpiration(ms);
    }
}

void Wheels::bwd(unsigned int ms)
{
    if (_state != 2) {
        change(2);

        _rpwrTarget = 75;
        _lpwrTarget = 75;

        gpioWrite(L298N_IN1, 0);
        gpioWrite(L298N_IN2, 1);
        gpioWrite(L298N_IN3, 0);
        gpioWrite(L298N_IN4, 1);
    }

    if (ms > 0) {
        setExpiration(ms);
    }
}

void Wheels::ror(unsigned int ms)
{
    if (_state != 3) {
        change(3);

        _rpwrTarget = 50;
        _lpwrTarget = 50;

        gpioWrite(L298N_IN1, 0);
        gpioWrite(L298N_IN2, 1);
        gpioWrite(L298N_IN3, 1);
        gpioWrite(L298N_IN4, 0);
    }

    if (ms > 0) {
        setExpiration(ms);
    }
}

void Wheels::rol(unsigned int ms)
{
    if (_state != 4) {
        change(4);

        _rpwrTarget = 50;
        _lpwrTarget = 50;

        gpioWrite(L298N_IN1, 1);
        gpioWrite(L298N_IN2, 0);
        gpioWrite(L298N_IN3, 0);
        gpioWrite(L298N_IN4, 1);
    }

    if (ms > 0) {
        setExpiration(ms);
    }
}

void Wheels::fwr(unsigned int ms)
{
    if (_state != 5) {
        change(5);

        _rpwrTarget = 8;
        _lpwrTarget = 75;

        gpioWrite(L298N_IN1, 1);
        gpioWrite(L298N_IN2, 0);
        gpioWrite(L298N_IN3, 1);
        gpioWrite(L298N_IN4, 0);
    }

    if (ms > 0) {
        setExpiration(ms);
    }
}

void Wheels::fwl(unsigned int ms)
{
    if (_state != 6) {
        change(6);

        _rpwrTarget = 75;
        _lpwrTarget = 8;

        gpioWrite(L298N_IN1, 1);
        gpioWrite(L298N_IN2, 0);
        gpioWrite(L298N_IN3, 1);
        gpioWrite(L298N_IN4, 0);
    }

    if (ms > 0) {
        setExpiration(ms);
    }
}

void Wheels::bwr(unsigned int ms)
{
    if (_state != 7) {
        change(7);

        _rpwrTarget = 8;
        _lpwrTarget = 75;

        gpioWrite(L298N_IN1, 0);
        gpioWrite(L298N_IN2, 1);
        gpioWrite(L298N_IN3, 0);
        gpioWrite(L298N_IN4, 1);
    }

    if (ms > 0) {
        setExpiration(ms);
    }
}

void Wheels::bwl(unsigned int ms)
{
    if (_state != 8) {
        change(8);

        _rpwrTarget = 75;
        _lpwrTarget = 8;

        gpioWrite(L298N_IN1, 0);
        gpioWrite(L298N_IN2, 1);
        gpioWrite(L298N_IN3, 0);
        gpioWrite(L298N_IN4, 1);
    }

    if (ms > 0) {
        setExpiration(ms);
    }
}

unsigned int Wheels::state(void) const
{
    return _state;
}

const char *Wheels::stateStr(void) const
{
    if (_state <= 8) {
        return state_string[_state];
    }

    return state_string[9];
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

unsigned int Wheels::fwr_ms(void) const
{
    if (_state == 5) {
        return _fwr_ms + now_diff_ts_ms();
    }

    return _fwr_ms;
}

unsigned int Wheels::fwl_ms(void) const
{
    if (_state == 6) {
        return _fwl_ms + now_diff_ts_ms();
    }

    return _fwl_ms;
}

unsigned int Wheels::bwr_ms(void) const
{
    if (_state == 7) {
        return _bwr_ms + now_diff_ts_ms();
    }

    return _bwr_ms;
}

unsigned int Wheels::bwl_ms(void) const
{
    if (_state == 8) {
        return _bwl_ms + now_diff_ts_ms();
    }

    return _bwl_ms;
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

    ms = now_diff_ts_ms();

    switch (_state) {
    case 0:                break;
    case 1: _fwd_ms += ms; break;
    case 2: _bwd_ms += ms; break;
    case 3: _ror_ms += ms; break;
    case 4: _rol_ms += ms; break;
    case 5: _fwr_ms += ms; break;
    case 6: _fwl_ms += ms; break;
    case 7: _bwr_ms += ms; break;
    case 8: _bwl_ms += ms; break;
    default:               break;
    }

    if (_state != state) {
        char buf[64];

        if (_state == 0) {
            snprintf(buf, sizeof(buf) - 1, "Wheel %s -> %s\n",
                     state_string[_state], state_string[state]);
        } else {
            snprintf(buf, sizeof(buf) - 1, "Wheel %s %ums -> %s\n",
                     state_string[_state], ms, state_string[state]);
        }

        switch (state) {
        case 0:
            if (ms > 1000) {
                //speech->speak("Halt");
            }
            break;
        case 1:
        case 5:
        case 6:
            if (_state != 1 && _state != 5 && _state != 6) {
                //speech->speak("Forward");
            }
            break;
        case 2:
        case 7:
        case 8:
            if (_state != 2 && _state != 7 && _state != 8) {
                //speech->speak("Backward");
            }
            break;
        case 3:
            //speech->speak("Spin right");
            break;
        case 4:
            //speech->speak("Spin left");
            break;
        default:
            break;
        }

        LOG(buf);
    }

    _state = state;
    gettimeofday(&_ts, NULL);
}

void Wheels::setExpiration(unsigned int ms)
{
    struct timeval now;
    struct timeval expiry;

    gettimeofday(&now, NULL);

    expiry.tv_sec = ms / 1000;
    expiry.tv_usec = (ms - (ms / 1000)) * 1000;

    timeradd(&now, &expiry, &_expire);
}

bool Wheels::hasExpired(void) const
{
    struct timeval now;

    gettimeofday(&now, NULL);

    if (timercmp(&now, &_expire, >=)) {
        return true;
    }

    return false;
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
