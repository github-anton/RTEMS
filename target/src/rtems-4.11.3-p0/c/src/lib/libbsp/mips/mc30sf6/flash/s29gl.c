#if 0
	#define DEBUG
#endif

#if 1
	#define TRACE_ERROR
#endif

#include <bsp/auxio.h> 
#include <bsp/auxtimer.h>
#include <rtems.h>
#include <bsp.h>
#include <errno.h>
#include <stdio.h>
#include <bsp/s29gl.h>
#include <rtems/libio.h>
#include <rtems/blkdev.h>

//
// TODO: Move to flash_config.h?
//

#define FLASHDISK_CONFIG_COUNT 1

#define FLASHDISK_DEVICE_COUNT 1

//#define FLASHDISK_SEGMENT_COUNT 256U
#define FLASHDISK_SEGMENT_COUNT 32U

#define FLASHDISK_SEGMENT_SIZE (1024 * 128)

#define FLASHDISK_BLOCK_SIZE 512U

#define FLASHDISK_BLOCKS_PER_SEGMENT \
  (FLASHDISK_SEGMENT_SIZE / FLASHDISK_BLOCK_SIZE)

#define FLASHDISK_SIZE \
  (FLASHDISK_SEGMENT_COUNT * FLASHDISK_SEGMENT_SIZE)

//
// TODO: Move next variable to flash_config.c?
//
static const rtems_fdisk_segment_desc flashdisk_segment_desc = {
  .count = FLASHDISK_SEGMENT_COUNT,
  .segment = 0,
  .offset = 0,
  .size = FLASHDISK_SEGMENT_SIZE
};

uint32_t rtems_flashdisk_configuration_size = FLASHDISK_CONFIG_COUNT;
  
static uint8_t flashdisk_data [FLASHDISK_SIZE];

/**
 * The device configuration.
 */
typedef struct
{
	int   bus_8bit;
	void* base;
} rtems_s29gl_config;

const rtems_s29gl_config rtems_s29gl_configuration[FLASHDISK_CONFIG_COUNT] = {
	{
		.bus_8bit = 1,
		.base = (void*)0xbc000000	// 0x1c000000 physical address
	}
} ;

//extern const rtems_s29gl_config rtems_s29gl_configuration[] ;


static uint8_t *get_data_pointer(
  const rtems_fdisk_segment_desc *sd,
  uint32_t segment,
  uint32_t offset
)
{
  offset += sd->offset + (segment - sd->segment) * sd->size;

  return &flashdisk_data [offset];
}

static int
s29gl_toggle_wait_8 (volatile uint8_t* status)
{
	DTRACEK("BEGIN: status=0x%x\n", status) ; 
	
	while (1)
	{
		volatile uint8_t status1 = *status;
		volatile uint8_t status2 = *status;

		/*if (status2 & (1 << 7) != 0) {
			if (((status1 ^ status2) & (1 << 2)) != 0) {
				DTRACEK("RETURN 0, bit 7=1,bit 2 toggle\n") ;
				return 0;
			}
		}*/
	
		if (((status1 ^ status2) & (1 << 6)) == 0) {
			DTRACEK("RETURN 0, bit6\n") ;
			return 0;
		}

		if ((status1 & (1 << 5)) != 0)
		{
			status1 = *status;
			status2 = *status;

			if (((status1 ^ status2) & (1 << 6)) == 0) {
				DTRACEK("RETURN 0, bit5\n") ;  
				return 0;
			}

#ifdef TRACE_ERROR
			TRACEK("S29GL: error bit detected: %p = 0x%04x\n", status, status1);
#endif

			*status = 0xf0;
			return EIO;
		}
	}
	
	*status = 0xf0 ;
    return EIO;
}


static int
s29gl_write_data_8 (volatile uint8_t* base,
                              uint32_t          offset,
                              const uint8_t*    data,
                              uint32_t          size)
{
	volatile uint8_t*     seg = base + offset;
	rtems_interrupt_level level;

	DTRACEK("BEGIN: base=0x%x, off=0x%x, len=%i\n", base, offset, size) ;
	/*
	* Issue a reset.
	*/
	*base = 0xf0;

	while (size)
	{
		rtems_interrupt_disable (level);
		*(base + 0xaaa) = 0xaa;
		*(base + 0x555) = 0x55;
		*(base + 0xaaa) = 0xa0;
		*seg = *data++;
		rtems_interrupt_enable (level);
		if ( s29gl_toggle_wait_8 (seg++) != 0 ) {
			DTRACEK("RETURN %i\n", EIO) ;
			return EIO;
		}
		size--;
  }

  /*
   * Issue a reset.
   */
	*base = 0xf0;

	DTRACEK("RETURN 0\n") ;
	return 0;
}

