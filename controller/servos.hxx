/*
 * servos.hxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#ifndef SERVOS_HXX
#define SERVOS_HXX

#include <vector>

#define SERVO_CONTROLLERS            2
#define SERVO_CHANNELS              (16 * SERVO_CONTROLLERS)
#define SERVO_SCHEDULE_INTERVAL_MS  50

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

    bool isDeviceOnline(void) const;

    void setRange(unsigned int chan, unsigned int lo, unsigned int hi);
    unsigned int loRange(unsigned int chan);
    unsigned int hiRange(unsigned int chan);
    unsigned int pulse(unsigned int chan) const;
    void setPulse(unsigned int chan, unsigned int pulse,
                  bool ignoreRange = false, bool clearMotions = true);
    void setPct(unsigned int chan, unsigned int pct);
    void center(unsigned int chan);
    void scheduleMotions(unsigned int chan,
                         const std::vector<struct servo_motion> &motions,
                         bool append = false);
    void clearMotionSchedule(unsigned int chan);
    bool hasMotionSchedule(unsigned int chan) const;
    void syncMotionSchedule(uint32_t chan_mask);
    bool lastMotionPulseInPlan(unsigned int chan, unsigned int *pulse) const;

private:

    void probeOpenDevice(void);
    int readReg(unsigned int id, uint8_t reg, uint8_t *val);
    int writeReg(unsigned int id, uint8_t reg, uint8_t val);
    void setPwm(unsigned int chan, unsigned int on, unsigned int off);

    static void *thread_func(void *);
    void run(void);

    int _handle[SERVO_CONTROLLERS];
    unsigned int _freq;
    unsigned int _lo[SERVO_CHANNELS * SERVO_CONTROLLERS];
    unsigned int _hi[SERVO_CHANNELS * SERVO_CONTROLLERS];
    unsigned int _pulse[SERVO_CHANNELS * SERVO_CONTROLLERS];

    bool _running;
    pthread_t _thread;
    pthread_mutex_t _mutex;
    pthread_cond_t _cond;

    std::vector<struct servo_motion_exec>
    _motions[SERVO_CHANNELS * SERVO_CONTROLLERS];
    std::vector<struct servo_motion_sync *> _syncs;
};

inline bool Servos::isDeviceOnline(void) const
{
    unsigned int i;

    for (i = 0; i < SERVO_CONTROLLERS; i++) {
        if (_handle[i] == -1) {
            return false;
        }
    }

    return true;
}

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
