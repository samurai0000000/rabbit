/*
 * rabbit.cxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include <signal.h>
#include <termios.h>
#include <sys/select.h>
#include <time.h>
#include <sys/time.h>
#include <iostream>
#include "rabbit.hxx"
#include "osdcam.hxx"

using namespace std;

static int daemonize = 0;
static struct termios t_old;

nadjieb::MJPEGStreamer *mjpeg_streamer = NULL;
Mosquitto *mosquitto = NULL;
Servos *servos = NULL;
ADC *adc = NULL;
Camera *camera = NULL;
StereoVision *stereovision = NULL;
Proximity *proximity = NULL;
Wheels *wheels = NULL;
Arm *rightArm = NULL;
Arm *leftArm = NULL;
Power *power = NULL;
Compass *compass = NULL;
Ambience *ambience = NULL;
Head *head = NULL;
LiDAR *lidar = NULL;
Mouth *mouth = NULL;
Speech *speech = NULL;
Voice *voice = NULL;
Crond *crond = NULL;

static void announce_clock(void);

static void cleanup(void)
{
    if (!daemonize) {
        tcsetattr(fileno(stdin), TCSANOW, &t_old);
    }

    if (crond) {
        delete crond;
        crond = NULL;
    }

    if (camera) {
        delete camera;
        camera = NULL;
    }

    if (stereovision) {
        delete stereovision;
        stereovision = NULL;
    }

    if (wheels) {
        delete wheels;
        wheels = NULL;
    }

    if (proximity) {
        delete proximity;
        proximity = NULL;
    }

    if (rightArm) {
        delete rightArm;
        rightArm = NULL;
    }

    if (leftArm) {
        delete leftArm;
        leftArm = NULL;
    }

    if (voice) {
        delete voice;
        voice = NULL;
    }

    if (power) {
        delete power;
        power = NULL;
    }

    if (compass) {
        delete compass;
        compass = NULL;
    }

    if (head) {
        delete head;
        head = NULL;
    }

    if (lidar) {
        delete lidar;
        lidar = NULL;
    }

    if (mouth) {
        delete mouth;
        mouth = NULL;
    }

    if (ambience) {
        delete ambience;
        ambience = NULL;
    }

    if (adc) {
        delete adc;
        adc = NULL;
    }

    if (servos) {
        delete servos;
        servos = NULL;
    }

    if (speech) {
        delete speech;
        speech = NULL;
    }

    if (mosquitto) {
        delete mosquitto;
        mosquitto = NULL;
    }

    if (mjpeg_streamer) {
        mjpeg_streamer->stop();
        delete mjpeg_streamer;
        mjpeg_streamer = NULL;
    }

    websock_cleanup();
    gpioTerminate();
}

static void sig_handler(int signal)
{
    switch (signal) {
    case SIGINT:
    case SIGTERM:
    case SIGKILL:
        exit(EXIT_SUCCESS);
        break;
    case SIGSEGV:
        fprintf(stderr, "Caught SIGSEGV!\n");
        gpioTerminate();     // Free up hardware resources as fail-safe
        exit(EXIT_FAILURE);
        break;
    default:
        fprintf(stderr, "Caught unexpected signal %d\n", signal);
        gpioTerminate();     // Free up hardware resources as fail-safe
        exit(EXIT_FAILURE);
        break;
    }
}

static void print_help(int argc, char **argv)
{
    (void)(argc);

    printf("Usage: %s [OPTIONS]\n", argv[0]);
    printf("  --help,-h      This message\n");
    printf("  --daemon,-d    Run %s as daemon\n", argv[0]);
}

static const struct option long_options[] = {
    { "help", no_argument, NULL, 'h', },
    { "daemon", no_argument, NULL, 'd', },
};

int main(int argc, char **argv)
{
    int ret;
    struct termios t_new;

    for (;;) {
        int option_index = 0;
        int c = getopt_long(argc, argv, "hd",
                            long_options, &option_index);
        if (c == -1) {
            break;
        }

        switch (c) {
        case 'h':
            print_help(argc, argv);
            return 0;
        case 'd':
            daemonize = 1;
            break;
        default:
            print_help(argc, argv);
            return -1;
            break;
        }
    }

    if (daemonize) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            /* Child */
            close(fileno(stdin));
            close(fileno(stdout));
            close(fileno(stderr));
        } else {
            /* Parent */
            exit(EXIT_SUCCESS);
        }
    } else {
        tcgetattr(fileno(stdin), &t_old);
        t_new = t_old;
        t_new.c_lflag &= (tcflag_t) ~(ICANON | ECHO);
        tcsetattr(fileno(stdin), TCSANOW, &t_new);
    }

    ret = gpioCfgSetInternals(gpioCfgGetInternals() | PI_CFG_NOSIGHANDLER);
    if (ret != 0) {
        fprintf(stderr, "gpioCfgSetInternals failed (%d)!\n", ret);
    }

    ret = gpioInitialise();
    if (ret == PI_INIT_FAILED) {
        fprintf(stderr, "gpioInitialize failed (%d)!\n", ret);
        exit(EXIT_FAILURE);
    }

    websock_init();

    atexit(cleanup);
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGSEGV, sig_handler);
    signal(SIGKILL, sig_handler);

    OsdCam::initialize();
    mjpeg_streamer = new nadjieb::MJPEGStreamer();
    mjpeg_streamer->start(8000);
    mosquitto = new Mosquitto();
    servos = new Servos();
    adc = new ADC();
    camera = new Camera();
    stereovision = new StereoVision();
    proximity = new Proximity();
    wheels = new Wheels();
    rightArm = new Arm(RIGHT_ARM);
    leftArm = new Arm(LEFT_ARM);
    compass = new Compass();
    ambience = new Ambience();
    head = new Head();
    lidar = new LiDAR();
    speech = new Speech();
    mouth = new Mouth();
    voice = new Voice();
    crond = new Crond();
    crond->activate(announce_clock, "*/2 * * * *");

    cout << "Rabbit'bot is alive!" << endl;

    if (daemonize) {
        for (;;) {
            sleep(60);
        }
    } else {
        for (;;) {
            int ret;
            int nfds;
            fd_set readfds;
            struct timeval timeout = {
                .tv_sec = 0,
                .tv_usec = 5000,
            };

            FD_ZERO(&readfds);
            FD_SET(STDIN_FILENO, &readfds);
            nfds = STDIN_FILENO + 1;

            ret = select(nfds, &readfds, NULL, NULL, &timeout);
            if (ret <= 0) {
                continue;
            }

            if (FD_ISSET(STDIN_FILENO, &readfds)) {
                int k1, k2, k3;
                int nread;

                nread = 0;
                ret = ioctl(STDIN_FILENO, FIONREAD, &nread);
                if (ret != 0) {
                    perror("ioctl");
                    continue;
                }
                nread--;

                switch (k1 = getchar()) {
                case 0x1b:
                    if (nread < 2) {
                        continue;
                    } else {
                        k2 = getchar();
                        k3 = getchar();
                    }

                    if (k2 == 0x5b && k3 == 0x41) {
                        rabbit_keycontrol('8');
                    } else if (k2 == 0x5b && k3 == 0x42) {
                        rabbit_keycontrol('2');
                    } else if (k2 == 0x5b && k3 == 0x43) {
                        rabbit_keycontrol('6');
                    } else if (k2 == 0x5b && k3 == 0x44) {
                        rabbit_keycontrol('4');
                    }
                    break;
                default:
                    rabbit_keycontrol(k1);
                    break;
                }
            }
        }
    }

    return 0;
}

