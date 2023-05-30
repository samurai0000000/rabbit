/*
 * voice.hxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#ifndef VOICE_HXX
#define VOICE_HXX

#define VOL_HIST_SIZE 100

struct libusb_context;
struct libusb_device_handle;

struct vol_hist_point {
    int16_t min;
    int16_t max;
};

class Voice {

public:

    struct prop {
        uint32_t AECFreezeOnOff;
        float    AECNorm;
        uint32_t AECPathChange;
        float    RT60;
        uint32_t HPFOnOff;
        float    AECSilenceLevel;
        uint32_t AECSilenceMode;
        uint32_t AGCOnOff;
        float    AGCMaxGain;
        float    AGCDesiredLevel;
        float    AGCGain;
        float    AGCTime;
        uint32_t CNIOnOff;
        uint32_t FreezeOnOff;
        uint32_t StatNoiseOnOff;
        float    GammaNS;
        float    MinNS;
        uint32_t NonStatNoiseOnOff;
        float    GamaNN;
        float    MinNN;
        uint32_t EchoOnOff;
        float    GammaE;
        float    GammaETail;
        float    GammaENL;
        uint32_t NLAttenOnOff;
        uint32_t NLAECMode;
        uint32_t SpeechDetected;
        uint32_t FSBUpdated;
        uint32_t FSBPathChange;
        uint32_t TransientOnOff;
        uint32_t VoiceActivity;
        uint32_t StatNoiseOnOffSR;
        uint32_t NonStatNoiseOnOffSR;
        float    GammaNSSR;
        float    GammaNNSR;
        float    MinNSSR;
        float    MinNNSR;
        float    GammaVADSR;
        uint32_t DOAAngle;
    };

public:

    Voice();
    ~Voice();

    void enable(bool en);
    bool isEnabled(void) const;
    bool isOnline(void) const;

    size_t volHistSize(void) const;
    unsigned int getVolHist(struct vol_hist_point *hist,
                            unsigned int points) const;

    const Voice::prop &getProp(void) const;

private:

    void probeOpenAudioDevice(void);
    static void *thread_func(void *args);
    void run(void);

    void probeOpenUSBDevice(void);
    uint32_t readUsbIntReg(unsigned int id, unsigned int offset);
    float readUsbFloatReg(unsigned int id, unsigned int offset);
    void writeUsbIntReg(unsigned int id, unsigned int offset,
                        uint32_t value);
    void writeUsbFloatReg(unsigned int id, unsigned int offset,
                          float value);
    static void *thread_func2(void *args);
    void run2(void);

    void *_handle;
    unsigned int _rate;
    bool _enable;
    libusb_context *_usbctx;
    libusb_device_handle *_usbdev;

    struct vol_hist_point _volHist[VOL_HIST_SIZE];
    unsigned int _volHistCur;

    struct prop _prop;

    bool _running;
    pthread_t _thread;
    pthread_t _thread2;
    pthread_mutex_t _mutex;
    pthread_cond_t _cond;

};

inline bool Voice::isEnabled(void) const
{
    return _enable;
}

inline bool Voice::isOnline(void) const
{
    return _handle != NULL;
}

inline size_t Voice::volHistSize(void) const
{
    return VOL_HIST_SIZE;
}

inline const Voice::prop &Voice::getProp(void) const
{
    return _prop;
}

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
