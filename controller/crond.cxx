/*
 * crond.xxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <bsd/sys/time.h>
#include <pthread.h>
#include <string.h>
#include "crond.hxx"

using namespace std;

static unsigned int instance = 0;

Crond::Crond()
{
    if (instance != 0) {
        fprintf(stderr, "Crond can be instantiated only once!\n");
        exit(EXIT_FAILURE);
    } else {
        instance++;
    }

    _running = true;
    pthread_mutex_init(&_mutex, NULL);
    pthread_cond_init(&_cond, NULL);
    pthread_create(&_thread, NULL, Crond::thread_func, this);

    printf("Crond is online\n");
}

Crond::~Crond()
{
    _running = false;
    pthread_cond_broadcast(&_cond);
    pthread_join(_thread, NULL);
    pthread_mutex_destroy(&_mutex);
    pthread_cond_destroy(&_cond);

    instance--;
    printf("Crond is offline\n");
}

void *Crond::thread_func(void *args)
{
    Crond *crond = (Crond *) args;

    crond->run();

    return NULL;
}

void Crond::run(void)
{
    struct timespec ts, tloop;
    vector<struct crond_entry>::iterator it;
    const struct tm *tm;

    clock_gettime(CLOCK_REALTIME, &ts);

    /* First, sleep until the next 0 second time epoch */
    tm = localtime(&ts.tv_sec);
    ts.tv_sec += (60 - tm->tm_sec);
    pthread_mutex_lock(&_mutex);
    pthread_cond_timedwait(&_cond, &_mutex, &ts);
    pthread_mutex_unlock(&_mutex);

    /* From now on, we sleep every 60 seconds */
    tloop.tv_sec = 60;
    tloop.tv_nsec = 0;

    while (_running) {
        tm = localtime(&ts.tv_sec);
        timespecadd(&ts, &tloop, &ts);

        for (it = _entries.begin(); it != _entries.end(); it++) {
            if (it->map.m[tm->tm_min] == true &&
                it->map.h[tm->tm_hour] == true &&
                it->map.dom[tm->tm_mday - 1] == true &&
                it->map.mon[tm->tm_mon] == true &&
                it->map.dow[tm->tm_wday] == true) {
                it->f();
            }
        }

        pthread_mutex_lock(&_mutex);
        pthread_cond_timedwait(&_cond, &_mutex, &ts);
        pthread_mutex_unlock(&_mutex);
    }
}

