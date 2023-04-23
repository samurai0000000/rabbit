/*
 * qmc5883l_regs.h
 *
 * Copyright (C) 2023, Charles Chiou
 */

#ifndef QMC5883L_REGS_H
#define QMC5883L_REGS_H

#define DATA_X_LSB_REG         0x00
#define DATA_X_MSB_REG         0x01
#define DATA_Y_LSB_REG         0x02
#define DATA_Y_MSB_REG         0x03
#define DATA_Z_LSB_REG         0x04
#define DATA_Z_MSB_REG         0x05
#define STATUS_REG             0x06
#define   STATUS_DOR             0x04
#define   STATUS_OVL             0x02
#define   STATUS_DRDY            0x01
#define TOUT_LSB_REG           0x07
#define TOUT_MSB_REG           0x08
#define CTRL_1_REG             0x09
#define   MODE_STANDBY           (0 << 0)
#define   MODE_CONTINUOUS        (1 << 0)
#define   ODR_10HZ               (0 << 2)
#define   ODR_50HZ               (1 << 2)
#define   ODR_100HZ              (2 << 2)
#define   ODR_200HZ              (3 << 2)
#define   RNG_2G                 (0 << 4)
#define   RNG_8G                 (1 << 4)
#define   OSR_512                (0 << 6)
#define   OSR_256                (1 << 6)
#define   OSR_128                (2 << 6)
#define   OSR_64                 (3 << 6)
#define CTRL_2_REG             0x0a
#define   SOFT_RST               (1 << 7)
#define   ROL_PNT                (1 << 6)
#define   INT_ENB                (1 << 0)
#define SR_PERIOD_REG          0x0b
#define CHIPID_REG             0x0d
#define   CHIPID_VAL             0xff

#endif

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
