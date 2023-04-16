
/*
 * CAppTransmitRaw.cpp
 *
 *  Created on: Jul 25, 2019
 *      Author: anton
 */
#if 0
	#define DEBUG
#endif

#if 1
	#define VERBOSE
#endif

#include <auxio.h>
#include <errno.h>
#include <math.h>
#include "CAppTransmit.h"
#include "../System/MMIProto.h"

#if 1
    #define ENABLE_WRITER
	#define WRITER_FILE_DEV_PATH	"/dev/lport1"
	//#define FILE_TO_READ            "/CH1_30mps_4slots32sec.bin"		// RAW (Quad)
	#define FILE_TO_READ            "/ch1_5mps_4slots_16sec_00.bin"	// DEC (Inf)
#endif
#if 1
    #define ENABLE_READER
	#define READER_FILE_DEV_PATH	"/dev/lport0"
#endif

CAppTransmit::CAppTransmit(CARINCSched *pSched) {
	pSysSwIO = (CSysSwIO*)pSched->getTaskStatusRow(CSysSwIO::init)->pTask ;
	pMachine = (CSysMpoMachine*)pSched->getTaskStatusRow(CSysMpoMachine::init)->pTask ;

	DTRACE("pSysSwIO=0x%x, pMachine=0x%x\n", pSysSwIO, pMachine) ;

#if defined(ENABLE_READER)
	pRLPort = new CLPort(READER_FILE_DEV_PATH, O_RDONLY) ;

	if (!pRLPort->isOk()) {
		printf("Can't open file %s: %s\n\r", READER_FILE_DEV_PATH, strerror(errno)) ;
		return ;
	}

	// We always work in NON BLOCK mode because of
	// ARINC dispatcher
	pRLPort->setNonBlock(true) ;
#endif

#if defined(ENABLE_WRITER)
    pWLPort = new CLPort(WRITER_FILE_DEV_PATH, O_WRONLY) ;

	if (!pWLPort->isOk()) {
		printf("Can't open file %s: %s\n\r", WRITER_FILE_DEV_PATH, strerror(errno)) ;
		return ;
	}
#endif

#if defined(ENABLE_WRITER)
	fd = open(FILE_TO_READ, O_RDONLY) ;
    if ( fd < 0 ) {
        printf("Cant open %s: %s\n", FILE_TO_READ, strerror(errno)) ;
    }

    // Enable Non-Block mode to be able sending data
    // when receiver is not ready yet. The data will be just
    // written to a driver buffer in this case.
    pWLPort->setNonBlock(true) ;
#endif

    //
    // Create FPGA object instance.
    //
    spFPGA = std::make_shared<CFPGA>() ;

    pFPGAParser = new CFPGAPacketParser(spFPGA) ;
}

CAppTransmit::~CAppTransmit() {
	// TODO Auto-generated destructor stub
}

