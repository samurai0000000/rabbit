/*
 * amvience.hxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#ifndef AMBIENCE_HXX
#define AMBIENCE_HXX

class Ambience {

public:

    Ambience();
    ~Ambience();

    float cpuTemp(void) const;
    float gpuTemp(void) const;
    unsigned int temp(void) const;
    unsigned int pressure(void) const;
    unsigned int humidity(void) const;

private:

    int _bme280;
    int _sht3x;

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
