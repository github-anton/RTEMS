/*
 * MCT-03PEM-6U Interrupt Defenitions
 * 
 * Copyright 2018, Anton Ermakov
 * 
 * The authors hereby grant permission to use, copy, modify, distribute,
 * and license this software and its documentation for any purpose, provided
 * that existing copyright notices are retained in all copies and that this
 * notice is included verbatim in any distributions. No written agreement,
 * license, or royalty fee is required for any of the authorized uses.
 * Modifications to this software may be copyrighted by their authors
 * and need not follow the licensing terms described here, provided that
 * the new terms are clearly indicated on the first page of each file where
 * they apply.
*/ 

#ifndef LIBBSP_MIPS_MCT03PEM_IRQ_H
#define LIBBSP_MIPS_MCT03PEM_IRQ_H

#ifndef ASM
  #include <rtems.h>
  #include <rtems/irq.h>
  #include <rtems/irq-extension.h>
  #include <rtems/score/mips.h>
  #include <bsp/config.h>
#endif

#define BSP_INTERRUPT_VECTOR_MIN        0

#define MC1892_CPU_INT_START            (MIPS_INTERRUPT_BASE+0)
#define MC1892_CPU_INT_SW0              (MC1892_CPU_INT_START+0)
#define MC1892_CPU_INT_SW2              (MC1892_CPU_INT_START+1)
#define MC1892_CPU_INT0                 (MC1892_CPU_INT_START+2)
#define MC1892_CPU_INT1                 (MC1892_CPU_INT_START+3)
#define MC1892_CPU_INT2                 (MC1892_CPU_INT_START+4)
#define MC1892_CPU_INT3                 (MC1892_CPU_INT_START+5)
#define MC1892_CPU_INT4                 (MC1892_CPU_INT_START+6)
#define MC1892_CPU_INT5                 (MC1892_CPU_INT_START+7)
#define MC1892_CPU_INT_LAST             (MC1892_CPU_INT5)

#define MC1892_QSTR0_IRQ_START          (MC1892_CPU_INT_LAST+1)
#define MC1892_QSTR0_IRQ_0              (MC1892_QSTR0_IRQ_START+0)
#define MC1892_QSTR0_IRQ_1              (MC1892_QSTR0_IRQ_START+1)
#define MC1892_QSTR0_IRQ_2              (MC1892_QSTR0_IRQ_START+2)
#define MC1892_QSTR0_IRQ_3              (MC1892_QSTR0_IRQ_START+3)
#define MC1892_QSTR0_IRQ_4              (MC1892_QSTR0_IRQ_START+4)
#define MC1892_QSTR0_IRQ_5              (MC1892_QSTR0_IRQ_START+5)
#define MC1892_QSTR0_IRQ_16             (MC1892_QSTR0_IRQ_START+16)
#define MC1892_QSTR0_IRQ_17             (MC1892_QSTR0_IRQ_START+17)
#define MC1892_QSTR0_IRQ_18             (MC1892_QSTR0_IRQ_START+18)
#define MC1892_QSTR0_IRQ_19             (MC1892_QSTR0_IRQ_START+19)
#define MC1892_QSTR0_IRQ_20             (MC1892_QSTR0_IRQ_START+20)
#define MC1892_QSTR0_IRQ_21             (MC1892_QSTR0_IRQ_START+21)
#define MC1892_QSTR0_IRQ_22             (MC1892_QSTR0_IRQ_START+22)
#define MC1892_QSTR0_IRQ_23             (MC1892_QSTR0_IRQ_START+23)
#define MC1892_QSTR0_IRQ_24             (MC1892_QSTR0_IRQ_START+24)
#define MC1892_QSTR0_IRQ_25             (MC1892_QSTR0_IRQ_START+25)
#define MC1892_QSTR0_IRQ_26             (MC1892_QSTR0_IRQ_START+26)
#define MC1892_QSTR0_IRQ_27             (MC1892_QSTR0_IRQ_START+27)
#define MC1892_QSTR0_IRQ_28             (MC1892_QSTR0_IRQ_START+28)
#define MC1892_QSTR0_IRQ_29             (MC1892_QSTR0_IRQ_START+29)
#define MC1892_QSTR0_IRQ_30             (MC1892_QSTR0_IRQ_START+30)
#define MC1892_QSTR0_IRQ_31             (MC1892_QSTR0_IRQ_START+31)
#define MC1892_QSTR0_IRQ_LAST           (MC1892_QSTR0_IRQ_31)

