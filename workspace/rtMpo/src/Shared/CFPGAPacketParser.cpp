//============================================================================
// Name        : lxDdcRead.cpp
// Author      : Anton Ermakov
// Version     :
// Copyright   : Your copyright notice
// Description :
//============================================================================

#if 0
	#define DEBUG
#endif

#if 1
	#define VERBOSE
#endif

#if 0
	#define REMOVE_DUPLICATED
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include "auxio.h"
#include "CFPGAPacketParser.h"
#include "../System/MMIProto.h"

enum state {st_FIND_FPGA_TOKEN, st_READ_FPGA_HEADER, st_READ_PAYLOAD} ;
enum msym {ss_BOTTOM=1 , ss_INF=2, ss_QUAD=3, ss_START=4} ;

CFPGAPacketParser::CFPGAPacketParser() {
    
	// Create new objects
    pStack = new CPushStack( ss_BOTTOM, 16 ) ;
    pCRCFifo = new CRingBuffer( 1024 ) ;
	pDataBuf = new CBuffer(4096) ;
	pWndBuf = new CWndBuffer(4) ;
	pMMIComposer = new CMMIComposer() ;
    
	// Set up Initial state
	state = st_FIND_FPGA_TOKEN ;
	pStack->push(ss_START) ;

    // Set up initial values
	curTokenNo = 0 ;
	duplicatedPackets = 0 ;
	CRCErrors = 0 ;

	// Enable override mode when CRC buffer is overflowed
	pCRCFifo->setOverride(true) ;

	spFPGA = NULL ;
}

CFPGAPacketParser::CFPGAPacketParser(std::shared_ptr<CFPGA> &spFPGA): CFPGAPacketParser() {

	this->spFPGA = spFPGA ;
}

CFPGAPacketParser::~CFPGAPacketParser() {

	// Delete all dynamic objects
	delete pMMIComposer ;
	delete pWndBuf ;
	delete pDataBuf ;
	delete pCRCFifo ;
	delete pStack ;
}

/*
 * AIS information messages processing.
 */

int CFPGAPacketParser::processCurAISInfFPGAToken() {

#ifdef VERBOSE
	printf("================================================\n") ;
	printf("%i: DDC_INF_MSG_TOKEN=0x%x\n", curTokenNo, pWndBuf->getWord(0)) ;
#endif

	return 0 ;
}

int CFPGAPacketParser::processCurAISInfFPGAHdr() {
	pld_inf_msg_hdr_t *pHdr = (pld_inf_msg_hdr_t*)pDataBuf->getPtr(0);

	// Get length of a packet, in header
	// length specified in bits.
	curPacketLenBits = pHdr->len ;
	curPacketLenBytes = curPacketLenBits / 8 ;
	if ( curPacketLenBits % 8 ) curPacketLenBytes ++ ;

	// Too big packet, probably wrong token
	// so try to find a new token
	//if (curPacketLenBytes > 1920/8) {
	if (curPacketLenBytes > MMI_MAX_INF_DATA_BLOCK_SIZE ) {
		state = st_FIND_FPGA_TOKEN ;
		pStack->clear() ;
		pStack->push(ss_START) ;

	    VPRINTF("len(bytes) = %i: AIS data is too big!\n", curPacketLenBytes) ;
	    curTokenNo -- ;
	    return 0 ;
	}

	if ( pHdr->crc_err )	{
		curPacketCRCError = true ;
		CRCErrors ++ ;
	} else {
		curPacketCRCError = false ;
	}

	// Doppler Offset Frequency Code calculation:
	int deltaDopFreqEd = pHdr->freq_hz ;
	int deltaDopFreqHz = deltaDopFreqEd * (double)pow(2, -24) * 8 * 9600;
	int deltaDopFreqCHz = (deltaDopFreqHz - 4000) / 500 ;

	// Signal channel calculation:
	int chNo = pHdr->chan;
	chNo %= 4 ;

	// Signal receiver(tract) calculation:
	int rcvNo = pHdr->chan ;
	rcvNo /= 4 ;

	// Signal power calculation:
	int powCdBm = pHdr->ampl ;

#ifdef VERBOSE
	printf("FPGAParser: %i: INFO packet HEADER, token=0x%x, chan=%i, len(bits,bytes) = (%i,%i), freq_hz=%i, deltaFrqDopHz = %i,\n ampl = %i, crc_err=%d\n",
			curTokenNo, pHdr->token, chNo, curPacketLenBits, curPacketLenBytes, deltaDopFreqEd, deltaDopFreqHz, pHdr->ampl, pHdr->crc_err) ;

	printf("FPGAParser: %i: HDR:\n", curTokenNo) ;

	pDataBuf->printHexDump() ;
#endif

	// Add AIS data block header to AIS packet
	// if no CRC error is message
	/*if (!curPacketCRCError) {
		pMMIComposer->addAISInfDataBlockHdr(0, powCdBm, chNo, rcvNo, deltaDopFreqCHz) ;
	}*/

	// Add AIS MMI header even message has CRC error, because we can fix it later.
	pMMIComposer->addAISInfDataBlockHdr(0, powCdBm, chNo, rcvNo, deltaDopFreqCHz) ;

	return 0 ;
}

