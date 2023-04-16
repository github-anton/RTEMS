/*
 * This file is subject to the terms and conditions of the
 * Open Software License. 
 * See http://www.opensource.org/licenses/osl-2.0.php for more details.
 *
 * Copyright (C) 2011 Elvees, Inc.
 * Modified 2017, 2018, 2019 by Anton Ermakov
 */

#include <rtems.h>
#include <rtems/io.h>
#include <rtems/rtems/intr.h>
#include <bsp/swic.h>
#include <bsp.h>
#include <bsp/irq.h>
#include <rtems/libio.h>
#include <bsp/swic-regs.h>
#include <bsp/dma.h>
#include <errno.h>
#include <bsp/auxtimer.h>
#include <stdio.h>

#if 0
#   define DEBUG
#endif

#include <bsp/auxio.h>

// DMA buffer size
#define SPW_RX_DATA_BUFSZ	65536
#define SPW_RX_DESC_BUFSZ	384
#define SPW_TX_DATA_BUFSZ	4096
#define SPW_TX_ASYNC_DATA_BUFSZ 163840
#define SPW_TX_ASYNC_DESC_BUFSZ 1536
#define SPW_ONE_RX_BUFFER_DESC_CNT	(SPW_RX_DESC_BUFSZ / 8)
#define SPW_ONE_TX_ASYNC_BUFFER_DESC_CNT  (SPW_TX_ASYNC_DESC_BUFSZ / 8)

const char swic_driver_path[][BSP_DEV_PATH_MAX_LEN] = {
	"/dev/spw0",
	"/dev/spw1",
	"/dev/spw2",
	"/dev/spw3"
} ;

// Формат дескриптора пакета
#if defined(CONFIG_MCT03P)
typedef union __attribute__((__packed__)) _swic_descriptor_t
{
    unsigned	size   : 25;	// Размер данных
    unsigned	unused : 4;	// Не используется
    unsigned	type   : 2;	// Тип конца пакета: EOP или EEP
    unsigned	valid  : 1;	// Признак заполнения дескриптора
		            		// действительными данными
    unsigned	padding;	// Дополнительные неиспользуемые 4 байта,
                        // т.к. DMA передаёт 8-байтовыми словами.
} swic_descriptor_t;

#elif defined(CONFIG_MC30SF6) || defined(CONFIG_MC30MPO2)
typedef struct __attribute__((__packed__)) {
    unsigned	size   : 25;	// Размер данных
    unsigned	unused : 4;	// Не используется
    unsigned	type   : 2;	// Тип конца пакета: EOP или EEP
    unsigned	valid  : 1;	// Признак заполнения дескриптора
		            		// действительными данными
    unsigned    port ;      // SpaceWire port
} swic_descriptor_t;
#endif

// Один буфер двойной буферизации
typedef struct {
	char       *buf;		// Непосредственно буфер (должен быть выравнен на границу 8 байт для
					// нормальной работы DMA)
	char       *rw_p;	    // Текущий указатель, с которого производится чтение/запись
	int 		empty;		// Признак пустого буфера
	unsigned	chain_addr;	// Адрес цепочки инициализации (физ.), если она используется, иначе 0
} swic_one_buffer_t ;

// Двойной буфер
typedef struct {
	swic_one_buffer_t	half [2];	// Две половины буфера
	int		size;		// Размер одной половины
	int		rw_idx;	    // Индекс читателя или писателя
	int		dma_idx;	// Индекс DMA
	int		dma_chan;	// Канал DMA
} swic_double_buffer_t ;

// Элемент цепочки DMA
typedef struct __attribute__((__packed__)) _swic_dma_params_t 
{
	uint32_t	zero;		// Неиспользуемое поле
	uint32_t	ir;		// Значение, загружаемое в IR
	uint32_t	cp;		// Значение, загружаемое в CP
	uint32_t	csr;		// Значение, загружаемое в CSR
} swic_dma_params_t;

typedef struct {
	int			    dev_no;				// Номер канала spacewire, считая от 0
	unsigned	    speed;				// Рабочая скорость канала
	int			    open;				// Признак открытия канала
	int			    connected;
	int			    nonblock;			// Признак неблокирующего ввода-вывода
	char				    name [BSP_DEV_PATH_MAX_LEN];	// Имя устройства
	
	// Sync Writing
	swic_descriptor_t *	    tx_desc;			// Указатель на буфер с дескриптором передаваемого пакета
	char *				    tx_data_buffer;			// Указатель на буфер с данными передаваемого пакета
	
	// Async writing
	swic_double_buffer_t			txa_desc_buf;			// Двойной буфер отправки дескрипторов
	swic_double_buffer_t			txa_data_buf;			// Двойной буфер отправки данных
	//swic_double_buffer_t           txa_desc_chain ;
	//swic_dma_params_t *			txa_desc_chain [2];		// Цепочки DMA для отправки дескрипторов
	
	// Reading
	swic_double_buffer_t			rx_desc_buf;			// Двойной буфер приёма дескрипторов
	swic_double_buffer_t			rx_data_buf;			// Двойной буфер приёма данных
	swic_dma_params_t *			rx_desc_chain [2];		// Цепочки DMA для приёма дескрипторов
	
	rtems_id 	    stws, rxdataws, rxdescws, txws, time;	// Семафоры для функций старта, read и write (в режиме блокирующего ввода-вывода)
	
	// Буфер для дескриптора передаваемого пакета
	swic_descriptor_t		txdescbuf __attribute__ ((aligned (8)));
	// Буфер для передаваемого пакета
	char 				txdatabuf [SPW_TX_DATA_BUFSZ] __attribute__ ((aligned (8)));
    
    // Буфер для отправляемых дескрипторов TX DESC DMA
	swic_descriptor_t 		txasyncdescbuf [2][SPW_ONE_TX_ASYNC_BUFFER_DESC_CNT] __attribute__ ((aligned (8)));
	//swic_dma_params_t			txasyncdescchain [2][SPW_ONE_TX_ASYNC_BUFFER_DESC_CNT] __attribute__ ((aligned (8)));
	// Буфер для принятых данных RX DATA DMA
	char 				txasyncdatabuf [2][SPW_TX_ASYNC_DATA_BUFSZ] __attribute__ ((aligned (8)));
    
	// Буфер для принятых дескрипторов RX DESC DMA
	swic_descriptor_t 		rxdescbuf [2][SPW_ONE_RX_BUFFER_DESC_CNT] __attribute__ ((aligned (8)));
	swic_dma_params_t			rxdescchain [2][SPW_ONE_RX_BUFFER_DESC_CNT] __attribute__ ((aligned (8)));
	// Буфер для принятых данных RX DATA DMA
	char 				rxdatabuf [2][SPW_RX_DATA_BUFSZ] __attribute__ ((aligned (8)));
	
	// Статистика
	unsigned			rx_eop;		// Количество принятых пакетов с EOP
	unsigned			rx_eep;		// Количество принятых пакетов с EEP
	unsigned			rx_bytes;	// Количество принятых байт
	unsigned			tx_packets;	// Количество переданных пакетов
	unsigned			tx_bytes;	// Количество переданных байт
	unsigned			txdma_waits;	// Количество ожиданий освобождения DMA передатчика
#if defined(CONFIG_MC30SF6) || defined(CONFIG_MC30MPO2)
	u_int 				out_port ;
#endif
} swic_t ;

swic_t swic_channel [CONFIG_MULTICORE_SWIC_QTY];

// Interrupt handlers
static void swic_connected_ih (void *dev_id) ;
static void swic_disconnected_ih (void *dev_id) ;
static void swic_dma_tx_desc_ih (void *dev_id) ;
static void swic_dma_tx_data_ih (void *dev_id) ;
static void swic_dma_rx_desc_ih (void *dev_id) ;
static void swic_dma_rx_data_ih (void *dev_id) ;
static void swic_time_ih(void *dev_id) ;

// Routines
static void swic_start (swic_t *u) ;
static inline void swic_start_rx_dma (swic_double_buffer_t *pdbuf) ;
static inline void swic_start_tx_dma (swic_double_buffer_t *pdbuf) ;
static void swic_start_rx_dma_if_needed (swic_double_buffer_t *pdbuf) ;
static void swic_start_tx_dma_if_needed(swic_double_buffer_t *pdbuf) ;
void swic_set_tx_speed (swic_t *u, unsigned mbit_per_sec) ;
static inline int swic_connected (swic_t *u) ;
static void swic_move_reader_p (swic_double_buffer_t *pdbuf, int distance) ;
int swic_write_dbuffer(swic_double_buffer_t *dest, void *source, size_t len) ;
int swic_swap_onebuffers(swic_double_buffer_t *pdbuf) ;
static inline unsigned swic_avail_size (swic_double_buffer_t *pdbuf) ;
static inline unsigned swic_size_to_end_of_cur_buffer (swic_double_buffer_t *pdbuf) ;
void swic_set_dma_enabled(bool dma_en) ;
void swic_set_work_enabled(bool work_en) ;
int swic_write_sync(swic_t *u, char *buf, size_t size) ;
int swic_write_async(swic_t *u, char *buf, size_t size) ;
int swic_set_out_port(swic_t *u, u_int port) ;


#if defined(CONFIG_MC30SF6) || defined(CONFIG_MC30MPO2)

void swic_form_routing_table(bool del_header) ;
void swic_gspw_form_adg(void) ;

void swic_form_routing_table(bool del_header) {
    unsigned int i ;
    DTRACEK("Forming routing table...\n\r") ;
    DTRACEK("MC_ROUTING_TABLE[256]={\n\r") ;
    for ( i = 0 ; i < 7 ; i++) {
        MC_ROUTING_TABLE[i] = (1 << i) | ((del_header & 1) << 8) ;
    }
    // 3-31 ports are not used
    for ( i = 7; i < 32; i++) {
        MC_ROUTING_TABLE[i] = 0x0 ;
    }
    // Assign all logical addresses to port 0
    for ( i = 32; i < 256; i++) {
        MC_ROUTING_TABLE[i] = 0x1 ;
    }
    
#if defined(DEBUG) && 1
    for (i = 0; i <= 255; i++) {
        if (!(i % 12) && i) printk("\n\r") ;

        i < 255 ? printk("0x%X, ", MC_ROUTING_TABLE[i]) :
        printk("0x%X", MC_ROUTING_TABLE[i]) ;
    }
    printk("}\n\r") ;
#endif

}

void swic_gspw_form_adg(void) {
    int i;
    DTRACEK("Form adg...\n\r");
    DTRACEK("MC_GSPW_ADG[6]={") ;
    for (i = 0; i < 6; i++) {
        MC_GSPW_ADG[i] = ((unsigned int)2 << i) ;
#ifdef DEBUG
        i < 5 ? printk("0x%X, ", MC_GSPW_ADG[i]) :
        printk("0x%X", MC_GSPW_ADG[i]) ;
#endif
    }
    DPRINTK("}\n\r") ;
}

