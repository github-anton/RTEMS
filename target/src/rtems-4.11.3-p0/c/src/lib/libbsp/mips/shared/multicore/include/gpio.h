#ifndef LIBBSP_MIPS_MFBSP_GPIO_H_
#define LIBBSP_MIPS_MFBSP_GPIO_H_

#include <rtems/io.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * ioctl() commands definition.
 */
#define GPIO_SET_DIRECTION		3
#define GPIO_GET_DIRECTION		4

/*
 * Driver routinies declaration.
 */
rtems_device_driver gpio_init(	rtems_device_major_number major, rtems_device_minor_number minor, void *arg ) ;
rtems_device_driver gpio_open(	rtems_device_major_number major, rtems_device_minor_number minor, void *arg ) ;
rtems_device_driver gpio_close( rtems_device_major_number major, rtems_device_minor_number minor, void *arg ) ;
rtems_device_driver gpio_read(	rtems_device_major_number major, rtems_device_minor_number minor, void *arg ) ;
rtems_device_driver gpio_write( rtems_device_major_number major, rtems_device_minor_number minor, void *arg ) ;
rtems_device_driver gpio_ctl( rtems_device_major_number major, rtems_device_minor_number minor, void *arg ) ;

#ifdef __cplusplus
}
#endif

/*
 * Driver table definition.
 */
#define GPIO_DRIVER_TABLE_ENTRY { gpio_init, gpio_open, gpio_close, gpio_read, gpio_write, gpio_ctl }

#endif 
