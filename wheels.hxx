/*
 * wheels.hxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#ifndef WHEELS_HXX
#define WHEELS_HXX

#include <sys/time.h>

class Wheels {

public:

    Wheels();
    ~Wheels();

    void halt(void);
    void fwd(unsigned int ms);
    void bwd(unsigned int ms);
    void ror(unsigned int ms);
    void rol(unsigned int ms);

    unsigned int state(void) const;
    const char *stateStr(void) const;

    unsigned int fwd_ms(void) const;
    unsigned int bwd_ms(void) const;
    unsigned int ror_ms(void) const;
    unsigned int rol_ms(void) const;

private:

    void change(unsigned int state);

    unsigned int _state;
    struct timeval _ts;
    unsigned int _fwd_ms;
    unsigned int _bwd_ms;
    unsigned int _ror_ms;
    unsigned int _rol_ms;

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
