/*
 * This include file contains some definitions specific to the
 * MIPS MCT-03PEM-6U Board.
*/

/*
 *  COPYRIGHT (c) 1989-2012.
 *  On-Line Applications Research Corporation (OAR).
 *  Copyright 2018, Anton Ermakov
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.org/license/LICENSE.
 */

#ifndef LIBBSP_MIPS_MCT03PEM_BSP_H
#define LIBBSP_MIPS_MCT03PEM_BSP_H

#if 1
#define BSP_ENABLE_CPU_CACHE
#endif

#ifndef ASM

#include <bspopts.h>
#include <bsp/default-initial-extension.h>

#include <rtems.h>
#include <rtems/iosupp.h>
#include <rtems/console.h>
#include <rtems/clockdrv.h>
#include <bsp/mcspw.h>
#include <bsp/cpu.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BSP_FEATURE_IRQ_EXTENSION
#define BSP_SHARED_HANDLER_SUPPORT  1
#define BSP_MAXIMUM_DEVICES         5

/* Taken from mct-03p mcLinux .config */
#define CONFIG_MULTICORE_CLOCKFREQ              10
#define CONFIG_MULTICORE_SWIC_DEF_WORK_SPEED    10
#define CONFIG_MULTICORE_SWIC_START_SPEED       10

/* Taken from mcLinux multicore.h */
#define CSR_CLKMASK	0x3f
#define CSR_CLKSHIFT	0x0
#define MC_QUARTZ_CLOCK_FREQ ((CONFIG_MULTICORE_CLOCKFREQ * 1000000) / 2)
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

typedef volatile unsigned   mc_reg_t ;

/* Taken from io-mct02.h */
#define MC_BASE_ADDR	0xB82F0000UL
#define MC_R(a)		*(mc_reg_t*)(MC_BASE_ADDR + (a))
#define MC_CRPLL	MC_R (0x4000)	/* Control of PLL */
#define MC_CLKEN	MC_R (0x4004)	/* Управление отключением частоты от устройств */
#define MC_MASKR0	MC_R (0x4010)	/* Interrupt mask for MC_QSTR0 */
#define MC_QSTR0	MC_R (0x4014)	/* Interrupt requests from IT, RTT, WDT, UART[3:0], nIRQ[3:0] */
#define MC_MASKR1	MC_R (0x4018)	/* Interrupt mask for MC_QSTR1 */
#define MC_QSTR1	MC_R (0x401C)	/* Interrupt requests от DMA MEM */
#define MC_MASKR2	MC_R (0x4020)	/* Interrupt mask for MC_QSTR2 */
#define MC_QSTR2	MC_R (0x4024)	/* Interrupt requsts from SWIC */
#define MC_MASKR3	MC_R (0x4028)	/* Imterrupt mask for MC_QSTR3 */
#define MC_QSTR3	MC_R (0x402c)	/* Interrupt requsts from Hemming controller */
#define MC_MASKR4	MC_R (0x4038)	/* Interrupt mask for MC_QSTR4 */
#define MC_QSTR4	MC_R (0x403c)	/* Interrupt requsts from MFBSP */
#define MC_FREQUENCY_MULTIPLIER ((MC_CRPLL & CSR_CLKMASK) >> CSR_CLKSHIFT)

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

// BSP_CLOCK_RATE must be 100000000
#define BSP_CLOCK_RATE  ( MC_FREQUENCY_MULTIPLIER * MC_QUARTZ_CLOCK_FREQ )
#define CPU_CLOCK_RATE_MHZ  ( BSP_CLOCK_RATE / 1000000 )

void bsp_disable_irq(int irq) ;
void bsp_enable_irq(int irq) ;

/*
 * Prototypes for methods called from .S for dependency tracking
 */
void init_tlb(void);
void resettlb(int i);   // not exits yet

#ifdef __cplusplus
}
#endif

#endif  // ASM

#endif  // LIBBSP_MIPS_MCT03PEM_BSP_H
