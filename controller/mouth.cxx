/*
 * mouth.cxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <stdio.h>
#include <sys/time.h>
#include <bsd/sys/time.h>
#include <pthread.h>
#include <pigpio.h>
#include "max7219_defs.h"
#include "mouth.hxx"

#define MAX7219_SPI_CHAN   0
#define MAX7219_SPI_SPEED  102400000
#define MAX7219_SPI_MODE                    \
    PI_SPI_FLAGS_BITLEN(0)   |              \
    PI_SPI_FLAGS_RX_LSB(0)   |              \
    PI_SPI_FLAGS_TX_LSB(0)   |              \
    PI_SPI_FLAGS_3WREN(0)    |              \
    PI_SPI_FLAGS_3WIRE(1)    |              \
    PI_SPI_FLAGS_AUX_SPI(0)  |              \
    PI_SPI_FLAGS_RESVD(0)    |              \
    PI_SPI_FLAGS_CSPOLS(0)   |              \
    PI_SPI_FLAGS_MODE(0)
#define MAX7219_COUNT      4

/*
 * MAX 7219:
 * https://www.sparkfun.com/datasheets/Components/General/COM-09622-MAX7219-MAX7221.pdf
 */

Mouth::Mouth()
    : _handle(-1)
{
    _handle = spiOpen(MAX7219_SPI_CHAN, MAX7219_SPI_SPEED, MAX7219_SPI_MODE);
    if (_handle < 0) {
        fprintf(stderr, "Open Max7219 SPI channel failed %d!\n", _handle);
        return;
    }

    writeMax7219(DECODE_MODE_REG, 0);
    writeMax7219(INTENSITY_REG, 3);
    writeMax7219(SCAN_LIMIT_REG, 7);
    writeMax7219(SHUTDOWN_REG, 1);
    writeMax7219(DISPLAY_TEST_REG, 0);

    _running = true;
    pthread_mutex_init(&_mutex, NULL);
    pthread_cond_init(&_cond, NULL);
    pthread_create(&_thread, NULL, Mouth::thread_func, this);
}

Mouth::~Mouth()
{
    _running = false;
    pthread_cond_broadcast(&_cond);
    pthread_join(_thread, NULL);
    pthread_mutex_destroy(&_mutex);
    pthread_cond_destroy(&_cond);

    if (_handle >= 0) {
        writeMax7219(SHUTDOWN_REG, 0);
        spiClose(_handle);
        _handle = -1;
    }
}

void *Mouth::thread_func(void *args)
{
    Mouth *mouth = (Mouth *) args;

    mouth->run();

    return NULL;
}

void Mouth::run(void)
{
    struct timespec ts, tloop;

    clock_gettime(CLOCK_REALTIME, &ts);
    tloop.tv_sec = 0;
    tloop.tv_nsec = 100000000;

    while (_running) {
        timespecadd(&ts, &tloop, &ts);
        pthread_mutex_lock(&_mutex);
        pthread_cond_timedwait(&_cond, &_mutex, &ts);
        pthread_mutex_unlock(&_mutex);
    }
}

void Mouth::writeMax7219(uint8_t reg, uint8_t data) const
{
    int ret;
    unsigned int i;
    char xmit[MAX7219_COUNT * 2];

    for (i = 0; i < MAX7219_COUNT; i++) {
        xmit[i * 2 + 0] = reg;
        xmit[i * 2 + 1] = data;
    }

    ret = spiWrite(_handle, xmit, sizeof(xmit));
    if (ret != sizeof(xmit)) {
            fprintf(stderr, "Mouth::writeMax7219 failed %d\n", ret);
    }
}

void Mouth::displayText(const char *text, bool scroll, unsigned int speed)
{
    (void)(text);
    (void)(scroll);
    (void)(speed);
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
