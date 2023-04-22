/*
 * pca9285_regs.h
 *
 * Copyright (C) 2023, Charles Chiou
 */

#ifndef PCA9285_REGS_H
#define PCA9285_REGS_H

#define MODE1_REG         0x00
#define   MODE1_RESTART      0x80
#define   MODE1_EXTCLK       0x40
#define   MODE1_AI           0x20
#define   MODE1_SLEEP        0x10
#define   MODE1_SUB1         0x08
#define   MODE1_SUB2         0x04
#define   MODE1_SUB3         0x02
#define   MODE1_ALLCALL      0x01
#define MODE2_REG         0x01
#define   MODE2_INVRT        0x10
#define   MODE2_OCH          0x08
#define   MODE2_OUTDRV       0x04
#define   MODE2_OUTNE_HI     0x03
#define   MODE2_OUTNE_ON     0x01
#define   MODE2_OUTNE_OFF    0x00
#define SUBADR1_REG       0x02
#define SUBADR2_REG       0x03
#define SUBADR3_REG       0x04
#define ALLCALLADR_REG    0x05
#define LED_ON_L_REG(x)   (((x) * 4) + 0x06)
#define LED_ON_H_REG(x)   (((x) * 4) + 0x07)
#define LED_OFF_L_REG(x)  (((x) * 4) + 0x08)
#define LED_OFF_H_REG(x)  (((x) * 4) + 0x09)
#define ALL_LED_ON_L_REG  0xfa
#define ALL_LED_ON_H_REG  0xfb
#define ALL_LED_OFF_L_REG 0xfc
#define ALL_LED_OFF_H_REG 0xfd
#define PRE_SCALE_REG     0xfe
#define TESTMODE_REG      0xff

#endif

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
