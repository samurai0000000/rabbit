/*
 * camera.cxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <time.h>
#include <sys/time.h>
#include <iostream>
#include "rabbit.hxx"
#include "osdcam.hxx"

#define V4L_CAPTURE_DEVICE    0
#define CAMERA_RES_WIDTH    640
#define CAMERA_RES_HEIGHT   480

#define PAN_SERVO            12
#define PAN_LO_PULSE        640
#define PAN_HI_PULSE       2180
#define PAN_ANGLE_RANGE     140
#define PAN_ANGLE_MULT      ((PAN_HI_PULSE - PAN_LO_PULSE) /    \
                             PAN_ANGLE_RANGE)

#define TILT_SERVO           13
#define TILT_LO_PULSE       800
#define TILT_HI_PULSE      1550
#define TILT_ANGLE_RANGE     90
#define TILT_ANGLE_MULT     ((TILT_HI_PULSE - TILT_LO_PULSE) /  \
                             TILT_ANGLE_RANGE)

using namespace std;
using namespace cv;

static unsigned int instance = 0;

Camera::Camera()
    : _vc(NULL),
      _running(false),
      _vision(0),
      _fr(0.0),
      _sentry(false)
{
    if (instance != 0) {
        fprintf(stderr, "Camera can be instantiated only once!\n");
        exit(EXIT_FAILURE);
    } else {
        instance++;
    }

    try {
        _vc = new VideoCapture(V4L_CAPTURE_DEVICE, CAP_V4L);
        _vc->set(CAP_PROP_FRAME_WIDTH, CAMERA_RES_WIDTH);
        _vc->set(CAP_PROP_FRAME_HEIGHT, CAMERA_RES_HEIGHT);
   } catch(cv::Exception &e) {
        cerr << e.msg << endl;
    }

    servos->setRange(PAN_SERVO, PAN_LO_PULSE, PAN_HI_PULSE);
    servos->center(PAN_SERVO);
    servos->setRange(TILT_SERVO, TILT_LO_PULSE, TILT_HI_PULSE);
    servos->center(TILT_SERVO);

    _cascade.load("/usr/share/opencv4/haarcascades/haarcascade_frontalface_default.xml");

    _running = true;
    pthread_mutex_init(&_mutex, NULL);
    pthread_cond_init(&_cond, NULL);
    pthread_create(&_thread, NULL, Camera::thread_func, this);

    printf("Camera is online\n");
}

Camera::~Camera()
{
    _running = false;
    pthread_cond_broadcast(&_cond);
    pthread_join(_thread, NULL);
    pthread_mutex_destroy(&_mutex);
    pthread_cond_destroy(&_cond);

    servos->clearMotionSchedule(PAN_SERVO);
    servos->clearMotionSchedule(TILT_SERVO);

    if (_vc) {
        delete _vc;
        _vc = NULL;
    }

    instance--;
    printf("Camera is offline\n");
}

void *Camera::thread_func(void *args)
{
    Camera *camera = (Camera *) args;

    camera->run();

    return NULL;
}

void Camera::run(void)
{
    Mat screen;
    Mat osd;
    Mat frame;
    struct timeval ts, now, tdiff, tv;
    unsigned int frame_count = 0;
    vector<Point> ptFaces;
    vector<struct servo_motion> sentry_motions;
    struct servo_motion motion;

    /* Set up */
    osd.create(Size(300, CAMERA_RES_HEIGHT), CV_8UC3);
    screen.create(Size(CAMERA_RES_WIDTH + osd.cols, CAMERA_RES_HEIGHT),
                  CV_8UC3);
    gettimeofday(&now, NULL);
    tdiff.tv_sec = 1;
    tdiff.tv_usec = 0;
    timersub(&now, &tdiff, &ts);
    memcpy(&tv, &now, sizeof(struct timeval));

    /* Set up sentry motion vector */
    motion.pulse = (PAN_HI_PULSE - PAN_LO_PULSE) / 2 + PAN_LO_PULSE;
    motion.ms = 50;
    sentry_motions.push_back(motion);
    motion.pulse = PAN_HI_PULSE;
    motion.ms = 10000;
    sentry_motions.push_back(motion);
    motion.pulse = PAN_LO_PULSE;
    motion.ms = 20000;
    sentry_motions.push_back(motion);
    motion.pulse = (PAN_HI_PULSE - PAN_LO_PULSE) / 2 + PAN_LO_PULSE;
    motion.ms = 10000;
    sentry_motions.push_back(motion);

    do {
        struct timespec twait;

        /* Sentry */
        if (_sentry) {
            if (servos->hasMotionSchedule(PAN_SERVO) == false) {
                servos->scheduleMotions(PAN_SERVO, sentry_motions);
            }
        } else {
            if (servos->hasMotionSchedule(PAN_SERVO) == true) {
                servos->center(PAN_SERVO);
            }
        }

        if (!isVisionEn() && !mjpeg_streamer->hasClient("/camera")) {
            if (_vc->isOpened()) {
                _vc->release();
            }

            pthread_mutex_lock(&_mutex);
            clock_gettime(CLOCK_REALTIME, &twait);
            twait.tv_sec += 1;
            pthread_cond_timedwait(&_cond, &_mutex, &twait);
            pthread_mutex_unlock(&_mutex);
            continue;
        } else {
            if (!_vc->isOpened()) {
                _vc->open(V4L_CAPTURE_DEVICE, CAP_V4L);
                _vc->set(CAP_PROP_FRAME_WIDTH, CAMERA_RES_WIDTH);
                _vc->set(CAP_PROP_FRAME_HEIGHT, CAMERA_RES_HEIGHT);
            }
        }

        /* Read a frame from camera */
        _vc->read(frame);

        if (frame.empty()) {
            cerr << "empty frame!" << endl;
            clock_gettime(CLOCK_REALTIME, &twait);
            twait.tv_sec += 1;
            pthread_mutex_lock(&_mutex);
            pthread_cond_timedwait(&_cond, &_mutex, &twait);
            pthread_mutex_unlock(&_mutex);
            continue;
        } else {
            frame_count++;
        }

        /* Perform face detection */
        if (isVisionEn()) {
            detectFaces(frame, ptFaces);
        }

        /* Update frame rate */
        gettimeofday(&now, NULL);
        timersub(&now, &tv, &tv);
        _fr = 1.0 / (tv.tv_sec + (tv.tv_usec * 0.000001));
        memcpy(&tv, &now, sizeof(struct timeval));

        /* Update OSD */
        timersub(&now, &ts, &tdiff);
        if (tdiff.tv_sec > 0 || tdiff.tv_usec > 500000) {
            OsdCam::genOsdFrame(osd, frameRate());
            gettimeofday(&ts, NULL);
        }

        /* Compose screen */
        frame.copyTo(screen(Rect(0, 0, frame.cols, frame.rows)));
        osd.copyTo(screen(Rect(640, 0, osd.cols, osd.rows)));

        /* Publish */
        if (mjpeg_streamer->isRunning()) {
            std::vector<int> params = { IMWRITE_JPEG_QUALITY, 90, };
            vector<uchar> buff_rgb;

            imencode(".jpg", screen, buff_rgb, params);
            mjpeg_streamer->publish("/camera", string(buff_rgb.begin(),
                                                      buff_rgb.end()));
        }

        if (!ptFaces.empty()) {
            float panDeg, tiltDeg;

            if (ptFaces[0].x > (CAMERA_RES_WIDTH / 2)) {
                panDeg = -0.5;
            } else {
                panDeg = 0.5;
            }
            if (ptFaces[0].y > (CAMERA_RES_HEIGHT / 2)) {
                tiltDeg = 0.5;
            } else {
                tiltDeg = -0.5;
            }

            pan(panDeg, true);
            tilt(tiltDeg, true);

            ptFaces.clear();
        }
    } while (_running);
}

