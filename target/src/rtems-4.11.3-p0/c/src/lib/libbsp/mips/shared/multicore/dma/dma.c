#include <bsp/dma.h>
#include <bsp/auxmath.h>
#include <string.h>
#include <rtems/score/mips.h>

#if 0
	#define DEBUG
#endif

#include <bsp/auxio.h>

int dma_dbl_io_buf_read( dma_dbl_io_buf_t *p_dbl_buf, void *p_data, size_t size ) {
	
	// Get pointer to read IO buffer
	dma_io_buf_t *p_r_buf = dma_dbl_io_buf_get_r_buf(p_dbl_buf) ;
    
    DTRACEK("BEGIN, dbl_buf=0x%x, data=0x%x, len=%i\n", p_dbl_buf, p_data, size) ;
	DTRACEK("state 0: p_r_buf=0x%x, size=%i, r_idx=%i\n", p_r_buf, p_r_buf->size, p_r_buf->r_idx) ;
	
    DTRACEK("DMA_PORT_RUN(0x%X).b.run=%i\n", p_dbl_buf->port_off, DMA_PORT_RUN(p_dbl_buf->port_off).b.run) ;
	// If our read buffer is empty and DMA stopped.
	if ( dma_io_buf_get_fill(p_r_buf) == 0 && !DMA_PORT_RUN(p_dbl_buf->port_off).b.run ) {
        // Reset r/w position io the I/O buffer
		dma_io_buf_reset(p_r_buf) ;
		// Switch buffers. Maybe we will get some data to read from recently
		// writing IO buffer of DMA.
		dma_dbl_io_buf_swap_buf(p_dbl_buf) ;
		// Get a pointer to new read IO buffer
		p_r_buf = dma_dbl_io_buf_get_r_buf(p_dbl_buf) ;
	}
	
	DTRACEK("state 1: p_r_buf=0x%x, size=%i, r_idx=%i\n", p_r_buf, p_r_buf->size, p_r_buf->r_idx) ;
	
	// Get filling of read buffer again.
	size_t size_to_read = MIN(dma_io_buf_get_fill(p_r_buf), size) ;
	
	// Copy new data from IO buffer to 
	memcpy(p_data, mips_get_uncached_addr(&p_r_buf->data[p_r_buf->r_idx]), size_to_read) ;
    // Increment r_idx
    p_r_buf->r_idx += size_to_read ;
	
    DTRACEK("DONE, size_to_read=%i\n", size_to_read) ;
    
	return size_to_read ;
}

/*
 * Write data to DMA IO double buffer and automaticaly select
 * a IO buffer for writing and maybe switch the IO buffers.
 * Return: amount of writen data.
 */
int dma_dbl_io_buf_write( dma_dbl_io_buf_t *p_dbl_buf, void *p_data, size_t size ) {
	
	dma_io_buf_t *p_r_buf = dma_dbl_io_buf_get_r_buf (p_dbl_buf) ;
	dma_io_buf_t *p_w_buf = dma_dbl_io_buf_get_w_buf (p_dbl_buf) ;
	
	DTRACEK("dma_buf=0x%x, p_data=0x%x, len=%i\n", p_dbl_buf, p_data, size) ;
	DTRACEK("state 0: p_w_buf=0x%x, size=%i, w_idx=%i\n", p_w_buf, p_w_buf->size, p_w_buf->w_idx) ;
	
	// If reader buffer (DMA) is empty and DMA is stopped switch buffers and
	// write some data for DMA reader.
	if(dma_io_buf_get_fill(p_r_buf) == 0 && !DMA_PORT_RUN(p_dbl_buf->port_off).b.run ) {
		
		dma_dbl_io_buf_swap_buf(p_dbl_buf) ;
		p_r_buf = dma_dbl_io_buf_get_r_buf (p_dbl_buf) ;
		p_w_buf = dma_dbl_io_buf_get_w_buf (p_dbl_buf) ;
	}
	
	// If write IO buffer is empty
	if(dma_io_buf_get_fill(p_w_buf) == 0) {
		// Reset empty write IO buffer
		dma_io_buf_reset(p_w_buf) ;
	}
	
	DTRACEK("state 1: p_w_buf=0x%x, size=%i, w_idx=%i\n", p_w_buf, p_w_buf->size, p_w_buf->w_idx) ;
	
	// We don't have enough space in IO buffer, just exit and report an error
	if (dma_io_buf_get_avail(p_w_buf) < size ) {
		DTRACEK("Data is too big, avail-buffer-size=%i\n", dma_io_buf_get_avail(p_w_buf)) ;
		return -1 ;
	}
	
	// Copy new data to DMA IO half buffer which we write so far
	// but DMA doesn't read it yet.
	memcpy(mips_get_uncached_addr(&p_w_buf->data[p_w_buf->w_idx]), p_data, size) ;
	p_w_buf->w_idx += size ;
	
	DTRACEK("written p_w_buf=0x%x, w_idx=%i, size=%i\n", p_w_buf, p_w_buf->w_idx, size) ;
	
	return size ;
}

