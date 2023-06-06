/*
 * head.cxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <sys/time.h>
#include <bsd/sys/time.h>
#include "rabbit.hxx"

#define HEAD_ROTATION_SERVO         16
#define HEAD_ROTATION_LO_PULSE     620
#define HEAD_ROTATION_HI_PULSE    2300
#define HEAD_ROTATION_ANGLE_MULT  ((HEAD_ROTATION_HI_PULSE -    \
                                    HEAD_ROTATION_LO_PULSE)     \
                                   / 180)

#define HEAD_TILT_SERVO             17
#define HEAD_TILT_LO_PULSE         560
#define HEAD_TILT_HI_PULSE        2310
#define HEAD_TILT_ANGLE_MULT      ((HEAD_TILT_HI_PULSE - \
                                    HEAD_TILT_LO_PULSE)  \
                                   / 180)

#define EB_R_ROTATION_SERVO         20
#define EB_R_ROTATION_LO_PULSE     975
#define EB_R_ROTATION_HI_PULSE    1975
#define EB_R_ROTATION_ANGLE_MULT  ((EB_R_ROTATION_HI_PULSE - \
                                    EB_R_ROTATION_LO_PULSE)  \
                                   / 90)

#define EB_R_TILT_SERVO             21
#define EB_R_TILT_LO_PULSE         975
#define EB_R_TILT_HI_PULSE        1975
#define EB_R_TILT_ANGLE_MULT      ((EB_R_TILT_HI_PULSE - \
                                    EB_R_TILT_LO_PULSE) \
                                   / 90)

#define EB_L_ROTATION_SERVO         22
#define EB_L_ROTATION_LO_PULSE     975
#define EB_L_ROTATION_HI_PULSE    1975
#define EB_L_ROTATION_ANGLE_MULT  ((EB_L_ROTATION_HI_PULSE - \
                                    EB_L_ROTATION_LO_PULSE)  \
                                   / 90)

#define EB_L_TILT_SERVO             23
#define EB_L_TILT_LO_PULSE         975
#define EB_L_TILT_HI_PULSE        1975
#define EB_L_TILT_ANGLE_MULT      ((EB_L_TILT_HI_PULSE -    \
                                    EB_L_TILT_LO_PULSE) \
                                   / 90)

using namespace std;

static unsigned int instance = 0;

Head::Head()
    : _rotation(0.0),
      _tilt(0.0),
      _sentry(false)
{
    if (instance != 0) {
        fprintf(stderr, "Head can be instantiated only once!\n");
        exit(EXIT_FAILURE);
    } else {
        instance++;
    }

    servos->setRange(HEAD_ROTATION_SERVO,
                     HEAD_ROTATION_LO_PULSE,
                     HEAD_ROTATION_HI_PULSE);
    servos->center(HEAD_ROTATION_SERVO);
    servos->setRange(HEAD_TILT_SERVO,
                     HEAD_TILT_LO_PULSE,
                     HEAD_TILT_HI_PULSE);
    servos->center(HEAD_TILT_SERVO);

    servos->setRange(EB_R_ROTATION_SERVO,
                     EB_R_ROTATION_LO_PULSE,
                     EB_R_ROTATION_HI_PULSE);
    servos->setRange(EB_R_TILT_SERVO,
                     EB_R_TILT_LO_PULSE,
                     EB_R_TILT_HI_PULSE);
    servos->setRange(EB_L_ROTATION_SERVO,
                     EB_L_ROTATION_LO_PULSE,
                     EB_L_ROTATION_HI_PULSE);
    servos->setRange(EB_L_TILT_SERVO,
                     EB_L_TILT_LO_PULSE,
                     EB_L_TILT_HI_PULSE);

    rotate(0.0);
    tilt(0.0);
    eyebrowSetDisposition(EB_RELAXED);

    _running = true;
    pthread_mutex_init(&_mutex, NULL);
    pthread_cond_init(&_cond, NULL);
    pthread_create(&_thread, NULL, Head::thread_func, this);

    printf("Head is online\n");
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
    eyebrowSetDisposition(EB_RELAXED);

    instance--;
    printf("Head is offline\n");
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

        /* Update eyebrows */
        updateEyebrows();

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
            speech->speak("Head sentry mode enabled");
            LOG("Head sentry mode enabled\n");
        } else {
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
        pulse = (unsigned int)
            ((float) center - deg * HEAD_ROTATION_ANGLE_MULT);
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

    if (relative) {
        pulse = servos->pulse(HEAD_TILT_SERVO);
        pulse = (unsigned int) ((float) pulse - (HEAD_TILT_ANGLE_MULT * deg));
        servos->setPulse(HEAD_TILT_SERVO, pulse);
        _tilt += deg;
    } else {
        center =
            ((servos->hiRange(HEAD_TILT_SERVO) -
              servos->loRange(HEAD_TILT_SERVO)) / 2) +
            HEAD_TILT_LO_PULSE;
        pulse = (unsigned int) ((float) center + deg * HEAD_TILT_ANGLE_MULT);
        servos->setPulse(HEAD_TILT_SERVO, pulse);
        _tilt = deg;
    }

    snprintf(buf, sizeof(buf) - 1, "Head tilt to %.1f\n", tiltAt());
    LOG(buf);
}


