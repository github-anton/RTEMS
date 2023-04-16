/*
 * Multicore for mc30sf6 board.
 */
#ifndef LIBBSP_MIPS_MC_IO_H_
#define LIBBSP_MIPS_MC_IO_H_

#include <bsp/config.h>
#include <stdint.h>

/* Taken from mcLinux multicore.h */
#define CSR_CLKMASK     0x7f
#define CSR_CLKSHIFT	0x0
#define QSTR_QTY        4

typedef volatile unsigned   mc_reg_t ;

/* Taken from io-mct02.h */
#define MC_BASE_ADDR	0xB82F0000UL
//#define MC_R(a)		*(mc_reg_t*)(MC_BASE_ADDR + (a))

#define MC_RA(off)    ((mc_reg_t*)(MC_BASE_ADDR + (off)))
#define MC_R(off)     (*MC_RA(off))
#define MC_RTA(off, type)      ((type*)(MC_BASE_ADDR + (off)))
#define MC_RT(off, type)       (*MC_RTA(off, type))

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

#define MC_RISC_IRQ_MASK    MC_R(0xA018)    /* Global IRQ Mask. If respective bit is 0, then IRQ is masked! */

#define MC_QUARTZ_CLOCK_FREQ ((CONFIG_MULTICORE_CLOCKFREQ_MHZ * 1000000) / 2)
#define MC_FREQUENCY_MULTIPLIER ((MC_CRPLL & CSR_CLKMASK) >> CSR_CLKSHIFT)

// BSP_CLOCK_RATE must be 100000000
#define CPU_CLOCK_RATE  ( MC_FREQUENCY_MULTIPLIER * MC_QUARTZ_CLOCK_FREQ )
#define CPU_CLOCK_RATE_MHZ  ( CPU_CLOCK_RATE / 1000000 )

/*
 * Base IO for UART
 */
#define COM1_BASE_IO  0x3000
#define COM2_BASE_IO  0x3400

/* 
 * chip_divisor = 16 in Multicore Linux
 */
#define COM_IO_BASE_ADDRESS   MC_BASE_ADDR
#define COM_IO_REG_SHIFT    0x2

#endif
