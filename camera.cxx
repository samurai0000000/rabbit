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

#define CAMERA_RES_WIDTH    640
#define CAMERA_RES_HEIGHT   480
#define PAN_SERVO             2
#define PAN_LO_PULSE        640
#define PAN_HI_PULSE       2180
#define PAN_ANGLE_MULT       14
#define TILT_SERVO            3
#define TILT_LO_PULSE       800
#define TILT_HI_PULSE      1550
#define TILT_ANGLE_MULT      14

#ifndef FSHIFT
# define FSHIFT 16         /* nr of bits of precision */
#endif
#define FIXED_1            (1 << FSHIFT)     /* 1.0 as fixed-point */
#define LOAD_INT(x)        (unsigned int)((x) >> FSHIFT)
#define LOAD_FRAC(x)       LOAD_INT(((x) & (FIXED_1 - 1)) * 100)

Camera::Camera()
    : _vc(NULL),
      _running(false),
      _vision(0),
      _fr(0.0),
      _sentry()
{
    try {
        _vc = new VideoCapture(0, CAP_V4L);
        _vc->set(CAP_PROP_FRAME_WIDTH, CAMERA_RES_WIDTH);
        _vc->set(CAP_PROP_FRAME_HEIGHT, CAMERA_RES_HEIGHT);
   } catch(cv::Exception &e) {
        cerr << e.msg << endl;
        return;
    }

    servos->setRange(PAN_SERVO, PAN_LO_PULSE, PAN_HI_PULSE);
    servos->center(PAN_SERVO);
    servos->setRange(TILT_SERVO, TILT_LO_PULSE, TILT_HI_PULSE);
    servos->center(TILT_SERVO);

    _running = true;
    pthread_mutex_init(&_mutex, NULL);
    pthread_cond_init(&_cond, NULL);
    pthread_create(&_thread, NULL, Camera::thread_func, this);

    _streamer.start(8000);
}

Camera::~Camera()
{
    _running = false;
    pthread_cond_broadcast(&_cond);
    pthread_join(_thread, NULL);
    pthread_mutex_destroy(&_mutex);
    pthread_cond_destroy(&_cond);

    _streamer.stop();

    if (_vc) {
        delete _vc;
    }
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
    String text;
    Point pos;
    struct timeval since, ts, now, tdiff, tv;
    struct wifi_stat wifi_stat;
    unsigned int frame_count = 0;

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

    do {
        if (!isVisionEn() && !_streamer.hasClient("/rgb")) {
            struct timespec twait;
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
                _vc->open(0, CAP_V4L);
                _vc->set(CAP_PROP_FRAME_WIDTH, CAMERA_RES_WIDTH);
                _vc->set(CAP_PROP_FRAME_HEIGHT, CAMERA_RES_HEIGHT);
            }
        }

        /* Read a frame from camera */
        _vc->read(frame);

        if (frame.empty()) {
            cerr << "empty frame!" << endl;
            continue;
        } else {
            frame_count++;
        }

        /* Update frame rate */
        gettimeofday(&now, NULL);
        timersub(&now, &tv, &tv);
        _fr = 1.0 / (tv.tv_sec + (tv.tv_usec * 0.000001));
        memcpy(&tv, &now, sizeof(struct timeval));

        timersub(&now, &ts, &tdiff);
        if (tdiff.tv_sec > 0 || tdiff.tv_usec > 500000) {
            struct sysinfo info;
            unsigned int updays, uphours, upminutes, upseconds;
            char buf[128];
            time_t tt;
            struct tm *tm;

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

            timersub(&now, &since, &tdiff);
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

            snprintf(buf, sizeof(buf) - 1, "%.1f", compass->bearing());
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

            text = String("");
            text += "Pan: " + to_string(panAt()) + " deg ";
            text += "Tilt: " + to_string(tiltAt()) + " deg";
            pos.y += textSize.height;
            putText(osd1, text, pos,
                    fontFace, fontScale, fontColor, thickness, LINE_8, false);

            text = String("Wheels: ") + wheels->stateStr();
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

            gettimeofday(&ts, NULL);
        }

        /* Compose screen */
        frame.copyTo(screen(Rect(0, 0, frame.cols, frame.rows)));
        osd1.copyTo(screen(Rect(640, 0, osd1.cols, osd1.rows)));

        if (_streamer.isRunning()) {
            vector<uchar> buff_rgb;

            imencode(".jpg", screen, buff_rgb, params);
            _streamer.publish("/rgb", string(buff_rgb.begin(),
                                                     buff_rgb.end()));
        }

        /* Sentry */
        if (_sentry.enabled) {
            unsigned int pulse;

            pulse = servos->pulse(PAN_SERVO);
            if (pulse >= servos->hiRange(PAN_SERVO)) {
                _sentry.dir = 0;
            } else if (pulse <= servos->loRange(PAN_SERVO)) {
                _sentry.dir = 1;
            }

            if (_sentry.dir) {
                servos->setPulse(PAN_SERVO,
                                 servos->pulse(PAN_SERVO) + PAN_ANGLE_MULT);
            } else {
                servos->setPulse(PAN_SERVO,
                                 servos->pulse(PAN_SERVO) - PAN_ANGLE_MULT);
            }
        }
    } while (_running);
}

void Camera::pan(int deg, bool relative)
{
    unsigned int pulse;
    unsigned int center;

    if (relative) {
        pulse = servos->pulse(PAN_SERVO);
        pulse = (unsigned) ((int) pulse + (PAN_ANGLE_MULT * deg));
        servos->setPulse(PAN_SERVO, pulse);
    } else {
        center =
            ((servos->hiRange(PAN_SERVO) - servos->loRange(PAN_SERVO)) / 2) +
            PAN_LO_PULSE;
        pulse = (unsigned int) ((int) center + deg * PAN_ANGLE_MULT);
        servos->setPulse(PAN_SERVO, pulse);
    }
}

void Camera::tilt(int deg, bool relative)
{
    unsigned int pulse;
    unsigned int center;

    if (relative) {
        pulse = servos->pulse(TILT_SERVO);
        pulse = (unsigned) ((int) pulse + (TILT_ANGLE_MULT * deg));
        servos->setPulse(TILT_SERVO, pulse);
    } else {
        center =
            ((servos->hiRange(TILT_SERVO) - servos->loRange(TILT_SERVO)) / 2) +
            TILT_LO_PULSE;
        pulse = (unsigned int) ((int) center + deg * TILT_ANGLE_MULT);
        servos->setPulse(TILT_SERVO, pulse);
    }
}

int Camera::panAt(void) const
{
    unsigned int pulse;
    unsigned int center;

    pulse = servos->pulse(PAN_SERVO);
    center = ((servos->hiRange(PAN_SERVO) - servos->loRange(PAN_SERVO)) / 2) +
        PAN_LO_PULSE;

    return ((int) pulse - (int) center) / PAN_ANGLE_MULT;
}

int Camera::tiltAt(void) const
{
    unsigned int pulse;
    unsigned int center;

    pulse = servos->pulse(TILT_SERVO);
    center = ((servos->hiRange(TILT_SERVO) - servos->loRange(TILT_SERVO)) / 2) +
        TILT_LO_PULSE;

    return ((int) pulse - (int) center) / TILT_ANGLE_MULT;
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
