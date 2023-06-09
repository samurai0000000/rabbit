/*
 * keycontrol.cxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <string>
#include "rabbit.hxx"

using namespace std;

#define WHEEL_DRIVE_LIMIT_MS  100

static unsigned int chan = 12;

enum rabbit_console_mode {
    RABBIT_CONSOLE_MODE_CAMERA = 0,
    RABBIT_CONSOLE_MODE_WHEEL = 1,
    RABBIT_CONSOLE_MODE_R_SHOULDER = 2,
    RABBIT_CONSOLE_MODE_R_ELBOW = 3,
    RABBIT_CONSOLE_MODE_R_WRIST = 4,
    RABBIT_CONSOLE_MODE_R_GRIPPER = 5,
    RABBIT_CONSOLE_MODE_L_SHOULDER = 6,
    RABBIT_CONSOLE_MODE_L_ELBOW = 7,
    RABBIT_CONSOLE_MODE_L_WRIST = 8,
    RABBIT_CONSOLE_MODE_L_GRIPPER = 9,
    RABBIT_CONSOLE_MODE_HEAD = 10,
    RABBIT_CONSOLE_MODE_R_EYEBROW = 11,
    RABBIT_CONSOLE_MODE_L_EYEBROW = 12,
    RABBIT_CONSOLE_MODE_R_EAR = 13,
    RABBIT_CONSOLE_MODE_L_EAR = 14,
    RABBIT_CONSOLE_MODE_R_TW = 15,
    RABBIT_CONSOLE_MODE_L_TW = 16,
};

static enum rabbit_console_mode mode = RABBIT_CONSOLE_MODE_CAMERA;

static void action_up(void)
{
    switch (mode) {
    case RABBIT_CONSOLE_MODE_CAMERA:
        camera->tilt(-1.0, true);
        break;
    case RABBIT_CONSOLE_MODE_WHEEL:
        wheels->fwd(WHEEL_DRIVE_LIMIT_MS);
        break;
    case RABBIT_CONSOLE_MODE_R_SHOULDER:
        rightArm->rotateShoulder(1.0, 5, true);
        break;
    case RABBIT_CONSOLE_MODE_R_ELBOW:
        rightArm->extendElbow(-1.0, 5, true);
        break;
    case RABBIT_CONSOLE_MODE_R_WRIST:
        rightArm->rotateWrist(1.0, 5, true);
        break;
    case RABBIT_CONSOLE_MODE_R_GRIPPER:
        rightArm->setGripperPosition(1.0, 5, true);
        break;
    case RABBIT_CONSOLE_MODE_L_SHOULDER:
        leftArm->rotateShoulder(1.0, 5, true);
        break;
    case RABBIT_CONSOLE_MODE_L_ELBOW:
        leftArm->extendElbow(1.0, 5, true);
        break;
    case RABBIT_CONSOLE_MODE_L_WRIST:
        leftArm->rotateWrist(1.0, 5, true);
        break;
    case RABBIT_CONSOLE_MODE_L_GRIPPER:
        leftArm->setGripperPosition(1.0, 5, true);
        break;
    case RABBIT_CONSOLE_MODE_HEAD:
        head->tilt(1.0, true);
        break;
    case RABBIT_CONSOLE_MODE_R_EYEBROW:
        head->eyebrowTilt(1.0, true, 0x1);
        break;
    case RABBIT_CONSOLE_MODE_L_EYEBROW:
        head->eyebrowTilt(1.0, true, 0x2);
        break;
    case RABBIT_CONSOLE_MODE_R_EAR:
        head->earTilt(1.0, true, 0x1);
        break;
    case RABBIT_CONSOLE_MODE_L_EAR:
        head->earTilt(1.0, true, 0x2);
        break;
    case RABBIT_CONSOLE_MODE_R_TW:
        head->toothRotate(1.0, true, 0x1);
        break;
    case RABBIT_CONSOLE_MODE_L_TW:
        head->toothRotate(1.0, true, 0x2);
        break;
    default:
        break;
    }
}

static void action_down(void)
{
    switch (mode) {
    case RABBIT_CONSOLE_MODE_CAMERA:
        camera->tilt(1.0, true);
        break;
    case RABBIT_CONSOLE_MODE_WHEEL:
        wheels->bwd(WHEEL_DRIVE_LIMIT_MS);
        break;
    case RABBIT_CONSOLE_MODE_R_SHOULDER:
        rightArm->rotateShoulder(-1.0, 5, true);
        break;
    case RABBIT_CONSOLE_MODE_R_ELBOW:
        rightArm->extendElbow(1.0, 5, true);
        break;
    case RABBIT_CONSOLE_MODE_R_WRIST:
        rightArm->rotateWrist(-1.0, 5, true);
        break;
    case RABBIT_CONSOLE_MODE_R_GRIPPER:
        rightArm->setGripperPosition(-1.0, 5, true);
        break;
    case RABBIT_CONSOLE_MODE_L_SHOULDER:
        leftArm->rotateShoulder(-1.0, 5, true);
        break;
    case RABBIT_CONSOLE_MODE_L_ELBOW:
        leftArm->extendElbow(-1.0, 5, true);
        break;
    case RABBIT_CONSOLE_MODE_L_WRIST:
        leftArm->rotateWrist(-1.0, 5, true);
        break;
    case RABBIT_CONSOLE_MODE_L_GRIPPER:
        leftArm->setGripperPosition(-1.0, 5, true);
        break;
    case RABBIT_CONSOLE_MODE_HEAD:
        head->tilt(-1.0, true);
        break;
    case RABBIT_CONSOLE_MODE_R_EYEBROW:
        head->eyebrowTilt(-1.0, true, 0x1);
        break;
    case RABBIT_CONSOLE_MODE_L_EYEBROW:
        head->eyebrowTilt(-1.0, true, 0x2);
        break;
    case RABBIT_CONSOLE_MODE_R_EAR:
        head->earTilt(-1.0, true, 0x1);
        break;
    case RABBIT_CONSOLE_MODE_L_EAR:
        head->earTilt(-1.0, true, 0x2);
        break;
    case RABBIT_CONSOLE_MODE_R_TW:
        head->toothRotate(-1.0, true, 0x1);
        break;
    case RABBIT_CONSOLE_MODE_L_TW:
        head->toothRotate(-1.0, true, 0x2);
        break;
    default:
        break;
    }
}

static void action_right(void)
{
    switch (mode) {
    case RABBIT_CONSOLE_MODE_CAMERA:
        camera->pan(-1, true);
        break;
    case RABBIT_CONSOLE_MODE_WHEEL:
        wheels->ror(WHEEL_DRIVE_LIMIT_MS);
        break;
    case RABBIT_CONSOLE_MODE_R_SHOULDER:
        rightArm->extendShoulder(-1.0, 5, true);
        break;
    case RABBIT_CONSOLE_MODE_R_ELBOW:
        rightArm->extendElbow(1.0, 5, true);
        break;
    case RABBIT_CONSOLE_MODE_R_WRIST:
        rightArm->extendWrist(1.0, 5, true);
        break;
    case RABBIT_CONSOLE_MODE_R_GRIPPER:
        rightArm->setGripperPosition(-1.0, 5, true);
        break;
    case RABBIT_CONSOLE_MODE_L_SHOULDER:
        leftArm->extendShoulder(-1.0, 5, true);
        break;
    case RABBIT_CONSOLE_MODE_L_ELBOW:
        leftArm->extendElbow(-1.0, 5, true);
        break;
    case RABBIT_CONSOLE_MODE_L_WRIST:
        leftArm->extendWrist(-1.0, 5, true);
        break;
    case RABBIT_CONSOLE_MODE_L_GRIPPER:
        leftArm->setGripperPosition(-1.0, 5, true);
        break;
    case RABBIT_CONSOLE_MODE_HEAD:
        head->rotate(1.0, true);
        break;
    case RABBIT_CONSOLE_MODE_R_EYEBROW:
        head->eyebrowRotate(1.0, true, 0x1);
        break;
    case RABBIT_CONSOLE_MODE_L_EYEBROW:
        head->eyebrowRotate(1.0, true, 0x2);
        break;
    case RABBIT_CONSOLE_MODE_R_EAR:
        head->earRotate(1.0, true, 0x1);
        break;
    case RABBIT_CONSOLE_MODE_L_EAR:
        head->earRotate(1.0, true, 0x2);
        break;
    case RABBIT_CONSOLE_MODE_R_TW:
        head->whiskerRotate(1.0, true, 0x1);
        break;
    case RABBIT_CONSOLE_MODE_L_TW:
        head->whiskerRotate(1.0, true, 0x2);
        break;
    default:
        break;
    }
}

static void action_left(void)
{
    switch (mode) {
    case RABBIT_CONSOLE_MODE_CAMERA:
        camera->pan(1.0, true);
        break;
    case RABBIT_CONSOLE_MODE_WHEEL:
        wheels->rol(WHEEL_DRIVE_LIMIT_MS);
        break;
    case RABBIT_CONSOLE_MODE_R_SHOULDER:
        rightArm->extendShoulder(1.0, 5, true);
        break;
    case RABBIT_CONSOLE_MODE_R_ELBOW:
        rightArm->extendElbow(-1.0, 5, true);
        break;
    case RABBIT_CONSOLE_MODE_R_WRIST:
        rightArm->extendWrist(-1.0, 5, true);
        break;
    case RABBIT_CONSOLE_MODE_R_GRIPPER:
        rightArm->setGripperPosition(1.0, 5, true);
        break;
    case RABBIT_CONSOLE_MODE_L_SHOULDER:
        leftArm->extendShoulder(1.0, 5, true);
        break;
    case RABBIT_CONSOLE_MODE_L_ELBOW:
        leftArm->extendElbow(1.0, 5, true);
        break;
    case RABBIT_CONSOLE_MODE_L_WRIST:
        leftArm->extendWrist(1.0, 5, true);
        break;
    case RABBIT_CONSOLE_MODE_L_GRIPPER:
        leftArm->setGripperPosition(1.0, 5, true);
        break;
    case RABBIT_CONSOLE_MODE_HEAD:
        head->rotate(-1.0, true);
        break;
    case RABBIT_CONSOLE_MODE_R_EYEBROW:
        head->eyebrowRotate(-1.0, true, 0x1);
        break;
    case RABBIT_CONSOLE_MODE_L_EYEBROW:
        head->eyebrowRotate(-1.0, true, 0x2);
        break;
    case RABBIT_CONSOLE_MODE_R_EAR:
        head->earRotate(-1.0, true, 0x1);
        break;
    case RABBIT_CONSOLE_MODE_L_EAR:
        head->earRotate(-1.0, true, 0x2);
        break;
    case RABBIT_CONSOLE_MODE_R_TW:
        head->whiskerRotate(-1.0, true, 0x1);
        break;
    case RABBIT_CONSOLE_MODE_L_TW:
        head->whiskerRotate(-1.0, true, 0x2);
        break;
    default:
        break;
    }
}

static void action_upright(void)
{
    switch (mode) {
    case RABBIT_CONSOLE_MODE_CAMERA:
        camera->tilt(-1.0, true);
        camera->pan(-1.0, true);
        break;
    case RABBIT_CONSOLE_MODE_WHEEL:
        wheels->fwr(WHEEL_DRIVE_LIMIT_MS);
        break;
    case RABBIT_CONSOLE_MODE_R_SHOULDER:
        rightArm->extendWrist(-1.0, 5, true);
        break;
    case RABBIT_CONSOLE_MODE_L_SHOULDER:
        leftArm->extendWrist(-1.0, 5, true);
        break;
    case RABBIT_CONSOLE_MODE_HEAD:
        head->tilt(-1.0, true);
        head->rotate(-1.0, true);
        break;
    default:
        break;
    }
}

static void action_upleft(void)
{
    switch (mode) {
    case RABBIT_CONSOLE_MODE_CAMERA:
        camera->tilt(-1.0, true);
        camera->pan(1.0, true);
        break;
    case RABBIT_CONSOLE_MODE_WHEEL:
        wheels->fwl(WHEEL_DRIVE_LIMIT_MS);
        break;
    case RABBIT_CONSOLE_MODE_R_SHOULDER:
        rightArm->extendElbow(-1.0, 5, true);
        break;
    case RABBIT_CONSOLE_MODE_L_SHOULDER:
        leftArm->extendElbow(-1.0, 5, true);
        break;
    case RABBIT_CONSOLE_MODE_HEAD:
        head->tilt(-1.0, true);
        head->rotate(1.0, true);
    default:
        break;
    }
}

static void action_downright(void)
{
    switch (mode) {
    case RABBIT_CONSOLE_MODE_CAMERA:
        camera->tilt(1.0, true);
        camera->pan(-1.0, true);
        break;
    case RABBIT_CONSOLE_MODE_WHEEL:
        wheels->bwr(WHEEL_DRIVE_LIMIT_MS);
        break;
    case RABBIT_CONSOLE_MODE_R_SHOULDER:
        rightArm->extendWrist(1.0, 5, true);
        break;
    case RABBIT_CONSOLE_MODE_L_SHOULDER:
        leftArm->extendWrist(1.0, 5, true);
        break;
    case RABBIT_CONSOLE_MODE_HEAD:
        head->tilt(1.0, true);
        head->rotate(-1.0, true);
        break;
    default:
        break;
    }
}

static void action_downleft(void)
{
    switch (mode) {
    case RABBIT_CONSOLE_MODE_CAMERA:
        camera->tilt(1.0, true);
        camera->pan(1.0, true);
        break;
    case RABBIT_CONSOLE_MODE_WHEEL:
        wheels->bwl(WHEEL_DRIVE_LIMIT_MS);
        break;
    case RABBIT_CONSOLE_MODE_R_SHOULDER:
        rightArm->extendElbow(1.0, 5, true);
        break;
    case RABBIT_CONSOLE_MODE_L_SHOULDER:
        leftArm->extendElbow(1.0, 5, true);
        break;
    case RABBIT_CONSOLE_MODE_HEAD:
        head->tilt(1.0, true);
        head->rotate(1.0, true);
        break;
    default:
        break;
    }
}

void rabbit_keycontrol(uint8_t key)
{
    string message;

    switch (key) {
    case 'q':
    case 'Q':
        LOG("Rabbit'bot quits\n");
        exit(EXIT_SUCCESS);
        break;
    case 'c':
    case 'C':
        if (mode != RABBIT_CONSOLE_MODE_CAMERA) {
            mode = RABBIT_CONSOLE_MODE_CAMERA;
            LOG("Camera control active\n");
        }
        break;
    case 'w':
    case 'W':
        if (mode != RABBIT_CONSOLE_MODE_WHEEL) {
            mode = RABBIT_CONSOLE_MODE_WHEEL;
            LOG("Wheel control active\n");
        }
        break;
    case 'h':
    case 'H':
        if (mode != RABBIT_CONSOLE_MODE_HEAD) {
            mode = RABBIT_CONSOLE_MODE_HEAD;
            LOG("Head control active\n");
        }
        break;
    case '>':
        if (mode >= RABBIT_CONSOLE_MODE_R_SHOULDER &&
            mode <= RABBIT_CONSOLE_MODE_R_GRIPPER) {
            mode = (enum rabbit_console_mode)
                (((unsigned int) mode) + 1);
            if (mode > RABBIT_CONSOLE_MODE_R_GRIPPER) {
                mode = RABBIT_CONSOLE_MODE_R_SHOULDER;
            }
        } else {
            mode = RABBIT_CONSOLE_MODE_R_SHOULDER;
        }
        switch (mode) {
        case RABBIT_CONSOLE_MODE_R_SHOULDER:
            LOG("Right shoulder control active\n");
            break;
        case RABBIT_CONSOLE_MODE_R_ELBOW:
            LOG("Right elbow control active\n");
            break;
        case RABBIT_CONSOLE_MODE_R_WRIST:
            LOG("Right wrist control active\n");
            break;
        case RABBIT_CONSOLE_MODE_R_GRIPPER:
            LOG("Right gripper control active\n");
            break;
        default:
            break;
        }
        break;
    case '<':
        if (mode >= RABBIT_CONSOLE_MODE_L_SHOULDER &&
            mode <= RABBIT_CONSOLE_MODE_L_GRIPPER) {
            mode = (enum rabbit_console_mode)
                (((unsigned int) mode) + 1);
            if (mode > RABBIT_CONSOLE_MODE_L_GRIPPER) {
                mode = RABBIT_CONSOLE_MODE_L_SHOULDER;
            }
        } else {
            mode = RABBIT_CONSOLE_MODE_L_SHOULDER;
        }
        switch (mode) {
        case RABBIT_CONSOLE_MODE_L_SHOULDER:
            LOG("Left shoulder control active\n");
            break;
        case RABBIT_CONSOLE_MODE_L_ELBOW:
            LOG("Left elbow control active\n");
            break;
        case RABBIT_CONSOLE_MODE_L_WRIST:
            LOG("Left wrist control active\n");
            break;
        case RABBIT_CONSOLE_MODE_L_GRIPPER:
            LOG("Left gripper control active\n");
            break;
        default:
            break;
        }
        break;
    case ')':
        if (mode != RABBIT_CONSOLE_MODE_R_EYEBROW) {
            mode = RABBIT_CONSOLE_MODE_R_EYEBROW;
            LOG("Right eyebrow control active\n");
        }
        break;
    case '(':
        if (mode != RABBIT_CONSOLE_MODE_L_EYEBROW) {
            mode = RABBIT_CONSOLE_MODE_L_EYEBROW;
            LOG("Right eyebrow control active\n");
        }
        break;
    case '}':
        if (mode != RABBIT_CONSOLE_MODE_R_EAR) {
            mode = RABBIT_CONSOLE_MODE_R_EAR;
            LOG("Right ear control active\n");
        }
        break;
    case '{':
        if (mode != RABBIT_CONSOLE_MODE_L_EAR) {
            mode = RABBIT_CONSOLE_MODE_L_EAR;
            LOG("Left ear control active\n");
        }
        break;
    case ']':
        if (mode != RABBIT_CONSOLE_MODE_R_TW) {
            mode = RABBIT_CONSOLE_MODE_R_TW;
            LOG("Right tooth/whisker active\n");
        }
        break;
    case '[':
        if (mode != RABBIT_CONSOLE_MODE_L_TW) {
            mode = RABBIT_CONSOLE_MODE_L_TW;
            LOG("Left tooth/whisker active\n");
        }
        break;
    case 'v':
    case 'V':
        camera->enVision(!camera->isVisionEn());
        stereovision->enVision(!stereovision->isVisionEn());
        break;
    case ' ':
        wheels->halt();
        rightArm->freeze();
        leftArm->freeze();
        camera->pan(0.0);
        camera->tilt(0.0);
        head->rotate(0.0);
        head->tilt(0.0);
        head->eyebrowSetDisposition(Head::EB_RELAXED);
        head->teethDown();
        head->whiskersCenter();
        break;
    case 'd':
    case 'D':
        lidar->enable(!lidar->isEnabled());
        break;
    case 'e':
    case 'E':
        stereovision->enableEmitter(!stereovision->isEmitterEnabled());
        break;
    case 'f':
    case 'F':
        stereovision->enableIMU(!stereovision->isIMUEnabled());
        break;
    case 's':
    case 'S':
        if (mode == RABBIT_CONSOLE_MODE_CAMERA) {
            camera->enSentry(!camera->isSentryEn());
        } else if (mode == RABBIT_CONSOLE_MODE_HEAD) {
            head->enSentry(!head->isSentryEn());
        }
        break;
    case 'x':
        mouth->beh();
        rightArm->rest();
        leftArm->rest();
        break;
    case 'z':
        mouth->beh();
        rightArm->surrender();
        leftArm->surrender();
        break;
    case 'y':
        mouth->smile();
        rightArm->hug();
        leftArm->hug();
        break;
    case 'a':
        mouth->beh();
        rightArm->pickup();
        break;
    case'A':
        mouth->beh();
        leftArm->pickup();
        break;
    case 'u':
        mouth->beh();
        rightArm->extend();
        break;
    case 'U':
        mouth->beh();
        leftArm->extend();
        break;
    case 'i':
        mouth->smile();
        rightArm->hi();
        break;
    case 'I':
        mouth->smile();
        leftArm->hi();
        break;
    case 'k':
        mouth->beh();
        rightArm->xferRL();
        leftArm->xferRL();
        break;
    case 'K':
        mouth->beh();
        rightArm->xferLR();
        leftArm->xferLR();
        break;
    case 't':
    case 'T':
        mouth->beh();
        rightArm->muscles();
        leftArm->muscles();
        break;
    case 'p':
    case 'P':
        chan -= 1;
        if (chan >= SERVO_CHANNELS) {
            chan = SERVO_CHANNELS - 1;
        }
        message = string("Servo #") + to_string(chan) + " selected\n";
        LOG(message.c_str());
        break;
    case 'N':
    case 'n':
        chan += 1;
        if (chan >= SERVO_CHANNELS) {
            chan = 0;
        }
        message = string("Servo #") + to_string(chan) + " selected\n";
        LOG(message.c_str());
        break;
    case '=':
        servos->center(chan);
        message = string("Servo #") + to_string(chan) +
            " centered to " + to_string(servos->pulse(chan)) + "\n";
        LOG(message.c_str());
        break;
    case '^':
        servos->setPulse(chan, servos->loRange(chan));
        message = string("Servo #") + to_string(chan) +
            " set lo to " + to_string(servos->pulse(chan)) + "\n";
        LOG(message.c_str());
        break;
    case '$':
        servos->setPulse(chan, servos->hiRange(chan));
        message = string("Servo #") + to_string(chan) +
            " set hi to " + to_string(servos->pulse(chan)) + "\n";
        LOG(message.c_str());
        break;
    case '+':
        servos->setPulse(chan, servos->pulse(chan) + 1, true);
        message = string("Servo #") + to_string(chan) +
            " set to " + to_string(servos->pulse(chan)) + "\n";
        LOG(message.c_str());
        break;
    case '-':
        servos->setPulse(chan, servos->pulse(chan) - 1, true);
        message = string("Servo #") + to_string(chan) +
            " set to " + to_string(servos->pulse(chan)) + "\n";
        LOG(message.c_str());
        break;
    case '@':
        servos->setPulse(chan, servos->pulse(chan) + 10, true);
        message = string("Servo #") + to_string(chan) +
            " set to " + to_string(servos->pulse(chan)) + "\n";
        LOG(message.c_str());
        break;
    case '!':
        servos->setPulse(chan, servos->pulse(chan) - 10, true);
        message = string("Servo #") + to_string(chan) +
            " set to " + to_string(servos->pulse(chan)) + "\n";
        LOG(message.c_str());
        break;
    case '8':
        action_up();
        break;
    case '2':
        action_down();
        break;
    case '6':
        action_right();
        break;
    case '4':
        action_left();
        break;
    case '9':
        action_upright();
        break;
    case '7':
        action_upleft();
        break;
    case '3':
        action_downright();
        break;
    case '1':
        action_downleft();
        break;
    default:
        break;
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
