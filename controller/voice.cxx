/*
 * voice.cxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <bsd/sys/time.h>
#include <alsa/asoundlib.h>
#include <libusb-1.0/libusb.h>
#include "rabbit.hxx"

/*
 * https://wiki.seeedstudio.com/ReSpeaker_Mic_Array_v2.0/
 */

#define AUDIO_PCM_INPUT_DEVICE   "plughw:1,0"
#define AUDIO_PCM_INPUT_FORMAT   SND_PCM_FORMAT_S16_LE
#define AUDIO_PCM_INPUT_RATE     22050
#define AUDIO_PCM_INPUT_CHANS    1

#define USB_ENDPOINT_IN	    (LIBUSB_ENDPOINT_IN  | 0x01)
#define USB_ENDPOINT_OUT    (LIBUSB_ENDPOINT_OUT | 0x01)
#define USB_TIMEOUT         1000

struct respeaker_ctrl {
    uint32_t offset;
    uint32_t value;
    uint32_t type;
};

static unsigned int instance = 0;

Voice::Voice()
    : _handle(NULL),
      _rate(AUDIO_PCM_INPUT_RATE),
      _enable(true),
      _usbctx(NULL),
      _usbdev(NULL),
      _volHist(),
      _volHistCur(0)
{
    if (instance != 0) {
        fprintf(stderr, "Voice can be instantiated only once!\n");
        exit(EXIT_FAILURE);
    } else {
        instance++;
    }

    _running = true;
    pthread_mutex_init(&_mutex, NULL);
    pthread_cond_init(&_cond, NULL);
    pthread_create(&_thread, NULL, Voice::thread_func, this);
    pthread_create(&_thread2, NULL, Voice::thread_func2, this);

    printf("Voice is online\n");
}

Voice::~Voice()
{
    _running = false;
    pthread_cond_broadcast(&_cond);
    pthread_join(_thread, NULL);
    pthread_join(_thread2, NULL);
    pthread_mutex_destroy(&_mutex);
    pthread_cond_destroy(&_cond);

    if (_handle != NULL) {
        snd_pcm_close((snd_pcm_t *) _handle);
        _handle = NULL;
    }

    if (_usbdev) {
        libusb_close(_usbdev);
        _usbdev = NULL;
    }

    if (_usbctx) {
        libusb_exit(_usbctx);
        _usbctx = NULL;
    }

    instance--;
    printf("Voice is offline\n");
}

