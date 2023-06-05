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
#include "osdcam.hxx"

using namespace std;
using namespace cv;

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
    struct timeval now, tdiff, tsColor, tsDepth, tsIR, tvColor, tvDepth, tvIR;
    Mat colorScreen, depthScreen, irScreen;
    Mat colorOsd, depthOsd, irOsd;
    rs2::colorizer color_map;

    /* Setup */
    colorOsd.create(Size(300, 480), CV_8UC3);
    colorScreen.create(Size(640 + colorOsd.cols, 480), CV_8UC3);
    depthOsd.create(Size(300, 480), CV_8UC3);
    depthScreen.create(Size(640 + depthOsd.cols, 480), CV_8UC3);
    irOsd.create(Size(300, 480), CV_8UC3);
    irScreen.create(Size(640 + irOsd.cols, 480), CV_8UC3);
    gettimeofday(&now, NULL);
    tdiff.tv_sec = 1;
    tdiff.tv_usec = 0;
    timersub(&now, &tdiff, &tsColor);
    timersub(&now, &tdiff, &tsDepth);
    timersub(&now, &tdiff, &tsIR);
    memcpy(&tvColor, &now, sizeof(struct timeval));
    memcpy(&tvDepth, &now, sizeof(struct timeval));
    memcpy(&tvIR, &now, sizeof(struct timeval));

    while (_running) {
        /* Close the device if there's no requestor */
        if ((mjpeg_streamer->isRunning() == false) ||
            (mjpeg_streamer->hasClient("/svcolor") == false &&
             mjpeg_streamer->hasClient("/svdepth") == false &&
             mjpeg_streamer->hasClient("/svir") == false)) {
            struct timespec twait;

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

            clock_gettime(CLOCK_REALTIME, &twait);
            twait.tv_sec += 1;
            pthread_mutex_lock(&_mutex);
            pthread_cond_timedwait(&_cond, &_mutex, &twait);
            pthread_mutex_unlock(&_mutex);
            continue;
        }

        /* Probe and open device */
        probeOpenDevice(mjpeg_streamer->hasClient("/svcolor"),
                        mjpeg_streamer->hasClient("/svdepth"),
                        mjpeg_streamer->hasClient("/svir"));
        if (_rs2_pipeline == NULL) {
            struct timespec twait;
            clock_gettime(CLOCK_REALTIME, &twait);
            twait.tv_sec += 1;
            pthread_mutex_lock(&_mutex);
            pthread_cond_timedwait(&_cond, &_mutex, &twait);
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
                Mat color(Size(640, 480), CV_8UC3,
                          (void *) color_frame.get_data(),
                          Mat::AUTO_STEP);

                /* Update frame rate */
                gettimeofday(&now, NULL);
                timersub(&now, &tvColor, &tdiff);
                _frColor = 1.0 / (tdiff.tv_sec + (tdiff.tv_usec * 0.000001));
                memcpy(&tvColor, &now, sizeof(struct timeval));

                /* Update OSD */
                timersub(&now, &tsColor, &tdiff);
                if (tdiff.tv_sec > 0 || tdiff.tv_usec > 500000) {
                    OsdCam::genOsdFrame(colorOsd, colorFrameRate());
                    gettimeofday(&tsColor, NULL);
                }

                /* Compose screen */
                color.copyTo(colorScreen(Rect(0, 0, color.cols, color.rows)));
                colorOsd.copyTo(colorScreen(Rect(640, 0,
                                                 colorOsd.cols,
                                                 colorOsd.rows)));


                /* Publish */
                if (mjpeg_streamer->isRunning()) {
                    std::vector<int> params = { IMWRITE_JPEG_QUALITY, 90, };
                    vector<uchar> buff_svcolor;

                    imencode(".jpg", colorScreen, buff_svcolor, params);
                    mjpeg_streamer->publish("/svcolor",
                                            string(buff_svcolor.begin(),
                                                   buff_svcolor.end()));
                }
            }

            /* Depth frame */
            if (mjpeg_streamer->hasClient("/svdepth")) {
                rs2::frame depth_frame = frames.get_depth_frame();

                /* Encode depth in colors */
                rs2::frame depth_colorized =
                    depth_frame.apply_filter(color_map);
                Mat depth(Size(640, 480), CV_8UC3,
                          (void *) depth_colorized.get_data(),
                          Mat::AUTO_STEP);

                /* Update frame rate */
                gettimeofday(&now, NULL);
                timersub(&now, &tvDepth, &tdiff);
                _frDepth = 1.0 / (tdiff.tv_sec + (tdiff.tv_usec * 0.000001));
                memcpy(&tvDepth, &now, sizeof(struct timeval));

                /* Update OSD */
                timersub(&now, &tsDepth, &tdiff);
                if (tdiff.tv_sec > 0 || tdiff.tv_usec > 500000) {
                    OsdCam::genOsdFrame(depthOsd, depthFrameRate());
                    gettimeofday(&tsDepth, NULL);
                }

                /* Compose screen */
                depth.copyTo(depthScreen(Rect(0, 0, depth.cols, depth.rows)));
                depthOsd.copyTo(depthScreen(Rect(640, 0,
                                                 depthOsd.cols,
                                                 depthOsd.rows)));


                /* Publish */
                if (mjpeg_streamer->isRunning()) {
                    std::vector<int> params = { IMWRITE_JPEG_QUALITY, 90, };
                    vector<uchar> buff_svdepth;

                    imencode(".jpg", depthScreen, buff_svdepth, params);
                    mjpeg_streamer->publish("/svdepth",
                                            string(buff_svdepth.begin(),
                                                   buff_svdepth.end()));
                }
            }
            /* IR frame */
            if (mjpeg_streamer->hasClient("/svir")) {
                rs2::frame ir_frame = frames.first(RS2_STREAM_INFRARED);
                Mat ir(Size(640, 480), CV_8UC1,
                       (void *) ir_frame.get_data(),
                       Mat::AUTO_STEP);
                equalizeHist(ir, ir);
                applyColorMap(ir, ir, COLORMAP_JET);

                /* Update frame rate */
                gettimeofday(&now, NULL);
                timersub(&now, &tvIR, &tdiff);
                _frIR = 1.0 / (tdiff.tv_sec + (tdiff.tv_usec * 0.000001));
                memcpy(&tvIR, &now, sizeof(struct timeval));

                /* Update OSD */
                timersub(&now, &tsIR, &tdiff);
                if (tdiff.tv_sec > 0 || tdiff.tv_usec > 500000) {
                    OsdCam::genOsdFrame(irOsd, infraredFrameRate());
                    gettimeofday(&tsIR, NULL);
                }

                /* Compose screen */
                ir.copyTo(irScreen(Rect(0, 0, ir.cols, ir.rows)));
                irOsd.copyTo(irScreen(Rect(640, 0,
                                           irOsd.cols,
                                           irOsd.rows)));


                /* Publish */
                if (mjpeg_streamer->isRunning()) {
                    std::vector<int> params = { IMWRITE_JPEG_QUALITY, 90, };
                    vector<uchar> buff_svir;

                    imencode(".jpg", irScreen, buff_svir, params);
                    mjpeg_streamer->publish("/svir",
                                            string(buff_svir.begin(),
                                                   buff_svir.end()));
                }
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
