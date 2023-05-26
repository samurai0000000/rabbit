/*
 * ambience.cxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <stdlib.h>
#include <sys/time.h>
#include <bsd/sys/time.h>
#include <pigpio.h>
#include "rabbit.hxx"

/*
 * BME280
 * https://www.mouser.com/datasheet/2/783/BST-BME280-DS002-1509607.pdf
 *
 * 4pin:
 *   red    3.3v
 *   black   gnd
 *   oragne  scl
 *   brown   sda
 */
#define BME280_I2C_BUS  1
#define BME280_I2C_ADDR 0x76

static int8_t user_i2c_read(uint8_t reg_addr, uint8_t *data,
                            uint32_t len, void *intf_ptr)
{
    int ret = 0;
    int handle;

    handle = (int) (intptr_t) intf_ptr;
    if (handle == -1) {
        return BME280_E_COMM_FAIL;
    }

    while (len > 0) {
        //printf("i2c read 0x%.2x\n", reg_addr);
        ret = i2cReadByteData(handle, reg_addr);
        if (ret < 0) {
            fprintf(stderr, "BME280 read reg 0x%.2x failed!\n", reg_addr);
            return BME280_E_COMM_FAIL;
        } else {
            *data = (uint8_t) ret;
        }
        reg_addr++;
        data++;
        len--;
    }

    return BME280_OK;
}

static int8_t user_i2c_write(uint8_t reg_addr, const uint8_t *data,
                             uint32_t len, void *intf_ptr)
{
    int ret = 0;
    int handle;

    handle = (int) (intptr_t) intf_ptr;
    if (handle == -1) {
        return BME280_E_COMM_FAIL;
    }

    while (len > 0) {
        //printf("i2c write 0x%.2x 0x%.2x\n", reg_addr, *data);
        ret = i2cWriteByteData(handle, reg_addr, *data);
        if (ret < 0) {
            fprintf(stderr, "BME280 read reg 0x%.2x failed!\n", reg_addr);
            return BME280_E_COMM_FAIL;
        }
        reg_addr++;
        data++;
        len--;
    }

    return BME280_OK;
}

static void user_delay_us(uint32_t period, void *intf_ptr)
{
    (void)(intf_ptr);
    usleep(period);
}

Ambience::Ambience()
    : _dev(),
      _settings(),
      _bme280_delay_us(0),
      _bme280(-1),
      _running(false)
{
    _running = true;
    pthread_mutex_init(&_mutex, NULL);
    pthread_cond_init(&_cond, NULL);
    pthread_create(&_thread, NULL, Ambience::thread_func, this);
}

Ambience::~Ambience()
{
    _running = false;
    pthread_cond_broadcast(&_cond);
    pthread_join(_thread, NULL);
    pthread_mutex_destroy(&_mutex);
    pthread_cond_destroy(&_cond);

    if (_bme280 >= 0) {
        i2cClose(_bme280);
        _bme280 = -1;
    }
}

void *Ambience::thread_func(void *args)
{
    Ambience *ambience = (Ambience *) args;

    ambience->run();

    return NULL;
}

void Ambience::probeOpenDevice(void)
{
    int8_t rslt = BME280_OK;

    if (_bme280 != -1) {
        return;
    }

    _bme280 = i2cOpen(BME280_I2C_BUS, BME280_I2C_ADDR, 0);
    if (_bme280 < 0) {
        _bme280 = -1;
        goto bme280_done;
    }

    if (i2cReadByteData(_bme280, 0x0) < 0) {
        i2cClose(_bme280);
        _bme280 = -1;
        goto bme280_done;
    }

    /* Initialize the bme280 */
    _dev.intf_ptr = (void *) (intptr_t) _bme280;
    _dev.intf = BME280_I2C_INTF;
    _dev.read = user_i2c_read;
    _dev.write = user_i2c_write;
    _dev.delay_us = user_delay_us;
    rslt = bme280_init(&_dev);
    if (rslt != BME280_OK) {
        fprintf(stderr, "bme280_init failed (%+d).\n", rslt);
        goto bme280_done;
    }

    /* Get the current sensor settings */
    rslt = bme280_get_sensor_settings(&_settings, &_dev);
    if (rslt != BME280_OK) {
        fprintf(stderr, "bme280_get_sensor_settings failed (%+d)\n", rslt);
        goto bme280_done;
    }

    /* Recommended mode of operation: Indoor navigation */
    _settings.filter = BME280_FILTER_COEFF_16;
    _settings.osr_h = BME280_OVERSAMPLING_1X;
    _settings.osr_p = BME280_OVERSAMPLING_16X;
    _settings.osr_t = BME280_OVERSAMPLING_2X;

    /* Set the sensor settings */
    rslt = bme280_set_sensor_settings(BME280_SEL_ALL_SETTINGS,
                                      &_settings, &_dev);
    if (rslt != BME280_OK) {
        fprintf(stderr, "bme280_set_sensor_settings (%+d)\n", rslt);
        goto bme280_done;
    }

    /*
     * Calculate the minimum delay required between consecutive measurement
     * based upon the sensor enabled and the oversampling configuration.
     */
    rslt = bme280_cal_meas_delay(&_bme280_delay_us, &_settings);
    if (rslt != BME280_OK) {
        fprintf(stderr, "bme280_cal_meas_delay failed (%+d)\n", rslt);
    }

bme280_done:

    if ((_bme280 != -1) && (rslt != BME280_OK)) {
        i2cClose(_bme280);
        _bme280 = -1;
    }
}

