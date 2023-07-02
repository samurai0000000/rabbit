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
#define EB_R_ROTATION_LO_PULSE    1080
#define EB_R_ROTATION_HI_PULSE    2080
#define EB_R_ROTATION_ANGLE_MULT  ((EB_R_ROTATION_HI_PULSE - \
                                    EB_R_ROTATION_LO_PULSE)  \
                                   / 90)

#define EB_R_TILT_SERVO             21
#define EB_R_TILT_LO_PULSE        1050
#define EB_R_TILT_HI_PULSE        2050
#define EB_R_TILT_ANGLE_MULT      ((EB_R_TILT_HI_PULSE - \
                                    EB_R_TILT_LO_PULSE) \
                                   / 90)

#define EB_L_ROTATION_SERVO         22
#define EB_L_ROTATION_LO_PULSE     930
#define EB_L_ROTATION_HI_PULSE    1930
#define EB_L_ROTATION_ANGLE_MULT  ((EB_L_ROTATION_HI_PULSE - \
                                    EB_L_ROTATION_LO_PULSE)  \
                                   / 90)

#define EB_L_TILT_SERVO             23
#define EB_L_TILT_LO_PULSE        1000
#define EB_L_TILT_HI_PULSE        2000
#define EB_L_TILT_ANGLE_MULT      ((EB_L_TILT_HI_PULSE -    \
                                    EB_L_TILT_LO_PULSE) \
                                   / 90)

#define EAR_R_TILT_SERVO            24
#define EAR_R_TILT_LO_PULSE        650
#define EAR_R_TILT_HI_PULSE       2300
#define EAR_R_TILT_ANGLE_MULT     ((EAR_R_TILT_HI_PULSE -   \
                                    EAR_R_TILT_LO_PULSE) \
                                   / 180)

#define EAR_L_TILT_SERVO            25
#define EAR_L_TILT_LO_PULSE        650
#define EAR_L_TILT_HI_PULSE       2300
#define EAR_L_TILT_ANGLE_MULT     ((EAR_L_TILT_HI_PULSE -   \
                                    EAR_L_TILT_LO_PULSE)    \
                                   / 180)

#define EAR_R_ROTATION_SERVO        26
#define EAR_R_ROTATION_LO_PULSE    700
#define EAR_R_ROTATION_HI_PULSE   2100
#define EAR_R_ROTATION_ANGLE_MULT ((EAR_R_ROTATION_HI_PULSE -   \
                                    EAR_R_ROTATION_LO_PULSE)    \
                                   / 100)

#define EAR_L_ROTATION_SERVO        27
#define EAR_L_ROTATION_LO_PULSE    700
#define EAR_L_ROTATION_HI_PULSE   2100
#define EAR_L_ROTATION_ANGLE_MULT ((EAR_L_ROTATION_HI_PULSE -   \
                                    EAR_L_ROTATION_LO_PULSE)    \
                                   / 100)

#define EAR_DROP_SECONDS            15

#define TOOTH_R_ROTATION_SERVO          28
#define TOOTH_R_ROTATION_LO_PULSE     1000
#define TOOTH_R_ROTATION_HI_PULSE     1900
#define TOOTH_R_ROTATION_ANGLE_MULT             \
    ((TOOTH_R_ROTATION_HI_PULSE - TOOTH_R_ROTATION_LO_PULSE) / 90)

#define TOOTH_L_ROTATION_SERVO          29
#define TOOTH_L_ROTATION_LO_PULSE     1000
#define TOOTH_L_ROTATION_HI_PULSE     1900
#define TOOTH_L_ROTATION_ANGLE_MULT             \
    ((TOOTH_L_ROTATION_HI_PULSE - TOOTH_L_ROTATION_LO_PULSE) / 90)

#define WHISKER_R_ROTATION_SERVO        30
#define WHISKER_R_ROTATION_LO_PULSE   1300
#define WHISKER_R_ROTATION_HI_PULSE   1600
#define WHISKER_R_ROTATION_ANGLE_MULT             \
    ((WHISKER_R_ROTATION_HI_PULSE - WHISKER_R_ROTATION_LO_PULSE) / 60)