int dma_start_rx_ch_if_needed(dma_dbl_io_buf_t *p_dbl_buf, size_t len, size_t bs) {
#ifdef DEBUG
	mc_reg_t sr ;
#endif
    
	// Get DMA channel address.
	u_int addr = p_dbl_buf->port_off ;
	
    DTRACEK("BEGIN: p_dbl_buf=0x%x, port_off=0x%x\n", p_dbl_buf, p_dbl_buf->port_off) ;
    
	dma_io_buf_t *p_r_buf = dma_dbl_io_buf_get_r_buf(p_dbl_buf) ;
	dma_io_buf_t *p_w_buf = dma_dbl_io_buf_get_w_buf(p_dbl_buf) ;
	
    // If DMA is not running
    DTRACEK("DMA_PORT_RUN(0x%X).b.run=%i\n", addr, DMA_PORT_RUN(addr).b.run) ;
	if ( !DMA_PORT_RUN(addr).b.run ) {
		//// If reader doesn't have any data to read
		//if(dma_io_buf_get_avail(p_r_buf) == 0) {
        // If we have in writers's(DMA) buffer something to read.
        if ( dma_io_buf_get_fill(p_w_buf) != 0 ) {
            // If we have also in reader's buffer enough spcae for
            // writing a data packet, then switch the buffers
            if ( dma_io_buf_get_avail(p_r_buf) >= len ) {
                // Swap IO buffers
                dma_dbl_io_buf_swap_buf(p_dbl_buf) ;
                // Get a new pointer to read and write IO buffer
                p_r_buf = dma_dbl_io_buf_get_r_buf(p_dbl_buf) ;
                p_w_buf = dma_dbl_io_buf_get_w_buf(p_dbl_buf) ;
                
            } else if (dma_io_buf_get_avail(p_w_buf) < len ) {
                DTRACEK("DONE, -1: Buffer is overrun\n") ;
                return -1 ;
            } 
		}
		
		// If buffer is empty
		if (dma_io_buf_get_fill(p_w_buf) == 0) {
			// Set R/W position to beginning
			dma_io_buf_reset(p_w_buf) ;
		}
		
		// Настраиваем DMA на периём
		DMA_PORT_IR(addr) = (uint32_t)mips_virt_to_phys(&p_w_buf->data[p_w_buf->w_idx]) ; // записываем физический адрес в регистр индекса
		DMA_PORT_CSR(addr).b.wcx = (len / 8) - 1; 	//  WCX = word64s - 1
		DMA_PORT_CSR(addr).b.wn = (bs / 8) - 1 ;   	//   WN = 15 (размер пачки - 16 слов)
		// Start DMA channel
		DMA_PORT_CSR(addr).b.run = 1;		//   RUN = 1
		
		DTRACEK("buf=0x%x, size to receive = %i, w_idx=%i\n", p_w_buf, len, p_w_buf->w_idx) ;
		DTRACEK("Using DMA registers:\n") ;
		DTRACEK("DMA_PORT_IR(0x%x): *0x%x=0x%x\n", addr, &DMA_PORT_IR(addr), DMA_PORT_IR(addr)) ;
		DTRACEK("DMA_PORT_CSR(0x%x): *0x%x=0x%x\n", addr, &DMA_PORT_CSR(addr), DMA_PORT_RUN(addr).dw) ;
            
        //mips_get_sr(sr) ;
        //DTRACEK("sr=0x%x\n", sr) ;
	}
	
	DTRACEK("DONE\n") ;
	
	return 0 ;
}

/*
 * Start DMA channel if thre is some data to tranmit and channel
 * is not already running.
 */
