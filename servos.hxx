/*
 * servos.hxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#ifndef SERVOS_HXX
#define SERVOS_HXX

#define SERVO_CHANNELS 16

class Servos {

public:

    Servos();
    ~Servos();

public:

    void setRange(unsigned int chan, unsigned int lo, unsigned int hi);
    unsigned int loRange(unsigned int chan);
    unsigned int hiRange(unsigned int chan);
    unsigned int pulse(unsigned int chan) const;
    void setPulse(unsigned int chan, unsigned int pulse,
                  bool ignoreRange = false);
    void setPct(unsigned int chan, unsigned int pct);
    void center(unsigned int chan);

private:

    int readReg(uint8_t reg, uint8_t *val) const;
    int writeReg(uint8_t reg, uint8_t val) const;
    void setPwm(unsigned int chan, unsigned int on, unsigned int off);

    int _handle;
    unsigned int _freq;
    unsigned int _lo[SERVO_CHANNELS];
    unsigned int _hi[SERVO_CHANNELS];
    unsigned int _pulse[SERVO_CHANNELS];

};

inline unsigned int Servos::loRange(unsigned int chan)
{
    if (chan > SERVO_CHANNELS) {
        return 0;
    }

    return _lo[chan];
}

inline unsigned int Servos::hiRange(unsigned int chan)
{
    if (chan > SERVO_CHANNELS) {
        return 0;
    }

    return _hi[chan];
}

inline unsigned int Servos::pulse(unsigned int chan) const
{
    if (chan > SERVO_CHANNELS) {
        return 0;
    }

    return _pulse[chan];
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
