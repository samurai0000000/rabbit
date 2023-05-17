/*
 * head.cxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <sys/time.h>
#include <bsd/sys/time.h>
#include "rabbit.hxx"

#define HEAD_ROTATION_SERVO         16
#define HEAD_ROTATION_LO_PULSE     610
#define HEAD_ROTATION_HI_PULSE    2260
#define HEAD_ROTATION_ANGLE_MULT  ((HEAD_ROTATION_HI_PULSE - HEAD_ROTATION_LO_PULSE) \
                                   / 180)

#define HEAD_TILT_SERVO             17
#define HEAD_TILT_LO_PULSE         600
#define HEAD_TILT_HI_PULSE        2300
#define HEAD_TILT_ANGLE_MULT       ((HEAD_TILT_HI_PULSE - HEAD_TILT_LO_PULSE) \
                                   / 180)

Head::Head()
    : _rotation(0.0),
      _tilt(0.0),
      _sentry(false)
{
    servos->setRange(HEAD_ROTATION_SERVO, HEAD_ROTATION_LO_PULSE, HEAD_ROTATION_HI_PULSE);
    servos->center(HEAD_ROTATION_SERVO);
    servos->setRange(HEAD_TILT_SERVO, HEAD_TILT_LO_PULSE, HEAD_TILT_HI_PULSE);
    servos->center(HEAD_TILT_SERVO);

    rotate(0.0);
    tilt(0.0);

    _running = true;
    pthread_mutex_init(&_mutex, NULL);
    pthread_cond_init(&_cond, NULL);
    pthread_create(&_thread, NULL, Head::thread_func, this);
}

Head::~Head()
{
    _running = false;
    pthread_cond_broadcast(&_cond);
    pthread_join(_thread, NULL);
    pthread_mutex_destroy(&_mutex);
    pthread_cond_destroy(&_cond);

    rotate(0.0);
    tilt(0.0);
}

void *Head::thread_func(void *args)
{
    Head *head = (Head *) args;

    head->run();

    return NULL;
}

void Head::run(void)
{
    struct timespec ts, tloop;
    vector<struct servo_motion> sentry_motions;
    struct servo_motion motion;

    clock_gettime(CLOCK_REALTIME, &ts);
    tloop.tv_sec = 0;
    tloop.tv_nsec = 100000000;

    /* Set up sentry motion vector */
    motion.pulse = (HEAD_ROTATION_HI_PULSE - HEAD_ROTATION_LO_PULSE) / 2 +
        HEAD_ROTATION_LO_PULSE;
    motion.ms = 50;
    sentry_motions.push_back(motion);
    motion.pulse = HEAD_ROTATION_HI_PULSE;
    motion.ms = 5000;
    sentry_motions.push_back(motion);
    motion.pulse = HEAD_ROTATION_LO_PULSE;
    motion.ms = 10000;
    sentry_motions.push_back(motion);
    motion.pulse = (HEAD_ROTATION_HI_PULSE - HEAD_ROTATION_LO_PULSE) / 2 +
        HEAD_ROTATION_LO_PULSE;
    motion.ms = 5000;
    sentry_motions.push_back(motion);

    while (_running) {
        timespecadd(&ts, &tloop, &ts);

        /* Sentry */
        if (_sentry) {
            if (servos->hasMotionSchedule(HEAD_ROTATION_SERVO) == false) {
                servos->scheduleMotions(HEAD_ROTATION_SERVO, sentry_motions);
            }
        } else {
            if (servos->hasMotionSchedule(HEAD_ROTATION_SERVO) == true) {
                servos->center(HEAD_ROTATION_SERVO);
            }
        }

        pthread_mutex_lock(&_mutex);
        pthread_cond_timedwait(&_cond, &_mutex, &ts);
        pthread_mutex_unlock(&_mutex);
    }
}

void Head::enSentry(bool enable)
{
    enable = (enable ? true : false);

    if (_sentry != enable) {
        _sentry = enable;
        if (enable) {
            mouth->cylon();
            speech->speak("Head sentry mode enabled");
            LOG("Head sentry mode enabled\n");
        } else {
            mouth->beh();
            speech->speak("Head sentry mode disabled");
            LOG("Head sentry mode disabled\n");
        }
    }
}

bool Head::isSentryEn(void) const
{
    return _sentry;
}

void Head::rotate(float deg, bool relative)
{
    unsigned int pulse;
    unsigned int center;
    char buf[128];

    deg = -deg;

    if (relative) {
        pulse = servos->pulse(HEAD_ROTATION_SERVO);
        pulse = (unsigned int) ((float) pulse + (HEAD_ROTATION_ANGLE_MULT * deg));
        servos->setPulse(HEAD_ROTATION_SERVO, pulse);
        _rotation += deg;
    } else {
        center =
            ((servos->hiRange(HEAD_ROTATION_SERVO) -
              servos->loRange(HEAD_ROTATION_SERVO)) / 2) +
            HEAD_ROTATION_LO_PULSE;
        pulse = (unsigned int) ((float) center - deg * HEAD_ROTATION_ANGLE_MULT);
        servos->setPulse(HEAD_ROTATION_SERVO, pulse);
        _rotation = deg;
    }

    snprintf(buf, sizeof(buf) - 1, "Head rotate to %.1f\n", rotationAt());
    LOG(buf);
}

void Head::tilt(float deg, bool relative)
{
    unsigned int pulse;
    unsigned int center;
    char buf[128];

    deg = -deg;

    if (relative) {
        pulse = servos->pulse(HEAD_TILT_SERVO);
        pulse = (unsigned int) ((float) pulse + (HEAD_TILT_ANGLE_MULT * deg));
        servos->setPulse(HEAD_TILT_SERVO, pulse);
        _tilt += deg;
    } else {
        center =
            ((servos->hiRange(HEAD_TILT_SERVO) - servos->loRange(HEAD_TILT_SERVO)) / 2) +
            HEAD_TILT_LO_PULSE;
        pulse = (unsigned int) ((float) center - deg * HEAD_TILT_ANGLE_MULT);
        servos->setPulse(HEAD_TILT_SERVO, pulse);
        _tilt = deg;
    }

    snprintf(buf, sizeof(buf) - 1, "Head tilt to %.1f\n", tiltAt());
    LOG(buf);
}


float Head::rotationAt(void) const
{
    return _rotation;
}

float Head::tiltAt(void) const
{
    return _tilt;
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
