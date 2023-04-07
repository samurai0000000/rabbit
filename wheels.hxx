/*
 * wheels.hxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#ifndef WHEELS_HXX
#define WHEELS_HXX

class Wheels {

public:

    Wheels();
    ~Wheels();

    void halt(void);
    void fwd(unsigned int steps);
    void bwd(unsigned int steps);
    void ror(unsigned int degrees);
    void rol(unsigned int degrees);

    unsigned int fwd_steps(void) const;
    unsigned int bwd_steps(void) const;
    unsigned int ror_degrees(void) const;
    unsigned int rol_degrees(void) const;

private:

    unsigned int _fwd_steps;
    unsigned int _bwd_steps;
    unsigned int _ror_degrees;
    unsigned int _rol_degrees;

};

inline Wheels::Wheels()
    : _fwd_steps(0),
      _bwd_steps(0),
      _ror_degrees(0),
      _rol_degrees(0)
{

}

inline Wheels::~Wheels()
{
    halt();
}

inline unsigned int Wheels::fwd_steps(void) const
{
    return _fwd_steps;
}

inline unsigned int Wheels::bwd_steps(void) const
{
    return _bwd_steps;
}

inline unsigned int Wheels::ror_degrees(void) const
{
    return _ror_degrees;
}

inline unsigned int Wheels::rol_degrees(void) const
{
    return _rol_degrees;
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
