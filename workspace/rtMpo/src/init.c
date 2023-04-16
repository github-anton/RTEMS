/*
 *  COPYRIGHT (c) 1989-2012.
 *  On-Line Applications Research Corporation (OAR).
 *	Modified by Anton Ermakov, 2018
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.org/license/LICENSE.
 */
#if 1
	#define VERBOSE
#endif

#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

//#include <rtems/test.h>

#include <bsp.h> /* for device driver prototypes */
#include <bsp/swic.h>
#include <bsp/lport.h>
#include <bsp/gpio.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <rtems/ramdisk.h>
#include <rtems/untar.h>
#include "FilesystemImage.h"

#include <rtems/test.h>

#include "Shared/auxio.h"

#define RTEMS_DRIVER_AUTO_MAJOR (0)

rtems_ramdisk_config rtems_ramdisk_configuration [] = {
  { .block_size = 512, .block_num = 1024, .location = NULL }
};

size_t rtems_ramdisk_configuration_size = RTEMS_ARRAY_SIZE(rtems_ramdisk_configuration);

/*
 * Create the RAM-disk driver table entry.
 */
rtems_driver_address_table rtems_ramdisk_io_ops = {
  initialization_entry: ramdisk_initialize,
  open_entry:           rtems_blkdev_generic_open,
  close_entry:          rtems_blkdev_generic_close,
  read_entry:           rtems_blkdev_generic_read,
  write_entry:          rtems_blkdev_generic_write,
  control_entry:        rtems_blkdev_generic_ioctl
};


int main(void) ;

/* forward declarations to avoid warnings */
rtems_task Init(rtems_task_argument argument);
int setup_ramdisk (const char* mntpath) ;

rtems_task Init(
  rtems_task_argument ignored
)
{
	int ret;

	VPRINTF("MPO started...\n\r") ;

	//gdb_stub_init() ;

	VPRINTF("Unpacking tar filesystem:\t\t...");
	if(Untar_FromMemory((char*) FilesystemImage, FilesystemImage_size) != 0) {
		VPRINTF("Can't unpack tar filesystem: %s\n", strerror(errno));
	    exit(-1);
	}
	VPRINTF("SUCCESSFUL.\n") ;

	VPRINTF("Setup RAM Disk:\t\t\t\t...") ;
	ret = setup_ramdisk ("/mnt/ramdisk");
	if (ret) {
		fprintf(stderr, "Can't setup ramdisk: %s\n", strerror(errno)) ;
		exit (ret);
	}
	VPRINTF("SUCCESSFUL.\n") ;

	main() ;

	VPRINTF("MPO exited\n") ;

    exit( 0 );
}

int setup_ramdisk (const char* mntpath)
{
  rtems_device_major_number major;
  rtems_status_code         sc;

  /*
   * Register the RAM Disk driver.
   */
  //printf ("Register RAM Disk Driver:\t");
  sc = rtems_io_register_driver (RTEMS_DRIVER_AUTO_MAJOR,
                                 &rtems_ramdisk_io_ops,
                                 &major);
  if (sc != RTEMS_SUCCESSFUL) {
    fprintf (stderr, "Error: ramdisk driver is not initialised: %s\n",
            rtems_status_text (sc));
    return -1;
  }

  //printf ("successful\n");
  return 0;
}

/*
 * Drivers
 */
#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_APPLICATION_EXTRA_DRIVERS SWIC_DRIVER_TABLE_ENTRY, \
				LPORT_DRIVER_TABLE_ENTRY, RAMDISK_DRIVER_TABLE_ENTRY, \
				GPIO_DRIVER_TABLE_ENTRY
#define CONFIGURE_APPLICATION_NEEDS_LIBBLOCK

/*
 * Filesystems
 */
#define CONFIGURE_FILESYSTEM_DEVFS
#define CONFIGURE_FILESYSTEM_DOSFS
#define CONFIGURE_MAXIMUM_FILE_DESCRIPTORS          40
#define CONFIGURE_IMFS_MEMFILE_BYTES_PER_BLOCK      512
#define CONFIGURE_LIBIO_MAXIMUM_FILE_DESCRIPTORS    16
//#define CONFIGURE_USE_DEVFS_AS_BASE_FILESYSTEM

/*
 * Memory Objects
 */
#define CONFIGURE_MAXIMUM_DRIVERS                   12
#define CONFIGURE_MAXIMUM_TASKS           			32
#define CONFIGURE_RTEMS_INIT_TASKS_TABLE
#define CONFIGURE_UNIFIED_WORK_AREAS
#define CONFIGURE_UNLIMITED_OBJECTS             // operators new and delete do need it to work!

/*
 * Task dispatch
 */
#define CONFIGURE_MICROSECONDS_PER_TICK 250		// If CPU cahce is disabled CONFIGURE_MICROSECONDS_PER_TICK
												// should be not less than 1000
/*
 * Extensions.
 */
#define CONFIGURE_INITIAL_EXTENSIONS   { .fatal = bsp_fatal_extension }, RTEMS_TEST_INITIAL_EXTENSION

#define CONFIGURE_INIT
#include <rtems/confdefs.h>