static void erase_device(void)
{
	DTRACEK("BEGIN\n") ;
	memset(&flashdisk_data [0], 0xff, FLASHDISK_SIZE);
	DTRACEK("RETURN\n") ;
}


rtems_device_driver s29gl_flash_initialize(
  rtems_device_major_number major,
  rtems_device_minor_number minor,
  void *arg
)
{
	rtems_device_driver	ret ;
	const rtems_s29gl_config*	ac = &rtems_s29gl_configuration[minor];
	
	DTRACEK("BEGIN, major=%i, minor=%i\n", major, minor) ;
	
	if (ac->bus_8bit) {
		volatile uint8_t*	seg_8 = ac->base;
		uint8_t manuf_id ;
	
		DTRACEK("ac->base=0x%x\n", ac->base) ;
	
		*((uint8_t*)seg_8 + 0xAAA) = 0xAA ;
		*((uint8_t*)seg_8 + 0x555) = 0x55 ;
		*((uint8_t*)seg_8 + 0xAAA) = 0x90 ;
		manuf_id = *((uint8_t*)seg_8 + 0x000) ;
	
		DTRACEK("manuf_id=0x%x\n", manuf_id) ;
		
		// Fail to read manufacturer ID.
		if (manuf_id != 0x1) {
			
			return RTEMS_UNSATISFIED ;
		}
	
		*((uint8_t*)seg_8 + 0x000) = 0xF0 ;
	} else {
		
		return RTEMS_NOT_IMPLEMENTED ;
	}
	
	ret = rtems_fdisk_initialize(major, minor, arg);
	
	DTRACEK("RETURN %i\n", ret) ;
	return ret ;
}

static int s29gl_flash_blank(
  const rtems_fdisk_segment_desc *sd,
  uint32_t device,
  uint32_t segment,
  uint32_t offset,
  uint32_t size
)
{
	int eno = 0;
	/*const uint8_t *current = get_data_pointer(sd, segment, offset);
	const uint8_t *end = current + size;*/
	DTRACEK("BEGIN: seg=%i, off=0x%x, len=%i\n", segment, offset, size) ;

  /*while (eno == 0 && current != end) {
    if (*current != 0xff) {
      eno = EIO;
    }
    ++current;
  }*/
  
	const rtems_s29gl_config* ac = &rtems_s29gl_configuration[device];
	
	if(ac->bus_8bit) {
		volatile uint8_t*	seg_8 = ac->base;
	
		offset += sd->offset + (segment - sd->segment) * sd->size;

		seg_8 += offset;
	
		DTRACEK("Start check blank seg_8=0x%x, size=%i\n", seg_8, size) ;
		while (size--) {
			if (*seg_8++ != 0xff)
			{
#if defined(TRACE_ERROR)
				TRACEK ("blank check error: *%p=0x%02x\n", seg_8 - 1, *(seg_8 - 1));
#endif
				DTRACEK("RETURN %i\n", EIO) ;
				return EIO;
			}
		}
	} else {
		
		return RTEMS_NOT_IMPLEMENTED ;
	}
	
	DTRACEK("RETURN %i\n", eno) ;
	
	return eno;
}

static int s29gl_flash_verify(
  const rtems_fdisk_segment_desc *sd,
  uint32_t device,
  uint32_t segment,
  uint32_t offset,
  const void *buffer,
  uint32_t size
)
{
	int eno = 0;
	/*uint8_t *data = get_data_pointer(sd, segment, offset);*/
	DTRACEK("BEGIN: seg=%i, off=0x%x, len=%i\n", segment, offset, size) ;


	/*if (memcmp(data, buffer, size) != 0) {
		eno = EIO;
	}*/
  
	const rtems_s29gl_config* ac = &rtems_s29gl_configuration[device];
	const uint8_t*	addr = ac->base;

	addr += (sd->offset + (segment - sd->segment) * sd->size) + offset;

	if (memcmp (addr, buffer, size) != 0) {
		DTRACEK("RETURN %i\n", EIO) ;
		return EIO;
	}

	DTRACEK("RETURN %i\n", eno) ;
	return eno;
}

static int s29gl_flash_read(
  const rtems_fdisk_segment_desc *sd,
  uint32_t device,
  uint32_t segment,
  uint32_t offset,
  void *buffer,
  uint32_t size
)
{
	int eno = 0;
	/*const uint8_t *data = get_data_pointer(sd, segment, offset);*/
	DTRACEK("BEGIN, seg=%i, off=0x%x, len=%i\n", segment, offset, size) ;

	/*memcpy(buffer, data, size);*/
  
	unsigned char* addr =
		rtems_s29gl_configuration[device].base +
		sd->offset + ((segment - sd->segment) * sd->size) + offset;
	memcpy (buffer, addr, size);

	DTRACEK("RETURN %i\n", eno) ;
	return eno;
}