void swic_set_dma_enabled(bool dma_en) {
    dma_en ? ( MC_MODE_R(0) |= 0x800000 ) : ( MC_MODE_R(0) &= ~ 0x800000 ) ; // SET_GIGASPWR_MODE_R_DMA_ENABLED(REGS->MODE_R, dma_en);
}

void swic_set_work_enabled(bool work_en) {
    
    MC_MODE_R(0) &= ~ 0xf ;    // Clear koeff10
    
    if ( work_en ) {
        MC_MODE_R(0) |= 0xb ;          // SET_GIGASPWR_MODE_R_MAIN_KOEFF10(REGS->MODE_R, 0xb);
        MC_MODE_R(0) |= 0x2000000 ;    // SET_GIGASPWR_MODE_R_GIGASPWR_WE(REGS->MODE_R, work_en);
    } else {
        MC_MODE_R(0) &= ~ 0x2000000 ;
    }
}

#endif


rtems_device_driver swic_initialize(
	rtems_device_major_number major,
	rtems_device_minor_number minor,
	void *arg
)
{
    DTRACEK("Invoked, major=0x%x, minor=0x%x\n\r", major, minor) ;
	// Initialize device-common data structures here
    int i ;
    for (i = 0; i < CONFIG_MULTICORE_SWIC_QTY; i++) {
		
        rtems_status_code sc = rtems_io_register_name(swic_driver_path[i], major, i);
        DTRACEK("rtems_io_register_name(minor=0x%x, path=%s) returned 0x%x\n\r", i, swic_driver_path[i], sc) ;
		
        if (sc != RTEMS_SUCCESSFUL) {
            TRACEK("FATAL(%i) Can't register device name: %s\n\r", sc, swic_driver_path) ;
            rtems_fatal_error_occurred(sc) ;
        }
    }

#if defined(CONFIG_MC30SF6) || defined(CONFIG_MC30MPO2)

#if 0
    DTRACEK ("Check register's addresses:\n\r") ;
    DTRACEK ("& MC_ROUTING_TABLE[0] = 0x%x\n\r", &MC_ROUTING_TABLE[0]) ;
    DTRACEK ("& MC_GSPW_ADG[0] = 0x%x\n\r", &MC_GSPW_ADG[0]) ;
    DPRINTK ("\n\r") ;
    DTRACEK ("& MC_SWIC_TX_DATA_CSR(0) = 0x%x\n\r", &MC_SWIC_TX_DATA_CSR(0)) ;
    DTRACEK ("& MC_SWIC_TX_DATA_CP(0) = 0x%x\n\r", &MC_SWIC_TX_DATA_CP(0)) ;
    DTRACEK ("& MC_SWIC_TX_DATA_IR(0) = 0x%x\n\r", &MC_SWIC_TX_DATA_IR(0)) ;
    DTRACEK ("& MC_SWIC_TX_DATA_RUN(0) = 0x%x\n\r", &MC_SWIC_TX_DATA_RUN(0)) ;
    DPRINTK ("\n\r") ;
    DTRACEK ("& MC_SWIC_TX_DESC_CSR(0) = 0x%x\n\r", &MC_SWIC_TX_DESC_CSR(0)) ;
    DTRACEK ("& MC_SWIC_TX_DESC_CP(0) = 0x%x\n\r", &MC_SWIC_TX_DESC_CP(0)) ;
    DTRACEK ("& MC_SWIC_TX_DESC_IR(0) = 0x%x\n\r", &MC_SWIC_TX_DESC_IR(0)) ;
    DTRACEK ("& MC_SWIC_TX_DESC_RUN(0) = 0x%x\n\r", &MC_SWIC_TX_DESC_RUN(0)) ;
    DPRINTK ("\n\r") ;
    DTRACEK ("& MC_SWIC_TX_SPEED(0) = 0x%x\n\r", &MC_SWIC_TX_SPEED(0));
    DTRACEK ("& MC_SWIC_TX_SPEED(1) = 0x%x\n\r", &MC_SWIC_TX_SPEED(1));
    DTRACEK ("& MC_GSPW_TX_SPEED(2) = 0x%x\n\r", &MC_GSPW_TX_SPEED(2));
    DTRACEK ("& MC_GSPW_PMA_MODE(2) = 0x%x\n\r", &MC_GSPW_PMA_MODE(2));
    DTRACEK ("& MC_GSPW_TX_SPEED(3) = 0x%x\n\r", &MC_GSPW_TX_SPEED(3));
    DTRACEK ("& MC_GSPW_PMA_MODE(3) = 0x%x\n\r", &MC_GSPW_PMA_MODE(3));
    DPRINTK ("\n\r") ;
    DTRACEK ("& MC_SPW_MODE_CR(0) = 0x%x\n\r", &MC_SPW_MODE_CR(0));
    DTRACEK ("& MC_SPW_MODE_CR(1) = 0x%x\n\r", &MC_SPW_MODE_CR(1));
    DTRACEK ("& MC_SPW_MODE_CR(4) = 0x%x\n\r", &MC_SPW_MODE_CR(4));
    DTRACEK ("& MC_SPW_MODE_CR(5) = 0x%x\n\r", &MC_SPW_MODE_CR(5));
    DTRACEK ("& MC_MODE_R(0) = 0x%x\n\r", &MC_MODE_R(0)) ;
#endif
    
    // Since we have SpaceWire router we should
    // form routing table.
    swic_form_routing_table(FALSE) ;
    swic_gspw_form_adg() ;
#endif
    
	return RTEMS_SUCCESSFUL;
}

static void swic_struct_init (swic_t *u, int dev_no, int speed)
{
	int i, j;
	
	DTRACEK ("swic_struct_init, dev_no = %d\n\r", dev_no);
	
	memset (u, 0, sizeof (swic_t));
	
	u->dev_no = dev_no;
	u->speed = speed;
	memcpy(u->name, swic_driver_path[dev_no], BSP_DEV_PATH_MAX_LEN) ;
	swic_set_out_port(u, 1) ;		// PORT#1 is default output
	
	// Инициализация буферов (двойной буферизации) принятых дескрипторов
	u->rx_desc_buf.size = SPW_ONE_RX_BUFFER_DESC_CNT * sizeof (swic_descriptor_t);
	u->rx_desc_buf.dma_chan = SWIC_RX_DESC_CHAN (dev_no);
	u->rx_desc_buf.half[0].empty = 1;
	u->rx_desc_buf.half[1].empty = 1;
	
	u->rx_desc_buf.half[0].rw_p = u->rx_desc_buf.half[0].buf = (char *) u->rxdescbuf[0];
	u->rx_desc_buf.half[1].rw_p = u->rx_desc_buf.half[1].buf = (char *) u->rxdescbuf[1];
	u->rx_desc_chain[0] = u->rxdescchain[0];
	u->rx_desc_chain[1] = u->rxdescchain[1];
	memset (u->rx_desc_buf.half[0].buf, 0, u->rx_desc_buf.size);
	memset (u->rx_desc_buf.half[1].buf, 0, u->rx_desc_buf.size);

	// Инициализация цепочек самоинициализации RX DESC DMA для каждого
	// приёмного буфера двойной буферизации
	for (j = 0; j < 2; ++j) {
		for (i = 0; i < SPW_ONE_RX_BUFFER_DESC_CNT; ++i) {
			u->rx_desc_chain[j][i].ir  = mips_virt_to_phys (
				(unsigned) (u->rx_desc_buf.half[j].buf + i * sizeof (swic_descriptor_t)));
			u->rx_desc_chain[j][i].csr = MC_DMA_CSR_IM | MC_DMA_CSR_CHEN | 
				MC_DMA_CSR_WN(0) | MC_DMA_CSR_WCX(0) | MC_DMA_CSR_RUN;
			u->rx_desc_chain[j][i].cp  = mips_virt_to_phys ((unsigned) &u->rx_desc_chain[j][i + 1]);
		}
		u->rx_desc_chain[j][i - 1].csr = MC_DMA_CSR_IM | MC_DMA_CSR_WN(0) | MC_DMA_CSR_WCX(0) | MC_DMA_CSR_RUN;
		u->rx_desc_chain[j][i - 1].cp = 0;
		u->rx_desc_buf.half[j].chain_addr = (unsigned) u->rx_desc_chain[j];
	}
	
	DTRACEK ("CSR in chain: %08X, chain addr = %08X\n\r", ((swic_dma_params_t *) u->rx_desc_buf.half[0].chain_addr)->csr,
		u->rx_desc_buf.half[0].chain_addr);

	// Инициализация буферов (двойной буферизации) принятых данных
	u->rx_data_buf.size = (SPW_RX_DATA_BUFSZ / 2 + 7) & ~7;
	u->rx_data_buf.dma_chan = SWIC_RX_DATA_CHAN (dev_no);
	u->rx_data_buf.half[0].empty = 1;
	u->rx_data_buf.half[1].empty = 1;	

	u->rx_data_buf.half[0].rw_p = u->rx_data_buf.half[0].buf = (char *) u->rxdatabuf[0];
	u->rx_data_buf.half[1].rw_p = u->rx_data_buf.half[1].buf = (char *) u->rxdatabuf[1];
	memset (u->rx_data_buf.half[0].buf, 0, u->rx_data_buf.size);
	memset (u->rx_data_buf.half[1].buf, 0, u->rx_data_buf.size);

    // Data transmission
    // Synchronious
	u->tx_desc = &u->txdescbuf;

	u->tx_data_buffer = u->txdatabuf;
    
    // Asynchronious writing
    
    // Инициализация буферов (двойной буферизации) дескрипторов для передачи
	u->txa_desc_buf.size = SPW_ONE_TX_ASYNC_BUFFER_DESC_CNT * sizeof (swic_descriptor_t);
	u->txa_desc_buf.dma_chan = SWIC_TX_DESC_CHAN (dev_no);
    for(i = 0; i < 2; i++) {
        u->txa_desc_buf.half[i].empty = 1;
        u->txa_desc_buf.half[i].rw_p = u->txa_desc_buf.half[i].buf = (char *) u->txasyncdescbuf[i];
        //u->txa_desc_chain.half[i].rw_p = u->txa_desc_chain.half[i].buf = (char*)u->txasyncdescchain[i];
        memset (u->txa_desc_buf.half[i].buf, 0, u->txa_desc_buf.size);
        //u->txa_desc_buf.half[i].chain_addr = (unsigned)u->txa_desc_chain.half[i].buf ;
        u->txa_desc_buf.half[i].chain_addr = 0 ;
    }
    
    // Инициализация буферов (двойной буферизации) передаваемых данных
	u->txa_data_buf.size = (SPW_TX_ASYNC_DATA_BUFSZ / 2 + 7) & ~7;
	u->txa_data_buf.dma_chan = SWIC_TX_DATA_CHAN (dev_no);
    
    
    for (i = 0; i < 2; i++) {
        u->txa_data_buf.half[i].empty = 1;
        u->txa_data_buf.half[i].rw_p = u->txa_data_buf.half[i].buf = (char *) u->txasyncdatabuf[i];
        memset (u->txa_data_buf.half[i].buf, 0, u->txa_data_buf.size);
        u->txa_data_buf.half[i].chain_addr = 0 ;
    }
    
    rtems_semaphore_create(
		rtems_build_name('s', 't', 's', '0' + u->dev_no), 
		0, 
		RTEMS_FIFO | RTEMS_SIMPLE_BINARY_SEMAPHORE | RTEMS_NO_INHERIT_PRIORITY | \
		RTEMS_NO_PRIORITY_CEILING, 
		0, 
		&(u->stws));
    
    rtems_semaphore_create(
		rtems_build_name('r', 'x', 'e', '0' + u->dev_no), 
		0, 
		RTEMS_FIFO | RTEMS_SIMPLE_BINARY_SEMAPHORE | RTEMS_NO_INHERIT_PRIORITY | \
		RTEMS_NO_PRIORITY_CEILING, 
		0, 
		&(u->rxdescws));
    
    rtems_semaphore_create(
		rtems_build_name('r', 'x', 'd', '0' + u->dev_no), 
		0, 
		RTEMS_FIFO | RTEMS_SIMPLE_BINARY_SEMAPHORE | RTEMS_NO_INHERIT_PRIORITY | \
		RTEMS_NO_PRIORITY_CEILING, 
		0, 
		&(u->rxdataws));
	
	rtems_semaphore_create(
		rtems_build_name('t', 'x', 'w', '0' + u->dev_no), 
		0, 
		RTEMS_FIFO | RTEMS_SIMPLE_BINARY_SEMAPHORE | RTEMS_NO_INHERIT_PRIORITY | \
		RTEMS_NO_PRIORITY_CEILING, 
		0, 
		&(u->txws));
    
    rtems_semaphore_create(
		rtems_build_name('t', 'i', 'm', '0' + u->dev_no), 
		0, 
		RTEMS_FIFO | RTEMS_SIMPLE_BINARY_SEMAPHORE | RTEMS_NO_INHERIT_PRIORITY | \
		RTEMS_NO_PRIORITY_CEILING, 
		0, 
		&(u->time));
}

