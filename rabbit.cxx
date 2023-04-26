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
#include <iostream>
#include "rabbit.hxx"

#define WHEEL_DRIVE_LIMIT_MS  100

static int daemonize = 0;
static struct termios t_old;

Servos *servos = NULL;
ADC *adc = NULL;
Camera *camera = NULL;
Wheels *wheels = NULL;
Power *power = NULL;
Compass *compass = NULL;
Ambience *ambience = NULL;

static void cleanup(void)
{
    if (!daemonize) {
        tcsetattr(fileno(stdin), TCSANOW, &t_old);
    }

    if (camera) {
        delete camera;
        camera = NULL;
    }

    if (wheels) {
        delete wheels;
        wheels = NULL;
    }

    if (power) {
        delete power;
        power = NULL;
    }

    if (compass) {
        delete compass;
        compass = NULL;
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

    gpioTerminate();
}

static void sig_handler(int signal)
{
    if (signal == SIGALRM) {
        wheels->halt();
    } else {
        exit(EXIT_SUCCESS);
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


    if (gpioInitialise() < 0) {
        cerr << "gpioInitialise failed!" << endl;
        return -1;
    }

    atexit(cleanup);
    signal(SIGTERM, sig_handler);
    signal(SIGALRM, sig_handler);

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
        signal(SIGINT, sig_handler);
        tcgetattr(fileno(stdin), &t_old);
        t_new = t_old;
        t_new.c_lflag &= (tcflag_t) ~(ICANON | ECHO);
        tcsetattr(fileno(stdin), TCSANOW, &t_new);
    }

    servos = new Servos();
    adc = new ADC();
    camera = new Camera();
    wheels = new Wheels();
    power = new Power();
    compass = new Compass();
    ambience = new Ambience();

    if (daemonize) {
        for (;;) {
            sleep(60);
        }
    } else {
        unsigned int chan = 12;
        enum {
            RABBIT_CONSOLE_MODE_WHEEL = 0,
            RABBIT_CONSOLE_MODE_CAMERA = 1,
        } mode = RABBIT_CONSOLE_MODE_WHEEL;

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
                case 'q':
                case 'Q':
                    exit(EXIT_SUCCESS);
                    break;
                case 'c':
                case 'C':
                    mode = RABBIT_CONSOLE_MODE_CAMERA;
                    break;
                case 'w':
                case 'W':
                    mode = RABBIT_CONSOLE_MODE_WHEEL;
                    break;
                case 'v':
                case 'V':
                    camera->enVision(!camera->isVisionEn());
                    break;
                case 'h':
                case 'H':
                    wheels->halt();
                    break;
                case 'f':
                case 'F':
                    wheels->fwd(WHEEL_DRIVE_LIMIT_MS);
                    break;
                case 'b':
                case 'B':
                    wheels->bwd(WHEEL_DRIVE_LIMIT_MS);
                    break;
                case 'r':
                case 'R':
                    wheels->ror(WHEEL_DRIVE_LIMIT_MS);
                    break;
                case 'l':
                case 'L':
                    wheels->rol(WHEEL_DRIVE_LIMIT_MS);
                    break;
                case ' ':
                    camera->pan(0);
                    camera->tilt(0);
                    break;
                case 's':
                case 'S':
                    camera->enSentry(!camera->isSentryEn());
                    cout << "Sentry "
                         << (camera->isSentryEn() ? "enabled" : "disabled")
                         << endl;
                    break;
                case 'P':
                case 'p':
                    chan -= 1;
                    if (chan >= SERVO_CHANNELS) {
                        chan = SERVO_CHANNELS - 1;
                    }
                    cout << "Servo #" << to_string(chan) << " selected" << endl;
                    break;
                case 'N':
                case 'n':
                    chan += 1;
                    if (chan >= SERVO_CHANNELS) {
                        chan = 0;
                    }
                    cout << "Servo #" << to_string(chan) << " selected" << endl;
                    break;
                case '=':
                    servos->center(chan);
                    cout << "Servo #" << to_string(chan)
                         << " centered to " << to_string(servos->pulse(chan))
                         << endl;
                    break;
                case '^':
                    servos->setPulse(chan, servos->loRange(chan));
                    cout << "Servo #" << to_string(chan)
                         << " set lo to " << to_string(servos->pulse(chan))
                         << endl;
                    break;
                case '$':
                    servos->setPulse(chan, servos->hiRange(chan));
                    cout << "Servo #" << to_string(chan)
                         << " set hi to " << to_string(servos->pulse(chan))
                         << endl;
                    break;
                case '+':
                    servos->setPulse(chan, servos->pulse(chan) + 1, true);
                    cout << "Servo #" << to_string(chan)
                         << " set to " << to_string(servos->pulse(chan))
                         << endl;
                    break;
                case '-':
                    servos->setPulse(chan, servos->pulse(chan) - 1, true);
                    cout << "Servo #" << to_string(chan)
                         << " set to " << to_string(servos->pulse(chan))
                         << endl;
                    break;
                case '@':
                    servos->setPulse(chan, servos->pulse(chan) + 10, true);
                    cout << "Servo #" << to_string(chan)
                         << " set to " << to_string(servos->pulse(chan))
                         << endl;
                    break;
                case '!':
                    servos->setPulse(chan, servos->pulse(chan) - 10, true);
                    cout << "Servo #" << to_string(chan)
                         << " set to " << to_string(servos->pulse(chan))
                         << endl;
                    break;
                case 0x1b:
                    if (nread < 2) {
                        continue;
                    } else {
                        k2 = getchar();
                        k3 = getchar();
                    }

                    if (k2 == 0x5b && k3 == 0x41) {
                        /* Up arrow */
                        if (mode == RABBIT_CONSOLE_MODE_CAMERA) {
                            camera->tilt(-1, true);
                        } else {
                            wheels->fwd(WHEEL_DRIVE_LIMIT_MS);
                        }
                    } else if (k2 == 0x5b && k3 == 0x42) {
                        /* Down arrow */
                        if (mode == RABBIT_CONSOLE_MODE_CAMERA) {
                            camera->tilt(1, true);
                        } else {
                            wheels->bwd(WHEEL_DRIVE_LIMIT_MS);
                        }
                    } else if (k2 == 0x5b && k3 == 0x44) {
                        /* Left arrow */
                        if (mode == RABBIT_CONSOLE_MODE_CAMERA) {
                            camera->pan(1, true);
                        } else {
                            wheels->rol(WHEEL_DRIVE_LIMIT_MS);
                        }
                    } else if (k2 == 0x5b && k3 == 0x43) {
                        /* Right arrow */
                        if (mode == RABBIT_CONSOLE_MODE_CAMERA) {
                            camera->pan(-1, true);
                        } else {
                            wheels->ror(WHEEL_DRIVE_LIMIT_MS);
                        }
                    }
                    break;
                default:
                    break;
                }
            }
        }
    }

    return 0;
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
