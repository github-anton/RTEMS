/*
 * CStack.cpp
 *
 *  Created on: Jan 20, 2020
 *      Author: anton
 */

#if 1
#	define DEBUG
#endif

#include <string.h>
#include <stdlib.h>
#include "CStack.h"
#include "auxio.h"
#include "auxmath.h"

CStack::CStack(int depth) {
	pArea = new u_char[depth];
	topIdx = 0 ;
	this->depth = depth ;
}

CStack::~CStack() {
	topIdx = 0 ;
	delete pArea ;
}

int CStack::push(void *pData, size_t len) {

	if (len > getAvail()) {
		DTRACE("WARNING: stack overflow!\n") ;
		return 0 ;
	}

	for (int i = 0; i < len; i++) {
		pArea[topIdx+i] = ((u_char*)pData)[i] ;
	}

	topIdx += len ;
	
	return len ;
}

int CStack::pop(void *pData, size_t len) {

	int acc_len = MIN(len, getFill()) ;
	topIdx -= acc_len ;

#if defined(DEBUG)
	if(acc_len < len) {
		TRACE("WARNIG: stack has less data than was asked!\n") ;
	}
#endif

	for (int i = 0; i < acc_len; i++) {
		((u_char*)pData)[i] = pArea[topIdx+i] ;
	}

	return acc_len ;
}

int CStack::pop(size_t len) {
    
	len = MIN(len, getFill()) ;
	topIdx -= len ;
    
	return len ;
}

int CStack::readTop(void *pData, size_t len) {

	//memcpy( pData, &pArea[topIdx-len], len) ;
    for (int i = 0; i < len; i++) {
        ((u_char*)pData)[i] = pArea[topIdx-len+i] ;
    }

	return len ;
}

int CStack::getFill() {
	
	return topIdx ;
}

int CStack::getAvail() {

	return (depth - topIdx) ;
}

void CStack::clear() {

	topIdx = 0 ;
}
