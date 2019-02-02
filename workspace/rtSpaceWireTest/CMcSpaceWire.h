/*
 * CMcSpaceWire.h
 *
 *  Created on: 28 нояб. 2017 г.
 *      Author: anton
 */

#ifndef CMCSPACEWIRE_H_
#define CMCSPACEWIRE_H_

#include <sys/types.h>
<<<<<<< HEAD
#include "CSpaceWire.h"

class CMcSpaceWire: public CSpaceWire {
=======

class CMcSpaceWire {
>>>>>>> 212e3b0a2a9fecda746bc7803e945f3a46a1a98a
private:
	int fd ;
	bool ok ;
	u_char myAddr ;
	u_char *pOutBuf ;
	u_char *pInpBuf ;
	size_t addrLen ;
<<<<<<< HEAD
	size_t maxPacketLen ;
    
public:
	CMcSpaceWire( const char *strPath, size_t maxPacketLen, size_t addrLen );
=======
	size_t maxCargoLen ;
public:
	CMcSpaceWire( const char *strPath, size_t addrLen );
>>>>>>> 212e3b0a2a9fecda746bc7803e945f3a46a1a98a
	virtual ~CMcSpaceWire();
	bool isOk() ;
	bool isConnected() ;
	int sendTo(u_char addr[], void *pData, size_t len) ;
	int recv(void *pData, size_t len) ;
<<<<<<< HEAD
    int selectChannel(u_int chNo) ;
	void setBlocked(bool flag) ;
    u_int getTimeCode() ;
    u_int waitTimeCode() ;
    int setTxSpeed(u_int MbitPs) ;
    int getRxSpeed() ;
=======
	size_t getMaxCargoLen() ;
>>>>>>> 212e3b0a2a9fecda746bc7803e945f3a46a1a98a
};

#define SPACEWIRE_MAX_PACKET_LEN 	4096

#endif /* CMCSPACEWIRE_H_ */
