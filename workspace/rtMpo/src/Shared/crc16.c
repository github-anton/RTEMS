/*
 * crc16.c
 *
 *  Created on: Jan 15, 2020.
 *
 *  CRC16 calculation routines.
 *
 *  Copyright (C) 2020, 2021 Anton Ermakov.
 */

#include "crc16.h"

/*
 * CCITT-FALSE: POLY = 0x1021, INIT = 0xFFFF, RefIn=false, RefOut=false, FINAL_XOR = 0x0000
 * CRC-16/X25:	POLY = 0x1021, INIT = 0xFFFF, RefIn=true,  RefOut=true,  FINAL_XOR = 0xFFFF
 */
#define POLY 		p_context->settings.poly
#define INIT		p_context->settings.init
#define RefIn		p_context->settings.ref_in
#define RefOut		p_context->settings.ref_out
#define FINAL_XOR	p_context->settings.final_xor
#define TABLE		p_context->table

void crc16_calc_table (crc16_context_t *p_context, u_short poly, int ref_in, int ref_out, u_short final_xor) {
	uint16_t div ;
	int bit ;

	POLY = poly ;
	RefIn = ref_in ;
	RefOut = ref_out ;
	FINAL_XOR = final_xor ;

	/* Calculate straight crc16 table */

	for (div = 0; div < 256; div++) {
		uint16_t crc = div << 8 ;
		for (bit = 0; bit < 8; bit++) {
			crc = crc & 0x8000 ? (crc << 1) ^ POLY : crc << 1 ;
		}
		TABLE[div] = crc ;
	}
}

/*
 *  Calculate crc16.
 */
uint16_t crc16_calc (crc16_context_t *p_context, u_short init, void *buf, size_t len) {
	uint16_t crc = init ;
	u_char byte ;
	size_t i ;

	crc = (RefIn ? crc16_reflect(crc, sizeof(uint16_t)): crc) ;

	for ( i = 0; i < len; i++) {
		byte = ((u_char*)buf)[i] ;
		byte = (RefIn ? crc16_reflect(byte, sizeof(u_char)) : byte) ;
		u_char pos = byte ^ (crc >> 8) ;
		crc = (crc << 8) ^ TABLE[pos] ;
	}

	crc = (RefOut ? crc16_reflect(crc, sizeof(uint16_t)): crc) ;

	return crc ^ FINAL_XOR ;
}

/*
 * Reflect word or byte, depends on wide (can be 1 or 2).
 */
uint16_t crc16_reflect(uint16_t word, size_t wide) {
	size_t i ;
	uint16_t ref = 0 ;

	for( i = 0; i < wide * 8; i++ ) {
		if (word & (1 << i)) {
			ref = ref | 1 << ((wide * 8) - i - 1) ;
		}
	}
	return ref ;
}
