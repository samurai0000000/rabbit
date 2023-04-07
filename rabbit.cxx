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
#include <pigpio.h>
#include "video.hxx"
#include "wheels.hxx"

#define WHEEL_DRIVE_LIMIT_MS  3000

static int daemonize = 0;
static Video *video = NULL;
static Wheels *wheels = NULL;
static struct termios t_old;

static void cleanup(void)
{
    if (!daemonize) {
        tcsetattr(fileno(stdin), TCSANOW, &t_old);
    }

    if (video) {
        delete video;
        video = NULL;
    }

    if (wheels) {
        delete wheels;
        wheels = NULL;
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

    video = new Video();
    wheels = new Wheels();

    if (daemonize) {
        for (;;) {
            sleep(60);
        }
    } else {
        cout << "Press 'q' to quit ..." << endl;

        for (;;) {
            int nfds;
            fd_set readfds;
            struct timeval timeout = {
                .tv_sec = 0,
                .tv_usec = 200000,
            };

            FD_ZERO(&readfds);
            FD_SET(STDIN_FILENO, &readfds);
            nfds = STDIN_FILENO + 1;

            select(nfds, &readfds, NULL, NULL, &timeout);

            if (FD_ISSET(STDIN_FILENO, &readfds)) {
                int key = getchar();
                switch (key) {
                case 'q':
                case 'Q':
                    exit(EXIT_SUCCESS);
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
