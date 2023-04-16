/*
 * CMMIComposer.h
 *
 *  Created on: Jul 31, 2020
 *      Author: anton
 */

#ifndef CMMICOMPOSER_H_
#define CMMICOMPOSER_H_

#include <CBuffer.h>
#include <CRingBuffer.h>
#include <crc16.h>

class CMMIComposer {
	CBuffer 	*pInfBuffer ;
	int 		infState ;
	u_int		infIdx ;

	CBuffer 	*pQuadBuffer[2] ;
	int 		quadState[2] ;
	u_int		quadIdx ;

	enum states {stMin, stInitInfPacketHdr, stWaitInfBlockHdr, stWaitInfBlockMsg, stInfPacketIsReady,
		stInitQuadPacketHdr, stWaitQuadBlockMsg, stQuadPacketIsReady, stMax} ;
	static char strState[stMax][32] ;

public:
	CMMIComposer();
	virtual ~CMMIComposer();

	//
	//	Information packet routines
	//
	void initAISInfPacketHdr() ;
	int addAISInfDataBlockHdr(uint16_t BShVOff, uint8_t powCdBm, uint8_t chNo, uint8_t recvNo, uint8_t DopFrqOffCHz) ;
	int removeAISInfDataBlockHdr() ;
	int addAISInfDataBlockMsg(void *pData, size_t len) ;
	void *getAISInfPacketPtr() ;
	int getAISInfPacketLen() ;
	void clearAISInfPacket() ;

	//
	//	Quadrature packet routines
	//
	int initFPGAQuadPacketHdr(uint16_t BShVOff, uint8_t chNo) ;
	int addFPGAQuadDataBlockMsg(void *pData, size_t len) ;
	//void alignFPGAQuadPacketZero() ;
	//void calcFPGAQuadPacketCRC16() ;
	void *getFPGAQuadPacketPtr() ;
	int getFPGAQuadPacketLen() ;
	void clearFPGAQuadPacket() ;

	char *stateToStr(int state) ;

	enum status { STATUS_OK, STATUS_AIS_INF_PACKET_IS_READY, STATUS_FPGA_QUAD_PACKET_IS_READY } ;
};

#endif /* CMMICOMPOSER_H_ */
