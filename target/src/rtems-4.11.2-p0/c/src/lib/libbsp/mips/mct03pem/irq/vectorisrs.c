/*
 *  Based on BSP for MIPS malta and sources mct03pem for Linux
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

#include <rtems.h>
#include <stdlib.h>
#include <bsp/irq-generic.h>
#include <bsp.h>
#include <bsp/irq.h>

void mips_default_isr( int vector );
void mips_vector_isr_handlers( CPU_Interrupt_frame *frame );
int bsp_get_qstr_irq(int qstr_no) ;
int bsp_get_qstr_no(int irq) ;
int bsp_get_qstr_bit(int irq) ;
mc_reg_t *bsp_get_qstr_addr(int qstr_no) ;
mc_reg_t *bsp_get_maskr_addr(int qstr_no) ;

#include <rtems/bspIo.h>  /* for printk */

#if 0
#define DEBUG
#endif

#include <bsp/aux.h>

void mips_vector_isr_handlers( CPU_Interrupt_frame *frame )
{
    unsigned int sr;
    unsigned int cause;
    unsigned int pending;
    int i ;

    mips_get_sr( sr );
    mips_get_cause( cause );

    pending = (cause & sr & 0xff00) >> CAUSE_IPSHIFT;
  
    /* SW Bits */
    /*if ( pending & 0x01) {
        printk("Pending IRQ_SW0 0x%x\n", pending );
    }

    if ( pending & 0x02) {
        printk("Pending IRQ_SW1 0x%x\n", pending );
    }
    */
  
    // Handle the QSTR interrupts
    for (i = 0; i < 5; i++) {
        if ( pending & ( 0x04 << i ) ) {
            int qstr_irq ;
            while((qstr_irq = bsp_get_qstr_irq(i)) >= 0) {
                bsp_interrupt_handler_dispatch( qstr_irq ) ;
            }
        }
    }
  
    // Handle the ticker interupt
    if ( pending & 0x80 ) {
        bsp_interrupt_handler_dispatch( MC1892_INT_TICKER );
    }
}

void mips_default_isr( int vector )
{
  unsigned int sr;
  unsigned int cause;

  mips_get_sr( sr );
  mips_get_cause( cause );

  printk( "Unhandled isr exception: vector 0x%02x, cause 0x%08X, sr 0x%08X\n",
      vector, cause, sr );

  while(1);      /* Lock it up */

  rtems_fatal_error_occurred(1);
}

/*
 * Return -1 when there is no more IRQ bits in respective QSTR register
 */
int bsp_get_qstr_irq(int qstr_no) {
    static int irq_bit ;
    static int prev_qstr_no ;
    mc_reg_t *p_qstr = bsp_get_qstr_addr(qstr_no);
    int qstr_irq_base = bsp_get_qstr_irq_start(qstr_no);
    mc_reg_t *p_maskr = bsp_get_maskr_addr(qstr_no) ;
    
    if ( p_qstr == NULL || p_maskr == NULL || qstr_irq_base == -1 ) {
        return -1 ;
    }
    
    // Reset starting IRQ point when switch to the next QSTRX register
    if (qstr_no != prev_qstr_no) {
        prev_qstr_no = qstr_no ;
        irq_bit = 0 ;
    }
    
    // Reset starting IRQ point when it out of range
    if(irq_bit >= sizeof(mc_reg_t)*8) {
        irq_bit = 0 ;
    }
    
    for (; irq_bit < (sizeof(mc_reg_t)*8); irq_bit++) {
        if ( ( (*p_qstr) >> irq_bit ) & 1 ) {
            if ( ( (*p_maskr) >> irq_bit ) & 1 ) {
                DTRACEK("QSTR%i bit%i, IRQ%i occured\n", qstr_no, irq_bit, irq_bit + qstr_irq_base) ;
                return ( irq_bit + qstr_irq_base ) ;
            } else {
                DTRACEK("QSTR%i bit%i, IRQ%i occured, masked\n", qstr_no, irq_bit, irq_bit + qstr_irq_base) ;
            }
        }
    }
    DTRACEK("QSTR IRQ bit not found.\n") ;
    return -1 ;
}

