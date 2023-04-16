/*
 * CSched.cpp
 *
 *  Created on: Jul 5, 2019
 *      Author: Anton Ermakov
 */
#include "CARINCSched.h"

#include <rtems.h>
#include "../Tasks/CSysMpoMachine.h"
#include <stdlib.h>
#include <errno.h>

#if 0
	#define DEBUG
	#define TIME_SCALE	200
#else
	#define TIME_SCALE	5
#endif

#if 1
	#define VERBOSE
#endif

#include <auxio.h>

#define PRIORITY_LOW	50
#define PRIORITY_HIGH	30
#define PRIORITY_HIGEST	10

CARINCSched::CARINCSched(CARINCSched::TTaskDescRow *pTaskDesc, size_t rows) {
	rtems_task_priority new_prio = PRIORITY_HIGH, old_prio ;

	this->pTaskDesc = pTaskDesc ;
	this->pStatus = new CARINCSched::TStatusRow[rows] ;
	this->pProfile = NULL ;
	//this->profileSwitching = false ;

	rtems_id self_id = rtems_task_self() ;
	rtems_task_set_priority(self_id, new_prio, &old_prio) ;
	DTRACE("Create Sched taskId=%lu, change prio: %i->%i \n", self_id, old_prio, new_prio ) ;

	for(unsigned i = 0; i < rows; i++) {
		pStatus[pTaskDesc[i].no].pSched = this ;
		pStatus[pTaskDesc[i].no].pTask = NULL ;
		pStatus[pTaskDesc[i].no].pTaskDescRow = &pTaskDesc[i] ;
		for (int j = INIT; j <= RUN; j++)
			pStatus[pTaskDesc[i].no].stage[j] = CARINCSched::TASK_STAGE_CREATED ;

		pStatus[pTaskDesc[i].no].taskNo = pTaskDesc[i].no ;
		if( pTaskDesc[i].init != NULL ) {

			for(int j = INIT; j <= RUN; j++) {

				int sc = rtems_task_create(
					0xffff0000 | pTaskDesc[i].no << 8 | j, PRIORITY_LOW, 4096, RTEMS_DEFAULT_MODES,
					RTEMS_DEFAULT_ATTRIBUTES, &pStatus[pTaskDesc[i].no].taskId[j]
				);

				if ( sc != RTEMS_SUCCESSFUL) {
					TRACE("Can't create task %i [%i], sc=%i\n\r", i, j, sc) ;
				}
			}

		} else {
			for (int j = INIT; j <= RUN; j++) {
				pStatus[pTaskDesc[i].no].taskId[j] = 0 ;
			}
		}
		DTRACE("Created task %i, taskId[INIT,RUN]=%lu,%lu, init()=0x%x\n\r", pTaskDesc[i].no,
				pStatus[pTaskDesc[i].no].taskId[INIT], pStatus[pTaskDesc[i].no].taskId[RUN], (unsigned)pTaskDesc[i].init) ;
	}
	this->taskDescRows = rows ;

	this->ticksPerMilliSecond = rtems_clock_get_ticks_per_second() / 1000.0 ;

	tactNo = 0 ;
	tactTime = 0 ;

	DTRACE("ticks per millisecond = %f\n\r", ticksPerMilliSecond ) ;

	int rc = rtems_semaphore_create(
			rtems_build_name('S', 'P', 'S', '0'),
			1,
			RTEMS_FIFO | RTEMS_SIMPLE_BINARY_SEMAPHORE | RTEMS_NO_INHERIT_PRIORITY | \
			RTEMS_NO_PRIORITY_CEILING,
			0,
			&semProfile);

	if ( rc != RTEMS_SUCCESSFUL ) {
		TRACE("Can't create semphore: return-code=%i\n", rc) ;
	}
}

CARINCSched::~CARINCSched() {
	// TODO Auto-generated destructor stub
}

