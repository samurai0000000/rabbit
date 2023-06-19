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

static bool attention = false;

static int match_keywords(const char *text, const char *argv[])
{
    unsigned int i;

    if (argv == NULL) {
        return 0;
    }

    for (i = 0; argv[i] != NULL; i++) {
        if (strcasestr(text, argv[i]) == NULL) {
            return 0;
        }
    }

    return 1;
}

static void do_hear(const struct mosquitto_message *msg)
{
    const char *text = (const char *) msg->payload;
    static const char *ok_robot[] = { "ok", "robot", NULL, };
    static const char *ok2_robot[] = { "okay", "robot", NULL, };
    static const char *give_hug[] = { "give", "hug", NULL, };
    static const char *raise_arms[] = { "raise", "arms", NULL, };
    static const char *rest_arms[] = { "rest", "arms", NULL, };
    static const char *stretch_right_arm[] = {
        "stretch", "right", "arm", NULL, };
    static const char *stretch_left_arm[] = {
        "stretch", "left", "arm", NULL, };
    static const char *hello[] = { "hello", NULL, };
    static const char *nice_see_you[] = { "nice", "see", "you", NULL, };
    static const char *xfer_right_arm[] = {
        "transfer", "right", "arm", NULL, };
    static const char *xfer_left_arm[] = {
        "transfer", "left", "arm", NULL, };
    static const char *pick_right_arm[] = { "pick", "right", "arm", NULL, };
    static const char *pick_left_arm[] = { "pick", "left", "arm", NULL, };
    static const char *whoareyou[] = { "who", "are", "you", NULL, };
    static const char *emotions[] = { "what", "emotions", NULL, };
    static const char *earsup[] = { "ears", "up", NULL, };
    static const char *earsback[] = { "ears", "back", NULL, };
    static const char *earsdown[] = { "ears", "down", NULL, };

    if (strchr(text, ' ') == NULL) {
        return;  // Less than two words
    }

    head->earsUp();

    if (attention == false) {
        if (match_keywords(text, ok_robot) ||
            match_keywords(text, ok2_robot)) {
            mouth->cylon();
            speech->speak("yes");
            attention = true;
        }
    } else {
        if (match_keywords(text, ok_robot)) {
            mouth->cylon();
            speech->speak("yes");
            attention = true;
        } else if (match_keywords(text, give_hug)) {
            rabbit_keycontrol('y');
        } else if (match_keywords(text, raise_arms)) {
            rabbit_keycontrol('z');
        } else if (match_keywords(text, rest_arms)) {
            speech->speak("ok");
            rabbit_keycontrol('x');
        } else if (match_keywords(text, stretch_right_arm)) {
            rabbit_keycontrol('u');
        } else if (match_keywords(text, stretch_left_arm)) {
            rabbit_keycontrol('U');
        } else if (match_keywords(text, hello)) {
            rabbit_keycontrol('i');
        } else if (match_keywords(text, nice_see_you)) {
            rabbit_keycontrol('I');
        } else if (match_keywords(text, xfer_right_arm)) {
            rabbit_keycontrol('k');
        } else if (match_keywords(text, xfer_left_arm)) {
            rabbit_keycontrol('K');
        } else if (match_keywords(text, pick_right_arm)) {
            rabbit_keycontrol('a');
        } else if (match_keywords(text, pick_left_arm)) {
            rabbit_keycontrol('A');
        } else if (match_keywords(text, whoareyou)) {
            rabbit_keycontrol('t');
        } else if (match_keywords(text, emotions)) {
            speech->speak("I'm relaxed");
            mouth->beh();
            head->eyebrowSetDisposition(Head::EB_RELAXED);
            sleep(2);
            speech->speak("perplexed");
            mouth->beh();
            head->eyebrowSetDisposition(Head::EB_PERPLEXED);
            sleep(2);
            speech->speak("surprised");
            mouth->beh();
            head->eyebrowSetDisposition(Head::EB_SURPRISED);
            sleep(2);
            speech->speak("happy");
            mouth->smile();
            head->eyebrowSetDisposition(Head::EB_HAPPY);
            sleep(2);
            speech->speak("jubilant");
            mouth->smile();
            head->eyebrowSetDisposition(Head::EB_JUBILANT);
            sleep(2);
            speech->speak("angry");
            mouth->beh();
            head->eyebrowSetDisposition(Head::EB_ANGRY);
            sleep(2);
            speech->speak("furiuous");
            mouth->beh();
            head->eyebrowSetDisposition(Head::EB_FURIOUS);
            sleep(2);
            speech->speak("sad");
            mouth->beh();
            head->eyebrowSetDisposition(Head::EB_SAD);
            sleep(2);
            speech->speak("depressed");
            mouth->beh();
            head->eyebrowSetDisposition(Head::EB_DEPRESSED);
            sleep(2);
            head->eyebrowSetDisposition(Head::EB_RELAXED);
            mouth->cylon();
        } else if (match_keywords(text, earsup)) {
            speech->speak("I'm all ears");
            head->earsUp();
            head->eyebrowSetDisposition(Head::EB_RELAXED);
        } else if (match_keywords(text, earsback)) {
            speech->speak("Super cool");
            head->earsBack();
            head->eyebrowSetDisposition(Head::EB_JUBILANT);
        } else if (match_keywords(text, earsdown)) {
            speech->speak("yikes");
            head->earsDown();
            head->eyebrowSetDisposition(Head::EB_DEPRESSED);
        } else {
            mouth->beh();
            speech->speak("sorry!");
            attention = false;
        }
    }
}

static void do_speak(const struct mosquitto_message *msg)
{
    if (speech) {
        speech->speak((const char *) msg->payload);
    }
}

static mosq_sub_action mosq_sub_actions[] = {
    { "rabbit/voice/transcribed", do_hear, 1, },
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
    mosquitto_publish_callback_set(mosq, this->onPublish);
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

void Mosquitto::onPublish(struct mosquitto *mosq, void *obj, int mid)
{
    Mosquitto *mosquitto = (Mosquitto *) obj;

    (void)(mosq);
    (void)(obj);
    (void)(mid);

    //printf("Mosquitto::onPublish: %d\n", mid);
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
    if (mosq == NULL) {
        return -1;
    }

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
