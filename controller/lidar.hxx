/*
 * lidar.hxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#ifndef LIDAR_HXX
#define LIDAR_HXX

class LiDAR {

public:

    LiDAR();
    ~LiDAR();

    bool isEnabled(void) const;
    void enable(bool en);

    unsigned int speed(void) const;
    void setSpeed(unsigned int speed);
    unsigned int rpm(void) const;
    unsigned int pps(void) const;

private:

    bool processData(const uint8_t *buf, size_t size);
    void thread_wait_interruptible(unsigned int ms);
    static void *thread_func(void *args);
    void run(void);

    int _handle;
    bool _operational;
    unsigned int _speed;
    unsigned int _rpm;
    unsigned int _pps;

    bool _running;
    pthread_t _thread;
    pthread_mutex_t _mutex;
    pthread_cond_t _cond;

};

inline bool LiDAR::isEnabled(void) const
{
    return _operational;
}

inline unsigned int LiDAR::rpm(void) const
{
    return _rpm;
}

inline unsigned int LiDAR::pps(void) const
{
    return _pps;
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
