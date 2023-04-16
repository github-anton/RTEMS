/*
 * CWndBuf.cpp
 *
 *  Created on: May 29, 2020
 *      Author: anton
 */

#include "CWndBuffer.h"

#include <string.h>
#include <stdio.h>

CWndBuffer::CWndBuffer(size_t wide) {

	this->wide = wide ;
	pArea = new u_char[wide] ;
	memset(pArea, 0, wide) ;
}

CWndBuffer::~CWndBuffer() {

	delete pArea ;
}

void CWndBuffer::add(u_char byte) {
	size_t i ;

	for (i = 0; i < wide-1 ; i++) {
		pArea[i] = pArea[i+1] ;
	}

	pArea[i] = byte ;
}

// off>0 - offset from beginning of buffer
// off<0 - offset from end of buffer
size_t CWndBuffer::calcIdx(int off) {
	size_t idx ;

	if (off >= 0)
		idx = off ;
	else if (off < 0)
		idx = wide + off ;

	return idx ;
}

u_char CWndBuffer::getByte(int off) {

	return pArea[calcIdx(off)] ;
}

u_short CWndBuffer::getWord(int off) {

	return *(u_short*)&pArea[calcIdx(off)] ;
}

u_int CWndBuffer::getDWord(int off) {

	return *(u_int*)&pArea[calcIdx(off)] ;
}

/*
void CWndBuffer::printHexDump() {
	size_t i ;

	for( i = 0; i < wide; i++ ) {
		printf("%02x ", pArea[i]) ;
		if (!(i % 40 ) && i ) {
			printf("\n");
		}
	}
	if ( i % 40 ) {
		printf("\n");
	}
}

void CWndBuffer::printBinDump() {
	size_t i ;

	for ( i = 0; i < wide; i++) {
		for (size_t j = 0; j < sizeof(u_char)*8; j++) {
			printf("%i", (pArea[i] >> (sizeof(u_char)*8 - 1 - j)) & 0x1 ) ;
		}
		if ( !(i % 11) && i ) {
			printf ("\n") ;
		} else {
			printf(" ") ;
		}
	}
	if ( i % 11 ) {
		printf("\n");
	}
}*/