#define WHISKER_L_ROTATION_SERVO        31
#define WHISKER_L_ROTATION_LO_PULSE   1300
#define WHISKER_L_ROTATION_HI_PULSE   1600
#define WHISKER_L_ROTATION_ANGLE_MULT             \
    ((WHISKER_L_ROTATION_HI_PULSE - TOOTH_L_ROTATION_LO_PULSE) / 60)

using namespace std;

static unsigned int instance = 0;

Head::Head()
    : _rotation(0.0),
      _tilt(0.0),
      _teethAnimateEn(false),
      _teethAnimateSync(true),
      _teethRandPct(0),
      _whiskersAnimateEn(false),
      _whiskersAnimateSync(true),
      _whiskersRandPct(0),
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

    servos->setRange(EAR_R_TILT_SERVO,
                     EAR_R_TILT_LO_PULSE,
                     EAR_R_TILT_HI_PULSE);
    servos->setRange(EAR_L_TILT_SERVO,
                     EAR_L_TILT_LO_PULSE,
                     EAR_L_TILT_HI_PULSE);
    servos->setRange(EAR_R_ROTATION_SERVO,
                     EAR_R_ROTATION_LO_PULSE,
                     EAR_R_ROTATION_HI_PULSE);
    servos->setRange(EAR_L_ROTATION_SERVO,
                     EAR_L_ROTATION_LO_PULSE,
                     EAR_L_ROTATION_HI_PULSE);

    servos->setRange(TOOTH_R_ROTATION_SERVO,
                     TOOTH_R_ROTATION_LO_PULSE,
                     TOOTH_R_ROTATION_HI_PULSE);
    servos->setRange(TOOTH_L_ROTATION_SERVO,
                     TOOTH_L_ROTATION_LO_PULSE,
                     TOOTH_L_ROTATION_HI_PULSE);

    servos->setRange(WHISKER_R_ROTATION_SERVO,
                     WHISKER_R_ROTATION_LO_PULSE,
                     WHISKER_R_ROTATION_HI_PULSE);
    servos->setRange(WHISKER_L_ROTATION_SERVO,
                     WHISKER_L_ROTATION_LO_PULSE,
                     WHISKER_L_ROTATION_HI_PULSE);

    rotate(0.0);
    tilt(0.0);
    eyebrowSetDisposition(EB_RELAXED);
    earsUp();
    teethDown();
    teethRandomize(1);
    whiskersCenter();
    whiskersRandomize(1);

    _running = true;
    pthread_mutex_init(&_mutex, NULL);
    pthread_cond_init(&_cond, NULL);
    pthread_create(&_thread, NULL, Head::thread_func, this);
    pthread_setname_np(_thread, "R'Head");

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
    earsDown();
    teethDown();
    whiskersDown();

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
        clock_gettime(CLOCK_REALTIME, &ts);
        timespecadd(&ts, &tloop, &ts);

        updateEyebrows();
        updateEars();
        updateTeeth();
        updateWhiskers();

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
        eyebrowRotate(-45.0, false, 0x1);
        eyebrowRotate(0.0, false, 0x2);
        eyebrowTilt(0.0, false, 0x1);
        eyebrowTilt(0.0, false, 0x2);
        break;
    case EB_SURPRISED:
        eyebrowRotate(5.0);
        eyebrowTilt(45.0);
        break;
    case EB_HAPPY:
        eyebrowRotate(5.0);
        eyebrowTilt(30.0);
        break;
    case EB_JUBILANT:
        eyebrowRotate(5.0);
        eyebrowTilt(45.0);
        break;
    case EB_ANGRY:
        eyebrowRotate(-20.0);
        eyebrowTilt(-5.0);
        break;
    case EB_FURIOUS:
        eyebrowRotate(-35.0);
        eyebrowTilt(-15.0);
        break;
    case EB_SAD:
        eyebrowRotate(20.0);
        eyebrowTilt(-20.0);
        break;
    case EB_DEPRESSED:
        eyebrowRotate(25.0);
        eyebrowTilt(-35.0);
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

