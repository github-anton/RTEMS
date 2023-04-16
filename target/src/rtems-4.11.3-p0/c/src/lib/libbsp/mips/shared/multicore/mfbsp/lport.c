#include <rtems.h>
#include <rtems/io.h>
#include <rtems/rtems/intr.h>
#include <bsp.h>
#include <bsp/irq.h>
#include <rtems/libio.h>
#include <errno.h>
#include <bsp/mfbsp.h>
#include <bsp/lport.h>
#include <stdio.h>
#include <bsp/dma.h>

#if 0
#   define DEBUG
#endif

#include <bsp/auxio.h>

#define LPORT_TXRX_BUFFER_LEN		2048 // 4096(full size)
//#define LPORT_TXRX_BUFFER_LEN		12288 // 24576(full size)
#define LPORT_DMA_RX_DATA_LEN       128
//#define LPORT_DMA_RX_DATA_LEN       4096
#define LPORT_DMA_RX_DATA_BS        32

char lport_dev_path[][BSP_DEV_PATH_MAX_LEN] = {
	"/dev/lport0",
	"/dev/lport1",
	"/dev/lport2",
	"/dev/lport3",
	"/dev/lport4",
	"/dev/lport5"
} ;

typedef struct {
    int  port ;
	char name[BSP_DEV_PATH_MAX_LEN] ;
	dma_dbl_io_buf_t	dma_buf ;
	u_char	dma_buf_data[2][LPORT_TXRX_BUFFER_LEN] __attribute__ ((aligned(8))) ;
	int nonblock ;		// Nonblock mode, default not
	rtems_id sem_write ;
	rtems_id sem_read ;
} lport_t ;

lport_t lport_channel[CONFIG_MULTICORE_MFBSP_QTY] ;

// DMA interrupt handlers
static void lport_dma_rx_data_ih (void *arg) ;
static void lport_dma_tx_data_ih (void *arg) ;

// Initialization routine, called before open(), righta after
// system boot
rtems_device_driver lport_init(
	rtems_device_major_number major,
	rtems_device_minor_number minor,
	void *arg
) {
	int i, j ;

	DTRACEK("BEGIN, major=%i, minor=%i, arg=0x%x\n", major, minor, arg) ;
	
	for (i = 0; i < CONFIG_MULTICORE_MFBSP_QTY; i++) {
		
		// Clear memory
		memset( &lport_channel[i], 0, sizeof(lport_t)) ;
		
		lport_channel[i].port = i ;
		memcpy (lport_channel[i].name, lport_dev_path[i], BSP_DEV_PATH_MAX_LEN) ;
		
		// Register the device driver.
		rtems_status_code sc = rtems_io_register_name(lport_dev_path[i], major, i);
		if (sc != RTEMS_SUCCESSFUL) {
            TRACEK("FATAL(%i) Can't register device name: %s\n", sc, lport_dev_path[i]) ;
            rtems_fatal_error_occurred(sc) ;
        } else {
			DTRACEK("%s was registred successfully.\n", lport_dev_path[i]) ;
		}
		
		// Initialize internal data
		for ( j = 0; j < 2; j++ ) {
			lport_channel[i].dma_buf.half[j].data = lport_channel[i].dma_buf_data[j] ;
			lport_channel[i].dma_buf.half[j].size = LPORT_TXRX_BUFFER_LEN ;
			lport_channel[i].dma_buf.port_off = DMA_MFBSP_PORT_ADDR(i) ;
			DTRACEK("dma_buf=0x%x, half[%i]=0x%x, half[%i].data=0x%x, half[%i].size=%i\n",
					&lport_channel[i].dma_buf, j, &lport_channel[i].dma_buf.half[j], j, lport_channel[i].dma_buf.half[j].data, j, lport_channel[i].dma_buf.half[j].size) ;
		}
	}
	
	DTRACEK("DONE\n") ;
	
	return RTEMS_SUCCESSFUL ;
}