rtems_device_driver swic_open(
	rtems_device_major_number major,
	rtems_device_minor_number minor,
	void *arg
)
{
    DTRACEK("Invoked, major=0x%x, minor=0x%x\n\r", major, minor) ;
    
    unsigned chan_num = minor ;		// получаем номер канала SpW
	//swic_t *u = &swic_channel [chan_num];
	
	// We will use non-cachable segment kseg1 (0xA0000000-0xBFFFFFFF) 
	// instead of cachable segment kseg1 (0x80000000-0x9FFFFFFF) of same physical memory,
	// so we don't have to invalidate cpu data cache before read data from DMA.
	swic_t *u = mips_get_uncached_addr( &swic_channel [chan_num] ) ;
	int ret;
    
	if (chan_num >= CONFIG_MULTICORE_SWIC_QTY)
		return RTEMS_INVALID_NAME ;

	// Запрещаем повторное открытие порта
	if (u->open)
		return RTEMS_RESOURCE_IN_USE ;
		
	swic_struct_init (u, chan_num, CONFIG_MULTICORE_SWIC_DEF_WORK_SPEED);		
	u->open = 1;
	
    // Temporarily disabled
	//u->nonblock = file->f_flags & O_NONBLOCK;

	ret = rtems_interrupt_handler_install (swic_rx_desc_irq(u->dev_no), u->name, RTEMS_INTERRUPT_SHARED, swic_dma_rx_desc_ih, u);
    DTRACEK("rtems_interrupt_handler_install ( swic_dma_rx_desc_ih ) = %i\n\r", ret) ;
    if (ret != RTEMS_SUCCESSFUL) {
        return ret;
    }
    
	ret = rtems_interrupt_handler_install (swic_rx_data_irq(u->dev_no), u->name, RTEMS_INTERRUPT_SHARED, swic_dma_rx_data_ih, u);
    DTRACEK("rtems_interrupt_handler_install ( swic_dma_rx_data_ih ) = %i\n\r", ret) ;
    if (ret != RTEMS_SUCCESSFUL) {
        rtems_interrupt_handler_remove (swic_rx_desc_irq(u->dev_no), swic_dma_rx_desc_ih, u);
        return ret;
	}
	
	ret = rtems_interrupt_handler_install (swic_tx_data_irq (u->dev_no), u->name, RTEMS_INTERRUPT_SHARED, swic_dma_tx_data_ih, u);
    DTRACEK("rtems_interrupt_handler_install ( swic_dma_tx_data_ih ) = %i\n\r", ret) ;
    if (ret != RTEMS_SUCCESSFUL) {
		rtems_interrupt_handler_remove (swic_rx_desc_irq (u->dev_no), swic_dma_rx_desc_ih, u);
		rtems_interrupt_handler_remove (swic_rx_data_irq (u->dev_no), swic_dma_rx_data_ih, u);
		return ret;
	}
	
	ret = rtems_interrupt_handler_install (swic_link_irq (u->dev_no), u->name, RTEMS_INTERRUPT_SHARED, swic_connected_ih, u);
    DTRACEK("rtems_interrupt_handler_install ( swic_connected_ih ) = %i\n\r", ret) ;
    if (ret != RTEMS_SUCCESSFUL) {
		rtems_interrupt_handler_remove (swic_rx_desc_irq (u->dev_no), swic_dma_rx_desc_ih, u);
		rtems_interrupt_handler_remove (swic_rx_data_irq (u->dev_no), swic_dma_rx_data_ih, u);
		rtems_interrupt_handler_remove (swic_tx_data_irq (u->dev_no), swic_dma_tx_data_ih, u);
		return ret;
	}
    
    ret = rtems_interrupt_handler_install (swic_time_irq (u->dev_no), u->name, RTEMS_INTERRUPT_SHARED, swic_time_ih, u);
    DTRACEK("rtems_interrupt_handler_install ( swic_time_ih ) = %i\n\r", ret) ;
    if (ret != RTEMS_SUCCESSFUL) {
		rtems_interrupt_handler_remove (swic_rx_desc_irq (u->dev_no), swic_dma_rx_desc_ih, u);
		rtems_interrupt_handler_remove (swic_rx_data_irq (u->dev_no), swic_dma_rx_data_ih, u);
		rtems_interrupt_handler_remove (swic_tx_data_irq (u->dev_no), swic_dma_tx_data_ih, u);
        rtems_interrupt_handler_remove (swic_link_irq (u->dev_no), swic_connected_ih, u);
		return ret;
	}
    
    ret = rtems_interrupt_handler_install (swic_tx_desc_irq (u->dev_no), u->name, RTEMS_INTERRUPT_SHARED, swic_dma_tx_desc_ih, u);
    DTRACEK("rtems_interrupt_handler_install ( swic_tx_desc_ih ) = %i\n\r", ret) ;
    if (ret != RTEMS_SUCCESSFUL) {
		rtems_interrupt_handler_remove (swic_rx_desc_irq (u->dev_no), swic_dma_rx_desc_ih, u);
		rtems_interrupt_handler_remove (swic_rx_data_irq (u->dev_no), swic_dma_rx_data_ih, u);
		rtems_interrupt_handler_remove (swic_tx_data_irq (u->dev_no), swic_dma_tx_data_ih, u);
        rtems_interrupt_handler_remove (swic_link_irq (u->dev_no), swic_connected_ih, u);
        rtems_interrupt_handler_remove (swic_time_irq (u->dev_no), swic_time_ih, u);
		return ret;
	}
    
    ret = rtems_interrupt_handler_install (swic_err_irq (u->dev_no), u->name, RTEMS_INTERRUPT_SHARED, swic_disconnected_ih, u);
    DTRACEK("rtems_interrupt_handler_install ( swic_disconnected_ih ) = %i\n\r", ret) ;
    if (ret != RTEMS_SUCCESSFUL) {
		rtems_interrupt_handler_remove (swic_rx_desc_irq (u->dev_no), swic_dma_rx_desc_ih, u);
		rtems_interrupt_handler_remove (swic_rx_data_irq (u->dev_no), swic_dma_rx_data_ih, u);
		rtems_interrupt_handler_remove (swic_tx_data_irq (u->dev_no), swic_dma_tx_data_ih, u);
        rtems_interrupt_handler_remove (swic_link_irq (u->dev_no), swic_connected_ih, u);
        rtems_interrupt_handler_remove (swic_time_irq (u->dev_no), swic_time_ih, u);
        rtems_interrupt_handler_remove (swic_tx_desc_irq (u->dev_no), swic_dma_tx_desc_ih, u);
		return ret;
	}
    
    bsp_disable_irq(swic_rx_desc_irq(u->dev_no)) ;
    bsp_disable_irq(swic_tx_data_irq(u->dev_no)) ;
    
    // Disable its or we will get unhandled exception.
    bsp_disable_irq(swic_tx_desc_irq(u->dev_no)) ;
    //bsp_disable_irq(swic_err_irq(u->dev_no)) ;
    bsp_enable_irq(swic_err_irq(u->dev_no)) ;
    //bsp_disable_irq(swic_time_irq(u->dev_no)) ;
	
#if defined(CONFIG_MCT03P)
	DTRACEK ("MASKR2 = 0x%08X, QSTR2 = 0x%08X\n\r", MC_MASKR2, MC_QSTR2);
#elif defined(CONFIG_MC30SF6) || defined(CONFIG_MC30MPO2)
    DTRACEK ("MASKR0 = 0x%08X, QSTR0 = 0x%08X\n\r", MC_MASKR0, MC_QSTR0);
#endif
    
	swic_start (u);

	DTRACEK("Done\n\r") ;
    
	return RTEMS_SUCCESSFUL;
}

