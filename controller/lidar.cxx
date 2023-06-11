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
#include "rabbit.hxx"

/*
 * CLDR-08SB
 *
 * https://www.csensor.com/product-cldr
 * https://www.icshop.com.tw/pd/368030200713/datasheet.pdf
 */

#define LIDAR_TTY        "/dev/ttyUSB0"
#define LIDAR_BAUD_RATE         B115200
#define LIDAR_ON_SERVO               18
#define LIDAR_ON_LO_PULSE             0
#define LIDAR_ON_HI_PULSE         19990
#define LIDAR_ROT_SERVO              19
#define LIDAR_ROT_LO_PULSE         6000
#define LIDAR_ROT_HI_PULSE        19990

struct cldr_message {
    uint16_t ph;
    uint8_t ct;
    uint8_t lsn;
    uint16_t fsa;
    uint16_t lsa;
    uint16_t cs;
} __attribute__ ((packed));

static unsigned int instance = 0;

LiDAR::LiDAR()
    : _handle(-1),
      _operational(false),
      _speed(25),
      _rpm(0)
{
    if (instance != 0) {
        fprintf(stderr, "LiDAR can be instantiated only once!\n");
        exit(EXIT_FAILURE);
    } else {
        instance++;
    }

    servos->setRange(LIDAR_ON_SERVO, LIDAR_ON_LO_PULSE, LIDAR_ON_HI_PULSE);
    if (_operational) {
        servos->setPulse(LIDAR_ON_SERVO, LIDAR_ON_HI_PULSE);
    } else {
        servos->setPulse(LIDAR_ON_SERVO, LIDAR_ON_LO_PULSE);
    }

    servos->setRange(LIDAR_ROT_SERVO, LIDAR_ROT_LO_PULSE, LIDAR_ROT_HI_PULSE);
    setSpeed(_speed);

    _running = true;
    pthread_mutex_init(&_mutex, NULL);
    pthread_cond_init(&_cond, NULL);
    pthread_create(&_thread, NULL, LiDAR::thread_func, this);
    pthread_setname_np(_thread, "R'LiDAR");

    printf("LiDAR is online\n");
}

LiDAR::~LiDAR()
{
    _running = false;
    pthread_cond_broadcast(&_cond);
    pthread_join(_thread, NULL);
    pthread_mutex_destroy(&_mutex);
    pthread_cond_destroy(&_cond);

    instance--;
    printf("LiDAR is offline\n");
}

void *LiDAR::thread_func(void *args)
{
    LiDAR *lidar = (LiDAR *) args;

    lidar->run();

    return NULL;
}

bool LiDAR::processData(const uint8_t *buf, size_t size)
{
    const struct cldr_message *message = (const struct cldr_message *) buf;
    unsigned int i;
    unsigned int lsn;
    uint16_t cs;
    double fsa;
    double lsa;
    double anglepp;
    const uint16_t *si;
    double distance;
    double angle;

    if (size < sizeof(struct cldr_message)) {
        fprintf(stderr, "LiDAR short packet %zu\n", size);
        return false;  /* Reject */
    }

    if (size % 2 != 0) {
        fprintf(stderr, "LiDAR odd packet size %zu\n", size);
        return false;  /* Reject */
    }

    if (0) {
        // Debug: print the packet
        const uint8_t *raw = (const uint8_t *) buf;
        for (i = 0; i < size; i++) {
            printf("%.2x ", raw[i]);
        }
        printf("\n");
    }

    if (message->ph != 0x55aa) {
        fprintf(stderr, "LiDAR wrong header!\n");
        return false;  /* Reject */
    }

    lsn = message->lsn;

    if (size != ((sizeof(struct cldr_message) + lsn * 2))) {
        fprintf(stderr, "LiDAR packet size %zu mispatches lsn %u\n",
                size, lsn);
        return false;  /* Reject */
    }

    cs = 0x0;

    for (i = 0; i < 8; i += 2) {
        cs = cs ^ (buf[i] + (buf[i + 1] << 8));
    }

    for (i = 10; i < (10 + lsn * 2); i += 2) {
        cs = cs ^ (buf[i] + (buf[i + 1] << 8));
    }

    if (cs != message->cs) {
        fprintf(stderr, "LiDAR checksum mismatch 0x%.4x != 0x%.4x\n",
                cs, message->cs);
        return false;  /* Reject */
    }

    /* Grab new RPM info */
    if ((message->ct & 0x1) == 0x1) {
        _rpm = (message->ct >> 1) * 60 / 10;
    }

    fsa = (double) (message->fsa >> 1) / 64;
    lsa = (double) (message->lsa >> 1) / 64;
    anglepp = lsa - fsa;
    if (anglepp < 0.0) {
        anglepp += 360.0;
    }
    anglepp /= (double) (lsn - 1);
    //printf("fsa=%.2f lsa=%.2f anglepp=%.2f\n", fsa, lsa, anglepp);

    angle = fsa;
    for (i = 0, si = (const uint16_t *) &buf[sizeof(struct cldr_message)];
         i < lsn;
         i++, si++) {
        distance = (double) (*si / 4);
        if (0) {
            printf("(%.0f, %.2f)\n", distance, angle);
        }
        angle += anglepp;
        if (angle > 360.0) {
            angle -= 360.0;
        }
    }

    return true;
}