void Voice::probeOpenAudioDevice(void)
{
    int ret = 0;
    snd_pcm_hw_params_t *hw_params = NULL;

    if (_handle != NULL) {
        goto done;
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
    bool on = !_enable;

    frames = AUDIO_PCM_INPUT_RATE / VOL_HIST_SIZE;
    bufsize = frames * (snd_pcm_format_width(AUDIO_PCM_INPUT_FORMAT) / 8) *
        AUDIO_PCM_INPUT_CHANS;
    buf = (uint8_t *) malloc(bufsize);

    while (_running) {
        /* Probe and open audio input device */
        probeOpenAudioDevice();
        if (_handle == NULL) {
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
            on = false;
            mosquitto->publish("rabbit/voice/state",
                               3, "off", 1, 0);
            continue;
        }

        if (on != _enable) {
            on = _enable;
            if (on) {
                mosquitto->publish("rabbit/voice/state",
                                   2, "on", 1, 0);
            } else {
                mosquitto->publish("rabbit/voice/state",
                                   3, "off", 1, 0);
            }
        }

        /* Discard frames if disabled. Note that we still fetch
         * data from the device to prevent a broken pipe... */
        if (_enable == false) {
            continue;
        }

        /* Publish sampled data to MQTT */
        mosquitto->publish("rabbit/voice/pcm",
                           bufsize, buf, 2, 0);

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

void Voice::probeOpenUSBDevice(void)
{
    int ret;

    if (_usbctx == NULL) {
        ret = libusb_init(&_usbctx);
        if (ret != LIBUSB_SUCCESS) {
            fprintf(stderr, "libusb_init failed: %s\n",
                    libusb_strerror(ret));
            goto done;
        }
    }

    if (_usbdev == NULL) {
        _usbdev = libusb_open_device_with_vid_pid(_usbctx, 0x2886, 0x0018);
        if (_usbdev == NULL) {
            ret = -1;
            goto done;
        }
    }

done:

    if (ret != 0 && _usbdev != NULL) {
        libusb_close(_usbdev);
        _usbdev = NULL;
    }
}

uint32_t Voice::readUsbIntReg(unsigned int id, unsigned int offset)
{
    int ret;
    uint16_t cmd;
    uint32_t response = 0;

    if (_usbdev == NULL) {
        return response;
    }

    cmd = 0x80 | 0x40 | (uint8_t) offset;
    ret = libusb_control_transfer(_usbdev,
                                  LIBUSB_REQUEST_TYPE_VENDOR |
                                  LIBUSB_ENDPOINT_IN,
                                  0,
                                  cmd,
                                  (uint16_t) id,
                                  (unsigned char *) &response,
                                  sizeof(response),
                                  USB_TIMEOUT);
    if (ret < 0) {
        fprintf(stderr, "libusb_control_transfer: %s\n",
                libusb_strerror(ret));
        libusb_close(_usbdev);
        _usbdev = NULL;
    }

    return response;
}

void Voice::writeUsbIntReg(unsigned int id, unsigned int offset,
                           uint32_t value)
{
    int ret;
    uint32_t cmd[3];

    if (_usbdev == NULL) {
        return;
    }

    cmd[0] = offset;
    cmd[1] = value;
    cmd[2] = 1;
    ret = libusb_control_transfer(_usbdev,
                                  LIBUSB_REQUEST_TYPE_VENDOR |
                                  LIBUSB_ENDPOINT_OUT,
                                  0,
                                  0,
                                  (uint16_t) id,
                                  (unsigned char *) cmd,
                                  sizeof(cmd),
                                  USB_TIMEOUT);
    if (ret < 0) {
        fprintf(stderr, "libusb_control_transfer: %s\n",
                libusb_strerror(ret));
        libusb_close(_usbdev);
        _usbdev = NULL;
    }
}

void Voice::writeUsbFloatReg(unsigned int id, unsigned int offset,
                             float value)
{
    int ret;
    uint32_t cmd[3];

    if (_usbdev == NULL) {
        return;
    }

    cmd[0] = offset;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
    cmd[1] = *((const uint32_t *) ((const void *) &value));
#pragma GCC diagnostic pop
    cmd[2] = 0;
    ret = libusb_control_transfer(_usbdev,
                                  LIBUSB_REQUEST_TYPE_VENDOR |
                                  LIBUSB_ENDPOINT_OUT,
                                  0,
                                  0,
                                  (uint16_t) id,
                                  (unsigned char *) cmd,
                                  sizeof(cmd),
                                  USB_TIMEOUT);
    if (ret < 0) {
        fprintf(stderr, "libusb_control_transfer: %s\n",
                libusb_strerror(ret));
        libusb_close(_usbdev);
        _usbdev = NULL;
    }
}

float Voice::readUsbFloatReg(unsigned int id, unsigned int offset)
{
    int ret;
    uint16_t cmd;
    int32_t response[2] = { 0, 0, };

    if (_usbdev == NULL) {
        return 0.0;
    }

    cmd = 0x80 | (uint8_t) offset;
    ret = libusb_control_transfer(_usbdev,
                                  LIBUSB_REQUEST_TYPE_VENDOR |
                                  LIBUSB_ENDPOINT_IN,
                                  0,
                                  cmd,
                                  (uint16_t) id,
                                  (unsigned char *) response,
                                  sizeof(response),
                                  USB_TIMEOUT);
    if (ret < 0) {
        fprintf(stderr, "libusb_control_transfer: %s\n",
                libusb_strerror(ret));
        libusb_close(_usbdev);
        _usbdev = NULL;
    }

    return ((float) response[0] * powf(2.0, (float) response[1]));
}

void *Voice::thread_func2(void *args)
{
    Voice *voice = (Voice *) args;

    voice->run2();

    return NULL;
}

void Voice::run2(void)
{
    struct timespec ts, tloop;

    tloop.tv_sec = 0;
    tloop.tv_nsec = 500000000;

    while (_running) {
        /* Probe and open USB device */
        probeOpenUSBDevice();
        if (_usbdev == NULL) {
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += 1;
            pthread_mutex_lock(&_mutex);
            pthread_cond_timedwait(&_cond, &_mutex, &ts);
            pthread_mutex_unlock(&_mutex);
            continue;
        }

        _prop.AECFreezeOnOff = readUsbIntReg(18, 7);
        _prop.AECNorm = readUsbFloatReg(18, 19);
        _prop.AECPathChange = readUsbIntReg(18, 25);
        _prop.RT60 = readUsbFloatReg(18, 26);
        _prop.HPFOnOff = readUsbIntReg(18, 27);
        _prop.AECSilenceLevel = readUsbFloatReg(18, 30);
        _prop.AECSilenceMode = readUsbIntReg(18, 31);
        _prop.AGCOnOff = readUsbIntReg(19, 0);
        _prop.AGCMaxGain = readUsbFloatReg(19, 1);
        _prop.AGCDesiredLevel = readUsbFloatReg(19, 2);
        _prop.AGCGain = readUsbFloatReg(19, 3);
        _prop.AGCTime = readUsbFloatReg(19, 4);
        _prop.CNIOnOff = readUsbIntReg(19, 5);
        _prop.FreezeOnOff = readUsbIntReg(19, 6);
        _prop.StatNoiseOnOff = readUsbIntReg(19, 8);
        _prop.GammaNS = readUsbFloatReg(19, 9);
        _prop.MinNS = readUsbFloatReg(19, 10);
        _prop.NonStatNoiseOnOff = readUsbIntReg(19, 11);
        _prop.GamaNN = readUsbFloatReg(19, 12);
        _prop.MinNN = readUsbFloatReg(19, 13);
        _prop.EchoOnOff = readUsbIntReg(19, 14);
        _prop.GammaE = readUsbFloatReg(19, 15);
        _prop.GammaETail = readUsbFloatReg(19, 16);
        _prop.GammaENL = readUsbFloatReg(19, 17);
        _prop.NLAttenOnOff = readUsbIntReg(19, 18);
        _prop.NLAECMode = readUsbIntReg(19, 20);
        _prop.SpeechDetected = readUsbIntReg(19, 22);
        _prop.FSBUpdated = readUsbIntReg(19, 23);
        _prop.FSBPathChange = readUsbIntReg(19, 24);
        _prop.TransientOnOff = readUsbIntReg(19, 29);
        _prop.VoiceActivity = readUsbIntReg(19, 32);
        _prop.StatNoiseOnOffSR = readUsbIntReg(19, 33);
        _prop.NonStatNoiseOnOffSR = readUsbIntReg(19, 34);
        _prop.GammaNSSR = readUsbFloatReg(19, 35);
        _prop.GammaNNSR = readUsbFloatReg(19, 36);
        _prop.MinNSSR = readUsbFloatReg(19, 37);
        _prop.MinNNSR = readUsbFloatReg(19, 38);
        _prop.GammaVADSR = readUsbFloatReg(19, 39);
        _prop.DOAAngle = readUsbIntReg(21, 0);

        clock_gettime(CLOCK_REALTIME, &ts);
        timespecadd(&ts, &tloop, &ts);
        pthread_mutex_lock(&_mutex);
        pthread_cond_timedwait(&_cond, &_mutex, &ts);
        pthread_mutex_unlock(&_mutex);
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
