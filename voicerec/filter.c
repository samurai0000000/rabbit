/*
 * filter.c
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <limits.h>
#include "voicerec.h"

#define VOLUME_THRESHOLD  1000
#define SILENT_THRESHOLD   200
#define ACTIVE_THRESHOLD     1
#define PUBLISH_THRESHOLD  500   // ms

static unsigned int silent_count = 0;
static unsigned int active_count = 0;

static void get_frame_vol_minmax(const void *pcm, size_t size,
                                 int16_t *min, int16_t *max)
{
    unsigned int count;
    const int16_t *sample;
    int16_t lmin = SHRT_MAX, lmax = SHRT_MIN;

    assert(pcm != NULL);
    assert((size > 0) && (size % 2) == 0);
    assert(min != NULL);
    assert(max != NULL);

    for (count = size / sizeof(int16_t), sample = (const int16_t *) pcm;
         count > 0;
         count--, sample++) {
        if (*sample < lmin) {
            lmin = *sample;
        }
        if (*sample > lmax) {
            lmax = *sample;
        }
    }

    *min = lmin;
    *max = lmax;
}

void filter_pcm_in(const void *pcm, size_t size)
{
    static unsigned int capture = 0;
    int16_t min, max;
    unsigned int silent = 0;

    get_frame_vol_minmax(pcm, size, &min, &max);
    if ((max - min) < VOLUME_THRESHOLD) {
        silent = 1;
    }

    if (silent) {
        silent_count++;
        active_count = 0;
    } else {
        active_count++;
        silent_count = 0;
    }

    if ((capture == 1) &&
        (silent_count >= SILENT_THRESHOLD)) {
        wav_stop();
        if (wav_time_elapsed_ms() > PUBLISH_THRESHOLD) {
            voicerec_wav_publish(wav_filename());
        }
        capture = 0;
    } else if (capture == 0) {
        if (active_count >= ACTIVE_THRESHOLD) {
            wav_pcm_in(pcm, size);
            capture = 1;
        }
    } else if (capture == 1) {
        wav_pcm_in(pcm, size);
    }
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
