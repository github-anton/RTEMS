/*
 * CAbstractSpaceWire.h
 *
 *  Created on: 31 янв. 2018 г.
 *      Author: anton
 */

#ifndef SRC_CSPACEWIRE_H_
#define SRC_CSPACEWIRE_H_

#include <sys/types.h>

class CSpaceWire {
public:
    CSpaceWire() ;
    virtual ~CSpaceWire() ;
	virtual int sendTo(u_char addr[], void *pData, size_t len) = 0 ;
	virtual int recv(void *pData, size_t len) = 0 ;
	virtual void setBlocked(bool flag) = 0 ;
    virtual bool isConnected() = 0 ;
    virtual u_int getTimeCode() = 0 ;
    virtual u_int waitTimeCode() = 0 ;
    virtual bool isOk() = 0 ;
    virtual int setTxSpeed(u_int MbitPs) = 0 ;
    virtual int getRxSpeed() = 0 ;
};

#endif /* SRC_CSPACEWIRE_H_ */
