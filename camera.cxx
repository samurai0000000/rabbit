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

Camera::Camera()
    : _vc(NULL),
      _running(0),
      _vision(0),
      _pan(0),
      _tilt(0)
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
    struct timeval ts, now, tdiff;
    struct wifi_stat wifi_stat;

    /* Set up */
    camera->_vc->read(frame);
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

    do {

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

        /* Read a frame from camera */
        camera->_vc->read(frame);

        if (frame.empty()) {
            cerr << "empty frame!" << endl;
            continue;
        }

        gettimeofday(&now, NULL);
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

            text = String("");
            text += "Pan: " + to_string(camera->panAt()) + " deg ";
            text += "Tilt: " + to_string(camera->tiltAt()) + " deg";
            pos.y += textSize.height;
            putText(osd1, text, pos,
                    fontFace, fontScale, fontColor, thickness, LINE_8, false);

            text = String("Wheels: ") + wheels->stateStr();
            pos.y += textSize.height;
            putText(osd1, text, pos,
                    fontFace, fontScale, fontColor, thickness, LINE_8, false);

            text = String("Temperature: ") + to_string(ambience->temp()) +
                String("C");
            pos.y += textSize.height;
            putText(osd1, text, pos,
                    fontFace, fontScale, fontColor, thickness, LINE_8, false);

            text = String("Humidity: ") + to_string(ambience->humidity()) +
                String("%");
            pos.y += textSize.height;
            putText(osd1, text, pos,
                    fontFace, fontScale, fontColor, thickness, LINE_8, false);

            text = String("Barometric Pressure: ") +
                to_string(ambience->pressure()) + String("kPa");
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

        if (camera->_streamer.isRunning()) {
            vector<uchar> buff_rgb;

            imencode(".jpg", screen, buff_rgb, params);
            camera->_streamer.publish("/rgb", string(buff_rgb.begin(),
                                                     buff_rgb.end()));
        }
    } while (camera->_running);

    return NULL;
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
