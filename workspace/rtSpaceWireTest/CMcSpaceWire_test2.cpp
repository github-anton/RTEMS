/*
 * CSpaceWire_test.cpp
 *
 *  Created on: 28 нояб. 2017 г.
 *      Author: anton
 */

#if 1
#   define DEBUG
#endif

#include "CMcSpaceWire.h"
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include "aux.h"

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
#	define ENABLE_SNOOZE
#endif

#if 0
#	define FILL_WRITER_PACKET
#endif

#define WRITER_DEV_PATH		"/dev/spw0"
#define READER_DEV_PATH		"/dev/spw1"

CMcSpaceWire *pReader = NULL ;
CMcSpaceWire *pWriter = NULL ;
pthread_t readerThread ;
pthread_t writerThread ;

void *readerProc(void *arg) ;
void *writerProc(void *arg) ;
void hexdump ( u_char buf[], size_t len ) ;

int main(void) {
    
#ifdef ENABLE_WRITER
	pWriter = new CMcSpaceWire(WRITER_DEV_PATH, 0) ;
    if (!pWriter->isOk()) {
        return -1 ;
    }
    size_t outDataLen = pWriter->getMaxCargoLen() ;
    //size_t outDataLen = 16 ;
	u_char *outData = new u_char[outDataLen] ;
#endif
    
#ifdef ENABLE_READER
	pReader = new CMcSpaceWire(READER_DEV_PATH, 0) ;
    if (!pReader->isOk()) {
        return -1 ;
    }
    size_t inDataLen = pReader->getMaxCargoLen() ;
	u_char *inData = new u_char[inDataLen] ;
#endif    

    
    while(true) {
#ifdef ENABLE_WRITER
        bool connected = pWriter->isConnected() ;
#else
        bool connected = pReader->isConnected() ;
#endif
		if (connected) {
			DTRACE("CONNECTED\n") ;
#ifdef ENABLE_WRITER            
            DTRACE("-> Sending a packet size=%i...\n", outDataLen) ;
			if ( pWriter->sendTo(NULL, outData, outDataLen) == -1 ) {
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
			DTRACE("<- Receiving packet...\n") ;
            int nBytes = pReader->recv(inData, inDataLen) ;
            if(nBytes == -1) {
                DTRACE("<- Receiving FAIL, recv() returned -1\n");
            } else {
                DTRACE("<- Receiving is OK, len=%i\n", nBytes) ;
                DTRACE("Received first 16 bytes:\n") ;
#ifdef DEBUG
                hexdump(inData, 16) ;
#endif
            }
#endif  // ENABLE_READER
#ifdef ENABLE_SNOOZE
            //SNOOZE(2000) ;
            sleep(1) ;
#endif
		} else {
			DTRACE("DISCONNECTED\n") ;
            //SNOOZE(1000) ;
            sleep(1) ;
		}
    }
    
	return 0 ;
}

void hexdump ( u_char buf[], size_t len ) {
	for ( size_t i = 0; i < len; i++ ) {
		printf("0x%X ", buf[i]) ;
	}
	printf("\n") ;
}
