/*
 * camera.hxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#ifndef CAMERA_HXX
#define CAMERA_HXX

#include <pthread.h>
#include <opencv2/opencv.hpp>
#include <nadjieb/mjpeg_streamer.hpp>

using namespace std;
using namespace cv;
using namespace nadjieb;

class Camera {

public:

    Camera();
    ~Camera();

    void enVision(bool enable);
    bool isVisionEn(void) const;

    void enSentry(bool enable);
    bool isSentryEn(void) const;

    void pan(int deg, bool relative = false);
    void tilt(int deg, bool relative = false);

    int panAt(void) const;
    int tiltAt(void) const;
    float frameRate(void) const;

private:

    static void *thread_func(void *args);
    void run();

    VideoCapture *_vc;
    MJPEGStreamer _streamer;
    pthread_t _thread;
    int _running;
    bool _vision;
    int _pan;
    int _tilt;
    float _fr;
    struct {
        bool enabled;
        int dir;
    } _sentry;

};

inline void Camera::enVision(bool enable)
{
    _vision = enable ? true : false;
}

inline bool Camera::isVisionEn(void) const
{
    return _vision;
}

inline void Camera::enSentry(bool enable)
{
    _sentry.enabled = enable ? true : false;
    enVision(enable);
}

inline bool Camera::isSentryEn(void) const
{
    return _sentry.enabled;
}

inline int Camera::panAt(void) const
{
    return _pan;
}

inline int Camera::tiltAt(void) const
{
    return _tilt;
}

inline float Camera::frameRate(void) const
{
    return _fr;
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
