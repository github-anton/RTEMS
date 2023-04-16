/*
 * CSysInit.h
 *
 *  Created on: Jul 25, 2019
 *      Author: anton
 */

#ifndef TASKS_CSYSINIT_H_
#define TASKS_CSYSINIT_H_

#include <CTask.h>
#include "../Shared/CFPGA.h"
#include <crc16.h>
#include "../System/CARINCSched.h"

extern crc16_context_t	crc16_arc_context ;

class CSysInit: public CTask<CSysInit> {
private:
	CFPGA *pFPGA ;

public:
	CSysInit(CARINCSched *pSched);
	virtual ~CSysInit();
	virtual int run(CARINCSched *pSched) ;
};

#endif /* TASKS_CSYSINIT_H_ */