void CARINCSched::setProfile(CARINCSched::TProfileRow *pProfile, size_t rows, char *name) {

	VPRINTF("Shed: Select profile -> %s\n", name) ;

	DTRACE("tactTime = %i\n", tactTime) ;

	rtems_semaphore_obtain(semProfile, RTEMS_WAIT, RTEMS_NO_TIMEOUT) ;

	this->pProfile = pProfile ;
	this->profileRows = rows ;
	this->profileName = name ;
	this->stage = PROFILE_STAGE_INIT ;

	setFirstProfileStage() ;

	// Go through all task list and set stage to
	// TASK_STAGE_OBSOLETE for INIT section to be able
	// to recreate the task.
	for(unsigned i = 0; i < rows; i++) {
		if(pProfile[i].call[INIT] &&
				pStatus[pProfile[i].taskNo].stage[INIT] != TASK_STAGE_CREATED) {
			pStatus[pProfile[i].taskNo].stage[INIT] = TASK_STAGE_OBSOLETE ;
		}

		/*if(pProfile[i].call[RUN] &&
				pStatus[pProfile[i].taskNo].stage[RUN] == TASK_STAGE_EXECUTE ) {
			TRACE("WARNING: taskNo=%i is EXECUTING.\n", pProfile[i].taskNo) ;
		}*/

		//
		// Restart task if it is not in below stages
		//
		if(pProfile[i].call[RUN] &&
				pStatus[pProfile[i].taskNo].stage[RUN] != TASK_STAGE_CREATED /*&&
				pStatus[pProfile[i].taskNo].stage[RUN] != TASK_STAGE_WAIT &&
				pStatus[pProfile[i].taskNo].stage[RUN] != TASK_STAGE_EXECUTE*/ ) {
			pStatus[pProfile[i].taskNo].stage[RUN] = TASK_STAGE_OBSOLETE ;
		}
	}

	rtems_semaphore_release( semProfile );

	DTRACE("Profile setting is DONE.\n") ;
}

void CARINCSched::setTactLenMs(int ms) {
	this->tactLenMs = ms ;
}

