/*
 * mosquitto.hxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#ifndef MOSQUITTO_HXX
#define MOSQUITTO_HXX

struct mosquitto;
struct mosquitto_message;

class Mosquitto {

public:

    Mosquitto();
    ~Mosquitto();

    int publish(const char *topic,
                int payloadlen, const void *payload,
                int qos, bool retain);

    unsigned int published(void) const;
    unsigned int publishConfirmed(void) const;
    unsigned int messaged(void) const;

private:

    static void onConnect(struct mosquitto *mosq, void *obj, int reason_code);
    static void onPublish(struct mosquitto *mosq, void *obj, int mid);
    static void onSubscribe(struct mosquitto *mosq, void *obj,
                            int mid, int qos_count, const int *granted_qos);
    static void onMessage(struct mosquitto *mosq, void *obj,
                          const struct mosquitto_message *msg);

    unsigned int _published;
    unsigned int _publishConfirmed;
    unsigned int _messaged;

};

inline unsigned int Mosquitto::published(void) const
{
    return _published;
}

inline unsigned int Mosquitto::publishConfirmed(void) const
{
    return _publishConfirmed;
}

inline unsigned int Mosquitto::messaged(void) const
{
    return _messaged;
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
