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

unsigned int Arm::lastPlannedPulse(unsigned index) const
{
    bool valid;
    unsigned int servoId;
    unsigned int pulse;

    if (_side == RIGHT_ARM) {
        servoId = index;
    } else {
        servoId = index + 6;
    }

    valid = servos->lastMotionPulseInPlan(servoId, &pulse);
    if (valid == false) {
        pulse = servos->pulse(servoId);
    }

    return pulse;
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
        servos->scheduleMotions(servoId, motions, true);
    }
}

void Arm::clearMotions(void)
{
    unsigned int i;
    unsigned int servoId;

    for (i = 0; i < 6; i++) {
        if (_side == RIGHT_ARM) {
            servoId = i;
        } else {
            servoId = i + 6;
        }

        servos->clearMotionSchedule(servoId);
    }
}

void Arm::syncMotions(bool bothArms)
{
    if (bothArms) {
        servos->syncMotionSchedule(0x0fff);
    } else {
        if (_side == RIGHT_ARM) {
            servos->syncMotionSchedule(0x003f);
        } else {
            servos->syncMotionSchedule(0x0fc0);
        }
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

bool Arm::isShoulderRotation(float deg) const
{
    unsigned int newPulse;
    int offset;
    unsigned index = 0;

    offset = (int) roundf(_ppd[index] * deg);
    if (_side == RIGHT_ARM) {
        newPulse = (unsigned int) (((int) _center[index]) + offset);
    } else {
        newPulse = (unsigned int) (((int) _center[index]) - offset);
    }

    return newPulse == pulse(index);
}

bool Arm::isShoulderExtension(float deg) const
{
    unsigned int newPulse;
    int offset;
    unsigned index = 1;

    offset = (int) roundf(_ppd[index] * deg);
    if (_side == RIGHT_ARM) {
        newPulse = (unsigned int) (((int) _center[index]) + offset);
    } else {
        newPulse = (unsigned int) (((int) _center[index]) - offset);
    }

    return newPulse == pulse(index);
}

bool Arm::isElbowExtension(float deg) const
{
    unsigned int newPulse;
    int offset;
    unsigned index = 2;

    offset = (int) roundf(_ppd[index] * deg);
    if (_side == RIGHT_ARM) {
        newPulse = (unsigned int) (((int) _center[index]) + offset);
    } else {
        newPulse = (unsigned int) (((int) _center[index]) - offset);
    }

    return newPulse == pulse(index);
}

bool Arm::isWristExtension(float deg) const
{
    unsigned int newPulse;
    int offset;
    unsigned index = 3;

    offset = (int) roundf(_ppd[index] * deg);
    if (_side == RIGHT_ARM) {
        newPulse = (unsigned int) (((int) _center[index]) + offset);
    } else {
        newPulse = (unsigned int) (((int) _center[index]) - offset);
    }

    return newPulse == pulse(index);
}

bool Arm::isWristRotation(float deg) const
{
    unsigned int newPulse;
    int offset;
    unsigned index = 4;

    offset = (int) roundf(_ppd[index] * deg);
    if (_side == RIGHT_ARM) {
        newPulse = (unsigned int) (((int) _center[index]) + offset);
    } else {
        newPulse = (unsigned int) (((int) _center[index]) - offset);
    }

    return newPulse == pulse(index);
}

bool Arm::isGripperPosition(float pos) const
{
    unsigned int newPulse;
    unsigned index = 5;

    newPulse = (((unsigned int) pos) * _ppd[index]) + _loRange[index] ;

    return newPulse == pulse(index);
}

float Arm::lastShoulderRotationInPlan(void) const
{
    float degree;
    unsigned int index = 0;
    unsigned int pulse;

    pulse = lastPlannedPulse(index);

    if (_side == RIGHT_ARM) {
        degree = (float) ((int) pulse - (int) _center[index]);
    } else {
        degree = (float) ((int) _center[index] - (int) pulse);
    }

    degree *= 180.0;
    degree /= (float) _range[index];

    return degree;
}

float Arm::lastShoulderExtensionInPlan(void) const
{
    float degree;
    unsigned int index = 1;
    unsigned int pulse;

    pulse = lastPlannedPulse(index);

    if (_side == RIGHT_ARM) {
        degree = (float) ((int) pulse - (int) _center[index]);
    } else {
        degree = (float) ((int) _center[index] - (int) pulse);
    }

    degree *= 180.0;
    degree /= (float) _range[index];

    return degree;
}

float Arm::lastElbowExtensionInPlan(void) const
{
    float degree;
    unsigned int index = 2;
    unsigned int pulse;

    pulse = lastPlannedPulse(index);

    if (_side == RIGHT_ARM) {
        degree = (float) ((int) pulse - (int) _center[index]);
    } else {
        degree = (float) ((int) _center[index] - (int) pulse);
    }

    degree *= 180.0;
    degree /= (float) _range[index];

    return degree;
}

float Arm::lastWristExtensionInPlan(void) const
{
    float degree;
    unsigned int index = 3;
    unsigned int pulse;

    pulse = lastPlannedPulse(index);

    if (_side == RIGHT_ARM) {
        degree = (float) ((int) pulse - (int) _center[index]);
    } else {
        degree = (float) ((int) _center[index] - (int) pulse);
    }

    degree *= 180.0;
    degree /= (float) _range[index];

    return degree;
}

float Arm::lastWristRotationInPlan(void) const
{
    float degree;
    unsigned int index = 4;
    unsigned int pulse;

    pulse = lastPlannedPulse(index);

    if (_side == RIGHT_ARM) {
        degree = (float) ((int) pulse - (int) _center[index]);
    } else {
        degree = (float) ((int) _center[index] - (int) pulse);
    }

    degree *= 180.0;
    degree /= (float) _range[index];

    return degree;
}

float Arm::lastGripperPositionInPlan(void) const
{
    float pos;
    unsigned index = 5;
    unsigned int pulse;

    pulse = lastPlannedPulse(index);
    pos = (float) ((pulse - _loRange[index]) / _ppd[index]);

    return pos;
}

void Arm::rotateShoulder(float deg, unsigned int ms, bool relative)
{
    unsigned int newPulse;
    int offset;
    unsigned index = 0;
    char buf[128];

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

    snprintf(buf, sizeof(buf) - 1,
             "%s arm rotate shoulder to %.1f degree in %ums\n",
             _side == RIGHT_ARM ? "Right" : "Left", deg, ms);
    LOG(buf);
}

void Arm::extendShoulder(float deg, unsigned int ms, bool relative)
{
    unsigned int newPulse;
    int offset;
    unsigned index = 1;
    char buf[128];

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
    snprintf(buf, sizeof(buf) - 1,
             "%s arm extend shoulder to %.1f degree in %ums\n",
             _side == RIGHT_ARM ? "Right" : "Left", deg, ms);
    LOG(buf);
}

void Arm::extendElbow(float deg, unsigned int ms, bool relative)
{
    unsigned int newPulse;
    int offset;
    unsigned index = 2;
    char buf[128];

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
    snprintf(buf, sizeof(buf) - 1,
             "%s arm extend elbow to %.1f degree in %ums\n",
             _side == RIGHT_ARM ? "Right" : "Left", deg, ms);
    LOG(buf);
}

void Arm::extendWrist(float deg, unsigned int ms, bool relative)
{
    unsigned int newPulse;
    int offset;
    unsigned index = 3;
    char buf[128];

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
    snprintf(buf, sizeof(buf) - 1,
             "%s arm extend wrist to %.1f degree in %ums\n",
             _side == RIGHT_ARM ? "Right" : "Left", deg, ms);
    LOG(buf);
}

void Arm::rotateWrist(float deg, unsigned int ms, bool relative)
{
    unsigned int newPulse;
    int offset;
    unsigned index = 4;
    char buf[128];

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
    snprintf(buf, sizeof(buf) - 1,
             "%s arm rotate wrist to %.1f degree in %ums\n",
             _side == RIGHT_ARM ? "Right" : "Left", deg, ms);
    LOG(buf);
}

void Arm::setGripperPosition(float pos, unsigned int ms, bool relative)
{
    unsigned int newPulse;
    unsigned index = 5;
    char buf[128];

    if (relative) {
        pos = gripperPosition() + pos;
        if (pos < 0) {
            pos = 0.0;
        }
    }

    newPulse = (((unsigned int) pos) * _ppd[index]) + _loRange[index] ;

    setPulse(index, newPulse, ms);
    snprintf(buf, sizeof(buf) - 1,
             "%s arm set gripper position to %.1f in %ums\n",
             _side == RIGHT_ARM ? "Right" : "Left", pos, ms);
    LOG(buf);
}

void Arm::planMotions(float shoulderRotateDeg, float shoulderExtensionDeg,
                      float elbowExtensionDeg,
                      float wristExtensionDeg, float wristRotationDeg,
                      float gripperPositionPos,
                      unsigned int ms,
                      bool skipIfAtPoint)
{
    /* Normalize the NAN values */
    if (isnan(shoulderRotateDeg)) {
        shoulderRotateDeg = lastShoulderRotationInPlan();
    }
    if (isnan(shoulderExtensionDeg)) {
        shoulderExtensionDeg = lastShoulderExtensionInPlan();
    }
    if (isnan(elbowExtensionDeg)) {
        elbowExtensionDeg = lastElbowExtensionInPlan();
    }
    if (isnan(wristExtensionDeg)) {
        wristExtensionDeg = lastWristExtensionInPlan();
    }
    if (isnan(wristRotationDeg)) {
        wristRotationDeg = lastWristRotationInPlan();
    }
    if (isnan(gripperPositionPos)) {
        gripperPositionPos = lastGripperPositionInPlan();
    }

    /* Skip the proposed plan if we're already at current position */
    if (skipIfAtPoint) {
        if (isShoulderRotation(shoulderRotateDeg) &&
            isShoulderExtension(shoulderExtensionDeg) &&
            isElbowExtension(elbowExtensionDeg) &&
            isWristExtension(wristExtensionDeg) &&
            isWristRotation(wristRotationDeg) &&
            isGripperPosition(gripperPositionPos)) {
            return;
        }
    }

    /* Schedule the motions */
#if 0
    printf("plan %f %f %f %f %f %f\n",
           shoulderRotateDeg, shoulderExtensionDeg,
           elbowExtensionDeg,
           wristExtensionDeg, wristRotationDeg,
           gripperPositionPos);
#endif
    rotateShoulder(shoulderRotateDeg, ms);
    extendShoulder(shoulderExtensionDeg, ms);
    extendElbow(elbowExtensionDeg, ms);
    extendWrist(wristExtensionDeg, ms);
    rotateWrist(wristRotationDeg, ms);
    setGripperPosition(gripperPositionPos, ms);
}

void Arm::rest(void)
{
    clearMotions();

    planMotions(0.0, -85.0,
                -90.0,
                -40.0, -90.0,
                NAN,
                1500,
                true);
}

void Arm::freeze(void)
{
    clearMotions();
}

void Arm::surrender(void)
{
    clearMotions();

    planMotions(50.0, 70.0,
                25.0,
                0.0, 0.0,
                NAN,
                1500,
                true);

    planMotions(90.0, 60.0,
                25.0,
                -12.5, 0.0,
                NAN,
                1500);
}

void Arm::hug(void)
{
    clearMotions();

    planMotions(10.0, 85.0,
                45.0,
                0.0, 0.0,
                NAN,
                1500);
}

void Arm::extend(void)
{
    clearMotions();

    planMotions(10.0, 0.0,
                45.0,
                0.0, 0.0,
                NAN,
                1500);
}

void Arm::hi(void)
{
    clearMotions();

    planMotions(50.0, 65.0,
                25.0,
                0.0, 0.0,
                20.0,
                1500);

    planMotions(90.0, 65.0,
                25.0,
                0.0, -90.0,
                NAN,
                1500);

    for (unsigned int i = 0; i < 5; i++) {
        planMotions(NAN, NAN,
                    NAN,
                    45.0, NAN,
                    NAN,
                    500);

        planMotions(NAN, NAN,
                    NAN,
                    0.0, NAN,
                    NAN,
                    500);
    }
}

void Arm::pickup(void)
{
    clearMotions();

    planMotions(NAN, NAN,
                NAN,
                NAN, NAN,
                0.0,
                1500);

    planMotions(-32.5, 40.0,
                -25.0,
                -5.0, -90.0,
                0.0,
                1500);

    planMotions(NAN, NAN,
                NAN,
                NAN, NAN,
                30.0,
                1500);

    planMotions(0.0, -85.0,
                -90.0,
                -40.0, -90.0,
                NAN,
                1500);
}

void Arm::xferRL(void)
{
    static const float xfer_R_SR_Deg = -5.0;
    static const float xfer_R_SE_Deg = 88.0;
    static const float xfer_R_EE_Deg = 30.0;
    static const float xfer_R_WE_Deg = -70.0;
    static const float xfer_R_WR_Deg = -90.0;
    static const float xfer_L_SR_Deg = 5.0;
    static const float xfer_L_SE_Deg = 88.0;
    static const float xfer_L_EE_Deg = 40.0;
    static const float xfer_L_WE_Deg = -85.0;
    static const float xfer_L_WR_Deg = -90.0;
    bool inPosition = false;

    clearMotions();

    if (_side == RIGHT_ARM) {
        if (isShoulderRotation(xfer_R_SR_Deg) &&
            isShoulderExtension(xfer_R_SE_Deg) &&
            isElbowExtension(xfer_R_EE_Deg) &&
            isWristExtension(xfer_R_WE_Deg) &&
            isWristRotation(xfer_R_WR_Deg)) {
            inPosition = true;
        }
    } else {
        if (isShoulderRotation(xfer_L_SR_Deg) &&
            isShoulderExtension(xfer_L_SE_Deg) &&
            isElbowExtension(xfer_L_EE_Deg) &&
            isWristExtension(xfer_L_WE_Deg) &&
            isWristRotation(xfer_L_WR_Deg)) {
            inPosition = true;
        }
    }

    if (inPosition == false) {
        /* Move into position */
        if (_side == RIGHT_ARM) {
            planMotions(10.0, 85.0,
                        45.0,
                        0.0, xfer_R_WR_Deg,
                        NAN,
                        1500, true);

            planMotions(xfer_R_SR_Deg - 10.0, xfer_R_SE_Deg,
                        xfer_R_EE_Deg,
                        xfer_R_WE_Deg, NAN,
                        NAN,
                        1000,
                        true);
        } else {
            planMotions(10.0, 85.0,
                        45.0,
                        0.0, xfer_L_WR_Deg,
                        0.0,
                        1500, true);

            planMotions(xfer_L_SR_Deg + 10.0, xfer_L_SE_Deg,
                        xfer_L_EE_Deg,
                        xfer_L_WE_Deg, NAN,
                        NAN,
                        1000,
                        true);
        }

        if (_side == RIGHT_ARM) {
            planMotions(xfer_R_SR_Deg, NAN,
                        NAN,
                        NAN, NAN,
                        NAN,
                        2000);
        } else {
            planMotions(xfer_L_SR_Deg, NAN,
                        NAN,
                        NAN, NAN,
                        NAN,
                        2000);
        }
    } else {
        /* Transfer gripped object */
        if (_side == RIGHT_ARM) {
            planMotions(NAN, NAN,
                        NAN,
                        NAN, NAN,
                        NAN,
                        1000);
            planMotions(NAN, NAN,
                        NAN,
                        NAN, NAN,
                        0.0,
                        1000);
        } else {
            planMotions(NAN, NAN,
                        NAN,
                        NAN, NAN,
                        30.0,
                        1000);
            planMotions(NAN, NAN,
                        NAN,
                        NAN, NAN,
                        NAN,
                        1000);
        }
    }
}

void Arm::xferLR(void)
{
    static const float xfer_R_SR_Deg = 5.0;
    static const float xfer_R_SE_Deg = 88.0;
    static const float xfer_R_EE_Deg = 30.0;
    static const float xfer_R_WE_Deg = -70.0;
    static const float xfer_R_WR_Deg = -90.0;
    static const float xfer_L_SR_Deg = -5.0;
    static const float xfer_L_SE_Deg = 88.0;
    static const float xfer_L_EE_Deg = 40.0;
    static const float xfer_L_WE_Deg = -85.0;
    static const float xfer_L_WR_Deg = -90.0;
    bool inPosition = false;

    clearMotions();

    if (_side == RIGHT_ARM) {
        if (isShoulderRotation(xfer_R_SR_Deg) &&
            isShoulderExtension(xfer_R_SE_Deg) &&
            isElbowExtension(xfer_R_EE_Deg) &&
            isWristExtension(xfer_R_WE_Deg) &&
            isWristRotation(xfer_R_WR_Deg)) {
            inPosition = true;
        }
    } else {
        if (isShoulderRotation(xfer_L_SR_Deg) &&
            isShoulderExtension(xfer_L_SE_Deg) &&
            isElbowExtension(xfer_L_EE_Deg) &&
            isWristExtension(xfer_L_WE_Deg) &&
            isWristRotation(xfer_L_WR_Deg)) {
            inPosition = true;
        }
    }

    if (inPosition == false) {
        /* Move into position */
        if (_side == RIGHT_ARM) {
            planMotions(10.0, 85.0,
                        45.0,
                        0.0, xfer_R_WR_Deg,
                        0.0,
                        1500, true);

            planMotions(xfer_R_SR_Deg + 10.0, xfer_R_SE_Deg,
                        xfer_R_EE_Deg,
                        xfer_R_WE_Deg, NAN,
                        NAN,
                        1000,
                        true);
        } else {
            planMotions(10.0, 85.0,
                        45.0,
                        0.0, xfer_L_WR_Deg,
                        NAN,
                        1500, true);

            planMotions(xfer_L_SR_Deg - 10.0, xfer_L_SE_Deg,
                        xfer_L_EE_Deg,
                        xfer_L_WE_Deg, NAN,
                        NAN,
                        1000,
                        true);
        }

        if (_side == RIGHT_ARM) {
            planMotions(xfer_R_SR_Deg, NAN,
                        NAN,
                        NAN, NAN,
                        NAN,
                        2000);
        } else {
            planMotions(xfer_L_SR_Deg, NAN,
                        NAN,
                        NAN, NAN,
                        NAN,
                        2000);
        }
    } else {
        /* Transfer gripped object */
        if (_side == RIGHT_ARM) {
            planMotions(NAN, NAN,
                        NAN,
                        NAN, NAN,
                        30.0,
                        1000);
            planMotions(NAN, NAN,
                        NAN,
                        NAN, NAN,
                        NAN,
                        1000);
        } else {
            planMotions(NAN, NAN,
                        NAN,
                        NAN, NAN,
                        NAN,
                        1000);
            planMotions(NAN, NAN,
                        NAN,
                        NAN, NAN,
                        0.0,
                        1000);
        }
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

