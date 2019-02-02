/*
 * CSpaceWire_test.cpp
 *
 *  Created on: 28 нояб. 2017 г.
 *      Author: anton
 */

#if 1
#define DEBUG
#endif

#include "CMcSpaceWire.h"
#include "ThreadedSpaceWireRw.h"
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include "aux.h"

#define WRITER_DEV_PATH		"/dev/spw0"
#define READER_DEV_PATH		"/dev/spw1"

CSpaceWire *pReader = NULL ;
CSpaceWire *pWriter = NULL ;
pthread_t readerThread ;
pthread_t writerThread ;
pthread_t replyCheckerThread ;
pthread_t timeCodeReceiverThread ;

u_char readerAddr[] = {39} ;
u_char writerAddr[] = {33} ;

int main(void) {

	printf("***Multicore SapceWire test***\n") ;

#ifdef ENABLE_READER
	pReader = new CMcSpaceWire(READER_DEV_PATH, 4096, 1) ;
#endif
#ifdef ENABLE_WRITER
	pWriter = new CMcSpaceWire(WRITER_DEV_PATH, 4096, 1) ;
#endif

#ifdef ENABLE_READER
	pthread_create(&readerThread, NULL, readerProc, NULL) ;
    pthread_create(&timeCodeReceiverThread, NULL, timeCodeReceiverProc, NULL) ;
#endif
#if defined(RECEIVE_REPLY_ASYNC) && defined(REPLY_ON_PACKET)
	pthread_create(&replyCheckerThread, NULL, replyCheckerProc, NULL) ;
#endif
#ifdef ENABLE_WRITER
	pthread_create(&writerThread, NULL, writerProc, NULL) ;
#endif

#ifdef ENABLE_READER
	pthread_join(readerThread, NULL) ;
#endif
#ifdef ENABLE_WRITER
	pthread_join(writerThread, NULL) ;
#endif

	return 0 ;
}