void Camera::enVision(bool enable)
{
    enable = (enable ? true : false);

    if (_vision != enable) {
        _vision = enable;
        if (enable) {
            speech->speak("Camera vision enabled");
            LOG("Camera vision enabled\n");
        } else {
            speech->speak("Camera vision disabled");
            LOG("Camera vision disabled\n");
        }
    }
}

void Camera::detectFaces(Mat &frame, vector<Point> &ptFaces)
{
    vector<Rect> faces;
    Mat gray, smallImg;
    double scale = 4;
    double fx = 1 / scale;

    cvtColor(frame, gray, COLOR_BGR2GRAY);
    resize(gray, smallImg, Size(), fx, fx, INTER_LINEAR);
    equalizeHist(smallImg, smallImg);
    _cascade.detectMultiScale(smallImg, faces, 1.1, 2, CASCADE_SCALE_IMAGE,
                              Size(5, 5), Size(480, 480));

    ptFaces.clear();
    for (size_t i = 0; i < faces.size(); i++) {
        Rect r = faces[i];
        Point center;
        Scalar color = Scalar(255, 0, 0);
        int radius;

        double aspect_ratio = (double) r.width / r.height;
        if (0.75 < aspect_ratio && aspect_ratio < 1.3) {
            center.x = cvRound((r.x + r.width * 0.5) * scale);
            center.y = cvRound((r.y + r.height * 0.5) * scale);
            radius = cvRound((r.width + r.height) * 0.25 * scale);
            circle(frame, center, radius, color, 3, 8, 0 );

            ptFaces.push_back(center);
        } else {
            rectangle(frame, Point(cvRound(r.x * scale),
                                   cvRound(r.y * scale)),
                      Point(cvRound((r.x + r.width - 1) * scale),
                            cvRound((r.y + r.height - 1) * scale)),
                      color, 3, 8, 0);
        }
    }
}

