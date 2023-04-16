/*
 * MMIProto.h
 * MPO-MZU Interchange Protocol.
 *
 *  Created on: Mar 1, 2019
 *      Author: anton
 */

#ifndef MPOSWPROTOCOL_H_
#define MPOSWPROTOCOL_H_

#include <auxmath.h>

#define MMI_PROTO_ID				0xF675

// All packets has this header
typedef struct __attribute__((__packed__)) {
	uint16_t	proto_id ;	// Protocol ID, for MMI is always = 0xF675
	uint16_t	msg_id ;	// Message ID
	uint32_t	bshv_time ;	// On-board time
} mmi_hdr0_t;

// MPO mode setting up command
typedef struct __attribute__((__packed__)) {
	mmi_hdr0_t	hdr0 ;		// MMI header
							// DO NOT FORGET ALIGN!!!
	uint16_t	mode_id ;	// New mode for MPO
	uint16_t	arg[9] ;	// Advanced arguments for a command
	uint16_t	unused0 ;
	uint16_t	crc16 ;		// CRC16/ARC of all bytes in packet except CRC16
} mmi_cmd_mpo_setmode_t ;

#define MMI_CMD_MPO_SETMODE_MSG_ID		0x1			// Setup MPO mode command

#define MMI_MPO_MODE_ID_INIT			0x1			// Start initializing MPO
#define MMI_MPO_MODE_ID_WAIT			0x2			// Wait for any command
#define MMI_MPO_MODE_ID_TRRAW			0x3			// Start transmit AIS decoded messages
													// (AIS data packets)
#define MMI_MPO_MODE_ID_TRDEC			0x4			// Start transmitting raw messages
													// (Quadrature packets)
#define MMI_MPO_MODE_ID_SERVICE			0x5			// Switch to service mode
													// (Setup FPGA coefficients, flash new MPO firmware.)
#define MMI_MPO_MODE_ARG0_NOREPAIR		0x0			// Do not try to fix damaged packets
#define MMI_MPO_MODE_ARG0_REPAIR		0x1			// Try to fix damaged packets

// MPO status request
typedef struct __attribute__((__packed__)) {
	mmi_hdr0_t	hdr0 ;		// MMI header
	uint16_t	unused1 ;
	uint16_t	crc16 ;		// CRC16/ARC of all bytes in packet except CRC16 itself
} mmi_cmd_mpo_statusrq_t;

#define MMI_CMD_MPO_STATUSRQ_MSG_ID		0x2			// Request MPO status/state command

// State reply from MPO
typedef struct __attribute__((__packed__)) {
	mmi_hdr0_t	hdr0 ;		// MMI header
	uint16_t	state ;		// State of MPO Finite Machine
	uint16_t	arg ;		// Additional argument which determines mode feature
	uint16_t 	unused1	;	// Align crc16
	uint16_t	crc16 ;		// CRC16/ARC of all bytes in packet except CRC16 itself
} mmi_mpo_status_t;

#define MMI_MPO_STATUS_MSG_ID			0x4			// Reply with MPO status

//
// AIS information packet structures.
//
// AIS Packet header.
//
typedef struct __attribute__((__packed__)) {
	mmi_hdr0_t	hdr0 ;		// MMI header
} mmi_mpo_ais_inf_packet_hdr_t ;

#define MMI_MPO_AIS_INF_PACKET_MSG_ID			0xCBD1		// Packet with AIS data blocks
#define MMI_MPO_AIS_INF_PACKET_LEN				1280		// Fixed size AIS packet length.

//
// AIS data block header
//
typedef struct __attribute__((__packed__)) {
	uint16_t	bshv_off ;			// BSHV time offset
	uint8_t		powc_dbm ;			// Power of a message
	uint8_t		ch_no: 3 ;			// AIS channel number
	uint8_t		recv_no: 1 ;		// Tract (receiver) number
	uint8_t		delta_Dop_frq_c_Hz: 4 ;// Doppler's frequency offset, Hz.
	uint8_t		len ;				// AIS data message length.
} mmi_mpo_ais_inf_data_block_hdr_t;

#define MMI_MIN_INF_DATA_BLOCK_SIZE		14				// 14 bytes
#define MMI_MAX_INF_DATA_BLOCK_SIZE		138				// 138 bytes

/*
 * FPGA quadrature structures.
 */
typedef struct __attribute__((__packed__)) {
	mmi_hdr0_t	hdr0 ;
	struct {
		uint32_t	freq ;			// Central frequency, Hz, fixed point
		uint16_t	counter ;
	} ;
} mmi_mpo_fpga_quad_packet_hdr_t;

#define MMI_MPO_FPGA_QUAD_PACKET_MSG_ID			0x728B		// Packet with raw data from FPGA
#define MMI_MPO_FPGA_QUAD_PACKET_LEN			1280		// Fixed size FPGA quadrature packet length.

#define MMI_MAX_QUAD_BLOCK_SIZE					1020		// Maximum length of quadrature data block

//
// MMI FPGA setup command?
//
/*typedef struct {
	mmi_hdr0_t				hdr0 ;
	fpga_recv_coefs_var_t		coef ;
	fpga_recv_lport_var_t	lport[2] ;		// LPort#1 and LPort#2 parameters
	uint16_t				unused1 ;
	uint16_t				crc16 ;			// CRC16/ARC of all bytes in packet except CRC16 itself
} mmi_cmd_tune_mpo_fpga_t ;*/

#define SW_MAX_INC_PACKET_LEN	MAX(sizeof(mmi_cmd_mpo_setmode_t),	\
								MAX(sizeof(mmi_cmd_mpo_statusrq_t),	\
								1))

#define SW_MAX_OUT_PACKET_LEN	MAX(sizeof(mmi_mpo_status_t), 		\
								MAX(MMI_MPO_AIS_INF_PACKET_LEN,		\
								MAX(MMI_MPO_FPGA_QUAD_PACKET_LEN,	\
								1)))

#define SW_MMI_DATA_LEN			SW_MAX_OUT_PACKET_LEN
#define SW_MMI_ADDR_LEN			1
#define SW_MAX_MMI_PACKET_LEN	(SW_MMI_DATA_LEN + SW_MMI_ADDR_LEN)

#define SW_MPO_ADDR				32
#define SW_MZU_ADDR				33

#endif /* MMIPROTO_H_ */
