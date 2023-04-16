
/*
 * CSwReceiver.cpp
 *
 *  Created on: Feb 19, 2019
 *      Author: Anton Ermakov
 */
#if 0
	#define DEBUG
#endif

#if 1
	#define VERBOSE
#endif

#if 0
	#define DUMP
#endif

#include "CSysSwIO.h"
#include <SpaceWire/CMcSpaceWireRouter.h>
#include <auxio.h>
#include <errno.h>
#include <stdlib.h>
#include "../Tasks.h"

CSpaceWireRouter *CSysSwIO::pSwr = 0 ;

CSysSwIO::CSysSwIO(CARINCSched *pSched) {
	DTRACE("BEGIN\n") ;
	if (!pSwr) {
		pSwr = new CMcSpaceWireRouter("/dev/spw0", SW_MAX_MMI_PACKET_LEN, 1) ;
		pSwr->setNonBlock(true) ;
		pSwr->setTxSpeed(10) ;
		linkedPort = 0 ;
		u_int	linkedPortsQty = 0 ;
	}

	int taskRows = pSched->getTaskDescRows() ;

	// Create SW queue from tasks to MZU
	pSwPipe = new CRingBuffer((sizeof(size_t) + SW_MAX_OUT_PACKET_LEN) * 16) ;
	DTRACE("Pipe-len = %i bytes\n", pSwPipe->getSize()) ;

	// Create SW queue from MZU to tasks
	ppTaskSwPipe = new CRingBuffer*[taskRows];
	DTRACE("Array-addr pipes of tasks = 0x%x\n", (unsigned)ppTaskSwPipe) ;

	int i ;
	for (i = 0; i < taskRows; i++) {
		ppTaskSwPipe[i] = new CRingBuffer((sizeof(size_t) + SW_MAX_INC_PACKET_LEN) * 1) ;
	}
	DTRACE("Task pipes=%i, task-pipe-len=%i bytes\n", i, ppTaskSwPipe[0]->getSize()) ;

	this->pSched = pSched ;

	linkState = stInit ;

	DTRACE("RETURN\n") ;
}

CSysSwIO::~CSysSwIO() {

	int taskRows = pSched->getTaskDescRows() ;

	// ERROR: After reopening fist packet is alwayw received
	// incorrectly
	//delete pSw ;

	delete pSwPipe ;
	for (int i = 0; i < taskRows; i++) {
		delete ppTaskSwPipe[i] ;
	}
}

