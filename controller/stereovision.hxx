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

private:

    void probeOpenDevice(bool color, bool depth, bool infrared);
    static void *thread_func(void *args);
    void run(void);

    void *_rs2_pipeline;

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