void Head::earTilt(float deg, bool relative, unsigned int lr)
{
    unsigned int pulse;
    unsigned int center;
    char buf[128];

    if (lr & 0x1) {
        if (relative) {
            pulse = servos->pulse(EAR_R_TILT_SERVO);
            pulse = (unsigned int) ((float) pulse +
                                    (EAR_R_TILT_ANGLE_MULT * deg));
            servos->setPulse(EAR_R_TILT_SERVO, pulse);
        } else {
            center =
                ((servos->hiRange(EAR_R_TILT_SERVO) -
                  servos->loRange(EAR_R_TILT_SERVO)) / 2) +
                EAR_R_TILT_LO_PULSE;
            pulse = (unsigned int)
                ((float) center + deg * EAR_R_TILT_ANGLE_MULT);
            servos->setPulse(EAR_R_TILT_SERVO, pulse);
        }

        snprintf(buf, sizeof(buf) - 1, "Right ear tilt to %.1f\n",
                 earTiltAt(0x1));
        LOG(buf);
    }

    if (lr & 0x2) {
        if (relative) {
            pulse = servos->pulse(EAR_L_TILT_SERVO);
            pulse = (unsigned int) ((float) pulse -
                                    (EAR_L_TILT_ANGLE_MULT * deg));
            servos->setPulse(EAR_L_TILT_SERVO, pulse);
        } else {
            center =
                ((servos->hiRange(EAR_L_TILT_SERVO) -
                  servos->loRange(EAR_L_TILT_SERVO)) / 2) +
                EAR_L_TILT_LO_PULSE;
            pulse = (unsigned int)
                ((float) center - deg * EAR_L_TILT_ANGLE_MULT);
            servos->setPulse(EAR_L_TILT_SERVO, pulse);
        }

        snprintf(buf, sizeof(buf) - 1, "Left ear tilt to %.1f\n",
                 earTiltAt(0x2));
        LOG(buf);
    }
}

void Head::earRotate(float deg, bool relative, unsigned int lr)
{
    unsigned int pulse;
    unsigned int center;
    char buf[128];

    if (lr & 0x1) {
        if (relative) {
            pulse = servos->pulse(EAR_R_ROTATION_SERVO);
            pulse = (unsigned int) ((float) pulse +
                                    (EAR_R_ROTATION_ANGLE_MULT * deg));
            servos->setPulse(EAR_R_ROTATION_SERVO, pulse);
        } else {
            center =
                ((servos->hiRange(EAR_R_ROTATION_SERVO) -
                  servos->loRange(EAR_R_ROTATION_SERVO)) / 2) +
                EAR_R_ROTATION_LO_PULSE;
            pulse = (unsigned int)
                ((float) center + deg * EAR_R_ROTATION_ANGLE_MULT);
            servos->setPulse(EAR_R_ROTATION_SERVO, pulse);
        }

        snprintf(buf, sizeof(buf) - 1, "Right ear rotate to %.1f\n",
                 earRotationAt(0x1));
        LOG(buf);
    }

    if (lr & 0x2) {
        if (relative) {
            pulse = servos->pulse(EAR_L_ROTATION_SERVO);
            pulse = (unsigned int) ((float) pulse +
                                    (EAR_L_ROTATION_ANGLE_MULT * deg));
            servos->setPulse(EAR_L_ROTATION_SERVO, pulse);
        } else {
            center =
                ((servos->hiRange(EAR_L_ROTATION_SERVO) -
                  servos->loRange(EAR_L_ROTATION_SERVO)) / 2) +
                EAR_L_ROTATION_LO_PULSE;
            pulse = (unsigned int)
                ((float) center + deg * EAR_L_ROTATION_ANGLE_MULT);
            servos->setPulse(EAR_L_ROTATION_SERVO, pulse);
        }

        snprintf(buf, sizeof(buf) - 1, "Left ear rotate to %.1f\n",
                 earRotationAt(0x2));
        LOG(buf);
    }
}

void Head::earsUp(void)
{
    servos->clearMotionSchedule(EAR_R_TILT_SERVO);
    servos->clearMotionSchedule(EAR_L_TILT_SERVO);
    servos->clearMotionSchedule(EAR_R_ROTATION_SERVO);
    servos->clearMotionSchedule(EAR_L_ROTATION_SERVO);

    if (lidar && lidar->isEnabled()) {
        return;  // Keep the ears foldeed to not obstruct lidar
    }

    gettimeofday(&_last_earsup, NULL);
    earTilt(0.0);
    earRotate(0.0);
}

