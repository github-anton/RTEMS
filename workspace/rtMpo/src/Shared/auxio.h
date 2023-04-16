/*
 * auxio.h
 *
 *  Created on: Jan 23, 2020
 *      Author: anton
 */

#ifndef AUXIO_H_
#define AUXIO_H_

#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#ifndef __FILENAME__
	#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#ifndef TRACE
	#define TRACE(fmt, ...) printf("%s: %s(): " fmt, __FILENAME__, __FUNCTION__, ##__VA_ARGS__); \
	fflush(stdout)
#endif

#ifdef DEBUG

	#undef DTRACE
	#define DTRACE(fmt, ...) printf("%s: %s(): " fmt, __FILENAME__, __FUNCTION__, ##__VA_ARGS__); \
	fflush(stdout)

#undef DPRINTF

	#define DPRINTF(fmt, ...) printf(fmt, ##__VA_ARGS__); \
	fflush(stdout)

#else

	#undef DTRACE
	#define DTRACE(fmt, ...)

	#undef DPRINTF
	#define DPRINTF(fmt, ...)

#endif

#ifdef VERBOSE
	#define VPRINTF		printf
#else
	#define VPRINTF(fmt, ...)
#endif

#if defined(DEBUG) || defined(VERBOSE)
	#define DVPRINTF	printf
#else
	#define DVPRINTF(fmt)
#endif

#ifdef __cplusplus
	extern "C" {
#endif

void print_hex_dump(void *array, size_t len) ;
void print_bin_dump(void *array, size_t len) ;

#define ARRLEN(a)			(sizeof(a)/sizeof(a[0]))

#ifdef __cplusplus
	}
#endif

#endif /* AUXIO_H_ */
