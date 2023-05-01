/*
 * medianfilter.hxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#ifndef MEDIANFILTER_HXX
#define MEDIANFILTER_HXX

template <typename T>
class MedianFilter
{

public:

    MedianFilter(unsigned int windowSize = 10);
    ~MedianFilter();

    void addSample(T s);
    T median(void) const;
    T average(void) const;

private:

    unsigned int _windowSize;
    T *_samples;
    T *_sorted;
    unsigned int _index;
    unsigned int _sampleCount;
    T _median;
    T _average;

};

template <typename T>
inline T MedianFilter<T>::median(void) const
{
    return _median;
}

template <typename T>
inline T MedianFilter<T>::average(void) const
{
    return _average;
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
