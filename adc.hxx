/*
 * adc.hxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#ifndef ADC_HXX
#define ADC_HXX

class ADC {

public:

    ADC();
    ~ADC();

    float v(unsigned int chan);

private:

    int readReg(uint8_t reg, uint16_t *val) const;
    int writeReg(uint8_t reg, uint16_t val) const;

    int _handle;
    uint16_t _config;
    pthread_mutex_t _mutex;

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
