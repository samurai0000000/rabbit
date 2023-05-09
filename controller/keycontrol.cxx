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
};

static enum rabbit_console_mode mode = RABBIT_CONSOLE_MODE_CAMERA;

static void action_up(void)
{
    switch (mode) {
    case RABBIT_CONSOLE_MODE_CAMERA:
        camera->tilt(-1, true);
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
    default:
        break;
    }
}

static void action_down(void)
{
    switch (mode) {
    case RABBIT_CONSOLE_MODE_CAMERA:
        camera->tilt(1, true);
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
    default:
        break;
    }
}

static void action_left(void)
{
    switch (mode) {
    case RABBIT_CONSOLE_MODE_CAMERA:
        camera->pan(1, true);
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
    default:
        break;
    }
}

static void action_upright(void)
{
    switch (mode) {
    case RABBIT_CONSOLE_MODE_CAMERA:
        camera->tilt(-1, true);
        camera->pan(-1, true);
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
    default:
        break;
    }
}

static void action_upleft(void)
{
    switch (mode) {
    case RABBIT_CONSOLE_MODE_CAMERA:
        camera->tilt(-1, true);
        camera->pan(1, true);
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
    default:
        break;
    }
}

static void action_downright(void)
{
    switch (mode) {
    case RABBIT_CONSOLE_MODE_CAMERA:
        camera->tilt(1, true);
        camera->pan(-1, true);
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
    default:
        break;
    }
}

static void action_downleft(void)
{
    switch (mode) {
    case RABBIT_CONSOLE_MODE_CAMERA:
        camera->tilt(1, true);
        camera->pan(1, true);
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
    case 'v':
    case 'V':
        camera->enVision(!camera->isVisionEn());
        break;
    case ' ':
        camera->pan(0);
        camera->tilt(0);
        wheels->halt();
        rightArm->freeze();
        leftArm->freeze();
        LOG("Camera centered, wheels halted, arms frozen\n");
        break;
    case 's':
    case 'S':
        camera->enSentry(!camera->isSentryEn());
        break;
    case 'r':
    case 'R':
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
    case 'l':
    case 'L':
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
    case 'x':
        rightArm->rest();
        leftArm->rest();
        LOG("Move arms to rest\n");
        break;
    case 'y':
        rightArm->hug();
        leftArm->hug();
        LOG("Move arms to hug\n");
        break;
    case 'z':
        rightArm->surrender();
        leftArm->surrender();
        LOG("Move arms to surrender\n");
        break;
    case 'a':
        rightArm->pickup();
        LOG("Right arm picks up\n");
        break;
    case'A':
        leftArm->pickup();
        LOG("Left arm picks up\n");
        break;
    case 'u':
        rightArm->extend();
        LOG("Right arm extends\n");
        break;
    case 'U':
        leftArm->extend();
        LOG("Left arm extends\n");
        break;
    case 'i':
        rightArm->hi();
        LOG("Right arm waves hi\n");
        break;
    case 'I':
        leftArm->hi();
        LOG("Left arm waves hi\n");
        break;
    case 'k':
        rightArm->xferRL();
        leftArm->xferRL();
        LOG("Transfer grip object from right to left\n");
        break;
    case 'K':
        rightArm->xferLR();
        leftArm->xferLR();
        LOG("Transfer grip object from left to right\n");
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
