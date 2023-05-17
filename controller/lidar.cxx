/*
 * lidar.cxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <sys/time.h>
#include <bsd/sys/time.h>
#include <pthread.h>
#include "lidar.hxx"

/*
 * CLDR-08SB
 *
 * https://www.csensor.com/product-cldr
 * https://www.icshop.com.tw/pd/368030200713/datasheet.pdf
 */

#define LIDAR_TTY        "/dev/ttyUSB0"
#define LIDAR_BAUD_RATE          B115200

struct cldr_message {
    uint16_t ph;
    uint8_t ct;
    uint8_t lsn;
    uint16_t fsa;
    uint16_t lsa;
    uint16_t cs;
} __attribute__ ((packed));

LiDAR::LiDAR()
    : _handle(-1),
      _speed(50),
      _operational(true)
{
    _running = true;
    pthread_mutex_init(&_mutex, NULL);
    pthread_cond_init(&_cond, NULL);
    pthread_create(&_thread, NULL, LiDAR::thread_func, this);
}

LiDAR::~LiDAR()
{
    _running = false;
    pthread_cond_broadcast(&_cond);
    pthread_join(_thread, NULL);
    pthread_mutex_destroy(&_mutex);
    pthread_cond_destroy(&_cond);
}

void *LiDAR::thread_func(void *args)
{
    LiDAR *lidar = (LiDAR *) args;

    lidar->run();

    return NULL;
}

void LiDAR::processData(const void *buf, size_t size)
{
    const struct cldr_message *message = (const struct cldr_message *) buf;

    (void)(message);
    (void)(size);
}


void LiDAR::thread_wait_interruptible(unsigned int ms)
{
    struct timespec ts, twait;

    twait.tv_sec = ms / 1000;
    twait.tv_nsec = (twait.tv_sec - ms) * 1000000;

    clock_gettime(CLOCK_REALTIME, &ts);
    timespecadd(&ts, &twait, &ts);
    pthread_mutex_lock(&_mutex);
    pthread_cond_timedwait(&_cond, &_mutex, &ts);
    pthread_mutex_unlock(&_mutex);
}

void LiDAR::run(void)
{
    int ret;
    struct termios tty;
    int nfds;
    fd_set readfds;
    struct timeval timeout = {
        .tv_sec = 0,
        .tv_usec = 100000,
    };
    uint8_t buf[1024];
    unsigned int count = 1;

    while (_running) {
        if (_operational == false) {
            thread_wait_interruptible(1000);
            continue;
        }

        if (_handle == -1) {
            _handle = open(LIDAR_TTY, O_RDONLY);
            if (_handle == -1) {
                thread_wait_interruptible(1000);
                continue;
            }

            ret = tcgetattr(_handle, &tty);
            if (ret != 0) {
                perror("tgetattr");
                thread_wait_interruptible(1000);
                close(_handle);
                _handle = -1;
                continue;
            }

            cfsetospeed(&tty, LIDAR_BAUD_RATE);
            cfsetispeed(&tty, LIDAR_BAUD_RATE);
            tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
            tty.c_cc[VMIN]  = 0;
            tty.c_cc[VTIME] = 0;
            tty.c_iflag &= ~(IXON | IXOFF | IXANY);
            tty.c_cflag |= (CLOCAL | CREAD);
            tty.c_cflag &= ~(PARENB | PARODD);
            tty.c_cflag &= ~CSTOPB;
            tty.c_cflag &= ~CRTSCTS;

            tcflush(_handle, TCIFLUSH);
            ret = tcsetattr(_handle, TCSANOW, &tty);
            if (ret != 0) {
                perror("tcsetattr");
                thread_wait_interruptible(1000);
                close(_handle);
                _handle = -1;
                continue;
            }
        }

        FD_ZERO(&readfds);
        FD_SET(_handle, &readfds);
        nfds = _handle + 1;

        ret = select(nfds, &readfds, NULL, NULL, &timeout);
        if (ret <= 0) {
            continue;
        }

        if (FD_ISSET(_handle, &readfds)) {
            ret = read(_handle, &buf[count], 1);
            if (ret == 0) {
                printf("zero!\n");
                continue;
            } else if (ret < 0) {
                perror("read");
                thread_wait_interruptible(1000);
                close(_handle);
                _handle = -1;
                continue;
            }

            if (count == 1) {
                if (buf[count] == 0x55 && buf[count - 1] == 0xaa) {
                    count++;
                } else {
                    buf[count - 1] = buf[count];
                }
            } else {
                if (buf[count] == 0x55 && buf[count - 1] == 0xaa) {
                    processData(buf, count - 1);

                    buf[0] = 0xaa;  // Move the matched headers to front
                    buf[1] = 0x55;
                    count = 2;      // Continue with next packet
                } else {
                    count++;
                    if (count > sizeof(buf)) {
                        fprintf(stderr, "would overflow lidar buffer!\n");
                        buf[1] = buf[sizeof(buf) - 1];
                        count = 1;
                    }
                }
            }
        }
    }

    if (_handle) {
        close(_handle);
        _handle = -1;
    }
}

void LiDAR::start(void)
{
    if (_operational == false) {
        _operational = true;
        pthread_cond_broadcast(&_cond);
    }
}

void LiDAR::stop(void)
{
    if (_operational == true) {
        _operational = false;
    }
}

unsigned int LiDAR::speed(void) const
{
    return _speed;
}

void LiDAR::setSpeed(unsigned int speed)
{
    if (_speed == speed) {
        return;
    }

    _speed = speed;
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
