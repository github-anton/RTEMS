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

#ifndef ASM

#include <bspopts.h>
#include <bsp/default-initial-extension.h>

#include <rtems.h>
#include <rtems/iosupp.h>
#include <rtems/console.h>
#include <rtems/clockdrv.h>
#include <bsp/config.h>
#include <bsp/multicore.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BSP_FEATURE_IRQ_EXTENSION
#define BSP_SHARED_HANDLER_SUPPORT  1
#define BSP_MAXIMUM_DEVICES         16
	
// Maximum length of device file path
#define BSP_DEV_PATH_MAX_LEN		32

#define BSP_PRINTK_PORT             0
#define BSP_GDB_STUB_PORT           1    

#define BSP_CLOCK_RATE				CPU_CLOCK_RATE

void bsp_disable_irq(int irq) ;
void bsp_enable_irq(int irq) ;
rtems_status_code bsp_interrupt_catch( rtems_isr_entry, int, rtems_isr_entry *) ;

/*
 * Prototypes for methods called from .S for dependency tracking
 */
void init_tlb(void) ;
void resettlb(int i) ;   // not exits yet

/* GDB-stub over UART support ruoutines */
int gdb_stub_init(void) ;

/* Console I/O support routines. */
void bsp_com_outch_polled(int minor, char ch) ;
int bsp_com_inch_polled( int minor ) ;
int bsp_com_inch_nonblocking_polled( int minor ) ;

#ifdef __cplusplus
}
#endif

#endif  // ASM

#endif  // LIBBSP_MIPS_MCT03PEM_BSP_H