int dma_start_tx_ch_if_needed(dma_dbl_io_buf_t *p_dbl_buf) {
#ifdef DEBUG
	mc_reg_t sr ;
#endif
    
	// Get DMA channel address.
	u_int addr = p_dbl_buf->port_off ;
	// get read IO buffer pointer
	dma_io_buf_t *p_r_buf = dma_dbl_io_buf_get_r_buf(p_dbl_buf) ;
	
	DTRACEK("BEGIN: p_dbl_buf=0x%x, port_off=0x%x\n", p_dbl_buf, p_dbl_buf->port_off) ;
	
	// If DMA is stopped
	if ( !DMA_PORT_RUN(addr).b.run ) {
		
		// If read buffer is empty, than switch buffers
		if( dma_io_buf_get_fill(p_r_buf) == 0) {
            // Reset empty I/O buffer
            dma_io_buf_reset(p_r_buf) ;            
			// Swap IO buffers
			dma_dbl_io_buf_swap_buf(p_dbl_buf) ;
			// Get a new pointer to read IO buffer
			p_r_buf = dma_dbl_io_buf_get_r_buf(p_dbl_buf) ;
		}
		
		// If we have some data it read buffer, then
		// start DMA to transmit that data!
		if( dma_io_buf_get_fill(p_r_buf) != 0 ) {
		
			// Настраиваем DMA на передачу
			DMA_PORT_IR(addr) = (uint32_t)mips_virt_to_phys(&p_r_buf->data[p_r_buf->r_idx]) ; // записываем физический адрес в регистр индекса
			DMA_PORT_CSR(addr).b.wcx =  dma_io_buf_get_fill(p_r_buf)/8 - 1 ; 		//  WCX = words(64) - 1
			DMA_PORT_CSR(addr).b.wn =  15 ;   	//   WN = 15 (размер пачки - 16 слов)
		
			// Start DMA channel
			DMA_PORT_CSR(addr).b.run = 1;		//   RUN = 1
			
			DTRACEK("Len to transmit = %i, r_idx=%i\n", dma_io_buf_get_fill(p_r_buf), p_r_buf->r_idx) ;
			DTRACEK("Using DMA registers:\n") ;
			DTRACEK("DMA_PORT_IR(0x%x): *0x%x=0x%x\n", addr, &DMA_PORT_IR(addr), DMA_PORT_IR(addr)) ;
            DTRACEK("DMA_PORT_CSR(0x%x): *0x%x=0x%x\n", addr, &DMA_PORT_CSR(addr), DMA_PORT_RUN(addr).dw) ;
            
            //mips_get_sr(sr) ;
            //DTRACEK("sr=0x%x\n", sr) ;
		}
	}
	
	DTRACEK("DONE\n") ;
	
	return 0 ;
}

/*
 * Swap IO buffers.
 */
void dma_dbl_io_buf_swap_buf(dma_dbl_io_buf_t *p_dbl_buf) {
	p_dbl_buf->r_idx = p_dbl_buf->w_idx ;
	p_dbl_buf->w_idx = (p_dbl_buf->w_idx + 1) % 2 ;
}

/*
 * Get IO buffer available for reading.
 */
dma_io_buf_t *dma_dbl_io_buf_get_r_buf (dma_dbl_io_buf_t *p_dbl_buf) {

	return &p_dbl_buf->half[p_dbl_buf->r_idx] ;
}

/*
 * Get IO buffer available for writing.
 */
dma_io_buf_t *dma_dbl_io_buf_get_w_buf (dma_dbl_io_buf_t *p_dbl_buf) {

	return &p_dbl_buf->half[p_dbl_buf->w_idx] ;
}

/*
 * Get to know how much data is in the IO buffer.
 */
size_t dma_io_buf_get_fill( dma_io_buf_t *p_buf ) {
    size_t fill ;
    
    fill = p_buf->w_idx - p_buf->r_idx ;
    
    DTRACEK("0x%x: %i\n", p_buf, fill) ;
	return fill ;
}

/*
 * Get to know tail free space for writing is in the IO buffer.
 */
size_t dma_io_buf_get_avail( dma_io_buf_t *p_buf ) {
    size_t avail ;
    
    avail = p_buf->size - p_buf->w_idx ;
    
    DTRACEK("0x%x: size=%i, r_idx=%i, w_idx=%i, avail=%i\n", p_buf, p_buf->size, p_buf->r_idx, p_buf->w_idx, avail) ;
	return avail ;
}

/*
 * Reset read/write position and clear filling.
 */
void dma_io_buf_reset(dma_io_buf_t *p_buf) {
	p_buf->w_idx = 0 ;
	p_buf->r_idx = 0 ;
}
