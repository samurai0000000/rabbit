/*
 * camera.cxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <time.h>
#include <sys/time.h>
#include <sys/sysinfo.h>
#include <iostream>
#include "rabbit.hxx"

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

#ifndef FSHIFT
# define FSHIFT 16         /* nr of bits of precision */
#endif
#define FIXED_1            (1 << FSHIFT)     /* 1.0 as fixed-point */
#define LOAD_INT(x)        (unsigned int)((x) >> FSHIFT)
#define LOAD_FRAC(x)       LOAD_INT(((x) & (FIXED_1 - 1)) * 100)

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
    std::vector<int> params = { IMWRITE_JPEG_QUALITY, 90, };
    Mat screen;
    Mat osd1;
    Mat frame;
    int fontFace = FONT_HERSHEY_PLAIN;
    double fontScale = 1;
    int thickness = 1;
    int baseline = 0;
    Scalar fontColor(255, 255, 255);
    Size textSize;
    struct timeval since, ts, now, tdiff, tv;
    unsigned int frame_count = 0;
    vector<Point> ptFaces;
    vector<struct servo_motion> sentry_motions;
    struct servo_motion motion;

    /* Set up */
    _vc->read(frame);
    osd1.create(Size(300, frame.rows), frame.type());
    screen.create(Size(frame.cols + osd1.cols,
                       frame.rows),
                  frame.type());
    textSize = getTextSize("Xquick", fontFace, fontScale, thickness, &baseline);
    textSize.height += 3;
    gettimeofday(&now, NULL);
    memcpy(&since, &now, sizeof(struct timeval));
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

        if (!isVisionEn() && !mjpeg_streamer->hasClient("/rgb")) {
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
            pthread_mutex_lock(&_mutex);
            clock_gettime(CLOCK_REALTIME, &twait);
            twait.tv_sec += 1;
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

        timersub(&now, &ts, &tdiff);
        if (tdiff.tv_sec > 0 || tdiff.tv_usec > 500000) {
            updateOsd1(osd1, &since,
                       fontFace, fontScale, thickness,
                       fontColor, textSize);
            gettimeofday(&ts, NULL);
        }

        /* Compose screen */
        frame.copyTo(screen(Rect(0, 0, frame.cols, frame.rows)));
        osd1.copyTo(screen(Rect(640, 0, osd1.cols, osd1.rows)));

        if (mjpeg_streamer->isRunning()) {
            vector<uchar> buff_rgb;

            imencode(".jpg", screen, buff_rgb, params);
            mjpeg_streamer->publish("/rgb", string(buff_rgb.begin(),
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

void Camera::updateOsd1(Mat &osd1, const struct timeval *since,
                        int fontFace, double fontScale, int thickness,
                        const Scalar &fontColor, const Size &textSize)
{
    String text;
    Point pos;
    struct sysinfo info;
    struct timeval now, tdiff;
    unsigned int updays, uphours, upminutes, upseconds;
    struct wifi_stat wifi_stat;
    char buf[128];
    time_t tt;
    struct tm *tm;

    gettimeofday(&now, NULL);

    osd1.setTo(Scalar::all(0));
    pos = Point(0, 0);

    text = String("Charlotte's Rabbit Robotic System");
    pos.y += textSize.height;
    putText(osd1, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    tt = time(NULL);
    tm = localtime(&tt);
    strftime(buf, sizeof(buf) - 1, "%Y-%m-%d %H:%M:%S %Z", tm);
    text = buf;
    pos.y += textSize.height;
    putText(osd1, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    sysinfo(&info);
    updays = info.uptime / (60 * 60 * 24);
    upminutes = info.uptime / 60;
    uphours = (upminutes / 60) % 24;
    upminutes %= 60;
    upseconds = info.uptime % 60;
    if (updays != 0) {
        snprintf(buf, sizeof(buf) - 1, "%ud %.2u:%.2u:%.2u",
                 updays, uphours, upminutes, upseconds);
    } else {
        snprintf(buf, sizeof(buf) - 1, "%.2u:%.2u:%.2u",
                 uphours, upminutes, upseconds);
    }

    text = String("System Uptime: ") + buf;
    pos.y += textSize.height;
    putText(osd1, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    timersub(&now, since, &tdiff);
    updays = tdiff.tv_sec / (60 * 60 * 24);
    upminutes = tdiff.tv_sec / 60;
    uphours = (upminutes / 60) % 24;
    upminutes %= 60;
    upseconds = tdiff.tv_sec % 60;
    if (updays != 0) {
        snprintf(buf, sizeof(buf) - 1, "%ud %.2u:%.2u:%.2u",
                 updays, uphours, upminutes, upseconds);
    } else {
        snprintf(buf, sizeof(buf) - 1, "%.2u:%.2u:%.2u",
                 uphours, upminutes, upseconds);
    }
    text = String("Rabbit Uptime: ") + buf;
    pos.y += textSize.height;
    putText(osd1, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    snprintf(buf, sizeof(buf) - 1, "%u.%.2u, %u.%.2u %u.%.2u",
             LOAD_INT(info.loads[0]), LOAD_FRAC(info.loads[0]),
             LOAD_INT(info.loads[1]), LOAD_FRAC(info.loads[1]),
             LOAD_INT(info.loads[2]), LOAD_FRAC(info.loads[2]));

    text = String("Load averages: ") + buf;
    pos.y += textSize.height;
    putText(osd1, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    snprintf(buf, sizeof(buf) - 1, "%.2f", power->voltage());
    text = String("Batt Voltage: ") + buf + String("V");
    pos.y += textSize.height;
    putText(osd1, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    snprintf(buf, sizeof(buf) - 1, "%.2f", power->current());
    text = String("Batt Current: ") + buf + String("A");
    pos.y += textSize.height;
    putText(osd1, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    snprintf(buf, sizeof(buf) - 1, "%.1f", compass->heading());
    text = String("Heading: ") + buf + String(" deg");
    pos.y += textSize.height;
    putText(osd1, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    text = String("Frame Rate: ");
    snprintf(buf, sizeof(buf) - 1, "%.2f", frameRate());
    text += buf;
    pos.y += textSize.height;
    putText(osd1, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    snprintf(buf, sizeof(buf) - 1, "%.1f", this->panAt());
    text = String("Camera Pan: ") + buf + String(" deg ");
    pos.y += textSize.height;
    putText(osd1, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    snprintf(buf, sizeof(buf) - 1, "%.1f", tiltAt());
    text = String("Camera Tilt: ") + buf + String(" deg");
    pos.y += textSize.height;
    putText(osd1, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    snprintf(buf, sizeof(buf) - 1, "%.1f", head->rotationAt());
    text = String("Head Rotation: ") + buf + String(" deg ");
    pos.y += textSize.height;
    putText(osd1, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    snprintf(buf, sizeof(buf) - 1, "%.1f", head->tiltAt());
    text = String("Head Tilt: ") + buf + String(" deg");
    pos.y += textSize.height;
    putText(osd1, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    text = String("LiDAR: ") + (lidar->isEnabled() ? "on" : "off");
    pos.y += textSize.height;
    putText(osd1, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    text = String("LiDAR RPM: ") +
        (lidar->isEnabled() ? to_string(lidar->rpm()) : "off");
    pos.y += textSize.height;
    putText(osd1, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    text = String("LiDAR PPS: ") +
        (lidar->isEnabled() ? to_string(lidar->pps()) : "off");
    pos.y += textSize.height;
    putText(osd1, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    text = String("Wheels: ") + wheels->stateStr();
    pos.y += textSize.height;
    putText(osd1, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    snprintf(buf, sizeof(buf) - 1, "%.1f %.1f",
             rightArm->shoulderRotation(),
             rightArm->shoulderExtension());
    text = String("Right Shoulder: ") + buf;
    pos.y += textSize.height;
    putText(osd1, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    snprintf(buf, sizeof(buf) - 1, "%.1f",
             rightArm->elbowExtension());
    text = String("Right Elbow: ") + buf;
    pos.y += textSize.height;
    putText(osd1, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    snprintf(buf, sizeof(buf) - 1, "%.1f %.1f",
             rightArm->wristExtension(),
             rightArm->wristRotation());
    text = String("Right Wrist: ") + buf;
    pos.y += textSize.height;
    putText(osd1, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    snprintf(buf, sizeof(buf) - 1, "%.1f",
             rightArm->gripperPosition());
    text = String("Right Gripper: ") + buf;
    pos.y += textSize.height;
    putText(osd1, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    snprintf(buf, sizeof(buf) - 1, "%.1f %.1f",
             leftArm->shoulderRotation(),
             leftArm->shoulderExtension());
    text = String("Left Shoulder: ") + buf;
    pos.y += textSize.height;
    putText(osd1, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    snprintf(buf, sizeof(buf) - 1, "%.1f",
             leftArm->elbowExtension());
    text = String("Left Elbow: ") + buf;
    pos.y += textSize.height;
    putText(osd1, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    snprintf(buf, sizeof(buf) - 1, "%.1f %.1f",
             leftArm->wristExtension(),
             leftArm->wristRotation());
    text = String("Left Wrist: ") + buf;
    pos.y += textSize.height;
    putText(osd1, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    snprintf(buf, sizeof(buf) - 1, "%.1f",
             leftArm->gripperPosition());
    text = String("Left Gripper: ") + buf;
    pos.y += textSize.height;
    putText(osd1, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    snprintf(buf, sizeof(buf) - 1, "%.2f", ambience->temp());
    text = String("Temperature: ") + buf + String("C");
    pos.y += textSize.height;
    putText(osd1, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    snprintf(buf, sizeof(buf) - 1, "%.2f", ambience->humidity());
    text = String("Humidity: ") + buf + String("%");
    pos.y += textSize.height;
    putText(osd1, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    snprintf(buf, sizeof(buf) - 1, "%.2f", ambience->pressure());
    text = String("Barometric Pressure: ") + buf + String("hPa");
    pos.y += textSize.height;
    putText(osd1, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    WIFI::stat(&wifi_stat);

    text = String("AP: ") + wifi_stat.ap;
    pos.y += textSize.height;
    putText(osd1, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    text = String("ESSID: ") + wifi_stat.essid;
    pos.y += textSize.height;
    putText(osd1, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    text = String();
    text += "Chan: " + to_string(wifi_stat.chan) + " ";
    text += "Link: " + to_string(wifi_stat.link_quality) + "% ";
    text += "Signal: " + to_string(wifi_stat.signal_level) + "dBm";
    pos.y += textSize.height;
    putText(osd1, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);
    snprintf(buf, sizeof(buf) - 1, "%.2f", ambience->socTemp());
    text = String("SoC Temp: ") + buf + String("C");
    pos.y += textSize.height;
    putText(osd1, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    text = String("MQTT pub: ") +
        to_string(mosquitto->publishConfirmed());
    pos.y += textSize.height;
    putText(osd1, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    text = String("MQTT msg: ") +
        to_string(mosquitto->messaged());
    pos.y += textSize.height;
    putText(osd1, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);
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

bool Camera::isVisionEn(void) const
{
    return _vision;
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

bool Camera::isSentryEn(void) const
{
    return _sentry;
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

float Camera::frameRate(void) const
{
    return _fr;
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
