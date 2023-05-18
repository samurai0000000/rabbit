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

    bool isDeviceOnline(void) const;
    float x(void);
    float y(void);
    float z(void);
    float heading(void);

private:

    void probeOpenDevice(void);
    static void *thread_func(void *);
    void run(void);

    int _handle;

    MedianFilter<float> _histX;
    MedianFilter<float> _histY;
    MedianFilter<float> _histZ;
    int16_t _minX, _maxX, _offsetX, _avgDeltaX;
    int16_t _minY, _maxY, _offsetY, _avgDeltaY;
    int16_t _minZ, _maxZ, _offsetZ, _avgDeltaZ;
    int16_t _avgDelta;
    float _scaleX, _scaleY, _scaleZ;

    bool _running;
    pthread_t _thread;
    pthread_mutex_t _mutex;
    pthread_cond_t _cond;

};

inline bool Compass::isDeviceOnline(void) const
{
    return _handle != -1;
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
