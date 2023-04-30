/*
 * arms.cxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include "rabbit.hxx"

Arm::Arm(unsigned int side)
{
    if (side != RIGHT_ARM && side != LEFT_ARM) {
        assert(0);
    }

    _side = side;
    if (side == RIGHT_ARM) {
        servos->setRange( 0, 530, 2580);
        servos->setRange( 1, 450, 2500);
        servos->setRange( 2, 450, 2500);
        servos->setRange( 3, 510, 2560);
        servos->setRange( 4, 530, 2580);
        servos->setRange( 5, 450, 2500);
    } else {
        servos->setRange( 6, 430, 2490);
        servos->setRange( 7, 450, 2500);
        servos->setRange( 8, 450, 2500);
        servos->setRange( 9, 450, 2500);
        servos->setRange(10, 430, 2480);
        servos->setRange(11, 450, 2500);
    }

    updateTrims();
    rest();
}

Arm::~Arm()
{
    rest();
    usleep(1000000);
}

void Arm::updateTrims(void)
{
    unsigned int i;
    unsigned int servoId;

    for (i = 0; i < 6; i++) {
        if (_side == RIGHT_ARM) {
            servoId = i;
        } else {
            servoId = i + 6;
        }

        _loRange[i] = servos->loRange(servoId);
        _hiRange[i] = servos->hiRange(servoId);
        _range[i] = _hiRange[i] - _loRange[i];
        _center[i] = _loRange[i] + (_range[i] / 2);
        if (i != 5) {
            _ppd[i] = _range[i] / 180.0;
        } else {
            _ppd[i] = _range[i] / 50.0;
        }
    }
}

unsigned int Arm::pulse(unsigned int index) const
{
    unsigned int servoId;

    if (_side == RIGHT_ARM) {
        servoId = index;
    } else {
        servoId = index + 6;
    }

    return servos->pulse(servoId);
}

void Arm::setPulse(unsigned int index, unsigned int pulse)
{
    unsigned int servoId;

    if (_side == RIGHT_ARM) {
        servoId = index;
    } else {
        servoId = index + 6;
    }

    return servos->setPulse(servoId, pulse);
}

float Arm::shoulderRotation(void) const
{
    float degree;
    unsigned int index = 0;

    if (_side == RIGHT_ARM) {
        degree = (float) ((int) pulse(index) - (int) _center[index]);
    } else {
        degree = (float) ((int) _center[index] - (int) pulse(index));
    }

    degree *= 180.0;
    degree /= (float) _range[index];

    return degree;
}

float Arm::shoulderExtension(void) const
{
    float degree;
    unsigned int index = 1;

    if (_side == RIGHT_ARM) {
        degree = (float) ((int) pulse(index) - (int) _center[index]);
    } else {
        degree = (float) ((int) _center[index] - (int) pulse(index));
    }

    degree *= 180.0;
    degree /= (float) _range[index];

    return degree;
}

float Arm::elbowExtension(void) const
{
    float degree;
    unsigned int index = 2;

    if (_side == RIGHT_ARM) {
        degree = (float) ((int) pulse(index) - (int) _center[index]);
    } else {
        degree = (float) ((int) _center[index] - (int) pulse(index));
    }

    degree *= 180.0;
    degree /= (float) _range[index];

    return degree;
}

float Arm::wristExtension(void) const
{
    float degree;
    unsigned int index = 3;

    if (_side == RIGHT_ARM) {
        degree = (float) ((int) pulse(index) - (int) _center[index]);
    } else {
        degree = (float) ((int) _center[index] - (int) pulse(index));
    }

    degree *= 180.0;
    degree /= (float) _range[index];

    return degree;
}

float Arm::wristRotation(void) const
{
    float degree;
    unsigned int index = 4;

    if (_side == RIGHT_ARM) {
        degree = (float) ((int) pulse(index) - (int) _center[index]);
    } else {
        degree = (float) ((int) _center[index] - (int) pulse(index));
    }

    degree *= 180.0;
    degree /= (float) _range[index];

    return degree;
}

float Arm::gripperPosition(void) const
{
    float pos;
    unsigned index = 5;

    pos = (float) ((pulse(index) - _loRange[index]) / _ppd[index]);

    return pos;
}

void Arm::rotateShoulder(float deg, bool relative)
{
    unsigned int newPulse;
    int offset;
    unsigned index = 0;

    if (relative) {
        deg = shoulderRotation() + deg;
    }

    offset = (int) roundf(_ppd[index] * deg);
    if (_side == RIGHT_ARM) {
        newPulse = (unsigned int) (((int) _center[index]) + offset);
    } else {
        newPulse = (unsigned int) (((int) _center[index]) - offset);
    }

    setPulse(index, newPulse);
}

void Arm::extendShoulder(float deg, bool relative)
{
    unsigned int newPulse;
    int offset;
    unsigned index = 1;

    if (relative) {
        deg = shoulderExtension() + deg;
    }

    offset = (int) roundf(_ppd[index] * deg);
    if (_side == RIGHT_ARM) {
        newPulse = (unsigned int) (((int) _center[index]) + offset);
    } else {
        newPulse = (unsigned int) (((int) _center[index]) - offset);
    }

    setPulse(index, newPulse);
}

void Arm::extendElbow(float deg, bool relative)
{
    unsigned int newPulse;
    int offset;
    unsigned index = 2;

    if (relative) {
        deg = elbowExtension() + deg;
    }

    offset = (int) roundf(_ppd[index] * deg);
    if (_side == RIGHT_ARM) {
        newPulse = (unsigned int) (((int) _center[index]) + offset);
    } else {
        newPulse = (unsigned int) (((int) _center[index]) - offset);
    }

    setPulse(index, newPulse);
}

void Arm::extendWrist(float deg, bool relative)
{
    unsigned int newPulse;
    int offset;
    unsigned index = 3;

    if (relative) {
        deg = wristExtension() + deg;
    }

    offset = (int) roundf(_ppd[index] * deg);
    if (_side == RIGHT_ARM) {
        newPulse = (unsigned int) (((int) _center[index]) + offset);
    } else {
        newPulse = (unsigned int) (((int) _center[index]) - offset);
    }

    setPulse(index, newPulse);
}

void Arm::rotateWrist(float deg, bool relative)
{
    unsigned int newPulse;
    int offset;
    unsigned index = 4;

    if (relative) {
        deg = wristRotation() + deg;
    }

    offset = (int) roundf(_ppd[index] * deg);
    if (_side == RIGHT_ARM) {
        newPulse = (unsigned int) (((int) _center[index]) + offset);
    } else {
        newPulse = (unsigned int) (((int) _center[index]) - offset);
    }

    setPulse(index, newPulse);
}

void Arm::setGripperPosition(float pos, bool relative)
{
    unsigned int newPulse;
    unsigned index = 5;

    if (relative) {
        pos = gripperPosition() + pos;
        if (pos < 0) {
            pos = 0.0;
        }
    }

    newPulse = (((unsigned int) pos) * _ppd[index]) + _loRange[index] ;

    setPulse(index, newPulse);
}

void Arm::rest(void)
{
    rotateShoulder(0.0);
    extendShoulder(-85.0);
    extendElbow(-90.0);
    extendWrist(-40.0);
    rotateWrist(-90.0);
    setGripperPosition(20.0);
}

void Arm::surrender(void)
{
    hug();

    usleep(500000);

    rotateShoulder(90.0);
    extendShoulder(85.0);
    extendElbow(36.0);
    extendWrist(0.0);
    rotateWrist(0.0);
    setGripperPosition(20.0);
}

void Arm::hug(void)
{
    rotateShoulder(10.0);
    extendShoulder(85.0);
    extendElbow(45.0);
    extendWrist(0.0);
    rotateWrist(0.0);
    setGripperPosition(20.0);
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

