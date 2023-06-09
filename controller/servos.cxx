/*
 * servos.cxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <math.h>
#include <sys/time.h>
#include <bsd/sys/time.h>
#include <pigpio.h>
#include "rabbit.hxx"
#include "pca9685_regs.h"

/*
 * PCA9685
 * https://www.mouser.com/datasheet/2/737/PCA9685-932827.pdf
 * https://cdn-shop.adafruit.com/datasheets/PCA9685.pdf
 * https://www.waveshare.com/wiki/Servo_Driver_HAT
 */
#define PWM_I2C_BUS    1
#define PWM_FREQ_KHZ   50

using namespace std;

static const unsigned int PWM_I2C_ADDR[SERVO_CONTROLLERS] = {
    0x40,
    0x41,
};

static unsigned int instance = 0;

Servos::Servos()
    : _handle(),
      _freq(PWM_FREQ_KHZ)
{
    unsigned int i;

    if (instance != 0) {
        fprintf(stderr, "Servos can be instantiated only once!\n");
        exit(EXIT_FAILURE);
    } else {
        instance++;
    }

    for (i = 0; i < SERVO_CONTROLLERS; i++) {
        _handle[i] = -1;
    }

    for (i = 0; i < SERVO_CHANNELS; i++) {
        _lo[i] = 450;
        _hi[i] = 2500;
    }

    _running = true;
    pthread_mutex_init(&_mutex, NULL);
    pthread_cond_init(&_cond, NULL);
    pthread_create(&_thread, NULL, Servos::thread_func, this);
    pthread_setname_np(_thread, "R'Servos");

    printf("Servos is online\n");
}

Servos::~Servos()
{
    unsigned int i;

    _running = false;
    pthread_cond_broadcast(&_cond);
    pthread_join(_thread, NULL);
    pthread_mutex_destroy(&_mutex);
    pthread_cond_destroy(&_cond);

    for (i = 0; i < SERVO_CONTROLLERS; i++) {
        if (_handle[i] >= 0) {
            i2cWriteByteData(_handle[i], MODE1_REG, MODE1_SLEEP);
            i2cClose(_handle[i]);
            _handle[i] = -1;
        }
    }

    instance--;
    printf("Servos is offline\n");
}

void Servos::probeOpenDevice(void)
{
    unsigned int i;

    for (i = 0; i < SERVO_CONTROLLERS; i++) {
        if (_handle[i] == -1) {
            _handle[i] = i2cOpen(PWM_I2C_BUS, PWM_I2C_ADDR[i], 0x0);
            if (_handle[i] < 0) {
                _handle[i] = -1;
                continue;
            } else {
                uint8_t v;
                float prescale;

                if (i2cReadByteData(_handle[i], 0x0) < 0) {
                    i2cClose(_handle[i]);
                    _handle[i] = -1;
                    continue;
                }

                prescale = 25000000;
                prescale /= 4096.0;
                prescale /= (float) _freq;
                prescale -= 1.0;
                v = floor(prescale + 0.5);

                writeReg(i, MODE2_REG, MODE2_OUTDRV);
                writeReg(i, PRE_SCALE_REG, v);
                writeReg(i, MODE1_REG, MODE1_RESTART);
            }
        }
    }
}

int Servos::readReg(unsigned int id, uint8_t reg, uint8_t *val)
{
    int ret = 0;

    if (_handle[id] < 0) {
        return _handle[id];
    }

    ret = i2cReadByteData(_handle[id], reg);
    if (ret < 0) {
        fprintf(stderr, "Servos::readReg i2cReadByteData 0x%.2x failed!\n",
                reg);
        i2cClose(_handle[id]);
        _handle[id] = -1;
    } else {
        *val = ret;
        ret = 0;
    }

    return ret;
}

int Servos::writeReg(unsigned int id, uint8_t reg, uint8_t val)
{
    int ret = 0;

    if (_handle[id] < 0) {
        return _handle[id];
    }

    ret = i2cWriteByteData(_handle[id], reg, val);
    if (ret != 0) {
        fprintf(stderr, "Servos::writeReg: i2cWriteByteData 0x%.2x failed!\n",
                reg);
        i2cClose(_handle[id]);
        _handle[id] = -1;
    }

    return ret;
}

