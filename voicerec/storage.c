/*
 * storage.c
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>
#include "voicerec.h"

struct wav_header {
    char riff[4];
    uint32_t file_size;
    char wave[4];
    char fmt_[4];
    uint32_t length;
    uint16_t type;
    uint16_t num_chans;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t bpschdiv8;
    uint16_t bps;
    char data[4];
    uint32_t data_size;
} __attribute__((packed));

static char path[256];
static time_t tfile = 0;
static struct timeval tstart;
static struct timeval tend;
static char filename[384];
static FILE *fout = NULL;

static void storage_cleanup_path(void)
{
    int ret;
    DIR *d;
    struct dirent *dir;

    ret = chdir(path);
    if (ret != 0) {
        perror(path);
        exit(EXIT_FAILURE);
    }

    d = opendir(path);
    if (d == NULL) {
        perror(path);
        exit(EXIT_FAILURE);
    }

    while ((dir = readdir(d)) != NULL) {
        size_t slen;

        if (dir->d_type != DT_REG) {
            continue;
        } else if ((dir->d_name[0] == '.') &&
            (dir->d_name[1] == '\0')) {
            continue;
        } else if ((dir->d_name[0] == '.') &&
                   (dir->d_name[1] == '.') &&
                   (dir->d_name[2] == '\0')) {
            continue;
        }

        slen = strlen(dir->d_name);
        if ((slen >= 4) && (strcasecmp(dir->d_name + slen - 4, ".wav") == 0)) {
            remove(dir->d_name);
        }
    }

    closedir(d);
}

void voicerec_storage_init(void)
{
    int ret;
    const char *home;

    home = getenv("HOME");
    if (home != NULL) {
        snprintf(path, sizeof(path) - 1, "%s/.rabbit", home);
        ret = mkdir(path, 0755);
        if (ret != 0 && errno != EEXIST) {
            perror(path);
            exit(EXIT_FAILURE);
        }
        snprintf(path, sizeof(path) - 1, "%s/.rabbit/wav", home);
        ret = mkdir(path, 0755);
        if (ret != 0 && errno != EEXIST) {
            perror(path);
            exit(EXIT_FAILURE);
        }
    } else {
        snprintf(path, sizeof(path) - 1, "/tmp/.rabbit");
        ret = mkdir(path, 0755);
        if (ret != 0 && errno != EEXIST) {
            perror(path);
            exit(EXIT_FAILURE);
        }
        snprintf(path, sizeof(path) - 1, "/tmp/.rabbit/wav");
        ret = mkdir(path, 0755);
        if (ret != 0 && errno != EEXIST) {
            perror(path);
            exit(EXIT_FAILURE);
        }
    }

    storage_cleanup_path();
}

void voicerec_storage_cleanup(void)
{
    wav_stop();
    storage_cleanup_path();
}

void wav_start(void)
{
    int ret;
    struct tm *tm;
    char buf[sizeof(filename) - sizeof(path) - 1];
    size_t empty_frames;

    if (fout != NULL) {
        wav_stop();
    }

    gettimeofday(&tstart, NULL);
    tfile = time(NULL);
    tm = localtime(&tfile);
    strftime(buf, sizeof(buf) - 1, "%Y%m%d%H%M%S.wav", tm);
    snprintf(filename, sizeof(filename) - 1, "%s/%s", path, buf);

    printf("Start recording %s\n", filename);
    fout = fopen(filename, "w");
    if (fout == NULL) {
        perror(filename);
        exit(EXIT_FAILURE);
    }


    empty_frames = ((22050 * 16 * 1) / 8) / 2;  // 0.5 second of silence
    ret = fseek(fout, sizeof(struct wav_header) + empty_frames, SEEK_SET);
    if (ret != 0) {
        perror(filename);
        exit(EXIT_FAILURE);
    }
}

void wav_pcm_in(const void *pcm, size_t size)
{
    size_t ret;

    assert(pcm != NULL);
    assert(size > 0);

    if (fout == NULL) {
        wav_start();
    }

    ret = fwrite(pcm, 1, size, fout);
    if (ret != size) {
        fprintf(stderr, "Error writing to %s at %ld!\n",
                filename, ftell(fout));
        exit(EXIT_FAILURE);
    }
}

void wav_stop(void)
{
    int ret;
    struct wav_header wav_header;

    if (fout == NULL) {
        return;
    }

    printf("Stop  recording %s\n", filename);

    wav_header.riff[0] = 'R';
    wav_header.riff[1] = 'I';
    wav_header.riff[2] = 'F';
    wav_header.riff[3] = 'F';
    wav_header.file_size = (uint32_t) ftell(fout);
    wav_header.wave[0] = 'W';
    wav_header.wave[1] = 'A';
    wav_header.wave[2] = 'V';
    wav_header.wave[3] = 'E';
    wav_header.fmt_[0] = 'f';
    wav_header.fmt_[1] = 'm';
    wav_header.fmt_[2] = 't';
    wav_header.fmt_[3] = ' ';
    wav_header.length = 16;
    wav_header.type = 1;
    wav_header.num_chans = 1;
    wav_header.sample_rate = 22050;
    wav_header.bps = 16;
    wav_header.bpschdiv8 = (wav_header.bps * wav_header.num_chans) / 8;
    wav_header.byte_rate =
        (wav_header.sample_rate * wav_header.bps * wav_header.num_chans) / 8;
    wav_header.data[0] = 'd';
    wav_header.data[1] = 'a';
    wav_header.data[2] = 't';
    wav_header.data[3] = 'a';
    wav_header.data_size = wav_header.file_size - sizeof(struct wav_header);

    ret = fseek(fout, 0, SEEK_SET);
    if (ret != 0) {
        perror(filename);
        exit(EXIT_FAILURE);
    }

    ret = (int) fwrite(&wav_header, 1, sizeof(struct wav_header), fout);
    if ((size_t) ret != sizeof(wav_header)) {
        fprintf(stderr, "Error writing wav header to %s!\n", filename);
        exit(EXIT_FAILURE);
    }

    fclose(fout);
    fout = NULL;

    gettimeofday(&tend, NULL);
}

const char *wav_filename(void)
{
    return filename;
}

unsigned int wav_time_elapsed_ms(void)
{
    unsigned int ms;
    struct timeval now, tdiff;

    if (fout == NULL) {
        timersub(&tend, &tstart, &tdiff);
    } else {
        gettimeofday(&now, NULL);
        timersub(&now, &tstart, &tdiff);
    }

    ms = (tdiff.tv_sec * 1000) +  (tdiff.tv_usec / 1000);

    return ms;
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