int bsp_get_qstr_no(int irq) {
    ////DTRACEK("irq=%i, MC1892_QSTR0_IRQ_START=%i, sizeof(mc_reg_t)=%i\n", irq, MC1892_QSTR0_IRQ_START, sizeof(mc_reg_t)) ;
    return (irq - (MC1892_QSTR0_IRQ_START)) / (sizeof(mc_reg_t)*8) ;
}

int bsp_get_qstr_bit(int irq) {
    return (irq - (MC1892_QSTR0_IRQ_START)) % (sizeof(mc_reg_t)*8) ;
}

void bsp_disable_irq(int irq) {
    int qstr_no = bsp_get_qstr_no(irq) ;
    mc_reg_t *p_maskr = bsp_get_maskr_addr(qstr_no) ;
    int qstr_bit = bsp_get_qstr_bit(irq) ;
    
    if(p_maskr != NULL) {
        *p_maskr = (0xffffffff ^ (1 << qstr_bit)) & (*p_maskr) ;
    }
    
    DTRACEK("irq%i, MASKR%i=0x%x bit%i\n", irq, qstr_no, *p_maskr, qstr_bit) ;
}

void bsp_enable_irq(int irq) {
    int qstr_no = bsp_get_qstr_no(irq) ;
    mc_reg_t *p_maskr = bsp_get_maskr_addr(qstr_no) ;
    int qstr_bit = bsp_get_qstr_bit(irq) ;
    
    if(p_maskr != NULL) {
        *p_maskr = (1 << qstr_bit) | (*p_maskr) ;
    }
    
    DTRACEK("irq%i, MASKR%i=0x%x bit%i\n", irq, qstr_no, *p_maskr, qstr_bit) ;
}

mc_reg_t *bsp_get_qstr_addr(int qstr_no) {
    mc_reg_t *p_qstr ;
    switch( qstr_no ) {
        case 0:
            p_qstr = &MC_QSTR0 ;
            break;
        case 1:
            p_qstr = &MC_QSTR1 ;
            break;
        case 2:
            p_qstr = &MC_QSTR2 ;
            break;
        case 3:
            p_qstr = &MC_QSTR3 ;
            break;
        case 4:
            p_qstr = &MC_QSTR4 ;
            break;
        default:
            p_qstr = NULL ;
    }
    return p_qstr ;
}

mc_reg_t *bsp_get_maskr_addr(int maskr_no) {
    mc_reg_t *p_maskr ;
    switch( maskr_no ) {
        case 0:
            p_maskr = &MC_MASKR0 ;
            break;
        case 1:
            p_maskr = &MC_MASKR1 ;
            break;
        case 2:
            p_maskr = &MC_MASKR2 ;
            break;
        case 3:
            p_maskr = &MC_MASKR3 ;
            break;
        case 4:
            p_maskr = &MC_MASKR4 ;
            break;
        default:
            p_maskr = NULL ;
    }
    return p_maskr ;
}

int bsp_get_qstr_irq_start(int qstr_no) {
    int qstr_irq_base ;
    switch( qstr_no ) {
        case 0:
            qstr_irq_base = MC1892_QSTR0_IRQ_START ;
            break;
        case 1:
            qstr_irq_base = MC1892_QSTR1_IRQ_START ;
            break;
        case 2:
            qstr_irq_base = MC1892_QSTR2_IRQ_START ;
            break;
        case 3:
            qstr_irq_base = MC1892_QSTR3_IRQ_START ;
            break;
        case 4:
            qstr_irq_base = MC1892_QSTR4_IRQ_START ;
            break;
        default:
            return qstr_irq_base = -1 ;
    }
    return qstr_irq_base ;
}