//int b = 1 ;
int CSysSwIO::run(CARINCSched *pSched) {
	u_char packet[SW_MAX_OUT_PACKET_LEN] ;
	int packetLen ;

	CSysMpoMachine *pMachine = (CSysMpoMachine*)pSched->getTaskStatusRow(CSysMpoMachine::init)->pTask ;

	while (true) {

		bool swAppOutBufIsEmpty = false ;
		bool swDrvInpBufIsEmpty = false ;

		/*if ( pMachine->getState() != CSysMpoMachine::stFail ) {
			if (selectLinkedPort() == -1) {
				// Put system to error state because
				// we have more than one simultaneously linked ports
				pMachine->writeCtlSym(CSysMpoMachine::csErr) ;
			}
		}*/

		/*
		if((getLinkedPortsQty() > 1) && (pMachine->getState() != CSysMpoMachine::stFail)) {
			VPRINTF("SysSwIO: System error: %i-x SW linked ports are detected!\n", getLinkedPortsQty()) ;
				// Put system to error state because
				// we have more than one simultaneously linked ports
			pMachine->writeCtlSym(CSysMpoMachine::csErr) ;
		}*/

		int links = getLinkedPortsQty() ;

		if (linkState == stInit) {
			linkState = stWaitForLink ;
			VPRINTF("SysSwIO: WAIT for link on any port.\n") ;
		}

		if (linkState == stWaitForLink && links > 0 ) {
			linkState = stLinked ;
			VPRINTF("SysSwIO: Link to MZU ESTABLISHED.\n") ;
		}

		if (linkState == stLinked && links != 1) {
			linkState = stLinkError ;
			VPRINTF("SysSwIO: System error: %i-x SW linked ports are detected!\n", links) ;
			pMachine->writeCtlSym(CSysMpoMachine::csErr) ;
		}

		while ( !(swAppOutBufIsEmpty && swDrvInpBufIsEmpty) && (pSched->getMyTimeLeftMs() > 1)) {

			// Receive some SW packet
			packetLen = pSwr->recv(packet, sizeof(packet)) ;

			if (packetLen == -1) {
				if ( errno == EAGAIN ) {
					swDrvInpBufIsEmpty = true ;
					DTRACE("SW DRIVER input buffer is EMPTY.\n") ;
				}
			}

			//DTRACE("Received <- SpaceWire packet, len=%i bytes\n", len) ;

			// Process the SW packet
			if ( packetLen >= (int)sizeof(mmi_hdr0_t) ) {
				DTRACE("Received <- len=%i bytes\n\r", packetLen) ;
				CARINCSched::TStatusRow *pStatusRow = NULL ;
				CFiniteMachine::TState state = pMachine->getState() ;
				//if(b++ == 3) mips_break(0) ;

				uint16_t msgId = getMessageId(packet) ;
				uint16_t protoId = getProtoId(packet) ;
				uint16_t modeId ;
				if (protoId == MMI_PROTO_ID ) {
					switch( msgId ) {

					case MMI_CMD_MPO_SETMODE_MSG_ID:

						DVPRINTF("SysSwIO: MMI SETMODE command received, len=%i\n", packetLen) ;
						pStatusRow = pSched->getTaskStatusRow(CSysMpoMachine::init) ;
						if (pStatusRow) {
							writePacketForTask(pStatusRow->taskNo, packet, packetLen) ;
						} else {
							DTRACE("Can't get pStatusRow=0x%x\n") ;
						}

						modeId = getModeId(packet) ;
						switch( modeId ) {
						case MMI_MPO_MODE_ID_TRDEC:
							pStatusRow = pSched->getTaskStatusRow(CAppTransmit::init) ;
							if (pStatusRow) {
								writePacketForTask(pStatusRow->taskNo, packet, packetLen) ;
							} else {
								DTRACE("Can't get pStatusRow=0x%x\n") ;
							}

							break ;
						case MMI_MPO_MODE_ID_TRRAW:
							pStatusRow = pSched->getTaskStatusRow(CAppTransmit::init) ;
							if (pStatusRow) {
								writePacketForTask(pStatusRow->taskNo, packet, packetLen) ;
							} else {
								DTRACE("Can't get pStatusRow=0x%x\n") ;
							}
							break ;
						}

						break ;

					case MMI_CMD_MPO_STATUSRQ_MSG_ID:

						DVPRINTF("SysSwIO: MMI STATUSRQ command received.\n") ;
						pStatusRow = pSched->getTaskStatusRow(CSysMpoMachine::init) ;
						if (pStatusRow) {
							writePacketForTask(pStatusRow->taskNo, packet, packetLen) ;
						} else {
							DTRACE("Can't get pStatusRow=0x%x\n") ;
						}
						break ;
					/*case MMI_CMD_MPO_DATARQ_MSG_ID:
						DTRACE("DATARQ command received.\n") ;
						pMachine->getState() ;
						if (state == CSysMpoMachine::stTrDec) {
							pStatusRow = pSched->getTaskStatusRow(CAppTransmitDec::init) ;
						} else if (state == CSysMpoMachine::stTrRaw ) {
							pStatusRow = pSched->getTaskStatusRow(CAppTransmitRaw::init) ;
						}
						break ;*/

					default:

						DVPRINTF("SysSwIO: Unknown MMI message received <- msgId=0x%lx\n", msgId) ;
						break ;
					}
				} else {
					DVPRINTF("SysSwIO: Unknown protocol packet received <- protoId=0x%x, msgId=0x%x\n", protoId, msgId) ;
#ifdef DUMP
					printf("len=%i, hex:\n", packetLen) ;
					print_hex_dump(packet, packetLen) ;
#endif
				}

/*				// Put this packet into the pipe of certain task.
				if(pStatusRow) {
					// Also we have to check free space in the pipe

					// Put packet into certain pipe
					DTRACE("Writing data for taskNo=%i, pipe=0x%x, len=%i\n\r", pStatusRow->taskNo, (unsigned)&ppTaskSwPipe[pStatusRow->taskNo], packetLen) ;
					if (packetLen <= SW_MAX_INC_PACKET_LEN) {
						ppTaskSwPipe[pStatusRow->taskNo]->put(&packetLen, sizeof(size_t)) ;
						ppTaskSwPipe[pStatusRow->taskNo]->put(packet, packetLen) ;
					} else {
						TRACE("Error: MMI packet is too big! size = %i, max-size=%i\n", packetLen, SW_MAX_INC_PACKET_LEN) ;
					}
				} else {

					DTRACE("Can't get pStatusRow=0x%x\n") ;
				}*/
			}

			// Take task's SW packet from pipe
			int len ;
			len = pSwPipe->get(&packetLen, sizeof(size_t)) ;
			if ( len > 0 ) {
				DTRACE("Read packet-length from pSwPipe=0x%x, len=%i\n", (unsigned)pSwPipe, len) ;
				if((int)sizeof(packet) >= packetLen) {
					DTRACE("Length of packet is correct.\n") ;
					len = pSwPipe->get(&packet, packetLen) ;
					DTRACE("Read packet from pSwPipe=0x%x, len=%i\n", (unsigned)pSwPipe, packetLen) ;
#if defined(VERBOSE) && !defined(DEBUG)
					printf("SysSwIO: Read packet from task, len=%i\n", len) ;
#endif

					if (packetLen == len) {
						DTRACE("We read full packet from Task buffer.\n") ;

						for(int i = 0; i < getLinkedPortsQty(); i++) {
							// Select next linked port
							selectLinkedPort() ;
							// Send the this packet to MZU
							u_char addr[1] = { SW_MZU_ADDR } ;
							pSwr->sendTo(addr, packet, packetLen) ;
#if defined(VERBOSE) || defined(DEBUG)
							printf("SysSwIO: Send -> packet to addr=%i, packet-len=%i\n", (int)addr[0], packetLen) ;
#endif
						}
					} else {	// packetLen < len
						DTRACE("We read a part of packet from TASK output buffer.\n") ;
						// Clear pipe if we can't read whole packet
						pSwPipe->clear() ;
						DTRACE("TASK output buffer is CLEARED.\n") ;
					}
				}
				swAppOutBufIsEmpty = false ;
			} else {
				swAppOutBufIsEmpty = true ;
				DTRACE("TASK output buffer is EMPY.\n") ;
			}
		}	// while input buffers is not empty || task output buffer is not empty

		pSched->waitForMyTime() ;
	} // while(true)

	return 0 ;
}