float Head::rotationAt(void) const
{
    unsigned int pulse;
    unsigned int center;

    pulse = servos->pulse(HEAD_ROTATION_SERVO);
    center = ((servos->hiRange(HEAD_ROTATION_SERVO) -
               servos->loRange(HEAD_ROTATION_SERVO)) / 2) +
        HEAD_ROTATION_LO_PULSE;

    return ((float) pulse - (float) center) / HEAD_ROTATION_ANGLE_MULT;
}

float Head::tiltAt(void) const
{
    unsigned int pulse;
    unsigned int center;

    pulse = servos->pulse(HEAD_TILT_SERVO);
    center = ((servos->hiRange(HEAD_TILT_SERVO) -
               servos->loRange(HEAD_TILT_SERVO)) / 2) +
        HEAD_TILT_LO_PULSE;

    return ((float) pulse - (float) center) / HEAD_TILT_ANGLE_MULT * -1.0;
}

void Head::eyebrowRotate(float deg, bool relative, unsigned int lr)
{
    unsigned int pulse;
    unsigned int center;
    char buf[128];

    if (lr & 0x1) {
        if (relative) {
            pulse = servos->pulse(EB_R_ROTATION_SERVO);
            pulse = (unsigned int) ((float) pulse +
                                    (EB_R_ROTATION_ANGLE_MULT * deg));
            servos->setPulse(EB_R_ROTATION_SERVO, pulse);
            _eb_r_rotation += deg;
        } else {
            center =
                ((servos->hiRange(EB_R_ROTATION_SERVO) -
                  servos->loRange(EB_R_ROTATION_SERVO)) / 2) +
                EB_R_ROTATION_LO_PULSE;
            pulse = (unsigned int)
                ((float) center - deg * EB_R_ROTATION_ANGLE_MULT);
            servos->setPulse(EB_R_ROTATION_SERVO, pulse);
            _eb_r_rotation = deg;
        }

        snprintf(buf, sizeof(buf) - 1, "Right eyebrow rotate to %.1f\n",
                 eyebrowRotationAt(0x1));
        LOG(buf);
    }

    if (lr & 0x2) {
        deg = -deg;

        if (relative) {
            pulse = servos->pulse(EB_L_ROTATION_SERVO);
            pulse = (unsigned int) ((float) pulse +
                                    (EB_L_ROTATION_ANGLE_MULT * deg));
            servos->setPulse(EB_L_ROTATION_SERVO, pulse);
            _eb_l_rotation += deg;
        } else {
            center =
                ((servos->hiRange(EB_L_ROTATION_SERVO) -
                  servos->loRange(EB_L_ROTATION_SERVO)) / 2) +
                EB_L_ROTATION_LO_PULSE;
            pulse = (unsigned int)
                ((float) center - deg * EB_L_ROTATION_ANGLE_MULT);
            servos->setPulse(EB_L_ROTATION_SERVO, pulse);
            _eb_l_rotation = deg;
        }

        snprintf(buf, sizeof(buf) - 1, "Left eyebrow rotate to %.1f\n",
                 eyebrowRotationAt(0x2));
        LOG(buf);
    }
}

