#ifndef LIBBSP_AUX_H
#define LIBBSP_AUX_H

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define TRACEK(fmt, ...) printk("%s: %s(): " fmt "\r", __FILENAME__, __FUNCTION__, ##__VA_ARGS__)

#ifdef DEBUG
#	define DTRACEK(fmt, ...) printk("%s: %s(): " fmt "\r", __FILENAME__, __FUNCTION__, ##__VA_ARGS__)
#else
#	define DTRACEK(fmt, ...)
#endif

#define uchar_t unsigned char
#define u8  uchar_t

#endif