uint32_t CSysSwIO::getProtoId(void *pMpoPacket) {
	mmi_hdr0_t *pHdr = (mmi_hdr0_t*)pMpoPacket ;

	return pHdr->proto_id ;
}

uint32_t CSysSwIO::getMessageId(void *pMpoPacket) {
	mmi_hdr0_t *pHdr = (mmi_hdr0_t*)pMpoPacket ;

	return pHdr->msg_id ;
}

uint32_t CSysSwIO::getModeId(void *pMpoPacket) {
	mmi_cmd_mpo_setmode_t *pCmd = (mmi_cmd_mpo_setmode_t*)pMpoPacket ;

	//
	//	It is necessary to add hdr0 check here.
	//
	return pCmd->mode_id ;
}

/*
 * The task calls this procedure to read its packet which came from MZU.
 */
int CSysSwIO::getMyPacket(void *pData, size_t len) {
	CARINCSched::TStatusRow *pSr = pSched->getTaskStatusRow(rtems_task_self()) ;

	DTRACE("this=0x%x\n", (unsigned)this) ;
	DTRACE("pStatusRow=0x%x, taskNo=%i, pipe=0x%x\n\r", (unsigned)pSr, pSr->taskNo, (unsigned)&ppTaskSwPipe[pSr->taskNo]) ;
	DTRACE("pSwPipe=0x%x\n\r", (unsigned)pSwPipe) ;

	size_t plen ;	// Packet length
	int rlen ;	// Actual data length
	rlen = ppTaskSwPipe[pSr->taskNo]->get(&plen, sizeof(size_t)) ;
	DTRACE("Read from pipe: len-of-len=%i, packet-len=%i\n\r", rlen, plen) ;
	if (rlen > 0 ) {
		if (len >= plen ) {
			rlen = ppTaskSwPipe[pSr->taskNo]->get(pData, plen) ;
			if ((int)plen == rlen) {
				return plen ;
			} else {
				// If we can't read whole packet then clear the pipe
				ppTaskSwPipe[pSr->taskNo]->clear() ;
			}
		}
	}
	return -1 ;
}