#define MC1892_QSTR1_IRQ_START          (MC1892_QSTR0_IRQ_LAST+1)
#define MC1892_QSTR1_IRQ_0              (MC1892_QSTR1_IRQ_START+0)
#define MC1892_QSTR1_IRQ_3              (MC1892_QSTR1_IRQ_START+3)
#define MC1892_QSTR1_IRQ_8              (MC1892_QSTR1_IRQ_START+8)
#define MC1892_QSTR1_IRQ_11             (MC1892_QSTR1_IRQ_START+11)
#define MC1892_QSTR1_IRQ_31             (MC1892_QSTR1_IRQ_START+31)
#define MC1892_QSTR1_IRQ_LAST           (MC1892_QSTR1_IRQ_31)

#define MC1892_QSTR2_IRQ_START          (MC1892_QSTR1_IRQ_LAST+1)
#define MC1892_QSTR2_IRQ_0              (MC1892_QSTR2_IRQ_START+0)
#define MC1892_QSTR2_IRQ_1              (MC1892_QSTR2_IRQ_START+1)
#define MC1892_QSTR2_IRQ_2              (MC1892_QSTR2_IRQ_START+2)
#define MC1892_QSTR2_IRQ_3              (MC1892_QSTR2_IRQ_START+3)
#define MC1892_QSTR2_IRQ_4              (MC1892_QSTR2_IRQ_START+4)
#define MC1892_QSTR2_IRQ_5              (MC1892_QSTR2_IRQ_START+5)
#define MC1892_QSTR2_IRQ_6              (MC1892_QSTR2_IRQ_START+6)
#define MC1892_QSTR2_IRQ_7              (MC1892_QSTR2_IRQ_START+7)
#define MC1892_QSTR2_IRQ_8              (MC1892_QSTR2_IRQ_START+8)
#define MC1892_QSTR2_IRQ_9              (MC1892_QSTR2_IRQ_START+9)
#define MC1892_QSTR2_IRQ_10             (MC1892_QSTR2_IRQ_START+10)
#define MC1892_QSTR2_IRQ_12             (MC1892_QSTR2_IRQ_START+12)
#define MC1892_QSTR2_IRQ_13             (MC1892_QSTR2_IRQ_START+13)
#define MC1892_QSTR2_IRQ_14             (MC1892_QSTR2_IRQ_START+14)
#define MC1892_QSTR2_IRQ_15             (MC1892_QSTR2_IRQ_START+15)
#define MC1892_QSTR2_IRQ_16             (MC1892_QSTR2_IRQ_START+16)
#define MC1892_QSTR2_IRQ_17             (MC1892_QSTR2_IRQ_START+17)
#define MC1892_QSTR2_IRQ_18             (MC1892_QSTR2_IRQ_START+18)
#define MC1892_QSTR2_IRQ_20             (MC1892_QSTR2_IRQ_START+20)
#define MC1892_QSTR2_IRQ_21             (MC1892_QSTR2_IRQ_START+21)
#define MC1892_QSTR2_IRQ_22             (MC1892_QSTR2_IRQ_START+22)
#define MC1892_QSTR2_IRQ_23             (MC1892_QSTR2_IRQ_START+23)
#define MC1892_QSTR2_IRQ_24             (MC1892_QSTR2_IRQ_START+24)
#define MC1892_QSTR2_IRQ_25             (MC1892_QSTR2_IRQ_START+25)
#define MC1892_QSTR2_IRQ_26             (MC1892_QSTR2_IRQ_START+26)
#define MC1892_QSTR2_IRQ_28             (MC1892_QSTR2_IRQ_START+28)
#define MC1892_QSTR2_IRQ_29             (MC1892_QSTR2_IRQ_START+29)
#define MC1892_QSTR2_IRQ_30             (MC1892_QSTR2_IRQ_START+30)
#define MC1892_QSTR2_IRQ_31             (MC1892_QSTR2_IRQ_START+31)
#define MC1892_QSTR2_IRQ_LAST           (MC1892_QSTR2_IRQ_31)

