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
    T median(void);
    T average(void);

private:

    unsigned int _windowSize;
    T *_samples;
    T *_sorted;
    unsigned int _index;
    unsigned int _sampleCount;
    T _median;
    bool _medianUp2date;
    T _average;
    bool _averageUp2date;

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
