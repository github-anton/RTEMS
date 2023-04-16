#ifndef LIBBSP_AUXIO_H
#define LIBBSP_AUXIO_H

/*
 * auxio.h - auxilary Input/Output routines for debug purpose.
 * 
 * Copyright (C) 2019 by Anton Ermakov
 */

#include <rtems/bspIo.h>
#include <string.h>

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define TRACEK(fmt, ...) printk("%s: %s(): " fmt, __FILENAME__, __FUNCTION__, ##__VA_ARGS__)

#ifdef DEBUG
#	define DTRACEK(fmt, ...) printk("%s: %s(): " fmt , __FILENAME__, __FUNCTION__, ##__VA_ARGS__)
#   define DPRINTK(fmt, ...) printk( fmt , ##__VA_ARGS__)
#else
#	define DTRACEK(fmt, ...)
#   define DPRINTK(fmt, ...)
#endif

#define uchar_t unsigned char
#define u8  uchar_t

#endif
