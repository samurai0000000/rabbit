/*
 * speech.cxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <bsd/sys/time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include <alsa/asoundlib.h>
#include "rabbit.hxx"

using namespace std;

Speech::Speech()
    : _vol(100),
      _pid(-1)
{
    pthread_mutex_init(&_mutex, NULL);

    setVolume(85);

    _running = true;
    pthread_mutex_init(&_mutex, NULL);
    pthread_cond_init(&_cond, NULL);
    pthread_create(&_thread, NULL, Speech::thread_func, this);

    speak("The rabbit bot is ready to see the world!");
}

Speech::~Speech()
{
    speak("The rabbit bot is going to sleep!");

    _running = false;
    pthread_cond_broadcast(&_cond);
    pthread_join(_thread, NULL);
    pthread_mutex_destroy(&_mutex);
    pthread_cond_destroy(&_cond);
}

void *Speech::thread_func(void *args)
{
    Speech *speech = (Speech *) args;

    speech->run();

    return NULL;
}

static void espeak_atexit(void)
{

}

static void espeak_sighandler(int signal)
{
    (void)(signal);
    exit(0);
}

void Speech::run(void)
{
    while (_running || _messages.empty() == false) {
        if (_messages.empty()) {
            struct timespec ts, twait;

            twait.tv_sec = 1;
            twait.tv_nsec = 0;
            clock_gettime(CLOCK_REALTIME, &ts);
            timespecadd(&ts, &twait, &ts);
            pthread_mutex_lock(&_mutex);
            pthread_cond_timedwait(&_cond, &_mutex, &ts);
            pthread_mutex_unlock(&_mutex);
        } else {
            int ret;
            char cmd[256];
            int espeak_stdin[2] = { -1, -1, };
            int wstatus;
            string message;
            unsigned int mode;

            snprintf(cmd, sizeof(cmd) - 1,
                     "/usr/bin/espeak -s %d 2>/dev/null", 120);

            ret = pipe(espeak_stdin);
            if (ret != 0) {
                perror("espeak");
            } else {

                pthread_mutex_lock(&_mutex);
                mode = mouth->mode();

                _pid = fork();
                if (_pid == -1) {
                    close(espeak_stdin[0]);
                    close(espeak_stdin[1]);
                    perror("fork");
                } else if (_pid == 0) {
                    /* Child */
                    atexit(espeak_atexit);
                    signal(SIGTERM, espeak_sighandler);
                    signal(SIGINT, espeak_sighandler);

                    close(espeak_stdin[1]);
                    dup2(espeak_stdin[0], STDIN_FILENO);

                    ret = execl("/bin/sh", "sh", "-c", cmd, NULL);
                    exit(ret);
                } else {
                    /* Parent */
                    close(espeak_stdin[0]);
                    espeak_stdin[0] = -1;

                    message = _messages.at(0);
                    ret = write(espeak_stdin[1], message.c_str(), message.length());
                    (void)(ret);
                    ret = write(espeak_stdin[1], "\n", 1);
                    (void)(ret);
                    close(espeak_stdin[1]);
                    _messages.erase(_messages.begin());
                }


                pthread_mutex_unlock(&_mutex);

                if (_pid != -1) {
                    mouth->speak();
                    waitpid(_pid, &wstatus, 0);
                    _pid = -1;
                    mouth->setMode(mode);
                }
            }
        }
    }
}

void Speech::speak(const char *message, bool immediate)
{
    if (message == NULL) {
        return;
    }

    pthread_mutex_lock(&_mutex);
    if (immediate) {
        if (_pid != -1) {
            kill(_pid, SIGTERM);
        }
        _messages.clear();
    }
    _messages.push_back(message);
    pthread_mutex_unlock(&_mutex);

    pthread_cond_broadcast(&_cond);
}

unsigned int Speech::volume(void) const
{
    return _vol;
}

static long lrint_dir(double x, int dir)
{
    if (dir > 0)
        return lrint(ceil(x));
    else if (dir < 0)
        return lrint(floor(x));
    else
        return lrint(x);
}

void Speech::setVolume(unsigned int vol)
{
    int ret;
    long min, max, val;
    snd_mixer_t *handle = NULL;
    snd_mixer_selem_id_t *sid = NULL;
    const char *card = "default";
    const char *selem_name = "PCM";
    snd_mixer_elem_t *elem = NULL;

    pthread_mutex_lock(&_mutex);

    if (vol > 100) {
        vol = 100;
    }

    ret = snd_mixer_open(&handle, 0);
    if (ret != 0) {
        fprintf(stderr, "snd_mixer_open %d\n", ret);
        goto done;
    }

    ret = snd_mixer_attach(handle, card);
    if (ret != 0) {
        fprintf(stderr, "snd_mixer_attach %d\n", ret);
        goto done;
    }

    ret = snd_mixer_selem_register(handle, NULL, NULL);
    if (ret != 0) {
        fprintf(stderr, "snd_mixer_selem_register %d\n", ret);
        goto done;
    }

    ret = snd_mixer_load(handle);
    if (ret != 0) {
        fprintf(stderr, "snd_mixer_load %d\n", ret);
        goto done;
    }

    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, selem_name);

    elem = snd_mixer_find_selem(handle, sid);
    if (elem == NULL) {
        fprintf(stderr, "snd_mixer_find_selem failed\n");
        goto done;
    }

    ret = snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
    if (ret != 0) {
        fprintf(stderr, "snd_mixer_selem_get_playback_volume_range %d\n", ret);
        goto done;
    }

    val = lrint_dir(vol / 100.0 * (max - min), 0) + min;
    ret = snd_mixer_selem_set_playback_volume_all(elem, val);
    if (ret != 0) {
        fprintf(stderr, "snd_mixer_selem_set_playback_volume_all %d\n", ret);
        goto done;
    }

    _vol = 0;

done:

    if (handle) {
        snd_mixer_close(handle);
    }

    pthread_mutex_unlock(&_mutex);
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