void LiDAR::thread_wait_interruptible(unsigned int ms)
{
    struct timespec ts, twait;

    twait.tv_sec = ms / 1000;
    twait.tv_nsec = ((twait.tv_sec * 1000) - ms) * 1000000;

    clock_gettime(CLOCK_REALTIME, &ts);
    timespecadd(&ts, &twait, &ts);
    pthread_mutex_lock(&_mutex);
    pthread_cond_timedwait(&_cond, &_mutex, &ts);
    pthread_mutex_unlock(&_mutex);
}

void LiDAR::run(void)
{
    int ret;
    struct timeval now, tlast, tdiff;
    struct termios tty;
    int nfds;
    fd_set readfds;
    struct timeval timeout;
    uint8_t buf[1024];
    unsigned int pos = 0;
    unsigned int lsn;
    unsigned int need = sizeof(struct cldr_message);
    unsigned int points_received = 0;

    gettimeofday(&now, NULL);
    memcpy(&tlast, &now, sizeof(struct timeval));

    while (_running) {
        /* Update statistics */
        gettimeofday(&now, NULL);
        timersub(&now, &tlast, &tdiff);
        if (tdiff.tv_sec >= 1) {
            _pps =   // Approximate, don't need fractional second
                points_received;
            memcpy(&tlast, &now, sizeof(struct timeval));
            points_received = 0;
        }

        if (_operational == false) {
            thread_wait_interruptible(1000);
            continue;
        }

        if (_handle == -1) {
            _handle = open(LIDAR_TTY, O_RDWR | O_NOCTTY);
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

            cfmakeraw(&tty);

            tty.c_cc[VMIN]  = 1;
            tty.c_cc[VTIME] = 10;
            tty.c_cflag &= ~CSTOPB;
            tty.c_cflag &= ~CRTSCTS;
            tty.c_cflag |= (CLOCAL | CREAD);

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

        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        ret = select(nfds, &readfds, NULL, NULL, &timeout);
        if (ret == -1) {
            perror("select");
            thread_wait_interruptible(1000);
            close(_handle);
            _handle = -1;
            continue;
        }

        if (ret == 0) {
            continue; /* Timeout with no input */
        }

        if (!FD_ISSET(_handle, &readfds)) {
            fprintf(stderr, "descriptor not set!\n");
            thread_wait_interruptible(1000);
            close(_handle);
            _handle = -1;
            continue;
        }

        ret = read(_handle, &buf[pos], 1);
        if (ret != 1) {
            fprintf(stderr, "read ret=%d %s\n", ret, strerror(errno));
            thread_wait_interruptible(1000);
            close(_handle);
            _handle = -1;
            continue;
        }

        if (pos == 0) {
            if (buf[pos] != 0xaa) {
                pos = 0;   /* Rewind */
            } else {
                pos++;
            }
            continue;
        } else if (pos == 1) {
            if (buf[pos] != 0x55) {
                pos = 0;   /* Rewind */
            } else {
                pos++;
            }
            continue;
        } else if (pos == 2) {
            pos++;
            continue;
        } else if (pos == 3) {
            lsn = (unsigned int) buf[pos];
            need = sizeof(struct cldr_message) + (lsn * 2) - 4;
            pos++;
            continue;
        }

        need--;
        pos++;

        if (need == 0) {
            if (processData(buf, pos) == true) {
                /* Update statistics */
                const struct cldr_message *message =
                    (const struct cldr_message *) buf;
                points_received += (unsigned int) message->lsn;
            }

            /* Reset input buffer */
            pos = 0;
            need = sizeof(struct cldr_message);
        }
    }

    if (_handle) {
        close(_handle);
        _handle = -1;
    }
}

void LiDAR::enable(bool en)
{
    en = en ? true : false;

    if (en == true && _operational == false) {
        _operational = true;
        servos->setPulse(LIDAR_ON_SERVO, LIDAR_ON_HI_PULSE);
        pthread_cond_broadcast(&_cond);
        head->earsBack();
        LOG("LiDAR enabled\n");
        speech->speak("Lie Dar enabled");
    } else if (en == false && _operational == true) {
        _operational = false;
        servos->setPulse(LIDAR_ON_SERVO, LIDAR_ON_LO_PULSE);
        head->earsUp();
        LOG("LiDAR disabled\n");
        speech->speak("Lie Dar disabled");
    }
}

unsigned int LiDAR::speed(void) const
{
    return _speed;
}

void LiDAR::setSpeed(unsigned int speed)
{
    unsigned int pulse;

    if (speed > 100) {
        speed = 100;
    }

    pulse = (speed * ((LIDAR_ROT_HI_PULSE - LIDAR_ROT_LO_PULSE)) / 100) +
        LIDAR_ROT_LO_PULSE;
    servos->setPulse(LIDAR_ROT_SERVO, pulse);
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
