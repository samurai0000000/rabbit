/*
 * arms.cxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include "rabbit.hxx"

Arm::Arm(unsigned int side)
{
    if (side == RIGHT_ARM || side == LEFT_ARM) {
        _side = side;
        rest();
    }
}

Arm::~Arm()
{
    rest();
    sleep(3);
}

void Arm::rest(void)
{
    if (_side == RIGHT_ARM) {
        servos->setPct(0, 50);
        servos->setPct(1, 5);
        servos->setPct(2, 5);
        servos->setPct(3, 30);
        servos->setPct(4, 0);
        servos->setPct(5, 0);
    } else {
        servos->setPct(6, 50);
        servos->setPct(7, 95);
        servos->setPct(8, 95);
        servos->setPct(9, 70);
        servos->setPct(10, 100);
        servos->setPct(11, 0);
    }
}

void Arm::surrender(void)
{
    if (_side == RIGHT_ARM) {
        servos->setPct(0, 50);
        servos->setPct(1, 95);
        servos->setPct(2, 75);
        servos->setPct(3, 50);
        servos->setPct(4, 50);
        servos->setPct(5, 0);
    } else {
        servos->setPct(6, 50);
        servos->setPct(7, 5);
        servos->setPct(8, 25);
        servos->setPct(9, 50);
        servos->setPct(10, 50);
        servos->setPct(11, 0);
    }

    sleep(2);

    if (_side == RIGHT_ARM) {
        servos->setPct(0, 100);
        servos->setPct(1, 95);
        servos->setPct(2, 75);
        servos->setPct(3, 50);
        servos->setPct(4, 50);
        servos->setPct(5, 0);
    } else {
        servos->setPct(6, 0);
        servos->setPct(7, 5);
        servos->setPct(8, 25);
        servos->setPct(9, 50);
        servos->setPct(10, 50);
        servos->setPct(11, 0);
    }
}

void Arm::hug(void)
{
    if (_side == RIGHT_ARM) {
        servos->setPct(0, 50);
        servos->setPct(1, 95);
        servos->setPct(2, 75);
        servos->setPct(3, 50);
        servos->setPct(4, 50);
        servos->setPct(5, 0);
    } else {
        servos->setPct(6, 50);
        servos->setPct(7, 5);
        servos->setPct(8, 25);
        servos->setPct(9, 50);
        servos->setPct(10, 50);
        servos->setPct(11, 0);
    }
}

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */

