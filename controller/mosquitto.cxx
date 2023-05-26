/*
 * mosquitto.cxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <string>
#include <mosquitto.h>
#include "rabbit.hxx"

using namespace std;

struct mosquitto *mosq = NULL;

Mosquitto::Mosquitto()
{
    int ret;
    string onMessage("Rabbit Controller is Online");

    if (mosq != NULL) {
        fprintf(stderr, "Mosquitto constructor called more than once!\n");
        exit(EXIT_FAILURE);
    }

    ret = mosquitto_lib_init();
    if (ret != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "mosquitto_lib_init failed (%d)!\n", ret);
        exit(EXIT_FAILURE);
    }

    mosq = mosquitto_new(NULL, true, NULL);
    if (mosq == NULL) {
        fprintf(stderr, "mosquitto_new failed!\n");
        exit(EXIT_FAILURE);
    }

    ret = mosquitto_reinitialise(mosq, "Rabbit Controller", true, this);
    if (ret != 0) {
        fprintf(stderr, "mosquitto_reinitialize failed: %s\n",
                mosquitto_strerror(ret));
        exit(EXIT_FAILURE);
    }

    mosquitto_connect_callback_set(mosq, this->onConnect);
    mosquitto_publish_callback_set(mosq, this->onPublush);

    ret = mosquitto_connect(mosq, "localhost", 1883, 60);
    if (ret != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "mosquitto_connect failed: %s\n",
                mosquitto_strerror(ret));
        exit(EXIT_FAILURE);
    }

    ret = mosquitto_loop_start(mosq);
    if (ret != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "mosquitto_loop_start failed: %s\n",
                mosquitto_strerror(ret));
        exit(EXIT_FAILURE);
    }

    publish("rabbit/controller",
            onMessage.length(), onMessage.c_str(),
            0, false);
}

Mosquitto::~Mosquitto()
{
    int ret;
    string offMessage("Rabbit Controller is Offline");

    publish("rabbit/controller",
            offMessage.length(), offMessage.c_str(),
            0, false);

    ret = mosquitto_disconnect(mosq);
    if (ret != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "mosquitto_disconnect failed: %s\n",
                mosquitto_strerror(ret));
    }

    ret = mosquitto_loop_stop(mosq, false);
    if (ret != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "mosquitto_loop_stop failed: %s\n",
                mosquitto_strerror(ret));
    }

    if (mosq) {
        mosquitto_destroy(mosq);
        mosq = NULL;
    }

    mosquitto_lib_cleanup();
}

void Mosquitto::onConnect(struct mosquitto *mosq, void *obj, int reason_code)
{
    (void)(mosq);
    (void)(obj);

    printf("Mosquitto::onConnect: %s\n", mosquitto_connack_string(reason_code));
}

void Mosquitto::onPublush(struct mosquitto *mosq, void *obj, int mid)
{
    (void)(mosq);
    (void)(obj);
    (void)(mid);

    //printf("Mosquitto::onPublush: %d\n", mid);
}

int Mosquitto::publish(const char *topic,
                       int payloadlen, const void *payload,
                       int qos, bool retain)
{
    int ret = mosquitto_publish(mosq, NULL,
                                topic,
                                payloadlen, payload,
                                qos, retain);
	if (ret != MOSQ_ERR_SUCCESS){
		fprintf(stderr, "mosquitto_publish failed: %s\n",
                mosquitto_strerror(ret));
	}

    return ret;
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