static int translate_spec(struct crond_map *map, const char *__spec)
{
    int ret = 0;
    char *spec = NULL;
    char *m_spec = NULL;
    char *h_spec = NULL;
    char *dom_spec = NULL;
    char *mon_spec = NULL;
    char *dow_spec = NULL;
    unsigned long int divisor, val;
    char *endptr;
    unsigned int i;
    const char *s;

    if (map == NULL || __spec == NULL) {
        ret = -1;
        goto done;
    }

    spec = strdup(__spec);
    memset(map, 0x0, sizeof(*map));

    s = strtok((char *) spec, " \t");
    if (s == NULL) {
        ret = -1;
        goto done;
    } else {
        m_spec = strdup(s);
    }

    s = strtok(NULL, " \t");
    if (s == NULL) {
        ret = -1;
        goto done;
    } else {
        h_spec = strdup(s);
    }

    s = strtok(NULL, " \t");
    if (s == NULL) {
        ret = -1;
        goto done;
    } else {
        dom_spec = strdup(s);
    }

    s = strtok(NULL, " \t");
    if (s == NULL) {
        ret = -1;
        goto done;
    } else {
        mon_spec = strdup(s);
    }

    s = strtok(NULL, " \t");
    if (s == NULL) {
        ret = -1;
        goto done;
    } else {
        dow_spec = strdup(s);
    }

    if (strtok(NULL, " \t") != NULL) {
        ret = -1;
        goto done;
    }

    /* Process m_spec */

    if (m_spec[0] == '*' && m_spec[1] == '\0') {
        divisor = 60;
    } else if (m_spec[0] == '*' && m_spec[1] == '/') {
        divisor = strtoul(&m_spec[2], &endptr, 10);
        if (*endptr != '\0') {
            ret = -1;
            goto done;
        } else if (divisor <= 0 || divisor > 60) {
            ret = -1;
            goto done;
        }
    } else {
        divisor = 0;
    }

    if (divisor > 0) {
        for (i = 0; i < 60; i += (60 / divisor)) {
            map->m[i] = true;
        }
    } else {
        for (s = strtok(m_spec, ","); s != NULL; s = strtok(m_spec, ",")) {
            val = strtoul(s, &endptr, 10);
            if (*endptr != '\0') {
                ret = -1;
                goto done;
            } else if (val <= 0 || val > 60) {
                ret = -1;
                goto done;
            } else {
                map->m[val] = true;
            }
        }
    }

    /* Process h_spec */

    if (h_spec[0] == '*' && h_spec[1] == '\0') {
        divisor = 24;
    } else if (h_spec[0] == '*' && h_spec[1] == '/') {
        divisor = strtoul(&h_spec[2], &endptr, 10);
        if (*endptr != '\0') {
            ret = -1;
            goto done;
        } else if (divisor <= 0 || divisor > 24) {
            ret = -1;
            goto done;
        }
    } else {
        divisor = 0;
    }

    if (divisor > 0) {
        for (i = 0; i < 24; i += (24 / divisor)) {
            map->h[i] = true;
        }
    } else {
        for (s = strtok(h_spec, ","); s != NULL; s = strtok(h_spec, ",")) {
            val = strtoul(s, &endptr, 10);
            if (*endptr != '\0') {
                ret = -1;
                goto done;
            } else if (val <= 0 || val > 24) {
                ret = -1;
                goto done;
            } else {
                map->h[val] = true;
            }
        }
    }

    /* Process dom_spec */

    if (dom_spec[0] == '*' && dom_spec[1] == '\0') {
        divisor = 31;
    } else if (dom_spec[0] == '*' && dom_spec[1] == '/') {
        divisor = strtoul(&dom_spec[2], &endptr, 10);
        if (*endptr != '\0') {
            ret = -1;
            goto done;
        } else if (divisor <= 0 || divisor > 31) {
            ret = -1;
            goto done;
        }
    } else {
        divisor = 0;
    }

    if (divisor > 0) {
        for (i = 0; i < 31; i += (31 / divisor)) {
            map->dom[i] = true;
        }
    } else {
        for (s = strtok(dom_spec, ","); s != NULL; s = strtok(dom_spec, ",")) {
            val = strtoul(s, &endptr, 10);
            if (*endptr != '\0') {
                ret = -1;
                goto done;
            } else if (val <= 0 || val > 31) {
                ret = -1;
                goto done;
            } else {
                map->dom[val] = true;
            }
        }
    }

    /* Process mon_spec */

    if (mon_spec[0] == '*' && mon_spec[1] == '\0') {
        divisor = 12;
    } else if (mon_spec[0] == '*' && mon_spec[1] == '/') {
        divisor = strtoul(&mon_spec[2], &endptr, 10);
        if (*endptr != '\0') {
            ret = -1;
            goto done;
        } else if (divisor <= 0 || divisor > 12) {
            ret = -1;
            goto done;
        }
    } else {
        divisor = 0;
    }

    if (divisor > 0) {
        for (i = 0; i < 12; i += (12 / divisor)) {
            map->mon[i] = true;
        }
    } else {
        for (s = strtok(mon_spec, ","); s != NULL; s = strtok(mon_spec, ",")) {
            val = strtoul(s, &endptr, 10);
            if (*endptr != '\0') {
                ret = -1;
                goto done;
            } else if (val <= 0 || val > 12) {
                ret = -1;
                goto done;
            } else {
                map->mon[val] = true;
            }
        }
    }

    /* Process dow_spec */

    if (dow_spec[0] == '*' && dow_spec[1] == '\0') {
        divisor = 7;
    } else if (dow_spec[0] == '*' && dow_spec[1] == '/') {
        divisor = strtoul(&dow_spec[2], &endptr, 10);
        if (*endptr != '\0') {
            ret = -1;
            goto done;
        } else if (divisor <= 0 || divisor > 7) {
            ret = -1;
            goto done;
        }
    } else {
        divisor = 0;
    }

    if (divisor > 0) {
        for (i = 0; i < 7; i += (7 / divisor)) {
            map->dow[i] = true;
        }
    } else {
        for (s = strtok(dow_spec, ","); s != NULL; s = strtok(dow_spec, ",")) {
            val = strtoul(s, &endptr, 10);
            if (*endptr != '\0') {
                ret = -1;
                goto done;
            } else if (val <= 0 || val > 7) {
                ret = -1;
                goto done;
            } else {
                map->dow[val] = true;
            }
        }
    }

    ret = 0;

done:

    if (spec != NULL) {
        free(spec);
    }

    if (m_spec != NULL) {
        free(m_spec);
    }

    if (h_spec != NULL) {
        free(h_spec);
    }

    if (dom_spec != NULL) {
        free(dom_spec);
    }

    if (mon_spec != NULL) {
        free(mon_spec);
    }

    if (dow_spec != NULL) {
        free(dow_spec);
    }

    return ret;
}

void Crond::activate(void (*f)(void), const char *spec)
{
    struct crond_entry entry;

    translate_spec(&entry.map, spec);
    entry.f = f;

    _entries.push_back(entry);
}

void Crond::deactivate(void (*f)(void), const char *spec)
{
    struct crond_map map;
    vector<struct crond_entry>::iterator it;

    if (spec != NULL) {
        translate_spec(&map, spec);
    }

    for (it = _entries.begin(); it != _entries.end(); it++) {
        if (it->f == f) {
            if (spec == NULL || memcmp(&map, &(it->map), sizeof(map)) == 0) {
                _entries.erase(it);
            }
        }
    }
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