// FIXME: After reopening file of device, the first
// packet always is received incorrectly. We hawe to do
// full reset DMA, but I don't know how.
rtems_device_driver swic_close(
	rtems_device_major_number major,
	rtems_device_minor_number minor,
	void *arg
)
{
    int ret ;
    swic_t *u = mips_get_uncached_addr( &swic_channel [minor] ) ;
    
    DTRACEK("Invoked, major=0x%x, minor=0x%x\n\r", major, minor) ;

#if defined(CONFIG_MCT03P)
    // Сброс контроллера
	MC_SWIC_MODE_CR(u->dev_no) = SWIC_RESET;
    
#elif defined(CONFIG_MC30SF6) || defined(CONFIG_MC30MPO2)
    
    swic_set_dma_enabled(FALSE) ;
    swic_set_work_enabled(FALSE) ;
    //MC_MODE_R(0) |= MC_GSPW_DMA_CLR_FIFO ;
    
    /*MC_SPW_MODE_CR(0) = MC_SPW_BdsReset ;
    MC_SPW_MODE_CR(1) = MC_SPW_BdsReset ;
    MC_SPW_MODE_CR(4) = MC_SPW_BdsReset ;
    MC_SPW_MODE_CR(5) = MC_SPW_BdsReset ;*/
#endif
    
    ret = rtems_interrupt_handler_remove (swic_rx_desc_irq (u->dev_no), swic_dma_rx_desc_ih, u) ;
    DTRACEK("rtems_interrupt_handler_remove ( swic_rx_desc_irq ) = %i\n\r", ret) ;
    
    ret = rtems_interrupt_handler_remove (swic_rx_data_irq (u->dev_no), swic_dma_rx_data_ih, u) ;
    DTRACEK("rtems_interrupt_handler_remove ( swic_dma_rx_data_ih ) = %i\n\r", ret) ;
	
    ret = rtems_interrupt_handler_remove (swic_tx_data_irq (u->dev_no), swic_dma_tx_data_ih, u) ;
    DTRACEK("rtems_interrupt_handler_remove ( swic_dma_tx_data_ih ) = %i\n\r", ret) ;
    
    ret = rtems_interrupt_handler_remove (swic_link_irq (u->dev_no), swic_connected_ih, u) ;
    DTRACEK("rtems_interrupt_handler_remove ( swic_connected_ih ) = %i\n\r", ret) ;
    
    ret = rtems_interrupt_handler_remove (swic_time_irq (u->dev_no), swic_time_ih, u) ;
    DTRACEK("rtems_interrupt_handler_remove ( swic_time_ih ) = %i\n\r", ret) ;

    rtems_semaphore_delete(u->stws) ;
    rtems_semaphore_delete(u->rxdescws) ;
    rtems_semaphore_delete(u->rxdataws) ;
    rtems_semaphore_delete(u->txws) ;
    rtems_semaphore_delete(u->time) ;
    
    u->open = 0 ;
    
	return RTEMS_SUCCESSFUL ;
}

rtems_device_driver swic_read(
	rtems_device_major_number major,
	rtems_device_minor_number minor,
	void *arg
)
{
    DTRACEK("Invoked, major=0x%x, minor=0x%x\n\r", major, minor) ;
	
	//swic_t *u = &swic_channel [minor];
    swic_t *u = mips_get_uncached_addr ( &swic_channel [minor] ) ;
	unsigned sz_to_end, nbytes, rest, completed;
	char *pdata;
	swic_descriptor_t *pdesc = (swic_descriptor_t *) u->rx_desc_buf.half[u->rx_desc_buf.rw_idx].rw_p;
    
    rtems_libio_rw_args_t *rw_args = (rtems_libio_rw_args_t*)arg ;
    size_t size = rw_args->count ;
    char *buf = rw_args->buffer ;
    
    DTRACEK ("=== channel = %d, pdesc=0x%x\n\r", u->dev_no, pdesc);
	
	if (! swic_connected (u)) {
		DTRACEK ("swic_read: channel %d not connected, wait for connection\n\r", u->dev_no);
		//MC_SWIC_MODE_CR(u->dev_no) |= MC_SWIC_LINK_MASK;
		swic_start (u) ;
		if (u->nonblock) {
            rw_args->bytes_moved = 0 ;
            // We return -1 and set errno to EAGAIN,
            // because 0 is ambigous.
			DTRACEK("RETURN (RTEMS_TIMEOUT): Not connected\n") ;
			return RTEMS_TIMEOUT ;
		}
		
		// if (wait_event_interruptible (u->stwq, u->connected)) {
		// FIXME: Enbale connected_ih IRQ may be?
		if (rtems_semaphore_obtain(u->stws, RTEMS_WAIT, RTEMS_NO_TIMEOUT)) {
			return RTEMS_UNSATISFIED ;
		}
		
		DTRACEK ("swic_read: channel %d, waiting for connection finished\n\r", u->dev_no);
	}
	
	while (1) {
		if (!pdesc->valid) {
			// Нет принятых дескрипторов
			DTRACEK ("=== swic_read, channel = %d, no data\n\r", u->dev_no);
		
			// Запускаем RX DMA, если он ещё не запущен.
			swic_start_rx_dma_if_needed (&u->rx_desc_buf);
			bsp_disable_irq (swic_rx_data_irq (u->dev_no));
			swic_start_rx_dma_if_needed (&u->rx_data_buf);
			bsp_enable_irq (swic_rx_data_irq (u->dev_no));
		
			// Если неблокирующий режим ввода-вывода и данных нет,
			// то сразу выходим с нулевым результатом.
			if (u->nonblock) {
                rw_args->bytes_moved = 0 ;
				DTRACEK("RETURN (RTEMS_TIMEOUT): No valid descriptors\n") ;
                return RTEMS_TIMEOUT ;
            }

			// Если блокирующий режим, то дожидаемся получения
			// пакета без ошибок
			while (!pdesc->valid) {
				DTRACEK ("=== swic_read, channel = %d, waiting for a packet\n\r", u->dev_no);
				DTRACEK ("RX DESC DMA CSR = %08X, RX DATA DMA CSR = %08X\n\r", 
					SWIC_DMA_RUN (u->rx_desc_buf.dma_chan), SWIC_DMA_RUN (u->rx_data_buf.dma_chan));
				//mutex_wait (&u->rx_desc_lock);
				bsp_enable_irq (swic_rx_desc_irq (u->dev_no));
                //if (wait_event_interruptible (u->rxdescwq, pdesc->valid)) {
				if (rtems_semaphore_obtain (u->rxdescws, RTEMS_WAIT, RTEMS_NO_TIMEOUT)) {
					bsp_disable_irq (swic_rx_desc_irq (u->dev_no));
					return RTEMS_UNSATISFIED;
				}
				bsp_disable_irq (swic_rx_desc_irq (u->dev_no));
				SWIC_DMA_CSR (u->rx_desc_buf.dma_chan);	// Сброс признака прерывания
				DTRACEK ("=== swic_read, channel = %d, waiting done\n\r", u->dev_no);
			}
		}

		pdesc->valid = 0;
		
        DTRACEK("pdesc=0x%x, pdesc->type = %i\n\r", pdesc, pdesc->type) ;
		if (pdesc->type == EOP) {
			break;
		} else {      // EEP
			// Получен дескриптор с ошибкой, ждём следующего дескриптора
			u->rx_eep++;
			swic_move_reader_p (&u->rx_desc_buf, sizeof (swic_descriptor_t));
			pdesc = (swic_descriptor_t *) u->rx_desc_buf.half[u->rx_desc_buf.rw_idx].rw_p;
            DTRACEK("Got new pdesc=0x%x\n\r", pdesc) ;
			
			bsp_disable_irq (swic_rx_data_irq (u->dev_no));
			swic_move_reader_p (&u->rx_data_buf, pdesc->size);
			bsp_enable_irq (swic_rx_data_irq (u->dev_no));
            bsp_enable_irq (swic_rx_desc_irq (u->dev_no));
            
            // In case of EEP we return an error.
            rw_args->bytes_moved = 0 ;
			DTRACEK("RETURN (RTEMS_IO_ERROR): EEP received\n") ;
            return RTEMS_IO_ERROR ;
		}
	}
	
	DTRACEK ("=== swic_read, processing packet\n\r");
	
	completed = 0;
	rest = pdesc->size;
	if (rest > size) {
		// Ошибка: пользователь предоставил маленький буфер
		bsp_disable_irq (swic_rx_data_irq (u->dev_no));
		swic_move_reader_p (&u->rx_data_buf, rest);
		bsp_enable_irq (swic_rx_data_irq (u->dev_no));
		
		swic_move_reader_p (&u->rx_desc_buf, sizeof (swic_descriptor_t));
		DTRACEK("RETURN (RTEMS_INVALID_SIZE): User buffer is too small\n") ;
		return RTEMS_INVALID_SIZE;
	}
	
	bsp_disable_irq (swic_rx_data_irq (u->dev_no));
	
	while (rest > 0) {
		pdata = u->rx_data_buf.half[u->rx_data_buf.rw_idx].rw_p;
		DTRACEK ("pdata = %08X\n\r", pdata);
		/*do nbytes = swic_avail_size (&u->rx_data_buf);
		while (nbytes == 0);*/
        nbytes = swic_avail_size (&u->rx_data_buf);
        if (nbytes == 0) {
            rw_args->bytes_moved = completed ;
            bsp_enable_irq (swic_rx_data_irq (u->dev_no));
            swic_move_reader_p (&u->rx_desc_buf, sizeof (swic_descriptor_t));
			DTRACEK("RETURN (RTEMS_IO_ERROR): Internal error, no more data in DMA buffer\n") ;
            return RTEMS_IO_ERROR ;
        }
        
		DTRACEK ("swic_avail_size = %d, rest = %d\n\r", nbytes, rest);
		sz_to_end = swic_size_to_end_of_cur_buffer (&u->rx_data_buf);
		
		bsp_enable_irq (swic_rx_data_irq (u->dev_no));
		
		if (nbytes > sz_to_end) nbytes = sz_to_end;
		if (nbytes > rest) nbytes = rest;
		memcpy (buf, pdata, nbytes);
		rest -= nbytes;
		completed += nbytes;
	
		bsp_disable_irq (swic_rx_data_irq (u->dev_no));
		swic_move_reader_p (&u->rx_data_buf, nbytes);
	}
	bsp_enable_irq (swic_rx_data_irq (u->dev_no));

	swic_move_reader_p (&u->rx_desc_buf, sizeof (swic_descriptor_t));
	
	u->rx_eop++;
	u->rx_bytes += completed;

	DTRACEK ("=== swic_read exit, channel = %d, completed = %d\n\r", u->dev_no, completed);
	rw_args->bytes_moved = completed;
	
    DTRACEK("RETURN (RTEMS_SUCCESSFUL)\n") ;
	
	return RTEMS_SUCCESSFUL;
}

