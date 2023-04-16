#ifndef S29GL_H_
#define S29GL_H_

#include <bsp/config.h>
#include <bsp.h>
#include <rtems/flashdisk.h>
#include <rtems/io.h>

#ifdef __cplusplus
extern "C" {
#endif

rtems_device_driver s29gl_flash_initialize( rtems_device_major_number major, rtems_device_minor_number minor, void *arg ) ;

#define FLASHDISK_DRIVER { \
  .initialization_entry = s29gl_flash_initialize, \
  .open_entry = rtems_blkdev_generic_open, \
  .close_entry = rtems_blkdev_generic_close, \
  .read_entry = rtems_blkdev_generic_read, \
  .write_entry = rtems_blkdev_generic_write, \
  .control_entry = rtems_blkdev_generic_ioctl \
}

#ifdef __cplusplus
}
#endif

#endif
