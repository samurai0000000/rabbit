/*
 * arms.hxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#ifndef ARMS_HXX
#define ARMS_HXX

#define RIGHT_ARM  0
#define LEFT_ARM   1

class Arm {

public:

    Arm(unsigned int side);
    ~Arm();

    void rest(void);
    void surrender(void);
    void hug(void);

private:

    unsigned int _side;

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
