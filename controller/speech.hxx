/*
 * speech.hxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#ifndef SPEECH_HXX
#define SPEECH_HXX

//#define ESPEAK_POPEN
#define ESPEAK_FORK

class Speech {

public:

    Speech();
    ~Speech();

    void speak(const char *message, bool immediate = false, int wpm = -1);

    unsigned int volume(void) const;
    void setVolume(unsigned int vol);

private:

#if defined(ESPEAK_POPEN)
    FILE *_espeak;
#endif
#if defined(ESPEAK_FORK)
    pid_t _pid;
    int _stdin[2];
#endif

    unsigned int _vol;
    int _wpm;

    pthread_mutex_t _mutex;

};

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
