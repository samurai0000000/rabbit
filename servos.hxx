/*
 * servos.hxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#ifndef SERVOS_HXX
#define SERVOS_HXX

class Servos {

public:

    Servos();
    ~Servos();

    unsigned int pos(unsigned int index) const;
    unsigned int posPct(unsigned int index) const;
    void setPos(unsigned int index, unsigned int pos);
    void setPosPct(unsigned int index, unsigned int pct);

private:

    unsigned int *_pos;
    int _handle;

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
