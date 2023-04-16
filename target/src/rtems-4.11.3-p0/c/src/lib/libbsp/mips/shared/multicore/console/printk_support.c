/**
 *  @file
 *
 *  This file contains a stub for the required printk support.
 */

/*
 *  COPYRIGHT (c) 1989-2012.
 *  On-Line Applications Research Corporation (OAR).
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.org/license/LICENSE.
 */

#include <rtems.h>
#include <bsp.h>
#include <libchip/serial.h>
#include <libchip/ns16550.h>

rtems_device_minor_number         BSPPrintkPort = BSP_PRINTK_PORT ;     // 0

void BSP_com_outch(char ch);
int BSP_com_inch( void );

/*
 *  Following assume all are ns16650
 */
void bsp_com_outch_polled(int minor, char ch)
{
    console_tbl                   *cptr;
    
    static int initialized ;
    
    cptr = &Console_Configuration_Ports[minor];
  
    ns16550_outch_polled( cptr, ch );
}

int bsp_com_inch_polled( int minor )
{
    int           result;
    console_tbl   *cptr;

    cptr = &Console_Configuration_Ports[minor];

    do {
        result = ns16550_inch_polled( cptr );
    } while (result == -1);

    return result;
}

int bsp_com_inch_nonblocking_polled( int minor ) {
    int           result;
    console_tbl   *cptr;

    cptr = &Console_Configuration_Ports[minor];
    
    result = ns16550_inch_polled( cptr );
    
    return result ;
}

void BSP_com_outch(char ch) {
    
    bsp_com_outch_polled( BSPPrintkPort, ch ) ;
}

int BSP_com_inch(void) {
    
    return bsp_com_inch_polled(BSPPrintkPort) ;
}

BSP_output_char_function_type     BSP_output_char = BSP_com_outch;
BSP_polling_getchar_function_type BSP_poll_char = BSP_com_inch;