/*
 * This procedure puts a packet from task to an internal queue, and then
 * SysSwio will read this packet and send it to MZU.
 */
int CSysSwIO::putMyPacket(void *pData, size_t len) {
#ifdef DEBUG
	CARINCSched::TStatusRow *pSr = pSched->getTaskStatusRow(rtems_task_self()) ;
#endif

	CSysMpoMachine *pMachine = (CSysMpoMachine*)pSched->getTaskStatusRow(CSysMpoMachine::init)->pTask ;

	DTRACE("this=0x%x\n", (unsigned)this) ;
	DTRACE("pStatusRow=0x%x, taskNo=%i, pipe=0x%x,  len=%i\n\r", (unsigned)pSr, pSr->taskNo, (unsigned)pSwPipe, len) ;
	DTRACE("pSwPipe=0x%x\n\r", (unsigned)pSwPipe) ;

	size_t availSize = pSwPipe->getAvailSize() ;

	if (availSize < sizeof(size_t) + len) {

		TRACE("Internal error: TASK output buffer OVERFLOW.\n") ;
		DTRACE("pipe-avail-size=%i\n", availSize) ;
		pMachine->writeCtlSym(CSysMpoMachine::csErr) ;
		return 0 ;
	}

	pSwPipe->put(&len, sizeof(size_t)) ;
	pSwPipe->put(pData, len) ;

	return 0 ;
}

/*
 * Put this packet into the pipe of certain task.
 */
int CSysSwIO::writePacketForTask(int taskNo, void *pData, size_t len) {
		// Also we have to check free space in the pipe
		// Put packet into certain pipe
	DTRACE("Writing data for taskNo=%i, pipe=0x%x, len=%i\n\r", taskNo, (unsigned)&ppTaskSwPipe[taskNo], len) ;
	CSysMpoMachine *pMachine = (CSysMpoMachine*)pSched->getTaskStatusRow(CSysMpoMachine::init)->pTask ;

	if (len <= SW_MAX_INC_PACKET_LEN) {
		ppTaskSwPipe[taskNo]->put(&len, sizeof(size_t)) ;
		ppTaskSwPipe[taskNo]->put(pData, len) ;
	} else {
		TRACE("Error: MMI packet is too big! size = %i, max-size=%i\n", len, SW_MAX_INC_PACKET_LEN) ;
		pMachine->writeCtlSym(CSysMpoMachine::csErr) ;
	}
	return 0 ;
}

int CSysSwIO::selectLinkedPort() {
	u_int	newLinkedPort = linkedPort ;
	u_int	linkedPorts = getLinkedPortsQty() ;

	// Select new linked one
	for (u_int i = 1; i <=2; i++) {
		if(pSwr->isPortConnected(i) && linkedPort != i) {
			newLinkedPort = i ;
			break ;
		}
	}

	if (linkedPort != newLinkedPort) {
		linkedPort = newLinkedPort ;
		VPRINTF("SysSwIO: Port#%i is CONNECTED.\n", linkedPort) ;
		pSwr->setOutputPort(linkedPort) ;
		VPRINTF("SysSWIO: Set Port#%i to be OUTPUT\n", linkedPort) ;
	}

	return 0 ;
}

int CSysSwIO::getLinkedPortsQty() {
	u_int linkedPorts = 0 ;

	// Count all linked ports
	for (u_int i = 1; i <=2; i++) {
		if(pSwr->isPortConnected(i)) {
			linkedPorts++ ;
		}
	}

	return linkedPorts ;
}
