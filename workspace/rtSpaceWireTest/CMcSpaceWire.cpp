/*
 * CMcSpaceWire.cpp
 *
 *  Created on: 28 нояб. 2017 г.
 *      Author: anton
 */

#if 0
#	define DEBUG
#endif

#include "CMcSpaceWire.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/ioctl.h>
#ifdef __linux__
#   include <asm/spw.h>
#else // RTEMS
#include <bsp.h>
#endif
#include <unistd.h>
#include "aux.h"

CMcSpaceWire::CMcSpaceWire(const char *strDev, size_t maxPacketLen, size_t addrLen) {
	ok = false ;
	this->addrLen = addrLen ;
    this->maxPacketLen = maxPacketLen ;
	pOutBuf = new u_char [this->maxPacketLen] ;
	pInpBuf = new u_char [this->maxPacketLen] ;
    fd = open(strDev, O_RDWR) ;
	if (fd == -1 ) {
		DTRACE("FATAL: Can't open %s: %s\n", strDev, strerror(errno)) ;
		return ;
	}
	ok = true ;
}

CMcSpaceWire::~CMcSpaceWire() {
	delete pOutBuf ;
	delete pInpBuf ;
}

bool CMcSpaceWire::isOk() {
	return ok ;
}

bool CMcSpaceWire::isConnected() {
	long connected = 0 ;
	connected = ioctl(fd, SPW_IS_CONNECTED ) ;
	if ( connected < 0 ) {
		DTRACE("ERR: ioctl() fail: returned %li\n", connected ) ;
		return false ;
	}
	return (connected != 0) ;
}

int CMcSpaceWire::sendTo(u_char addr[], void *pData, size_t len) {
	size_t packetLen = 0 ;
    int nBytes ;
    
    DTRACE("Called...\n") ;
    
	if (addrLen != 0) {
		memcpy(&pOutBuf[0], addr, addrLen) ;
		packetLen += addrLen ;
	}
	if (len != 0) {
		memcpy(&pOutBuf[addrLen], pData, len) ;
		packetLen += len ;
	}
	
	nBytes = write(fd, pOutBuf, packetLen) ;
	DTRACE("write() returned %i\n", nBytes) ;
	if( nBytes == -1 ){
		DTRACE ("ERR: Can't write data buffer: %s\n", strerror(errno)) ;
		return -1 ;
	}
	return nBytes - addrLen;
}

int CMcSpaceWire::recv(void *pData, size_t len) {
	int nBytes = read(fd, pInpBuf, SPACEWIRE_MAX_PACKET_LEN) ;
	DTRACE("read() returned %i\n", nBytes) ;
	if (nBytes == -1) {
		DTRACE("ERR: Can't read data from fd=%i: %s\n", fd, strerror(errno)) ;
		return -1 ;
	}

	if( nBytes > 0 ) {
        len = MIN(nBytes - addrLen, len) ;
		memcpy(pData, &pInpBuf[addrLen], nBytes - addrLen) ;
	} else {
        len = nBytes ;
    }
	return len ;
}

int CMcSpaceWire::selectChannel(u_int chNo) {
    return 0 ;
}

void CMcSpaceWire::setBlocked(bool flag) {
}    

u_int CMcSpaceWire::getTimeCode() {
    return ioctl(fd, SPW_GET_TIME_CODE) ;
}

u_int CMcSpaceWire::waitTimeCode() {
    return ioctl(fd, SPW_WAIT_TIME_CODE) ;
}

int CMcSpaceWire::setTxSpeed(u_int MbitPs) {
    return ioctl(fd, SPW_SET_TX_SPEED, &MbitPs) ;
}

int CMcSpaceWire::getRxSpeed() {
    return ioctl(fd, SPW_GET_RX_SPEED) ;
}
