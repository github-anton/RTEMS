/*
 * CMpoMachine.h
 *
 *  Created on: Feb 25, 2019
 *      Author: anton
 */

#ifndef CMPOMACHINE_H_
#define CMPOMACHINE_H_

#include <CFiniteMachine.h>
#include <CTask.h>
#include "../System/CARINCSched.h"

class CSysMpoMachine: public CFiniteMachine, public CTask<CSysMpoMachine> {
public:
	enum states { st0 = 0, stInit = 1, stWait = 2, stTrRaw = 3, stTrDec= 4, stServ = 5, stFail = 6, stMax = 7  } ;
	enum inputs { cs0 = 0, csInit = 1, csWait = 2, csTrRaw = 3, csTrDecoded = 4, csService = 5, csErr = 6, csOk = 7, csMax = 8 } ;

private:
	static CFiniteMachine::TBranch state_table[][csMax] ;

public:
	CSysMpoMachine(CARINCSched *pSched);
	virtual ~CSysMpoMachine();
	virtual int run(CARINCSched *pSched) ;
	static CFiniteMachine::TSym getPacketCtlSym(void *mpoPacket) ;
	static const char *strState(int state) ;
	static const char *strCtlSym(int ctlSym) ;

	static void startInit(void *arg) ;
	static void startWait(void *arg) ;
	static void startTrDec(void *arg) ;
	static void startTrRaw(void *arg) ;
	static void startServ(void *arg) ;
	static void startFail(void *arg) ;
} ;

#endif /* CMPOMACHINE_H_ */
