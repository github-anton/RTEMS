/*
 * CBuffer.h
 *
 *  Created on: May 27, 2020
 *      Author: anton
 */

#ifndef CBUFFER_H_
#define CBUFFER_H_

#include <sys/types.h>

class CBuffer {
	u_char *pArea ;
	size_t wIdx ;
	size_t size ;

	size_t calcIdx(int off) ;

public:
	CBuffer(size_t len);
	virtual ~CBuffer();
	virtual int add(void *part, size_t len) ;
	virtual int add(u_char byte) ;
	virtual int add(u_short word) ;
	virtual int add(u_int dword) ;
	virtual void clear() ;
	virtual u_char *getPtr(size_t off) ;
	virtual u_char getByte(size_t idx) ;
	virtual int read(size_t off, void *array, size_t len) ;
	virtual void printHexDump() ;
	virtual void printBinDump() ;
	virtual size_t getLastIdx() ;
	virtual size_t getFill() ;
	virtual size_t getSize() ;
	virtual size_t getAvail() ;
	virtual void trim(int idx) ;
};

#endif /* CBUFFER_H_ */
