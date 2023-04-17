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
#include "bme280.h"
#include "rabbit.hxx"

/*
 * BME280
 * https://www.mouser.com/datasheet/2/682/Sensirion_Humidity_Sensors_SHT3x_Datasheet_digital-971521.pdf
 */
#define BME280_I2C_BUS  1
#define BME280_I2C_ADDR 0x76

/*
 * GY-SHT30-D
 * https://www.mouser.com/datasheet/2/682/Sensirion_Humidity_Sensors_SHT3x_Datasheet_digital-971521.pdf
 */
#define SHT3X_I2C_BUS 1
#define SHT3X_I2C_ADDR  0x44

struct identifier
{
    uint8_t dev_addr;
    int8_t fd;
};

struct my_bme280_dev
{
    struct bme280_dev dev;
    struct identifier id;
};

static int8_t user_i2c_read(uint8_t reg_addr, uint8_t *data, uint32_t len,
                            void *intf_ptr)
{
    struct identifier id;

    id = *((struct identifier *)intf_ptr);

    write(id.fd, &reg_addr, 1);
    read(id.fd, data, len);

    return 0;
}

static void user_delay_us(uint32_t period, void *intf_ptr)
{
    (void)(intf_ptr);
    usleep(period);
}

static int8_t user_i2c_write(uint8_t reg_addr, const uint8_t *data,
                             uint32_t len, void *intf_ptr)
{
    uint8_t *buf;
    struct identifier id;

    id = *((struct identifier *)intf_ptr);

    buf = (uint8_t *) malloc(len + 1);
    buf[0] = reg_addr;
    memcpy(buf + 1, data, len);
    if (write(id.fd, buf, len + 1) < (uint16_t)len)
    {
        return BME280_E_COMM_FAIL;
    }

    free(buf);

    return BME280_OK;
}

Ambience::Ambience()
    : _bme280(NULL),
      _sht3x(-1)
{
    struct my_bme280_dev *my_bme280_dev;

    _bme280 = malloc(sizeof(struct my_bme280_dev));
    my_bme280_dev = (struct my_bme280_dev *) _bme280;

    my_bme280_dev->dev.intf = BME280_I2C_INTF;
    my_bme280_dev->dev.read = user_i2c_read;
    my_bme280_dev->dev.write = user_i2c_write;
    my_bme280_dev->dev.delay_us = user_delay_us;
    my_bme280_dev->dev.intf_ptr = &my_bme280_dev->id;
    my_bme280_dev->dev.settings.osr_h = BME280_OVERSAMPLING_1X;
    my_bme280_dev->dev.settings.osr_p = BME280_OVERSAMPLING_16X;
    my_bme280_dev->dev.settings.osr_t = BME280_OVERSAMPLING_2X;
    my_bme280_dev->dev.settings.filter = BME280_FILTER_COEFF_16;
    my_bme280_dev->id.dev_addr = BME280_I2C_ADDR_PRIM;

    if ((my_bme280_dev->id.fd = open("/dev/i2c-1", O_RDWR)) < 0) {
        fprintf(stderr, "failed to open /dev/i2c-1!\n");
        goto bme280_done;
    }

    if (ioctl(my_bme280_dev->id.fd, I2C_SLAVE,
              my_bme280_dev->id.dev_addr) != 0) {
        perror("ioctl");
        goto bme280_done;
    }

    if (bme280_init(&my_bme280_dev->dev) != BME280_OK) {
        cerr << "bme280_init failed!" << endl;
        goto bme280_done;
    }

    if (bme280_set_sensor_settings(BME280_OSR_PRESS_SEL |
                                   BME280_OSR_TEMP_SEL |
                                   BME280_OSR_HUM_SEL |
                                   BME280_FILTER_SEL,
                                   &my_bme280_dev->dev) !=
        BME280_OK) {
        cerr << "bme280_set_sensor_settings failed!" << endl;
        goto bme280_done;
    }

bme280_done:

    _sht3x = i2cOpen(SHT3X_I2C_BUS, SHT3X_I2C_ADDR, 0);
    if (_sht3x < 0) {
        cerr << "Open SHT3X failed" << endl;
    }
}

Ambience::~Ambience()
{
    if (_bme280) {
        struct my_bme280_dev *my_bme280_dev =
            (struct my_bme280_dev *) _bme280;
        close(my_bme280_dev->id.fd);
        free(_bme280);
        _bme280 = NULL;
    }

    if (_sht3x >= 0) {
        i2cClose(_sht3x);
    }
}

float Ambience::socTemp(void) const
{
    float temp = 0.0;
    char cmd[128];
    char result[128];
    FILE *fin;

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
                temp = atof(&result[5]);
            }
        }

        pclose(fin);
    }

    return temp;
}

unsigned int Ambience::temp(void) const
{
    // TODO
    return 0;
}

unsigned int Ambience::pressure(void) const
{
    // TODO
    return 0;
}

unsigned int Ambience::humidity(void) const
{
    // TODO
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
