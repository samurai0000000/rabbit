/*
 * stereovision.hxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#ifndef STEREOVISION_HXX
#define STEREOVISION_HXX

class StereoVision {

public:

    StereoVision();
    ~StereoVision();

    void enVision(bool enable);
    bool isVisionEn(void) const;

    bool isIMUEnabled(void) const;
    void enableIMU(bool en);

    bool isEmitterEnabled(void) const;
    void enableEmitter(bool en);

    float colorFrameRate(void) const;
    float depthFrameRate(void) const;
    float infraredFrameRate(void) const;
    float gyroSamplesPerSec(void) const;
    float accelSamplesPerSec(void) const;

private:

    void probeOpenDevice(bool color, bool depth, bool infrared,
                         bool gyro, bool accel);
    static void *thread_func(void *args);
    void run(void);

    void *_rs2_pipeline;
    bool _vision;
    bool _imuEn;
    bool _emitterEn;
    float _frColor;
    float _frDepth;
    float _frIR;
    float _gyroSPS;
    float _accelSPS;

    pthread_t _thread;
    pthread_mutex_t _mutex;
    pthread_cond_t _cond;
    bool _running;

};

inline bool StereoVision::isVisionEn(void) const
{
    return _vision;
}

inline bool StereoVision::isIMUEnabled(void) const
{
    return _imuEn;
}

inline void StereoVision::enableIMU(bool en)
{
    en = en ? true : false;
    _imuEn = en;
}

inline bool StereoVision::isEmitterEnabled(void) const
{
    return _emitterEn;
}

inline void StereoVision::enableEmitter(bool en)
{
    en = en ? true : false;
    _emitterEn = en;
}

inline float StereoVision::colorFrameRate(void) const
{
    return _frColor;
}

inline float StereoVision::depthFrameRate(void) const
{
    return _frDepth;
}

inline float StereoVision::infraredFrameRate(void) const
{
    return _frIR;
}

inline float StereoVision::gyroSamplesPerSec(void) const
{
    return _gyroSPS;
}

inline float StereoVision::accelSamplesPerSec(void) const
{
    return _accelSPS;
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