void Head::earsBack(void)
{
    servos->clearMotionSchedule(EAR_R_TILT_SERVO);
    servos->clearMotionSchedule(EAR_L_TILT_SERVO);
    servos->clearMotionSchedule(EAR_R_ROTATION_SERVO);
    servos->clearMotionSchedule(EAR_L_ROTATION_SERVO);

    earRotate(45.0, false, 0x1);
    earRotate(-45.0, false, 0x2);
    earTilt(45.0);
}

void Head::earsDown(void)
{
    servos->clearMotionSchedule(EAR_R_TILT_SERVO);
    servos->clearMotionSchedule(EAR_L_TILT_SERVO);
    servos->clearMotionSchedule(EAR_R_ROTATION_SERVO);
    servos->clearMotionSchedule(EAR_L_ROTATION_SERVO);

    earRotate(-32.5, false, 0x1);
    earRotate(32.5, false, 0x2);
    earTilt(-88.0);
}

void Head::earsFold(void)
{
    static const unsigned EAR_R_TILT_DEGREE = 0;
    static const unsigned EAR_L_TILT_DEGREE = 10;
    vector<struct servo_motion> motions;
    struct servo_motion motion;
    char buf[128];
    unsigned int center;

    servos->clearMotionSchedule(EAR_R_TILT_SERVO);
    servos->clearMotionSchedule(EAR_L_TILT_SERVO);
    servos->clearMotionSchedule(EAR_R_ROTATION_SERVO);
    servos->clearMotionSchedule(EAR_L_ROTATION_SERVO);

    earRotate(25.0, false, 0x1);
    earRotate(-30.0, false, 0x2);

    motions.clear();
    motions.push_back(motion);
    motion.pulse = servos->hiRange(EAR_R_TILT_SERVO) -
        (EAR_R_TILT_ANGLE_MULT * (EAR_R_TILT_DEGREE + 5));
    motion.ms = 300;
    motions.push_back(motion);
    servos->scheduleMotions(EAR_R_TILT_SERVO, motions);
    {
        center = ((servos->hiRange(EAR_R_TILT_SERVO) -
                   servos->loRange(EAR_R_TILT_SERVO)) / 2) +
            EAR_R_ROTATION_LO_PULSE;
        snprintf(buf, sizeof(buf) - 1, "Right ear tilt to %.1f\n",
                 ((float) motion.pulse - (float) center) /
                 EAR_R_ROTATION_ANGLE_MULT);
        LOG(buf);
    }

    motions.clear();
    motion.pulse = servos->loRange(EAR_L_TILT_SERVO) +
        (EAR_L_TILT_ANGLE_MULT * EAR_L_TILT_DEGREE);
    motion.ms = 400;
    motions.push_back(motion);
    servos->scheduleMotions(EAR_L_TILT_SERVO, motions);
    {
        center = ((servos->hiRange(EAR_L_TILT_SERVO) -
                   servos->loRange(EAR_L_TILT_SERVO)) / 2) +
            EAR_L_ROTATION_LO_PULSE;
        snprintf(buf, sizeof(buf) - 1, "Left ear tilt to %.1f\n",
                 ((float) center - (float) motion.pulse) /
                 EAR_L_ROTATION_ANGLE_MULT);
        LOG(buf);
    }
}

void Head::earsHalfDown(void)
{
    servos->clearMotionSchedule(EAR_R_TILT_SERVO);
    servos->clearMotionSchedule(EAR_L_TILT_SERVO);
    servos->clearMotionSchedule(EAR_R_ROTATION_SERVO);
    servos->clearMotionSchedule(EAR_L_ROTATION_SERVO);

    earRotate(-32.5, false, 0x1);
    earRotate(32.5, false, 0x2);
    earTilt(-45.0);
}

void Head::earsPointTo(float deg)
{
    (void)(deg);
}

