/*
 * CPushStack.cpp
 *
 *  Created on: Jan 20, 2020
 *      Author: anton
 */

#include "CPushStack.h"

CPushStack::CPushStack(char bottom, int depth) : CStack(depth) {
    
	this->bottom = bottom ;
}

CPushStack::~CPushStack() {
	// TODO Auto-generated destructor stub
}

int CPushStack::push(char sym) {

	return CStack::push(&sym, sizeof(sym)) ;
}

int CPushStack::pop() {
	char sym ;

	return CStack::pop(&sym, sizeof(char)) ;
}

char CPushStack::getTop() {
	char sym ;

	if (!CStack::getFill()) {
        
		return	bottom ;
	}
        
    readTop(&sym, sizeof(sym)) ;

	return sym ;
}

void CPushStack::clear() {

	CStack::clear() ;
}