// Open routine - open().
rtems_device_driver lport_open(
	rtems_device_major_number major,
	rtems_device_minor_number minor,
	void *arg
)
{	
	rtems_libio_open_close_args_t *p_rw_arg = arg ;
	int rc ;
	
	lport_t *p_ch = &lport_channel[minor] ;
	
	DTRACEK("BEGIN, major=%i, minor=%i, flags=0x%x\n", major, minor, p_rw_arg->flags) ;
    
	// Enable device.
    MFBSP_CSR(minor).b.lclk_rate_hi = (MFBSP_LCLK_RATE >> 1) & 0x7 ;
	MFBSP_CSR(minor).b.lclk_rate_lo = MFBSP_LCLK_RATE & 0x1;
	MFBSP_CSR(minor).b.ldw = 1 ;    // 1 byte word length
	MFBSP_CSR(minor).b.len = 1 ;
	
    DTRACEK("MFBSP_LCLK_RATE=%i\n", MFBSP_LCLK_RATE) ;
    DTRACEK("CPU_CLOCK_RATE=%i\n", CPU_CLOCK_RATE) ;
    
	// Check in which mode we open file device.
	// Since we can only transmit or only receive
	// we have to check flags was set properly and
	// initialize device respectivly.
    switch (p_rw_arg->flags & (LIBIO_FLAGS_READ | LIBIO_FLAGS_WRITE)) {
		
		case LIBIO_FLAGS_READ:
			DTRACEK("Opening in READ mode.\n") ;
            DTRACEK("Install interupt handler lport_dma_rx_data_ih(), IRQ=%i\n", DMA_MFBSP_DATA_IRQ(minor)) ;
			rtems_interrupt_handler_install(DMA_MFBSP_DATA_IRQ(minor), p_ch->name, RTEMS_INTERRUPT_SHARED, lport_dma_rx_data_ih, p_ch) ;
			rc = rtems_semaphore_create(
				rtems_build_name('l', 'r', 'd', '0' + p_ch->port), 
				0, 
				RTEMS_FIFO | RTEMS_SIMPLE_BINARY_SEMAPHORE | RTEMS_NO_INHERIT_PRIORITY | \
				RTEMS_NO_PRIORITY_CEILING, 
				0, 
				&(p_ch->sem_read));
			break ;
			
		case LIBIO_FLAGS_WRITE:
			DTRACEK("Opening in WRITE mode.\n") ;
			MFBSP_CSR(minor).b.ltran = 1;				// configure as transmitter
			DTRACEK("Install interrupt handler lport_dma_tx_data_ih(), IRQ=%i\n", DMA_MFBSP_DATA_IRQ(minor)) ;
			rtems_interrupt_handler_install(DMA_MFBSP_DATA_IRQ(minor), p_ch->name, RTEMS_INTERRUPT_SHARED, lport_dma_tx_data_ih, p_ch) ;
			rc = rtems_semaphore_create(
				rtems_build_name('l', 'w', 'r', '0' + p_ch->port), 
				0, 
				RTEMS_FIFO | RTEMS_SIMPLE_BINARY_SEMAPHORE | RTEMS_NO_INHERIT_PRIORITY | \
				RTEMS_NO_PRIORITY_CEILING, 
				0, 
				&(p_ch->sem_write));
			if ( rc != RTEMS_SUCCESSFUL ) {
				DTRACEK("rtems_semaphore_create() error: %i\n", rc) ;
				return rc ;
			}
			break ;
			
		default:
			return RTEMS_NOT_IMPLEMENTED ;
	}
	
	bsp_enable_irq(DMA_MFBSP_DATA_IRQ(minor)) ;
    DTRACEK("bsp_enable_irq(%i) -> MC_MASKR2=0x%x\n", DMA_MFBSP_DATA_IRQ(minor), MC_MASKR2) ;
	
	DTRACEK("Using registers:\n") ;
	DTRACEK("MFBSP_CSR(%i): *0x%x=0x%x\n", minor, &MFBSP_CSR(minor), MFBSP_CSR(minor)) ;
	
    DTRACEK("DONE\n") ;
    
	return RTEMS_SUCCESSFUL ;
}

// Close routine - close().
rtems_device_driver lport_close(
	rtems_device_major_number major,
	rtems_device_minor_number minor,
	void *arg
)
{	
	return RTEMS_SUCCESSFUL ;
}

