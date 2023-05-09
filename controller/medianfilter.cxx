/*
 * medianfilter.cxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "medianfilter.hxx"

template <typename T>
MedianFilter<T>::MedianFilter(unsigned int windowSize)
{
    _windowSize = windowSize;
    _samples = new T[windowSize];
    _sorted = new T[windowSize];
    _index = 0;
    _median = (T) 0;
    _medianUp2date = false;
    _average = (T) 0;
    _averageUp2date = false;
}

template <typename T>
MedianFilter<T>::~MedianFilter()
{
    if (_samples) {
        delete [] _samples;
        _samples = NULL;
    }

    if (_sorted) {
        delete [] _sorted;
        _sorted = NULL;
    }
}

template <typename T>
static int compar(T x, T y)
{
    if (x > y) {
        return 1;
    }

    if (x < y) {
        return -1;
    }

    return 0;
}

template <typename T>
void MedianFilter<T>::addSample(T s)
{
    _samples[_index] = s;
    _sampleCount++;
    _index++;
    _index %= _windowSize;

    _medianUp2date = false;
    _averageUp2date = false;
}

template <typename T>
T MedianFilter<T>::median(void)
{
    unsigned int len;

    if (_medianUp2date) {
        return _median;
    }

    if (_sampleCount < _windowSize) {
        len = _sampleCount;
    } else {
        len = _windowSize;
    }

    memcpy(_sorted, _samples, sizeof(T) * len);
    qsort(_sorted, len, sizeof(T), compar);
    _median = _sorted[len / 2];

    _medianUp2date = true;

    return _median;
}

template <typename T>
T MedianFilter<T>::average(void)
{
    unsigned int i;
    unsigned int len;

    if (_averageUp2date) {
        return _average;
    }

    if (_sampleCount < _windowSize) {
        len = _sampleCount;
    } else {
        len = _windowSize;
    }

    for (i = 0; i < len; i++) {
        if (i == 0) {
            _average = _samples[i];
        } else {
            _average = _average * (i - 1) / i + (_samples[i] / i);
        }
    }

    _averageUp2date = true;

    return _average;
}

/*
 * Explicit instantiation of types to template:
 */

template class MedianFilter<float>;

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
