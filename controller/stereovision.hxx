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

    float colorFrameRate(void) const;
    float depthFrameRate(void) const;
    float infraredFrameRate(void) const;

private:

    void probeOpenDevice(bool color, bool depth, bool infrared);
    static void *thread_func(void *args);
    void run(void);

    void *_rs2_pipeline;
    float _frColor;
    float _frDepth;
    float _frIR;

    pthread_t _thread;
    pthread_mutex_t _mutex;
    pthread_cond_t _cond;
    bool _running;

};

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
