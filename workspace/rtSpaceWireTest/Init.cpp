/*
 *  COPYRIGHT (c) 1989-2012.
 *  On-Line Applications Research Corporation (OAR).
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.org/license/LICENSE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <rtems/test.h>

#include <bsp.h> /* for device driver prototypes */

#include <stdio.h>
#include <stdlib.h>

int main(void) ;

/* forward declarations to avoid warnings */
extern "C" {
rtems_task Init(rtems_task_argument argument);
};

const char rtems_test_name[] = "Multicore SpaceWire C++";

rtems_task Init(
  rtems_task_argument ignored
)
{
    rtems_test_begin();
    
    main() ;
    
    rtems_test_end();
    exit( 0 );
}


#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_APPLICATION_EXTRA_DRIVERS MCSPW_DRIVER_TABLE_ENTRY

#define CONFIGURE_MAXIMUM_TASKS            1
#define CONFIGURE_USE_DEVFS_AS_BASE_FILESYSTEM

#define CONFIGURE_RTEMS_INIT_TASKS_TABLE

#define CONFIGURE_INITIAL_EXTENSIONS RTEMS_TEST_INITIAL_EXTENSION

#define CONFIGURE_UNIFIED_WORK_AREAS
#define CONFIGURE_UNLIMITED_OBJECTS             // operators new and delete really need it!
#define CONFIGURE_FILESYSTEM_DEVFS
#define CONFIGURE_LIBIO_MAXIMUM_FILE_DESCRIPTORS 8

#define CONFIGURE_INIT
#include <rtems/confdefs.h>
