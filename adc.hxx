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

    unsigned int channels(void) const;
    unsigned int min(void) const;
    unsigned int max(void) const;

    unsigned int val(unsigned int chan) const;

private:

    int *_devs;

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
