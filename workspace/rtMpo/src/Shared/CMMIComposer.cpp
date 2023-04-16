/*
 * CMMIComposer.cpp
 *
 *  Created on: Jul 31, 2020
 *      Author: anton
 */

#if 0
	#define DEBUG
#endif

#if 1
	#define VERBOSE
#endif

#include "CMMIComposer.h"
#include "../System/MMIProto.h"
#include "auxio.h"
#include <string.h>
#include <crc16.h>
#include "../Tasks/CSysInit.h"

char CMMIComposer::strState[stMax][32] = { 0 };

CMMIComposer::CMMIComposer() {
	DTRACE("BEGIN\n") ;

	pInfBuffer = new CBuffer(MMI_MPO_AIS_INF_PACKET_LEN) ;
	infState = stInitInfPacketHdr ;


	for (u_int i = 0; i < 2; i++) {
		pQuadBuffer[i] = new CBuffer(MMI_MPO_FPGA_QUAD_PACKET_LEN) ;
		quadState[i] = stInitQuadPacketHdr ;
	}
	quadIdx = 0 ;

	// Initialize state to string array.
	if (*strState[stMin+1] == 0) {
		strcpy(strState[stInitInfPacketHdr], "stInitInfPacketHdr") ;
		strcpy(strState[stWaitInfBlockHdr], "stWaitInfBlockHdr") ;
		strcpy(strState[stWaitInfBlockMsg], "stWaitInfBlockMsg") ;
		strcpy(strState[stInfPacketIsReady], "stInfPacketIsReady") ;
	}

	DTRACE("InfBuffer:(ptr=0x%x, size=%i, avail=%i), infState=%s\n",
			pInfBuffer, pInfBuffer->getSize(), pInfBuffer->getAvail(), stateToStr(infState)) ;
	DTRACE("RETURN\n") ;
}

CMMIComposer::~CMMIComposer() {
	delete pInfBuffer ;
}

/*
 * Common routines.
 */

char *CMMIComposer::stateToStr(int state) {
	// Check correctness of a state
	if (state < stMin || state > stMax) {
		return NULL ;
	}

	return strState[state] ;
}

/*
 * AIS information message processing
 */

int CMMIComposer::addAISInfDataBlockHdr(uint16_t BShVOff, uint8_t powCdBm, uint8_t chNo, uint8_t recvNo, uint8_t deltaDopFrqCHz) {
	DTRACE("BEGIN\n") ;
	DTRACE("infState=%s, InfBuffer:fill=%i\n", stateToStr(infState), pInfBuffer->getFill()) ;

	if(infState == stInitInfPacketHdr) {
		mmi_mpo_ais_inf_packet_hdr_t packetHdr = { 0 } ;

		infState = stWaitInfBlockHdr ;

		packetHdr.hdr0.proto_id = MMI_PROTO_ID ;
		packetHdr.hdr0.msg_id = MMI_MPO_AIS_INF_PACKET_MSG_ID ;

		// Clear the buffer if needed.
		if(pInfBuffer->getFill() > 0) {
			pInfBuffer->clear() ;
		}

		pInfBuffer->add(&packetHdr, sizeof(packetHdr)) ;
		DTRACE("infState=%s, InfBuffer:fill=%i\n", stateToStr(infState), pInfBuffer->getFill()) ;
	}

	if(infState == stWaitInfBlockHdr) {
		mmi_mpo_ais_inf_data_block_hdr_t blockHdr = { 0 } ;

		infState = stWaitInfBlockMsg ;

		blockHdr.bshv_off = BShVOff ;
		blockHdr.powc_dbm = powCdBm ;
		blockHdr.ch_no = chNo ;
		blockHdr.recv_no = recvNo ;
		blockHdr.delta_Dop_frq_c_Hz = deltaDopFrqCHz ;

		pInfBuffer->add(&blockHdr, sizeof(blockHdr)) ;

		DTRACE("infState=%s, InfBuffer:fill=%i\n", stateToStr(infState), pInfBuffer->getFill()) ;
		DTRACE("RETURN 0\n") ;
		return 0 ;
	}

	DTRACE("RETURN -1\n") ;
	return -1 ;
}

