/*
 * mpo.cpp
 *
 *  Created on: Feb 19, 2019
 *      Author: anton
 */

#include <stdio.h>
#include <rtems/score/assert.h>
#include "System/CMpoSched.h"

int main(void) {

	//mips_break(0) ;
	//_Assert(0) ;

	// Create new dispatcher instance
	CARINCSched *pSched = new CMpoSched() ;

	// Set initial profile
	pSched->setProfile(CMpoSched::initProfile, CMpoSched::initProfileSize/sizeof(CARINCSched::TProfileRow), CMpoSched::initProfileName) ;

	// Set length of dispatcher tact
	pSched->setTactLenMs(100) ;

	// Start dispatcher
	pSched->start() ;

	return 0 ;
}
