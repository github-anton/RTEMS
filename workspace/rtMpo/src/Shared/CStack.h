/*
 * CStack.h
 *
 *  Created on: Jan 20, 2020
 *      Author: anton
 */

#ifndef CSTACK_H_
#define CSTACK_H_

#include <sys/types.h>

class CStack {

	u_char *pArea ;
	int topIdx ;
	int depth ;

public:
	CStack(int depth);
	virtual ~CStack();
	virtual int push (void *pData, size_t len) ;
	virtual int pop (void *pData, size_t len) ;
    virtual int pop(size_t len) ;
	virtual int readTop(void *pData, size_t len) ;
	virtual int getFill() ;
	virtual int getAvail() ;
	virtual void clear() ;
};

#endif /* CSTACK_H_ */