int CMMIComposer::addAISInfDataBlockMsg(void *pData, size_t len) {
	int status = -1 ;

	DTRACE("BEGIN, pData=0x%x, len=%d\n", pData, len) ;
	DTRACE("infState=%s, InfBuffer:fill=%i\n", stateToStr(infState), pInfBuffer->getFill()) ;

	if (infState == stWaitInfBlockMsg) {
		mmi_mpo_ais_inf_data_block_hdr_t *pBlockHdr ;

		infState = stWaitInfBlockHdr ;

		// Write down length of the block into block header
		// first
		pBlockHdr = (mmi_mpo_ais_inf_data_block_hdr_t*)pInfBuffer->getPtr(-sizeof(mmi_mpo_ais_inf_data_block_hdr_t)) ;
		pBlockHdr->len = len ;
		// Now, add AIS data message
		pInfBuffer->add(pData, len) ;

		// if the rest of buffer less than maximum size AIS data
		// block, then fill rest by 0.
		u_short crc16 ;
		if (pInfBuffer->getAvail() < MMI_MAX_INF_DATA_BLOCK_SIZE + sizeof(crc16)) {
			//
			// Fill free space by 0 except last 2 bytes
			//
			while(pInfBuffer->getAvail() > sizeof(crc16)) {
				pInfBuffer->add((u_char)0) ;
			}

		} else {

			status =  STATUS_OK ;
		}

		if (pInfBuffer->getAvail() == sizeof(crc16)) {
			//
			//	Calculate crc16 and write into the end of data packet
			//
			DTRACE("Calculating CRC16 for Inf packet...\n") ;
			crc16 = crc16_calc(&crc16_arc_context, CRC16_ARC_INIT_VAL, pInfBuffer->getPtr(0), pInfBuffer->getFill()) ;

			DTRACE("Adding CRC16 to the end of Inf packet.\n") ;
			pInfBuffer->add(crc16) ;

			infState = stInfPacketIsReady ;
			status = STATUS_AIS_INF_PACKET_IS_READY ;
		}

	}

	DTRACE("infState=%s, InfBuffer:fill=%i\n", stateToStr(infState), pInfBuffer->getFill()) ;
	DTRACE("RETURN %i\n", status) ;
	return status ;
}

int CMMIComposer::removeAISInfDataBlockHdr() {
	int status = -1 ;
	DTRACE("BEGIN\n") ;

	if (infState == stWaitInfBlockMsg) {

		infState = stWaitInfBlockHdr ;
		pInfBuffer->trim(-sizeof(mmi_mpo_ais_inf_data_block_hdr_t)) ;

		DTRACE("infState=%s, InfBuffer:fill=%i\n", stateToStr(infState), pInfBuffer->getFill()) ;
		status = STATUS_OK ;
	}

	DTRACE("RETURN %i\n", status) ;
	return status ;
}

void *CMMIComposer::getAISInfPacketPtr() {

	if (infState == stInfPacketIsReady) {
		return pInfBuffer->getPtr(0) ;
	}

	return NULL ;
}

int CMMIComposer::getAISInfPacketLen() {

	return pInfBuffer->getFill() ;
}

void CMMIComposer::clearAISInfPacket() {

	infState = stInitInfPacketHdr ;
	pInfBuffer->clear() ;
}

/*
 * FPGA quadrature messages processing.
 */