void Head::eyebrowTilt(float deg, bool relative, unsigned int lr)
{
    unsigned int pulse;
    unsigned int center;
    char buf[128];

    deg = -deg;

    if (lr & 0x1) {
        if (relative) {
            pulse = servos->pulse(EB_R_TILT_SERVO);
            pulse = (unsigned int) ((float) pulse +
                                    (EB_R_TILT_ANGLE_MULT * deg));
            servos->setPulse(EB_R_TILT_SERVO, pulse);
            _eb_r_tilt += deg;
        } else {
            center =
                ((servos->hiRange(EB_R_TILT_SERVO) -
                  servos->loRange(EB_R_TILT_SERVO)) / 2) +
                EB_R_TILT_LO_PULSE;
            pulse = (unsigned int)
                ((float) center + deg * EB_R_TILT_ANGLE_MULT);
            servos->setPulse(EB_R_TILT_SERVO, pulse);
            _eb_r_tilt = deg;
        }

        snprintf(buf, sizeof(buf) - 1, "Right eyebrow tilt to %.1f\n",
                 eyebrowTiltAt(0x1));
        LOG(buf);
    }

    if (lr & 0x2) {
        if (relative) {
            pulse = servos->pulse(EB_L_TILT_SERVO);
            pulse = (unsigned int) ((float) pulse -
                                    (EB_L_TILT_ANGLE_MULT * deg));
            servos->setPulse(EB_L_TILT_SERVO, pulse);
            _eb_l_tilt += deg;
        } else {
            center =
                ((servos->hiRange(EB_L_TILT_SERVO) -
                  servos->loRange(EB_L_TILT_SERVO)) / 2) +
                EB_L_TILT_LO_PULSE;
            pulse = (unsigned int)
                ((float) center - deg * EB_L_TILT_ANGLE_MULT);
            servos->setPulse(EB_L_TILT_SERVO, pulse);
            _eb_l_tilt = deg;
        }

        snprintf(buf, sizeof(buf) - 1, "Left eyebrow tilt to %.1f\n",
                 eyebrowTiltAt(0x2));
        LOG(buf);
    }
}

void Head::eyebrowSetDisposition(enum EyebrowDisposition disposition)
{
    _eyebrow = disposition;

    switch (disposition) {
    case EB_RELAXED:
        eyebrowRotate(0.0);
        eyebrowTilt(0.0);
        break;
    case EB_PERPLEXED:
        eyebrowRotate(20.0, false, 0x1);
        eyebrowRotate(0.0, false, 0x2);
        eyebrowTilt(0.0, false, 0x1);
        eyebrowTilt(-45.0, false, 0x2);
        break;
    case EB_SURPRISED:
        eyebrowRotate(30.0);
        eyebrowTilt(25.0);
        break;
    case EB_HAPPY:
        eyebrowRotate(10.0);
        eyebrowTilt(10.0);
        break;
    case EB_JUBILANT:
        eyebrowRotate(30.0);
        eyebrowTilt(20.0);
        break;
    case EB_ANGRY:
        eyebrowRotate(-10.0);
        eyebrowTilt(-20.0);
        break;
    case EB_FURIOUS:
        eyebrowRotate(-20.0);
        eyebrowTilt(-30.0);
        break;
    case EB_SAD:
        eyebrowRotate(-20.0);
        eyebrowTilt(20.0);
        break;
    case EB_DEPRESSED:
        eyebrowRotate(-30.0);
        eyebrowTilt(30.0);
        break;
    case EB_MANUAL:
    default:
        break;
    }
}

