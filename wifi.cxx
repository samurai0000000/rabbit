/*
 * wifi.cxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <iwlib.h>
#include "rabbit.hxx"

#define WIFI_DEV "wlan0"

static const char *wifi_dev = WIFI_DEV;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"

int WIFI::stat(struct wifi_stat *wifi_stat)
{
    int sockfd = -1;
    struct iwreq request;
    struct iw_range range;
    uint8_t wev;
    struct timeval ts, te, tdiff;
    bool reply = false;
    bool timeout = false;
    char buf[0xffff];

    if (wifi_stat == NULL) {
        goto done;
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("socket");
        goto done;
    }

    if ((iw_get_range_info(sockfd, wifi_dev, &range) < 0) ||
        (range.we_version_compiled < 14)) {
        printf("%s - interface doesn't support scanning\n", wifi_dev);
        goto done;
    } else {
        wev = range.we_version_compiled;
    }

    gettimeofday(&ts, NULL);

    while ((reply == false) && (timeout == false)) {
        memset(buf, 0, sizeof(buf));
        request.u.data.pointer = buf;
        request.u.data.length = sizeof(buf);
        request.u.data.flags = 0;

        int result = iw_get_ext(sockfd,
                                wifi_dev,
                                SIOCGIWSCAN,
                                &request);
        if (result == -1) {
            if (errno != EAGAIN) {
                perror("iw_get_ext(SICGIWSCAN)");
                goto done;
            }
        }

        if (result == 0) {
            reply = true;
        }

        gettimeofday(&te, NULL);
        timersub(&te, &ts, &tdiff);

        if (tdiff.tv_sec > 10) {
            timeout = true;
        } else if (reply == false) {
            usleep(100000);
        }
    }

    if (reply) {
        struct iw_event iwe;
        struct stream_descr stream;

        iw_init_event_stream(&stream,
                             buf,
                             request.u.data.length);

        char eventBuffer[512];

        while (iw_extract_event_stream(&stream, &iwe, wev) > 0) {
            switch(iwe.cmd) {
            case SIOCGIWAP:
                if (1) {
                    const struct ether_addr *eap =
                        (const struct ether_addr *) iwe.u.ap_addr.sa_data;
                    iw_ether_ntop(eap, eventBuffer);
                    snprintf(wifi_stat->ap, sizeof(wifi_stat->ap) - 1,
                             "%s", eventBuffer);
                }
                break;
            case SIOCGIWFREQ:
                if (1) {
                    double frequency = iw_freq2float(&(iwe.u.freq));
                    int channel = iw_freq_to_channel(frequency, &range);

                    if (frequency > 1.0) {
                        frequency /= 1000000.0;
                    }

                    if (channel != -1) {
                        wifi_stat->chan = channel;
                    }
                }
                break;
            case SIOCGIWESSID:
                if (1) {
                    char _essid[IW_ESSID_MAX_SIZE + 10];
                    memset(_essid, '\0', sizeof(_essid));

                    if ((iwe.u.essid.pointer) && (iwe.u.essid.length)) {
                        memcpy(_essid,
                               iwe.u.essid.pointer,
                               iwe.u.essid.length);
                    }

                    if (iwe.u.essid.flags) {
                        if((iwe.u.essid.flags & IW_ENCODE_INDEX) > 1) {
                            char encodeIndex[10];
                            sprintf(encodeIndex,
                                    " [%d]",
                                    iwe.u.essid.flags & IW_ENCODE_INDEX);
                            strcat(_essid, encodeIndex);
                        }
                    } else {
                        strcpy(_essid, "hidden");
                    }

                    strncpy(wifi_stat->essid, _essid,
                            sizeof(wifi_stat->essid));
                }
                break;
            case IWEVQUAL:
                if (1) {
                    double signalQuality =
                        (iwe.u.qual.qual * 100.0) / range.max_qual.qual;

                        wifi_stat->link_quality = (unsigned int) signalQuality;
                }
                if (1) {
                    if (iwe.u.qual.updated & IW_QUAL_RCPI) {
                        double signalLevel = (iwe.u.qual.level / 2.0) - 110.0;
                        wifi_stat->signal_level = (int) signalLevel;
                    } else if (iwe.u.qual.updated & IW_QUAL_DBM) {
                        int dbLevel = iwe.u.qual.level;

                        if (dbLevel >= 64) {
                            dbLevel -= 0x100;
                        }

                        wifi_stat->signal_level = dbLevel;
                    } else if ((iwe.u.qual.updated & IW_QUAL_LEVEL_INVALID) ==
                               0) {
                        wifi_stat->signal_level =
                            iwe.u.qual.level / range.max_qual.level;
                    }
                }
                break;
            default:
                break;
            }
        }
    }

done:

    if (sockfd != -1) {
        close(sockfd);
    }

    return 0;
}

#pragma GCC diagnostic pop

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
