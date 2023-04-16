
/*
 * CMpoMachine.cpp
 *
 *  Created on: Feb 25, 2019
 *      Author: anton
 */
#if 0
	#define DEBUG
#endif

#if 1
	#define VERBOSE
#endif

#include "../Tasks/CSysMpoMachine.h"
#include <auxio.h>
#include "../System/CMpoSched.h"
#include "CSysSwIO.h"

CFiniteMachine::TBranch CSysMpoMachine::state_table[][csMax] = {
	/*					cs0		csInit		 		 csWait		   		  csTrRaw	   		 	 csTrDec			    csService			csErr 		   		csOk*/
	/* 0, st0		*/ {{0, 0}, {0, 0},	   	 		 {0, 0},	   		  {0, 0},			 	 {0, 0},			    {0, 0},				{0, 0}, 	   		{0, 0}},
	/* 1, stInit 	*/ {{0, 0}, {stInit, 0}, 		 {stInit, 0},  		  {stInit, 0},	   	  	 {stInit, 0},	 	    {stInit, 0},		{stFail, startFail},{stWait, startWait}/*{stTrRaw, startTrRaw}*/},
	/* 2, stWait	*/ {{0, 0}, {stInit, startInit}, {stWait, 0},  		  {stTrRaw, startTrRaw}, {stTrDec, startTrDec}, {stServ, startServ},{stFail, startFail},{stWait, startWait}},
	/* 3, stTrRaw	*/ {{0, 0}, {stTrRaw, 0}, 		 {stWait, startWait}, {stTrRaw, 0},   	  	 {stTrRaw, 0},   	    {stTrRaw, 0},		{stFail, startFail},{stWait, startWait}},
	/* 4, stTrDec	*/ {{0, 0}, {stTrDec, 0}, 		 {stWait, startWait}, {stTrDec, 0}, 		 {stTrDec, 0}, 	     	{stTrDec, 0},		{stFail, startFail},{stTrDec, startWait}},
	/* 5, stServ	*/ {{0, 0}, {stServ, 0}, 		 {stWait, startWait}, {stServ, 0}, 		  	 {stServ, 0}, 	 	    {stServ, 0},		{stFail, startFail},{stServ, startWait}},
	/* 6, stFail 	*/ {{0, 0}, {stInit, startInit}, {stFail, 0},  		  {stFail, 0}, 		  	 {stFail, 0},   		{stFail, 0},		{stFail, 0},		{stFail, 0}}
	};

CSysMpoMachine::CSysMpoMachine(CARINCSched *pSched)
: CFiniteMachine(pSched) {
	pStateTable = &state_table[0][0] ;
	setCtlSymsQty(csMax) ;
	state = stInit ;
}

CSysMpoMachine::~CSysMpoMachine() {
	// TODO Auto-generated destructor stub
}

/*
 * Printing from this routine will cause to
 * hangs program on!!!
 */
int CSysMpoMachine::run(CARINCSched *pSched) {

	CSysSwIO *pSysSwIO = (CSysSwIO*)pSched->getTaskStatusRow(CSysSwIO::init)->pTask ;
	u_char packet[SW_MAX_OUT_PACKET_LEN] ;
	mmi_mpo_status_t *pRep = (mmi_mpo_status_t*)packet ;

	DTRACE("pSysSwIO = 0x%x\n\r", (unsigned)pSysSwIO) ;
	VPRINTF("SysMpoMachine: STARTED\n") ;

	while(true) {

		// Take control symbol from SW packet
		// and feed it to Machine
		int len = pSysSwIO->getMyPacket(packet, sizeof(packet)) ;

		bool mmi_command_received = false ;
		if (len > 0) {
			int msgId = CSysSwIO::getMessageId(packet) ;
			TSym sym ;
			int ret ;
			switch(msgId) {
			case MMI_CMD_MPO_SETMODE_MSG_ID:
				sym = getPacketCtlSym(packet) ;
				VPRINTF("SysMpoMachine: received <- ctlSym=%s\n", strCtlSym(sym)) ;
				writeCtlSym(sym) ;
				mmi_command_received = true ;
				break ;
			case MMI_CMD_MPO_STATUSRQ_MSG_ID:
				pRep->hdr0.msg_id = MMI_MPO_STATUS_MSG_ID ;
				pRep->state = getState() ;
				pSysSwIO->putMyPacket(packet, sizeof(mmi_mpo_status_t)) ;
				break ;
			}

		}

		TState st0 = getState() ;

		// Call parent run() method to process
		// control symbol on input
		step(pSched) ;
		TState st1 = getState() ;
		DTRACE("State: %s -> %s\n", strState(st0), strState(st1)) ;

#if defined(VERBOSE) && !defined(DEBUG)
		// Show state transition
		if (st0 != st1) {
			printf("SysMpoMachine: %s -> %s\n", strState(st0), strState(st1)) ;
		}
#endif

		// if machine went from one state to another, then
		// send notification to MZU
		if( mmi_command_received ) {
			pRep->hdr0.msg_id = MMI_MPO_STATUS_MSG_ID ;
			pRep->state = st1 ;
			pSysSwIO->putMyPacket(packet, sizeof(mmi_mpo_status_t)) ;
		}

		pSched->waitForMyTime() ;
	} // End of while()

	return 0 ;
}

