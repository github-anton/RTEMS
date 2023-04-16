/*
 * crc16_test.cpp
 *
 *  Created on: Jan 28, 2020
 *      Author: anton
 */
#include <stdio.h>
#include <string.h>
#include "crc16.h"

/*
 * CRC16/CCITT-FALSE
 * str = 1234, crc16 = 0x5349
 * str = 12345678, crc16 = 0xa12b
 */

crc16_context_t	arc_context ;

int main(void) {

	char str[][9] = {
			"1234",
			"12345678"
	} ;

	printf("*****  CRC16 test.  *****\n\n") ;

	CRC16_ARC_CALC_TABLE(&arc_context) ;

	for (int i = 0; i < 2; i++) {
		u_short crc0 ;

		//
		// Calculate CRC16 of input array
		//
		u_short crc = crc16_calc(&arc_context, CRC16_ARC_INIT_VAL, str[i], strlen(str[i])) ;
		printf("CRC16(\"%s\") = 0x%x\n", str[i], crc) ;

		//
		// To check if CRC16 was calculated correctly,
		// we calculate CRC16 of input array + CRC16.
		// And result must be equal to 0.
		//
		crc0 = crc16_calc(&arc_context, crc, &crc, sizeof(crc) ) ;
		printf("CRC16(\"%s\"+0x%x) = 0x%x\n", str[i], crc, crc0) ;
	}


	printf("\n***** End of test. *****\n") ;

	return 0 ;
}
