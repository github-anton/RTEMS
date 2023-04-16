#ifndef AUXTIMER_H_
#define AUXTIMER_H

#include <time.h>

#define SNOOZE(msec)  { \
	struct timespec ts = {.tv_sec = msec/1000, .tv_nsec = (msec % 1000) * 1000000} ; \
	nanosleep(&ts, NULL) ; \
} 

#endif
