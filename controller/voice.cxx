/*
 * voice.cxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <alsa/asoundlib.h>
#include "rabbit.hxx"

#define AUDIO_PCM_INPUT_DEVICE   "plughw:1"
#define AUDIO_PCM_INPUT_FORMAT   SND_PCM_FORMAT_S16_LE
#define AUDIO_PCM_INPUT_RATE     22050
#define AUDIO_PCM_INPUT_CHANS    1

Voice::Voice()
    : _handle(NULL),
      _rate(AUDIO_PCM_INPUT_RATE),
      _enable(true),
      _volHist(),
      _volHistCur(0)
{
    _running = true;
    pthread_mutex_init(&_mutex, NULL);
    pthread_cond_init(&_cond, NULL);
    pthread_create(&_thread, NULL, Voice::thread_func, this);
}

Voice::~Voice()
{
    _running = false;
    pthread_cond_broadcast(&_cond);
    pthread_join(_thread, NULL);
    pthread_mutex_destroy(&_mutex);
    pthread_cond_destroy(&_cond);

    if (_handle != NULL) {
        snd_pcm_close((snd_pcm_t *) _handle);
        _handle = NULL;
    }
}

void Voice::probeOpenDevice(void)
{
    int ret;
    snd_pcm_hw_params_t *hw_params = NULL;

    if (_handle != NULL) {
        return;
    }

    ret = snd_pcm_open((snd_pcm_t **) &_handle,
                       AUDIO_PCM_INPUT_DEVICE,
                       SND_PCM_STREAM_CAPTURE, 0);
    if (ret != 0) {
        fprintf(stderr, "Voice::probeOpenDevice %s\n", snd_strerror(ret));
        goto done;
    }

    ret = snd_pcm_hw_params_malloc(&hw_params);
    if (ret != 0) {
        fprintf(stderr, "Voice::probeOpenDevice %s\n", snd_strerror(ret));
        goto done;
    }

    ret = snd_pcm_hw_params_any((snd_pcm_t *) _handle, hw_params);
    if (ret != 0) {
        fprintf(stderr, "Voice::probeOpenDevice %s\n", snd_strerror(ret));
        goto done;
    }

    ret = snd_pcm_hw_params_set_access((snd_pcm_t *) _handle,
                                       hw_params,
                                       SND_PCM_ACCESS_RW_INTERLEAVED);
    if (ret != 0) {
        fprintf(stderr, "Voice::probeOpenDevice %s\n", snd_strerror(ret));
        goto done;
    }

    ret = snd_pcm_hw_params_set_format ((snd_pcm_t *) _handle,
                                        hw_params,
                                        AUDIO_PCM_INPUT_FORMAT);
    if (ret != 0) {
        fprintf(stderr, "Voice::probeOpenDevice %s\n", snd_strerror(ret));
        goto done;
    }

    ret = snd_pcm_hw_params_set_rate_near((snd_pcm_t *) _handle,
                                          hw_params,
                                          &_rate,
                                          0);
    if (ret != 0) {
        fprintf(stderr, "Voice::probeOpenDevice %s\n", snd_strerror(ret));
        goto done;
    }

    ret = snd_pcm_hw_params_set_channels((snd_pcm_t *) _handle,
                                         hw_params,
                                         AUDIO_PCM_INPUT_CHANS);
    if (ret != 0) {
        fprintf(stderr, "Voice::probeOpenDevice %s\n", snd_strerror(ret));
        goto done;
    }

    ret = snd_pcm_hw_params((snd_pcm_t *) _handle, hw_params);
    if (ret != 0) {
        fprintf(stderr, "Voice::probeOpenDevice %s\n", snd_strerror(ret));
        goto done;
    }

    ret = snd_pcm_prepare((snd_pcm_t *) _handle);
    if (ret != 0) {
        fprintf(stderr, "Voice::probeOpenDevice %s\n", snd_strerror(ret));
        goto done;
    }

done:

    if (hw_params != NULL) {
        snd_pcm_hw_params_free(hw_params);
    }

    if (ret != 0 && _handle != NULL) {
        snd_pcm_close((snd_pcm_t *) _handle);
        _handle = NULL;
    }
}

void *Voice::thread_func(void *args)
{
    Voice *voice = (Voice *) args;

    voice->run();

    return NULL;
}

void Voice::run(void)
{
    int ret;
    struct timespec ts;
    unsigned int frames;
    uint8_t *buf = NULL;
    size_t bufsize;
    unsigned int i;
    const int16_t *pcm_sample;
    int16_t max, min;

    frames = AUDIO_PCM_INPUT_RATE / VOL_HIST_SIZE;
    bufsize = frames * (snd_pcm_format_width(AUDIO_PCM_INPUT_FORMAT) / 8) *
        AUDIO_PCM_INPUT_CHANS;
    buf = (uint8_t *) malloc(bufsize);

    while (_running) {
        /* Probe and open audio input device */
        probeOpenDevice();
        if ((_handle == NULL) || (_enable == false)) {
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += 1;
            pthread_mutex_lock(&_mutex);
            pthread_cond_timedwait(&_cond, &_mutex, &ts);
            pthread_mutex_unlock(&_mutex);
            continue;
        }

        /* Capture PCM data */
        ret = snd_pcm_readi((snd_pcm_t *) _handle, buf, frames);
        if (ret != (int) frames) {
            fprintf(stderr, "Voice::run %s\n", snd_strerror(ret));
            snd_pcm_close((snd_pcm_t *) _handle);
            _handle = NULL;
            memset(_volHist, 0x0, sizeof(_volHist));
            _volHistCur = 0;
            continue;
        }

        /* Compute the maximum */
        for (i = 0, pcm_sample = (const int16_t *) buf,
                 max = SHRT_MIN, min = SHRT_MAX;
             i < frames;
             i++, pcm_sample++) {
            if (*pcm_sample > max) {
                max = *pcm_sample;
            }
            if (*pcm_sample < min) {
                min = *pcm_sample;
            }
        }

        /* Update histogram */
        _volHist[_volHistCur].min = min;
        _volHist[_volHistCur].max = max;
        _volHistCur++;
        _volHistCur %= volHistSize();
    }

    if (buf) {
        free(buf);
        buf = NULL;
    }
}

void Voice::enable(bool en)
{
    en = en ? true : false;
    _enable = en;
    if (en) {
        pthread_cond_broadcast(&_cond);
    }
}

unsigned int Voice::getVolHist(struct vol_hist_point *hist,
                               unsigned int points) const
{
    unsigned int i;
    unsigned int cur;

    if (hist == NULL) {
        return 0;
    }

    for (i = 0, cur = _volHistCur;
         (i < volHistSize()) && (i < points);
         i++, cur = ((cur + 1) % volHistSize())) {
        hist[i].min = _volHist[cur].min;
        hist[i].max = _volHist[cur].max;
    }

    return i;
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