static int s29gl_flash_write(
  const rtems_fdisk_segment_desc *sd,
  uint32_t device,
  uint32_t segment,
  uint32_t offset,
  const void *buffer,
  uint32_t size
)
{
  int eno = 0;
  /*uint8_t *data = get_data_pointer(sd, segment, offset);*/
  DTRACEK("BEGIN: seg=%i, off=0x%x, len=%i\n", segment, offset, size) ;

  /*memcpy(data, buffer, size);*/
  
  eno = s29gl_flash_verify (sd, device, segment, offset, buffer, size);

  if (eno != 0)
  {
    const rtems_s29gl_config* ac = &rtems_s29gl_configuration[device];
    uint32_t	soffset;

    soffset = offset + sd->offset + ((segment - sd->segment) * sd->size);

    if (offset & 1)
      TRACEK ("offset is odd\n");

    if (size & 1)
      TRACEK ("size is odd\n");

    if (ac->bus_8bit)
		eno = s29gl_write_data_8 (ac->base, soffset, buffer, size);
    else {
		DTRACEK("RETURN %i\n", eno) ;
		return RTEMS_NOT_IMPLEMENTED ;
	}
    /*
     * Verify the write worked.
     */
    if (eno == 0)
    {
      eno = s29gl_flash_verify (sd, device, segment, offset, buffer, size);
#if defined(TRACE_ERROR)
      if (eno)
        TRACEK ("verify failed: %ld-%ld-%08lx: s=%ld\n",
                device, segment, offset, size);
#endif
    }
  }

  DTRACEK("RETURN %i\n", eno) ;
  return eno;
}

static int s29gl_flash_erase(
  const rtems_fdisk_segment_desc *sd,
  uint32_t device,
  uint32_t segment
)
{
	int eno = 0;
	const rtems_s29gl_config *ac = &rtems_s29gl_configuration[device] ;
	uint32_t                      offset;
    rtems_interrupt_level         level;
	/*uint8_t *data = get_data_pointer(sd, segment, 0);*/
	
	DTRACEK("BEGIN, device=%i, segment=%i\n", device, segment) ;
	
	offset = sd->offset + ((segment - sd->segment) * sd->size);

	/*memset(data, 0xff, sd->size);*/
	
	if (ac->bus_8bit)
    {
		volatile uint8_t* base = ac->base;
		volatile uint8_t* seg  = base + offset;
		DTRACEK("seg=%i\n", seg) ;
		
		/*
		* Issue a reset.
		*/
		rtems_interrupt_disable (level);
		*base = 0xf0;
		*(base + 0xaaa) = 0xaa;
		*(base + 0x555) = 0x55;
		*(base + 0xaaa) = 0x80;
		*(base + 0xaaa) = 0xaa;
		*(base + 0x555) = 0x55;
		*seg = 0x30;
		rtems_interrupt_enable (level);

		eno = s29gl_toggle_wait_8 (seg);

		/*
		* Issue a reset.
		*/
		*base = 0xf0;
	} else {
		DTRACEK("RETURN EIO=%i\n", EIO) ;
		return EIO ;
	}
	
	if (eno == 0)
    {
	  SNOOZE(200) ;
      eno = s29gl_flash_blank (sd, device, segment, 0, sd->size);
#ifdef DEBUG
      if (eno)
        printk ("S29GL: erase failed: %ld-%ld\n", device, segment);
#endif
    }
	
	DTRACEK("RETURN %i\n", eno) ;
	return eno;
}

static int s29gl_flash_erase_device(
  const rtems_fdisk_device_desc *sd,
  uint32_t device
)
{
  int eno = 0;
  DTRACEK("BEGIN\n") ;

  erase_device();

  DTRACEK("RETURN %i\n", eno) ;
  return eno;
}

static const rtems_fdisk_driver_handlers flashdisk_ops = {
  .read = s29gl_flash_read,
  .write = s29gl_flash_write,
  .blank = s29gl_flash_blank,
  .verify = s29gl_flash_verify,
  .erase = s29gl_flash_erase,
  .erase_device = s29gl_flash_erase_device
};

static const rtems_fdisk_device_desc flashdisk_device = {
  .segment_count = 1,
  .segments = &flashdisk_segment_desc,
  .flash_ops = &flashdisk_ops
};

const rtems_flashdisk_config
rtems_flashdisk_configuration [FLASHDISK_CONFIG_COUNT] = {
  {
    .block_size = FLASHDISK_BLOCK_SIZE,
    .device_count = FLASHDISK_DEVICE_COUNT,
    .devices = &flashdisk_device,
    .flags = RTEMS_FDISK_CHECK_PAGES
      | RTEMS_FDISK_BLANK_CHECK_BEFORE_WRITE,
    .unavail_blocks = FLASHDISK_BLOCKS_PER_SEGMENT,
    .compact_segs = 2,
    .avail_compact_segs = 1,
    .info_level = 0
  }
} ;
