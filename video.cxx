/*
 * video.cxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <iostream>
#include "video.hxx"

Video::Video()
{
    try {
        _vc = new VideoCapture(0, CAP_V4L);
        _vc->set(CAP_PROP_FRAME_WIDTH, 640);
        _vc->set(CAP_PROP_FRAME_HEIGHT, 480);
    } catch(cv::Exception &e) {
        cerr << e.msg << endl;
        return;
    }

    _running = 1;
    pthread_create(&_thread, NULL, Video::run, this);

    _streamer.start(8000);
}

Video::~Video()
{
    _streamer.stop();

    _running = 0;
    pthread_join(_thread, NULL);

    if (_vc) {
        delete _vc;
    }
}

void *Video::run(void *args)
{
    Video *video = (Video *) args;
    std::vector<int> params = { IMWRITE_JPEG_QUALITY, 90, };

    do {
        Mat frame;

        video->_vc->read(frame);

        if (frame.empty()) {
            cerr << "empty frame!" << endl;
            continue;
        }

        if (video->_streamer.isRunning()) {
            vector<uchar> buff_rgb;

            imencode(".jpg", frame, buff_rgb, params);
            video->_streamer.publish("/rgb", string(buff_rgb.begin(),
                                                    buff_rgb.end()));
        }
    } while (video->_running);

    return NULL;
}

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