#define MC1892_QSTR3_IRQ_START          (MC1892_QSTR2_IRQ_LAST+1)
#define MC1892_QSTR3_IRQ_0              (MC1892_QSTR3_IRQ_START+0)
#define MC1892_QSTR3_IRQ_1              (MC1892_QSTR3_IRQ_START+1)
#define MC1892_QSTR3_IRQ_2              (MC1892_QSTR3_IRQ_START+2)
#define MC1892_QSTR3_IRQ_4              (MC1892_QSTR3_IRQ_START+4)
#define MC1892_QSTR3_IRQ_5              (MC1892_QSTR3_IRQ_START+5)
#define MC1892_QSTR3_IRQ_6              (MC1892_QSTR3_IRQ_START+6)
#define MC1892_QSTR3_IRQ_7              (MC1892_QSTR3_IRQ_START+7)
#define MC1892_QSTR3_IRQ_31             (MC1892_QSTR3_IRQ_START+31)
#define MC1892_QSTR3_IRQ_LAST           (MC1892_QSTR3_IRQ_31)

#define BSP_INTERRUPT_VECTOR_MAX        MC1892_QSTR3_IRQ_LAST


/*
 * Redefine interrupts with more descriptive names.
 * The Generic ones above match the hardware name,
 * where these match the device name.
 */

#define MC1892_INT_TICKER               MC1892_CPU_INT5

#define MC1892_INT_QSTR0                MC1892_CPU_INT0

#define MC1892_IRQ_TTY0                 MC1892_QSTR0_IRQ_4
#define MC1892_IRQ_TTY1                 MC1892_QSTR0_IRQ_5

#define MC1892_IRQ_SW0_LINK             MC1892_QSTR0_IRQ_24
#define MC1892_IRQ_SW0_ERR              MC1892_QSTR0_IRQ_25
//#define MC1892_IRQ_SW0_ERR              MC1892_QSTR0_IRQ_24
//#define MC1892_IRQ_SW0_LINK             MC1892_QSTR0_IRQ_25
#define MC1892_IRQ_SW0_TIME             MC1892_QSTR0_IRQ_26
#define MC1892_IRQ_SW0_RX_DESC          MC1892_QSTR0_IRQ_16
#define MC1892_IRQ_SW0_RX_DATA          MC1892_QSTR0_IRQ_17
#define MC1892_IRQ_SW0_TX_DESC          MC1892_QSTR0_IRQ_18
#define MC1892_IRQ_SW0_TX_DATA          MC1892_QSTR0_IRQ_19

#define MC1892_IRQ_MFBSP0_RX_DATA		MC1892_QSTR2_IRQ_1
#define MC1892_IRQ_MFBSP0_TX_DATA		MC1892_QSTR2_IRQ_2
#define MC1892_IRQ_MFBSP0_DMA			MC1892_QSTR2_IRQ_3
#define MC1892_IRQ_MFBSP1_RX_DATA		MC1892_QSTR2_IRQ_5
#define MC1892_IRQ_MFBSP1_TX_DATA		MC1892_QSTR2_IRQ_6
#define MC1892_IRQ_MFBSP1_DMA			MC1892_QSTR2_IRQ_7
#define MC1892_IRQ_MFBSP2_RX_DATA		MC1892_QSTR2_IRQ_9
#define MC1892_IRQ_MFBSP2_TX_DATA		MC1892_QSTR2_IRQ_10
#define MC1892_IRQ_MFBSP2_DMA			MC1892_QSTR2_IRQ_11
#define MC1892_IRQ_MFBSP3_RX_DATA		MC1892_QSTR2_IRQ_13
#define MC1892_IRQ_MFBSP3_TX_DATA		MC1892_QSTR2_IRQ_14
#define MC1892_IRQ_MFBSP3_DMA			MC1892_QSTR2_IRQ_15

#endif  // LIBBSP_MIPS_MCT03PEM_IRQ_H
