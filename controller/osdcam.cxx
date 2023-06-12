/*
 * osdcam.cxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <time.h>
#include <sys/time.h>
#include <bsd/sys/time.h>
#include <sys/sysinfo.h>
#include <pthread.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include "osdcam.hxx"
#include "rabbit.hxx"

#ifndef FSHIFT
# define FSHIFT 16         /* nr of bits of precision */
#endif
#define FIXED_1            (1 << FSHIFT)     /* 1.0 as fixed-point */
#define LOAD_INT(x)        (unsigned int)((x) >> FSHIFT)
#define LOAD_FRAC(x)       LOAD_INT(((x) & (FIXED_1 - 1)) * 100)

using namespace std;
using namespace cv;

static const int fontFace = FONT_HERSHEY_PLAIN;
static const double fontScale = 1.0;
static const int thickness = 1;
static int baseline = 0;
static Size textSize;
static const Scalar fontColor(255, 255, 255);
static struct timeval since;

void OsdCam::initialize(void)
{
    static bool done_setup = false;

    if (done_setup) {
        return;
    }

    gettimeofday(&since, NULL);
    textSize = getTextSize("Xquick", fontFace, fontScale, thickness, &baseline);
    textSize.height += 3;

    done_setup = true;
}

void OsdCam::genOsdFrame(cv::Mat &osdFrame, float videoFrameRate)
{
    initialize();

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

    osdFrame.setTo(Scalar::all(0));
    pos = Point(0, 0);

    text = String("Charlotte's Rabbit Robotic System");
    pos.y += textSize.height;
    putText(osdFrame, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    tt = time(NULL);
    tm = localtime(&tt);
    strftime(buf, sizeof(buf) - 1, "%Y-%m-%d %H:%M:%S %Z", tm);
    text = buf;
    pos.y += textSize.height;
    putText(osdFrame, text, pos,
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
    putText(osdFrame, text, pos,
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
    putText(osdFrame, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    snprintf(buf, sizeof(buf) - 1, "%u.%.2u, %u.%.2u %u.%.2u",
             LOAD_INT(info.loads[0]), LOAD_FRAC(info.loads[0]),
             LOAD_INT(info.loads[1]), LOAD_FRAC(info.loads[1]),
             LOAD_INT(info.loads[2]), LOAD_FRAC(info.loads[2]));

    text = String("Load averages: ") + buf;
    pos.y += textSize.height;
    putText(osdFrame, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    snprintf(buf, sizeof(buf) - 1, "%.2f", power->voltage());
    text = String("Batt Voltage: ") + buf + String("V");
    pos.y += textSize.height;
    putText(osdFrame, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    snprintf(buf, sizeof(buf) - 1, "%.2f", power->current());
    text = String("Batt Current: ") + buf + String("A");
    pos.y += textSize.height;
    putText(osdFrame, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    snprintf(buf, sizeof(buf) - 1, "%.1f", compass->heading());
    text = String("Heading: ") + buf + String(" deg");
    pos.y += textSize.height;
    putText(osdFrame, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    text = String("Frame Rate: ");
    snprintf(buf, sizeof(buf) - 1, "%.2f", videoFrameRate);
    text += buf;
    pos.y += textSize.height;
    putText(osdFrame, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    snprintf(buf, sizeof(buf) - 1, "%.1f", camera->panAt());
    text = String("Camera Pan: ") + buf + String(" deg ");
    pos.y += textSize.height;
    putText(osdFrame, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    snprintf(buf, sizeof(buf) - 1, "%.1f", camera->tiltAt());
    text = String("Camera Tilt: ") + buf + String(" deg");
    pos.y += textSize.height;
    putText(osdFrame, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    snprintf(buf, sizeof(buf) - 1, "%.1f", head->rotationAt());
    text = String("Head Rotation: ") + buf + String(" deg ");
    pos.y += textSize.height;
    putText(osdFrame, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    snprintf(buf, sizeof(buf) - 1, "%.1f", head->tiltAt());
    text = String("Head Tilt: ") + buf + String(" deg");
    pos.y += textSize.height;
    putText(osdFrame, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    text = String("LiDAR: ") + (lidar->isEnabled() ? "on" : "off");
    pos.y += textSize.height;
    putText(osdFrame, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    text = String("LiDAR RPM: ") +
        (lidar->isEnabled() ? to_string(lidar->rpm()) : "off");
    pos.y += textSize.height;
    putText(osdFrame, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    text = String("LiDAR PPS: ") +
        (lidar->isEnabled() ? to_string(lidar->pps()) : "off");
    pos.y += textSize.height;
    putText(osdFrame, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    text = String("Wheels: ") + wheels->stateStr();
    pos.y += textSize.height;
    putText(osdFrame, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    snprintf(buf, sizeof(buf) - 1, "%.1f %.1f",
             rightArm->shoulderRotation(),
             rightArm->shoulderExtension());
    text = String("Right Shoulder: ") + buf;
    pos.y += textSize.height;
    putText(osdFrame, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    snprintf(buf, sizeof(buf) - 1, "%.1f",
             rightArm->elbowExtension());
    text = String("Right Elbow: ") + buf;
    pos.y += textSize.height;
    putText(osdFrame, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    snprintf(buf, sizeof(buf) - 1, "%.1f %.1f",
             rightArm->wristExtension(),
             rightArm->wristRotation());
    text = String("Right Wrist: ") + buf;
    pos.y += textSize.height;
    putText(osdFrame, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    snprintf(buf, sizeof(buf) - 1, "%.1f",
             rightArm->gripperPosition());
    text = String("Right Gripper: ") + buf;
    pos.y += textSize.height;
    putText(osdFrame, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    snprintf(buf, sizeof(buf) - 1, "%.1f %.1f",
             leftArm->shoulderRotation(),
             leftArm->shoulderExtension());
    text = String("Left Shoulder: ") + buf;
    pos.y += textSize.height;
    putText(osdFrame, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    snprintf(buf, sizeof(buf) - 1, "%.1f",
             leftArm->elbowExtension());
    text = String("Left Elbow: ") + buf;
    pos.y += textSize.height;
    putText(osdFrame, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    snprintf(buf, sizeof(buf) - 1, "%.1f %.1f",
             leftArm->wristExtension(),
             leftArm->wristRotation());
    text = String("Left Wrist: ") + buf;
    pos.y += textSize.height;
    putText(osdFrame, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    snprintf(buf, sizeof(buf) - 1, "%.1f",
             leftArm->gripperPosition());
    text = String("Left Gripper: ") + buf;
    pos.y += textSize.height;
    putText(osdFrame, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    snprintf(buf, sizeof(buf) - 1, "%.2f", ambience->temp());
    text = String("Temperature: ") + buf + String("C");
    pos.y += textSize.height;
    putText(osdFrame, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    snprintf(buf, sizeof(buf) - 1, "%.2f", ambience->humidity());
    text = String("Humidity: ") + buf + String("%");
    pos.y += textSize.height;
    putText(osdFrame, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    snprintf(buf, sizeof(buf) - 1, "%.2f", ambience->pressure());
    text = String("Barometric Pressure: ") + buf + String("hPa");
    pos.y += textSize.height;
    putText(osdFrame, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    WIFI::stat(&wifi_stat);

    text = String("AP: ") + wifi_stat.ap;
    pos.y += textSize.height;
    putText(osdFrame, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    text = String("ESSID: ") + wifi_stat.essid;
    pos.y += textSize.height;
    putText(osdFrame, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    text = String();
    text += "Chan: " + to_string(wifi_stat.chan) + " ";
    text += "Link: " + to_string(wifi_stat.link_quality) + "% ";
    text += "Signal: " + to_string(wifi_stat.signal_level) + "dBm";
    pos.y += textSize.height;
    putText(osdFrame, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);
    snprintf(buf, sizeof(buf) - 1, "%.2f", ambience->socTemp());
    text = String("SoC Temp: ") + buf + String("C");
    pos.y += textSize.height;
    putText(osdFrame, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    text = String("MQTT pub: ") +
        to_string(mosquitto->publishConfirmed());
    pos.y += textSize.height;
    putText(osdFrame, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);

    text = String("MQTT msg: ") +
        to_string(mosquitto->messaged());
    pos.y += textSize.height;
    putText(osdFrame, text, pos,
            fontFace, fontScale, fontColor, thickness, LINE_8, false);
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
