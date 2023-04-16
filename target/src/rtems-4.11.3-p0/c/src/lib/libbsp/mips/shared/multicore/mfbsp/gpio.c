/*
 * RTEMS MFBSP GPIO driver.
 * 
 * Copyright (C) 2021 Ermakov Anton
 * 
 */
#include <rtems.h>
#include <rtems/io.h>
#include <rtems/rtems/intr.h>
#include <bsp.h>
#include <bsp/irq.h>
#include <rtems/libio.h>
#include <errno.h>
#include <bsp/mfbsp.h>
#include <bsp/gpio.h>
#include <stdio.h>

#if 0
#   define DEBUG
#endif

#include <bsp/auxio.h> 

char gpio_dev_path[][BSP_DEV_PATH_MAX_LEN] = {
	"/dev/gpio0",
	"/dev/gpio1",
	"/dev/gpio2",
	"/dev/gpio3",
	"/dev/gpio4",
	"/dev/gpio5"
} ;

/*
 *	RTEMS driver init() function.
 */
rtems_device_driver gpio_init(
	rtems_device_major_number major,
	rtems_device_minor_number minor,
	void *arg
) {
	int i ;
	DTRACEK("BEGIN, major=%i, minor=%i, arg=0x%X\n", major, minor, arg) ;
	
	//
	//	Register GPIO devices in /dev/ filesystem.
	//
	for (i = 0; i < CONFIG_MULTICORE_MFBSP_QTY; i++) {
		
#if 1	
	DTRACEK("&MFBSP_CSR(%i)=0x%X\n", i, &MFBSP_CSR(i)) ;
	DTRACEK("&MFBSP_DIR(%i)=0x%X\n", i, &MFBSP_DIR(i)) ;
	DTRACEK("&MFBSP_GPIO(%i)=0x%X\n", i, &MFBSP_GPIO(i)) ;
#endif
		
		rtems_status_code sc = rtems_io_register_name(gpio_dev_path[i], major, i);
		if (sc != RTEMS_SUCCESSFUL) {
            TRACEK("FATAL(%i) Can't register device name: %s\n", sc, gpio_dev_path[i]) ;
            rtems_fatal_error_occurred(sc) ;
        } else {
			DTRACEK("%s was registred successfully.\n", gpio_dev_path[i]) ;
		}
	}
	
	DTRACEK("RETURN %i\n", RTEMS_SUCCESSFUL) ;
	return RTEMS_SUCCESSFUL ;
}

/*
 * RTEMS driver open() function.
 */
rtems_device_driver gpio_open(
	rtems_device_major_number major,
	rtems_device_minor_number minor,
	void *arg
)
{	
	rtems_libio_open_close_args_t *p_rw_arg = arg ;
	DTRACEK("BEGIN, major=%i, minor=%i, flags=0x%x\n", major, minor, p_rw_arg->flags) ;
	
	//
	// Set up GPIO mode on specified MFBSP port.
	//
	MFBSP_CSR(minor).b.spi_i2s_en = 0 ;
	MFBSP_CSR(minor).b.len = 0 ;
	
	DTRACEK("RETURN %i\n", RTEMS_SUCCESSFUL) ;
	return RTEMS_SUCCESSFUL ;
}

/*
 * RTEMS driver close() function.
 */
rtems_device_driver gpio_close(
	rtems_device_major_number major,
	rtems_device_minor_number minor,
	void *arg
)
{
	DTRACEK("BEGIN, major=%i, minor=%i, arg=0x%x\n", major, minor, arg) ;
	
	//
	// Nothing to do heare so far.
	//
	
	DTRACEK("RETURN %i\n", RTEMS_SUCCESSFUL) ;
	return RTEMS_SUCCESSFUL ;
}

/*
 * RTEMS driver read() function.
 */
rtems_device_driver gpio_read(
	rtems_device_major_number major,
	rtems_device_minor_number minor,
	void *arg
)
{
	//
	// Get R/W parameters from *arg:
	//
	rtems_libio_rw_args_t *rw_args = (rtems_libio_rw_args_t*)arg ;
    size_t len = rw_args->count ;
    uint16_t *p_data = rw_args->buffer ;
	
	DTRACEK("BEGIN, minor=%i, len=%i\n", minor, len) ;
	
	//
	// Check if application tries to read correct
	// amount of data.
	//
	if (len != sizeof(*p_data)) {
		
		rw_args->bytes_moved = -1 ;
		DTRACEK("RETURN %i\n", RTEMS_INVALID_SIZE) ;
		return RTEMS_INVALID_SIZE ;
	}
	
	//
	//	Read GPIO byte and set up return status.
	//
	*p_data = MFBSP_GPIO(minor).data ;
	DTRACEK("read port data=0x%02x\n", *p_data) ;
	rw_args->bytes_moved = sizeof(*p_data) ;
	
	DTRACEK("RETURN %i\n", RTEMS_SUCCESSFUL) ;
    return RTEMS_SUCCESSFUL ;
}

/*
 * RTEMS driver write() function.
 */
rtems_device_driver gpio_write(
	rtems_device_major_number major,
	rtems_device_minor_number minor,
	void *arg
)
{
	// Get R/W parameters from *arg:
	rtems_libio_rw_args_t *rw_args = (rtems_libio_rw_args_t*)arg ;
    size_t len = rw_args->count ;
    uint16_t *p_data = rw_args->buffer ;
	
	DTRACEK("BEGIN, minor=%i, data=0x%02X, len=%i\n", minor, *p_data, len) ;
	
	//
	// Check if application tries to read correct
	// amount of data.
	//
	if (len != sizeof(*p_data)) {
		
		rw_args->bytes_moved = -1 ;
		DTRACEK("RETURN %i\n", RTEMS_INVALID_SIZE) ;
		return RTEMS_INVALID_SIZE ;
	}
	
	//
	//	Write GPIO byte and set up return status.
	//
	MFBSP_GPIO(minor).data = *p_data ;
	rw_args->bytes_moved = sizeof(*p_data) ;
	
	DTRACEK("RETURN %i\n", RTEMS_SUCCESSFUL) ;
    return RTEMS_SUCCESSFUL ;
}

/*
 * RTEMS input/output control routine.
 * Set up GPIO direction.
 */
rtems_device_driver gpio_ctl(
	rtems_device_major_number major,
	rtems_device_minor_number minor,
	void *arg
)
{
	rtems_libio_ioctl_args_t *ioarg = (rtems_libio_ioctl_args_t *) arg;
    uint16_t *p_data = ioarg->buffer;
    unsigned int cmd = ioarg->command ;
	
	DTRACEK("BEGIN: minor=%i, data=0x%02X\n\r", minor, *p_data) ;
	
	switch (cmd) {
		//
		// Set up GPIOx direction.
		// 
		case GPIO_SET_DIRECTION:
			MFBSP_DIR(minor).data = *p_data ;
			break ;
		//
		//	read GPIOx direction
		//
		case GPIO_GET_DIRECTION:
			ioarg->ioctl_return = MFBSP_DIR(minor).data ;
			break ;
		//
		//	Other arguments aren't implemented.
		//
		default:
			DTRACEK("RETURN %i\n", RTEMS_NOT_IMPLEMENTED) ;
			return RTEMS_NOT_IMPLEMENTED ;
	}
	
	DTRACEK("RETURN %i\n", RTEMS_SUCCESSFUL) ;
    return RTEMS_SUCCESSFUL ;
}
