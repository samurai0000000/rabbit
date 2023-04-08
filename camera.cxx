/*
 * camera.cxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <iostream>
#include "rabbit.hxx"

#define CAMERA_RES_WIDTH   640
#define CAMERA_RES_HEIGHT  480

Camera::Camera()
    : _vc(NULL),
      _running(0),
      _vision(0)
{
    try {
        _vc = new VideoCapture(0, CAP_V4L);
        _vc->set(CAP_PROP_FRAME_WIDTH, CAMERA_RES_WIDTH);
        _vc->set(CAP_PROP_FRAME_HEIGHT, CAMERA_RES_HEIGHT);
   } catch(cv::Exception &e) {
        cerr << e.msg << endl;
        return;
    }

    _running = 1;
    pthread_create(&_thread, NULL, Camera::run, this);

    _streamer.start(8000);
}

Camera::~Camera()
{
    _running = 0;
    pthread_join(_thread, NULL);

    _streamer.stop();

    if (_vc) {
        delete _vc;
    }
}

void *Camera::run(void *args)
{
    Camera *camera = (Camera *) args;
    std::vector<int> params = { IMWRITE_JPEG_QUALITY, 90, };

    do {
        Mat frame;

        if (!camera->isVisionEn() && !camera->_streamer.hasClient("/rgb")) {
            if (camera->_vc->isOpened()) {
                camera->_vc->release();
            }
            usleep(1000000);
            continue;
        } else {
            if (!camera->_vc->isOpened()) {
                camera->_vc->open(0, CAP_V4L);
                camera->_vc->set(CAP_PROP_FRAME_WIDTH, CAMERA_RES_WIDTH);
                camera->_vc->set(CAP_PROP_FRAME_HEIGHT, CAMERA_RES_HEIGHT);
            }
        }

        camera->_vc->read(frame);

        if (frame.empty()) {
            cerr << "empty frame!" << endl;
            continue;
        }

        putText(frame,
                wheels->stateStr(),
                Point(CAMERA_RES_WIDTH / 2,      // X-pos
                      CAMERA_RES_HEIGHT - 50),   // Y-pos
                FONT_HERSHEY_COMPLEX,            // Font
                1,                               // Size
                Scalar(255, 255, 255),           // Color
                2                                // Weight
            );

        if (camera->_streamer.isRunning()) {
            vector<uchar> buff_rgb;

            imencode(".jpg", frame, buff_rgb, params);
            camera->_streamer.publish("/rgb", string(buff_rgb.begin(),
                                                     buff_rgb.end()));
        }
    } while (camera->_running);

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
