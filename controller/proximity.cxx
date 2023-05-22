/*
 * proximity.cxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <bsd/sys/time.h>
#include <pthread.h>
#include <termios.h>
#include <errno.h>
#include <pigpio.h>
#include <string>
#include "rabbit.hxx"

using namespace std;

Proximity::Proximity()
    : _enabled(true)
{
    unsigned int i;

    for (i = 0; i < RABBIT_MCUS; i++) {
        _handle[i] = -1;
        _node[i] = "";
        _temp_c[i] = 0.0;
    }

    bzero(&_ir, sizeof(_ir));
    bzero(&_us, sizeof(_us));

    _running = true;
    pthread_mutex_init(&_mutex, NULL);
    pthread_cond_init(&_cond, NULL);
    pthread_create(&_thread, NULL, Proximity::thread_func, this);
}

Proximity::~Proximity()
{
    unsigned int i;

    _running = false;
    pthread_cond_broadcast(&_cond);
    pthread_join(_thread, NULL);
    pthread_mutex_destroy(&_mutex);
    pthread_cond_destroy(&_cond);

    stop();

    for (i = 0; i < RABBIT_MCUS; i++) {
        if (_handle[i] != -1) {
            errCloseDevice(i);
        }
    }
}

void *Proximity::thread_func(void *args)
{
    Proximity *proximity = (Proximity *) args;

    proximity->run();

    return NULL;
}

static size_t serial_scan_line(int fd, char *line, size_t len,
                               unsigned int timeout_us)
{
    int ret;
    int nfds;
    fd_set readfds;
    struct timeval now, expire;
    struct timeval timeout;
    size_t size = 0;

    if ((fd == -1) || (line == NULL) || (len == 0)) {
        return 0;
    }

    gettimeofday(&now, NULL);
    if (timeout_us > 1000000) {
        timeout.tv_sec = timeout_us / 1000000;
        timeout_us %= 1000000;
    }
    timeout.tv_usec = timeout_us;
    timeradd(&now, &timeout, &expire);

    while (size < (len - 1)) {
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        nfds = fd + 1;

        gettimeofday(&now, NULL);
        if (timercmp(&now, &expire, >=)) {
            break;
        }
        timersub(&expire, &now, &timeout);

        ret = select(nfds, &readfds, NULL, NULL, &timeout);
        if (ret != 1) {
            break;
        }

        if (FD_ISSET(fd, &readfds)) {
            ret = read(fd, &line[size], 1);
            if (ret != 1) {
                break;
            }

            if (line[size] == '\r' || line[size] == '\n') {
                line[size] = '\0';
                if (size > 0) {
                    break;
                }
            } else {
                size++;
            }
        }
    }

    return size;
}

void Proximity::probeOpenDevice(unsigned int id)
{
    int ret;
    char devname[32];
    char devid[64];
    struct termios tty;
    int speed = B115200;
    char buf[128];

    if (id >= RABBIT_MCUS) {
        return;
    } else if (_handle[id] != -1) {
        return;
    }

    snprintf(devname, sizeof(devname) - 1, "/dev/ttyACM%u", id);
    _handle[id] = open(devname, O_RDWR);
    if (_handle[id] == -1) {
        return;
    }

    /* Set up TTY  for serial device */
    if (tcgetattr(_handle[id], &tty) != 0) {
        fprintf(stderr, "tcgetattr('%s'): %s\n", devname, strerror(errno));
        return;
    }
    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_iflag &= ~IGNBRK;
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 0;
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~(PARENB | PARODD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;
    if (tcsetattr(_handle[id], TCSANOW, &tty) != 0) {
        fprintf(stderr, "tcsetattr('%s'): %s\n", devname, strerror(errno));
        return;
    }

    /* Ensure streaming has stopped */
    ret = write(_handle[id], "stop\r", 5);
    if (ret != 5) {
        close(_handle[id]);
        _handle[id] = -1;
        return;
    }

    /* Flush input */
    for (;;) {
        ret = serial_scan_line(_handle[id], devid, sizeof(devid), 10000);
        if (ret == 0) {
            break;
        }
    }

    /* Request the device ID */
    ret = write(_handle[id], "id\r", 3);
    if (ret != 3) {
        close(_handle[id]);
        _handle[id] = -1;
        return;
    }

    /* Get the device ID */
    ret = serial_scan_line(_handle[id], devid, sizeof(devid), 10000);
    if (ret == 0) {
        close(_handle[id]);
        _handle[id] = -1;
        return;
    }

    /* Ensure the ID shows the correct MCU */
    if (strstr(devid, "Rabbit MCU on ") != devid) {
        close(_handle[id]);
        _handle[id] = -1;
        return;
    } else {
        _node[id] = string(devid + 14);
    }

    snprintf(buf, sizeof(buf) - 1, "MCU %s is online\n", _node[id].c_str());
    LOG(buf);

    /* Start streaming */
    if (_enabled) {
        ret = write(_handle[id], "stream\r", 7);
        if (ret != 7) {
            close(_handle[id]);
            _handle[id] = -1;
        }
    }
}

