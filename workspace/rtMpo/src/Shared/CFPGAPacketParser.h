/*
 * ddcProto.h
 *
 *  Created on: Jan 15, 2020
 *      Author: anton
 */
#ifndef CPLDPARSER_H_
#define CPLDPARSER_H_

#include <sys/types.h>
#include <stdint.h>
#include <memory>

#include "../Shared/CRingBuffer.h"
#include "../Shared/CWndBuffer.h"
#include "crc16.h"
#include "CBuffer.h"
#include "CPushStack.h"
#include "CMMIComposer.h"
#include "CFPGA.h"

/*
 * pragma pack(push, 2) cause exception 4
 * inside switch/case operator when read p_hdr->len.
 */
#pragma pack(push,1)

// PLD header of INFO packet
typedef struct {
	// First double word
	uint32_t	token: 16 ;
	uint32_t	len: 12 ;
	uint32_t	crc_err: 1 ;
	uint32_t	chan: 3 ;

	// Second double word
	uint32_t	freq_hz ;

	// Third double word
	uint32_t	ampl ;

} pld_inf_msg_hdr_t ;

#define FPGA_INF_MSG_TOKEN	0x55aa

// PLD header of QUAD packet
typedef struct {
	uint32_t	token: 16 ;
	uint32_t	len: 8 ;
	uint32_t	src: 1 ;
	uint32_t	chan: 3 ;
	uint32_t	unused0: 4 ;

} pld_quad_msg_hdr_t ;

#define FPGA_QUAD_MSG_TOKEN	0x55ab

typedef union {
	u_char		byte[4] ;
	uint32_t	dword ;
	struct {
		uint16_t l ;
		uint16_t h ;
	} word ;
} wnd32_buf_t ;

/*typedef struct {
	uint16_t	time_off ;
	uint8_t		power ;
	uint8_t		channel: 3 ;
	uint8_t		tract: 1 ;
	uint8_t		frq_off: 4 ;
	uint8_t		len ;		// 9-133 bytes
} mpo_AIS_inf_block_hdr_t ;*/

#pragma pack(pop)

class CFPGAPacketParser {
private:
	int			state ;			// parser state
	CPushStack	*pStack ;

	CRingBuffer *pCRCFifo ;
	CBuffer		*pDataBuf ;
	CWndBuffer	*pWndBuf ;
	int			curTokenNo ;
	bool		curPacketIsDuplicated ;
	int			curPacketLenBits ;
	int			curPacketLenBytes ;
	bool		curPacketCRCError ;
	int			duplicatedPackets ;
	int			CRCErrors ;
	int			blockType ;
	CMMIComposer	*pMMIComposer ;
	std::shared_ptr<CFPGA> spFPGA ;

public:
	CFPGAPacketParser() ;
	CFPGAPacketParser(std::shared_ptr<CFPGA> &spFPGA) ;
	virtual ~CFPGAPacketParser() ;
	int feed(u_char byte) ;

	int processCurAISInfFPGAToken() ;
	int processCurAISInfFPGAHdr() ;
	int processCurAISInfMsgFPGA() ;

	int processCurFPGAQuadToken() ;
	int processCurFPGAQuadHdr() ;
	int processCurFPGAQuadMsg() ;
	void printStatistics() ;

	void* getCurDataBufferPtr() ;
	int getCurDataBufferLen() ;

	CMMIComposer *getMMIComposerPtr() ;
} ;

#endif /* CPLDPARSER_H_ */
