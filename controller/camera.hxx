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

class Camera {

public:

    Camera();
    ~Camera();

    void enVision(bool enable);
    bool isVisionEn(void) const;

    void enSentry(bool enable);
    bool isSentryEn(void) const;

    void pan(float deg, bool relative = false);
    void tilt(float deg, bool relative = false);
    float panAt(void) const;
    float tiltAt(void) const;

    float frameRate(void) const;

private:

    static void *thread_func(void *args);
    void run(void);
    void detectFaces(cv::Mat &frame, std::vector<cv::Point> &ptFaces);
    void updateOsd1(cv::Mat &osd1, const struct timeval *since,
                    int fontFace, double fontScale, int thickness,
                    const cv::Scalar &fontColor, const cv::Size &textSize);

    cv::VideoCapture *_vc;
    nadjieb::MJPEGStreamer _streamer;
    pthread_t _thread;
    pthread_mutex_t _mutex;
    pthread_cond_t _cond;
    bool _running;
    bool _vision;
    float _fr;
    bool _sentry;
    cv::CascadeClassifier _cascade;

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
