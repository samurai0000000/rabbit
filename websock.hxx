/*
 * websock.hxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#ifndef WEBSOCK_HXX
#define WEBSOCK_HXX

extern void websock_init(void);
extern void websock_cleanup(void);
extern void websock_broadcast(const char *message);

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
