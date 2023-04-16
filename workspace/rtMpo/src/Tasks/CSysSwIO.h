/*
 * CSwReceiver.h
 *
 *  Created on: Feb 19, 2019
 *      Author: Anton Ermakov
 */

#ifndef CSYSSWRECV_H_
#define CSYSSWRECV_H_

#include <pthread.h>
#include <SpaceWire/CSpaceWireRouter.h>
#include "../System/CARINCSched.h"
#include "../Tasks/CSysMpoMachine.h"
#include <CTask.h>
#include "../System/MMIProto.h"

class CSysSwIO: public CTask<CSysSwIO> {
	pthread_t	threadId ;
	static CSpaceWireRouter	*pSwr ;

	CRingBuffer	*pSwPipe ;
	CRingBuffer	**ppTaskSwPipe ;

	CARINCSched		*pSched ;

	static int ctlSymTable[0x11+1] ;

	u_int	linkedPort ;

	enum TStates {stInit=0, stWaitForLink=1, stLinked=2, stLinkError=3} ;
	TStates		linkState ;

	int writePacketForTask(int taskNo, void *pData, size_t len) ;

public:
	CSysSwIO(CARINCSched *pSched) ;
	virtual ~CSysSwIO() ;
	virtual int run(CARINCSched *pSched) ;
	static uint32_t getProtoId(void *mpoPacket) ;
	static uint32_t getMessageId(void *mpoPacket) ;
	static uint32_t getModeId(void *mpoPacket) ;
	int getMyPacket(void *pData, size_t len) ;
	int putMyPacket(void *pData, size_t len) ;
	int selectLinkedPort();
	int getLinkedPortsQty() ;
} ;

#endif /* CSYSSWRECV_H_ */