float Head::earTiltAt(unsigned int lr) const
{
    unsigned int pulse;
    unsigned int center;

    if (lr & 0x1) {
        pulse = servos->pulse(EAR_R_TILT_SERVO);
        center = ((servos->hiRange(EAR_R_TILT_SERVO) -
                   servos->loRange(EAR_R_TILT_SERVO)) / 2) +
            EAR_R_TILT_LO_PULSE;
        return ((float) pulse - (float) center) / EAR_R_TILT_ANGLE_MULT;
    }

    pulse = servos->pulse(EAR_L_TILT_SERVO);
    center = ((servos->hiRange(EAR_L_TILT_SERVO) -
               servos->loRange(EAR_L_TILT_SERVO)) / 2) +
        EAR_L_TILT_LO_PULSE;

    return ((float) center - (float) pulse) / EAR_R_TILT_ANGLE_MULT;
}

float Head::earRotationAt(unsigned int lr) const
{
    unsigned int pulse;
    unsigned int center;

    if (lr & 0x1) {
        pulse = servos->pulse(EAR_R_ROTATION_SERVO);
        center = ((servos->hiRange(EAR_R_ROTATION_SERVO) -
                   servos->loRange(EAR_R_ROTATION_SERVO)) / 2) +
            EAR_R_ROTATION_LO_PULSE;
        return ((float) pulse - (float) center) / EAR_R_ROTATION_ANGLE_MULT;
    }

    pulse = servos->pulse(EAR_L_ROTATION_SERVO);
    center = ((servos->hiRange(EAR_L_ROTATION_SERVO) -
               servos->loRange(EAR_L_ROTATION_SERVO)) / 2) +
        EAR_L_ROTATION_LO_PULSE;

    return ((float) pulse - (float) center) / EAR_L_ROTATION_ANGLE_MULT;
}

void Head::updateEars(void)
{
    struct timeval now;

    gettimeofday(&now, NULL);
    now.tv_sec -= EAR_DROP_SECONDS;

    if (earTiltAt(0x1) == 0.0 && earTiltAt(0x2) == 0.0 &&
        voice != NULL) {
        static uint32_t last_doa = 0;
        uint32_t doa;
        bool speech_detected;
        float deg;

        doa = voice->getProp().DOAAngle;
        speech_detected = voice->getProp().SpeechDetected;
        if (speech_detected && (doa <= 90 || doa >= 270)) {
            // Filter out sounds from servo motors mounted behind
            if (doa != last_doa) {
                // Skip updating servo if doa is same as last sampling time
                last_doa = doa;
                deg = (float) doa;
                if (deg > 180.0) {
                    deg = deg - 360.0;
                }
                earRotate(deg);
            }
        }
    }

    if (earTiltAt(0x1) == 0.0 && earTiltAt(0x2) == 0.0 &&
        timercmp(&now, &_last_earsup, >=)) {
        earsHalfDown();
    }
}

void Head::teethUp(void)
{
    toothRotate(45.0);
}

void Head::teethDown(void)
{
    toothRotate(-45.0);
}

void Head::teethAnimate(bool en, bool sync)
{
    _teethAnimateEn = en ? true : false;
    _teethAnimateSync = sync ? true : false;

    if (en == false) {
        servos->clearMotionSchedule(TOOTH_R_ROTATION_SERVO);
        servos->clearMotionSchedule(TOOTH_L_ROTATION_SERVO);
        teethDown();
    }
}

void Head::teethRandomize(unsigned int probPct)
{
    if (probPct > 100) {
        probPct = 100;
    }
    _teethRandPct = probPct;
}

