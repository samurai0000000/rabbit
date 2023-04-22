/*
 * camera.cxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <time.h>
#include <iostream>
#include "rabbit.hxx"

#define CAMERA_RES_WIDTH   640
#define CAMERA_RES_HEIGHT  480
#define PAN_SERVO          2
#define TILT_SERVO         3

Camera::Camera()
    : _vc(NULL),
      _running(0),
      _vision(0),
      _pan(0),
      _tilt(0),
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

    servos->setRange(PAN_SERVO, 640, 2180);
    servos->center(PAN_SERVO);
    servos->setRange(TILT_SERVO, 800, 1550);
    servos->center(TILT_SERVO);

    _running = 1;
    pthread_create(&_thread, NULL, Camera::thread_func, this);

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
    struct timeval ts, now, tdiff, tv;
    struct wifi_stat wifi_stat;

    /* Set up */
    _vc->read(frame);
    osd1.create(Size(300, frame.rows), frame.type());
    screen.create(Size(frame.cols + osd1.cols,
                       frame.rows),
                  frame.type());
    textSize = getTextSize("Xquick", fontFace, fontScale, thickness, &baseline);
    textSize.height += 3;
    gettimeofday(&now, NULL);
    tdiff.tv_sec = 1;
    tdiff.tv_usec = 0;
    timersub(&now, &tdiff, &ts);
    memcpy(&tv, &now, sizeof(struct timeval));

    do {

        if (!isVisionEn() && !_streamer.hasClient("/rgb")) {
            if (_vc->isOpened()) {
                _vc->release();
            }
            usleep(50000);
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
        }

        /* Update frame rate */
        gettimeofday(&now, NULL);
        timersub(&now, &tv, &tv);
        _fr = 1.0 / (tv.tv_sec + (tv.tv_usec * 0.000001));
        memcpy(&tv, &now, sizeof(struct timeval));

        timersub(&now, &ts, &tdiff);
        if (tdiff.tv_sec > 0 || tdiff.tv_usec > 500000) {
            char buf[128];
            time_t tt;
            struct tm *tm;

            osd1.setTo(Scalar::all(0));
            pos = Point(0, 0);

            tt = time(NULL);
            tm = localtime(&tt);
            strftime(buf, sizeof(buf) - 1, "%Y-%m-%d %H:%M:%S %Z", tm);
            text = buf;
            pos.y += textSize.height;
            putText(osd1, text, pos,
                    fontFace, fontScale, fontColor, thickness, LINE_8, false);

            text = String("Batt Voltage: ") + to_string(power->voltage()) +
                String("V");
            pos.y += textSize.height;
            putText(osd1, text, pos,
                    fontFace, fontScale, fontColor, thickness, LINE_8, false);

            text = String("Batt Current: ") + to_string(power->current()) +
                String("A");
            pos.y += textSize.height;
            putText(osd1, text, pos,
                    fontFace, fontScale, fontColor, thickness, LINE_8, false);

            text = String("Heading: ") + to_string(compass->azimuth()) +
                String(" deg");
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
                servos->setPulse(PAN_SERVO, pulse + 5);
            } else {
                servos->setPulse(PAN_SERVO, pulse - 5);
            }
        }
    } while (_running);
}

void Camera::pan(int deg, bool relative)
{
    // TODO
    (void)(deg);
    (void)(relative);
}

void Camera::tilt(int deg, bool relative)
{
    // TODO
    (void)(deg);
    (void)(relative);
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
