/*
 * websock.cxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <vector>
#include <ws.h>
#include "rabbit.hxx"

using namespace std;

extern "C" void rabbit_keycontrol(uint8_t key);

vector<ws_cli_conn_t *> G_websocks;

static void onopen(ws_cli_conn_t *client)
{
	char *cli;
    char buf[64];

    G_websocks.push_back(client);

	cli = ws_getaddress(client);
	fprintf(stderr, "websocket connected from %s\n", cli);
    snprintf(buf, sizeof(buf) - 1,
             "%s has connected to the rabbit'bot, welcome!\n", cli);
    speech->speak("A web client just connected and joined");
    websock_broadcast(buf);
}

static void onclose(ws_cli_conn_t *client)
{
	char *cli;
    vector<ws_cli_conn_t *>::iterator it;
    char buf[64];

    cli = ws_getaddress(client);
	fprintf(stderr, "websocket closed from %s\n", cli);

    for (it = G_websocks.begin(); it != G_websocks.end(); it++) {
        if (*it == client) {
            it = G_websocks.erase(it);
            break;
        }
    }

    speech->speak("A web client has left the session");
    snprintf(buf, sizeof(buf) - 1,
             "%s has disconnected from the rabbit'bot, bye!\n", cli);
    websock_broadcast(buf);

}

static void onmessage(ws_cli_conn_t *client,
                      const unsigned char *msg, uint64_t size, int type)
{
    (void)(client);
    (void)(type);

    while (size > 0) {
        if (*msg != 'q' && *msg != 'Q') {
            rabbit_keycontrol(*msg);
        }
        msg++;
        size--;
    }
}

void websock_init(void)
{
	struct ws_events evs;

	evs.onopen    = &onopen;
	evs.onclose   = &onclose;
	evs.onmessage = &onmessage;

	ws_socket(&evs, 18888, 1, 1000);
}

void websock_cleanup(void)
{
    G_websocks.clear();
}

void websock_broadcast(const char *message)
{
    vector<ws_cli_conn_t *>::iterator it;

    for (it = G_websocks.begin(); it != G_websocks.end(); it++) {
        ws_sendframe(*it, message, strlen(message), WS_FR_OP_TXT);
    }
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

