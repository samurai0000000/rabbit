/*
 * logging.cxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#include <iostream>
#include "websock.hxx"

using namespace std;

void logging_init(void)
{

}

void logging_cleanup(void)
{

}

void logging_log(const char *message)
{
    cout << message;
    websock_broadcast(message);
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

