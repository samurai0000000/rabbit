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

    void start(void);
    void stop(void);

    unsigned int speed(void) const;
    void setSpeed(unsigned int speed);

private:

    void processData(const void *buf, size_t size);
    void thread_wait_interruptible(unsigned int ms);
    static void *thread_func(void *args);
    void run(void);

    int _handle;
    unsigned int _speed;
    bool _operational;

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
