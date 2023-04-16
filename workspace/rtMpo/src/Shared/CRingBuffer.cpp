
/*
 * CRingBuf.cpp
 *
 *  Created on: 30 марта 2017 г.
 *      Author: anton
 */
#if 0
	#define DEBUG
#endif

#if 1
	#define USE_POSIX_MUTEXES
#endif

#include "CRingBuffer.h"
#include "auxmath.h"
#include "auxio.h"
#include <stdlib.h>
#include <pthread.h>

CRingBuffer::CRingBuffer(size_t size) {
	pChunk = (char*)malloc(size) ;
	BIdx = 0 ;
	Fill = 0 ;
	Size = size ;
	block = false ;

#if defined(USE_POSIX_MUTEXES)
	int err ;
	pthread_mutexattr_t attr ;

	pthread_mutexattr_init(&attr) ;
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL) ;

	err = pthread_mutex_init(&eof, &attr) ;
	if ( err != 0) {
		TRACE("Can't initialize eof mutex, return=%i.\n", err) ;
	}
	DTRACE("Lock eof=0x%X mutex.\n", &eof) ;
	err = pthread_mutex_lock(&eof) ;
	if( err != 0) {
		TRACE("Can't lock eof mutex, return=%i.\n", err) ;
	}
#endif
}

CRingBuffer::~CRingBuffer() {
	BIdx = 0 ;
	Fill = 0 ;
	free(pChunk) ;
}

int CRingBuffer::put(void *data, size_t len) {
	char *pData = (char*)data ;

	DTRACE("pData=0x%x, pData[0]=%i, len=%i\n\r", pData, pData[0], len) ;

	int i ;
	// Write into the buffer a data while there is a free space
	// in it
	for ( i = 0; 0 < MIN(len - i, Size - Fill); i++) {
		DTRACE("write to 0x%x <- %i\n", &pChunk[(BIdx + Fill) % Size], pData[i]) ;
		pChunk[(BIdx + Fill++) % Size] = pData[i] ;
		//Fill = Fill % (Size + 1) ;
	}

	DTRACE("Size=%i, Fill=%i\n", Size, Fill) ;

	// Override oldest data with new one if
	// "Override" mode is selected.
	if (override && ( i < len ) && ( Fill == Size )) {
		for (; i < len; i++) {
			pChunk[(BIdx++ + Fill) % Size] = pData[i] ;
		}
	}

	if(block && Fill) {
		DTRACE("Unlock eof=0x%X mutex.\n", &eof) ;
		pthread_mutex_unlock(&eof) ;
	}

	return i ;
}

int CRingBuffer::get(void *data, size_t len) {
	char *pData = (char*)data ;
	int i ;
	int err ;

	if( !Fill && block ) {
		DTRACE("Lock eof=0x%X mutex.\n", &eof) ;
		err = pthread_mutex_lock(&eof) ;
		if( err != 0) {
			TRACE("Can't lock eof mutex, return=%i.\n", err) ;
		}
	}

	if (!Fill) return -1 ;	// Return -1 if there is nothing in ring buffer
							// and we did't blocked

	for(i = 0; (i < (int)len) && Fill ; i++) {
		DTRACE("Read from 0x%x -> %i\n", &pChunk[(BIdx) % Size], pChunk[(BIdx) % Size]) ;
		pData[i] = pChunk[(BIdx++) % Size] ;
		Fill -- ;
	}

	DTRACE("RETURN %i\n", i) ;
	return i ;
}

int CRingBuffer::read(int offset, void *data, size_t len) {
	char *pData = (char*)data ;
	int i ;
	for(i = 0; (i < (int)len) && Fill ; i++) {
		pData[i] = pChunk[(BIdx+offset+i) % Size] ;
	}
	return i ;
}

size_t CRingBuffer::getSize() {
	return Size ;
}

size_t CRingBuffer::getFill() {
	return Fill ;
}

size_t CRingBuffer::getAvailSize() {
	return	(Size - Fill) ;
}

void CRingBuffer::setBlock(bool yes) {
	block = yes ;
}


void CRingBuffer::clear() {
	Fill = 0 ;
}

void CRingBuffer::setOverride(bool yes) {

	override = yes ;
}