static void announce_clock(void)
{
    struct timeval now;
    const struct tm *tm;
    const char *timeofday;
    char announcement[256];

    gettimeofday(&now, NULL);
    tm = localtime(&now.tv_sec);

    if (tm->tm_hour == 0) {
        timeofday = "midnight";
    } else if (tm->tm_hour >= 1 && tm->tm_hour <= 11) {
        timeofday = "in the morning";
    } else if (tm->tm_hour == 12) {
        timeofday = "at noon";
    } else if (tm->tm_hour >= 13 && tm->tm_hour <= 18) {
        timeofday = "in the afternoon";
    } else {
        timeofday = "in the evening";
    }

    if (tm->tm_min == 0) {
        unsigned int hour;
        hour = tm->tm_hour % 12;
        if (hour == 0) {
            hour = 12;
        }
        snprintf(announcement, sizeof(announcement) - 1,
                 "Now is %d o'clock %s\n",
                 hour, timeofday);
    } else {
        unsigned int hour;
        hour = tm->tm_hour % 12;
        if (hour == 0) {
            hour = 12;
        }
        snprintf(announcement, sizeof(announcement) - 1,
                 "Now is %d o'clock and %d %s %s\n",
                 hour, tm->tm_min,
                 tm->tm_min == 1 ? "minute" : "minutes",
                 timeofday);
    }

    LOG(announcement);
    speech->speak(announcement);
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
