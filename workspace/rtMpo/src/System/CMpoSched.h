/*
 * CMpoSched.h
 *
 *  Created on: Jul 9, 2019
 *      Author: anton
 */

#ifndef CMPOSCHED_H_
#define CMPOSCHED_H_

#include "CARINCSched.h"

class CMpoSched: public CARINCSched {
public:
	static CARINCSched::TTaskDescRow	taskDesc[] ;
	static size_t taskDescSize ;

	static CARINCSched::TProfileRow	initProfile[] ;
	static size_t initProfileSize ;
	static char initProfileName[] ;

	static CARINCSched::TProfileRow	waitProfile[] ;
	static size_t	waitProfileSize ;
	static char waitProfileName[] ;

	static CARINCSched::TProfileRow	trRawProfile[] ;
	static size_t trRawProfileSize ;
	static char trRawProfileName[] ;

	static CARINCSched::TProfileRow	trDecProfile[] ;
	static size_t trDecProfileSize ;
	static char trDecProfileName[] ;

	static CARINCSched::TProfileRow	servProfile[] ;
	static size_t servProfileSize ;
	static char servProfileName[] ;

	static CARINCSched::TProfileRow	errorProfile[] ;
	static size_t errorProfileSize ;
	static char errorProfileName[] ;

	CMpoSched();
	virtual ~CMpoSched();
};

#endif /* CMPOSCHED_H_ */
