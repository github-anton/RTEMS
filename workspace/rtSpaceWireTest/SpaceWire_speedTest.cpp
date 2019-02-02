/*
 * CSpaceWire_test.cpp
 *
 *  Created on: 28 нояб. 2017 г.
 *      Author: anton
 */

#if 0
#   define DEBUG
#endif

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "aux.h"
#include "SpaceWire_speedTest.h"

CSpaceWire *pReader = NULL ;
CSpaceWire *pWriter = NULL ;

#define PACKETS     756

void hexdump ( u_char buf[], size_t len ) ;

u_char inData[4096] ;
u_char outData[4096] ;

int main(void) {

#ifdef ENABLE_WRITER    
	pWriter = new SPACEWIRE_WRITER_CONSTRUCTOR ;
    if (!pWriter->isOk()) {
        TRACE("Can't create Writer.\n") ;
        return -1 ;
    }
#endif
#ifdef ENABLE_READER    
	pReader = new SPACEWIRE_READER_CONSTRUCTOR ;
    if (!pReader->isOk()) {
        TRACE("Can't create Reader.\n") ;
        return -1 ;
    }
#endif
    TRACE("Waiting for connection...\n") ;
    bool writerConnected = true ;
    bool readerConnected = true ;
    do {
        SNOOZE(1000) ;
#ifdef ENABLE_WRITER
        readerConnected = pWriter->isConnected() ;
#endif
#ifdef ENABLE_READER
        writerConnected = pReader->isConnected() ;
#endif
    } while (!readerConnected || !writerConnected) ;
    TRACE("CONNECTED\n") ;
    
#ifdef ENABLE_WRITER
    TRACE("Setting up speed to %iMbit/s\n", WRITER_RATE) ;
    int stat = pWriter->setTxSpeed(WRITER_RATE) ;
    TRACE("setTxSpeed() returned %i\n", stat) ;
    pWriter->sendTo(NULL, outData, sizeof(outData)) ;
#endif
    
    TRACE("Calculating speed...\n") ;
    bool firstPacket = true ;
    struct timespec tstart, tstop ;
    clock_gettime(CLOCK_REALTIME, &tstart) ;
    for (int i = 0; i < PACKETS ; i++) {
        DTRACE("PacketNo=%i\n", i) ;
#ifdef ENABLE_WRITER       
        DTRACE("-> Sending a packet size=%i...\n", sizeof(outData)) ;
		if ( pWriter->sendTo(NULL, outData, sizeof(outData)) == -1 ) {
            DTRACE("-> Sending FAIL, write() returned -1\n") ;
        } else {
            DTRACE("-> Sending is OK\n") ;
            DTRACE("First sent 16 bytes:\n") ;
#ifdef DEBUG
            hexdump(outData, 16) ;
#endif
        }
#endif // ENABLE_WRITER

#ifdef ENABLE_READER
		int nBytes ;
		if (firstPacket) {
            DTRACE("<- Receiving first packet...\n") ;
            nBytes = pReader->recv(inData, sizeof(inData)) ;
            clock_gettime(CLOCK_REALTIME, &tstart) ;
            firstPacket = false ;
        }
		
		DTRACE("<- Receiving packet...\n") ;
        if (!firstPacket) {
            nBytes = pReader->recv(inData, sizeof(inData)) ;
        }
        if(nBytes == -1) {
            DTRACE("<- Receiving FAIL, recv() returned -1\n");
        } else {
            DTRACE("<- Receiving is OK, len=%i\n", nBytes) ;
            DTRACE("Received first 16 bytes:\n") ;
#ifdef DEBUG
            hexdump(inData, 16) ;
#endif
        }
#endif // ENABLE_READER
    }
    clock_gettime(CLOCK_REALTIME, &tstop) ;
    
    if((tstop.tv_nsec == tstart.tv_nsec) && (tstop.tv_sec == tstart.tv_sec) ) {
        TRACE ("Too few packets: deltaTime=0 ms.\n") ;
        return -1 ;
    }
    
	float delta = (tstop.tv_nsec - tstart.tv_nsec) / 1000000000.0 + (tstop.tv_sec - tstart.tv_sec) ;
	TRACE("%.2f Kb/s\n", 4 * (PACKETS - 1) / delta) ;
#ifdef ENABLE_READER
    TRACE("getRxSpeed(): rxSpeed=%i Mbit/s\n", pReader->getRxSpeed()) ;
#endif

	return 0 ;
}

void hexdump ( u_char buf[], size_t len ) {
	for ( size_t i = 0; i < len; i++ ) {
		printf("0x%X ", buf[i]) ;
	}
	printf("\n") ;
}
