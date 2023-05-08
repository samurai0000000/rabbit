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

static int daemonize = 0;
static struct termios t_old;

Servos *servos = NULL;
ADC *adc = NULL;
Camera *camera = NULL;
Wheels *wheels = NULL;
Arm *rightArm = NULL;
Arm *leftArm = NULL;
Power *power = NULL;
Compass *compass = NULL;
Ambience *ambience = NULL;

static void cleanup(void)
{
    if (!daemonize) {
        tcsetattr(fileno(stdin), TCSANOW, &t_old);
    }

    websock_cleanup();

    if (camera) {
        delete camera;
        camera = NULL;
    }

    if (wheels) {
        delete wheels;
        wheels = NULL;
    }

    if (rightArm) {
        delete rightArm;
        rightArm = NULL;
    }

    if (leftArm) {
        delete leftArm;
        leftArm = NULL;
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
    (void)(signal);
    exit(EXIT_SUCCESS);
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
    rightArm = new Arm(RIGHT_ARM);
    leftArm = new Arm(LEFT_ARM);
    power = new Power();
    compass = new Compass();
    ambience = new Ambience();

    cout << "Rabbit 'bot' is alive!" << endl;

    websock_init();

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

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