int CARINCSched::start() {

	int sc = rtems_rate_monotonic_create(rtems_build_name('S', 'C', 'R', 'M'), &rmId) ;
	if ( sc != RTEMS_SUCCESSFUL) {
		TRACE("rtems_rate_monotonic_create() fail: %i\n\r", sc) ;
	}

	int initLine = 0 ;

	// Wait for signal from the timer
	DTRACE("Waiting for period...\n\r") ;
	for(tactTime = 0 ; (sc = rtems_rate_monotonic_period(rmId, ticksPerMilliSecond * TIME_SCALE )) == RTEMS_SUCCESSFUL ; tactTime++, tactTime %= tactLenMs ) {

		rtems_semaphore_obtain(semProfile, RTEMS_WAIT, RTEMS_NO_TIMEOUT) ;

		if (tactTime == 0) {
			tactNo ++ ;
			DTRACE("************ New tact = %i *************\n", tactNo) ;
		}

		DTRACE("Beginning of subtact, tt=%i, sched-stage=%s, profile=%s\n\r", tactTime, profileStageToStr(stage), profileName) ;

		// Counter of tasks in each state either INIT or RUN
		// for certain profile
		int tasks[2] = {0, 0} ;

		for (int i = 0; i < profileRows; i++) {
			for (int j = INIT; j <= RUN; j++) {
				if(pProfile[i].call[j]) tasks[j]++ ;
			}
		}

		// Go through all profile lines to check and manage state of tasks
		for (int line = 0; line < profileRows; line++) {
			switch (stage) {
			case CARINCSched::PROFILE_STAGE_INIT:
				if (&pStatus[pProfile[line].taskNo] == NULL) {
					initLine++ ;
					continue ;
				}
				//if (pProfile[line].beg_cycle_time == t) {
				switch (pStatus[pProfile[line].taskNo].stage[INIT]) {
				case CARINCSched::TASK_STAGE_CREATED:
					if (initLine == line && pProfile[line].call[INIT] ) {
						// Switch task to init mode
						DTRACE("INIT: Set task%i CREATED -> EXECUTE state...\n\r", pProfile[line].taskNo ) ;
						pStatus[pProfile[line].taskNo].stage[INIT] = CARINCSched::TASK_STAGE_EXECUTE ;
						rtems_task_start(pStatus[pProfile[line].taskNo].taskId[INIT], taskProcInit,
								(rtems_task_argument)&pStatus[pProfile[line].taskNo]) ;
					}
					break ;
				/*case CARINCSched::TASK_STAGE_EXECUTE:
					execInitCounter ++ ;
					if (initLine == line && tactTime == tactLenMs && pProfile[line].call[INIT]) {
						DTRACE("INIT: Set task%i(%s) EXECUTE -> WAIT state\n\r", pProfile[line].taskNo, pTaskDesc[pProfile[line].taskNo].descr) ;
						pStatus[pProfile[line].taskNo].stage[INIT] = CARINCSched::TASK_STAGE_WAIT ;
						rtems_task_suspend(pStatus[pProfile[line].taskNo].taskId[INIT]) ;
					}
					break ;
				case CARINCSched::TASK_STAGE_WAIT:
					execInitCounter ++ ;
					if (initLine == line && tactTime == 0 && pProfile[line].call[INIT]) {
						// Resume execution of task and
						// set status to TASK_STATUS_INIT
						DTRACE("INIT: Set task%i WAIT -> EXECUTE state...\n\r", pProfile[line].taskNo ) ;
						pStatus[pProfile[line].taskNo].stage[INIT] = CARINCSched::TASK_STAGE_EXECUTE ;
						rtems_task_resume(pStatus[pProfile[line].taskNo].taskId[INIT]) ;
					}*/
					break ;
				case CARINCSched::TASK_STAGE_DONE:
					if(initLine == line && pProfile[line].call[INIT]) {
						initLine ++ ;
					}

					if (initLine >= tasks[INIT]) {
						if (tactTime != 0) {
							stage = CARINCSched::PROFILE_STAGE_INIT_DONE ;
							initLine = 0 ;
						} else {
							stage = CARINCSched::PROFILE_STAGE_RUN ;
						}
					}

					break ;
				case CARINCSched::TASK_STAGE_OBSOLETE:
					if (initLine == line && pProfile[line].call[INIT] ) {
						// Switch task to init mode
						DTRACE("INIT: Set task%i OBSOLETE -> EXECUTE state...\n\r", pProfile[line].taskNo ) ;
						pStatus[pProfile[line].taskNo].stage[INIT] = CARINCSched::TASK_STAGE_EXECUTE ;
						rtems_task_restart(pStatus[pProfile[line].taskNo].taskId[INIT],
								(rtems_task_argument)&pStatus[pProfile[line].taskNo]) ;
					}
					break ;
				default:
					//An error
					break ;
				}

				break ;
			case CARINCSched::PROFILE_STAGE_INIT_DONE:
				if (tactTime == tactLenMs - 1) {
					stage = CARINCSched::PROFILE_STAGE_RUN ;
				}
				break ;
			case CARINCSched::PROFILE_STAGE_RUN:
				if (&pStatus[pProfile[line].taskNo] == NULL) {
					continue ;
				}
				switch (pStatus[pProfile[line].taskNo].stage[RUN]) {
				case CARINCSched::TASK_STAGE_CREATED:
					if (pProfile[line].beg_cycle_time == tactTime && pProfile[line].call[RUN]) {
						DTRACE("RUN: Set task%i CREATED -> EXECUTE state\n\r", pProfile[line].taskNo) ;
						pStatus[pProfile[line].taskNo].stage[RUN] = CARINCSched::TASK_STAGE_EXECUTE ;
						rtems_task_start(pStatus[pProfile[line].taskNo].taskId[RUN], taskProcRun,
							(rtems_task_argument)&pStatus[pProfile[line].taskNo]) ;
					}
					break ;
				case CARINCSched::TASK_STAGE_WAIT:
					if (pProfile[line].beg_cycle_time == tactTime && pProfile[line].call[RUN]) {
						DTRACE("RUN: Set task%i(%s) WAIT -> EXECUTE state\n\r", pProfile[line].taskNo, pTaskDesc[pProfile[line].taskNo].descr) ;
						pStatus[pProfile[line].taskNo].stage[RUN] = CARINCSched::TASK_STAGE_EXECUTE ;
						rtems_task_resume(pStatus[pProfile[line].taskNo].taskId[RUN]) ;
					}
					break ;
				case CARINCSched::TASK_STAGE_EXECUTE:
					if (pProfile[line].end_cycle_time == tactTime && pProfile[line].call[RUN]) {
						DTRACE("Set task%i EXECUTE -> WAIT state\n\r", pProfile[line].taskNo) ;
						pStatus[pProfile[line].taskNo].stage[RUN] = CARINCSched::TASK_STAGE_WAIT ;
						//rtems_task_suspend(pStatus[pProfile[line].taskNo].taskId[RUN]) ;
					}
					break ;
				case CARINCSched::TASK_STAGE_OBSOLETE:
					if (pProfile[line].beg_cycle_time == tactTime && pProfile[line].call[RUN] ) {
						// Switch task to execute mode
						DTRACE("RUN: Set task%i OBSOLETE -> EXECUTE state...\n\r", pProfile[line].taskNo ) ;
						pStatus[pProfile[line].taskNo].stage[RUN] = CARINCSched::TASK_STAGE_EXECUTE ;
						rtems_task_restart(pStatus[pProfile[line].taskNo].taskId[RUN],
								(rtems_task_argument)&pStatus[pProfile[line].taskNo]) ;
					}
					break ;
				}
				break ;
			case CARINCSched::PROFILE_STAGE_RUN_DONE:
				break ;
			// Wait beginning next tact then switch to INIT
			// after switching profile
			case CARINCSched::PROFILE_STAGE_WAIT_INIT:
				if (tactTime == tactLenMs - 1) {
					setNextWaitInitStage() ;
				}
				break ;
			}
			DTRACE("taskNo=%i, task-stage[INIT,RUN]=%s,%s\n\r", pProfile[line].taskNo,
				taskStageToStr(pStatus[pProfile[line].taskNo].stage[INIT]),
				taskStageToStr(pStatus[pProfile[line].taskNo].stage[RUN])) ;
		} // for line
		DTRACE("tasks[INIT,RUN]=%i,%i, initLine=%i\n\r", tasks[INIT], tasks[RUN], initLine) ;

		rtems_semaphore_release( semProfile );

	} // for sigwait()
	TRACE( "rtems_rate_monotonic_period(): %i\n\r", sc ) ;
	return -1 ;
}

