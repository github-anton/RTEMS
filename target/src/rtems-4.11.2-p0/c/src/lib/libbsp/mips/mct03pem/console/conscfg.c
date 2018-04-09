/**
 *  @file
 *
 *  This file contains the libchip configuration information
 *  to instantiate the libchip driver for the serial ports.
 */

/*
 *  COPYRIGHT (c) 1989-2012.
 *  On-Line Applications Research Corporation (OAR).
 *  Copyright 2018, Anton Ermakov
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.org/license/LICENSE.
 * 
 *  Adopted for Elvees MCT-03PEM board by Ermakov Anton, 2017
 */

#include <unistd.h> /* write */

#include <bsp.h>
#include <libchip/serial.h>
#include <libchip/ns16550.h>
#include <rtems/pci.h>
#include <bsp/irq.h>
#include <bsp/aux.h>
#include <libchip/ns16550_p.h>

#if 1
#define COM_CONSOLE_FUNCTIONS  &ns16550_fns_polled
#else
#define COM_CONSOLE_FUNCTIONS  &ns16550_fns
#endif

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

static uint8_t com_get_register(uintptr_t addr, uint8_t i);
static void com_set_register(uintptr_t addr, uint8_t i, uint8_t val);

/* Compare UART registers, first column taken from the mcLinux */
/*static const u8 au_io_in_map[] = {
	UART_RX:  0 - Linux, NS16550_RECEIVE_BUFFER: 0 - RTEMS
	UART_IER: 1 - Linux, NS16550_INTERRUPT_ENABLE: 1 - RTEMS
	UART_IIR: 2 - Linux, NS16550_INTERRUPT_ID: 2 - RTEMS
	UART_LCR: 3 - Linux, NS16550_LINE_CONTROL: 3 - RTEMS
	UART_MCR: 4 - Linux, NS16550_MODEM_CONTROL: 4 - RTEMS
	UART_LSR: 5 - Linxu, NS16550_LINE_STATUS: 5 - RTEMS
	UART_MSR: 6 - Linux, NS16550_MODEM_STATUS: 6 - RTEMS
} ;
*/

uint8_t com_get_register(uintptr_t addr, uint8_t i)
{
  uint8_t val;
  volatile uint8_t *ptr;
  ptr = (volatile uint8_t *) COM_IO_BASE_ADDRESS;
  ptr += addr;
  // Recalculate register address
  ptr += i << COM_IO_REG_SHIFT ;
  val = *ptr;

  return val;
}

void com_set_register(uintptr_t addr, uint8_t i, uint8_t val)
{
  volatile uint8_t *ptr;

  ptr = (volatile uint8_t *) COM_IO_BASE_ADDRESS;
  ptr += addr;
  // Recalculate register address
  ptr += i << COM_IO_REG_SHIFT ;
  *ptr = val;
}

console_tbl     Console_Configuration_Ports[] = {
  {
    "/dev/tty0",                          /* sDeviceName */
    SERIAL_NS16550,                       /* deviceType */
    COM_CONSOLE_FUNCTIONS,                /* pDeviceFns */
    NULL,                                 /* deviceProbe, assume it is there */
    NULL,                                 /* pDeviceFlow */
    16,                                   /* ulMargin */
    8,                                    /* ulHysteresis */
    (void *) 115200,        /* Baud Rate */ /* pDeviceParams */
    COM1_BASE_IO,                         /* ulCtrlPort1 */
    0x00000000,                           /* ulCtrlPort2 is ignored */
    COM1_BASE_IO,                         /* ulDataPort */
    com_get_register,                     /* getRegister */
    com_set_register,                     /* setRegister */
    NULL,/* unused */                     /* getData */
    NULL,/* unused */                     /* setData */
    0x0,                                  /* ulClock, will be initialized at runtime in bsp_start() 
                                             = MC_FREQUENCY_MULTIPLIER() * MC_QUARTZ_CLOCK_FREQ */
    MC1892_IRQ_TTY0                       /* ulIntVector -- base for port */
  },
  {
    "/dev/tty1",                          /* sDeviceName */
    SERIAL_NS16550,                       /* deviceType */
    COM_CONSOLE_FUNCTIONS,                /* pDeviceFns */
    NULL,                                 /* deviceProbe, assume it is there */
    NULL,                                 /* pDeviceFlow */
    16,                                   /* ulMargin */
    8,                                    /* ulHysteresis */
    (void *) 115200,        /* Baud Rate */ /* pDeviceParams */
    COM2_BASE_IO,                         /* ulCtrlPort1 */
    0x00000000,                           /* ulCtrlPort2 is ignored */
    COM2_BASE_IO,                         /* ulDataPort */
    com_get_register,                     /* getRegister */
    com_set_register,                     /* setRegister */
    NULL,/* unused */                     /* getData */
    NULL,/* unused */                     /* setData */
    0x0,                                  /* ulClock, will be initialized at runtime in bsp_start() 
                                             = MC_FREQUENCY_MULTIPLIER() * MC_QUARTZ_CLOCK_FREQ */
    MC1892_IRQ_TTY1                       /* ulIntVector -- base for port */
  },
};

/*
 *  Define a variable that contains the number of statically configured
 *  console devices.
 */
unsigned long  Console_Configuration_Count = \
    (sizeof(Console_Configuration_Ports)/sizeof(console_tbl));