void Head::toothRotate(float deg, bool relative, unsigned int lr)
{
    unsigned int pulse;
    unsigned int center;
    char buf[128];

    if (lr & 0x1) {
        if (relative) {
            pulse = servos->pulse(TOOTH_R_ROTATION_SERVO);
            pulse = (unsigned int) ((float) pulse -
                                    (TOOTH_R_ROTATION_ANGLE_MULT * deg));
            servos->setPulse(TOOTH_R_ROTATION_SERVO, pulse);
        } else {
            center =
                ((servos->hiRange(TOOTH_R_ROTATION_SERVO) -
                  servos->loRange(TOOTH_R_ROTATION_SERVO)) / 2) +
                TOOTH_R_ROTATION_LO_PULSE;
            pulse = (unsigned int)
                ((float) center - deg * TOOTH_R_ROTATION_ANGLE_MULT);
            servos->setPulse(TOOTH_R_ROTATION_SERVO, pulse);
        }

        snprintf(buf, sizeof(buf) - 1, "Right tooth rotate to %.1f\n",
                 toothRotationAt(0x1));
        LOG(buf);
    }

    if (lr & 0x2) {
        if (relative) {
            pulse = servos->pulse(TOOTH_L_ROTATION_SERVO);
            pulse = (unsigned int) ((float) pulse +
                                    (TOOTH_L_ROTATION_ANGLE_MULT * deg));
            servos->setPulse(TOOTH_L_ROTATION_SERVO, pulse);
        } else {
            center =
                ((servos->hiRange(TOOTH_L_ROTATION_SERVO) -
                  servos->loRange(TOOTH_L_ROTATION_SERVO)) / 2) +
                TOOTH_L_ROTATION_LO_PULSE;
            pulse = (unsigned int)
                ((float) center + deg * TOOTH_L_ROTATION_ANGLE_MULT);
            servos->setPulse(TOOTH_L_ROTATION_SERVO, pulse);
        }

        snprintf(buf, sizeof(buf) - 1, "Left tooth rotate to %.1f\n",
                 toothRotationAt(0x2));
        LOG(buf);
    }
}

float Head::toothRotationAt(unsigned int lr) const
{
    unsigned int pulse;
    unsigned int center;

    if (lr & 0x1) {
        pulse = servos->pulse(TOOTH_R_ROTATION_SERVO);
        center = ((servos->hiRange(TOOTH_R_ROTATION_SERVO) -
                   servos->loRange(TOOTH_R_ROTATION_SERVO)) / 2) +
            TOOTH_R_ROTATION_LO_PULSE;
        return ((float) center - (float) pulse) / TOOTH_R_ROTATION_ANGLE_MULT;
    }

    pulse = servos->pulse(TOOTH_L_ROTATION_SERVO);
    center = ((servos->hiRange(TOOTH_L_ROTATION_SERVO) -
               servos->loRange(TOOTH_L_ROTATION_SERVO)) / 2) +
        TOOTH_L_ROTATION_LO_PULSE;

    return ((float) pulse - (float) center) / TOOTH_L_ROTATION_ANGLE_MULT;
}

void Head::updateTeeth(void)
{
    vector<struct servo_motion> motions;
    struct servo_motion motion;

    if (_teethAnimateEn) {
        if (servos->hasMotionSchedule(TOOTH_R_ROTATION_SERVO) == false) {
            if (_teethAnimateSync) {
                servos->clearMotionSchedule(TOOTH_L_ROTATION_SERVO);

                motions.clear();
                motion.pulse = TOOTH_R_ROTATION_HI_PULSE;
                motion.ms = 250;
                motions.push_back(motion);
                motion.pulse = TOOTH_R_ROTATION_LO_PULSE;
                motion.ms = 250;
                motions.push_back(motion);
                servos->scheduleMotions(TOOTH_R_ROTATION_SERVO, motions);

                motions.clear();
                motion.pulse = TOOTH_L_ROTATION_LO_PULSE;
                motion.ms = 250;
                motions.push_back(motion);
                motion.pulse = TOOTH_L_ROTATION_HI_PULSE;
                motion.ms = 250;
                motions.push_back(motion);
                servos->scheduleMotions(TOOTH_L_ROTATION_SERVO, motions);
            } else {
                servos->clearMotionSchedule(TOOTH_L_ROTATION_SERVO);

                motions.clear();
                motion.pulse = TOOTH_R_ROTATION_HI_PULSE;
                motion.ms = 250;
                motions.push_back(motion);
                motion.pulse = TOOTH_R_ROTATION_LO_PULSE;
                motion.ms = 250;
                motions.push_back(motion);
                servos->scheduleMotions(TOOTH_R_ROTATION_SERVO, motions);

                motions.clear();
                motion.pulse = TOOTH_L_ROTATION_HI_PULSE;
                motion.ms = 250;
                motions.push_back(motion);
                motion.pulse = TOOTH_L_ROTATION_LO_PULSE;
                motion.ms = 250;
                motions.push_back(motion);
                servos->scheduleMotions(TOOTH_L_ROTATION_SERVO, motions);
            }
        }
    }

    if ((_teethRandPct) > 0 &&
        (servos->hasMotionSchedule(TOOTH_R_ROTATION_SERVO) == false) &&
        (servos->hasMotionSchedule(TOOTH_L_ROTATION_SERVO) == false) &&
        ((((unsigned int) rand()) % 1000) <= _teethRandPct)) {
        unsigned int id;
        vector<struct servo_motion> motions;
        struct servo_motion motion;
        unsigned int pr;

        pr = random_servo_pos_unadjusted(TOOTH_R_ROTATION_SERVO);

        id = TOOTH_R_ROTATION_SERVO;
        motions.clear();
        motion.pulse = servos->loRange(id) + pr;
        motion.ms = 200;
        motions.push_back(motion);
        motion.pulse = servos->pulse(id);
        motion.ms = 200;
        motions.push_back(motion);
        servos->scheduleMotions(id, motions);

        id = TOOTH_L_ROTATION_SERVO;
        motions.clear();
        motion.pulse = servos->hiRange(id) - pr;
        motion.ms = 200;
        motions.push_back(motion);
        motion.pulse = servos->pulse(id);
        motion.ms = 200;
        motions.push_back(motion);
        servos->scheduleMotions(id, motions);
    }
}

