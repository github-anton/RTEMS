/*
 * CAppService.h
 *
 *  Created on: Jul 25, 2019
 *      Author: anton
 */

#ifndef TASKS_CAPPSERVICE_H_
#define TASKS_CAPPSERVICE_H_

#include "../System/CARINCSched.h"
#include <CTask.h>

class CAppService: public CTask<CAppService> {
public:
	CAppService(CARINCSched *pSched);
	virtual ~CAppService();
	virtual int run(CARINCSched *pSched) ;
};

#endif /* TASKS_CAPPSERVICE_H_ */
