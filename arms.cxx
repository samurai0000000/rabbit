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

    rotateShoulder(0.0);
    extendShoulder(-85.0);
    extendElbow(-90.0);
    extendWrist(-40.0);
    rotateWrist(-90.0);
    setGripperPosition(20.0);
}

Arm::~Arm()
{
    rest();
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

void Arm::clearSchedules(void)
{
    unsigned int i;
    unsigned int servoId;

    for (i = 0; i < 6; i++) {
        if (_side == RIGHT_ARM) {
            servoId = i;
        } else {
            servoId = i + 6;
        }

        servos->clearSchedule(servoId);
    }
}

void Arm::setPulse(unsigned int index, unsigned int pulse, unsigned int ms)
{
    unsigned int servoId;
    vector<struct servo_motion> motions;
    struct servo_motion motion;

    if (_side == RIGHT_ARM) {
        servoId = index;
    } else {
        servoId = index + 6;
    }

    if (ms <= SERVO_SCHEDULE_INTERVAL_MS) {
        servos->setPulse(servoId, pulse);
    } else {
        motion.pulse = pulse;
        motion.ms = ms;

        motions.push_back(motion);
        servos->schedule(servoId, motions, true);
    }
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

void Arm::rotateShoulder(float deg, unsigned int ms, bool relative)
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

    setPulse(index, newPulse, ms);
}

void Arm::extendShoulder(float deg, unsigned int ms, bool relative)
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

    setPulse(index, newPulse, ms);
}

void Arm::extendElbow(float deg, unsigned int ms, bool relative)
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

    setPulse(index, newPulse, ms);
}

void Arm::extendWrist(float deg, unsigned int ms, bool relative)
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

    setPulse(index, newPulse, ms);
}

void Arm::rotateWrist(float deg, unsigned int ms, bool relative)
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

    setPulse(index, newPulse, ms);
}

void Arm::setGripperPosition(float pos, unsigned int ms, bool relative)
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

    setPulse(index, newPulse, ms);
}

void Arm::rest(void)
{
    clearSchedules();

    rotateShoulder(0.0, 1500);
    extendShoulder(-85.0, 1500);
    extendElbow(-90.0, 1500);
    extendWrist(-40.0, 1500);
    rotateWrist(-90.0, 1500);
    setGripperPosition(20.0, 1500);
}

void Arm::surrender(void)
{
    clearSchedules();

    rotateShoulder(10.0, 1500);
    extendShoulder(85.0, 1500);
    extendElbow(45.0, 1500);
    extendWrist(0.0, 1500);
    rotateWrist(0.0, 1500);
    setGripperPosition(20.0, 1500);

    rotateShoulder(90.0, 1500);
    extendShoulder(85.0, 1500);
    extendElbow(36.0, 1500);
    extendWrist(0.0, 1500);
    rotateWrist(0.0, 1500);
    setGripperPosition(20.0, 1500);
}

void Arm::hug(void)
{
    clearSchedules();

    rotateShoulder(10.0, 1500);
    extendShoulder(85.0, 1500);
    extendElbow(45.0, 1500);
    extendWrist(0.0, 1500);
    rotateWrist(0.0, 1500);
    setGripperPosition(20.0, 1500);
}

void Arm::pickup(void)
{
    clearSchedules();

    rotateShoulder(-32.5, 1500);
    extendShoulder(40.0, 1500);
    extendElbow(-25.0, 1500);
    extendWrist(-5.0, 1500);
    rotateWrist(-90.0, 1500);
    setGripperPosition(0.0, 1500);

    rotateShoulder(-32.5, 1500);
    extendShoulder(40.0, 1500);
    extendElbow(-25.0, 1500);
    extendWrist(-5.0, 1500);
    rotateWrist(-90.0, 1500);
    setGripperPosition(28.0, 1500);

    rotateShoulder(90, 1500);
    extendShoulder(-70.0, 1500);
    extendElbow(-80.0, 1500);
    extendWrist(-40.0, 1500);
    rotateWrist(-90.0, 1500);
    setGripperPosition(28.0, 1500);

    rotateShoulder(90, 1500);
    extendShoulder(51.0, 1500);
    extendElbow(-23.0, 1500);
    extendWrist(-50.0, 1500);
    rotateWrist(0.0, 1500);
    setGripperPosition(28.0, 1500);

    rotateShoulder(90, 1500);
    extendShoulder(51.0, 1500);
    extendElbow(-23.0, 1500);
    extendWrist(-50.0, 1500);
    rotateWrist(0.0, 1500);
    setGripperPosition(0.0, 1500);

    rotateShoulder(0.0, 1500);
    extendShoulder(-85.0, 1500);
    extendElbow(-90.0, 1500);
    extendWrist(-40.0, 1500);
    rotateWrist(-90.0, 1500);
    setGripperPosition(20.0, 1500);
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

