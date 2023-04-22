/*
 * ambience.cxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <stdlib.h>
#include <pigpio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include "rabbit.hxx"

/*
 * BME280
 * https://www.mouser.com/datasheet/2/783/BST-BME280-DS002-1509607.pdf
 */
#define BME280_I2C_BUS  1
#define BME280_I2C_ADDR 0x76

/*
 * GY-SHT30-D
 * https://www.mouser.com/datasheet/2/682/Sensirion_Humidity_Sensors_SHT3x_Datasheet_digital-971521.pdf
 */
#define SHT3X_I2C_BUS   1
#define SHT3X_I2C_ADDR  0x44

static int8_t user_i2c_read(uint8_t reg_addr, uint8_t *data,
                            uint32_t len, void *intf_ptr)
{
    int ret = 0;

    while (len > 0) {
        //printf("i2c read 0x%.2x\n", reg_addr);
        ret = i2cReadByteData((int) (intptr_t) intf_ptr, reg_addr);
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

    while (len > 0) {
        //printf("i2c write 0x%.2x 0x%.2x\n", reg_addr, *data);
        ret = i2cWriteByteData((int) (intptr_t) intf_ptr, reg_addr, *data);
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
      _sht3x(-1)
{
    int8_t rslt = BME280_OK;

    _bme280 = i2cOpen(BME280_I2C_BUS, BME280_I2C_ADDR, 0);
    if (_bme280 < 0) {
        cerr << "Open BME280 failed" << endl;
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
        fprintf(stderr, "bme280_get_sensor_settings failed (%+d).", rslt);
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
        fprintf(stderr, "bme280_set_sensor_settings (%+d).", rslt);
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

    _sht3x = i2cOpen(SHT3X_I2C_BUS, SHT3X_I2C_ADDR, 0);
    if (_sht3x < 0) {
        cerr << "Open SHT3X failed" << endl;
        goto sht3x_done;
    }

sht3x_done:

    _running = 1;
    pthread_create(&_thread, NULL, Ambience::thread_func, this);
}

Ambience::~Ambience()
{
    if (_bme280) {
        i2cClose(_bme280);
    }

    if (_sht3x >= 0) {
        i2cClose(_sht3x);
    }

    _running = 0;
    pthread_join(_thread, NULL);
}

void *Ambience::thread_func(void *args)
{
    Ambience *ambience = (Ambience *) args;

    ambience->run();

    return NULL;
}

void Ambience::run(void)
{
    int8_t rslt;

    while (_running) {
        char cmd[128];
        char result[128];
        FILE *fin;
        struct bme280_data data;

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

        /* Set the sensor to forced mode */
        rslt = bme280_set_sensor_mode(BME280_POWERMODE_FORCED, &_dev);
        if (rslt != BME280_OK) {
            fprintf(stderr, "bme280_set_sensor_mode (%+d).", rslt);
            continue;
        }

        /* Wait for the measurement to complete */
        usleep(_bme280_delay_us);

        /* Get sensor data */
        rslt = bme280_get_sensor_data(BME280_ALL, &data, &_dev);
        if (rslt != BME280_OK) {
            fprintf(stderr, "bme280_get_sensor_data (%+d).", rslt);
            continue;
        }

        _temp = data.temperature;
        _pressure = data.pressure * 0.01;
        _humidity = data.humidity;

        usleep(500000);
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
