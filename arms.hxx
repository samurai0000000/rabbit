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

    void rotateShoulder(float deg, bool relative = false);
    void extendShoulder(float deg, bool relative = false);
    void extendElbow(float deg, bool relative = false);
    void extendWrist(float deg, bool relative = false);
    void rotateWrist(float deg, bool relative = false);
    void setGripperPosition(float pos, bool relative = false);

    void rest(void);
    void surrender(void);
    void hug(void);

private:

    void updateTrims(void);
    unsigned int pulse(unsigned int index) const;
    void setPulse(unsigned int index, unsigned int pulse);

    unsigned int _side;
    unsigned int _loRange[6];
    unsigned int _hiRange[6];
    unsigned int _range[6];
    unsigned int _center[6];
    float _ppd[6];

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
