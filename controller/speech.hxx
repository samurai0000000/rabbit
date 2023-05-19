/*
 * speech.hxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#ifndef SPEECH_HXX
#define SPEECH_HXX

#include <string>
#include <vector>

class Speech {

public:

    Speech();
    ~Speech();

    void speak(const char *message, bool immediate = false);

    unsigned int volume(void) const;
    void setVolume(unsigned int vol);

private:

    static void *thread_func(void *args);
    void run(void);

    unsigned int _vol;

    pid_t _pid;
    std::vector<std::string> _messages;

    bool _running;
    pthread_t _thread;
    pthread_mutex_t _mutex;
    pthread_cond_t _cond;

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
