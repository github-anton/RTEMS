/*
 * CTask.h
 *
 *  Created on: Jul 11, 2019
 *      Author: anton
 */

#ifndef CTASK_H_
#define CTASK_H_

class CARINCSched ;

class CBaseTask {
public:
	CBaseTask() ;
	virtual ~CBaseTask() ;
	virtual int run(CARINCSched *pSched) = 0 ;
};

template <typename T>  class CTask: public CBaseTask {
public:
	CTask() ;
	virtual ~CTask() ;
	static CBaseTask *init(CARINCSched *pSched) ;
	virtual int run(CARINCSched *pSched) = 0 ;
} ;

template <typename T>
CTask<T>::CTask() {
	// TODO Auto-generated constructor stub
}

template <typename T>
CTask<T>::~CTask() {
	// TODO Auto-generated destructor stub
}

template  <typename T>
CBaseTask *CTask<T>::init(CARINCSched *pSched) {
	return new T(pSched) ;
} ;

#endif /* CTASK_H_ */