int CFPGAPacketParser::processCurAISInfMsgFPGA() {
    /*
     * Process a packet.
	 */
	curPacketIsDuplicated = false ;
	int status = 0 ;
	uint16_t CRC16 ;

	DTRACE("BEGIN\n") ;

	//if (!curPacketCRCError) {
		// Caclulate CRC of AIS packet without last 2 bytes, because they are CRC16 too.
		// uint16_t crc = crc16_calc(pDataBuf->getArray(), pDataBuf->getFill()-2) ;

	// Get CRC16 from AIS packet
	int dataLen = pDataBuf->getFill()-2 ;
	DTRACE("DataBuf: dataLen = %i\n", dataLen) ;
	pDataBuf->read(dataLen, &CRC16, sizeof(CRC16)) ;

	// Check packet is duplicated
	size_t CRCArrayLen = pCRCFifo->getFill()/sizeof(CRC16) ;
	DTRACE("CRCArrayLen=%i\n", CRCArrayLen) ;
	for(size_t j = 0; j < CRCArrayLen; j++) {
		uint16_t bufCrc ;
		pCRCFifo->read(j*sizeof(bufCrc), &bufCrc, sizeof(bufCrc)) ;
		if ( bufCrc == CRC16 ) {
			curPacketIsDuplicated = true ;
			break ;
		}
	}

	//VPRINTF("FPGAParser: %i: AIS PAYLOAD+CRC16: \n", curTokenNo) ;

#ifdef VERBOSE
	printf("FPGAParser: %i: INF Payload+CRC16: actual-len=%i:\n", curTokenNo, pDataBuf->getFill()) ;
	printf("HEX:\n") ;
	pDataBuf->printHexDump() ;

#if 0
	printf("BIN:\n") ;
	pDataBuf->printBinDump() ;
#endif

	printf("FPGAParser: %i: crc16 = 0x%x\n", curTokenNo, (u_int)CRC16) ;
	//printf("FPGAParser: %i: actual-len = %lu\n", curTokenNo, pDataBuf->getFill()) ;
#endif

	if(curPacketIsDuplicated) {
#ifdef VERBOSE
			printf("%i: packet DUPLICATE (CRC match).\n", curTokenNo) ;
#endif
			duplicatedPackets++ ;
	}

	// Try to fix CRC error if needed
	if (!curPacketIsDuplicated && curPacketCRCError) {
		// Fix CRC error here.
		// If successful, then curPacketCRCError = false ;
	}

	// Do not check duplicated messages for debug purpose, add it anyway
#if defined(REMOVE_DUPLICATED)
	if (!curPacketIsDuplicated && !curPacketCRCError) {
#else
	if (!curPacketCRCError) {
#endif
		pCRCFifo->put(&CRC16, sizeof(CRC16)) ;
		status = pMMIComposer->addAISInfDataBlockMsg( pDataBuf->getPtr(0), dataLen ) ;
	}

	// Duplicated packets doesn't remove for debug purposes
#if defined(REMOVE_DUPLICATED)
	if( curPacketIsDuplicated || curPacketCRCError ) {
#else
	if( curPacketCRCError ) {
#endif
		pMMIComposer->removeAISInfDataBlockHdr() ;

	}

	/*
	 * End of the packet processing.
	 */
	DTRACE("RETURN %i\n", status) ;
	return status ;
}

/*
 * FPGA quadrature messages processing.
 */

int CFPGAPacketParser::processCurFPGAQuadToken() {

#ifdef VERBOSE
	printf("================================================\n") ;
	printf("%i: DDC_QUAD_MSG_TOKEN=0x%x\n", curTokenNo, pWndBuf->getWord(0)) ;
#endif

	return 0 ;
}

int CFPGAPacketParser::processCurFPGAQuadHdr() {

	pld_quad_msg_hdr_t *p_hdr = (pld_quad_msg_hdr_t*)pDataBuf->getPtr(0);

	//
	// Get length of packets which specified in
	// 32bit words
	//
	curPacketLenBytes = p_hdr->len ;
	curPacketLenBytes *= 4 ;

	if (curPacketLenBytes > 255*4) {
		state = st_FIND_FPGA_TOKEN ;
	    pStack->clear() ;
	    pStack->push(ss_START) ;
	    curTokenNo -- ;

	    return 0 ;
	}

#ifdef VERBOSE
	printf("FPGAParser: %i: QUAD packet HEADER, len(bytes) = %i\n", curTokenNo, curPacketLenBytes) ;
	printf("HEX:\n") ;
	pDataBuf->printHexDump() ;
#endif

	return 0 ;
}

int CFPGAPacketParser::processCurFPGAQuadMsg() {
	int status ;

	int dataLen = pDataBuf->getFill() ;

	pMMIComposer->initFPGAQuadPacketHdr(/*uitnt32_t BShVTime*/0, 0/*spFPGA->getCurrentFreqChannel(0)*/) ;

	status = pMMIComposer->addFPGAQuadDataBlockMsg(pDataBuf->getPtr(0), dataLen) ;

#ifdef VERBOSE
	printf("FPGAParser: %i: QUAD Payload, len(bytes) = %i\n", curTokenNo, dataLen) ;
	printf("HEX:\n") ;
	pDataBuf->printHexDump() ;
#endif

	return status ;
}

/*
 * Returns true when AIS data block is recognized and it
 * is not a duplicate, returns false otherwise.
 */
int CFPGAPacketParser::feed(u_char byte) {
	int status = 0 ;
	pWndBuf->add(byte) ;

	switch(state) {

	// Find packet token in a data flow
	case st_FIND_FPGA_TOKEN:

		if (pStack->getTop() == ss_START) {
			pStack->pop() ;
			//
			// If we just start finding FPGA packet token
			// then clear data buffer.
			//
			pDataBuf->clear() ;
		}

		if ( pWndBuf->getWord(0) == FPGA_INF_MSG_TOKEN ||
			 pWndBuf->getWord(0) == FPGA_QUAD_MSG_TOKEN ) {

			state = st_READ_FPGA_HEADER ;

			pDataBuf->clear() ;
			pDataBuf->add(pWndBuf->getDWord(0)) ;

			DTRACE("WndBuf->getDWord()=0x%x\n", pWndBuf->getDWord(0)) ;

			curTokenNo++ ;
		}

		// Process information data token
		if ( pWndBuf->getWord(0) == FPGA_INF_MSG_TOKEN ) {
			pStack->push(ss_INF) ;

			processCurAISInfFPGAToken() ;
		}

		// Process telemetry token
		else if ( pWndBuf->getWord(0) == FPGA_QUAD_MSG_TOKEN ) {
			pStack->push(ss_QUAD) ;

			processCurFPGAQuadToken() ;
		}
		break ;

	//
	// Processing FPGA data header state:
	//
	case st_READ_FPGA_HEADER:

		if (pStack->getTop() == ss_INF) {

			if ( pDataBuf->getFill() < sizeof(pld_inf_msg_hdr_t) ) {
				pDataBuf->add(pWndBuf->getByte(3)) ;

			} else {
				state = st_READ_PAYLOAD ;
				pStack->push(ss_START) ;

				processCurAISInfFPGAHdr() ;
			}
		}

		//
		// Process quadrature header
		//
		else if (pStack->getTop() == ss_QUAD) {

			if ( pDataBuf->getFill() < sizeof(pld_quad_msg_hdr_t) ) {
				pDataBuf->add(pWndBuf->getByte(3)) ;

			} else {
				//
				// Switch to the next state.
				//
				state = st_READ_PAYLOAD ;
				pStack->push(ss_START) ;

				processCurFPGAQuadHdr() ;
			}
		}
		break ;

	//
	// Reading AIS data block state:
	//
	case st_READ_PAYLOAD:

		// When we just start read the payload
		if(pStack->getTop() == ss_START) {
			pStack->pop() ;

			//
			// If we just start reading AIS data block, then clear data
			// buffer and add to it first byte
			//
			pDataBuf->clear() ;
			pDataBuf->add(pWndBuf->getByte(2)) ;
		}

		if ( pDataBuf->getFill() < (size_t)curPacketLenBytes) {
			pDataBuf->add(pWndBuf->getByte(3)) ;

		} else {
			//
            // When we reach the end of payload switch to
			// the next state
			//
			state = st_FIND_FPGA_TOKEN ;

			if (pStack->getTop() == ss_INF) {

				status = processCurAISInfMsgFPGA() ;
				DTRACE("processCurAISInfMsgFPGA() return=%i\n", status) ;

			} else if (pStack->getTop() == ss_QUAD) {

				status = processCurFPGAQuadMsg() ;
				DTRACE("processCuFPGAQuadMsg() return=%i\n", status) ;
			}

			pStack->pop() ;					// ss_TEL or ss_INF
			pStack->push(ss_START) ;
		}
		break ;
	}

	return status ;
}

void CFPGAPacketParser::printStatistics() {
	printf("\n") ;
	printf("Tokens = %i\n", curTokenNo) ;
	printf("Found DUPLICATES = %i !!!\n", duplicatedPackets) ;
	printf("CRC Errors = %i\n", CRCErrors) ;
}

CMMIComposer *CFPGAPacketParser::getMMIComposerPtr() {

	return pMMIComposer;
}