int CARINCSched::getState() {

	return 0 ;
}

// Start Task Initialization thread
void CARINCSched::taskProcInit(rtems_task_argument arg) {
	TStatusRow *pStatusRow = (TStatusRow*)arg;

	DTRACE("Starting task, taskId[INIT] = %lu\n\r", pStatusRow->taskId[INIT]) ;

	// Delete old task object to destroy task context
	if(pStatusRow->pTask != NULL) {
		// Remove old task
		delete pStatusRow->pTask ;
		pStatusRow->pTask = NULL ;
	}

	// Create new task object, which practically will be created
	// in the same memory address
	DTRACE("init()=0x%x\n\r", (unsigned)pStatusRow->pTaskDescRow->init) ;
	CBaseTask *pTask = pStatusRow->pTaskDescRow->init(pStatusRow->pSched) ;
	DTRACE("taskNo=%i, pTask = 0x%x, descr=%s\n", pStatusRow->taskNo, pTask, pStatusRow->pTaskDescRow->descr) ;

	// Check if we successfully created a Task Object.
	if(!pTask) {
		TRACE("Can't create task object: %s\n", pStatusRow->pTaskDescRow->descr, strerror(errno)) ;
		exit(-1) ;
	}

	// Initialize Dispatcher data
	pStatusRow->pTask = pTask ;
	pStatusRow->stage[INIT] = CARINCSched::TASK_STAGE_DONE ;

	DTRACE("Done\n") ;

	// Suspend thread, because if not, system will be halted
	while(1) rtems_task_suspend(rtems_task_self()) ;
}

