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
<<<<<<< HEAD
#include "ThreadedSpaceWireRw.h"
=======
>>>>>>> 212e3b0a2a9fecda746bc7803e945f3a46a1a98a
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include "aux.h"

<<<<<<< HEAD
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
=======
#if 1
#	define ENABLE_WRITER
#endif

#if 1
#	define ENABLE_READER
#endif

#if 0
#	define CHECK_RECEIVED_PACKET
#endif

#if 1
#	define ENABLE_WRITER_SNOOZE
#endif

#if 0
#	define FILL_WRITER_PACKET
#endif

#define READER_DEV_PATH		"/dev/spw1"
#define WRITER_DEV_PATH		"/dev/spw0"

CMcSpaceWire *pReader = NULL ;
CMcSpaceWire *pWriter = NULL ;
pthread_t readerThread ;
pthread_t writerThread ;

void *readerProc(void *arg) ;
void *writerProc(void *arg) ;
void hexdump ( u_char buf[], size_t len ) ;
bool checkPacket(u_char packetNo, u_char buf[], size_t len) ;

u_char packetNo = 0;
>>>>>>> 212e3b0a2a9fecda746bc7803e945f3a46a1a98a

int main(void) {

	printf("***Multicore SapceWire test***\n") ;

#ifdef ENABLE_READER
<<<<<<< HEAD
	pReader = new CMcSpaceWire(READER_DEV_PATH, 4096, 1) ;
#endif
#ifdef ENABLE_WRITER
	pWriter = new CMcSpaceWire(WRITER_DEV_PATH, 4096, 1) ;
=======
	pReader = new CMcSpaceWire(READER_DEV_PATH, 0) ;
#endif
#ifdef ENABLE_WRITER
	pWriter = new CMcSpaceWire(WRITER_DEV_PATH, 0) ;
>>>>>>> 212e3b0a2a9fecda746bc7803e945f3a46a1a98a
#endif

#ifdef ENABLE_READER
	pthread_create(&readerThread, NULL, readerProc, NULL) ;
<<<<<<< HEAD
    pthread_create(&timeCodeReceiverThread, NULL, timeCodeReceiverProc, NULL) ;
#endif
#if defined(RECEIVE_REPLY_ASYNC) && defined(REPLY_ON_PACKET)
	pthread_create(&replyCheckerThread, NULL, replyCheckerProc, NULL) ;
=======
>>>>>>> 212e3b0a2a9fecda746bc7803e945f3a46a1a98a
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
<<<<<<< HEAD
=======

void *readerProc(void *arg) {
	u_char data[pReader->getMaxCargoLen()] ;
	size_t wrongPackets = 0 ;

	TRACE("Start reading...\n") ;

	// Just print the received data...
	while(true) {
#ifndef ENABLE_WRITER
		bool connected = pReader->isConnected() ;
		if (connected) {
			DTRACE("CONNECTED\n") ;
		} else {
			DTRACE("DISCONNECTED\n") ;
		}
#endif
		int nBytes = pReader->recv(data, sizeof(data)) ;
		DTRACE("<- Received a packet size=%i, firts 16 bytes:\n", nBytes) ;
		if (nBytes > 0) {
			// Dump first 16 bytes
#ifdef DEBUG
			hexdump(data, 16) ;
#endif
#ifdef CHECK_RECEIVED_PACKET
			if ( checkPacket(packetNo, data, sizeof(data)) ) {	// OK
				DPRINTF("OK\n") ;
			} else {	// WRONG
				DPRINTF("WRONG\n") ;
				wrongPackets ++ ;
			}
#endif
		}
		DTRACE("wrongPackets=%i\n", wrongPackets) ;
	}
	return NULL ;
}

void *writerProc(void *arg) {
	size_t cargoLen = pWriter->getMaxCargoLen() ;
	u_char *data = new u_char[cargoLen] ;
	u_char destAddr[] = {32, 0, 0, 0} ;
	size_t frameNo = 0;

	TRACE("Start writing...\n") ;
	while (true) {
		DTRACE("Frame %i\n", frameNo++) ;

#ifdef FILL_WRITER_PACKET
		// Fill the data buffer
		data[0] = packetNo ;
		for (size_t i = 1; i < cargoLen; i++) {
			data[i] = data[i-1] +1 ;
		}
#endif
		bool connected = pWriter->isConnected() ;

		if ( connected ) {
			// If the connection is established print status and send
			// some data...

			DTRACE ("CONNECTED\n") ;

			DTRACE("-> Send a packet no=%i size=%i, first 16 bytes:\n", packetNo, cargoLen) ;
			hexdump(data, 16) ;
			if ( pWriter->sendTo(destAddr, data, cargoLen) == -1 ) {
				fprintf(stderr, "writerProc(): Sending ERROR, write() returned -1\n") ;
			}
		} else {
			// If disconnected print that.
			DTRACE ("DISCONNECTED\n") ;
			// Send only address to establish a connection
			pWriter->sendTo(destAddr, NULL, 0) ;
		}

#ifdef ENABLE_WRITER_SNOOZE
		// Wait a while
		//SNOOZE(900) ;
		sleep(1) ;
#endif

		packetNo ++ ;
	}
	return NULL ;
}

void hexdump ( u_char buf[], size_t len ) {
	for ( size_t i = 0; i < len; i++ ) {
		printf("0x%X ", buf[i]) ;
	}
	printf("\n") ;
}

bool checkPacket(u_char packetNo, u_char buf[], size_t len) {
	if ( buf[0] != packetNo ) {
		DTRACE("Wrong packet no=%i\n", buf[0]) ;
		return false ;
	}

	for (size_t i = 0; i < len -1 ; i++) {
		if ( (u_char)(buf[i] + 1) != buf[i+1]) {
			DTRACE("Sequence missmatch: buf[%i]=%i, buf[%i+1]=%i\n", i, buf[i], i, buf[i+1]) ;
			return false ;
		}
	}
	return true ;
}
>>>>>>> 212e3b0a2a9fecda746bc7803e945f3a46a1a98a
