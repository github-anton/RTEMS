/*
 * stdaux.h
 *
 *  Created on: 31 янв. 2018 г.
 *      Author: anton
 */

#ifndef SRC_AUX_H_
#define SRC_AUX_H_

#include <time.h>
#include <string.h>

#define MIN(a, b) ((a) < (b)? (a): (b))

#define SNOOZE(msec)  { \
	struct timespec ts = {.tv_sec = msec/1000, .tv_nsec = (msec % 1000) * 1000000} ; \
	nanosleep(&ts, NULL) ; \
}

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define TRACE(fmt, ...) printf("%s: %s(): " fmt, __FILENAME__, __FUNCTION__, ##__VA_ARGS__); \
	fflush(stdout)

#ifdef DEBUG
#	define DTRACE(fmt, ...) printf("%s: %s(): " fmt, __FILENAME__, __FUNCTION__, ##__VA_ARGS__); \
	fflush(stdout)
#	define DPRINTF(fmt, ...) printf(fmt, ##__VA_ARGS__); \
	fflush(stdout)
#else
#	define DTRACE(fmt, ...)
#	define DPRINTF(fmt, ...)
#endif

#endif /* SRC_AUX_H_ */