int swic_write_sync(swic_t *u, char *buf, size_t size) {
    
    unsigned nbytes, completed = 0;
    //int i ;
    //unsigned run ;
    
	DTRACEK ("=== channel = %d, size = %d\n\r", u->dev_no, size);

	if (! swic_connected (u)) {
		DTRACEK ("swic_write: channel %d not connected, wait for connection\n\r", u->dev_no);
		//MC_SWIC_MODE_CR(u->dev_no) |= MC_SWIC_LINK_MASK;
		swic_start (u);
		if (u->nonblock) {
			return -RTEMS_TIMEOUT;
		}
		
		// if (wait_event_interruptible (u->stwq, u->connected)) {
		if (rtems_semaphore_obtain (u->stws, RTEMS_WAIT, RTEMS_NO_TIMEOUT)) {
			return -RTEMS_UNSATISFIED ;
		}
		
		DTRACEK ("swic_write: channel %d, waiting for connection finished\n\r", u->dev_no);
	}
	
	DTRACEK ("MASKR2 = %08X, prev tx_desc = %08X\n\r", MC_MASKR2, *((unsigned *) u->tx_desc));
	
	// Если занят канал TX DMA, ...
	while (MC_SWIC_TX_DATA_RUN (u->dev_no) & 1) {

		// При неблокирующем вводе-выводе, если канал TX DMA занят, сразу выходим 
		if (u->nonblock) {
			return -RTEMS_TIMEOUT;
		}

		// При блокирующем вводе-выводе ждем сигнала о завершении
		// текущей передачи TX DMA. Сигнал приходит от обработчика 
		// прерывания по окончанию передачи TX DMA.
		DTRACEK ("Waiting for TX DATA DMA free, channel %d\n\r", u->dev_no);
		DTRACEK ("RX DATA CSR = %08X\n\r", MC_SWIC_RX_DATA_RUN (!u->dev_no));
		
		bsp_enable_irq (swic_tx_data_irq (u->dev_no));
		// if (wait_event_interruptible (u->txwq, !(MC_SWIC_TX_DATA_RUN (u->dev_no) & 1))) {
        if (rtems_semaphore_obtain (u->txws, RTEMS_WAIT, RTEMS_NO_TIMEOUT)) {
			bsp_disable_irq (swic_tx_data_irq (u->dev_no));
			return RTEMS_UNSATISFIED ;
		}
		bsp_disable_irq (swic_tx_data_irq (u->dev_no));
		
		DTRACEK ("Waiting for TX DMA done\n\r");
		u->txdma_waits++;
	}

	// Настраиваем DMA на выдачу пакета
	u->tx_desc->size = size;
	u->tx_desc->type = EOP;
	u->tx_desc->valid = 1;
	u->tx_desc->unused = 0;
#if defined(CONFIG_MC30SF6) || defined(CONFIG_MC30MPO2)
    //u->tx_desc->port = 0x2 ;	// Send to SWIC0
    //u->tx_desc->port = 0x4 ;	// Send to SWIC1
	//u->tx_desc->port = 0x6  ;	// Send to SWIC0 & SWIC1
	u->tx_desc->port = u->out_port ;	// get output port from config
#endif
    DTRACEK("((unsigned*)tx_desc)[0]=0x%x, ((unsigned*)tx_desc)[1]=0x%x\n\r", ((unsigned*)u->tx_desc)[0], ((unsigned*)u->tx_desc)[1]) ;
	MC_SWIC_TX_DESC_IR(u->dev_no) = mips_virt_to_phys ((unsigned) u->tx_desc); // Адрес начала буфера
	MC_SWIC_TX_DESC_CSR(u->dev_no) = MC_DMA_CSR_WCX(0) | MC_DMA_RUN; // 1 8-байтовое слово
	
	while (size > 0) {
		nbytes = size;
		if (nbytes > SPW_TX_DATA_BUFSZ) nbytes = SPW_TX_DATA_BUFSZ;
		
		memcpy (u->tx_data_buffer, buf + completed, nbytes);	// Приходится копировать из-за проблем в DMA
		
		MC_SWIC_TX_DATA_IR(u->dev_no) = mips_virt_to_phys ((unsigned) u->tx_data_buffer);
		
        DTRACEK ("tp1, TX_DATA_RUN = %08X, TX_DESC_RUN = %08X, channel %d\n\r", 
                MC_SWIC_TX_DATA_RUN(u->dev_no), MC_SWIC_TX_DESC_RUN (u->dev_no), u->dev_no);
        
		DTRACEK ("transmit: TX_DATA_IR = %08X, TX_DESC_IR = %08X, channel %d\n\r", 
                MC_SWIC_TX_DATA_IR (u->dev_no), MC_SWIC_TX_DESC_IR (u->dev_no), u->dev_no);		
		
        MC_SWIC_TX_DATA_CSR(u->dev_no) = MC_DMA_CSR_IM | MC_DMA_CSR_WCX(((nbytes + 7) >> 3) - 1)  | MC_DMA_RUN;
		
		DTRACEK ("tp2, TX_DATA_RUN = %08X, TX_DESC_RUN = %08X, channel %d\n\r", 
			MC_SWIC_TX_DATA_RUN (u->dev_no), MC_SWIC_TX_DESC_RUN (u->dev_no), u->dev_no);

#if 0
    for (i = 0; i < 10; i++) {
        SNOOZE(1) ;
        DTRACEK ("Check DMA: TX_DATA_RUN = %08X, TX_DESC_RUN = %08X, channel %d\n\r", 
            MC_SWIC_TX_DATA_RUN(u->dev_no), MC_SWIC_TX_DESC_RUN (u->dev_no), u->dev_no);
    }
#endif

		completed += nbytes;
		size -= nbytes;

		if (size > 0) {
			bsp_enable_irq (swic_tx_data_irq (u->dev_no));
			//if (wait_event_interruptible (u->txwq, !(MC_SWIC_TX_DATA_RUN (u->dev_no) & 1))) {
            if (rtems_semaphore_obtain (u->txws, RTEMS_WAIT, RTEMS_NO_TIMEOUT)) {
				bsp_disable_irq (swic_tx_data_irq (u->dev_no));
				return RTEMS_UNSATISFIED ;
			}
			bsp_disable_irq (swic_tx_data_irq (u->dev_no));
		}
	}
	
	u->tx_packets++;
	u->tx_bytes += completed;

	return completed ;
}

int swic_write_async(swic_t *u, char *buf, size_t size) {
    int ret ;
    
    DTRACEK("Invoked, buf=0x%x, size=%i\n\r", buf, size) ;
    
    bsp_disable_irq (swic_tx_desc_irq (u->dev_no));
    bsp_disable_irq (swic_tx_data_irq (u->dev_no));
    
	// Create descriptor:
    
    swic_descriptor_t desc ;
    desc.size = size;
	desc.type = EOP;
	desc.valid = 1;
	desc.unused = 0;
#if defined(CONFIG_MC30SF6) || defined(CONFIG_MC30MPO2)
    //desc.port = 0x2 ;	// Send to SWIC0
	//desc.port = 0x4 ;	// Send to SWIC1
	//desc.port = 0x6 ;	// Send to SWIC0 & SWIC 1
	desc.port = u->out_port ;	// get output port from config
	
#endif
    
    ret = swic_write_dbuffer(&u->txa_desc_buf, &desc, sizeof(desc)) ;
    if ( ret < 0 ) {
        bsp_enable_irq (swic_tx_desc_irq (u->dev_no));
        bsp_enable_irq (swic_tx_data_irq (u->dev_no));
        
        return -RTEMS_TOO_MANY ;
    }
	
    // Create data:
    
    //int half_data_no = u->txa_data_buf.rw_idx ;
    ret = swic_write_dbuffer(&u->txa_data_buf, buf, size) ;
    
    /*if ( i == 3 ) {
        bsp_enable_irq (swic_tx_desc_irq (u->dev_no));
        
        // Swap buffers
        //swic_swap_onebuffers(&u->txa_desc_buf) ;
        //swic_swap_onebuffers(&u->txa_data_buf) ;
                
        swic_start_tx_dma_if_needed(&u->txa_desc_buf) ;
        swic_start_tx_dma_if_needed(&u->txa_data_buf) ;
        
        int wcx ;
        int csr ;
        do {
            //csr = MC_SWIC_TX_DATA_CSR(u->dev_no) ;
            csr = MC_SWIC_TX_DESC_CSR(u->dev_no) ;
            wcx = (csr & 0xFFFF0000) >> 16;
            //DTRACEK("Waiting for WCX=0xFFFF (all transmitted), MC_SWIC_TX_DATA_CSR(%i)=0x%x, WCX=0x%x\n\r", u->dev_no, csr, wcx) ;
            DTRACEK("Waiting for WCX=0xFFFF (all transmitted), MC_SWIC_TX_DESC_CSR(%i)=0x%x, WCX=0x%x\n\r", u->dev_no, csr, wcx) ;
        } while ( (wcx & 0xFFFF) != 0xFFFF ) ;
    }*/
    
    swic_start_tx_dma_if_needed(&u->txa_desc_buf) ;
    swic_start_tx_dma_if_needed(&u->txa_data_buf) ;
    
    bsp_enable_irq (swic_tx_desc_irq (u->dev_no));
    bsp_enable_irq (swic_tx_data_irq (u->dev_no));
    
    return ret ;
}

rtems_device_driver swic_write(
	rtems_device_major_number major,
	rtems_device_minor_number minor,
	void *arg
)
{
    DTRACEK("Invoked, major=0x%x, minor=0x%x\n\r", major, minor) ;
	
    swic_t *u = mips_get_uncached_addr ( &swic_channel [minor] ) ;
    rtems_libio_rw_args_t *rw_args = (rtems_libio_rw_args_t*)arg ;
    char *buf = rw_args->buffer ;
    size_t size = rw_args->count ;
    int bytes ;

    bytes = u->nonblock ? swic_write_async (u, buf, size) : swic_write_sync (u, buf, size) ;
    
    if ( bytes < 0 ) {
        rw_args->bytes_moved = 0 ;
        return -bytes ;
    }
    
	rw_args->bytes_moved = bytes;
    DTRACEK ("=== swic_write exit, channel = %d, completed = %d\n\r", u->dev_no, bytes);
    
    return RTEMS_SUCCESSFUL;
}

