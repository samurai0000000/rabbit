/*
 * power.hxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#ifndef POWER_HXX
#define POWER_HXX

class Power {

public:

    Power();
    ~Power();

    unsigned int voltage(void) const;
    unsigned int current(void) const;

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
