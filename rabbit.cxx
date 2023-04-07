/*
 * rabbit.cxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <unistd.h>
#include <stdio.h>
#include <termios.h>
#include <sys/select.h>
#include <iostream>
#include "video.hxx"

static Video *video = NULL;
static struct termios t_old;

void cleanup(void)
{
    tcsetattr(fileno(stdin), TCSANOW, &t_old);

    if (video) {
        delete video;
        video = NULL;
    }
}

int main(int argc, char **argv)
{
    (void)(argc);
    (void)(argv);
    struct termios t_new;

    tcgetattr(fileno(stdin), &t_old);

    atexit(cleanup);

    t_new = t_old;
    t_new.c_lflag &= (tcflag_t) ~(ICANON | ECHO);
    tcsetattr(fileno(stdin), TCSANOW, &t_new);

    video = new Video();

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
            if (key == 'q' || key == 'Q') {
                exit(EXIT_SUCCESS);
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
