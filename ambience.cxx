/*
 * ambience.cxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <stdlib.h>
#include <pigpio.h>
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

Ambience::Ambience()
    : _bme280(-1),
      _sht3x(-1)
{
    _bme280 = i2cOpen(BME280_I2C_BUS, BME280_I2C_ADDR, 0);
    if (_bme280 < 0) {
        cerr << "Open BME280 failed" << endl;
    }

    _sht3x = i2cOpen(SHT3X_I2C_BUS, SHT3X_I2C_ADDR, 0);
    if (_sht3x < 0) {
        cerr << "Open SHT3X failed" << endl;
    }
}

Ambience::~Ambience()
{
    if (_bme280 >= 0) {
        i2cClose(_bme280);
    }

    if (_sht3x >= 0) {
        i2cClose(_sht3x);
    }
}

float Ambience::cpuTemp(void) const
{
    float temp = 0.0;
    FILE *fin;

    fin = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (fin) {
        char buf[128];
        size_t len;
        unsigned long val;
        char *endptr = NULL;

        len = fread(buf, 1, sizeof(buf), fin);
        if (len > 0) {
            val = strtoul(buf, &endptr, 10);
            if (errno == 0) {
                temp = (float) val;
                temp /= 1000;
            }
        }

        fclose(fin);
    }

    return temp;
}

float Ambience::gpuTemp(void) const
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
