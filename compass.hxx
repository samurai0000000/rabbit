/*
 * compass.hxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#ifndef COMPASS_HXX
#define COMPASS_HXX

class Compass {

public:

    Compass();
    ~Compass();

    unsigned int x(void) const;
    unsigned int y(void) const;
    unsigned int z(void) const;
    unsigned int azimuth(void) const;

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
