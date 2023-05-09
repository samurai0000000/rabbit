/*
 * wifi.hxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#ifndef WIFI_HXX
#define WIFI_HXX

struct wifi_stat {
    char ap[128];
    char essid[128];
    unsigned int chan;
    unsigned int link_quality;
    int signal_level;
};

class WIFI {

public:

    static int stat(struct wifi_stat *wifi_stat);

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
