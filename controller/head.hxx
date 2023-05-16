/*
 * head.hxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#ifndef HEAD_HXX
#define HEAD_HXX

class Head {

public:

    Head();
    ~Head();

    void rotate(float deg, bool relative = false);
    void tilt(float deg, bool relative = false);
    float rotationAt(void) const;
    float tiltAt(void) const;

    void enSentry(bool enable);
    bool isSentryEn(void) const;

private:

    static void *thread_func(void *args);
    void run(void);

    float _rotation;
    float _tilt;
    bool _sentry;

    pthread_t _thread;
    pthread_mutex_t _mutex;
    pthread_cond_t _cond;
    bool _running;

};

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
