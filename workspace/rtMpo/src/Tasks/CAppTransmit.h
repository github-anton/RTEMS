/*
 * CAppTransmitRaw.h
 *
 *  Created on: Jul 25, 2019
 *      Author: anton
 */

#ifndef TASKS_CAPPTRANSMITRAW_H_
#define TASKS_CAPPTRANSMITRAW_H_

#include "../System/CARINCSched.h"
#include <CTask.h>
#include "CSysSwIO.h"
#include "CSysMpoMachine.h"
#include <Mfbsp/CLPort.h>
#include <CFPGA.h>
#include <memory>

#include "../Shared/CFPGAPacketParser.h"
#include "../Shared/CMMIComposer.h"

class CAppTransmit: public CTask<CAppTransmit> {
private:
	CSysSwIO *pSysSwIO ;
	CFiniteMachine *pMachine ;
	int state ;		// Current state
	enum states {STATE_WAIT_RQ = 0, STATE_SEND_DATA = 1} ;
	int amount ;	// Amount of packets to be transmitted to MZU
	CFPGAPacketParser	*pFPGAParser ;
	CLPort	*pRLPort ;
	CLPort	*pWLPort ;
	int fd ;
	std::shared_ptr<CFPGA> spFPGA;

public:
	CAppTransmit(CARINCSched *pSched);
	virtual ~CAppTransmit();
	virtual int run(CARINCSched *pSched) ;
};

#endif /* TASKS_CAPPTRANSMITRAW_H_ */
