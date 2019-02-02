/*
<<<<<<< HEAD
 * stdaux.h
 *
 *  Created on: 31 янв. 2018 г.
 *      Author: anton
 */

#ifndef SRC_AUX_H_
#define SRC_AUX_H_
=======
 * aux.h
 *
 *  Created on: 25 дек. 2017 г.
 *      Author: anton
 */

#ifndef AUX_H_
#define AUX_H_
>>>>>>> 212e3b0a2a9fecda746bc7803e945f3a46a1a98a

#include <time.h>
#include <string.h>

<<<<<<< HEAD
#define MIN(a, b) ((a) < (b)? (a): (b))

#define SNOOZE(msec)  { \
	struct timespec ts = {.tv_sec = msec/1000, .tv_nsec = (msec % 1000) * 1000000} ; \
=======
#define MIN(a,b) ( a < b ? a: b )

#define SNOOZE(msec)  { \
	struct timespec ts = {.tv_sec = 0, .tv_nsec = msec * 1000000} ; \
>>>>>>> 212e3b0a2a9fecda746bc7803e945f3a46a1a98a
	nanosleep(&ts, NULL) ; \
}

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define TRACE(fmt, ...) printf("%s: %s(): " fmt, __FILENAME__, __FUNCTION__, ##__VA_ARGS__); \
	fflush(stdout)

#ifdef DEBUG
#	define DTRACE(fmt, ...) printf("%s: %s(): " fmt, __FILENAME__, __FUNCTION__, ##__VA_ARGS__); \
	fflush(stdout)
<<<<<<< HEAD
#	define DPRINTF(fmt, ...) printf(fmt, ##__VA_ARGS__); \
=======
#	define DPRINTF(fmt, ...) printf( fmt, ##__VA_ARGS__); \
>>>>>>> 212e3b0a2a9fecda746bc7803e945f3a46a1a98a
	fflush(stdout)
#else
#	define DTRACE(fmt, ...)
#	define DPRINTF(fmt, ...)
#endif

<<<<<<< HEAD
#endif /* SRC_AUX_H_ */
=======
#endif /* AUX_H_ */
>>>>>>> 212e3b0a2a9fecda746bc7803e945f3a46a1a98a
