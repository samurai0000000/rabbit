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

    float shoulderRotation(void) const;
    float shoulderExtension(void) const;
    float elbowExtension(void) const;
    float wristExtension(void) const;
    float wristRotation(void) const;
    float gripperPosition(void) const;

    bool isShoulderRotation(float deg) const;
    bool isShoulderExtension(float deg) const;
    bool isElbowExtension(float deg) const;
    bool isWristExtension(float deg) const;
    bool isWristRotation(float deg) const;
    bool isGripperPosition(float pos) const;

    float lastShoulderRotationInPlan(void) const;
    float lastShoulderExtensionInPlan(void) const;
    float lastElbowExtensionInPlan(void) const;
    float lastWristExtensionInPlan(void) const;
    float lastWristRotationInPlan(void) const;
    float lastGripperPositionInPlan(void) const;

    void rotateShoulder(float deg,
                        unsigned int ms = SERVO_SCHEDULE_INTERVAL_MS,
                        bool relative = false);
    void extendShoulder(float deg,
                        unsigned int ms = SERVO_SCHEDULE_INTERVAL_MS,
                        bool relative = false);
    void extendElbow(float deg,
                     unsigned int ms = SERVO_SCHEDULE_INTERVAL_MS,
                     bool relative = false);
    void extendWrist(float deg,
                     unsigned int ms = SERVO_SCHEDULE_INTERVAL_MS,
                     bool relative = false);
    void rotateWrist(float deg,
                     unsigned int ms = SERVO_SCHEDULE_INTERVAL_MS,
                     bool relative = false);
    void setGripperPosition(float pos,
                            unsigned int ms = SERVO_SCHEDULE_INTERVAL_MS,
                            bool relative = false);
    void planMotions(float shoulderRotateDeg, float shoulderExtensionDeg,
                     float elbowExtensionDeg,
                     float wristExtensionDeg, float wristRotationDeg,
                     float gripperPositionPos,
                     unsigned int ms,
                     bool skipIfAtPoint = false);

    void rest(void);
    void freeze(void);
    void surrender(void);
    void hug(void);
    void extend(void);
    void hi(void);
    void pickup(void);
    void xferRL(void);
    void xferLR(void);

private:

    void updateTrims(void);
    unsigned int pulse(unsigned int index) const;
    unsigned int lastPlannedPulse(unsigned index) const;
    void setPulse(unsigned int index, unsigned int pulse, unsigned int ms);
    void clearMotions(void);
    void syncMotions(bool bothArms = false);

    unsigned int _side;
    unsigned int _loRange[6];
    unsigned int _hiRange[6];
    unsigned int _range[6];
    unsigned int _center[6];
    float _ppd[6];  // Pulses per degree

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