float Head::eyebrowRotationAt(unsigned int lr) const
{
    unsigned int pulse;
    unsigned int center;

    if (lr & 0x1) {
        pulse = servos->pulse(EB_R_ROTATION_SERVO);
        center = ((servos->hiRange(EB_R_ROTATION_SERVO) -
                   servos->loRange(EB_R_ROTATION_SERVO)) / 2) +
            EB_R_ROTATION_LO_PULSE;
        return ((float) center - (float) pulse) / EB_R_ROTATION_ANGLE_MULT;
    }

    pulse = servos->pulse(EB_L_ROTATION_SERVO);
    center = ((servos->hiRange(EB_L_ROTATION_SERVO) -
               servos->loRange(EB_L_ROTATION_SERVO)) / 2) +
        EB_L_ROTATION_LO_PULSE;
    return ((float) pulse - (float) center) / EB_L_ROTATION_ANGLE_MULT;
}

float Head::eyebrowTiltAt(unsigned int lr) const
{
    unsigned int pulse;
    unsigned int center;

    if (lr & 0x1) {
        pulse = servos->pulse(EB_R_TILT_SERVO);
        center = ((servos->hiRange(EB_R_TILT_SERVO) -
                   servos->loRange(EB_R_TILT_SERVO)) / 2) +
            EB_R_TILT_LO_PULSE;
        return ((float) center - (float) pulse) / EB_R_TILT_ANGLE_MULT;
    }

    pulse = servos->pulse(EB_L_TILT_SERVO);
    center = ((servos->hiRange(EB_L_TILT_SERVO) -
               servos->loRange(EB_L_TILT_SERVO)) / 2) +
        EB_L_TILT_LO_PULSE;
    return ((float) pulse - (float) center) / EB_R_TILT_ANGLE_MULT;
}

enum Head::EyebrowDisposition Head::eyebrowDisposition(void) const
{
    return _eyebrow;
}

static unsigned int random_servo_pos_unadjusted(unsigned int id)
{
    unsigned int lo, hi;
    unsigned newpulse;

    lo = servos->loRange(id);
    hi = servos->hiRange(id);
    newpulse = random() % (hi - lo);

    return newpulse;
}

void Head::updateEyebrows(void)
{
    /* Twitch from time to time */
    if (((random() % 1000) == 888) &&
        (servos->hasMotionSchedule(EB_R_ROTATION_SERVO) == false) &&
        (servos->hasMotionSchedule(EB_R_TILT_SERVO) == false) &&
        (servos->hasMotionSchedule(EB_L_ROTATION_SERVO) == false) &&
        (servos->hasMotionSchedule(EB_L_TILT_SERVO) == false)) {
        unsigned int id;
        vector<struct servo_motion> motions;
        struct servo_motion motion;
        unsigned int pr, pt;

        pr = random_servo_pos_unadjusted(EB_R_ROTATION_SERVO);
        pt = random_servo_pos_unadjusted(EB_R_TILT_SERVO);

        id = EB_R_ROTATION_SERVO;
        motions.clear();
        motion.pulse = servos->loRange(id) + pr;
        motion.ms = 200;
        motions.push_back(motion);
        motion.pulse = servos->pulse(id);
        motion.ms = 200;
        motions.push_back(motion);
        servos->clearMotionSchedule(id);
        servos->scheduleMotions(id, motions);

        id = EB_L_ROTATION_SERVO;
        motions.clear();
        motion.pulse = servos->hiRange(id) - pr;
        motion.ms = 200;
        motions.push_back(motion);
        motion.pulse = servos->pulse(id);
        motion.ms = 200;
        motions.push_back(motion);
        servos->clearMotionSchedule(id);
        servos->scheduleMotions(id, motions);

        id = EB_R_TILT_SERVO;
        motions.clear();
        motion.pulse = servos->loRange(id) + pt;
        motion.ms = 200;
        motions.push_back(motion);
        motion.pulse = servos->pulse(id);
        motion.ms = 200;
        motions.push_back(motion);
        servos->clearMotionSchedule(id);
        servos->scheduleMotions(id, motions);


        id = EB_L_TILT_SERVO;
        motions.clear();
        motion.pulse = servos->hiRange(id) - pt;
        motion.ms = 200;
        motions.push_back(motion);
        motion.pulse = servos->pulse(id);
        motion.ms = 200;
        motions.push_back(motion);
        servos->clearMotionSchedule(id);
        servos->scheduleMotions(id, motions);
    }
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