void Head::whiskersUp(void)
{
    whiskerRotate(45.0);
}

void Head::whiskersDown(void)
{
    whiskerRotate(-45.0);
}

void Head::whiskersCenter(void)
{
    whiskerRotate(0.0);
}

void Head::whiskersAnimate(bool en, bool sync)
{
    _whiskersAnimateEn = en ? true : false;
    _whiskersAnimateSync = sync ? true : false;

    if (en == false) {
        servos->clearMotionSchedule(WHISKER_R_ROTATION_SERVO);
        servos->clearMotionSchedule(WHISKER_L_ROTATION_SERVO);
        whiskersCenter();
    }
}

void Head::whiskersRandomize(unsigned int probPct)
{
    if (probPct > 100) {
        probPct = 100;
    }
    _whiskersRandPct = probPct;
}

void Head::whiskerRotate(float deg, bool relative, unsigned int lr)
{
    unsigned int pulse;
    unsigned int center;
    char buf[128];

    if (lr & 0x1) {
        if (relative) {
            pulse = servos->pulse(WHISKER_R_ROTATION_SERVO);
            pulse = (unsigned int) ((float) pulse +
                                    (WHISKER_R_ROTATION_ANGLE_MULT * deg));
            servos->setPulse(WHISKER_R_ROTATION_SERVO, pulse);
        } else {
            center =
                ((servos->hiRange(WHISKER_R_ROTATION_SERVO) -
                  servos->loRange(WHISKER_R_ROTATION_SERVO)) / 2) +
                WHISKER_R_ROTATION_LO_PULSE;
            pulse = (unsigned int)
                ((float) center + deg * WHISKER_R_ROTATION_ANGLE_MULT);
            servos->setPulse(WHISKER_R_ROTATION_SERVO, pulse);
        }

        snprintf(buf, sizeof(buf) - 1, "Right whisker rotate to %.1f\n",
                 whiskerRotationAt(0x1));
        LOG(buf);
    }

    if (lr & 0x2) {
        if (relative) {
            pulse = servos->pulse(WHISKER_L_ROTATION_SERVO);
            pulse = (unsigned int) ((float) pulse -
                                    (WHISKER_L_ROTATION_ANGLE_MULT * deg));
            servos->setPulse(WHISKER_L_ROTATION_SERVO, pulse);
        } else {
            center =
                ((servos->hiRange(WHISKER_L_ROTATION_SERVO) -
                  servos->loRange(WHISKER_L_ROTATION_SERVO)) / 2) +
                WHISKER_L_ROTATION_LO_PULSE;
            pulse = (unsigned int)
                ((float) center - deg * WHISKER_L_ROTATION_ANGLE_MULT);
            servos->setPulse(WHISKER_L_ROTATION_SERVO, pulse);
        }

        snprintf(buf, sizeof(buf) - 1, "Left whisker rotate to %.1f\n",
                 whiskerRotationAt(0x2));
        LOG(buf);
    }
}

