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

    float x(void) const;
    float y(void) const;
    float z(void) const;
    float bearing(void) const;

private:

    static void *thread_func(void *);
    void run(void);

    int _handle;

    float _x;
    float _y;
    float _z;
    float _bearing;

    MedianFilter<float> _histX;
    MedianFilter<float> _histY;
    MedianFilter<float> _histZ;

    pthread_t _thread;
    pthread_mutex_t _mutex;
    pthread_cond_t _cond;
    bool _running;
};

inline float Compass::x(void) const
{
    return _x;
}

inline float Compass::y(void) const
{
    return _y;
}

inline float Compass::z(void) const
{
    return _z;
}

inline float Compass::bearing(void) const
{
    return _bearing;
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