int CMMIComposer::initFPGAQuadPacketHdr(uint16_t BShVOff, uint8_t chNo) {

	DTRACE("BEGIN\n") ;

	//
	// Add quadratures packet header first.
	//
	if(quadState[quadIdx] == stInitQuadPacketHdr) {
		mmi_mpo_fpga_quad_packet_hdr_t packetHdr = { 0 } ;

		quadState[quadIdx] = stWaitQuadBlockMsg ;

		packetHdr.hdr0.proto_id = MMI_PROTO_ID ;
		packetHdr.hdr0.msg_id = MMI_MPO_FPGA_QUAD_PACKET_MSG_ID ;

		// Clear the buffer if needed.
		if(pQuadBuffer[quadIdx]->getFill() > 0) {
			pQuadBuffer[quadIdx]->clear() ;
		}

		pQuadBuffer[quadIdx]->add(&packetHdr, sizeof(packetHdr)) ;
		DTRACE("quadState=%s, QuadBuffer:fill=%i\n", stateToStr(quadState[quadIdx]), pQuadBuffer[quadIdx]->getFill()) ;
	}

	DTRACE("RETURN 0\n") ;
	return 0 ;

}

int CMMIComposer::addFPGAQuadDataBlockMsg(void *pData, size_t len) {
	int status = STATUS_OK ;

	DTRACE("BEGIN\n") ;
	DTRACE("quadState=%s, QuadBuffer:fill=%i\n", stateToStr(quadState[quadIdx]), pQuadBuffer[quadIdx]->getFill()) ;

	//
 	// Then add next quadrature message block to the packet.
	//
	if(quadState[quadIdx] == stWaitQuadBlockMsg) {
		quadState[quadIdx] = stWaitQuadBlockMsg ;

		// if the rest of buffer less than maximum size FPGA
		// quadrature block, then fill rest by 0.
		u_short crc16 ;
		if (pQuadBuffer[quadIdx]->getAvail() < len + sizeof(crc16)) {

			while(pQuadBuffer[quadIdx]->getAvail() > sizeof(crc16)) {
				pQuadBuffer[quadIdx]->add((u_char)0) ;
			}

			// We have data which size is bigger than rest of
			// current Quad buffer, so switch to another and write
			// this data into it.
			pQuadBuffer[(quadIdx+1)%2]->add(pData, len) ;
		} else {

			pQuadBuffer[quadIdx]->add(pData, len) ;
		}

		if (pQuadBuffer[quadIdx]->getAvail() == sizeof(crc16)) {
			//
			//	Calculate crc16 value and write into the end of the data packet
			//
			crc16 = crc16_calc(&crc16_arc_context, CRC16_ARC_INIT_VAL, pQuadBuffer[quadIdx]->getPtr(0), pQuadBuffer[quadIdx]->getFill()) ;
			pQuadBuffer[quadIdx]->add(crc16) ;

			quadState[quadIdx] = stQuadPacketIsReady ;
			status = STATUS_FPGA_QUAD_PACKET_IS_READY ;

			DTRACE("quadState[%i]=%s, QuadBuffer:fill=%i\n", quadIdx, stateToStr(quadState[quadIdx]), pQuadBuffer[quadIdx]->getFill()) ;

			// Switch buffer index to another half when currnt packet is
			// completed.
			++quadIdx %= 2 ;
		}
	}

	DTRACE("RETURN %i\n", status) ;
	return status ;
}

void *CMMIComposer::getFPGAQuadPacketPtr() {

	for (u_int i = 0; i < 2; i++) {
		if (quadState[i] == stQuadPacketIsReady) {
			return pQuadBuffer[i]->getPtr(0) ;
		}
	}

	return NULL ;
}

int CMMIComposer::getFPGAQuadPacketLen() {
	for (u_int i = 0; i < 2; i++) {
		if (quadState[i] == stQuadPacketIsReady) {
			return pQuadBuffer[i]->getFill() ;
		}
	}
	return -1 ;
}

void CMMIComposer::clearFPGAQuadPacket() {

	for (u_int i = 0; i< 2; i++) {
		if (quadState[i] == stQuadPacketIsReady) {
			pQuadBuffer[i]->clear() ;
			quadState[i] = stInitQuadPacketHdr ;
			return ;
		}
	}
}
