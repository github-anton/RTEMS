/*
 * CSched.h
 *
 *  Created on: Jul 5, 2019
 *      Author: Anton Ermakov
 *
 *      ARINC scheduler.
 */

#ifndef CARINCSCHED_H_
#define CARINCSCHED_H_

#include <rtems.h>
#include <sys/types.h>

#include <CFiniteMachine.h>
#include <CTask.h>

class CARINCSched {
public:

	typedef CBaseTask* (*TProcInit)(CARINCSched*) ;

	typedef struct {
		int no ;
		TProcInit	init ;
		const char *descr ;
	} TTaskDescRow ;

	typedef struct {
		int	taskNo ;
		bool	call[2] ;		// 0 - Initialization part, 1 - Run part
		bool	destroy_after ;
		unsigned beg_cycle_time ;
		unsigned end_cycle_time ;
	} TProfileRow ;

	enum { INIT = 0, RUN = 1 } ;

	typedef struct {
		TProfileRow *pProfile ;
		int rows ;
	} TProfileSetRow ;

	typedef struct {
		int 	taskNo ;
		int		stage[2] ;			// 0 - Initialization part, 1 - Run part
		CBaseTask	*pTask ;
		rtems_id	taskId[2] ;		// 0 - Initialization part, 1 - Run part
		CARINCSched	*pSched ;
		TTaskDescRow *pTaskDescRow ;
	} TStatusRow ;

	enum { PROFILE_STAGE_WAIT_INIT = 1, PROFILE_STAGE_INIT = 2, PROFILE_STAGE_INIT_DONE = 3, PROFILE_STAGE_RUN = 4, PROFILE_STAGE_RUN_DONE = 5 } ;

	enum { TASK_STAGE_CREATED = 1, TASK_STAGE_WAIT = 2, TASK_STAGE_EXECUTE = 3, TASK_STAGE_DONE = 4, TASK_STAGE_OBSOLETE = 5 } ;

	TTaskDescRow	*pTaskDesc ;
	size_t			taskDescRows ;

private:
	TProfileRow	*pProfile ;	// pointer to the active parofile
	TStatusRow	*pStatus ;	// pointer to the status table of the tasks
	int		profileRows ;		// Number of lines in the active profile
	char	*profileName ;		// Name of current profile (for debug purpose mostly)
	unsigned	tactLenMs ;	// Length of tact
	timer_t timerId;	// ID of the timer which scheduler is driven
	int 	stage ;		// the stage of execution whether mainly INIT or RUN
	int		currentTask ;	// The active line of profile and status table (current task)
	TProfileRow *getActiveProfileRowPtr() ;
	TStatusRow *getActiveStatusRowPtr() ;
	float		ticksPerMilliSecond ;
	unsigned	tactTime ;	// Time inside one tact
	int			tactNo ;	// Current number of tact
	rtems_id	rmId ;		// Rate monotonic ID
	rtems_id	semProfile ;	// Mutual access to pProfile variable

public:
	CARINCSched(CARINCSched::TTaskDescRow *pTaskDesc, size_t rows) ;
	virtual ~CARINCSched() ;
	virtual void setProfile(TProfileRow *pProfile, size_t rows, char *name) ;
	virtual void setTactLenMs(int ms) ;
	// Start Init/Run cycle
	virtual int start() ;
	virtual int getState() ;
	static void taskProcInit(rtems_task_argument arg) ;
	static void	taskProcRun(rtems_task_argument arg) ;
	virtual void waitForMyTime() ;
	CARINCSched::TStatusRow *getTaskStatusRow(rtems_id taskId) ;
	CARINCSched::TStatusRow *getTaskStatusRow(TProcInit init) ;
	virtual float getMyTimeLeftMs() ;
	size_t	getTaskDescRows() ;
	void setFirstProfileStage() ;
	void setNextWaitInitStage() ;
	static char *profileStageToStr(int stage) ;
	static char *taskStageToStr(int stage) ;
} ;

#endif /* CSCHED_H_ */
