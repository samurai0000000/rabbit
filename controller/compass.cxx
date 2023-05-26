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

#define MIN_X_VAL -1227
#define MAX_X_VAL 935
#define MIN_Y_VAL -1543
#define MAX_Y_VAL 711
#define MIN_Z_VAL -760
#define MAX_Z_VAL 922

static unsigned int instance = 0;

Compass::Compass()
    : _handle(-1)
{
    if (instance != 0) {
        fprintf(stderr, "Compass can be instantiated only once!\n");
        exit(EXIT_FAILURE);
    } else {
        instance++;
    }

    _minX = MIN_X_VAL;
    _maxX = MAX_X_VAL;
    _minY = MIN_Y_VAL;
    _maxY = MAX_Y_VAL;
    _minZ = MIN_Z_VAL;
    _maxZ = MAX_Z_VAL;
    _offsetX = (_minX + _maxX) / 2;
    _offsetY = (_minY + _maxY) / 2;
    _offsetZ = (_minZ + _maxZ) / 2;
    _avgDeltaX = (_maxX - _minX) / 2;
    _avgDeltaY = (_maxY - _minY) / 2;
    _avgDeltaZ = (_maxZ - _minZ) / 2;
    _avgDelta = (_avgDeltaX + _avgDeltaY + _avgDeltaZ) / 3;
    if (_avgDelta == 0.0) {
        _scaleX = _scaleY = _scaleZ = 1.0;
    } else {
        _scaleX = (float) _avgDelta / (float) _avgDeltaX;
        _scaleY = (float) _avgDelta / (float) _avgDeltaY;
        _scaleZ = (float) _avgDelta / (float) _avgDeltaZ;
    }

    _running = true;
    pthread_mutex_init(&_mutex, NULL);
    pthread_cond_init(&_cond, NULL);
    pthread_create(&_thread, NULL, Compass::thread_func, this);

    printf("Compass is online\n");
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
        _handle = -1;
    }

    instance--;
    printf("Compass is offline\n");
}

void Compass::probeOpenDevice(void)
{
    int ret = 0;
    uint8_t chipid;

    if (_handle == -1) {
        _handle = i2cOpen(COMPASS_I2C_BUS, COMPASS_I2C_ADDR, 0x0);
        if (_handle < 0) {
            _handle = -1;
            goto done;
        }

        if (i2cReadByteData(_handle, 0x0) < 0) {
            i2cClose(_handle);
            _handle = -1;
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
    }

done:

    if (_handle != -1 && ret != 0) {
        i2cClose(_handle);
        _handle = -1;
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
    int ret = 0;
    uint8_t status;
    int16_t x, y, z;
    struct timespec ts, tloop;
    bool dirtyX, dirtyY, dirtyZ;
    float heading_now;

    clock_gettime(CLOCK_REALTIME, &ts);
    tloop.tv_sec = 0;
    tloop.tv_nsec = 50000000;

    while (_running) {
        timespecadd(&ts, &tloop, &ts);

        /* Probe and open device */
        if (_handle == -1) {
            probeOpenDevice();
            if (_handle == -1) {
                goto done;
            }
        }

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

        dirtyX = false;
        dirtyY = false;
        dirtyZ = false;

        if (x < _minX) {
            _minX = x;
            dirtyX = true;
        }

        if (x > _maxX) {
            _maxX = x;
            dirtyX = true;
        }

        if (dirtyX) {
            _offsetX = (_minX + _maxX) / 2;
            _avgDeltaX = (_maxX - _minX) / 2;
        }

        if (y < _minY) {
            _minY = y;
            dirtyY = true;
        }

        if (y > _maxY) {
            _maxY = y;
            dirtyY = true;
        }

        if (dirtyY) {
            _offsetY = (_minY + _maxY) / 2;
            _avgDeltaY = (_maxY - _minY) / 2;
        }

        if (z < _minZ) {
            _minZ = z;
            dirtyZ = true;
        }

        if (z > _maxZ) {
            _maxZ = z;
            dirtyZ = true;
        }

        if (dirtyZ) {
            _offsetZ = (_minZ + _maxZ) / 2;
            _avgDeltaZ = (_maxZ - _minZ) / 2;
        }

        if (dirtyX || dirtyY || dirtyZ) {
            _avgDelta = (_avgDeltaX + _avgDeltaY + _avgDeltaZ) / 3;
            _scaleX = (float) _avgDelta / (float) _avgDeltaX;
            _scaleY = (float) _avgDelta / (float) _avgDeltaY;
            _scaleZ = (float) _avgDelta / (float) _avgDeltaZ;
        }

        //printf("%d <= x <= %d, %d <= y <=%d, %d <= z <= %d\n",
        //       _minX, _maxX, _minY, _maxY, _minZ, _maxZ);

        _histX.addSample(((float) x - _offsetX) * _scaleX);
        _histY.addSample(((float) y - _offsetY) * _scaleY);
        _histZ.addSample(((float) z - _offsetZ) * _scaleZ);

        heading_now = heading();
        mosquitto->publish("rabbit/compass/heading",
                           sizeof(float), &heading_now, 2, 0);

        ret = 0;

    done:

        if (ret != 0) {
            i2cClose(_handle);
            _handle = -1;
        }

        pthread_mutex_lock(&_mutex);
        pthread_cond_timedwait(&_cond, &_mutex, &ts);
        pthread_mutex_unlock(&_mutex);
    }
}

float Compass::x(void)
{
    return _histX.average();
}

float Compass::y(void)
{
    return _histY.average();
}

float Compass::z(void)
{
    return _histZ.average();
}

float Compass::heading(void)
{
    float heading;

    heading = atan2f(y(), x()) * 180.0 / M_PI;
    if (heading < 0) {
        heading += 360.0;
    }

    return heading;
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
