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

struct mosq_sub_action {
    const char *topic;
    void(*act)(const struct mosquitto_message *msg);
    int qos;
};

static void wheels_move(const struct mosquitto_message *msg)
{
    (void)(msg);  // TODO
}

static void do_speak(const struct mosquitto_message *msg)
{
    if (speech) {
        speech->speak((const char *) msg->payload);
    }
}

static mosq_sub_action mosq_sub_actions[] = {
    { "rabbit/wheels/move", wheels_move, 0, },
    { "rabbit/speech/speak", do_speak, 2, },
};

static unsigned int instance = 0;

Mosquitto::Mosquitto()
    : _published(0),
      _publishConfirmed(0),
      _messaged(0)
{
    int ret;
    string onMessage("Rabbit Controller is Online");

    if (instance != 0) {
        fprintf(stderr, "Mosquitto can be instantiated only once!\n");
        exit(EXIT_FAILURE);
    } else {
        instance++;
    }

    if (mosq != NULL) {
        fprintf(stderr, "Mosquitto constructor called more than once!\n");
        exit(EXIT_FAILURE);
    }

    ret = mosquitto_lib_init();
    if (ret != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "mosquitto_lib_init failed (%d)!\n", ret);
        exit(EXIT_FAILURE);
    }

    mosq = mosquitto_new("Rabbit Controller", true, this);
    if (mosq == NULL) {
        fprintf(stderr, "mosquitto_new failed!\n");
        exit(EXIT_FAILURE);
    }

    mosquitto_connect_callback_set(mosq, this->onConnect);
    mosquitto_publish_callback_set(mosq, this->onPublush);
    mosquitto_subscribe_callback_set(mosq, this->onSubscribe);
    mosquitto_message_callback_set(mosq, this->onMessage);

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

    printf("Mosquitto is online\n");
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
    printf("Mosquitto is offline\n");
}

void Mosquitto::onConnect(struct mosquitto *mosq, void *obj, int reason_code)
{
    int ret = 0;
    unsigned int i;
    unsigned int count;

    (void)(obj);

    if (reason_code != 0) {
        printf("Mosquitto::onConnect: %s\n",
               mosquitto_connack_string(reason_code));
        mosquitto_disconnect(mosq);
    }

    count = sizeof(mosq_sub_actions) / sizeof(struct mosq_sub_action);
    for (i = 0; i < count; i++) {
        ret = mosquitto_subscribe(mosq,
                                  NULL,
                                  mosq_sub_actions[i].topic,
                                  mosq_sub_actions[i].qos);
        if (ret != MOSQ_ERR_SUCCESS) {
            goto done;
        }
    }

done:

    if (ret != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "mosquitto_subscribe failed: %s\n",
                mosquitto_strerror(ret));
        mosquitto_disconnect(mosq);
    }
}

void Mosquitto::onPublush(struct mosquitto *mosq, void *obj, int mid)
{
    Mosquitto *mosquitto = (Mosquitto *) obj;

    (void)(mosq);
    (void)(obj);
    (void)(mid);

    //printf("Mosquitto::onPublush: %d\n", mid);
    if (mosquitto != NULL) {
        mosquitto->_publishConfirmed++;
    }
}

void Mosquitto::onSubscribe(struct mosquitto *mosq, void *obj,
                            int mid, int qos_count, const int *granted_qos)
{
	int i;
	bool have_subscription = false;

    (void)(mosq);
    (void)(obj);
    (void)(mid);

	for (i = 0; i < qos_count; i++) {
		printf("Mosquitto::onSubscribe: %d: qos=%d\n", i, granted_qos[i]);
		if (granted_qos[i] <= 2){
			have_subscription = true;
		}
	}

	if (have_subscription == false) {
		fprintf(stderr, "Mosquitto::onSubscribe: all rejected!\n");
		mosquitto_disconnect(mosq);
	}
}

void Mosquitto::onMessage(struct mosquitto *mosq, void *obj,
                          const struct mosquitto_message *msg)
{
    Mosquitto *mosquitto = (Mosquitto *) obj;
    unsigned int i;
    unsigned int count;

    (void)(mosq);

    count = sizeof(mosq_sub_actions) / sizeof(struct mosq_sub_action);
    for (i = 0; i < count; i++) {
        if (strcmp(msg->topic, mosq_sub_actions[i].topic) == 0) {
            mosq_sub_actions[i].act(msg);
            break;
        }
    }

    if (i >= count) {
        fprintf(stderr, "Mosquitto::onMessage off-topic: %s\n", msg->topic);
    }

    if (mosquitto) {
        mosquitto->_messaged++;
    }
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
	} else {
        _published++;
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