int CAppTransmit::run(CARINCSched *pSched) {
	//sw_mpo_cmd_datarq_t request ;
	int transmitted = 0 ;
	u_char inpArray[128] ;
	u_char outArray[128] ;
	int nBytes ;
	mmi_cmd_mpo_setmode_t	mmiCmd ;

	DTRACE("Entering...\n\r") ;

	int len = pSysSwIO->getMyPacket(&mmiCmd, sizeof(mmiCmd)) ;
	if (len == -1) {
		TRACE("Internal error: Can't get command arguments.\n") ;
		//pMachine->writeCtlSym(CSysMpoMachine::csErr) ;
		//return 0 ;
	} else {
		VPRINTF("AppTransmit: received <- MMI cmd + ARG9, len=%i\n", len) ;
	}

	//
	// Load FPGA receiver coefficients before start
	// receiving.
	//
	CFPGA::recv_coefs_var_t coefs ;
	spFPGA->getDefaultReceiverCoeffs(coefs) ;

	CFPGA::recv_lport_var_t lport[2] ;
	spFPGA->getDefaultLPortParams(lport) ;

	if (len > 0) {
		spFPGA->parseMMISetModeCmdArgs(mmiCmd, coefs, lport) ;
	}

#ifdef VERBOSE
	printf("AppTransmit: APPLY FPGA receiver coeffs:\n") ;
	CFPGA::printCoefs(coefs) ;
#endif
	spFPGA->applyReceiverCoeffs(coefs) ;

	//
	// We have to clear all LPort buffered data at this point.
	//
	VPRINTF("AppTransmit: RESET lport.\n") ;
	CFPGA::recv_lport_var_t lport_reset[2] ;
	for(int i = 0; i < ARRLEN(lport_reset); i++) {
		lport_reset[i] = lport[i] ;
		lport_reset[i].en = 0 ;
	}
	spFPGA->applyLPortParams(lport_reset) ;

	pRLPort->flush() ;

	//
	// Load lport settings to transmit the data.
	//
#ifdef VERBOSE
	printf("AppTransmit: APPLY FPGA lport params:\n") ;
	CFPGA::printLPortParams(lport) ;
#endif
	spFPGA->applyLPortParams(lport) ;

	while(true) {

		// Receive data blocks from LPORT0
		// We continue processing lport data while we have time for it and
		// we have free space in task OUTPUT buffer.
		// When we stop processing the data lport stop sends ACK back to FPGA
		// and process to be halted.
		int n ;
		u_int dataAmount = 0 ;
		for (n=0; (pSched->getMyTimeLeftMs() > 10) && (dataAmount <= MMI_MPO_FPGA_QUAD_PACKET_LEN * 8) ; n++) {
#if 0
			// Read data block from file, then send it over LPORT1
			// to LPORT0
			nBytes = read(fd, outArray, 128) ;
			DTRACE("Read from file: fd=0x%x %i bytes.\n", fd, nBytes) ;
			if (nBytes > 0) {
				pWLPort->send(outArray, nBytes) ;
				DTRACE("Sent -> %i bytes of data over %s.\n", nBytes, WRITER_FILE_DEV_PATH) ;
			} else {
				// If we reach end of file, then rewind fd to the beginning
				DTRACE("Seek fd=0x%x to beginning.\n", fd) ;
				lseek(fd, 0, SEEK_SET) ;
			}
#endif

			if ( (nBytes = pRLPort->recv(inpArray, sizeof(inpArray))) < 0 ) {
				break ;
			}
#ifdef DEBUG
			DTRACE("Received <- %i bytes from %s.\n", nBytes, READER_FILE_DEV_PATH) ;
			print_hex_dump(inpArray, nBytes) ;
#endif
			for (int i = 0; i < nBytes; i++) {
				int status = 0 ;

				status = pFPGAParser->feed(inpArray[i]) ;

				/*
				 * Transmit FPGA quadratures packet if it is ready.
				 */
				void *pPacket ;
				int len ;
				if (status == CMMIComposer::STATUS_FPGA_QUAD_PACKET_IS_READY ) {

					while ( (pPacket = pFPGAParser->getMMIComposerPtr()->getFPGAQuadPacketPtr()) != NULL) {
						len = pFPGAParser->getMMIComposerPtr()->getFPGAQuadPacketLen() ;

						//
						// Exit from task when packet length is not
						// as expected.\n
						//
						if (len != MMI_MPO_FPGA_QUAD_PACKET_LEN) {
							pMachine->writeCtlSym(CSysMpoMachine::csErr) ;
							TRACE("Inernal ERROR: quad-packet-len=%i, but expected %i\n", len, MMI_MPO_FPGA_QUAD_PACKET_LEN) ;
							return 0 ;
						}

						VPRINTF("AppTransmit: Sending QUAD packet -> MZU, len=%i, CRC16=0x%x\n", len, ((u_short*)pPacket)[(len/sizeof(u_short))-1]) ;
						DTRACE("pPacket=0x%x\n", pPacket) ;

						pSysSwIO->putMyPacket(pPacket,	len) ;
						pFPGAParser->getMMIComposerPtr()->clearFPGAQuadPacket() ;

						dataAmount += MMI_MPO_FPGA_QUAD_PACKET_LEN ;
					}
				} else if (status == CMMIComposer::STATUS_AIS_INF_PACKET_IS_READY) {

					while ( (pPacket = pFPGAParser->getMMIComposerPtr()->getAISInfPacketPtr()) != NULL) {
						len = pFPGAParser->getMMIComposerPtr()->getAISInfPacketLen() ;

						//
						// Exit from task when packet length is not
						// as expected.\n
						//
						if (len != MMI_MPO_AIS_INF_PACKET_LEN) {
							pMachine->writeCtlSym(CSysMpoMachine::csErr) ;
							TRACE("Inernal ERROR: inf-packet-len=%i, but expected %i\n", len, MMI_MPO_AIS_INF_PACKET_LEN) ;
							return 0 ;
						}

						VPRINTF("TransmitDec: Sending INF packet -> MZU, len=%i, CRC16=0x%x\n", len, ((u_short*)pPacket)[(len/sizeof(u_short))-1]) ;
						DTRACE("pPacket=0x%x\n", pPacket) ;

						pSysSwIO->putMyPacket(pFPGAParser->getMMIComposerPtr()->getAISInfPacketPtr(), len) ;
						pFPGAParser->getMMIComposerPtr()->clearAISInfPacket() ;

						dataAmount += MMI_MPO_FPGA_QUAD_PACKET_LEN ;
					}
				}
			}
		}

		DTRACE("Received %i x 128 bytes data.\n", n) ;

		pSched->waitForMyTime() ;

	} // End of while() main loop ;
	return 0 ;
}
