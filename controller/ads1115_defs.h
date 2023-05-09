/*
 * ads1115_defs.h
 *
 * Copyright (C) 2023, Charles Chiou
 */

#ifndef ADS1115_DEFS_H
#define ADS1115_DEFS_H

#define CONVERSION_REG 0x00
#define CONFIG_REG     0x01
#define   COMP_QUE_ONE    (0 << 0)
#define   COMP_QUE_TWO    (1 << 0)
#define   COMP_QUE_FOUR   (2 << 0)
#define   COMP_QUE_DIS    (3 << 0)
#define   COMP_LAT_NO     (0 << 2)
#define   COMP_LAT_YES    (1 << 2)
#define   COMP_POL_LO     (0 << 3)
#define   COMP_POL_HI     (1 << 3)
#define   COMP_MODE_TRAD  (0 << 4)
#define   COMP_MODE_WIN   (1 << 4)
#define   DR_8_SPS        (0 << 5)
#define   DR_16_SPS       (1 << 5)
#define   DR_32_SPS       (2 << 5)
#define   DR_64_SPS       (3 << 5)
#define   DR_128_SPS      (4 << 5)
#define   DR_250_SPS      (5 << 5)
#define   DR_475_SPS      (6 << 5)
#define   DR_860_SPS      (7 << 5)
#define   MODE_CONT       (0 << 8)
#define   MODE_SING       (1 << 8)
#define   PGA_FSR_6_144V  (0 << 9)
#define   PGA_FSR_4_096V  (1 << 9)
#define   PGA_FSR_2_048V  (2 << 9)
#define   PGA_FSR_1_024V  (3 << 9)
#define   PGA_FSR_0_512V  (4 << 9)
#define   PGA_FSR_0_256V  (5 << 9)
#define   MUX_AIN0_AIN1   (0 << 12)
#define   MUX_AIN0_AIN3   (1 << 12)
#define   MUX_AIN1_AIN3   (2 << 12)
#define   MUX_AIN2_AIN3   (3 << 12)
#define   MUX_AIN0_GND    (4 << 12)
#define   MUX_AIN1_GND    (5 << 12)
#define   MUX_AIN2_GND    (6 << 12)
#define   MUX_AIN3_GND    (7 << 12)
#define   OS              (1 << 15)
#define LO_THRESH_REG  0x02
#define HI_THRESH_REG  0x03

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
