/*
 * auxmath.h
 *
 *  Created on: Jan 23, 2020
 *      Author: anton
 */

#ifndef AUXMATH_H_
#define AUXMATH_H_

#ifndef MIN
	#define MIN(a, b) ((a) < (b)? (a): (b))
#endif

#ifndef MAX
	#define MAX(a, b) ((a) > (b)? (a): (b))
#endif

#ifndef ABS
	#define ABS(a)	( (a) > 0 ? (a) : (-1*(a)))
#endif

#endif /* AUXMATH_H_ */
