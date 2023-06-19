/*
 * mouth.cxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <bsd/sys/time.h>
#include <pthread.h>
#include <pigpio.h>
#include "max7219_defs.h"
#include "mouth.hxx"

#define MAX7219_SPI_CHAN   0
#define MAX7219_SPI_SPEED  1000000
#define MAX7219_SPI_MODE                    \
    PI_SPI_FLAGS_BITLEN(0)   |              \
    PI_SPI_FLAGS_RX_LSB(0)   |              \
    PI_SPI_FLAGS_TX_LSB(0)   |              \
    PI_SPI_FLAGS_3WREN(0)    |              \
    PI_SPI_FLAGS_3WIRE(0)    |              \
    PI_SPI_FLAGS_AUX_SPI(0)  |              \
    PI_SPI_FLAGS_RESVD(0)    |              \
    PI_SPI_FLAGS_CSPOLS(0)   |              \
    PI_SPI_FLAGS_MODE(0)
#define MAX7219_COUNT      4

#define ALWAYS_REFRESH_ALL

/*
 * MAX 7219:
 * https://www.sparkfun.com/datasheets/Components/General/COM-09622-MAX7219-MAX7221.pdf
 *
 * yellow    din   mosi(10)
 * green     cs    spi_ce0(8)
 * blue      clk   spi_clk(11)
 */

enum mouth_mode {
    MOUTH_BEH,
    MOUTH_SMILE,
    MOUTH_CYLON,
    MOUTH_SPEAK,
    MOUTH_TEXT,
};

static unsigned int instance = 0;

Mouth::Mouth()
  : _handle(-1),
    _intensity(1),
    _fb(),
    _mode(MOUTH_BEH)
{
    if (instance != 0) {
        fprintf(stderr, "Mouth can be instantiated only once!\n");
        exit(EXIT_FAILURE);
    } else {
        instance++;
    }

    _handle = spiOpen(MAX7219_SPI_CHAN, MAX7219_SPI_SPEED, MAX7219_SPI_MODE);
    if (_handle < 0) {
        fprintf(stderr, "Open Max7219 SPI channel failed %d!\n", _handle);
        return;
    }

    writeMax7219(DECODE_MODE_REG, 0);
    writeMax7219(INTENSITY_REG, _intensity);
    writeMax7219(SCAN_LIMIT_REG, 7);
    writeMax7219(SHUTDOWN_REG, 1);
    writeMax7219(DISPLAY_TEST_REG, 0);
    writeMax7219(DIGIT0_REG, 0);
    writeMax7219(DIGIT1_REG, 0);
    writeMax7219(DIGIT2_REG, 0);
    writeMax7219(DIGIT3_REG, 0);
    writeMax7219(DIGIT4_REG, 0);
    writeMax7219(DIGIT5_REG, 0);
    writeMax7219(DIGIT6_REG, 0);
    writeMax7219(DIGIT7_REG, 0);

    beh();

    _running = true;
    pthread_mutex_init(&_mutex, NULL);
    pthread_cond_init(&_cond, NULL);
    pthread_create(&_thread, NULL, Mouth::thread_func, this);
    pthread_setname_np(_thread, "R'Mouth");

    printf("Mouth is online\n");
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

    instance--;
    printf("Mouth is offline\n");
}

void *Mouth::thread_func(void *args)
{
    Mouth *mouth = (Mouth *) args;

    mouth->run();

    return NULL;
}

void Mouth::behCycle(unsigned int cycle)
{
    static const uint32_t bitmap[][8] = {
        {
            0x00000000,
            0x00000000,
            0x00000000,
            0x0007e000,
            0x03ffffc0,
            0x0007e000,
            0x00000000,
            0x00000000,
        }, {
            0x00000000,
            0x00000000,
            0x00000000,
            0x0003c000,
            0x07ffffe0,
            0x0003c000,
            0x00000000,
            0x00000000,
        }, {
            0x00000000,
            0x00000000,
            0x00000000,
            0x00018000,
            0x0ffffff0,
            0x00018000,
            0x00000000,
            0x00000000,
        }, {
            0x00000000,
            0x00000000,
            0x00000000,
            0x00000000,
            0x1ffffff8,
            0x00000000,
            0x00000000,
            0x00000000,
        },
    };
    static int x = 0, dir = 0;

    if ((cycle % 20) == 0) {
        draw(&bitmap[x][0]);

        if (dir == 0) {
            x++;
        } else {
            x--;
        }

        if (x >= (int) ((sizeof(bitmap) / 32) - 1)) {
            x = (sizeof(bitmap) / 32) - 1;
            dir = 1;
        } else if (x < 0) {
            x = 1;
            dir = 0;
        }
    }
}

