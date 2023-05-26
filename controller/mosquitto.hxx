/*
 * mosquitto.hxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#ifndef MOSQUITTO_HXX
#define MOSQUITTO_HXX

struct mosquitto;

class Mosquitto {

public:

    Mosquitto();
    ~Mosquitto();

    int publish(const char *topic,
                int payloadlen, const void *payload,
                int qos, bool retain);

private:

    static void onConnect(struct mosquitto *mosq, void *obj, int reason_code);
    static void onPublush(struct mosquitto *mosq, void *obj, int mid);

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
