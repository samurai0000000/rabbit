/*
 * crond.hxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#ifndef CROND_HXX
#define CROND_HXX

#include <vector>

struct crond_map {
    bool m[60];
    bool h[60];
    bool dom[31];
    bool mon[12];
    bool dow[7];
};

struct crond_entry
{
    void (*f)(void);
    struct crond_map map;
};

class Crond {

public:

    Crond();
    ~Crond();

    void activate(void (*f)(void), const char *spec);
    void deactivate(void (*f)(void), const char *spec = NULL);

private:

    static void *thread_func(void *);
    void run(void);

    std::vector<struct crond_entry> _entries;

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
