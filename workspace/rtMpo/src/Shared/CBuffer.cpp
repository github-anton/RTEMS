/*
 * CBuffer.cpp
 *
 *  Created on: May 27, 2020
 *      Author: anton
 */

#include "CBuffer.h"
#include <stdio.h>
#include <auxio.h>

CBuffer::CBuffer(size_t len) {
	wIdx = 0 ;
	pArea = new u_char[len] ;
	size = len ;
}

CBuffer::~CBuffer() {

	delete pArea ;
}

// off > 0 - offset gets from beginning of buffer
// off < 0 - offset gets from end of buffer
size_t CBuffer::calcIdx(int off) {
	size_t idx ;

	if (off >= 0)
		idx = off ;
	else if (off < 0)
		idx = wIdx + off ;

	return idx ;
}

int CBuffer::add(void *part, size_t len) {

	// Check if we have enough free space
	// to not allow buffer overflow and
	// to support atomic operation
	if ( len > getAvail() ) {
		return -1 ;
	}

	// Add a data chunk to the data area
	for (size_t i = 0; i < len ; i++) {
		pArea[wIdx+i] = ((u_char*)part)[i] ;
	}
	wIdx += len ;

	return 0 ;
}

void CBuffer::clear() {
	wIdx = 0 ;
}

void CBuffer::printHexDump() {
	size_t i ;

/*	for( i = 0; i < wIdx; i++ ) {
		printf("%02x ", pArea[i]) ;
		if (!(i % 35 ) && i ) {
			printf("\n");
		}
	}
	if ( i % 35 ) {
		printf("\n");
	}*/

	print_hex_dump(pArea, wIdx) ;
}

void CBuffer::printBinDump() {
	/*size_t i ;

	for ( i = 0; i < wIdx; i++) {
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
	}*/

	print_bin_dump(pArea, wIdx) ;
}

size_t CBuffer::getLastIdx() {

	return wIdx - 1;
}

size_t CBuffer::getFill() {

	return wIdx ;
}

size_t CBuffer::getSize() {

	return size ;
}

size_t CBuffer::getAvail() {

	int avail = size - wIdx ;

	// If wIdx is out of range and too big
	// we just return 0
	if (avail < 0) {
		return 0 ;
	}

	return avail ;
}

int CBuffer::add(u_char byte) {

	if (size < wIdx + sizeof(byte)) {
		return -1 ;
	}

	// Add one byte to the end of
	// filled area.
	pArea[wIdx++] = byte ;

	return 0 ;
}

int CBuffer::add(u_short word) {

	if (size < wIdx + sizeof(word)) {
		return -1 ;
	}

	for (u_int i = 0; i < sizeof(word); i++) {
		pArea[wIdx++] = ((u_char*)&word)[i] ;
	}

	return 0 ;
}

int CBuffer::add(u_int dword) {

	if (size < wIdx + sizeof(dword)) {
		return -1 ;
	}

	for (u_int i = 0; i < sizeof(dword); i++) {
		pArea[wIdx++] = ((u_char*)&dword)[i] ;
	}

	return 0 ;
}

u_char *CBuffer::getPtr(size_t off) {

	return &pArea[calcIdx(off)] ;
}

u_char CBuffer::getByte(size_t idx) {

	return pArea[calcIdx(idx)] ;
}

int CBuffer::read(size_t off, void *array, size_t len) {
	size_t i ;

	for ( i = 0; i < len; i++) {
		((u_char*)array)[i] = pArea[off + i] ;
	}

	return i ;
}

/*
 * Trim data buffer tail which is greater than idx
 * if "idx" < 0, then it counts down from the end of
 * data.
 */
void CBuffer::trim(int idx) {

	wIdx = calcIdx(idx) ;
}
