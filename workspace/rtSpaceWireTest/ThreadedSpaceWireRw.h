#ifndef THREADED_SPACEWIRE_RW_H_
#define THREADED_SPACEWIRE_RW_H_

#if 1
#	define DEBUG
#endif

#include "CSpaceWire.h"
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

#if 1
#	define CHECK_RECEIVED_PACKET
#endif

#if 1
#	define ENABLE_WRITER_SNOOZE
#   define TH_WRITER_SNOOZE_TIME_MS    20
#endif

#if 1
#   define ENABLE_READER_DISCONNECTED_SNOOZE
#   define TH_READER_DISCONNECTED_SNOOZE_TIME_MS    2000
#endif

#if 1
#	define FILL_WRITER_PACKET
#endif

#if 1
#	define REPLY_ON_PACKET
#endif

#if 1
#	define WRITER_NEED_REPLY
#endif

#if 1
#	define RECEIVE_REPLY_SYNC
#else
#	define RECEIVE_REPLY_ASYNC
#endif

#endif

#define TH_MAX_PACKET_LEN	4095
#define WRITER_RATE			10
#define READER_REPLY_RATE   10

extern CSpaceWire *pReader ;
extern CSpaceWire *pWriter ;
extern u_char readerAddr[] ;
extern u_char writerAddr[] ;

void *readerProc(void *arg) ;
void *writerProc(void *arg) ;
void *replyCheckerProc(void *arg) ;
void *timeCodeReceiverProc(void *arg) ;

void hexdump ( u_char buf[], size_t len ) ;
bool checkPacket(u_char packetNo, u_char buf[], size_t len) ;

