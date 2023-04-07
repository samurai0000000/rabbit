/*
 * video.hxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#ifndef VIDEO_HXX
#define VIDEO_HXX

#include <pthread.h>
#include <opencv2/opencv.hpp>
#include <nadjieb/mjpeg_streamer.hpp>

using namespace std;
using namespace cv;
using namespace nadjieb;

class Video {

public:

    Video();
    ~Video();

private:

    static void *run(void *args);

    VideoCapture *_vc;
    MJPEGStreamer _streamer;
    pthread_t _thread;
    int _running;

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
