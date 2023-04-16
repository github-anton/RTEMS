/*
 * CRingBuf.h
 *
 *  Created on: 30 марта 2017 г.
 *      Author: Anton Ermakov
 */

#ifndef CRINGBUF_H_
#define CRINGBUF_H_

#include <sys/types.h>

class CRingBuffer {

private:
	char *pChunk ;
	int BIdx ;
	size_t Fill ;
	size_t Size ;
	pthread_mutex_t eof ;
	bool block ;
	bool override ;

public:
	CRingBuffer(size_t size);
	virtual ~CRingBuffer();
	int put(void *data, size_t len) ;
	int get(void *data, size_t len) ;
	int read(int offset, void *data, size_t len) ;
	size_t getSize() ;
	size_t getFill() ;
	size_t getAvailSize() ;
	void setBlock(bool yes) ;
	void setOverride(bool yes) ;
	void clear() ;
};

#endif /* CRINGBUF_H_ */
