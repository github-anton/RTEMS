/**
 *  @file
 *  
 *  This routine starts the application.  It includes application,
 *  board, and monitor specific initialization and configuration.
 *  The generic CPU dependent initialization has been performed
 *  before this routine is invoked.
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

#include <string.h>

#include <bsp.h>
#include <libcpu/isr_entries.h>
#include <bsp/irq-generic.h>
#include <libchip/serial.h>
#include <libchip/ns16550.h>

void bsp_start( void );
uint32_t bsp_clicks_per_microsecond;

/*
 *  bsp_start
 *
 *  This routine does the bulk of the system initialization.
 */
void bsp_start( void )
{
    int i = 0 ;

    bsp_clicks_per_microsecond = CPU_CLOCK_RATE_MHZ ;
  
    /* Set up COM ports clock rate must be 100000000 */
    for (i = 0; i < Console_Configuration_Count; i++) {
        Console_Configuration_Ports[i].ulClock = BSP_CLOCK_RATE ;
    }
    
    bsp_interrupt_initialize();
    
    /* Enable interrupts from Multicore Interrupt Controller
     */
#if defined(CONFIG_MCT03P)
    // Enable UARTx, WDT, ITx; disable nIRQx,
    MC_MASKR0 = 0x700030 ;
    // Enable DMA_MEM_CHx
    MC_MASKR1 = 0x909 ;
    // Enable SWICx, SW_RX_DES_CHx, SW_RX_DAT_CHx, SW_TX_DES_CHx, SW_TX_DAT_CHx
    MC_MASKR2 = 0xf7f7f7f7 ;
    // Enable Hemming
    MC_MASKR3 = 0xbf ;
    // Enable MFBSPx
    MC_MASKR4 = 0x3737 ;
	
#elif defined(CONFIG_MC30SF6)
    // Global CPU GSPW IRQ Mask
    // Enable SPW: Connect, Disconnect, Time, Int, Ack etc.
    MC_RISC_IRQ_MASK = 0x1FFF ;
    // Enable UARTx, WDT, ITx; disable nIRQx; enable SPW DMA TX/RX, Link, Time, Error
    MC_MASKR0 = 0xE17F0030 ;
    // Disable all
    MC_MASKR1 = 0x0 ;
    // Disable all
    MC_MASKR2 = 0x0 ;
    // Enable Hamming: ACC, DDR0-1 DSP0-1, MPORT, DCACHE, ICACHE, CRAM(3:0)
    MC_MASKR3 = 0x800033BF ;
	
#elif defined(CONFIG_MC30MPO2)
	// Global CPU GSPW IRQ Mask
	// Enable SPW: Connect, Disconnect, Time, Int, Ack etc.
    MC_RISC_IRQ_MASK = 0x1FFF ;
    // Enable UARTx, WDT, ITx; disable nIRQx; enable SPW DMA TX/RX, Link, Time, Error
    MC_MASKR0 = 0xE17F0030 ;
    // Disable all
    MC_MASKR1 = 0x0 ;
    // Disable all
    MC_MASKR2 = 0x0 ;
    // Enable Hamming: ACC, DDR0-1 DSP0-1, MPORT, DCACHE, ICACHE, CRAM(3:0)
    MC_MASKR3 = 0x800033BF ;
#endif
	
}
