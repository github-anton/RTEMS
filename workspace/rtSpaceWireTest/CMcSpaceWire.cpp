/*
 * CMcSpaceWire.cpp
 *
 *  Created on: 28 нояб. 2017 г.
 *      Author: anton
 */

<<<<<<< HEAD
#if 0
=======
#if 1
>>>>>>> 212e3b0a2a9fecda746bc7803e945f3a46a1a98a
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

<<<<<<< HEAD
CMcSpaceWire::CMcSpaceWire(const char *strDev, size_t maxPacketLen, size_t addrLen) {
	ok = false ;
	this->addrLen = addrLen ;
    this->maxPacketLen = maxPacketLen ;
	pOutBuf = new u_char [this->maxPacketLen] ;
	pInpBuf = new u_char [this->maxPacketLen] ;
=======
CMcSpaceWire::CMcSpaceWire(const char *strDev, size_t addrLen) {
	ok = false ;
	this->addrLen = addrLen ;
	maxCargoLen = SPACEWIRE_MAX_PACKET_LEN - addrLen ;
	pOutBuf = new u_char [SPACEWIRE_MAX_PACKET_LEN] ;
	pInpBuf = new u_char [SPACEWIRE_MAX_PACKET_LEN] ;
>>>>>>> 212e3b0a2a9fecda746bc7803e945f3a46a1a98a
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
<<<<<<< HEAD
    int nBytes ;
    
    DTRACE("Called...\n") ;
    
=======
>>>>>>> 212e3b0a2a9fecda746bc7803e945f3a46a1a98a
	if (addrLen != 0) {
		memcpy(&pOutBuf[0], addr, addrLen) ;
		packetLen += addrLen ;
	}
	if (len != 0) {
		memcpy(&pOutBuf[addrLen], pData, len) ;
		packetLen += len ;
	}
<<<<<<< HEAD
	
	nBytes = write(fd, pOutBuf, packetLen) ;
=======
	int nBytes = write(fd, pOutBuf, packetLen) ;
>>>>>>> 212e3b0a2a9fecda746bc7803e945f3a46a1a98a
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

<<<<<<< HEAD
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
=======
	/* Since heading bytes was deleted by routers we do not need to read
	 * path addr */
	if( nBytes > 0 ) {
		memcpy(pData, pInpBuf, nBytes) ;
	}
	return nBytes ;
}

size_t CMcSpaceWire::getMaxCargoLen() {
	return maxCargoLen ;
>>>>>>> 212e3b0a2a9fecda746bc7803e945f3a46a1a98a
}
