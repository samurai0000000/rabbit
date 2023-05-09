/*
 * amvience.hxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#ifndef AMBIENCE_HXX
#define AMBIENCE_HXX

#include "bme280.h"
#include "medianfilter.hxx"

class Ambience {

public:

    Ambience();
    ~Ambience();

    float socTemp(void) const;
    float temp(void) const;
    float pressure(void) const;
    float humidity(void) const;

private:

    static void *thread_func(void *args);
    void run(void);

    struct bme280_dev _dev;
    struct bme280_settings _settings;
    uint32_t _bme280_delay_us;
    int _bme280;

    float _socTemp;
    float _temp;
    float _pressure;
    float _humidity;

    MedianFilter<float> _histTemp;
    MedianFilter<float> _histPressure;
    MedianFilter<float> _histHumidity;

    pthread_t _thread;
    pthread_mutex_t _mutex;
    pthread_cond_t _cond;
    bool _running;
};

inline float Ambience::socTemp(void) const
{
    return _socTemp;
}

inline float Ambience::temp(void) const
{
    return _temp;
}

inline float Ambience::pressure(void) const
{
    return _pressure;
}

inline float Ambience::humidity(void) const
{
    return _humidity;
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
