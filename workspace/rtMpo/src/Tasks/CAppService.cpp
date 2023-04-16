/*
 * CAppService.cpp
 *
 *  Created on: Jul 25, 2019
 *      Author: anton
 */
#if 1
	#define DEBUG
#endif

#include <auxio.h>
#include "../Tasks/CAppService.h"

CAppService::CAppService(CARINCSched *pSched) {
	// TODO Auto-generated constructor stub

}

CAppService::~CAppService() {
	// TODO Auto-generated destructor stub
}

int CAppService::run(CARINCSched *pSched) {

	while(true) {
		pSched->waitForMyTime() ;

		DTRACE("Working!\n\r") ;
	}
	return 0 ;
}
