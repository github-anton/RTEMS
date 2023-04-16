/*
 * mfbsp.h  - common types and macros for Multicore MFBSP
 * 
 * Copyright (C) 2020 by Anton Ermakov
 */

#ifndef LIBBSP_MIPS_MFBSP_H_
#define LIBBSP_MIPS_MFBSP_H_

#include <bsp/config.h>
#include <bsp.h>
#include <stdint.h>

#define MFBSP_LCLK_RATE		((CPU_CLOCK_RATE_MHZ/(2*CONFIG_MULTICORE_LPORT_RATE_MHZ)) - 1)

/*
 * MFBSP_CSR register, control
 */
typedef union __attribute__((packed,aligned(4))) {
	struct {
		uint16_t len				   :1;	// 0	В режиме I2S/SPI должен быть установлен в 0
		uint16_t ltran		           :1;	// 1	Режим работы порта
		uint16_t lclk_rate_lo		   :1;	// 2	Делитель частоты LPORT
		uint16_t lstat				   :2;	// 4:3	состояние буфера
		uint16_t lrerr				   :1;	// 5	Ошибка приема данных
		uint16_t ldw				   :1;	// 6	Разрядность внешней шины данных
		uint16_t srq_tx			       :1;	// 7	Признак запроса обслуживания на передачу данных
		uint16_t srq_rx				   :1;	// 8	Признак запроса обслуживания на прием  данных
		uint16_t spi_i2s_en			   :1;	// 9	Включение режима SPI/I2S
		uint16_t lpt_irq_en   		   :1;	// 10	Разрешение прерывания SRQ
		uint16_t lclk_rate_hi	   	   :4;	// 14:11 Делитель частоты LPORT [4:1]
		uint16_t mfbsp_rxbuf_irq_en    :1;	// 15	 Разрешение прерывания MFBSP_RXBUF
		uint16_t mfbsp_txbuf_irq_en    :1;	// 16	 Разрешение прерывания MFBSP_TXBUF
		uint16_t reserved0   		   :13; // 29:17
		uint16_t tx_rdy_mode		   :1;	// 30	 Режим формирования признака готовности передачи данных из MFBSP в DMA
		uint16_t rx_rdy_mode 		   :1;	// 31	 Режим формирования признака готовности приема данных из DMA в MFBSP
	} b ;
	uint32_t    dw ;
} mfbsp_csr_t ;

/*
 * MFBSP_DIR register, configures IO direction
 */
typedef union __attribute__((packed,aligned(4))) {
	struct {
		uint16_t	lack: 1 ;
		uint16_t	lclk: 1 ;
		uint16_t	ldat: 8 ;	// bit in 1 - output, bit in 0 - input
	} b ;
	uint32_t	data ;			// Firts 10 bits is use
	uint32_t	dw ;
} mfbsp_dir_t ;

/*
 * MFBSP GPIO register.
 */
typedef union __attribute__((packed,aligned(4))) {
	struct {
		uint16_t	lack: 1 ;
		uint16_t	lclk: 1 ;
		uint16_t	ldat: 8 ;	// I/O byte
	} b ;
	uint32_t	data ;			// Firts 10 bits is use
	uint32_t	dw ;
} mfbsp_GPIO_t ;

/*
 * Macro for register access.
 * n - MFBSP number or DMA channel number
 */
#define MFBSP_RX(n)				MC_R(0x7000 + ( n << 8 ))
#define MFBSP_TX(n)				MC_R(0x7000 + ( n << 8 ))
#define MFBSP_CSR(n)			MC_RT(0x7004 + ( n << 8 ), mfbsp_csr_t)
#define MFBSP_DIR(n)			MC_RT(0x7008 + ( n << 8 ), mfbsp_dir_t)
#define MFBSP_GPIO(n)			MC_RT(0x700C + ( n << 8 ), mfbsp_GPIO_t)
#define MFBSP_RX_DATA_IRQ(n)	(MC1892_IRQ_MFBSP0_RX_DATA + ( n << 2 ) )
#define MFBSP_TX_DATA_IRQ(n)	(MC1892_IRQ_MFBSP0_TX_DATA + ( n << 2 ) )

#endif
