/*
 * compass.cxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <sys/time.h>
#include <bsd/sys/time.h>
#include <math.h>
#include <pigpio.h>
#include "rabbit.hxx"
#include "qmc5883l_defs.h"

/*
 * QMC5883L 3 axis magnetic sensor
 * https://datasheet.lcsc.com/szlcsc/QST-QMC5883L-TR_C192585.pdf
 */
#define COMPASS_I2C_BUS    1
#define COMPASS_I2C_ADDR   0x0d

Compass::Compass()
    : _handle(-1)
{
    int ret;
    uint8_t chipid;

    _handle = i2cOpen(COMPASS_I2C_BUS, COMPASS_I2C_ADDR, 0x0);
    if (_handle < 0) {
        goto done;
    }

    ret = i2cReadByteData(_handle, CHIPID_REG);
    if (ret < 0) {
        fprintf(stderr, "QMC5883L read CHIPID_REG failed!\n");
        goto done;
    } else {
        chipid = (uint8_t) ret;
    }

    if (chipid != CHIPID_VAL) {
        fprintf(stderr, "QMC5883L chip ID incorrect (0x%.2x)!\n", chipid);
        goto done;
    }

    ret = i2cWriteByteData(_handle, CTRL_2_REG, SOFT_RST);
    if (ret != 0) {
        fprintf(stderr, "QMC5883L write CTRL_2_REG failed!\n");
        goto done;
    }

    ret = i2cWriteByteData(_handle, CTRL_1_REG,
                           OSR_512 | RNG_8G | ODR_100HZ | MODE_CONTINUOUS);
    if (ret != 0) {
        fprintf(stderr, "QMC5883L write CTRL_1_REG failed!\n");
        goto done;
    }

    ret = i2cWriteByteData(_handle, SR_PERIOD_REG, 0x01);
    if (ret != 0) {
        fprintf(stderr, "QMC5883L write SR_PERIOD_REG failed!\n");
        goto done;
    }

done:

    _running = true;
    pthread_mutex_init(&_mutex, NULL);
    pthread_cond_init(&_cond, NULL);
    pthread_create(&_thread, NULL, Compass::thread_func, this);
}

Compass::~Compass()
{
    _running = false;
    pthread_cond_broadcast(&_cond);
    pthread_join(_thread, NULL);
    pthread_mutex_destroy(&_mutex);
    pthread_cond_destroy(&_cond);

    if (_handle >= 0) {
        i2cWriteByteData(_handle, CTRL_2_REG, SOFT_RST);
        i2cClose(_handle);
    }
}

void *Compass::thread_func(void *args)
{
    Compass *compass = (Compass *) args;

    compass->run();

    return NULL;
}

void Compass::run(void)
{
    int ret;
    uint8_t status;
    int16_t x, y, z;
    struct timespec ts, tloop;

    clock_gettime(CLOCK_REALTIME, &ts);
    tloop.tv_sec = 0;
    tloop.tv_nsec = 200000000;

    while (_running) {
        timespecadd(&ts, &tloop, &ts);

        /* Check status register for data ready */
        ret = i2cReadByteData(_handle, STATUS_REG);
        if (ret < 0) {
            fprintf(stderr, "QMC5883L read STATUS_REG failed!\n");
            goto done;
        } else {
            status = (uint8_t) ret;
        }

        if ((status & STATUS_DRDY) == 0) {
            goto done;
        }

        /* Request read from first register */
        ret = i2cWriteByte(_handle, DATA_X_LSB_REG);
        if (ret != 0) {
            fprintf(stderr, "QMC5883L write byte DATA_X_LSB_REG failed!\n");
            goto done;
        }

        /*
         * Read 6 consecutive bytes for x/y/z axis
         */

        ret = i2cReadByte(_handle);
        if (ret < 0) {
            fprintf(stderr, "QMC5883L read byte failed!\n");
            goto done;
        }
        x = ret;

        ret = i2cReadByte(_handle);
        if (ret < 0) {
            fprintf(stderr, "QMC5883L read byte failed!\n");
            goto done;
        }
        x |= (ret << 8);

        ret = i2cReadByte(_handle);
        if (ret < 0) {
            fprintf(stderr, "QMC5883L read byte failed!\n");
            goto done;
        }
        y = ret;

        ret = i2cReadByte(_handle);
        if (ret < 0) {
            fprintf(stderr, "QMC5883L read byte failed!\n");
            goto done;
        }
        y |= (ret << 8);

        ret = i2cReadByte(_handle);
        if (ret < 0) {
            fprintf(stderr, "QMC5883L read byte failed!\n");
            goto done;
        }
        z = ret;

        ret = i2cReadByte(_handle);
        if (ret < 0) {
            fprintf(stderr, "QMC5883L read byte failed!\n");
            goto done;
        }
        z |= (ret << 8);

        /*
         * Normalize and calculate the bearing
         */

        _histX.addSample(x);
        _histY.addSample(y);
        _histZ.addSample(z);

    done:

        pthread_mutex_lock(&_mutex);
        pthread_cond_timedwait(&_cond, &_mutex, &ts);
        pthread_mutex_unlock(&_mutex);
    }
}

float Compass::x(void)
{
    return (float) _histX.median() * 360.0 / 32768.0;
}

float Compass::y(void)
{
    return (float) _histY.median() * 360.0 / 32768.0;
}

float Compass::z(void)
{
    return (float) _histZ.median() * 360.0 / 32768.0;
}

float Compass::bearing(void)
{
    float x, y, bearing;

    x = this->x();
    y = this->y();

    bearing = atan2f(y, x) * 180.0 / M_PI;
    if (bearing < 0) {
        bearing += 360.0;
    }

    return bearing;
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