void Proximity::errCloseDevice(unsigned int id)
{
    char buf[128];

    if (id >= RABBIT_MCUS) {
        return;
    }

    if (_handle[id] == -1) {
        return;
    }

    snprintf(buf, sizeof(buf) - 1, "Put MCU %s offline\n", _node[id].c_str());
    LOG(buf);

    close(_handle[id]);
    _handle[id] = -1;
    _node[id] = "";
}

void Proximity::run(void)
{
    while (_running) {
        int ret;
        unsigned int id;
        char line[128];
        char *part, *partctx;
        char *subpart, *subpartctx;
        unsigned int valid;

        for (id = 0, valid = 0; id < RABBIT_MCUS; id++) {
            unsigned int k;

            if ((id == RABBIT_MCUS - 1) && (valid == 0) && (_running == true)) {
                /* Sleep to prevent thrashing if no device present */
                struct timespec ts;
                clock_gettime(CLOCK_REALTIME, &ts);
                ts.tv_sec += 1;
                pthread_mutex_lock(&_mutex);
                pthread_cond_timedwait(&_cond, &_mutex, &ts);
                pthread_mutex_unlock(&_mutex);
            }

            /* Probe and open device */
            if (_handle[id] == -1) {
                probeOpenDevice(id);
                if (_handle[id] == -1) {
                    continue;
                }
            }

            valid++;

            /* Process streaming data from MCU */
            pthread_mutex_lock(&_mutex);

            ret = serial_scan_line(_handle[id], line, sizeof(line), 50000);
            if (ret == 0) {
                errCloseDevice(id);
            }

            pthread_mutex_unlock(&_mutex);

            /*
             * Parse the output and update states
             */

            /* Process the IR part */
            part = strtok_r(line, ";", &partctx);
            if (part == NULL) {
                errCloseDevice(id);
                continue;
            }

            for (subpart = strtok_r(part, ",", &subpartctx), k = 0;
                 (subpart != NULL) && (k < (IR_DEVICES / RABBIT_MCUS));
                 subpart = strtok_r(NULL, ",", &subpartctx), k++) {
                _ir[id * 2 + k] = (unsigned int) atoi(subpart);
                //printf("_ir[%u]=%u\n", id * 2 + k, _ir[id * 2 + k]);
            }

            if ((k != (IR_DEVICES / RABBIT_MCUS)) ||
                (subpart != NULL)) {
                errCloseDevice(id);
                continue;
            }

            /* Process the US part */
            part = strtok_r(NULL, ";", &partctx);
            if (part == NULL) {
                errCloseDevice(id);
                continue;
            }

            for (subpart = strtok_r(part, ",", &subpartctx), k = 0;
                 (subpart != NULL) && (k < (ULTRASOUND_DEVICES / RABBIT_MCUS));
                 subpart = strtok_r(NULL, ",", &subpartctx), k++) {
                _us[id * 2 + k] = (unsigned int) atoi(subpart);
                //printf("_us[%u]=%u\n", id * 2 + k, _us[id * 2 + k]);
            }

            if ((k != (ULTRASOUND_DEVICES / RABBIT_MCUS)) ||
                (subpart != NULL)) {
                errCloseDevice(id);
                continue;
            }

            /* Process the temp_c part */
            part = strtok_r(NULL, ";", &partctx);
            if (part == NULL) {
                errCloseDevice(id);
                continue;
            }

            for (subpart = strtok_r(part, ",", &subpartctx), k = 0;
                 (subpart != NULL) && (k < 1);
                 subpart = strtok_r(NULL, ",", &subpartctx), k++) {
                _temp_c[id] = (float) atof(subpart);
                //printf("_temp_c[%u]=%.1f\n", id, _temp_c[id]);
            }

            if ((k != 1) ||
                (subpart != NULL)) {
                errCloseDevice(id);
                continue;
            }
        }
    }
}

void Proximity::enable(bool en)
{
    en = en ? true : false;
    if (en == _enabled) {
        return;
    }

    if (en) {
        stream();
    } else {
        stop();
    }
}

void Proximity::stream(void)
{
    int ret;
    unsigned int id;

    if (_enabled == true) {
        return;
    }

    pthread_mutex_lock(&_mutex);

    _enabled = true;
    for (id = 0; id < RABBIT_MCUS; id++) {
        if (_handle[id] == -1) {
            continue;
        }

        ret = write(_handle[id], "stream\r", 7);
        if (ret != 7) {
            errCloseDevice(id);
        }
    }

    pthread_mutex_unlock(&_mutex);
}

void Proximity::stop(void)
{
    int ret;
    unsigned int id;
    char line[128];
    unsigned int count;

    if (_enabled == false) {
        return;
    }

    pthread_mutex_lock(&_mutex);

    _enabled = false;
    for (id = 0; id < RABBIT_MCUS; id++) {
        if (_handle[id] == -1) {
            continue;
        }

        /* Stop streaming and flush input */
        count = 3;
        while (count > 0) {
            ret = write(_handle[id], "stop\r", 5);
            if (ret != 5) {
                errCloseDevice(id);
            }

            ret = serial_scan_line(_handle[id], line, sizeof(line), 10000);
            if (ret > 0) {
                count = 3;
            } else {
                count--;
            }
        }
    }

    pthread_mutex_unlock(&_mutex);
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