void Servos::setPwm(unsigned int chan, unsigned int on, unsigned int off)
{
    unsigned int id;

    id = chan / 16;
    chan = chan % 16;

    writeReg(id, LED_ON_L_REG(chan), on & 0xff);
    writeReg(id, LED_ON_H_REG(chan), on >> 8);
    writeReg(id, LED_OFF_L_REG(chan), off & 0xff);
    writeReg(id, LED_OFF_H_REG(chan), off >> 8);
}

void Servos::setRange(unsigned int chan, unsigned int lo, unsigned int hi)
{
    if (chan >= SERVO_CHANNELS) {
        return;
    } else if (_lo >= _hi) {
        return;
    }

    _lo[chan] = lo;
    _hi[chan] = hi;
}

void Servos::setPulse(unsigned int chan, unsigned int pulse,
                      bool ignoreRange, bool clearMotions)
{
    unsigned int on, off;

    if (chan >= SERVO_CHANNELS) {
        return;
    }

    if (ignoreRange == false) {
        if (pulse < _lo[chan]) {
            pulse = _lo[chan];
        }

        if (pulse > _hi[chan]) {
            pulse = _hi[chan];
        }
    }

    on = 0;
    off = pulse * 4096 / (1000000 / _freq);
    if (off >= 4096) {
        off = 4096;
    }

    pthread_mutex_lock(&_mutex);

    setPwm(chan, on, off);
    _pulse[chan] = pulse;
    if (clearMotions) {
        _motions[chan].clear();
    }

    pthread_mutex_unlock(&_mutex);
}

void Servos::setPct(unsigned int chan, unsigned int pct)
{
    unsigned int pulse;

    if (pct > 100) {
        pct = 100;
    }

    pulse = ((_hi[chan] - _lo[chan]) * pct / 100) + _lo[chan];
    setPulse(chan, pulse, false);
}

void Servos::center(unsigned int chan)
{
    unsigned int pulse;

    if (chan >= SERVO_CHANNELS) {
        return;
    }

    pulse = (_lo[chan] + _hi[chan]) / 2;
    setPulse(chan, pulse);
}

void *Servos::thread_func(void *args)
{
    Servos *servos = (Servos *) args;

    servos->run();

    return NULL;
}

void Servos::run(void)
{
    struct timespec ts, tn;
    unsigned int chan;
    unsigned int pulse;
    unsigned int on, off;
    bool unfinished = false;
    vector<struct servo_motion_sync *>::iterator it;

    tn.tv_sec = 0;
    tn.tv_nsec = 1000000 * SERVO_SCHEDULE_INTERVAL_MS;

    do {
        unfinished = false;          /* Reset unfinished variable */

        clock_gettime(CLOCK_REALTIME, &ts);
        timespecadd(&ts, &tn, &ts);  /* Update time to next epoch */

        probeOpenDevice();           /* Probe and open device(s) */

        pthread_mutex_lock(&_mutex);

        /*
         * Service each channel.
         */
        for (chan = 0; chan < SERVO_CHANNELS; chan++) {
            if ((chan % 16 == 0) && _handle[chan / 16] == -1) {
                continue;  // Servo controller is offline
            }

            if (_motions[chan].empty()) {
                continue;  // Empty schedule for channel
            }

            struct servo_motion_exec &e = _motions[chan].at(0);
            e.elapsed_intervals++;
            e.f_current_pulse += e.f_steps_per_interval;
            pulse = (unsigned int) ceil(e.f_current_pulse);

            if (pulse < _lo[chan]) {
                pulse = _lo[chan];
            }

            if (pulse > _hi[chan]) {
                pulse = _hi[chan];
            }

            if ((e.elapsed_intervals >= e.intervals)) {
                pulse = e.motion.pulse;
                _motions[chan].erase(_motions[chan].begin());
            }

            //printf("chan=%u interval=%u target=%u current=%u\n",
            //       chan, e.elapsed_intervals, e.motion.pulse, pulse);

            on = 0;
            off = pulse * 4096 / (1000000 / _freq);

            if (_pulse[chan] != pulse) {
                setPwm(chan, on, off);
                _pulse[chan] = pulse;
            }
        }

        /* Check if there is still motion to be scheduled */
        for (chan = 0; chan < SERVO_CHANNELS; chan++) {
            if (!_motions[chan].empty()) {
                unfinished = true;
                break;
            }
        }

        /* Broadcast to synchronizers */
        for (it = _syncs.begin(); it != _syncs.end(); it++) {
            uint32_t bitmap;
            bool empty = true;

            bitmap = (*it)->bitmap;
            for (chan = 0; chan < SERVO_CHANNELS; chan++) {
                if ((bitmap & (0x1 << chan)) == 0x0) {
                    continue;
                }

                if (!_motions[chan].empty()) {
                    empty = false;
                    break;
                }
            }

            if (empty) {
                pthread_cond_broadcast(&(*it)->cond);
            }
        }

        pthread_mutex_unlock(&_mutex);

        /* Sleep until the next epoch */
        pthread_mutex_lock(&_mutex);
        pthread_cond_timedwait(&_cond, &_mutex, &ts);
        pthread_mutex_unlock(&_mutex);
    } while (_running || unfinished);

    /* Broadcast to all out-standing synchronizers */
    pthread_mutex_lock(&_mutex);
    for (it = _syncs.begin(); it != _syncs.end(); it++) {
        pthread_cond_broadcast(&(*it)->cond);
    }
    pthread_mutex_unlock(&_mutex);
}

