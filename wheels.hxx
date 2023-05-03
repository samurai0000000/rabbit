/*
 * wheels.hxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#ifndef WHEELS_HXX
#define WHEELS_HXX

#include <sys/time.h>

class Wheels {

public:

    Wheels();
    ~Wheels();

    void halt(void);

    void fwd(unsigned int ms);
    void bwd(unsigned int ms);
    void ror(unsigned int ms);
    void rol(unsigned int ms);
    void fwr(unsigned int ms);
    void fwl(unsigned int ms);
    void bwr(unsigned int ms);
    void bwl(unsigned int ms);

    unsigned int state(void) const;
    const char *stateStr(void) const;

    unsigned int fwd_ms(void) const;
    unsigned int bwd_ms(void) const;
    unsigned int ror_ms(void) const;
    unsigned int rol_ms(void) const;
    unsigned int fwr_ms(void) const;
    unsigned int fwl_ms(void) const;
    unsigned int bwr_ms(void) const;
    unsigned int bwl_ms(void) const;

private:

    static void *thread_func(void *);
    void run(void);
    unsigned int now_diff_ts_ms(void) const;
    void change(unsigned int state);
    void setExpiration(unsigned int ms);
    bool hasExpired(void) const;

    unsigned int _state;
    unsigned int _rpwr;
    unsigned int _rpwrTarget;
    unsigned int _lpwr;
    unsigned int _lpwrTarget;

    bool _running;
    pthread_t _thread;
    pthread_mutex_t _mutex;
    pthread_cond_t _cond;

    struct timeval _ts;
    struct timeval _expire;
    unsigned int _fwd_ms;
    unsigned int _bwd_ms;
    unsigned int _ror_ms;
    unsigned int _rol_ms;
    unsigned int _fwr_ms;
    unsigned int _fwl_ms;
    unsigned int _bwr_ms;
    unsigned int _bwl_ms;

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