CSysMpoMachine::TSym CSysMpoMachine::getPacketCtlSym(void *mpoPacket) {
	mmi_cmd_mpo_setmode_t *p = (mmi_cmd_mpo_setmode_t*)mpoPacket ;
	CFiniteMachine::TSym sym = p->mode_id ;
	DTRACE("packet->mode_id=0x%lx, ctlSym=0x%x\n", p->mode_id, sym) ;
	return sym ;
}

const char *CSysMpoMachine::strState(int state) {
	// Index table of text states
	static const char *pStrState[] = {
			"0",
			"INIT",
			"WAIT",
			"TRRAW",
			"TRDEC",
			"SERV",
			"FAIL"
	};
	// An unknown state for out of range index
	static const char *strStateUnknown = "UNKNOWN" ;

	// If state out of range, then return string with
	// state UNKNOWN.
	if (state < st0 || state > stMax ) {
		return strStateUnknown ;
	}

	// Return normal state
	return pStrState[state] ;
}

const char *CSysMpoMachine::strCtlSym(int ctlSym) {
	// Index table of text states
	static const char *pStrCtlSym[] = {
			"0",
			"INIT",
			"WAIT",
			"TRRAW",
			"TRDEC",
			"SERV",
			"ERR",
			"OK"
	};
	// An unknown state for out of range index
	static char strCtlSymUnknown[32] ;

	// If state out of range, then return string with
	// ctlSym UNKNOWN.
	if (ctlSym < cs0 || ctlSym > csMax ) {
		VPRINTF(strCtlSymUnknown, "UNKNOWN: 0x%x", ctlSym) ;
		return strCtlSymUnknown ;
	}

	// Return normal state
	return pStrCtlSym[ctlSym] ;
}

/*
 * Init state processing.
 */
void CSysMpoMachine::startInit(void *arg) {

	DTRACE("Select initProfile for the scheduler\n\r") ;

	// Switch MpoSched to initProfile
	CARINCSched *pSched = (CARINCSched*)arg ;
	pSched->setProfile(CMpoSched::initProfile, CMpoSched::initProfileSize/sizeof(CARINCSched::TProfileRow), CMpoSched::initProfileName) ;
}

/*
 * Wait state processing.
 */
void CSysMpoMachine::startWait(void *arg) {

	DTRACE("Select waitProfile for the scheduler\n\r") ;

	// Switch MpoSched to waitProfile
	CARINCSched *pSched = (CARINCSched*)arg ;
	pSched->setProfile(CMpoSched::waitProfile, CMpoSched::waitProfileSize/sizeof(CARINCSched::TProfileRow), CMpoSched::waitProfileName) ;
}

/*
 * TrDecoded state processing.
 */
void CSysMpoMachine::startTrDec(void *arg) {

	DTRACE("Select trDecProfile for the scheduler\n\r") ;

	// Switch MpoSched to trDecProfile
	CARINCSched *pSched = (CARINCSched*)arg ;
	pSched->setProfile(CMpoSched::trDecProfile, CMpoSched::trDecProfileSize/sizeof(CARINCSched::TProfileRow), CMpoSched::trDecProfileName) ;
}

/*
 * TrRaw state processing.
 */
void CSysMpoMachine::startTrRaw(void *arg) {

	DTRACE("Select trRawProfile for the scheduler\n\r") ;

	// Switch MpoSched to trRawProfile
	CARINCSched *pSched = (CARINCSched*)arg ;
	pSched->setProfile(CMpoSched::trRawProfile, CMpoSched::trRawProfileSize/sizeof(CARINCSched::TProfileRow), CMpoSched::trRawProfileName) ;
}

/*
 * Service state processing.
 */
void CSysMpoMachine::startServ(void *arg) {

	DTRACE("Select servProfile for the scheduler\n\r") ;

	// Switch MpoSched to servProfile.
	CARINCSched *pSched = (CARINCSched*)arg ;
	pSched->setProfile(CMpoSched::servProfile, CMpoSched::servProfileSize/sizeof(CARINCSched::TProfileRow), CMpoSched::servProfileName) ;
}

/*
 * Fail state processing.
 */
void CSysMpoMachine::startFail(void *arg) {

	DTRACE("Select errorProfile for the scheduler\n\r") ;

	// Switch MpoSched to servProfile.
	CARINCSched *pSched = (CARINCSched*)arg ;
	pSched->setProfile(CMpoSched::errorProfile, CMpoSched::errorProfileSize/sizeof(CARINCSched::TProfileRow), CMpoSched::errorProfileName) ;
}
