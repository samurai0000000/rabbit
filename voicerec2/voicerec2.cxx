/*
 * voicerec2.c
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <vector>
#include <filesystem>
#include <whisper.h>
#include <mosquitto.h>

using namespace std;

#define WHISPER_SAMPLE_RATE 16000
#define MAIN_SAMPLE_SECONDS 5

struct audio_sample_main {
    float data[WHISPER_SAMPLE_RATE * MAIN_SAMPLE_SECONDS];
    unsigned int samples;
};

static struct whisper_context *whisper = NULL;
static struct mosquitto *mosq = NULL;

static struct audio_sample_main audio_sample_main = {
    .data = { 0, },
    .samples = 0,
};

static void make_inference(void);

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
        unsigned int new_samples;

        new_samples = msg->payloadlen / sizeof(int16_t);
        if ((audio_sample_main.samples + new_samples) <=
            (WHISPER_SAMPLE_RATE * MAIN_SAMPLE_SECONDS)) {
            unsigned int i;
            const int16_t *pcm_s16_le = (const int16_t *) msg->payload;
            float v;

            /* Convert from S16_LE to F32 */
            for (i = 0; i < new_samples; i++) {
                v = ((float) *pcm_s16_le) / 32768.0;
                audio_sample_main.data[audio_sample_main.samples] = v;
                audio_sample_main.samples++;
                pcm_s16_le++;
            }
        }

        if (audio_sample_main.samples >=
            (WHISPER_SAMPLE_RATE * MAIN_SAMPLE_SECONDS)) {
            make_inference();
            audio_sample_main.samples = 0;
        }
    } else if (strcmp(msg->topic, "rabbit/voice/state") == 0) {
        const char *str = (const char *) msg->payload;
        if (str != NULL) {
            if ((msg->payloadlen == 2) &&
                (str[0] == 'o') && (str[1] == 'n')) {
                /* No need to do anything */
            } else if ((msg->payloadlen == 3) &&
                       (str[0] == 'o') && (str[1] == 'f') && (str[2] == 'f')) {
                make_inference();
                audio_sample_main.samples = 0;
            }
        }
    } else {
        fprintf(stderr, "%s: off-topic: %s\n", __func__, msg->topic);
    }
}

static void make_inference(void)
{
    int ret;
    whisper_full_params wparams =
        whisper_full_default_params(WHISPER_SAMPLING_GREEDY);

    wparams.print_progress   = false;
    wparams.print_special    = false;
    wparams.print_realtime   = false;
    wparams.print_timestamps = false;
    wparams.translate        = false;
    wparams.single_segment   = false;
    wparams.max_tokens       = 32;
    wparams.language         = "en";
    wparams.n_threads        = 4;
    wparams.audio_ctx        = 0;
    wparams.speed_up         = false;

    printf("make inference with %u samples\n", audio_sample_main.samples);

    ret = whisper_full(whisper, wparams,
                       audio_sample_main.data,
                       audio_sample_main.samples);
    if (ret != 0) {
        fprintf(stderr, "failed to process audio (%d)!\n", ret);
    }

    const int n_segments = whisper_full_n_segments(whisper);
    for (int i = 0; i < n_segments; ++i) {
        const char *text = whisper_full_get_segment_text(whisper, i);

        printf("%s", text);
        fflush(stdout);
    }
    printf("\n");
}

static void cleanup(void)
{
    int ret;

    if (whisper) {
        whisper_free(whisper);
        whisper = NULL;
    }

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

static const char *model_candidates[] = {
    "models/ggml-base.en.bin",
    "3rdparty/whisper.cpp/models/ggml-base.en.bin",
    "/usr/local/share/models/ggml-base.en.bin",
    NULL,
};

int main(int argc, char **argv)
{
    int ret;
    unsigned int i;

    (void)(argc);
    (void)(argv);

    atexit(cleanup);
    signal(SIGINT, sig_handler);

    for (i = 0; model_candidates[i] != NULL; i++) {
        if (filesystem::exists(model_candidates[i])) {
            whisper = whisper_init_from_file(model_candidates[i]);
            break;
        }
    }

    if (whisper == NULL || model_candidates[i] == NULL) {
        fprintf(stderr, "Failed to locate and load a model!\n");
        exit(EXIT_FAILURE);
    }

    ret = mosquitto_lib_init();
    if (ret != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "mosquitto_lib_init failed (%d)!\n", ret);
        exit(EXIT_FAILURE);
    }

    mosq = mosquitto_new("Rabbit Voice Recognition using whisper.cpp",
                         true, NULL);
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

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
