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

    float voltage(void) const;
    float current(void) const;

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
