/*
 * proximity.hxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#ifndef PROXIMITY_HXX
#define PROXIMITY_HXX

#define RABBIT_MCUS          2
#define IR_DEVICES           8
#define ULTRASOUND_DEVICES  12

class Proximity {

public:

    Proximity();
    ~Proximity();

    bool isDeviceOnline(void) const;

    bool isEnabled(void) const;
    void enable(bool en);
    bool ir_state(unsigned int id) const;
    unsigned int ultrasound_d_mm(unsigned int id) const;

private:

    void probeOpenDevice(unsigned int id);
    void errCloseDevice(unsigned int id);
    static void *thread_func(void *);
    void run(void);
    void stream(void);
    void stop(void);

    bool _enabled;
    int _handle[RABBIT_MCUS];
    std::string _node[RABBIT_MCUS];
    float _temp_c[RABBIT_MCUS];
    bool _ir[IR_DEVICES];
    unsigned int _us[ULTRASOUND_DEVICES];

    bool _running;
    pthread_t _thread;
    pthread_mutex_t _mutex;
    pthread_cond_t _cond;

};

inline bool Proximity::isDeviceOnline(void) const
{
    unsigned int i;

    for (i = 0; i < RABBIT_MCUS; i++) {
        if (_handle[i] == -1) {
            return false;
        }
    }

    return true;
}

inline bool Proximity::isEnabled(void) const
{
    return _enabled;
}

inline bool Proximity::ir_state(unsigned int id) const
{
    if (id >= IR_DEVICES) {
        return false;
    }

    return _ir[id];
}

inline unsigned int Proximity::ultrasound_d_mm(unsigned int id) const
{
    if (id >= ULTRASOUND_DEVICES) {
        return 0;
    }

    return _us[id];
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
