/*
 * main.c
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <stdio.h>
#include <pico/stdlib.h>
#include <pico/unique_id.h>
#include "rabbit_mcu.h"

#define IR_SAMPLING_INTERVAL_US    500
#define US_SAMPLING_INTERVAL_US   5000
#define PUSH_DATA_INTERVAL_US     5000
#define MAIN_LOOP_DELAY_MS         500

void rabbit_panic(const char *msg)
{
    printf("panic: %s\n", msg);
    printf("halt!\n");
    for (;;);
}

static bool sample_ir(repeating_timer_t *timer)
{
    (void)(timer);

    ir_refresh();

    return true;
}

static bool sample_us(repeating_timer_t *timer)
{
    static unsigned int usid = 0;

    (void)(timer);

    ultrasound_trigger(usid);
    usid++;
    usid %= ULTRASOUND_DEVICES;

    onboard_temp_refresh();

    return true;
}

static const char *encode_result(void)
{
    static char line[64];
    char *s = line;
    int l = sizeof(line);
    unsigned int i;
    int ret;

    for (i = 0; i < IR_DEVICES; i++) {
        *s = ir_state[i] ? '1' : '0';
        s++;
        l--;
        if (i != (IR_DEVICES - 1)) {
            *s = ',';
            s++;
            l--;
        }
    }

    *s = ';';
    s++;
    l--;

    for (i = 0; i < ULTRASOUND_DEVICES; i++) {
        ret = snprintf(s, l - 1, "%u", ultrasound_d_mm[i]);
        s += ret;
        l -= ret;
        if (i != (ULTRASOUND_DEVICES - 1)) {
            *s = ',';
            s++;
            l--;
        }
    }

    *s = '\n';
    s++;
    l--;

    *s = '\0';
    s++;
    l--;

    return line;
}

static bool push_data(repeating_timer_t *timer)
{
    (void)(timer);

    /* Send official result via USB CDC */
    usb_printf(encode_result());

    return true;
}

const char *get_banner(void)
{
    static char banner[32];
    pico_unique_board_id_t board_id;
    size_t l = sizeof(banner);
    char *s = banner;
    unsigned int i;
    int ret;

    pico_get_unique_board_id(&board_id);

    ret = snprintf(s, l - 1, "Rabbit MCU on ");
    s += ret;
    l -= ret;
    for (i = 0; i < PICO_UNIQUE_BOARD_ID_SIZE_BYTES; i++) {
        uint8_t dh, dl;

        dh = (board_id.id[i] >> 4);
        dl = (board_id.id[i] & 0xf);
        if (dh < 10) {
            *s = '0' + dh;
        } else {
            *s = 'a' + (dh - 10);
        }
        s++;
        l--;
        if (dl < 10) {
            *s = '0' + dl;
        } else {
            *s = 'a' + (dl - 10);
        }
        s++;
        l--;
    }

    *s = '\n';
    s++;
    l--;

    *s = '\0';
    s++;
    l--;

    return banner;
}

int main(void)
{
    repeating_timer_t timer1, timer2, timer3;
    unsigned int i;
    unsigned int loop = 0;
    bool led_on = false;
    unsigned int last_rs232_rx_lines = (unsigned int) -1;
    bool debug_on = false;

    stdio_init_all();
    rs232_init();
    ir_init();
    ultrasound_init();
    led_init();
    onboard_temp_init();

    rs232_printf(get_banner());

    if (add_repeating_timer_us(-IR_SAMPLING_INTERVAL_US,
                               sample_ir, NULL, &timer1) ==
        false) {
        rabbit_panic("add_repeating_timer for infrared");
    }

    if (add_repeating_timer_us(-US_SAMPLING_INTERVAL_US,
                               sample_us, NULL, &timer2) ==
        false) {
        rabbit_panic("add_repeating_timer for ultrasound");
    }

    if (add_repeating_timer_us(-PUSH_DATA_INTERVAL_US,
                               push_data, NULL, &timer3) ==
        false) {
        rabbit_panic("add_repeating_timer for push");
    }


    for (loop = 0; true; loop++) {
        if (last_rs232_rx_lines != rs232_rx_lines) {
            printf("here %u, %u\n", last_rs232_rx_lines, rs232_rx_lines);
            debug_on = rs232_rx_lines % 2 == 1 ? 1 : 0;
            last_rs232_rx_lines = rs232_rx_lines;
            if (debug_on == false) {
                rs232_printf("Press enter to start/end debug\n");
            }
        }

        /* Debug over RS-232 */
        if (debug_on) {
            rs232_printf("\e[1;1H\e[2J");
            rs232_printf("loop %u\n", loop);
            for (i = 0; i < IR_DEVICES; i++) {
                rs232_printf("IR%u: %s\n", i, ir_state[i] ? "on" : "off");
            }
            for (i = 0; i < ULTRASOUND_DEVICES; i++) {
                rs232_printf("US%u: %u mm\n", i, ultrasound_d_mm[i]);
            }
        }

        /* Toggle LED */
        led_set(led_on);
        led_on = !led_on;

        /* Sleep */
        sleep_ms(MAIN_LOOP_DELAY_MS);
    }

    return 0;
}

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