void CARINCSched::taskProcRun(rtems_task_argument arg) {
	TStatusRow *pStatusRow = (TStatusRow*)arg;

	// Call execution procedure from task Object
	pStatusRow->pTask->run(pStatusRow->pSched) ;

	// When execution finishes, mark task stage as DONE
	pStatusRow->stage[1] = CARINCSched::TASK_STAGE_DONE ;

	// Suspend execution thread, because if not, system will be halted
	while(1) rtems_task_suspend(rtems_task_self()) ;
}

void CARINCSched::waitForMyTime() {

	rtems_id taskId = rtems_task_self() ;
	TStatusRow *pStatusRow = getTaskStatusRow(taskId) ;

	DTRACE("taskNo=%i, taskID=%lu\n\r", pStatusRow->taskNo, taskId) ;

	//CMachine *pMachine = (CMachine*)getTaskStatusRow(CSysMpoMachine::init)->pTask ;

	int taskType ;
	switch(stage) {
	case PROFILE_STAGE_INIT:
		taskType = INIT ;
		break ;
	case PROFILE_STAGE_RUN:
		taskType = RUN ;
		break ;
	default:
		DTRACE("Internal scheduler error occured!\n\r") ;
		//rtems_task_suspend(taskId) ;
		//pMachine->writeCtlSym(CSysMpoMachine::csErr) ;
	}

	/*switch(pSr->stage[task]) {
	case TASK_STAGE_CREATED:
	case TASK_STAGE_OBSOLETE:
		pSr->stage[task] = TASK_STAGE_EXECUTE ;
		break ;
	case TASK_STAGE_EXECUTE:
		pSr->stage[task] = TASK_STAGE_WAIT ;
		rtems_task_suspend(taskId) ;
		break ;
	case TASK_STAGE_WAIT:
		// An internal scheduler error!
		DTRACE("Internal scheduler error occured!\n\r") ;
		//rtems_task_suspend(taskId) ;
		//pMachine->writeCtlSym(CSysMpoMachine::csErr) ;
		break ;
	}*/

	/*
	 * That is strange but necessary to check by this task if
	 * it is already not in a WAIT stage. Otherwise task will
	 * often hangs on.
	 */
	/*if(pStatusRow->stage[taskType] != TASK_STAGE_WAIT) {
		pStatusRow->stage[taskType] = TASK_STAGE_WAIT ;
		rtems_task_suspend(taskId) ;
	}*/
	//pStatusRow->stage[taskType] = TASK_STAGE_WAIT ;
	if (stage == PROFILE_STAGE_RUN) {
		pStatusRow->stage[RUN] = TASK_STAGE_WAIT ;
		rtems_task_suspend(RTEMS_SELF) ;
	}
}

CARINCSched::TStatusRow *CARINCSched::getTaskStatusRow(rtems_id taskId) {
	for (unsigned i = 0; i < taskDescRows; i++) {
		for (int j = 0; j < 2; j++) {
			if (pStatus[pTaskDesc[i].no].taskId[j] == taskId) return &pStatus[i] ;
		}
	}

	return NULL ;
}