float Head::whiskerRotationAt(unsigned int lr) const
{
    unsigned int pulse;
    unsigned int center;

    if (lr & 0x1) {
        pulse = servos->pulse(WHISKER_R_ROTATION_SERVO);
        center = ((servos->hiRange(WHISKER_R_ROTATION_SERVO) -
                   servos->loRange(WHISKER_R_ROTATION_SERVO)) / 2) +
            WHISKER_R_ROTATION_LO_PULSE;
        return ((float) pulse - (float) center) / WHISKER_R_ROTATION_ANGLE_MULT;
    }

    pulse = servos->pulse(WHISKER_L_ROTATION_SERVO);
    center = ((servos->hiRange(WHISKER_L_ROTATION_SERVO) -
               servos->loRange(WHISKER_L_ROTATION_SERVO)) / 2) +
        WHISKER_L_ROTATION_LO_PULSE;

    return ((float) center - (float) pulse) / WHISKER_L_ROTATION_ANGLE_MULT;
}

void Head::updateWhiskers(void)
{
    vector<struct servo_motion> motions;
    struct servo_motion motion;

    if (_teethAnimateEn) {
        if (servos->hasMotionSchedule(WHISKER_R_ROTATION_SERVO) == false) {
            if (_teethAnimateSync) {
                servos->clearMotionSchedule(WHISKER_L_ROTATION_SERVO);

                motions.clear();
                motion.pulse = WHISKER_R_ROTATION_HI_PULSE;
                motion.ms = 150;
                motions.push_back(motion);
                motion.pulse = WHISKER_R_ROTATION_LO_PULSE;
                motion.ms = 150;
                motions.push_back(motion);
                servos->scheduleMotions(WHISKER_R_ROTATION_SERVO, motions);

                motions.clear();
                motion.pulse = WHISKER_L_ROTATION_LO_PULSE;
                motion.ms = 150;
                motions.push_back(motion);
                motion.pulse = WHISKER_L_ROTATION_HI_PULSE;
                motion.ms = 150;
                motions.push_back(motion);
                servos->scheduleMotions(WHISKER_L_ROTATION_SERVO, motions);
            } else {
                servos->clearMotionSchedule(WHISKER_L_ROTATION_SERVO);

                motions.clear();
                motion.pulse = WHISKER_R_ROTATION_HI_PULSE;
                motion.ms = 150;
                motions.push_back(motion);
                motion.pulse = WHISKER_R_ROTATION_LO_PULSE;
                motion.ms = 150;
                motions.push_back(motion);
                servos->scheduleMotions(WHISKER_R_ROTATION_SERVO, motions);

                motions.clear();
                motion.pulse = WHISKER_L_ROTATION_HI_PULSE;
                motion.ms = 150;
                motions.push_back(motion);
                motion.pulse = WHISKER_L_ROTATION_LO_PULSE;
                motion.ms = 150;
                motions.push_back(motion);
                servos->scheduleMotions(WHISKER_L_ROTATION_SERVO, motions);
            }
        }
    }

    if ((_whiskersRandPct) > 0 &&
        (servos->hasMotionSchedule(WHISKER_R_ROTATION_SERVO) == false) &&
        (servos->hasMotionSchedule(WHISKER_L_ROTATION_SERVO) == false) &&
        ((((unsigned int) rand()) % 1000) <= _whiskersRandPct)) {
        unsigned int id;
        vector<struct servo_motion> motions;
        struct servo_motion motion;
        unsigned int pr;

        pr = random_servo_pos_unadjusted(WHISKER_R_ROTATION_SERVO);

        id = WHISKER_R_ROTATION_SERVO;
        motions.clear();
        motion.pulse = servos->loRange(id) + pr;
        motion.ms = 200;
        motions.push_back(motion);
        motion.pulse = servos->pulse(id);
        motion.ms = 200;
        motions.push_back(motion);
        servos->scheduleMotions(id, motions);

        id = WHISKER_L_ROTATION_SERVO;
        motions.clear();
        motion.pulse = servos->hiRange(id) - pr;
        motion.ms = 200;
        motions.push_back(motion);
        motion.pulse = servos->pulse(id);
        motion.ms = 200;
        motions.push_back(motion);
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