void Mouth::smileCycle(unsigned int cycle)
{
    static const uint32_t bitmap[][8] = {
        {
            0xc0000003,
            0xc0000003,
            0x3000000c,
            0x3000000c,
            0x0c000030,
            0x0c000030,
            0x03ffffc0,
            0x03ffffc0,
        }, {
            0x00000000,
            0xc0000003,
            0x3000000c,
            0x3000000c,
            0x0c000030,
            0x0c000030,
            0x03ffffc0,
            0x03ffffc0,
        }, {
            0x00000000,
            0x00000000,
            0x00000000,
            0x3000000c,
            0x0c000030,
            0x0c000030,
            0x03ffffc0,
            0x03ffffc0,
        }, {
            0x00000000,
            0x00000000,
            0x00000000,
            0x00000000,
            0x0c000030,
            0x0c000030,
            0x03ffffc0,
            0x03ffffc0,
        }, {
            0x00000000,
            0x00000000,
            0x00000000,
            0x00000000,
            0x00000000,
            0x0c000030,
            0x03ffffc0,
            0x03ffffc0,
        }, {
            0x00000000,
            0x00000000,
            0x00000000,
            0x00000000,
            0x00000000,
            0x00000000,
            0x03ffffc0,
            0x03ffffc0,
        },
    };
    static int x = 0, dir = 0;

    if ((cycle % 20) == 0) {
        draw(&bitmap[x][0]);

        if (dir == 0) {
            x++;
        } else {
            x--;
        }

        if (x >= (int) ((sizeof(bitmap) / 32) - 1)) {
            x = (sizeof(bitmap) / 32) - 1;
            dir = 1;
        } else if (x < 0) {
            x = 0;
            dir = 0;
        }
    }
}

void Mouth::cylonCycle(unsigned int cycle)
{
    static int x = 0, dir = 0;
    unsigned int i;


    if (cycle) {
        for (i = 0; i < 8; i++) {
            _fb[i] = (0x1 << x);
        }

        if (dir == 0) {
            x++;
        } else {
            x--;
        }

        if (x > 31) {
            x = 31;
            dir = 1;
        } else if (x < 0) {
            x = 0;
            dir = 0;
        }
    }
}

void Mouth::speakCycle(unsigned int cycle)
{
    static const uint32_t bitmap[][8] = {
        {
            0x00000000,
            0x3ffffffc,
            0x3000000c,
            0x3000000c,
            0x3000000c,
            0x3000000c,
            0x3ffffffc,
            0x00000000,
        }, {
            0x00000000,
            0x00000000,
            0x3ffffffc,
            0x3000000c,
            0x3000000c,
            0x3000000c,
            0x3ffffffc,
            0x00000000,
        }, {
            0x00000000,
            0x00000000,
            0x3ffffffc,
            0x3000000c,
            0x3000000c,
            0x3ffffffc,
            0x00000000,
            0x00000000,
        }, {
            0x00000000,
            0x00000000,
            0x00000000,
            0x3ffffffc,
            0x3000000c,
            0x3ffffffc,
            0x00000000,
            0x00000000,
        }, {
            0x00000000,
            0x00000000,
            0x00000000,
            0x3ffffffc,
            0x3ffffffc,
            0x00000000,
            0x00000000,
            0x00000000,
        },
    };
    static int x = 0, dir = 0;

    if ((cycle % 5) == 0) {
        draw(&bitmap[x][0]);

        if (dir == 0) {
            x++;
        } else {
            x--;
        }

        if (x >= (int) ((sizeof(bitmap) / 32) - 1)) {
            x = (sizeof(bitmap) / 32) - 1;
            dir = 1;
        } else if (x < 0) {
            x = 0;
            dir = 0;
        }
    }
}

