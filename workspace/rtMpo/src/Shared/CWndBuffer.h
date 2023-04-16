/*
 * CWndBuf.h
 *
 *  Created on: May 29, 2020
 *      Author: anton
 */

#ifndef CWNDBUF_H_
#define CWNDBUF_H_

#include <sys/types.h>

class CWndBuffer {
private:
	u_char	*pArea ;
	size_t	wide ;
	size_t calcIdx(int off) ;

public:
	CWndBuffer(size_t wide);
	virtual ~CWndBuffer();
	virtual void add(u_char byte) ;
	virtual u_char getByte(int off) ;
	virtual u_short getWord(int off) ;
	virtual u_int getDWord(int off) ;
/*
	virtual void printHexDump() ;
	virtual void printBinDump() ;
	*/
};

#endif /* CWNDBUF_H_ */
