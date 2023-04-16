/*
 * CSysInit.cpp
 *
 *  Created on: Jul 25, 2019
 *      Author: Ermakov Anton
 */

#include "../Tasks/CSysInit.h"
#include "../Tasks.h"

#if 1
	#define VERBOSE
#endif

#if 0
	#define DEBUG
#endif

#include <auxio.h>
#include <auxtimer.h>

#define FPGA_BOOT_TIMEOUT_MS		10000

crc16_context_t	crc16_arc_context ;

CSysInit::CSysInit(CARINCSched *pSched) {

	//
	// Create FPGA object instance.
	//
	pFPGA = new CFPGA ;
}

CSysInit::~CSysInit() {

	//
	// Destroy existing FPGA object.
	//
	delete pFPGA ;
}

int CSysInit::run(CARINCSched *pSched) {

	CFiniteMachine *pMachine = (CSysMpoMachine*)pSched->getTaskStatusRow(CSysMpoMachine::init)->pTask ;
	DTRACE("BEGIN: pMachine=0x%x\n", (unsigned)pMachine) ;


#if 1
	//
	// Begin initialization. Try to boot FPGA.
	//
	int i = 0 ;	// 0 - try boot from #0 flash, 1 - try boot from #1 flash
	do {
		int timePassedMs = 0 ;
		struct timespec uptime0, uptime1 ;

		DTRACE("Start booting FPGA%i, timeout=%i ms...\n", i, FPGA_BOOT_TIMEOUT_MS) ;
		if (pFPGA->startBoot(i) == -1 ) {
			VPRINTF("SysInit: Can't start FPGA boot process.\n") ;
			pMachine->writeCtlSym(CSysMpoMachine::csErr) ;
			return 0 ;
		}
		rtems_clock_get_uptime(&uptime0) ;
		//
		//	Wait while FPGA booting
		//
		do {
			if ( pFPGA->isBooted() ) {
				VPRINTF("SysInit: Booting FPGA%i \t\t\t...SUCCESSFUL (%i ms).\n", i, timePassedMs) ;
				break ;
			}
			pSched->waitForMyTime() ;
			rtems_clock_get_uptime(&uptime1) ;
			timePassedMs = (uptime1.tv_sec - uptime0.tv_sec)*1000.0
					+ (uptime1.tv_nsec - uptime0.tv_nsec) / 1000000.0 ;
			DTRACE("time-passed=%i ms\n", timePassedMs) ;

		// If timeout is exceeded, try to load another PROM
		} while ( timePassedMs < FPGA_BOOT_TIMEOUT_MS ) ;

#ifdef VERBOSE
		if (!pFPGA->isBooted()) {
			printf("SysInit: Booting FPGA%i \t\t\t...FAILED\n", i) ;
		}
#endif

		i++ ;
	} while ( (i < 2) & !pFPGA->isBooted() ) ;

	//
	//	If fail to load FPGA, then notify MPO machine about
	//  an error.
	//
	if ( !pFPGA->isBooted() ) {

		pMachine->writeCtlSym(CSysMpoMachine::csErr) ;
		return 0 ;
	}
#endif

	//
	// Initialize CRC16 algorithm.
	//
	VPRINTF("SysInit: Initialize CRC16 ARC algorithm	...") ;
	CRC16_ARC_CALC_TABLE(&crc16_arc_context) ;
	VPRINTF("DONE.\n") ;

	//
	// After Initialization is compete,
	// initiate SysMpoMachine to go to the next state
	//
	pMachine->writeCtlSym(CSysMpoMachine::csOk) ;

	return 0 ;
}

