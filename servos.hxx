/*
 * servos.hxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#ifndef SERVOS_HXX
#define SERVOS_HXX

#include <vector>

#define SERVO_CHANNELS 16
#define SERVO_SCHEDULE_INTERVAL_MS 50

using namespace std;

struct servo_motion {
    unsigned int pulse;
    unsigned int ms;
};

struct servo_motion_exec {
    struct servo_motion motion;
    struct timeval t_start;
    unsigned int intervals;
    unsigned int elapsed_intervals;
    float f_steps_per_interval;
    float f_current_pulse;
};

struct servo_motion_sync {
    uint32_t bitmap;
    pthread_cond_t cond;
    pthread_mutex_t mutex;
};

class Servos {

public:

    Servos();
    ~Servos();

public:

    void setRange(unsigned int chan, unsigned int lo, unsigned int hi);
    unsigned int loRange(unsigned int chan);
    unsigned int hiRange(unsigned int chan);
    unsigned int pulse(unsigned int chan) const;
    void setPulse(unsigned int chan, unsigned int pulse,
                  bool ignoreRange = false, bool clearMotions = true);
    void setPct(unsigned int chan, unsigned int pct);
    void center(unsigned int chan);
    void scheduleMotions(unsigned int chan,
                         const vector<struct servo_motion> &motions,
                         bool append = false);
    void clearMotionSchedule(unsigned int chan);
    bool hasMotionSchedule(unsigned int chan) const;
    void syncMotionSchedule(uint32_t chan_mask);
    bool lastMotionPulseInPlan(unsigned int chan, unsigned int *pulse) const;

private:

    int readReg(uint8_t reg, uint8_t *val) const;
    int writeReg(uint8_t reg, uint8_t val) const;
    void setPwm(unsigned int chan, unsigned int on, unsigned int off);

    static void *thread_func(void *);
    void run(void);

    int _handle;
    unsigned int _freq;
    unsigned int _lo[SERVO_CHANNELS];
    unsigned int _hi[SERVO_CHANNELS];
    unsigned int _pulse[SERVO_CHANNELS];

    bool _running;
    pthread_t _thread;
    pthread_mutex_t _mutex;
    pthread_cond_t _cond;

    vector<struct servo_motion_exec> _motions[SERVO_CHANNELS];
    vector<struct servo_motion_sync *> _syncs;
};

inline unsigned int Servos::loRange(unsigned int chan)
{
    if (chan >= SERVO_CHANNELS) {
        return 0;
    }

    return _lo[chan];
}

inline unsigned int Servos::hiRange(unsigned int chan)
{
    if (chan >= SERVO_CHANNELS) {
        return 0;
    }

    return _hi[chan];
}

inline unsigned int Servos::pulse(unsigned int chan) const
{
    if (chan >= SERVO_CHANNELS) {
        return 0;
    }

    return _pulse[chan];
}

inline bool Servos::hasMotionSchedule(unsigned int chan) const
{
    if (_motions[chan].empty()) {
        return false;
    }

    return true;
}

#endif

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
