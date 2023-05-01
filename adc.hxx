/*
 * adc.hxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#ifndef ADC_HXX
#define ADC_HXX

#include "medianfilter.hxx"

#define ADC_CHANNELS 4

class ADC {

public:

    ADC();
    ~ADC();

    float v(unsigned int chan) const;

private:

    int readReg(uint8_t reg, uint16_t *val) const;
    int writeReg(uint8_t reg, uint16_t val) const;

    static void *thread_func(void *args);
    void run(void);
    void convert(unsigned int chan);

    int _handle;
    uint16_t _config;
    bool _running;
    pthread_t _thread;
    pthread_mutex_t _mutex;
    pthread_cond_t _cond;
    float _v[ADC_CHANNELS];
    MedianFilter<float> *_hist[ADC_CHANNELS];

};

inline float ADC::v(unsigned int chan) const
{
    if (chan >= ADC_CHANNELS) {
        return 0.0;
    }

    return _v[chan];
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
