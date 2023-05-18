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

    bool isDeviceOnline(void) const;
    float v(unsigned int chan) const;

private:

    void probeOpenDevice(void);
    int readReg(uint8_t reg, uint16_t *val);
    int writeReg(uint8_t reg, uint16_t val);

    static void *thread_func(void *args);
    void run(void);
    void convert(unsigned int chan);

    int _handle;
    uint16_t _config;

    bool _running;
    pthread_t _thread;
    pthread_mutex_t _mutex;
    pthread_cond_t _cond;
    MedianFilter<float> *_hist[ADC_CHANNELS];

};

inline bool ADC::isDeviceOnline(void) const
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
