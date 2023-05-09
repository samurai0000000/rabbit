/*
 * logging.hxx
 *
 * Copyright (C) 2023, Charles Chiou
 */

#ifndef LOGGING_HXX
#define LOGGING_HXX

extern void logging_init(void);
extern void logging_cleanup(void);
extern void logging_log(const char *message);
#define LOG logging_log

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

