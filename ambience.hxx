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

    unsigned int temp(void) const;
    unsigned int pressure(void) const;
    unsigned int humidity(void) const;

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