// Read routine - read().
rtems_device_driver lport_read(
	rtems_device_major_number major,
	rtems_device_minor_number minor,
	void *arg
)
{
	// Get R/W parameters from *arg:
	rtems_libio_rw_args_t *rw_args = (rtems_libio_rw_args_t*)arg ;
    size_t len = rw_args->count ;
    char *data = rw_args->buffer ;
	int nbytes ;
	
	// Get current lport channel context
	lport_t *p_ch = &lport_channel[minor] ;
	
	DTRACEK("BEGIN, minor=%i, data=0x%x, len=%i\n", minor, data, len) ;
	
	dma_start_rx_ch_if_needed(&p_ch->dma_buf, LPORT_DMA_RX_DATA_LEN, LPORT_DMA_RX_DATA_BS) ;

	nbytes = dma_dbl_io_buf_read(&p_ch->dma_buf, data, len) ;
	
	if (nbytes <= 0) {
		// If it is block mode, then wait for some data
		if (!p_ch->nonblock) {
            
            DTRACEK("OBTAIN sem_read: *0x%x=0x%x\n", &p_ch->sem_read, p_ch->sem_read) ;
			rtems_semaphore_obtain(p_ch->sem_read, RTEMS_WAIT, RTEMS_NO_TIMEOUT) ;
			
            while (rw_args->bytes_moved < len) {
                 lport_read(major, minor, arg) ;
            }
		// If it is nonblock mode and we have no data, then return -1 as length we
		// have read. Not 0, because an empty packet has zero length.
		} else {
			rw_args->bytes_moved = -1 ;
			return RTEMS_IO_ERROR ;
		}
	} else {
		rw_args->bytes_moved += nbytes ;
        rw_args->count -= nbytes ;
#if 0
        // To debug reading process
        printf("r:%i\n", nbytes) ;
#endif
	}
	
	DTRACEK("DONE\n") ;
	
    return RTEMS_SUCCESSFUL ;
}

// Write routine - write().
// Function can write synchronously or asynchronously
rtems_device_driver lport_write(
	rtems_device_major_number major,
	rtems_device_minor_number minor,
	void *arg
)
{
	// Get R/W parameters from *arg:
	rtems_libio_rw_args_t *rw_args = (rtems_libio_rw_args_t*)arg ;
    size_t len = rw_args->count ;
    char *data = rw_args->buffer ;
	int nbytes ;
	
	// Get current lport channel context
	lport_t *p_ch = &lport_channel[minor] ;
	
    //bsp_disable_irq(DMA_MFBSP_DATA_IRQ(minor)) ;
    
	DTRACEK("BEGIN, minor=%i, data=0x%x, len=%i\n", minor, data, len) ;
	
	// Copy data into DMA memory
	nbytes = dma_dbl_io_buf_write(&p_ch->dma_buf, data, len) ;
	
	// Check if writing is ok
	if ( nbytes < 0 ) {	// If an error uccurred
		rw_args->bytes_moved = 0 ;
		return RTEMS_INVALID_SIZE ;
	}
	
	// Startup DMA to transmit data if stopped.	
	dma_start_tx_ch_if_needed(&p_ch->dma_buf) ;
	
	// If blocked mode, wait competion data sending over lport.
	if (!p_ch->nonblock) {
		// Lock the mutex.
        DTRACEK("OBTAIN sem_write: *0x%x=0x%x\n", &p_ch->sem_write, p_ch->sem_write) ;
        //bsp_enable_irq(DMA_MFBSP_DATA_IRQ(minor)) ;
		rtems_semaphore_obtain(p_ch->sem_write, RTEMS_WAIT, RTEMS_NO_TIMEOUT) ;
	}
	
	rw_args->bytes_moved = len ;
	
	DTRACEK("DONE, len=%i\n", len) ;
	
    return RTEMS_SUCCESSFUL ;
}

