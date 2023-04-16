/*
 * CPushStack.h
 *
 *  Created on: Jan 20, 2020
 *      Author: anton
 */

#ifndef CPUSHSTACK_H_
#define CPUSHSTACK_H_

#include <sys/types.h>
#include "CStack.h"

class CPushStack: public CStack {
private:
	char bottom ;
	
public:
	CPushStack(char bottom, int depth);
	virtual ~CPushStack();
	virtual int push(char sym) ;
	virtual int pop() ;
	virtual char getTop() ;
	virtual void clear() ;
};

#endif /* CPUSHSTACK_H_ */
