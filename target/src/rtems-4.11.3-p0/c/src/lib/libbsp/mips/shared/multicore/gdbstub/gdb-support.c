/*
 * GDB stub debugging suppot.
 * 
 * Author: Anton Ermakov, 2019
 */

#include <bsp.h> 
#include "gdb_if.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/fcntl.h>

int gdb_stub_fd ;
rtems_device_minor_number   BSPGdbStubPort = BSP_GDB_STUB_PORT ;     // 1

/*
 *  Prototypes for methods assumed to exist by the gdb stub
 */
char getDebugChar (void);
void putDebugChar (char c);

char getDebugChar (void) {
    char c ;
    
    c = bsp_com_inch_polled ( BSPGdbStubPort ) ;
    
    return  c ;
}

void putDebugChar (char c) {
    
    bsp_com_outch_polled ( BSPGdbStubPort, c ) ;
    
}

int gdb_stub_init(void) {
    
    gdb_stub_fd = open("/dev/tty1", O_RDWR) ;
    
    if (gdb_stub_fd == -1) {
        /*
         * If an error occured errno will be set.
         */
        //return -1 ;
    }
    
    /* set up vectoring for gdb */
     mips_gdb_stub_install(-1);
     
    /*
      this is a rough approximation of our memory map.  Yours is
      probably different.  It only needs to be sufficient for the stub
      to know what it can and can't do and where.
    */
    gdbstub_add_memsegment(0         , 0x8000ffff, MEMOPT_READABLE );
    gdbstub_add_memsegment(0x80010000, 0x88000000, MEMOPT_READABLE | MEMOPT_WRITEABLE );
    gdbstub_add_memsegment(0xa0010000, 0xa8000000, MEMOPT_READABLE | MEMOPT_WRITEABLE );
    
    return 0 ;
}