// Input/Output control routine - ioctl().
rtems_device_driver lport_ctl(
	rtems_device_major_number major,
	rtems_device_minor_number minor,
	void *arg
)
{
	DTRACEK("BEGIN, major=0x%x, minor=0x%x\n\r", major, minor) ;
	
    rtems_libio_ioctl_args_t *ioarg = (rtems_libio_ioctl_args_t *) arg;
    unsigned int *data = ioarg->buffer;
    unsigned int cmd = ioarg->command ;
	uint32_t tmp ;
    
    // Get current lport channel context
	lport_t *p_ch = &lport_channel[minor] ;
    
    switch (cmd) {
    // Nonblock reading mode
    case FIONBIO:
        p_ch->nonblock = *data ;
        // If synchronous mode, then disable all tx IRQ as it requires
        // for swic_write_async() function.
        
        DTRACEK("FIONBIO -> %i\n", p_ch->nonblock) ;
        ioarg->ioctl_return = 0 ;
        break ;
		
	case LPORT_FLUSH:
		//
		//	Reset LPort and clear all buffered data.
		//
		MFBSP_CSR(minor).b.len = 0 ;
		
		while (MFBSP_CSR(minor).b.lstat) {
			
			tmp = MFBSP_RX(minor) ;
		}
		MFBSP_CSR(minor).b.len = 1 ;
		
		DTRACEK("FLUSH\n") ;
		break ;
    }
    
    DTRACEK("DONE\n") ;
    
    return RTEMS_SUCCESSFUL ;
}

static void lport_dma_rx_data_ih (void *arg) {
	lport_t *p_ch = (lport_t*)arg;
    dma_io_buf_t *p_w_buf ;
    
	DTRACEK("RISE, port=%i\n", p_ch->port) ;
	
	// Clear IRQ
	dma_port_ch_csr_t csr = DMA_MFBSP_CH_CSR(p_ch->port) ;
    DTRACEK("CLEAR IRQ, DMA_MFBSP_CH_CSR(%i): *0x%x=0x%x\n", p_ch->port, &DMA_MFBSP_CH_CSR(p_ch->port), csr) ;
	
    p_w_buf = dma_dbl_io_buf_get_w_buf(&p_ch->dma_buf) ;
    // Move write index (DMA) in current writing buffer, so
    // reader can get actual data
    p_w_buf->w_idx += LPORT_DMA_RX_DATA_LEN ;
    DTRACEK("p_w_buf=0x%x, r_idx=%i, w_idx=%i\n", p_w_buf, p_w_buf->r_idx, p_w_buf->w_idx) ;
    
	if(!p_ch->nonblock) {
        DTRACEK("RELEASE sem_read: *0x%x=0x%x\n", &p_ch->sem_read, p_ch->sem_read) ;
		rtems_semaphore_release( p_ch->sem_read );
	}
	
	// Restart DMA to continue receiving data.
    // Comment next line before debugging!!!
	dma_start_rx_ch_if_needed(&p_ch->dma_buf, LPORT_DMA_RX_DATA_LEN, LPORT_DMA_RX_DATA_BS) ;
    
    DTRACEK("DONE\n") ;
}

static void lport_dma_tx_data_ih (void *arg) {
	lport_t *p_ch = (lport_t*)arg;

	DTRACEK("RISE, port=%i\n", p_ch->port) ;
	
	// Clear IRQ
	dma_port_ch_csr_t csr = DMA_MFBSP_CH_CSR(p_ch->port) ;
    DTRACEK("CLEAR IRQ, DMA_MFBSP_CH_CSR(%i): *0x%x=0x%x\n", p_ch->port, &DMA_MFBSP_CH_CSR(p_ch->port), csr) ;
	
    // Move read index (DMA) in current reading buffer, so
    // writer can get actual data
    dma_dbl_io_buf_get_r_buf(&p_ch->dma_buf)->r_idx += dma_io_buf_get_fill(dma_dbl_io_buf_get_r_buf(&p_ch->dma_buf)) ;
    
	if(!p_ch->nonblock) {
        DTRACEK("RELEASE sem_write: *0x%x=0x%x\n", &p_ch->sem_write, p_ch->sem_write) ;
		rtems_semaphore_release( p_ch->sem_write );
	} else {

        // Restart DMA to transmit rest of data if we are working in
        // NONBLOCK mode.
        // Comment next line before debugging!!!
        dma_start_tx_ch_if_needed(&p_ch->dma_buf) ;
    }
    
    DTRACEK("DONE\n") ;
}
