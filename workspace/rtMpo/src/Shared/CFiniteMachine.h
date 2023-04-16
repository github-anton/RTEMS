/*
 * CMpoMachine.h
 *
 *  Created on: Feb 7, 2019
 *      Author: anton
 */

#ifndef CFINITEMACHINE_H_
#define CFINITEMACHINE_H_

#include <sys/types.h>
#include <pthread.h>

#include "CRingBuffer.h"

class CFiniteMachine {
public:
	typedef int TState ;
	typedef int TSym ;

	struct TBranch {
		TState new_state;
		void (*Proc)(void *) ;
	} ;

protected:
	TBranch *pStateTable ;	// Pointer to the state table
	TState state ;			// Current state

private:
	int syms ; 				// Quantity of the input symbols
	pthread_t threadId ;	// Working thread
	CRingBuffer	*pSymQueue ;	// Input symbols Queue

	CFiniteMachine::TSym readCtlSym() ;

public:
	CFiniteMachine(/*CSched *pSched*/void *arg);
	virtual void setCtlSymsQty(size_t syms) ;
	virtual ~CFiniteMachine();
	virtual int step(/*CSched *pSched*/void *arg) ;
	/*virtual int run(CSched *pSched) ;			// Working proc*/
	virtual void writeCtlSym(CFiniteMachine::TSym sym) ;
	virtual TState getState() ;
};

#endif /* CFINITEMACHINE_H_ */