float CARINCSched::getMyTimeLeftMs() {

	TStatusRow *pStatus = getTaskStatusRow(rtems_task_self()) ;

	int no = pStatus->taskNo ;

	rtems_semaphore_obtain(semProfile, RTEMS_WAIT, RTEMS_NO_TIMEOUT) ;

	int i ;
	for (i = 0; i < profileRows; i++) {
		if (pProfile[i].taskNo == no) break ;
	}

	int leftMs = pProfile[i].end_cycle_time - tactTime ;

	rtems_semaphore_release( semProfile );

	DTRACE ("leftMs=%i\n\r", leftMs) ;

	rtems_rate_monotonic_period_status rmStat ;
	rtems_rate_monotonic_get_status(rmId, &rmStat) ;

	int leftNs = 1000000 - rmStat.since_last_period.tv_nsec / TIME_SCALE ;
	DTRACE("leftNs=%i\n\r", leftNs) ;

	float timeLeft = leftMs + leftNs / 1000000.0 ;

	DTRACE("left %f ms\n\r", timeLeft) ;

	return  timeLeft ;
}

CARINCSched::TStatusRow *CARINCSched::getTaskStatusRow(TProcInit init) {

	for (unsigned i = 0; i < taskDescRows; i++) {
		DTRACE("init()=0x%x, pTask=0x%x\n\r", (unsigned)pTaskDesc[i].init, (unsigned)pStatus[pTaskDesc[i].no].pTask) ;
		if (pTaskDesc[i].init == init)	return &pStatus[pTaskDesc[i].no] ;
	}
	return NULL ;
}

size_t CARINCSched::getTaskDescRows() {

	return taskDescRows ;
}

void CARINCSched::setFirstProfileStage() {
	stage = CARINCSched::PROFILE_STAGE_RUN ;
	for (int i = 0; i < profileRows; i++) {
		if (pProfile[i].call[INIT]) {
			if(tactTime > pProfile[i].beg_cycle_time ) {
				stage = CARINCSched::PROFILE_STAGE_WAIT_INIT ;
			} else {
				stage = CARINCSched::PROFILE_STAGE_INIT ;
			}
			break ;
		}
	}
}

void CARINCSched::setNextWaitInitStage() {
	stage = CARINCSched::PROFILE_STAGE_RUN ;
	for (int i = 0; i < profileRows; i++) {
		if (pProfile[i].call[INIT]) {
			stage = CARINCSched::PROFILE_STAGE_INIT ;
			break ;
		}
	}
}

char *CARINCSched::profileStageToStr(int stage) {
	static char str[32] ;

	switch(stage) {
	case PROFILE_STAGE_WAIT_INIT:
		sprintf(str, "WAIT_INIT") ;
		break ;
	case PROFILE_STAGE_INIT:
		sprintf(str, "INIT") ;
		break ;
	case PROFILE_STAGE_INIT_DONE:
		sprintf(str, "INIT_DONE") ;
		break ;
	case PROFILE_STAGE_RUN:
		sprintf(str, "RUN") ;
		break ;
	case PROFILE_STAGE_RUN_DONE:
		sprintf(str, "RUN_DONE") ;
		break ;
	}
	return str ;
}

char *CARINCSched::taskStageToStr(int stage) {
	static char str[2][32] ;
	static int i ;

	i = (i + 1) % 2 ;
	switch(stage) {
	case TASK_STAGE_CREATED:
		sprintf(str[i], "CREATED") ;
		break ;
	case TASK_STAGE_WAIT:
		sprintf(str[i], "WAIT") ;
		break ;
	case TASK_STAGE_EXECUTE:
		sprintf(str[i], "EXECUTE") ;
		break ;
	case TASK_STAGE_DONE:
		sprintf(str[i], "DONE") ;
		break ;
	case TASK_STAGE_OBSOLETE:
		sprintf(str[i], "OBSOLETE") ;
		break ;
	}
	return str[i] ;
}
