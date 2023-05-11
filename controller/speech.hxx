/*
 * speech.hxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#ifndef SPEECH_HXX
#define SPEECH_HXX

class Speech {

public:

    Speech();
    ~Speech();

    void speak(const char *message, bool immediate = true, int wpm = -1);

    unsigned int volume(void) const;
    void setVolume(unsigned int vol);

private:

    unsigned int _vol;
    int _wpm;
    FILE *_espeak;

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
