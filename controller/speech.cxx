/*
 * speech.cxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/times.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include <alsa/asoundlib.h>
#include "speech.hxx"

Speech::Speech()
    :
#if defined(ESPEAK_POPEN)
    _espeak(NULL),
#else
    _pid(-1),
    _stdin(),
#endif
    _vol(100),
    _wpm(120)
{
    pthread_mutex_init(&_mutex, NULL);

    setVolume(88);
    speak("The rabbit bot is ready to see the world!");
}

Speech::~Speech()
{
    speak("The rabbit bot is going to sleep!");

#if defined(ESPEAK_POPEN)
    if (_espeak) {
        pclose(_espeak);
        _espeak = NULL;
    }
#endif
#if defined(ESPEAK_FORK)
    if (_pid != -1) {
        int wstatus;

        close(_stdin[1]);
        _stdin[1] = -1;

        waitpid(_pid, &wstatus, 0);
        _pid = -1;
    }
#endif

    pthread_mutex_destroy(&_mutex);
}

#if defined(ESPEAK_FORK)

static void atexit_do_nothing(void)
{

}

#endif

void Speech::speak(const char *message, bool immediate, int wpm)
{
    int ret;
    char cmd[256];

    if (message == NULL) {
        goto done;
    }

    if (wpm <= 0) {
        wpm = _wpm;
    }

    snprintf(cmd, sizeof(cmd) - 1,
             "/usr/bin/espeak -s %d 2>/dev/null", wpm);

    pthread_mutex_lock(&_mutex);

#if defined(ESPEAK_PIPE)
    if ((_espeak != NULL) &&
        ((immediate == true) || (wpm != _wpm))) {
        ret = system("killall espeak >/dev/null 2>&1");
        (void)(ret);
        pclose(_espeak);
        _espeak = NULL;
    }

    if (_espeak == NULL) {
        _espeak = popen(cmd, "w");
        if (_espeak == NULL) {
            perror(cmd);
            goto done;
        }
    }

    fwrite(message, 1, strlen(message), _espeak);
    fwrite("\n", 1, 1, _espeak);
    fflush(_espeak);
#endif

#if defined(ESPEAK_FORK)
    if ((_pid != -1) &&
        ((immediate == true) || (wpm != _wpm))) {
        int wstatus;

        close(_stdin[1]);
        _stdin[1] = -1;

        ret = kill(_pid, SIGKILL);
        do {
            (void)(ret);
            waitpid(_pid, &wstatus, 0);
        } while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));
        _pid = -1;
    }

    if (_pid == -1) {
        ret = pipe(_stdin);
        if (ret != 0) {
            goto done;
        }

        _pid = fork();
        if (_pid == -1) {
            close(_stdin[0]);
            close(_stdin[1]);
            goto done;
        } else if (_pid == 0) {
            /* Child */
            atexit(atexit_do_nothing);
            signal(SIGTERM, NULL);
            signal(SIGINT, NULL);

            close(_stdin[1]);
            dup2(_stdin[0], STDIN_FILENO);

            ret = execl("/bin/sh", "sh", "-c", cmd, NULL);
            exit(ret);
        } else {
            /* Parent */
            close(_stdin[0]);
            _stdin[0] = -1;
        }
    }

    ret = write(_stdin[1], message, strlen(message));
    (void)(ret);
    ret = write(_stdin[1], "\n", 1);
    (void)(ret);
#endif

done:

    pthread_mutex_unlock(&_mutex);
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