void Ambience::run(void)
{
    int8_t rslt;
    char cmd[128];
    char result[128];
    FILE *fin;
    struct bme280_data data;
    struct timespec ts, tbme, tloop;

    tbme.tv_sec = 0;
    tbme.tv_nsec = _bme280_delay_us * 1000;
    tloop.tv_sec = 1;
    tloop.tv_nsec = 0;

    while (_running) {
        /* Run vcgencmd to measure the SoC temperature */
        snprintf(cmd, sizeof(cmd) - 1, "vcgencmd measure_temp");
        fin = popen(cmd, "r");
        if (fin != NULL) {
            if (fgets(result, sizeof(result), fin) != NULL) {
                size_t len;
                len = strlen(result);
                if (strncmp(result, "temp=", 5) == 0 &&
                    len > 3 &&
                    result[len - 2] == 'C' &&
                    result[len - 3] == '\'') {
                    result[len - 3] = '\0';
                    _socTemp = atof(&result[5]);
                }
            }

            pclose(fin);
            fin = NULL;
        }

        /* Probe and open device */
        if (_bme280 == -1) {
            probeOpenDevice();
            if (_bme280 == -1) {
                clock_gettime(CLOCK_REALTIME, &ts);
                timespecadd(&ts, &tloop, &ts);
                pthread_mutex_lock(&_mutex);
                pthread_cond_timedwait(&_cond, &_mutex, &ts);
                pthread_mutex_unlock(&_mutex);
                continue;
            }
        }

        /* Set the BME sensor to forced mode */
        rslt = bme280_set_sensor_mode(BME280_POWERMODE_FORCED, &_dev);
        if (rslt != BME280_OK) {
            fprintf(stderr, "bme280_set_sensor_mode (%+d)\n", rslt);
            continue;
        }

        /* Wait for BME measurement to complete */
        pthread_mutex_lock(&_mutex);
        clock_gettime(CLOCK_REALTIME, &ts);
        timespecadd(&ts, &tbme, &ts);
        pthread_cond_timedwait(&_cond, &_mutex, &ts);
        pthread_mutex_unlock(&_mutex);
        if (!_running) {
            break;
        }

        /* Get BME sensor data */
        rslt = bme280_get_sensor_data(BME280_ALL, &data, &_dev);
        if (rslt != BME280_OK) {
            fprintf(stderr, "bme280_get_sensor_data (%+d)\n", rslt);
            continue;
        }

        _histTemp.addSample(data.temperature);
        _temp = _histTemp.median();
        _histPressure.addSample(data.pressure * 0.01);
        _pressure = _histPressure.median();
        _histHumidity.addSample(data.humidity);
        _humidity = _histHumidity.median();

        mosquitto->publish("rabbit/ambience/socTemp",
                           sizeof(float), &_temp, 2, 0);
        mosquitto->publish("rabbit/ambience/temp",
                           sizeof(float), &_temp, 2, 0);
        mosquitto->publish("rabbit/ambience/pressure",
                           sizeof(float), &_pressure, 2, 0);
        mosquitto->publish("rabbit/ambience/humidity",
                           sizeof(float), &_humidity, 2, 0);

        clock_gettime(CLOCK_REALTIME, &ts);
        timespecadd(&ts, &tloop, &ts);
        pthread_mutex_lock(&_mutex);
        pthread_cond_timedwait(&_cond, &_mutex, &ts);
        pthread_mutex_unlock(&_mutex);
    }
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