void Camera::enSentry(bool enable)
{
    enable = (enable ? true : false);

    if (_sentry != enable) {
        _sentry = enable;
        if (enable) {
            speech->speak("Camera sentry mode enabled");
            LOG("Camera sentry mode enabled\n");
        } else {
            speech->speak("Camera sentry mode disabled");
            LOG("Camera sentry mode disabled\n");
        }
    }
}

void Camera::pan(float deg, bool relative)
{
    unsigned int pulse;
    unsigned int center;
    char buf[128];

    if (relative) {
        pulse = servos->pulse(PAN_SERVO);
        pulse = (unsigned int) ((float) pulse + (PAN_ANGLE_MULT * deg));
        servos->setPulse(PAN_SERVO, pulse);
    } else {
        center =
            ((servos->hiRange(PAN_SERVO) - servos->loRange(PAN_SERVO)) / 2) +
            PAN_LO_PULSE;
        pulse = (unsigned int) ((float) center - deg * PAN_ANGLE_MULT);
        servos->setPulse(PAN_SERVO, pulse);
    }

    snprintf(buf, sizeof(buf) - 1, "Camera pan to %.1f\n", panAt());
    LOG(buf);
}

void Camera::tilt(float deg, bool relative)
{
    unsigned int pulse;
    unsigned int center;
    char buf[128];

    if (relative) {
        pulse = servos->pulse(TILT_SERVO);
        pulse = (unsigned int) ((float) pulse + (TILT_ANGLE_MULT * deg));
        servos->setPulse(TILT_SERVO, pulse);
    } else {
        center =
            ((servos->hiRange(TILT_SERVO) - servos->loRange(TILT_SERVO)) / 2) +
            TILT_LO_PULSE;
        pulse = (unsigned int) ((float) center - deg * TILT_ANGLE_MULT);
        servos->setPulse(TILT_SERVO, pulse);
    }

    snprintf(buf, sizeof(buf) - 1, "Camera tilt to %.1f\n", tiltAt());
    LOG(buf);
}

float Camera::panAt(void) const
{
    unsigned int pulse;
    unsigned int center;

    pulse = servos->pulse(PAN_SERVO);
    center = ((servos->hiRange(PAN_SERVO) - servos->loRange(PAN_SERVO)) / 2) +
        PAN_LO_PULSE;

    return ((float) pulse - (float) center) / PAN_ANGLE_MULT * -1.0;
}

float Camera::tiltAt(void) const
{
    unsigned int pulse;
    unsigned int center;

    pulse = servos->pulse(TILT_SERVO);
    center = ((servos->hiRange(TILT_SERVO) - servos->loRange(TILT_SERVO)) / 2) +
        TILT_LO_PULSE;

    return ((float) pulse - (float) center) / TILT_ANGLE_MULT * -1.0;
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
