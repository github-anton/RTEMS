#ifndef LIBBSP_MIPS_DMA_H
#define LIBBSP_MIPS_DMA_H

#include <bsp/multicore.h>
#include <sys/types.h>

/* Taken from io-elvees.h */
/* Регистр CSR для каналов DMA */
#define MC_DMA_CSR_RUN      0x00000001  /* Состояние работы канала DMA */
#define MC_DMA_CSR_DIR      0x00000002  /* Направление передачи для каналов MEM_CH */
#define MC_DMA_CSR_WN(n)    ((n) << 2)  /* Установка длины пачки */
#define MC_DMA_CSR_EN64     0x00000040  /* Передача 64-разрядных данных (для MEM_CH) */
#define MC_DMA_CSR_IPD      0x00000040  /* Запрет прерывания по запросу от порта при выключенном канале DMA(RUN=0) */
#define MC_DMA_CSR_START_DSP    0x00000080  /* Разрешение запуска DSP (для MEM_CH) */
#define MC_DMA_CSR_MODE     0x00000100  /* Режим модификация адреса регистра IR0 */
#define MC_DMA_CSR_2D       0x00000200  /* Режим модификации адреса регистра IR1 */
#define MC_DMA_CSR_CHEN     0x00001000  /* Признак разрешения самоинициализации */
#define MC_DMA_CSR_IM       0x00002000  /* Маска прерывания при окончании передачи блока */
#define MC_DMA_CSR_END      0x00004000  /* Признак завершения передачи блока данных */
#define MC_DMA_CSR_DONE     0x00008000  /* Признак завершения передачи цепочки блоков данных */
#define MC_DMA_CSR_WCX_MASK 0xffff0000  /* Маска счетчика слов */
#define MC_DMA_CSR_WCX(n)   ((n) << 16) /* Установка счетчика слов */

#define MC_CLKEN_SWIC(n)    (1 << (24+(n))) /* Включение тактовой частоты SWIC */

/* Псевдорегистр управления RUN */
#define MC_DMA_RUN      0x00000001  /* Управление битом RUN */

// DMA_PORT_CH_CSR register
typedef union __attribute__((packed,aligned(4))) {
	struct {
		uint16_t  run			:1;    // Состояние работы канала DMA
		uint16_t  rsrv_1		:1;    // Резерв
		uint16_t  wn			:4;    // Число слов данных (пачка), которое передается за одно предоставление прямого доступа
		uint16_t  ipd			:1;    // Запрет прерывания по запросу от порта при выключенном канале DMA (RUN=0).
		uint16_t  rsrv_2		:5;    // Резерв
		uint16_t  chen		    :1;    // Разрешение выполнения очередной процедуры	самоинициализации
		uint16_t  im			:1;    // Маска разрешение установки признака END
		uint16_t  end			:1;    // Признак завершения передачи блока данных (при IM=1)
		uint16_t  done		    :1;    // Признак завершения передачи блока данных (при CHEN=0)
		uint16_t  wcx			:16;   // Число слов данных, которые должен передать канал DMA
	} b ;
	uint32_t    dw ;
} dma_port_ch_csr_t ;

typedef struct __attribute__((packed,aligned(4))) {
	dma_port_ch_csr_t		csr ;	// Регистр управления и состояния
	mc_reg_t				cp ;	// Регистр указателя цепочки
	mc_reg_t				ir;		// Регистр индекса
	dma_port_ch_csr_t		run;	// На запись:Псевдорегистр управления состоянием бита RUN регистра CSR0
   									// На чтение: Регистр управления и состояния без сброса битов “END” и ”DONE”
} dma_port_ch_t ;

#define DMA_PORT_CSR(off)	MC_RT((off), dma_port_ch_csr_t)
#define DMA_PORT_CP(off)	MC_RT((off) + 0x4, uint32_t)	// Регистр указателя цепочки канала
#define DMA_PORT_IR(off)	MC_RT((off) + 0x8, uint32_t)	// Индексный регистр внешней памяти канала
#define DMA_PORT_RUN(off)	MC_RT((off) + 0xC, dma_port_ch_csr_t)	// Псевдорегистр управления состоянием бита RUN

#define DMA_MFBSP_PORT_ADDR(n)	(0x7800 + ( n << 6 ))

#define DMA_MFBSP_CH_CSR(n)		MC_RT(0x7800 + ( n << 6 ), dma_port_ch_csr_t)
#define DMA_MFBSP_CH_IR(n)		MC_RT(0x7808 + ( n << 6 ), uint32_t)

#define DMA_MFBSP_DATA_IRQ(n)		(MC1892_IRQ_MFBSP0_DMA + ( n << 2 ) )

typedef struct {
	u_char	*data ;
	int		w_idx ; /* First free data index. */
	int		r_idx ; /* First allocated data index. */
	size_t	size ;	// Maximum length of the data
} dma_io_buf_t ;

// Double IO buffer for DMA use
typedef struct {
	dma_io_buf_t half[2] ;
	int	w_idx ; /* half for writing */
	int r_idx ; /* hlaf for reading */
	u_int port_off ;	//	DMA port offset address (CSR)
} dma_dbl_io_buf_t ;

/*
 * DMA IO double buffer primitives.
 */
int dma_dbl_io_buf_read( dma_dbl_io_buf_t *p_dbl_buf, void *p_data, size_t size ) ;
int dma_dbl_io_buf_write( dma_dbl_io_buf_t *p_dbl_buf, void *p_data, size_t size ) ;
int dma_start_rx_ch_if_needed(dma_dbl_io_buf_t *p_dbl_buf, size_t len, size_t bs) ;
int dma_start_tx_ch_if_needed(dma_dbl_io_buf_t *p_dbl_buf) ;
void dma_dbl_io_buf_swap_buf(dma_dbl_io_buf_t *p_dbl_buf) ;
dma_io_buf_t *dma_dbl_io_buf_get_r_buf (dma_dbl_io_buf_t *p_dbl_buf) ;
dma_io_buf_t *dma_dbl_io_buf_get_w_buf (dma_dbl_io_buf_t *p_dbl_buf) ;

/*
 * DMA IO buffer primitives.
 */
size_t dma_io_buf_get_fill( dma_io_buf_t *p_buf ) ;
size_t dma_io_buf_get_avail( dma_io_buf_t *p_buf ) ;
void dma_io_buf_reset(dma_io_buf_t *p_buf) ;

/*
 * Map virtual address to physical address.
 */
static inline unsigned mips_virt_to_phys (unsigned virtaddr)
{
	switch (virtaddr >> 28 & 0xE) {
	default:  return virtaddr + 0x40000000;		/* kuseg */
	case 0x8: return virtaddr - 0x80000000;		/* kseg0 */
	case 0xA: return virtaddr - 0xA0000000;		/* kseg1 */
	case 0xC: return virtaddr;			/* kseg2 */
	case 0xE: return virtaddr;			/* kseg3 */
	}
}

/*
 * Get uncached addres for DMA use.
 */
static inline void* mips_get_uncached_addr(void *cached_addr) {
    
    return (void*)(((uint32_t)cached_addr & 0xFFFFFFF ) | 0xA0000000) ;
}

#endif
