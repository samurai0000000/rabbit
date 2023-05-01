/*
 * medianfilter.cxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "medianfilter.hxx"

template <typename T>
MedianFilter<T>::MedianFilter(unsigned int windowSize)
{
    _windowSize = windowSize;
    _samples = new T[windowSize];
    _sorted = new T[windowSize];
    _index = 0;
    _median = (T) 0;
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
    unsigned int i;
    unsigned int len;

    _samples[_index] = s;
    _sampleCount++;
    _index++;
    _index %= _windowSize;

    if (_sampleCount < _windowSize) {
        len = _sampleCount;
    } else {
        len = _windowSize;
    }

    memcpy(_sorted, _samples, sizeof(T) * len);
    qsort(_sorted, len, sizeof(T), compar);
    _median = _sorted[len / 2];
    for (i = 0, _average = (T) 0; i < len; i++) {
        _average += _sorted[i];
    }
    _average /= (T) len;
}

/*
 * Explicit instantiation of types to template:
 */

template class MedianFilter<unsigned int>;
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