rtems_device_driver swic_control(
	rtems_device_major_number major,
	rtems_device_minor_number minor,
	void *arg
)
{
    DTRACEK("Invoked, major=0x%x, minor=0x%x\n\r", major, minor) ;
	rtems_libio_ioctl_args_t *ioarg = (rtems_libio_ioctl_args_t *) arg;
    unsigned int *data = ioarg->buffer;
    unsigned int cmd = ioarg->command ;
    
    unsigned int chan_num = minor ;
	//swic_t *u = &swic_channel [chan_num];
    swic_t *u = mips_get_uncached_addr(&swic_channel [chan_num]) ;
	//unsigned freq_mult_mhz;

	switch (cmd) {
    // Nonblock reading mode
    case FIONBIO:
        u->nonblock = *data ;
        // If synchronous mode, then disable all tx IRQ as it requires
        // for swic_write_async() function.
        if (!u->nonblock) {
            bsp_disable_irq (swic_tx_desc_irq (u->dev_no));
            bsp_disable_irq (swic_tx_data_irq (u->dev_no));
        }
        ioarg->ioctl_return = 0 ;
        break ;
        
	// Получение значения частоты приема
	case SPW_GET_RX_SPEED:
		//freq_mult_mhz = MC_FREQUENCY_MULTIPLIER * MC_QUARTZ_CLOCK_FREQ;
		//ioarg->ioctl_return = ((MC_SWIC_RX_SPEED (u->dev_no) * freq_mult_mhz / 1000000) >> 7);
        ioarg->ioctl_return = (MC_SWIC_RX_SPEED (u->dev_no) * CPU_CLOCK_RATE_MHZ * 8 / 1024);
        break ;

	// Получение значения частоты передачи
	case SPW_GET_TX_SPEED:
		ioarg->ioctl_return = (MC_SWIC_TX_SPEED (u->dev_no) & MC_SWIC_TX_SPEED_PRM_MASK) * 5;
        break ;

	// Установка значения частоты передачи
	case SPW_SET_TX_SPEED:
        DTRACEK("SPW_SET_SPEED request, data[0]=%i\n\r", data[0]) ;
		if (data[0] < 5 || data[0] > 400) return RTEMS_UNSATISFIED;
		swic_set_tx_speed (u, data[0]);
		ioarg->ioctl_return = 0;
        break ;
		
	// Получение количества успешно принятых пакетов (EOP)
	case SPW_GET_STAT_RX_EOP:
		ioarg->ioctl_return = u->rx_eop;
        break ;
		
	// Получение количества принятых пакетов с ошибкой (EEP)
	case SPW_GET_STAT_RX_EEP:
		ioarg->ioctl_return = u->rx_eep;
        break ;

	// Получение количества принятых байт
	case SPW_GET_STAT_RX_BYTES:
		ioarg->ioctl_return = u->rx_bytes;
        break ;
		
	// Получение количества переданных пакетов
	case SPW_GET_STAT_TX_PACKETS:
		ioarg->ioctl_return = u->tx_packets;
        break ;

	// Получение количества переданных байт
	case SPW_GET_STAT_TX_BYTES:
		ioarg->ioctl_return = u->tx_bytes;
        break ;

	// Получение количества ожиданий освобождения DMA передачи
	case SPW_GET_STAT_TX_DMA_WAITS:
		ioarg->ioctl_return = u->txdma_waits;
        break ;
			
	// Сброс статистики
	case SPW_RESET_STAT:
		u->rx_eop = 0;
		u->rx_eep = 0;
		u->rx_bytes = 0;
		u->tx_packets = 0;
		u->tx_bytes = 0;
		u->txdma_waits = 0;
		ioarg->ioctl_return = 0;
        break ;
		
	//	Return states of all ports in corresponding bits in case of router,
	//  or just connectd/disconnected state in case of adapter.
	//	Returns 0 if all ports are disconnected.
    case SPW_IS_CONNECTED:
        DTRACEK("SPW_IS_CONNECTED request, swic_connected() return 0x%X\n\r", swic_connected(u)) ;
        ioarg->ioctl_return = swic_connected(u) ;
        break ;
        
    case SPW_RX_IS_EMPTY:
        ioarg->ioctl_return = u->rx_data_buf.half[u->rx_data_buf.rw_idx].empty ;
        break ;

#if defined(CONFIG_MCT03P)
    // It is possible in the MC30SF6 board to catch the time codes
    // they is stored into CUR_TIME and TRUE_TIME registers.
    case SPW_WAIT_TIME_CODE:
        if (rtems_semaphore_obtain(u->time, RTEMS_WAIT, RTEMS_NO_TIMEOUT)) {
			return RTEMS_UNSATISFIED ;
		}
		
    case SPW_GET_TIME_CODE:
        ioarg->ioctl_return = MC_SWIC_RX_CODE(u->dev_no) & MC_SWIC_TIME_CODE ;
        break ;
#endif
    
	case SPW_SET_OUTPUT_PORT:
		ioarg->ioctl_return = swic_set_out_port(u, *data) ;
		break ;
		
	default:
		return RTEMS_NOT_IMPLEMENTED ;
	}
    
	return RTEMS_SUCCESSFUL;
}

//
// Обработчик прерывания по завершению приёма данных
//
static void swic_dma_rx_data_ih (void *dev_id)
{
	swic_t *u = dev_id;
	
	// Снимаем признак прерывания
	MC_SWIC_RX_DATA_CSR (u->dev_no);
	
    // Temporarily disabled
	swic_start_rx_dma_if_needed (&u->rx_data_buf);
	
	rtems_semaphore_release( u->rxdataws );
}

//
// Обработчик прерывания по приёму дескриптора
//
static void swic_dma_rx_desc_ih (void *dev_id)
{
	swic_t *u = mips_get_uncached_addr( dev_id );
#ifdef DEBUG
	swic_descriptor_t *pdesc = (swic_descriptor_t *) u->rx_desc_buf.half[u->rx_desc_buf.rw_idx].rw_p;
#endif

// We use uncached segment kseg1 for DMA data buffer, so we do not need
// to invalidate CPU data cache.
/*#ifdef BSP_ENABLE_CPU_CACHE
    cpu_cache_invalidate_data() ;
#endif*/
    
    DTRACEK("Descriptor has been received, pdesc: *0x%x=0x%x, type=0x%x\n\r", pdesc,  *pdesc, pdesc->type) ;
    
	// Снимаем признак прерывания
	MC_SWIC_RX_DESC_CSR (u->dev_no);
    
	rtems_semaphore_release (u->rxdescws);
}

//
// Обработчик прерывания по завершению выдачи данных
//
static void swic_dma_tx_data_ih (void *dev_id)
{
	swic_t *u = dev_id;
	
    DTRACEK("All data transmitted, channel=%i\n\r", u->dev_no) ;
    
	// Снимаем признак прерывания
	MC_SWIC_TX_DATA_CSR (u->dev_no);
    
    if (u->nonblock) {
        //printk("%x: i\n\r", (unsigned)&u->txa_data_buf & 0xFF) ;
        u->txa_data_buf.half[u->txa_data_buf.dma_idx].empty = TRUE ;
        swic_start_tx_dma_if_needed(&u->txa_data_buf) ;
    } else {
        rtems_semaphore_release (u->txws);
    }
}

//
// Обработчик прерывания по завершению выдачи дескриптора
// For asynchronous writing only
//
static void swic_dma_tx_desc_ih (void *dev_id)
{
	swic_t *u = dev_id;
	
    DTRACEK("All descriptors transmitted, channel=%i\n\r", u->dev_no) ;
    
	// Снимаем признак прерывания
	MC_SWIC_TX_DESC_CSR (u->dev_no);
    
    if (u->nonblock) {
        u->txa_desc_buf.half[u->txa_desc_buf.dma_idx].empty = TRUE ;
        swic_start_tx_dma_if_needed(&u->txa_desc_buf) ;
    }
}

//
// Обработчик прерывания по установке соединения
//
static void swic_connected_ih (void *dev_id)
{
	swic_t *u = dev_id;
	
	DTRACEK ("BEGIN, channel: %d, STATUS = %08X\n\r", u->dev_no, MC_SWIC_STATUS (u->dev_no));
	
#if defined(CONFIG_MCT03P)
	if (MC_SWIC_STATUS (u->dev_no) & 0xE) {
		MC_SWIC_STATUS (u->dev_no) |= 0xF;
		DTRACEK ("Error detected, waiting reconnection\n\r");
		return ;
	}
    
    MC_SWIC_STATUS (u->dev_no) |= MC_SWIC_CONNECTED;      // clear IRQ

#elif defined(CONFIG_MC30SF6) || defined(CONFIG_MC30MPO2)
    // Clear all connected IRQ's
    DTRACEK("MC_STATE_R=0x%x\n", MC_STATE_R) ;
    
    MC_STATE_R = MC_STATE_R & MC_STATE_R_PORT_CONNECTED ;
    bsp_disable_irq(swic_link_irq (u->dev_no)) ;
    bsp_enable_irq(swic_err_irq(u->dev_no)) ;
    
    DTRACEK("CLEAR IRQ, MC_STATE_R=0x%x\n", MC_STATE_R) ;
#endif
	
#if defined(CONFIG_MCT03P)
    /* Fix: DMA appear to be run when program is restarted, but actually it is not.
    * So, start dma in any case.
    */
    //if ( !(SWIC_DMA_RUN (u->rx_desc_buf.dma_chan))) swic_start_dma (&u->rx_desc_buf);
    //if ( !(SWIC_DMA_RUN (u->rx_data_buf.dma_chan))) swic_start_dma (&u->rx_data_buf);
    swic_start_rx_dma (&u->rx_desc_buf);
    swic_start_rx_dma (&u->rx_data_buf);
    
#elif defined(CONFIG_MC30SF6) || defined(CONFIG_MC30MPO2)
    //if ( !(SWIC_DMA_RUN (u->rx_desc_buf.dma_chan))) swic_start_dma (&u->rx_desc_buf);
    //if ( !(SWIC_DMA_RUN (u->rx_data_buf.dma_chan))) swic_start_dma (&u->rx_data_buf);
    //swic_start_dma (&u->rx_desc_buf);
    //swic_start_dma (&u->rx_data_buf);
    //swic_start_rx_dma_if_needed (&u->rx_desc_buf);
    //swic_start_rx_dma_if_needed (&u->rx_data_buf);
    if ( !(SWIC_DMA_RUN (u->rx_desc_buf.dma_chan) & 1) ) {
        SWIC_DMA_CSR (u->rx_desc_buf.dma_chan);
        swic_start_rx_dma (&u->rx_desc_buf) ;
    } else {
        DTRACEK("DMA RX Desc dma_chan=0x%x is already running\n", u->rx_desc_buf.dma_chan) ;
    }
    if ( !(SWIC_DMA_RUN (u->rx_data_buf.dma_chan) & 1) ) {
        SWIC_DMA_CSR (u->rx_data_buf.dma_chan);
        swic_start_rx_dma (&u->rx_data_buf) ;
    } else {
        DTRACEK("DMA RX Data dma_chan=0x%x is already running\n", u->rx_data_buf.dma_chan) ;
    }
    
    /*if(MC_MODE_R(0) & MC_GSPW_DMA_CLR_FIFO) {
        MC_MODE_R(0) &= 0xffffffff ^ MC_GSPW_DMA_CLR_FIFO ;     // Disable clearing mode
        swic_move_reader_p (&u->rx_desc_buf, sizeof (swic_descriptor_t)) ;
        return ;
    }*/
    
#endif
	
    swic_set_tx_speed (u, u->speed);
	
    rtems_semaphore_release (u->stws);
    DTRACEK("RELEASE semaphore u->stws: *0x%x=0x%x\n", &u->stws, u->stws) ;

#if defined(CONFIG_MCT03P)    
    // Закрываем прерывание, иначе оно будет возникать по приему каждого пакета
    MC_SWIC_MODE_CR(u->dev_no) &= ~MC_SWIC_LINK_MASK;
#endif
	
    u->connected = 1 ;
    DTRACEK("DONE\n") ;
}

