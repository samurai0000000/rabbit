/*
 * websock.c
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ws.h>
#include "websock.h"

extern void rabbit_keycontrol(uint8_t key);

static void onopen(ws_cli_conn_t *client)
{
	char *cli;

	cli = ws_getaddress(client);
	fprintf(stderr, "websocket connected from %s\n", cli);
}

static void onclose(ws_cli_conn_t *client)
{
	char *cli;

    cli = ws_getaddress(client);
	fprintf(stderr, "websocket closed from %s\n", cli);
}

static void onmessage(ws_cli_conn_t *client,
                      const unsigned char *msg, uint64_t size, int type)
{
    (void)(client);
    (void)(type);

    while (size > 0) {
        rabbit_keycontrol(*msg);
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

