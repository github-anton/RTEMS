
/*
 * CMpoMachine.cpp
 *
 *  Created on: Feb 7, 2019
 *      Author: anton
 */
#if 0
	#define DEBUG
#endif

#include "CFiniteMachine.h"
#include "auxio.h"

CFiniteMachine::CFiniteMachine(/*CSched *pSched*/void *arg) {
	DTRACE("Entering constructor...\n\r") ;

	pSymQueue = new CRingBuffer(sizeof(TSym)*10) ;
	pSymQueue->setBlock(false) ;
	pStateTable = NULL ;
	state = 0 ;
}

CFiniteMachine::~CFiniteMachine() {
	delete pSymQueue ;
}

int CFiniteMachine::step(void *arg) {
	int sym ;
	CFiniteMachine::TBranch branch ;

	sym = readCtlSym() ;
	DTRACE("Read sym=%i\n", sym) ;
	if ( sym >= 0 ) {
		branch = pStateTable[state*syms + sym] ;
		state = branch.new_state ;
		if(branch.Proc) {
			DTRACE("Call proc 0x%x\n", (unsigned)branch.Proc) ;
			branch.Proc(arg) ;
		}
	}
	DTRACE("state=%i\n", state) ;

	return 0 ;
}

/*
int CMachine::run(CSched *pSched) {

	while (true) {
		// Wait until task's time window begins
		pSched->waitForMyTime() ;

		step(pSched) ;
	}

	return 0 ;
}
*/

void CFiniteMachine::setCtlSymsQty(size_t syms) {
	this->syms = syms ;
}

void CFiniteMachine::writeCtlSym(CFiniteMachine::TSym sym) {
	DTRACE("Writing ctlSym=%i\n\r", sym) ;
	pSymQueue->put(&sym, sizeof(sym)) ;
}

CFiniteMachine::TSym CFiniteMachine::readCtlSym() {
	TSym sym ;

	if (pSymQueue->get(&sym, sizeof(TSym)) <= 0) {
		return -1 ;
	}
	return sym ;
}

CFiniteMachine::TState CFiniteMachine::getState() {

	return state ;
}
