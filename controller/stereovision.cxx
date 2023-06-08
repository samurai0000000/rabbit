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
    : _rs2_pipeline(NULL),
      _vision(false),
      _imuEn(false),
      _emitterEn(false),
      _frColor(0.0),
      _frDepth(0.0),
      _frIR(0.0),
      _gyroSPS(0.0),
      _accelSPS(0.0)
{
    _running = true;
    pthread_mutex_init(&_mutex, NULL);
    pthread_cond_init(&_cond, NULL);
    pthread_create(&_thread, NULL, StereoVision::thread_func, this);
    pthread_setname_np(_thread, "R'StereoVision");

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

void StereoVision::probeOpenDevice(bool color, bool depth, bool infrared,
                                   bool gyro, bool accel)
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
        if (gyro) {
            cfg.enable_stream(RS2_STREAM_GYRO, RS2_FORMAT_MOTION_XYZ32F);
        }
        if (accel) {
            cfg.enable_stream(RS2_STREAM_ACCEL, RS2_FORMAT_MOTION_XYZ32F);
        }

        _rs2_pipeline = new rs2::pipeline();
        if (_rs2_pipeline) {
            ((rs2::pipeline *) _rs2_pipeline)->start(cfg);
            ret = 0;
        } else {
            ret = -1;
        }

        if (_emitterEn == false) {
            rs2::pipeline_profile profile;
            rs2::device device;
            vector<rs2::sensor> sensors;
            vector<rs2::sensor>::iterator it;

            profile = ((rs2::pipeline *) _rs2_pipeline)->get_active_profile();
            device = profile.get_device();
            sensors = device.query_sensors();
            for (it = sensors.begin(); it != sensors.end(); it++) {
                if (it->supports(RS2_OPTION_EMITTER_ENABLED)) {
                    it->set_option(RS2_OPTION_EMITTER_ENABLED, 0);
                }
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
        return;
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
    struct timeval now, tdiff;
    struct timeval tsColor, tsDepth, tsIR, tvColor, tvDepth, tvIR;
    struct timeval tvGyro, tvAccel;
    Mat colorScreen, depthScreen, irScreen;
    Mat colorOsd, depthOsd, irOsd;
    rs2::colorizer color_map;
    bool enableEmitter = isEmitterEnabled();

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
        if ((_vision == false) &&
            (_imuEn == false) &&
            (mjpeg_streamer->hasClient("/svcolor") == false) &&
            (mjpeg_streamer->hasClient("/svdepth") == false) &&
            (mjpeg_streamer->hasClient("/svir") == false)) {
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
        probeOpenDevice(_vision | mjpeg_streamer->hasClient("/svcolor"),
                        _vision | mjpeg_streamer->hasClient("/svdepth"),
                        _vision | mjpeg_streamer->hasClient("/svir"),
                        _vision | _imuEn,
                        _vision | _imuEn);
        if (_rs2_pipeline == NULL) {
            struct timespec twait;
            clock_gettime(CLOCK_REALTIME, &twait);
            twait.tv_sec += 1;
            pthread_mutex_lock(&_mutex);
            pthread_cond_timedwait(&_cond, &_mutex, &twait);
            pthread_mutex_unlock(&_mutex);
            continue;
        }

        /* Adjust emitter on/off */
        if (enableEmitter != _emitterEn) {
            rs2::pipeline_profile profile;
            rs2::device device;
            vector<rs2::sensor> sensors;
            vector<rs2::sensor>::iterator it;

            profile = ((rs2::pipeline *) _rs2_pipeline)->get_active_profile();
            device = profile.get_device();
            sensors = device.query_sensors();
            for (it = sensors.begin(); it != sensors.end(); it++) {
                if (it->supports(RS2_OPTION_EMITTER_ENABLED)) {
                    it->set_option(RS2_OPTION_EMITTER_ENABLED, _emitterEn);
                }
            }
            enableEmitter = _emitterEn;
        }

        /* Capture frame(s) */
        try {
            ret = 0;

            /* Wait for frames */
            rs2::frameset frames =
                ((rs2::pipeline *) _rs2_pipeline)->wait_for_frames();

            /* Color frame */
            if (_vision || mjpeg_streamer->hasClient("/svcolor")) {
                /* Get color frame */
                rs2::frame color_frame = frames.get_color_frame();
                /* Convert to OpenCV Mat */
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
            if (_vision || mjpeg_streamer->hasClient("/svdepth")) {
                /* Get depth frame */
                rs2::frame depth_frame = frames.get_depth_frame();

                /* Encode depth in colors */
                rs2::frame depth_colorized =
                    depth_frame.apply_filter(color_map);
                /* Convert to OpenCV Mat */
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
            if (_vision || mjpeg_streamer->hasClient("/svir")) {
                /* Get IR frame */
                rs2::frame ir_frame = frames.first(RS2_STREAM_INFRARED);

                /* Convert to OpenCV Mat */
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

            if (_imuEn) {
                /* Gyro motion frame */
                auto gyro_frame =
                    frames.first_or_default(RS2_STREAM_GYRO,
                                            RS2_FORMAT_MOTION_XYZ32F);
                auto gyro_motion = gyro_frame.as<rs2::motion_frame>();
                if (gyro_motion) {
                    /* Update sample rate */
                    gettimeofday(&now, NULL);
                    timersub(&now, &tvGyro, &tdiff);
                    _gyroSPS = 1.0 /
                        (tdiff.tv_sec + (tdiff.tv_usec * 0.000001));
                    memcpy(&tvGyro, &now, sizeof(struct timeval));

                    double ts = gyro_motion.get_timestamp();
                    rs2_vector gyro_data = gyro_motion.get_motion_data();
                    (void)(ts);
                    (void)(gyro_data);
                    //printf("G sps=%.1f ts=%f x=%.3f y=%.3f z=%.3f\n",
                    //       _gyroSPS, ts,
                    //       gyro_data.x, gyro_data.y, gyro_data.z);
                }

                /* Accelerometer frame */
                auto accel_frame =
                    frames.first_or_default(RS2_STREAM_ACCEL,
                                            RS2_FORMAT_MOTION_XYZ32F);
                auto accel_motion = accel_frame.as<rs2::motion_frame>();
                if (accel_motion) {
                    /* Update sample rate */
                    gettimeofday(&now, NULL);
                    timersub(&now, &tvAccel, &tdiff);
                    _accelSPS = 1.0 /
                        (tdiff.tv_sec + (tdiff.tv_usec * 0.000001));
                    memcpy(&tvAccel, &now, sizeof(struct timeval));

                    double ts = accel_motion.get_timestamp();
                    rs2_vector accel_data = accel_motion.get_motion_data();
                    (void)(ts);
                    (void)(accel_data);
                    //printf("A sps=%.1f ts=%f x=%.3f y=%.3f z=%.3f\n",
                    //       _accelSPS, ts,
                    //       accel_data.x, accel_data.y, accel_data.z);
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

void StereoVision::enVision(bool enable)
{
    enable = (enable ? true : false);
    if (_vision != enable) {
        _vision = enable;
        if (enable) {
            speech->speak("Stereo-vision vision enabled");
            LOG("Stereo-vision vision enabled\n");
        } else {
            speech->speak("Stereo-vision vision disabled");
            LOG("Stereo-vision vision disabled\n");
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
