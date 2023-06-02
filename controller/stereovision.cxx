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
#include "rabbit.hxx"

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

void StereoVision::probeOpenDevice(bool color, bool depth, bool infrared)
{
    int ret = 0;
    rs2::config cfg;

    if (_rs2_pipeline != NULL) {
        return;
    }

    try {
        if (color) {
            cfg.enable_stream(RS2_STREAM_COLOR, 640, 480, RS2_FORMAT_BGR8, 30);
        }
        if (depth) {
            cfg.enable_stream(RS2_STREAM_DEPTH, 640, 480, RS2_FORMAT_Z16, 30);
        }
        if (infrared) {
            cfg.enable_stream(RS2_STREAM_INFRARED, 640, 480, RS2_FORMAT_Y8, 30);
        }
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

    tloop.tv_sec = 1;
    tloop.tv_nsec = 0;

    while (_running) {
        /* Close the device if there's no requestor */
        if ((mjpeg_streamer->isRunning() == false) ||
            (mjpeg_streamer->hasClient("/svcolor") == false &&
             mjpeg_streamer->hasClient("/svdepth") == false &&
             mjpeg_streamer->hasClient("/svir") == false)) {
            try {
                if (_rs2_pipeline != NULL) {
                    delete (rs2::pipeline *) _rs2_pipeline;
                    _rs2_pipeline = NULL;
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

            clock_gettime(CLOCK_REALTIME, &ts);
            timespecadd(&ts, &tloop, &ts);
            pthread_mutex_lock(&_mutex);
            pthread_cond_timedwait(&_cond, &_mutex, &ts);
            pthread_mutex_unlock(&_mutex);
            continue;
        }

        /* Probe and open device */
        probeOpenDevice(mjpeg_streamer->hasClient("/svcolor"),
                        mjpeg_streamer->hasClient("/svdepth"),
                        mjpeg_streamer->hasClient("/svir"));
        if (_rs2_pipeline == NULL) {
            clock_gettime(CLOCK_REALTIME, &ts);
            timespecadd(&ts, &tloop, &ts);
            pthread_mutex_lock(&_mutex);
            pthread_cond_timedwait(&_cond, &_mutex, &ts);
            pthread_mutex_unlock(&_mutex);
            continue;
        }

        /* Capture frame(s) */
        try {
            ret = 0;
            rs2::frameset frames =
                ((rs2::pipeline *) _rs2_pipeline)->wait_for_frames();

            /* Color frame */
            if (mjpeg_streamer->hasClient("/svcolor")) {
                rs2::frame color_frame = frames.get_color_frame();
                cv::Mat color(cv::Size(640, 480), CV_8UC3,
                              (void *) color_frame.get_data(),
                              cv::Mat::AUTO_STEP);
                vector<uchar> buff_svcolor;
                std::vector<int> params = { cv::IMWRITE_JPEG_QUALITY, 90, };
                cv::imencode(".jpg", color, buff_svcolor, params);
                mjpeg_streamer->publish("/svcolor", string(buff_svcolor.begin(),
                                                           buff_svcolor.end()));
            }

            /* Depth frame */
            if (mjpeg_streamer->hasClient("/svdepth")) {
                rs2::frame depth_frame = frames.get_depth_frame();
                cv::Mat depth(cv::Size(640, 480), CV_16UC1,
                              (void *) depth_frame.get_data(),
                              cv::Mat::AUTO_STEP);
                vector<uchar> buff_svdepth;
                std::vector<int> params = { cv::IMWRITE_JPEG_QUALITY, 90, };
                cv::imencode(".jpg", depth, buff_svdepth, params);
                mjpeg_streamer->publish("/svdepth", string(buff_svdepth.begin(),
                                                           buff_svdepth.end()));
            }
            /* IR frame */
            if (mjpeg_streamer->hasClient("/svir")) {
                rs2::frame ir_frame = frames.first(RS2_STREAM_INFRARED);
                cv::Mat ir(cv::Size(640, 480), CV_8UC1,
                              (void *) ir_frame.get_data(),
                           cv::Mat::AUTO_STEP);
                cv::equalizeHist(ir, ir);
                cv::applyColorMap(ir, ir, cv::COLORMAP_JET);
                vector<uchar> buff_svir;
                std::vector<int> params = { cv::IMWRITE_JPEG_QUALITY, 90, };
                cv::imencode(".jpg", ir, buff_svir, params);
                mjpeg_streamer->publish("/svir", string(buff_svir.begin(),
                                                        buff_svir.end()));
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
