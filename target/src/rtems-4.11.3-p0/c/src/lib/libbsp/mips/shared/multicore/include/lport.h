#ifndef LIBBSP_MIPS_MFBSP_LPORT_H_
#define LIBBSP_MIPS_MFBSP_LPORT_H_

#include <rtems/io.h>
#include <sys/ioctl.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LPORT_FLUSH		3
	
rtems_device_driver lport_init(	rtems_device_major_number major, rtems_device_minor_number minor, void *arg ) ;
rtems_device_driver lport_open(	rtems_device_major_number major, rtems_device_minor_number minor, void *arg ) ;
rtems_device_driver lport_close( rtems_device_major_number major, rtems_device_minor_number minor, void *arg ) ;
rtems_device_driver lport_read(	rtems_device_major_number major, rtems_device_minor_number minor, void *arg ) ;
rtems_device_driver lport_write( rtems_device_major_number major, rtems_device_minor_number minor, void *arg ) ;
rtems_device_driver lport_ctl( rtems_device_major_number major, rtems_device_minor_number minor, void *arg ) ;

#ifdef __cplusplus
}
#endif

#define LPORT_DRIVER_TABLE_ENTRY { lport_init, lport_open, lport_close, lport_read, lport_write, lport_ctl }

#endif
