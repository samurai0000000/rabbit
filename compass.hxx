/*
 * compass.hxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#ifndef COMPASS_HXX
#define COMPASS_HXX

#include "medianfilter.hxx"

class Compass {

public:

    Compass();
    ~Compass();

    float x(void);
    float y(void);
    float z(void);
    float bearing(void);

private:

    static void *thread_func(void *);
    void run(void);

    int _handle;

    MedianFilter<int16_t> _histX;
    MedianFilter<int16_t> _histY;
    MedianFilter<int16_t> _histZ;

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
