/*
 * CMpoSched.cpp
 *
 *  Created on: Jul 9, 2019
 *      Author: Anton Ermakov
 */

#include "CMpoSched.h"

#include "../Tasks.h"

CARINCSched::TTaskDescRow CMpoSched::taskDesc[] = {
//		NO,	INIT,					DESCRIPTION
		{0, NULL, 					"No Task"				},
		{1, CSysSwIO::init,			"SpaceWire IO service"	},
		{2, CSysMpoMachine::init,	"Finite State Machine"	},
		{3, CSysInit::init,			"Common System Init"	},
		{4, CAppTransmit::init,		"Raw Transmission"		},
		{5, CAppService::init,		"Service Mode"			}
} ;

size_t CMpoSched::taskDescSize = sizeof(taskDesc) ;

CARINCSched::TProfileRow	CMpoSched::initProfile[] = {
//		NO,	CALL_INIT,	CALL_RUN,	DESTROY_AFTER	BT,	ET
		{ 1, true,		true,		false,			0,	25  },
		{ 2, true,		true,		false,			25,	50 },
		{ 3, true,		true,		false,			50, 60 },
		{ 4, true,		false,		false,			60, 70 },
		{ 5, true,		false,		false,			70, 80 }
} ;

size_t CMpoSched::initProfileSize = sizeof(CMpoSched::initProfile) ;
char CMpoSched::initProfileName[] = "Initialize" ;

CARINCSched::TProfileRow CMpoSched::waitProfile[] = {
//		NO,	CALL_INIT,	CALL_RUN,	DESTROY_AFTER	BT,	ET
		{ 1, false,		true,		false,			0,	10 },
		{ 2, false,		true,		false,			10,	20},
} ;

size_t CMpoSched::waitProfileSize = sizeof(CMpoSched::waitProfile) ;
char CMpoSched::waitProfileName[] = "Wait" ;

CARINCSched::TProfileRow CMpoSched::trRawProfile[] = {
//		NO,	CALL_INIT,	CALL_RUN,	DESTROY_AFTER	BT,	ET
		{ 1, false,		true,		false,			0,	10 },
		{ 2, false,		true,		false,			10,	20 },
		{ 4, false,		true,		false,			10,	60 }
} ;

size_t CMpoSched::trRawProfileSize = sizeof(CMpoSched::trRawProfile) ;
char CMpoSched::trRawProfileName[] = "Transmit_Raw" ;

CARINCSched::TProfileRow CMpoSched::trDecProfile[] = {
//		NO,	CALL_INIT,	CALL_RUN,	DESTROY_AFTER	BT,	ET
		{ 1, false,		true,		false,			0,	10 },
		{ 2, false,		true,		false,			10,	20 },
		{ 4, false,		true,		false,			20,	60 }
} ;

size_t CMpoSched::trDecProfileSize = sizeof(CMpoSched::trDecProfile) ;
char CMpoSched::trDecProfileName[] = "Transmit_Decoded" ;

CARINCSched::TProfileRow CMpoSched::servProfile[] = {
//		NO,	CALL_INIT,	CALL_RUN,	DESTROY_AFTER	BT,	ET
		{ 1, false,		true,		false,			0,	10 },
		{ 2, false,		true,		false,			10,	20 },
		{ 5, false,		true,		false,			20,	60 }
} ;

size_t CMpoSched::servProfileSize = sizeof(CMpoSched::servProfile) ;
char CMpoSched::servProfileName[] = "Service" ;

CARINCSched::TProfileRow	CMpoSched::errorProfile[] = {
//		NO,	CALL_INIT,	CALL_RUN,	DESTROY_AFTER	BT,	ET
		{ 1, false,		true,		false,			0,	10 },
		{ 2, false,		true,		false,			10,	20 }
} ;

size_t CMpoSched::errorProfileSize = sizeof(CMpoSched::errorProfile);
char CMpoSched::errorProfileName[] = "Error" ;

CMpoSched::CMpoSched()
: CARINCSched(taskDesc, taskDescSize/sizeof(CARINCSched::TTaskDescRow)) {
	// TODO Auto-generated constructor stub
}

CMpoSched::~CMpoSched() {
	// TODO Auto-generated destructor stub
}
