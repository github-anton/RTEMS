/*
 * crc16.h
 *
 *  Created on: Jan 23, 2020
 *      Author: anton
 */

#ifndef CRC16_H_
#define CRC16_H_

#include <sys/types.h>
#include <stdint.h>

#ifndef TRUE
	#define	TRUE	(1==1)
#endif

#ifndef FALSE
	#define	FALE	(!TRUE)
#endif

#ifdef __cplusplus
	extern "C" {
#endif

typedef struct {
	u_short poly ;
	u_short init ;
	int		ref_in ;
	int		ref_out ;
	u_short final_xor ;
} crc16_settings_t ;

typedef struct {
	crc16_settings_t settings ;
	uint16_t table[256] ;
} crc16_context_t;

void crc16_calc_table (crc16_context_t *p_context, u_short poly, int ref_in, int ref_out, u_short final_xor) ;
uint16_t crc16_calc (crc16_context_t *p_context, u_short init, void *buf, size_t len) ;
uint16_t crc16_reflect(uint16_t word, size_t wide) ;

#ifdef __cplusplus
	}
#endif

#define CRC16_X25_CALC_TABLE(context)	crc16_calc_table(context, 0x1021, TRUE, TRUE, 0xFFFF)
#define CRC16_ARC_CALC_TABLE(context)	crc16_calc_table(context, 0x8005, TRUE, TRUE, 0x0000)

#define CRC16_X25_INIT_VAL	0xFFFF
#define CRC16_ARC_INIT_VAL	0x0000

#endif /* CRC16_H_ */
