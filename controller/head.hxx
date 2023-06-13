/*
 * head.hxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#ifndef HEAD_HXX
#define HEAD_HXX

class Head {

public:

    Head();
    ~Head();

    void rotate(float deg, bool relative = false);
    void tilt(float deg, bool relative = false);
    float rotationAt(void) const;
    float tiltAt(void) const;

    enum EyebrowDisposition {
        EB_RELAXED = 0,
        EB_PERPLEXED = 1,
        EB_SURPRISED = 2,
        EB_HAPPY = 3,
        EB_JUBILANT = 4,
        EB_ANGRY = 5,
        EB_FURIOUS = 6,
        EB_SAD = 7,
        EB_DEPRESSED = 8,
        EB_MANUAL = 9,
    };

    void eyebrowRotate(float deg, bool relative = false, unsigned int lr = 0x3);
    void eyebrowTilt(float deg, bool relative = false, unsigned int lr = 0x03);
    void eyebrowSetDisposition(enum EyebrowDisposition disposition);
    float eyebrowRotationAt(unsigned int lr) const;
    float eyebrowTiltAt(unsigned int lr) const;
    enum EyebrowDisposition eyebrowDisposition(void) const;

    void earTilt(float deg, bool relative = false, unsigned int lr = 0x03);
    void earRotate(float deg, bool relative = false, unsigned int lr = 0x3);
    void earsUp(void);
    void earsBack(void);
    void earsDown(void);
    void earsFold(void);
    void earsHalfDown(void);
    void earsPointTo(float deg);
    float earTiltAt(unsigned int lr) const;
    float earRotationAt(unsigned int lr) const;

    void enSentry(bool enable);
    bool isSentryEn(void) const;

private:

    static void *thread_func(void *args);
    void run(void);
    void updateEyebrows(void);
    void updateEars(void);

    float _rotation;
    float _tilt;
    enum EyebrowDisposition _eyebrow;
    float _eb_r_rotation;
    float _eb_r_tilt;
    float _eb_l_rotation;
    float _eb_l_tilt;
    struct timeval _last_earsup;
    bool _sentry;

    pthread_t _thread;
    pthread_mutex_t _mutex;
    pthread_cond_t _cond;
    bool _running;

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
