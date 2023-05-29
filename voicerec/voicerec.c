/*
 * voicerec.c
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <mosquitto.h>
#include "voicerec.h"

struct mosquitto *mosq = NULL;

static void on_connect(struct mosquitto *mosq, void *obj, int reason_code)
{
    int ret = 0;

    (void)(obj);

    if (reason_code != 0) {
        printf("%s: %s\n", __func__, mosquitto_connack_string(reason_code));
        mosquitto_disconnect(mosq);
    }

    ret = mosquitto_subscribe(mosq, NULL, "rabbit/voice/pcm", 0);
    if (ret != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "mosquitto_subscribe failed: %s\n",
                mosquitto_strerror(ret));
        mosquitto_disconnect(mosq);
    }

    ret = mosquitto_subscribe(mosq, NULL, "rabbit/voice/state", 0);
    if (ret != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "mosquitto_subscribe failed: %s\n",
                mosquitto_strerror(ret));
        mosquitto_disconnect(mosq);
    }
}

static void on_publush(struct mosquitto *mosq, void *obj, int mid)
{
    (void)(mosq);
    (void)(obj);
    (void)(mid);

    //printf("Mosquitto::onPublish: %d\n", mid);
}

static void on_subscribe(struct mosquitto *mosq, void *obj,
                         int mid, int qos_count, const int *granted_qos)
{
	int i;
	bool have_subscription = false;

    (void)(mosq);
    (void)(obj);
    (void)(mid);

	for (i = 0; i < qos_count; i++) {
		printf("%s: %d: qos=%d\n", __func__, i, granted_qos[i]);
		if (granted_qos[i] <= 2){
			have_subscription = true;
		}
	}

	if (have_subscription == false) {
		fprintf(stderr, "%s: all rejected!\n", __func__);
		mosquitto_disconnect(mosq);
	}
}

static void on_message(struct mosquitto *mosq, void *obj,
                       const struct mosquitto_message *msg)
{
    (void)(mosq);
    (void)(obj);

    if (strcmp(msg->topic, "rabbit/voice/pcm") == 0) {
        filter_pcm_in(msg->payload, msg->payloadlen);
    } else if (strcmp(msg->topic, "rabbit/voice/state") == 0) {
        const char *str = (const char *) msg->payload;
        if (str != NULL) {
            if ((msg->payloadlen == 2) &&
                (str[0] == 'o') && (str[1] == 'n')) {
                /* No need to do anything */
            } else if ((msg->payloadlen == 3) &&
                       (str[0] == 'o') && (str[1] == 'f') && (str[2] == 'f')) {
                wav_stop();
            }
        }
    } else {
        fprintf(stderr, "%s: off-topic: %s\n", __func__, msg->topic);
    }
}

static void cleanup(void)
{
    int ret;

    if (mosq) {
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

        mosquitto_destroy(mosq);
        mosq = NULL;
    }

    mosquitto_lib_cleanup();
    voicerec_storage_cleanup();
}

static void sig_handler(int signal)
{
    switch (signal) {
    case SIGINT:
        exit(EXIT_SUCCESS);
        break;
    default:
        fprintf(stderr, "Caught unexpected signal %d\n", signal);
        exit(EXIT_FAILURE);
        break;
    }
}

int main(int argc, char **argv)
{
    int ret;

    (void)(argc);
    (void)(argv);

    atexit(cleanup);
    signal(SIGINT, sig_handler);

    voicerec_storage_init();
    ret = mosquitto_lib_init();
    if (ret != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "mosquitto_lib_init failed (%d)!\n", ret);
        exit(EXIT_FAILURE);
    }

    mosq = mosquitto_new("Rabbit Voice Recognition", true, NULL);
    if (mosq == NULL) {
        fprintf(stderr, "mosquitto_new failed!\n");
        exit(EXIT_FAILURE);
    }

    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_publish_callback_set(mosq, on_publush);
    mosquitto_subscribe_callback_set(mosq, on_subscribe);
    mosquitto_message_callback_set(mosq, on_message);

    ret = mosquitto_connect(mosq, "rabbit", 1883, 60);
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

    for (;;) {
        ret = mosquitto_loop_forever(mosq, -1, 1);
        fprintf(stderr, "mosquitto_loop_forever: %s\n",
                mosquitto_strerror(ret));
    }

    return 0;
}

void voicerec_wav_publish(const char *filename)
{
    int ret;

    if (filename == NULL) {
        return;
    }

    if (mosq == NULL) {
        return;
    }

    ret = mosquitto_publish(mosq, NULL, "rabbit/voice/wav",
                            (size_t) strlen(filename),
                            (const void *) filename,
                            1, 0);
	if (ret != MOSQ_ERR_SUCCESS){
		fprintf(stderr, "mosquitto_publish failed: %s\n",
                mosquitto_strerror(ret));
    }
}

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