void Servos::scheduleMotions(unsigned int chan,
                             const vector<struct servo_motion> &motions,
                             bool append)
{
    unsigned int i;
    unsigned int cur_pulse;

    if (chan >= SERVO_CHANNELS) {
        return;
    }

    if (motions.size() == 0) {
        return;
    }

    pthread_mutex_lock(&_mutex);

    if (append != true) {
        _motions[chan].clear();
    }

    if (_motions[chan].empty()) {
        cur_pulse = _pulse[chan];
    } else {
        cur_pulse = _motions[chan].back().motion.pulse;
    }

    for (i = 0; i < motions.size(); i++) {
        const struct servo_motion &m = motions.at(i);
        struct servo_motion_exec e;

        e.motion.ms = m.ms;
        e.motion.pulse = m.pulse;
        if (e.motion.pulse < _lo[chan]) {
            e.motion.pulse = _lo[chan];
        }

        if (e.motion.pulse > _hi[chan]) {
            e.motion.pulse = _hi[chan];
        }

        gettimeofday(&e.t_start, NULL);
        e.intervals = (e.motion.ms / SERVO_SCHEDULE_INTERVAL_MS) + 1;
        e.elapsed_intervals = 0;
        e.f_steps_per_interval =
            (float) ((int) e.motion.pulse - (int) cur_pulse) /
            (float) e.intervals;
        e.f_current_pulse = (float) cur_pulse;
        cur_pulse = e.motion.pulse;
        //printf("scheduled chan %u: intervals=%u spi=%.2f\n",
        //       chan, e.intervals, e.f_steps_per_interval);
        _motions[chan].push_back(e);
    }


    pthread_mutex_unlock(&_mutex);
}

void Servos::clearMotionSchedule(unsigned int chan)
{
    _motions[chan].clear();
}

void Servos::syncMotionSchedule(uint32_t chan_mask)
{
    struct servo_motion_sync sync;
    vector<struct servo_motion_sync *>::iterator it;

    if (chan_mask == 0x0) {
        return;
    }

    /* Set up */
    sync.bitmap = chan_mask;
    pthread_mutex_init(&sync.mutex, NULL);
    pthread_cond_init(&sync.cond, NULL);

    /* Add to _syncs vector */
    pthread_mutex_lock(&_mutex);
    _syncs.push_back(&sync);
    pthread_mutex_unlock(&_mutex);

    /* Wait for condition to be met and notified by running thread */
    pthread_mutex_lock(&sync.mutex);
    pthread_cond_wait(&sync.cond, &sync.mutex);
    pthread_mutex_unlock(&sync.mutex);

    /* Remove from _syncs vector */
    pthread_mutex_lock(&_mutex);
    for (it = _syncs.begin(); it != _syncs.end(); it++) {
        if (*it == &sync) {
            it = _syncs.erase(it);
            break;
        }
    }
    pthread_mutex_unlock(&_mutex);

    /* Destroy */
    pthread_mutex_destroy(&sync.mutex);
    pthread_cond_destroy(&sync.cond);
}

bool Servos::lastMotionPulseInPlan(unsigned int chan, unsigned int *pulse) const
{
    bool hasLastPulse = false;

    if (!_motions[chan].empty()) {
        hasLastPulse = true;

        if (pulse != NULL) {
            const struct servo_motion_exec &exec = _motions[chan].back();
            *pulse = exec.motion.pulse;
        }
    }

    return hasLastPulse;
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
