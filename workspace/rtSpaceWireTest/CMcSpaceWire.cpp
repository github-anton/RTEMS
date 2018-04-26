/*
 * CMcSpaceWire.cpp
 *
 *  Created on: 28 нояб. 2017 г.
 *      Author: anton
 */

#if 1
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

CMcSpaceWire::CMcSpaceWire(const char *strDev, size_t addrLen) {
	ok = false ;
	this->addrLen = addrLen ;
	maxCargoLen = SPACEWIRE_MAX_PACKET_LEN - addrLen ;
	pOutBuf = new u_char [SPACEWIRE_MAX_PACKET_LEN] ;
	pInpBuf = new u_char [SPACEWIRE_MAX_PACKET_LEN] ;
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
	if (addrLen != 0) {
		memcpy(&pOutBuf[0], addr, addrLen) ;
		packetLen += addrLen ;
	}
	if (len != 0) {
		memcpy(&pOutBuf[addrLen], pData, len) ;
		packetLen += len ;
	}
	int nBytes = write(fd, pOutBuf, packetLen) ;
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

	/* Since heading bytes was deleted by routers we do not need to read
	 * path addr */
	if( nBytes > 0 ) {
		memcpy(pData, pInpBuf, nBytes) ;
	}
	return nBytes ;
}

size_t CMcSpaceWire::getMaxCargoLen() {
	return maxCargoLen ;
}
