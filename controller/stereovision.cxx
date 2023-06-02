/*
 * stereovision.cxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <bsd/sys/time.h>
#include <pthread.h>
#include <iostream>
#include <opencv2/opencv.hpp>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-copy"
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#pragma GCC diagnostic ignored "-Wsign-compare"
#include <librealsense2/rs.hpp>
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#include "stereovision.hxx"

using namespace std;

static unsigned int instance = 0;

StereoVision::StereoVision()
    : _rs2_pipeline(NULL)
{
    _running = true;
    pthread_mutex_init(&_mutex, NULL);
    pthread_cond_init(&_cond, NULL);
    pthread_create(&_thread, NULL, StereoVision::thread_func, this);

    printf("StereoVision is online\n");
}

StereoVision::~StereoVision()
{
    _running = false;
    pthread_cond_broadcast(&_cond);
    pthread_join(_thread, NULL);
    pthread_mutex_destroy(&_mutex);
    pthread_cond_destroy(&_cond);

    if (_rs2_pipeline != NULL) {
        delete (rs2::pipeline *) _rs2_pipeline;
        _rs2_pipeline = NULL;
    }

    instance--;
    printf("StereoVision is offline\n");
}

void StereoVision::probeOpenDevice(void)
{
    int ret = 0;
    rs2::config cfg;

    if (_rs2_pipeline != NULL) {
        return;
    }

    try {
        cfg.enable_stream(RS2_STREAM_COLOR, 640, 480, RS2_FORMAT_BGR8, 30);
        _rs2_pipeline = new rs2::pipeline();
        if (_rs2_pipeline) {
            ((rs2::pipeline *) _rs2_pipeline)->start(cfg);
            ret = 0;
        } else {
            ret = -1;
        }
    } catch (const rs2::error &e) {
        cerr << "RealSense error calling " <<
            e.get_failed_function() << "(" <<
            e.get_failed_args() << "):\n    " << e.what() << endl;
        ret = -1;
    } catch (const exception& e) {
        cerr << e.what() << endl;
        ret = -1;
    }

    if (ret != 0 && _rs2_pipeline != NULL) {
        delete (rs2::pipeline *) _rs2_pipeline;
        _rs2_pipeline = NULL;
    }
}

void *StereoVision::thread_func(void *args)
{
    StereoVision *sv = (StereoVision *) args;

    sv->run();

    return NULL;
}

void StereoVision::run(void)
{
    int ret;
    struct timespec ts, tloop;
    rs2::pipeline *p;

    tloop.tv_sec = 1;
    tloop.tv_nsec = 0;

    while (_running) {
        /* Probe and open device */
        probeOpenDevice();
        if (_rs2_pipeline == NULL) {
            clock_gettime(CLOCK_REALTIME, &ts);
            timespecadd(&ts, &tloop, &ts);
            pthread_mutex_lock(&_mutex);
            pthread_cond_timedwait(&_cond, &_mutex, &ts);
            pthread_mutex_unlock(&_mutex);
            continue;
        }

        /* Capture frame(s) */
        p = (rs2::pipeline *) _rs2_pipeline;
        ret = 0;
        try {
            rs2::frameset frames = p->wait_for_frames();
            rs2::frame color_frame = frames.get_color_frame();
            cv::Mat color(cv::Size(640, 480), CV_8UC3,
                          (void *) color_frame.get_data(),
                          cv::Mat::AUTO_STEP);
        } catch (const rs2::error &e) {
            cerr << "RealSense error calling " <<
                e.get_failed_function() << "(" <<
                e.get_failed_args() << "):\n    " << e.what() << endl;
            ret = -1;
        } catch (const exception& e) {
            cerr << e.what() << endl;
            ret = -1;
        }

        if (ret != 0 && _rs2_pipeline != NULL) {
            delete (rs2::pipeline *) _rs2_pipeline;
            _rs2_pipeline = NULL;
        }
    }
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
