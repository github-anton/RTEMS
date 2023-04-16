/*
 * auxio.c
 *
 *  Created on: Jul 15, 2020
 *      Author: anton
 */
#include "auxio.h"

void print_hex_dump(void *array, size_t len) {
	size_t i ;

	for( i = 0; i < len; i++ ) {
		printf("%02x ", ((u_char*)array)[i]) ;
		if (!(i % 32 ) && i ) {
			printf("\n");
		}
	}
	if ( (i - 1) % 32 ) {
		printf("\n");
	}
}

void print_bin_dump(void *array, size_t len) {
	size_t i, j ;

	for ( i = 0; i < len; i++) {
		for (j = 0; j < sizeof(u_char)*8; j++) {
			printf("%i", (((u_char*)array)[i] >> (sizeof(u_char)*8 - 1 - j)) & 0x1 ) ;
		}
		if ( !(i % 11) && i ) {
			printf ("\n") ;
		} else {
			printf(" ") ;
		}
	}
}