//
// Обработчик прерывания по разрыву соединения
//
static void swic_disconnected_ih (void *dev_id)
{
	swic_t *u = dev_id;
	
	DTRACEK ("BEGIN, channel: %d, STATUS = %08X\n\r", u->dev_no, MC_SWIC_STATUS (u->dev_no));
	
#if defined(CONFIG_MC30SF6) || defined(CONFIG_MC30MPO2)
    DTRACEK("MC_STATE_R=0x%x\n", MC_STATE_R) ;
    
    // Clear all connected IRQ's
    MC_STATE_R = MC_STATE_R & MC_STATE_R_PORT_ERRORED ;
    bsp_disable_irq(swic_err_irq (u->dev_no)) ;
    bsp_enable_irq(swic_link_irq(u->dev_no)) ;
    
    DTRACEK("CLEAR IRQ, MC_STATE_R=0x%x\n", MC_STATE_R) ;
#endif
    
    u->connected = 0 ;
    DTRACEK("DONE\n") ;
}

// Handle time codes
static void swic_time_ih(void *dev_id) {
    swic_t *u = dev_id ;
    
    DTRACEK("SWIC STATUS=0x%x\n\r", MC_SWIC_STATUS (u->dev_no) ) ;
    DTRACEK ("Time code event: RX_CODE%i=0x%x\n\r", u->dev_no, MC_SWIC_RX_CODE(u->dev_no));
    
    MC_SWIC_STATUS (u->dev_no) |= MC_SWIC_GOT_TIME ;      // clear IRQ
    MC_SWIC_STATUS (u->dev_no) |= MC_SWIC_IRQ_TIM ;
    
    rtems_semaphore_release( u->time );
}

static void swic_start (swic_t *u)
{
	u->connected = 0;

#if defined(CONFIG_MCT03P) // CONFIG_MCT03P
    
	// Включение частоты
	MC_CLKEN |= MC_CLKEN_SWIC(u->dev_no);
	
	// Сброс контроллера
	MC_SWIC_MODE_CR(u->dev_no) = SWIC_RESET;
	
	// Запись тайминга для того, чтобы SWIC отрабатывал временные интер-
	// валы в соответствии со стандартом SpaceWire
	//MC_SWIC_MODE_CR(u->dev_no) = MC_SWIC_TIMING_WR_EN;
	//MC_SWIC_TX_SPEED(u->dev_no) = MC_SWIC_TIMING(KHZ/10000);
	
	MC_SWIC_MODE_CR(u->dev_no) = SWIC_START;

	// Начальные установки и пуск
	MC_SWIC_TX_SPEED(u->dev_no) = MC_SWIC_TX_SPEED_PRM(CONFIG_MULTICORE_SWIC_START_SPEED / 5) | MC_SWIC_PLL_TX_EN | MC_SWIC_LVDS_EN;
	//MC_SWIC_TX_SPEED(u->dev_no) = MC_SWIC_TX_SPEED_CON(SPW_START_SPEED / 5) | 
	//	MC_SWIC_TX_SPEED_PRM(u->speed / 5) | MC_SWIC_PLL_TX_EN | MC_SWIC_LVDS_EN;
    
	//mdelay (20);
	DTRACEK ("MC_SWIC_TX_SPEED(%d) = %08X\n\r", u->dev_no, MC_SWIC_TX_SPEED(u->dev_no));

	MC_SWIC_MODE_CR(u->dev_no) = MC_SWIC_LinkStart | MC_SWIC_AutoStart | 
		MC_SWIC_WORK_TYPE | MC_SWIC_LINK_MASK | MC_SWIC_TCODE_MASK | MC_SWIC_TIM_MASK ;
		
	//MC_SWIC_MODE_CR(u->dev_no) = MC_SWIC_LinkStart | MC_SWIC_AutoStart | 
	//	MC_SWIC_WORK_TYPE | MC_SWIC_LINK_MASK | MC_SWIC_AUTO_TX_SPEED;
      
	// Сброс всех признаков прерываний
	MC_SWIC_STATUS(u->dev_no) = SWIC_RESET_ALL_IRQS;

	// Сброс счетчиков принятых пакетов
	MC_SWIC_CNT_RX_PACK(u->dev_no) = 0;
	MC_SWIC_CNT_RX0_PACK(u->dev_no) = 0;
    
#elif defined(CONFIG_MC30SF6) || defined(CONFIG_MC30MPO2) // CONFIG_MC30SF6
    
    bsp_disable_irq(swic_link_irq (u->dev_no)) ;
    
    /* Set up initial speed */
    /* Set transmit speed spw = 10, gspw = 25 */
    DTRACEK("Setup transmit speed:\n\r") ;
    // For SPW0
    MC_SWIC_TX_SPEED(0) = MC_SWIC_TX_SPEED_PRM(CONFIG_MULTICORE_SWIC_START_SPEED / 5)
    | MC_SWIC_TX_SPEED_PRM(CONFIG_MULTICORE_SWIC_START_SPEED / 5) << 10
    | MC_SWIC_PLL_TX_EN | MC_SWIC_LVDS_EN | 0xC0000 ;
    DTRACEK ("MC_SWIC_TX_SPEED(0) = %08X\n\r", MC_SWIC_TX_SPEED(0));
    // For SPW1
    MC_SWIC_TX_SPEED(1) = MC_SWIC_TX_SPEED_PRM(CONFIG_MULTICORE_SWIC_START_SPEED / 5)
    | MC_SWIC_TX_SPEED_PRM(CONFIG_MULTICORE_SWIC_START_SPEED / 5) << 10
    | MC_SWIC_PLL_TX_EN | MC_SWIC_LVDS_EN | 0xC0000 ;
    DTRACEK ("MC_SWIC_TX_SPEED(1) = %08X\n\r", MC_SWIC_TX_SPEED(1));
    // For GSPW2
    MC_GSPW_PMA_MODE(2) = 0x1322e0 ;
    MC_GSPW_PMA_MODE(2) = 0x1326e0 ;
    DTRACEK ("MC_GSPW_PMA_MODE(2) = %08X\n\r", MC_GSPW_PMA_MODE(2));
    MC_GSPW_TX_SPEED(2) = 0x3fc1 ;
    DTRACEK ("MC_GSPW_TX_SPEED(2) = %08X\n\r", MC_GSPW_TX_SPEED(2));
    // For GSPW3
    MC_GSPW_PMA_MODE(3) = 0x1322e0 ;
    MC_GSPW_PMA_MODE(3) = 0x1326e0 ;
    DTRACEK ("MC_GSPW_PMA_MODE(3) = %08X\n\r", MC_GSPW_PMA_MODE(3));
    MC_GSPW_TX_SPEED(3) = 0x3fc1 ;
    DTRACEK ("MC_GSPW_TX_SPEED(3) = %08X\n\r", MC_GSPW_TX_SPEED(3));
        
    /* Set receive speed gspw = 25 */
    DTRACEK("Setup receive speed:\n\r") ;
    // For GSPW2
    MC_GSPW_PMA_MODE(2) = 0x132699 ;
    DTRACEK ("MC_GSPW_PMA_MODE(2) = %08X\n\r", MC_GSPW_PMA_MODE(2));
    // For GSPW3
    MC_GSPW_PMA_MODE(3) = 0x1c0299 ;
    DTRACEK ("MC_GSPW_PMA_MODE(3) = %08X\n\r", MC_GSPW_PMA_MODE(3));
    // For GSPW2
    MC_SPW_MODE_CR(4) = 0x2c83419 ;
    DTRACEK ("MC_SPW_MODE_CR(4) = %08X\n\r", MC_SPW_MODE_CR(4));
    // For GSPW3
    MC_SPW_MODE_CR(5) = 0x2c83419 ;
    DTRACEK ("MC_SPW_MODE_CR(5) = %08X\n\r", MC_SPW_MODE_CR(5));
    
    //MC_MODE_R(0) &= 0xffffffff ^ MC_GSPW_DMA_CLR_FIFO ;     // Disable clearing mode
    swic_set_dma_enabled(TRUE) ;
    swic_set_work_enabled(TRUE) ;
    
    DTRACEK("MC_MODE_R(0) = 0x%x\n\r", MC_MODE_R(0)) ;
    
    /* Start connecting */
    
    // For SPW0
    DTRACEK("Setup link connection:\n\r") ;
    MC_SPW_MODE_CR(0) = MC_SPW_AutoStart | MC_SPW_LinkStart | MC_SPW_BdsReset 
        /*| MC_SPW_CodecLoopback*/ | (0xd << 10) ;     // 0x340e
    DTRACEK ("MC_SPW_MODE_CR(0) = %08X\n\r", MC_SPW_MODE_CR(0));
    
    DTRACEK ("MC_RISC_IRQ_MASK=0x%X\n\r", MC_RISC_IRQ_MASK) ;
    DTRACEK ("MC_QSTR0 = 0x%X\n\r", MC_QSTR0) ;
    
    // For SPW1
    MC_SPW_MODE_CR(1) = MC_SPW_AutoStart | MC_SPW_LinkStart | MC_SPW_BdsReset 
        /*| MC_SPW_CodecLoopback*/ | (0xd << 10) ;     // 0x340e
    DTRACEK ("MC_SPW_MODE_CR(1) = %08X\n\r", MC_SPW_MODE_CR(1));
    // For GSPW2
    MC_SPW_MODE_CR(4) = MC_SPW_AutoStart | MC_SPW_LinkStart | MC_SPW_BdsReset 
        | MC_SPW_CodecLoopback | (0xd << 10); // 0x341e
    DTRACEK ("MC_SPW_MODE_CR(4) = %08X\n\r", MC_SPW_MODE_CR(4));
    // For GSPW3
    MC_SPW_MODE_CR(5) = MC_SPW_AutoStart | MC_SPW_LinkStart | MC_SPW_BdsReset 
        | MC_SPW_CodecLoopback | (0xd << 10); // 0x341e
    DTRACEK ("MC_SPW_MODE_CR(5) = %08X\n\r", MC_SPW_MODE_CR(5));
    
    /* After Link IRQ is occured, DMA will be started and default work
     * tx speed will set up.
     * For IRQ Link enabled it should be:
     *  QSTR0:[24] = 1 ;
     *  MC_RISC_IRQ_MASK:[0] = 1 ;
     */
    
    bsp_enable_irq(swic_link_irq (u->dev_no)) ;
#endif

}

