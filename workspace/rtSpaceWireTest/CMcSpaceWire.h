/*
 * CMcSpaceWire.h
 *
 *  Created on: 28 нояб. 2017 г.
 *      Author: anton
 */

#ifndef CMCSPACEWIRE_H_
#define CMCSPACEWIRE_H_

#include <sys/types.h>
#include "CSpaceWire.h"

class CMcSpaceWire: public CSpaceWire {
private:
	int fd ;
	bool ok ;
	u_char myAddr ;
	u_char *pOutBuf ;
	u_char *pInpBuf ;
	size_t addrLen ;
	size_t maxPacketLen ;
    
public:
	CMcSpaceWire( const char *strPath, size_t maxPacketLen, size_t addrLen );
	virtual ~CMcSpaceWire();
	bool isOk() ;
	bool isConnected() ;
	int sendTo(u_char addr[], void *pData, size_t len) ;
	int recv(void *pData, size_t len) ;
    int selectChannel(u_int chNo) ;
	void setBlocked(bool flag) ;
    u_int getTimeCode() ;
    u_int waitTimeCode() ;
    int setTxSpeed(u_int MbitPs) ;
    int getRxSpeed() ;
};

#define SPACEWIRE_MAX_PACKET_LEN 	4096

#endif /* CMCSPACEWIRE_H_ */
