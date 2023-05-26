/*
 * voice.hxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#ifndef VOICE_HXX
#define VOICE_HXX

class Voice {

public:

    Voice();
    ~Voice();

    void enable(bool en);
    bool isEnabled(void) const;
    bool isOnline(void) const;

private:

    void probeOpenDevice(void);
    static void *thread_func(void *args);
    void run(void);

    void *_handle;
    unsigned int _rate;
    bool _enable;

    bool _running;
    pthread_t _thread;
    pthread_mutex_t _mutex;
    pthread_cond_t _cond;

};

inline bool Voice::isEnabled(void) const
{
    return _enable;
}

inline bool Voice::isOnline(void) const
{
    return _handle != NULL;
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