//
// Запуск канала DMA для указанного двойного буфера
// на приём
//
static inline void swic_start_rx_dma (swic_double_buffer_t *pdbuf)
{
	swic_one_buffer_t *pcur;
	
	DTRACEK ("dma_chan = 0x%04X, dma_idx = %d\n\r", pdbuf->dma_chan, pdbuf->dma_idx);
	
	pcur = &pdbuf->half[pdbuf->dma_idx];
	pcur->empty = 0;
	DTRACEK ("pcur->chain_addr = %08X\n\r", pcur->chain_addr);
	if (pcur->chain_addr) {
		DTRACEK ("First CSR in chain: %08X\n\r", ((swic_dma_params_t *) pcur->chain_addr)->csr);
		SWIC_DMA_CP (pdbuf->dma_chan) = mips_virt_to_phys (pcur->chain_addr) | 1;
	} else {
		DTRACEK ("pcur->buf = %08X\n\r", pcur->buf);
		SWIC_DMA_IR (pdbuf->dma_chan) = mips_virt_to_phys ((unsigned) pcur->buf);
        int len = pdbuf->size ;
        SWIC_DMA_CSR (pdbuf->dma_chan) = MC_DMA_CSR_WN(0) | MC_DMA_CSR_IM | MC_DMA_CSR_WCX((len >> 3) - 1) | MC_DMA_CSR_RUN;
	}
}

//
// Запуск канала DMA для указанного двойного буфера
// на передачу
//
static inline void swic_start_tx_dma (swic_double_buffer_t *pdbuf)
{
	swic_one_buffer_t *pcur;
	
	DTRACEK ("dma_chan = 0x%04X, dma_idx = %d\n\r", pdbuf->dma_chan, pdbuf->dma_idx);
	
	pcur = &pdbuf->half[pdbuf->dma_idx];
	pcur->empty = 0;
	DTRACEK ("pcur->chain_addr = %08X\n\r", pcur->chain_addr);
	if (pcur->chain_addr) {
		DTRACEK ("First CSR in chain: %08X\n\r", ((swic_dma_params_t *) pcur->chain_addr)->csr);
		SWIC_DMA_CP (pdbuf->dma_chan) = mips_virt_to_phys (pcur->chain_addr) | 1;
	} else {
		DTRACEK ("pcur->buf = %08X\n\r", pcur->buf);
		SWIC_DMA_IR (pdbuf->dma_chan) = mips_virt_to_phys ((unsigned) pcur->buf);
        int len = (unsigned)pcur->rw_p - (unsigned)pcur->buf ;
        SWIC_DMA_CSR (pdbuf->dma_chan) = MC_DMA_CSR_WN(0) | MC_DMA_CSR_IM | MC_DMA_CSR_WCX((len >> 3) - 1) | MC_DMA_CSR_RUN;
	}
}

//
// Запуск DMA, если соблюдены необходимые условия
//
static void swic_start_rx_dma_if_needed (swic_double_buffer_t *pdbuf)
{
	DTRACEK ("BEGIN: SWIC_DMA_RUN(0x%04X) = %08X, second empty: %d, dma idx: %d, rd idx: %d\n\r", 
		pdbuf->dma_chan, SWIC_DMA_RUN (pdbuf->dma_chan), pdbuf->half[!pdbuf->dma_idx].empty,
		pdbuf->dma_idx, pdbuf->rw_idx);
	if ( !(SWIC_DMA_RUN (pdbuf->dma_chan) & 1) 	// Если канал DMA сейчас остановлен
	     && pdbuf->half[!pdbuf->dma_idx].empty)	// и соседний буфер пуст, то
	{
		SWIC_DMA_CSR (pdbuf->dma_chan);
		// Переключаем DMA на приём в соседний буфер
		pdbuf->dma_idx = !pdbuf->dma_idx;
		swic_start_rx_dma (pdbuf);
	}
	DTRACEK("DONE\n") ;
}

static void swic_start_tx_dma_if_needed(swic_double_buffer_t *pdbuf) {
    DTRACEK ("BEGIN: SWIC_DMA_RUN(0x%04X) = %08X, half0 empty: %d, half1 empty: %d, dma_idx: %d, rw_idx: %d\n\r", 
		pdbuf->dma_chan, SWIC_DMA_RUN (pdbuf->dma_chan), pdbuf->half[0].empty, pdbuf->half[1].empty,
		pdbuf->dma_idx, pdbuf->rw_idx);
    // If DMA is not running
    if ( !(SWIC_DMA_RUN (pdbuf->dma_chan) & 1) ) {
        //printk("%x: s\n\r", (unsigned)pdbuf & 0xFF) ;
        // If there is something to tranmit
        if( !pdbuf->half[pdbuf->rw_idx].empty ) {  
            swic_swap_onebuffers(pdbuf) ;
            swic_start_tx_dma(pdbuf) ;
        }
    } else {
        //printk("%x: r\n\r", (unsigned)pdbuf & 0xFF) ;
    }
    DTRACEK("DONE\n") ;
}

void swic_set_tx_speed (swic_t *u, unsigned mbit_per_sec)
{
#if defined(CONFIG_MC30SF6) || defined(CONFIG_MC30MPO2)
    int speed = mbit_per_sec / 5 ;
#endif
	u->speed = mbit_per_sec;
    
#if defined(CONFIG_MCT03P)	// SPW Adapter
	MC_SWIC_TX_SPEED (u->dev_no) = (MC_SWIC_TX_SPEED (u->dev_no) & ~MC_SWIC_TX_SPEED_PRM_MASK) |
		MC_SWIC_TX_SPEED_PRM (mbit_per_sec / 5);
#elif defined(CONFIG_MC30SF6) || defined(CONFIG_MC30MPO2)		// SPW Router
    MC_SWIC_TX_SPEED(0) = speed | (speed << 10) | 0x100 | 0x200 | 0xC0000 ;
    MC_SWIC_TX_SPEED(1) = speed | (speed << 10) | 0x100 | 0x200 | 0xC0000 ;
#endif
}
/*
 * Return status if SPW interface is connected. In case of router,
 * status all ports will be returned where each bit corresponds particular port.
 * In both cases returned value can be interpreted as boolean value: at least
 * one port is connected = true, or all ports are disconnected = false.
 */
static inline int swic_connected (swic_t *u)
{
    //DTRACEK ( "MC_SWIC_STATUS(%i)=0x%x\n\r", u->dev_no, MC_SWIC_STATUS (u->dev_no) ) ;
#if defined(CONFIG_MCT03P)
	return ((MC_SWIC_STATUS (u->dev_no) & 0x30E0) == 0x30A0);
#elif defined(CONFIG_MC30SF6) || defined(CONFIG_MC30MPO2)
    //return ((MC_SWIC_STATUS (u->dev_no) & 0x70) == 0x50);
    //return ((MC_STATE_R & MC_STATE_R_SPW_CONNECTED) >> 5) ;
	//return ((MC_STATE_R & MC_STATE_R_PORT_CONNECTED) >> 17) ;
	return	((((MC_STATE_R & MC_STATE_R_SPW_CONNECTED) >> 5) & 0xF) ^
			((MC_STATE_R & MC_STATE_R_PORT_CONNECTED) >> 17)) << 1 ;
#endif
}

// Moving the pointer of reader/writer in the double buffer
static void swic_move_reader_p (swic_double_buffer_t *pdbuf, int distance)
{
	swic_one_buffer_t *pcur = &pdbuf->half[pdbuf->rw_idx];
	int size_to_end = swic_size_to_end_of_cur_buffer (pdbuf);
	int aligned_distance = (distance + 7) & ~7;
	
	DTRACEK ("swic_move_reader_p, distance = %d, size_to_end = %d\n\r", 
		aligned_distance, size_to_end);
	
	if (aligned_distance < size_to_end)
		pcur->rw_p += aligned_distance;
	else {
		pcur->empty = 1;
		pdbuf->rw_idx = !pdbuf->rw_idx;
		pcur = &pdbuf->half[pdbuf->rw_idx];
		pcur->rw_p = pcur->buf + (aligned_distance - size_to_end);
		if (pcur->chain_addr) {
			swic_start_rx_dma_if_needed (pdbuf);
		}
	}
}

int swic_write_dbuffer(swic_double_buffer_t *dest, void *source, size_t len) {
    
    // Since DMA reads memory by 8 bytre blocks
    unsigned shift = (len + 7) & 0xFFFFFFF8 ;
    
    if ( dest->half[dest->rw_idx].rw_p + shift <= dest->half[dest->rw_idx].buf + dest->size ) {
        memcpy(dest->half[dest->rw_idx].rw_p, source, len) ;
        dest->half[dest->rw_idx].rw_p = dest->half[dest->rw_idx].rw_p + shift ;
        dest->half[dest->rw_idx].empty = FALSE ;
        return shift ;
    } else {
        //printk("%x: o\n\r", (unsigned)dest & 0xFF) ;
    }
    return -RTEMS_INVALID_SIZE ;
}

int swic_swap_onebuffers(swic_double_buffer_t *pdbuf) {
    // FIXME: Check buffer aviability first:

    pdbuf->rw_idx = !pdbuf->rw_idx ;
    pdbuf->dma_idx = !pdbuf->rw_idx ;
    pdbuf->half[pdbuf->rw_idx].rw_p = pdbuf->half[pdbuf->rw_idx].buf ;
    
    return 0 ;
}

//
// Расчет доступной для чтения информации в указанном буфере @pbuf.
//
static inline unsigned swic_avail_size (swic_double_buffer_t *pdbuf)
{
	swic_one_buffer_t *pcur = &pdbuf->half[pdbuf->rw_idx];
	
	int sz = SWIC_DMA_IR (pdbuf->dma_chan) - mips_virt_to_phys ((unsigned) pcur->rw_p);
	DTRACEK ("swic_avail_size: sz = %d, DMA CSR = %08X, IR = %08X, addr = %08X\n\r", sz, SWIC_DMA_CSR (pdbuf->dma_chan),
		SWIC_DMA_IR (pdbuf->dma_chan), mips_virt_to_phys ((unsigned) pcur->rw_p));
	if (sz >= 0) return sz;
	else return (sz + SPW_RX_DATA_BUFSZ);
}

static inline unsigned swic_size_to_end_of_cur_buffer (swic_double_buffer_t *pdbuf)
{
	swic_one_buffer_t *pcur = &pdbuf->half[pdbuf->rw_idx];
	return (pdbuf->size - ((char*)pcur->rw_p - pcur->buf)); 
}

#if defined(CONFIG_MC30SF6) || defined(CONFIG_MC30MPO2)
/*
 * Set default output port.
 */
int swic_set_out_port(swic_t *u, u_int port) {

	DTRACEK("BEGIN: port=%i\n", port) ;
	// Check port correctness
	if (port > 2) {
		return -1 ;
	}
	// 0 - config, 1 - SIWIC#0, 2 - SWIC#1
	u->out_port = 1 << port ;
	
	DTRACEK("out_port=0x%x\n", u->out_port) ;
	
	return 0 ;
}
#endif