void Mouth::textCycle(unsigned int cycle)
{
    (void)(cycle);
}

void Mouth::run(void)
{
    struct timespec ts, tloop;
    unsigned int l_intensity;
    uint32_t l_fb[8];
    char xmit[MAX7219_COUNT * 2];
    unsigned int cycle;

    l_intensity = _intensity;
    memset(l_fb, 0x0, sizeof(l_fb));

    clock_gettime(CLOCK_REALTIME, &ts);
    tloop.tv_sec = 0;
    tloop.tv_nsec = 20000000;

    for (cycle = 0; _running; cycle++) {
        int ret;
        unsigned int i;

        timespecadd(&ts, &tloop, &ts);

        switch (_mode) {
        case MOUTH_BEH:
            behCycle(cycle);
            break;
        case MOUTH_SMILE:
            smileCycle(cycle);
            break;
        case MOUTH_CYLON:
            cylonCycle(cycle);
            break;
        case MOUTH_SPEAK:
            speakCycle(cycle);
            break;
        case MOUTH_TEXT:
            textCycle(cycle);
            break;
        default:
            break;
        }

        /* Periodically reprogram the registers */
        if (cycle % 256 == 0) {
            writeMax7219(DECODE_MODE_REG, 0);
            writeMax7219(INTENSITY_REG, _intensity);
            writeMax7219(SCAN_LIMIT_REG, 7);
            writeMax7219(SHUTDOWN_REG, 1);
            writeMax7219(DISPLAY_TEST_REG, 0);
#if !defined(ALWAYS_REFRESH_ALL)
            for (i = 0; i < 8; i++) {
                l_fb[i] = ~_fb[i];  // Flip bits to force refresh
            }
#endif
        }

        /* Update intensity */
        if (l_intensity != _intensity) {
            l_intensity = _intensity;
            writeMax7219(INTENSITY_REG, l_intensity);
        }

#if defined(ALWAYS_REFRESH_ALL)
            for (i = 0; i < 8; i++) {
                l_fb[i] = ~_fb[i];  // Flip bits to force refresh
            }
#endif

        /* Update frame buffer */
        for (i = 0; i < 8; i++) {
            if (l_fb[i] != _fb[i]) {
                l_fb[i] = _fb[i];
                xmit[0] = DIGIT7_REG - i;
                xmit[1] = (uint8_t) (l_fb[i] >> 24);
                xmit[2] = DIGIT7_REG - i;
                xmit[3] = (uint8_t) (l_fb[i] >> 16);
                xmit[4] = DIGIT7_REG - i;
                xmit[5] = (uint8_t) (l_fb[i] >> 8);
                xmit[6] = DIGIT7_REG - i;
                xmit[7] = (uint8_t) (l_fb[i] >> 0);

                ret = spiWrite(_handle, xmit, sizeof(xmit));
                if (ret != sizeof(xmit)) {
                    fprintf(stderr, "Mouth::writeMax7219 failed %d\n", ret);
                }
            }
        }

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

unsigned int Mouth::intensity(void) const
{
    return _intensity;
}

void Mouth::setIntensity(unsigned int intensity)
{
    _intensity = intensity;
}

void Mouth::draw(const uint32_t fb[8])
{
    memcpy(_fb, fb, sizeof(_fb));
}

unsigned int Mouth::mode(void) const
{
    return _mode;
}

void Mouth::setMode(unsigned int mode)
{
    _mode = mode;
}

void Mouth::beh(void)
{
    setMode(MOUTH_BEH);
}

void Mouth::smile(void)
{
    setMode(MOUTH_SMILE);
}

void Mouth::cylon(void)
{
    setMode(MOUTH_CYLON);
}

void Mouth::speak(void)
{
    setMode(MOUTH_SPEAK);
}

void Mouth::displayText(const char *text, bool scroll, unsigned int speed)
{
    _text = text;
    _scroll = scroll;
    _speed = speed;
    setMode(MOUTH_TEXT);
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
