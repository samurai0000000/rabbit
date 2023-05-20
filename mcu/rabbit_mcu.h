/*
 * rabbit_mcu.h
 */

#ifndef RABBIT_MCU_H
#define RABBIT_MCU_H

extern void rabbit_panic(const char *msg);

extern void rs232_init(void);
extern int rs232_printf(const char *fmt, ...);
#if !defined(_STDIO_H) && !defined(printf)
#define printf rs232_printf
#endif

#define IR_DEVICES  4
extern bool ir_state[IR_DEVICES];
extern void ir_init(void);
extern void ir_refresh(void);
extern bool ir_triggered(void);

#define ULTRASOUND_DEVICES 6
extern unsigned int ultrasound_d_mm[ULTRASOUND_DEVICES];
extern void ultrasound_init(void);
extern void ultrasound_trigger(unsigned int id);

extern void led_init(void);
extern void led_set(bool on);

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
