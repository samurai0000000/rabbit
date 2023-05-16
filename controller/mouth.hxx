/*
 * mouth.hxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#ifndef MOUTH_HXX
#define LOUTH_HXX

class Mouth {

public:

    Mouth();
    ~Mouth();

    unsigned int intensity(void) const;
    void setIntensity(unsigned int intensity);

    void cylon(bool enable);
    bool cylonEnabled(void) const;
    void draw(const uint32_t fb[8]);
    void smile(void);
    void beh(void);
    void displayText(const char *text,
                     bool scroll = true,
                     unsigned int speed = 1);

private:

    static void *thread_func(void *);
    void run(void);

    void writeMax7219(uint8_t reg, uint8_t data) const;

    int _handle;
    unsigned int _intensity;
    uint32_t _fb[8];

    bool _cylon;

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
