#include <rtems.h>
#include <rtems/io.h>
#include <rtems/rtems/intr.h>
#include <bsp/mcspw.h>
#include <bsp.h>
#include <bsp/irq.h>
#include <rtems/libio.h>

#if 0
#   define DEBUG
#endif
#include <bsp/aux.h>

#define MCSPW_DEV_PATH_LEN   16
static const char mcspw_driver_path [][MCSPW_DEV_PATH_LEN] = { "/dev/spw0", "/dev/spw1" } ;
#define MCSPW_DEV_NUM   (sizeof(mcspw_driver_path)/MCSPW_DEV_PATH_LEN)

#define NB_OF_CHANNELS	2
#define DEVNAME "spw"

#define EOP     1
#define EEP     2

#define SWIC_RESET          	0x00000079
#define SWIC_START          	0x00000101
#define SWIC_RESET_ALL_IRQS 	0x0001d00f

#define SPW_RX_DATA_BUFSZ	65536
#define SPW_RX_DESC_BUFSZ	384
#define SPW_TX_DATA_BUFSZ	2048
#define SPW_ONE_BUFFER_DESC_CNT	(SPW_RX_DESC_BUFSZ / 48)

// Макросы для вычисления номеров прерываний
#define IRQ_SHIFT	3
#define spw_err_irq(port_id)		(MC1892_IRQ_SW0_ERR + ((port_id) << IRQ_SHIFT))
#define spw_time_irq(port_id)		(MC1892_IRQ_SW0_TIME + ((port_id) << IRQ_SHIFT))
#define spw_link_irq(port_id)		(MC1892_IRQ_SW0_LINK + ((port_id) << IRQ_SHIFT))
#define spw_rx_data_irq(port_id)	(MC1892_IRQ_SW0_RX_DATA + ((port_id) << IRQ_SHIFT))
#define spw_rx_desc_irq(port_id)	(MC1892_IRQ_SW0_RX_DESC + ((port_id) << IRQ_SHIFT))
#define spw_tx_data_irq(port_id)	(MC1892_IRQ_SW0_TX_DATA + ((port_id) << IRQ_SHIFT))
#define spw_tx_desc_irq(port_id)	(MC1892_IRQ_SW0_TX_DESC + ((port_id) << IRQ_SHIFT))

// Доступ к регистрам любого канала SWIC DMA по его адресу
#define SWIC_DMA_CSR(addr)	MC_R ((addr) + 0x0)	// Регистр управления и состояния канала
#define SWIC_DMA_CP(addr)	MC_R ((addr) + 0x4)	// Регистр указателя цепочки канала
#define SWIC_DMA_IR(addr)	MC_R ((addr) + 0x8)	// Индексный регистр внешней памяти канала
#define SWIC_DMA_RUN(addr)	MC_R ((addr) + 0xC)	// Псевдорегистр управления состоянием бита RUN
                                                // According to documentation RUN - writing
                                                // END, DONE - reading

// Адреса каналов DMA
#define SWIC_RX_DESC_CHAN(port)	(0xC800 + ((port) << 12))
#define SWIC_RX_DATA_CHAN(port)	(0xC840 + ((port) << 12))
#define SWIC_TX_DESC_CHAN(port)	(0xC880 + ((port) << 12))
#define SWIC_TX_DATA_CHAN(port)	(0xC8C0 + ((port) << 12))

/*
 * Регистры канала DMA записи в память дескрипторов принимаемых пакетов SWIC
 */
#define MC_SWIC_RX_DESC_CSR(n)	MC_R (0xC800 + ((n) << 12))	/* Регистр управления и состояния канала */
#define MC_SWIC_RX_DESC_CP(n)	MC_R (0xC804 + ((n) << 12))	/* Регистр указателя цепочки канала */
#define MC_SWIC_RX_DESC_IR(n)	MC_R (0xC808 + ((n) << 12))	/* Индексный регистр внешней памяти канала */
#define MC_SWIC_RX_DESC_RUN(n)	MC_R (0xC80C + ((n) << 12))	/* Псевдорегистр управления состоянием бита RUN */

/*
 * Регистры канала DMA записи в память принимаемых слов данных SWIC
 */
#define MC_SWIC_RX_DATA_CSR(n)	MC_R (0xC840 + ((n) << 12))	/* Регистр управления и состояния канала */
#define MC_SWIC_RX_DATA_CP(n)	MC_R (0xC844 + ((n) << 12))	/* Регистр указателя цепочки канала */
#define MC_SWIC_RX_DATA_IR(n)	MC_R (0xC848 + ((n) << 12))	/* Индексный регистр внешней памяти канала */
#define MC_SWIC_RX_DATA_RUN(n)	MC_R (0xC84C + ((n) << 12))	/* Псевдорегистр управления состоянием бита RUN */

/*
 * Регистры канала DMA чтения из памяти дескрипторов передаваемых пакетов SWIC
 */
#define MC_SWIC_TX_DESC_CSR(n)	MC_R (0xC880 + ((n) << 12))	/* Регистр управления и состояния канала */
#define MC_SWIC_TX_DESC_CP(n)	MC_R (0xC884 + ((n) << 12))	/* Регистр указателя цепочки канала */
#define MC_SWIC_TX_DESC_IR(n)	MC_R (0xC888 + ((n) << 12))	/* Индексный регистр внешней памяти канала */
#define MC_SWIC_TX_DESC_RUN(n)	MC_R (0xC88C + ((n) << 12))	/* Псевдорегистр управления состоянием бита RUN */

/*
 * Регистры канала DMA чтения из памяти передаваемых слов данных SWIC
 */
#define MC_SWIC_TX_DATA_CSR(n)	MC_R (0xC8C0 + ((n) << 12))	/* Регистр управления и состояния канала */
#define MC_SWIC_TX_DATA_CP(n)	MC_R (0xC8C4 + ((n) << 12))	/* Регистр указателя цепочки канала */
#define MC_SWIC_TX_DATA_IR(n)	MC_R (0xC8C8 + ((n) << 12))	/* Индексный регистр внешней памяти канала */
#define MC_SWIC_TX_DATA_RUN(n)	MC_R (0xC8CC + ((n) << 12))	/* Псевдорегистр управления состоянием бита RUN */

/*
 * Регистры SpaceWire
 */
#define MC_SWIC_HW_VER(n)	MC_R (0xC000 + ((n) << 12))	/* Регистр аппаратной версии контроллера */
#define MC_SWIC_STATUS(n)	MC_R (0xC004 + ((n) << 12))	/* Регистр состояния */
#define MC_SWIC_RX_CODE(n)	MC_R (0xC008 + ((n) << 12))	/* Регистр принятого управляющего символа */
#define MC_SWIC_MODE_CR(n)	MC_R (0xC00C + ((n) << 12))	/* Регистр управления режимом работы */
#define MC_SWIC_TX_SPEED(n)	MC_R (0xC010 + ((n) << 12))	/* Регистр управления скоростью передачи */
#define MC_SWIC_TX_CODE(n)	MC_R (0xC014 + ((n) << 12))	/* Регистр передаваемого управляющего символа */
#define MC_SWIC_RX_SPEED(n)	MC_R (0xC018 + ((n) << 12))	/* Регистр измерителя скорости приема */
#define MC_SWIC_CNT_RX_PACK(n)	MC_R (0xC01C + ((n) << 12))	/* Регистр счетчика принятых пакетов ненулевой длины */
#define MC_SWIC_CNT_RX0_PACK(n)	MC_R (0xC020 + ((n) << 12))	/* Регистр счетчика принятых пакетов нулевой длины */
#define MC_SWIC_ISR_L(n)	MC_R (0xC024 + ((n) << 12))	/* Регистр кодов распределенных прерываний (младшая часть) */
#define MC_SWIC_ISR_H(n)	MC_R (0xC028 + ((n) << 12))	/* Регистр кодов распределенных прерываний (старшая часть) */

/*
 * Маски для установки отдельных полей регистров
 */

/* STATUS */
#define MC_SWIC_DC_ERR      0x00000001  /* Признак ошибки рассоединения */
#define MC_SWIC_P_ERR       0x00000002  /* Признак ошибки четности */
#define MC_SWIC_ESC_ERR     0x00000004  /* Признак ошибки в ESC-последовательности */
#define MC_SWIC_CREDIT_ERR  0x00000008  /* Признак ошибки кредитования */
#define MC_SWIC_DS_STATE    0x000000E0  /* Состояние DS-макроячейки */
#define MC_SWIC_RX_BUF_FULL 0x00000100  /* Буфер приема полон */
#define MC_SWIC_RX_BUF_EMPTY    0x00000200  /* Буфер приема пуст */
#define MC_SWIC_TX_BUF_FULL 0x00000400  /* Буфер передачи полон */
#define MC_SWIC_TX_BUF_EMPTY    0x00000800  /* Буфер передачи пуст */
#define MC_SWIC_GOT_FIRST_BIT   0x00001000  /* Признак получения первого бита */
#define MC_SWIC_CONNECTED   0x00002000  /* Признак установленного соединения */
#define MC_SWIC_GOT_TIME    0x00004000  /* Принят маркер времени из сети */
#define MC_SWIC_GOT_INT     0x00008000  /* Принят код распределенного прерывания из сети */
#define MC_SWIC_GOT_POLL    0x00010000  /* Принят poll-код из сети */
#define MC_SWIC_FL_CONTROL  0x00020000  /* Признак занятости передачей управляющего кода */
#define MC_SWIC_IRQ_LINK    0x00040000  /* Состояние запроса прерырывания LINK */
#define MC_SWIC_IRQ_TIM     0x00080000  /* Состояние запроса прерырывания TIM */
#define MC_SWIC_IRQ_ERR     0x00100000  /* Состояние запроса прерырывания ERR */

/* Значения поля DS_STATE регистра STATUS */
#define MC_SWIC_DS_ERROR_RESET  0
#define MC_SWIC_DS_ERROR_WAIT   1
#define MC_SWIC_DS_READY    2
#define MC_SWIC_DS_STARTED  3
#define MC_SWIC_DS_CONNECTING   4
#define MC_SWIC_DS_RUN      5

/* RX_CODE */
#define MC_SWIC_TIME_CODE   0x000000FF  /* Значение маркера времени, принятого из сети последним */
#define MC_SWIC_INT_CODE    0x0000FF00  /* Значение кода распределенного прерывания, принятого из сети последним */
#define MC_SWIC_POLE_CODE   0x00FF0000  /* Значение poll-кода, принятого из сети последним */

/* MODE_CR */
#define MC_SWIC_LinkDisabled    0x00000001  /* Установка LinkDisabled для блока DS-кодирования */
#define MC_SWIC_AutoStart   0x00000002  /* Установка AutoStart для блока DS-кодирования */
#define MC_SWIC_LinkStart   0x00000004  /* Установка LinkStart для блока DS-кодирования */
#define MC_SWIC_RX_RST      0x00000008  /* Установка блока приема в начальное состояние */
#define MC_SWIC_TX_RST      0x00000010  /* Установка блока передачи в начальное состояние */
#define MC_SWIC_DS_RST      0x00000020  /* Установка DS-макроячейки в начальное состояние */
#define MC_SWIC_SWCORE_RST  0x00000040  /* Установка контроллера в начальное состояние */
#define MC_SWIC_WORK_TYPE   0x00000100  /* Тип режима работы */
#define MC_SWIC_TIMING_WR_EN    0x00004000  /* Разрешение записи в поле TIMING регистра TX_SPEED */
#define MC_SWIC_AUTO_TX_SPEED   0x00008000  /* Признак автоматического установления скорости передачи
                           после соединения (см. Спецификацию!!!) */
#define MC_SWIC_LINK_MASK   0x00040000  /* Маска прерывания LINK */
#define MC_SWIC_ERR_MASK    0x00080000  /* Маска прерывания TIM */
#define MC_SWIC_TIM_MASK    0x00100000  /* Маска прерывания ERR */
#define MC_SWIC_TCODE_MASK  0x00400000  /* Enable interrupt in case receiving a time code. */
#define MC_SWIC_INT_MASK    0x00800000  /* Enable dIRQ */

/* TX_SPEED */
#define MC_SWIC_TX_SPEED_PRM_MASK   0xFF        /* Маска коэффициента умножения TX_PLL */
#define MC_SWIC_TX_SPEED_PRM(x) ((x) & 0xFF)        /* Установка коэффициента умножения TX_PLL */
#define MC_SWIC_PLL_TX_EN   0x00000100      /* Разрешение работы TX_PLL */
#define MC_SWIC_LVDS_EN     0x00000200      /* Разрешение работы приемопередатчиков LVDS */
#define MC_SWIC_TX_SPEED_CON(x) (((x) & 0xFF) << 10)    /* Скорость передачи данных при установлении соединения */
#define MC_SWIC_TIMING(x)   (((x) & 0xF) << 20) /* В это поле необходимо записать код, равный тактовой
                               частоте работы CPU, деленной на 10*/

/* TX_CODE */
#define MC_SWIC_TXCODE      0x0000001F  /* Управляющий код (содержимое) */
#define MC_SWIC_CODETYPE    0x000000E0  /* Признак кода управления */

/* Значение поля CODETYPE регистра TX_CODE */
#define MC_SWIC_CODETYPE_TIME   2
#define MC_SWIC_CODETYPE_INT    3
#define MC_SWIC_CODETYPE_POLL   5

// Формат дескриптора пакета
typedef struct __attribute__((__packed__)) _spw_descriptor_t
{
	unsigned	size   : 25;	// Размер данных
	unsigned	unused : 4;	// Не используется
	unsigned	type   : 2;	// Тип конца пакета: EOP или EEP
	unsigned	valid  : 1;	// Признак заполнения дескриптора
		            		// действительными данными
	unsigned	padding;	// Дополнительные неиспользуемые 4 байта,
					// т.к. DMA передаёт 8-байтовыми словами.
} spw_descriptor_t;

// Один буфер двойной буферизации
typedef struct _io_buffer {
	char *		buf;		// Непосредственно буфер (должен быть выравнен на границу 8 байт для
					// нормальной работы DMA)
	char *		reader_p;	// Текущий указатель, с которого производится чтение
	int 		empty;		// Признак пустого буфера
	unsigned	chain_addr;	// Адрес цепочки инициализации (физ.), если она используется, иначе 0
} one_buffer;

// Двойной буфер
typedef struct _double_buffer {
	one_buffer	half [2];	// Две половины буфера
	int		size;		// Размер одной половины
	int		reader_idx;	// Индекс читателя
	int		dma_idx;	// Индекс DMA
	int		dma_chan;	// Канал DMA
} double_buffer;

// Элемент цепочки DMA
typedef struct __attribute__((__packed__)) _dma_params_t 
{
	uint32_t	zero;		// Неиспользуемое поле
	uint32_t	ir;		// Значение, загружаемое в IR
	uint32_t	cp;		// Значение, загружаемое в CP
	uint32_t	csr;		// Значение, загружаемое в CSR
} dma_params_t;

struct _spw_t {
	int				port;				// Номер канала spacewire, считая от 0
	unsigned			speed;				// Рабочая скорость канала
	int				open;				// Признак открытия канала
	int				connected;
	int				nonblock;			// Признак неблокирующего ввода-вывода
	char				name [sizeof (DEVNAME) + 1];	// Имя устройства
	spw_descriptor_t *		tx_desc;			// Указатель на буфер с дескриптором передаваемого пакета
	char *				tx_data_buffer;			// Указатель на буфер с данными передаваемого пакета
	double_buffer			rx_desc_buf;			// Двойной буфер приёма дескрипторов
	double_buffer			rx_data_buf;			// Двойной буфер приёма данных
	dma_params_t *			rx_desc_chain [2];		// Цепочки DMA для приёма дескрипторов
	rtems_id 	stws, rxdataws, rxdescws, txws, time;	// Семафоры для функций старта, read и write (в режиме блокирующего ввода-вывода)
	
	// Буфер для дескриптора передаваемого пакета
	spw_descriptor_t		txdescbuf __attribute__ ((aligned (8)));
	// Буфер для передаваемого пакета
	char 				txbuf [SPW_TX_DATA_BUFSZ] __attribute__ ((aligned (8)));
	// Буфер для принятых дескрипторов RX DESC DMA
	spw_descriptor_t 		rxdescbuf [2][SPW_ONE_BUFFER_DESC_CNT] __attribute__ ((aligned (8)));
	dma_params_t			rxdescchain [2][SPW_ONE_BUFFER_DESC_CNT] __attribute__ ((aligned (8)));
	// Буфер для принятых данных RX DATA DMA
	char 				rxdatabuf [2][SPW_RX_DATA_BUFSZ / 2] __attribute__ ((aligned (8)));
	
	// Статистика
	unsigned			rx_eop;		// Количество принятых пакетов с EOP
	unsigned			rx_eep;		// Количество принятых пакетов с EEP
	unsigned			rx_bytes;	// Количество принятых байт
	unsigned			tx_packets;	// Количество переданных пакетов
	unsigned			tx_bytes;	// Количество переданных байт
	unsigned			txdma_waits;	// Количество ожиданий освобождения DMA передатчика
};
typedef struct _spw_t spw_t;

spw_t spw_channel [NB_OF_CHANNELS];

static void spw_connected_ih (void *dev_id) ;
static void spw_dma_tx_data_ih (void *dev_id) ;
static void spw_dma_rx_desc_ih (void *dev_id) ;
static void spw_dma_rx_data_ih (void *dev_id) ;
static void spw_time_ih(void *dev_id) ;

static void spw_start (spw_t *u) ;
static inline void start_dma (double_buffer *pdbuf) ;
static void start_rx_dma_if_needed (double_buffer *pdbuf) ;
void spw_set_tx_speed (spw_t *u, unsigned mbit_per_sec) ;
static inline int spw_connected (spw_t *u) ;
static void move_reader_p (double_buffer *pdbuf, int distance) ;
static inline unsigned avail_size (double_buffer *pdbuf) ;
static inline unsigned size_to_end_of_cur_buffer (double_buffer *pdbuf) ;

rtems_device_driver mcspw_initialize(
	rtems_device_major_number major,
	rtems_device_minor_number minor,
	void *arg
)
{
    DTRACEK("Invoked, major=0x%x, minor=0x%x\n", major, minor) ;
	// Initialize device-common data structures here
    int i ;
    for (i = 0; i < MCSPW_DEV_NUM; i++) {
        rtems_status_code sc = rtems_io_register_name(mcspw_driver_path[i], major, i);
        DTRACEK("rtems_io_register_name(minor=0x%x, path=%s) returned 0x%x\n", i, mcspw_driver_path[i], sc) ;
        if (sc != RTEMS_SUCCESSFUL) {
            TRACEK("FATAL(%i) Can't register device name: %s\n", sc, mcspw_driver_path[i]) ;
            rtems_fatal_error_occurred(sc) ;
        }
    }
	return RTEMS_SUCCESSFUL;
}

static void spw_struct_init (spw_t *u, int port, int speed)
{
	int i, j;
	
	DTRACEK ("spw_struct_init, port = %d\n", port);
	
	memset (u, 0, sizeof (spw_t));
	
	u->port = port;
	u->speed = speed;
	
	// Инициализация буферов (двойной буферизации) принятых дескрипторов
	u->rx_desc_buf.size = SPW_ONE_BUFFER_DESC_CNT * sizeof (spw_descriptor_t);
	u->rx_desc_buf.dma_chan = SWIC_RX_DESC_CHAN (port);
	u->rx_desc_buf.half[0].empty = 1;
	u->rx_desc_buf.half[1].empty = 1;
	
	u->rx_desc_buf.half[0].reader_p = u->rx_desc_buf.half[0].buf = (char *) u->rxdescbuf[0];
	u->rx_desc_buf.half[1].reader_p = u->rx_desc_buf.half[1].buf = (char *) u->rxdescbuf[1];
	u->rx_desc_chain[0] = u->rxdescchain[0];
	u->rx_desc_chain[1] = u->rxdescchain[1];
	memset (u->rx_desc_buf.half[0].buf, 0, u->rx_desc_buf.size);
	memset (u->rx_desc_buf.half[1].buf, 0, u->rx_desc_buf.size);

	// Инициализация цепочек самоинициализации RX DESC DMA для каждого
	// приёмного буфера двойной буферизации
	for (j = 0; j < 2; ++j) {
		for (i = 0; i < SPW_ONE_BUFFER_DESC_CNT; ++i) {
			u->rx_desc_chain[j][i].ir  = mips_virt_to_phys (
				(unsigned) (u->rx_desc_buf.half[j].buf + i * sizeof (spw_descriptor_t)));
			u->rx_desc_chain[j][i].csr = MC_DMA_CSR_IM | MC_DMA_CSR_CHEN | 
				MC_DMA_CSR_WN(0) | MC_DMA_CSR_WCX(0) | MC_DMA_CSR_RUN;
			u->rx_desc_chain[j][i].cp  = mips_virt_to_phys ((unsigned) &u->rx_desc_chain[j][i + 1]);
		}
		u->rx_desc_chain[j][i - 1].csr = MC_DMA_CSR_IM | MC_DMA_CSR_WN(0) | MC_DMA_CSR_WCX(0) | MC_DMA_CSR_RUN;
		u->rx_desc_chain[j][i - 1].cp = 0;
		u->rx_desc_buf.half[j].chain_addr = (unsigned) u->rx_desc_chain[j];
	}
	
	DTRACEK ("CSR in chain: %08X, chain addr = %08X\n", ((dma_params_t *) u->rx_desc_buf.half[0].chain_addr)->csr,
		u->rx_desc_buf.half[0].chain_addr);

	// Инициализация буферов (двойной буферизации) принятых данных
	u->rx_data_buf.size = (SPW_RX_DATA_BUFSZ / 2 + 7) & ~7;
	u->rx_data_buf.dma_chan = SWIC_RX_DATA_CHAN (port);
	u->rx_data_buf.half[0].empty = 1;
	u->rx_data_buf.half[1].empty = 1;	

	u->rx_data_buf.half[0].reader_p = u->rx_data_buf.half[0].buf = (char *) u->rxdatabuf[0];
	u->rx_data_buf.half[1].reader_p = u->rx_data_buf.half[1].buf = (char *) u->rxdatabuf[1];
	memset (u->rx_data_buf.half[0].buf, 0, u->rx_data_buf.size);
	memset (u->rx_data_buf.half[1].buf, 0, u->rx_data_buf.size);

	u->tx_desc = &u->txdescbuf;
	
	u->tx_data_buffer = u->txbuf;
    
    rtems_semaphore_create(
		rtems_build_name('s', 't', 's', '0' + u->port), 
		0, 
		RTEMS_FIFO | RTEMS_SIMPLE_BINARY_SEMAPHORE | RTEMS_NO_INHERIT_PRIORITY | \
		RTEMS_NO_PRIORITY_CEILING, 
		0, 
		&(u->stws));
    
    rtems_semaphore_create(
		rtems_build_name('r', 'x', 'e', '0' + u->port), 
		0, 
		RTEMS_FIFO | RTEMS_SIMPLE_BINARY_SEMAPHORE | RTEMS_NO_INHERIT_PRIORITY | \
		RTEMS_NO_PRIORITY_CEILING, 
		0, 
		&(u->rxdescws));
    
    rtems_semaphore_create(
		rtems_build_name('r', 'x', 'd', '0' + u->port), 
		0, 
		RTEMS_FIFO | RTEMS_SIMPLE_BINARY_SEMAPHORE | RTEMS_NO_INHERIT_PRIORITY | \
		RTEMS_NO_PRIORITY_CEILING, 
		0, 
		&(u->rxdataws));
	
	rtems_semaphore_create(
		rtems_build_name('t', 'x', 'w', '0' + u->port), 
		0, 
		RTEMS_FIFO | RTEMS_SIMPLE_BINARY_SEMAPHORE | RTEMS_NO_INHERIT_PRIORITY | \
		RTEMS_NO_PRIORITY_CEILING, 
		0, 
		&(u->txws));
    
    rtems_semaphore_create(
		rtems_build_name('t', 'i', 'm', '0' + u->port), 
		0, 
		RTEMS_FIFO | RTEMS_SIMPLE_BINARY_SEMAPHORE | RTEMS_NO_INHERIT_PRIORITY | \
		RTEMS_NO_PRIORITY_CEILING, 
		0, 
		&(u->time));
}

rtems_device_driver mcspw_open(
	rtems_device_major_number major,
	rtems_device_minor_number minor,
	void *arg
)
{
    DTRACEK("Invoked, major=0x%x, minor=0x%x\n", major, minor) ;
    
    unsigned chan_num = minor ;		// получаем номер канала SpW
	spw_t *u = &spw_channel [chan_num];
	int ret;
    
	if (chan_num >= NB_OF_CHANNELS)
		return RTEMS_INVALID_NAME ;

	// Запрещаем повторное открытие порта
	if (u->open)
		return RTEMS_RESOURCE_IN_USE ;
		
	spw_struct_init (u, chan_num, CONFIG_MULTICORE_SWIC_DEF_WORK_SPEED);		
	u->open = 1;
	
    // Temporarily disabled
	//u->nonblock = file->f_flags & O_NONBLOCK;

	ret = rtems_interrupt_handler_install (spw_rx_desc_irq(u->port), u->name, RTEMS_INTERRUPT_SHARED, spw_dma_rx_desc_ih, u);
      	if (ret != RTEMS_SUCCESSFUL) return ret;
	ret = rtems_interrupt_handler_install (spw_rx_data_irq(u->port), u->name, RTEMS_INTERRUPT_SHARED, spw_dma_rx_data_ih, u);
      	if (ret != RTEMS_SUCCESSFUL) {
            rtems_interrupt_handler_remove (spw_rx_desc_irq(u->port), spw_dma_rx_desc_ih, u);
		return ret;
	}
	ret = rtems_interrupt_handler_install (spw_tx_data_irq (u->port), u->name, RTEMS_INTERRUPT_SHARED, spw_dma_tx_data_ih, u);
      	if (ret != RTEMS_SUCCESSFUL) {
		rtems_interrupt_handler_remove (spw_rx_desc_irq (u->port), spw_dma_rx_desc_ih, u);
		rtems_interrupt_handler_remove (spw_rx_data_irq (u->port), spw_dma_rx_data_ih, u);
		return ret;
	}
	ret = rtems_interrupt_handler_install (spw_link_irq (u->port), u->name, RTEMS_INTERRUPT_SHARED, spw_connected_ih, u);
      	if (ret != RTEMS_SUCCESSFUL) {
		rtems_interrupt_handler_remove (spw_rx_desc_irq (u->port), spw_dma_rx_desc_ih, u);
		rtems_interrupt_handler_remove (spw_rx_data_irq (u->port), spw_dma_rx_data_ih, u);
		rtems_interrupt_handler_remove (spw_tx_data_irq (u->port), spw_dma_tx_data_ih, u);
		return ret;
	}
    
    ret = rtems_interrupt_handler_install (spw_time_irq (u->port), u->name, RTEMS_INTERRUPT_SHARED, spw_time_ih, u);
      	if (ret != RTEMS_SUCCESSFUL) {
		rtems_interrupt_handler_remove (spw_rx_desc_irq (u->port), spw_dma_rx_desc_ih, u);
		rtems_interrupt_handler_remove (spw_rx_data_irq (u->port), spw_dma_rx_data_ih, u);
		rtems_interrupt_handler_remove (spw_tx_data_irq (u->port), spw_dma_tx_data_ih, u);
        rtems_interrupt_handler_remove (spw_rx_desc_irq (u->port), spw_connected_ih, u);
		return ret;
	}
    
    bsp_disable_irq(spw_rx_desc_irq(u->port)) ;
    bsp_disable_irq(spw_tx_data_irq(u->port)) ;
    
    // Disable its or we will get unhandled exception.
    bsp_disable_irq(spw_tx_desc_irq(u->port)) ;
    bsp_disable_irq(spw_err_irq(u->port)) ;
    //bsp_disable_irq(spw_time_irq(u->port)) ;
	
	DTRACEK ("MASKR2 = %08X, QSTR2 = %08X\n", MC_MASKR2, MC_QSTR2);
	spw_start (u);

	DTRACEK("Done\n") ;
    
	return RTEMS_SUCCESSFUL;
}

rtems_device_driver mcspw_close(
	rtems_device_major_number major,
	rtems_device_minor_number minor,
	void *arg
)
{
    DTRACEK("Invoked, major=0x%x, minor=0x%x\n", major, minor) ;
	// Initialize device-common data structures here
	return RTEMS_SUCCESSFUL;
}

rtems_device_driver mcspw_read(
	rtems_device_major_number major,
	rtems_device_minor_number minor,
	void *arg
)
{
    DTRACEK("Invoked, major=0x%x, minor=0x%x\n", major, minor) ;
	
	spw_t *u = &spw_channel [minor];
	unsigned sz_to_end, nbytes, rest, completed;
	char *pdata;
	spw_descriptor_t *pdesc = (spw_descriptor_t *) u->rx_desc_buf.half[u->rx_desc_buf.reader_idx].reader_p;
    
    rtems_libio_rw_args_t *rw_args = (rtems_libio_rw_args_t*)arg ;
    size_t size = rw_args->count ;
    char *buf = rw_args->buffer ;
    
    DTRACEK ("=== channel = %d, pdesc=0x%x\n", u->port, pdesc);
	
	if (! spw_connected (u)) {
		DTRACEK ("spw_read: channel %d not connected, wait for connection\n", u->port);
		//MC_SWIC_MODE_CR(u->port) |= MC_SWIC_LINK_MASK;
		spw_start (u);
		if (u->nonblock) {
			return 0;
		}
		
		// if (wait_event_interruptible (u->stwq, u->connected)) {
		// FIXME: Enbale connected_ih IRQ may be?
		if (rtems_semaphore_obtain(u->stws, RTEMS_WAIT, RTEMS_NO_TIMEOUT)) {
			return RTEMS_UNSATISFIED ;
		}
		
		DTRACEK ("spw_read: channel %d, waiting for connection finished\n", u->port);
	}
	
	while (1) {
		if (!pdesc->valid) {
			// Нет принятых дескрипторов
			DTRACEK ("=== spw_read, channel = %d, no data\n", u->port);
		
			// Запускаем RX DMA, если он ещё не запущен.
			start_rx_dma_if_needed (&u->rx_desc_buf);
			bsp_disable_irq (spw_rx_data_irq (u->port));
			start_rx_dma_if_needed (&u->rx_data_buf);
			bsp_enable_irq (spw_rx_data_irq (u->port));
		
			// Если неблокирующий режим ввода-вывода и данных нет,
			// то сразу выходим с нулевым результатом.
			if (u->nonblock) return 0;

			// Если блокирующий режим, то дожидаемся получения
			// пакета без ошибок
			while (!pdesc->valid) {
				DTRACEK ("=== spw_read, channel = %d, waiting for a packet\n", u->port);
				DTRACEK ("RX DESC DMA CSR = %08X, RX DATA DMA CSR = %08X\n", 
					SWIC_DMA_RUN (u->rx_desc_buf.dma_chan), SWIC_DMA_RUN (u->rx_data_buf.dma_chan));
				//mutex_wait (&u->rx_desc_lock);
				bsp_enable_irq (spw_rx_desc_irq (u->port));
                //if (wait_event_interruptible (u->rxdescwq, pdesc->valid)) {
				if (rtems_semaphore_obtain (u->rxdescws, RTEMS_WAIT, RTEMS_NO_TIMEOUT)) {
					bsp_disable_irq (spw_rx_desc_irq (u->port));
					return RTEMS_UNSATISFIED;
				}
				bsp_disable_irq (spw_rx_desc_irq (u->port));
				SWIC_DMA_CSR (u->rx_desc_buf.dma_chan);	// Сброс признака прерывания
				DTRACEK ("=== spw_read, channel = %d, waiting done\n", u->port);
			}
		}

		pdesc->valid = 0;
		
        DTRACEK("pdesc->type = %i\n", pdesc->type) ;
		if (pdesc->type == EOP) {
			break;
		} else {      // EEP
			// Получен дескриптор с ошибкой, ждём следующего дескриптора
			u->rx_eep++;
			move_reader_p (&u->rx_desc_buf, sizeof (spw_descriptor_t));
			pdesc = (spw_descriptor_t *) u->rx_desc_buf.half[u->rx_desc_buf.reader_idx].reader_p;
            DTRACEK("Got new pdesc=0x%x\n", pdesc) ;
			
			bsp_disable_irq (spw_rx_data_irq (u->port));
			move_reader_p (&u->rx_data_buf, pdesc->size);
			bsp_enable_irq (spw_rx_data_irq (u->port));
		}
	}
	
	DTRACEK ("=== spw_read, processing packet\n");
	
	completed = 0;
	rest = pdesc->size;
	if (rest > size) {
		// Ошибка: пользователь предоставил маленький буфер
		bsp_disable_irq (spw_rx_data_irq (u->port));
		move_reader_p (&u->rx_data_buf, rest);
		bsp_enable_irq (spw_rx_data_irq (u->port));
		
		move_reader_p (&u->rx_desc_buf, sizeof (spw_descriptor_t));
		return RTEMS_INVALID_SIZE;
	}
	
	bsp_disable_irq (spw_rx_data_irq (u->port));
	
	while (rest > 0) {
		pdata = u->rx_data_buf.half[u->rx_data_buf.reader_idx].reader_p;
		DTRACEK ("pdata = %08X\n", pdata);
		do nbytes = avail_size (&u->rx_data_buf);
		while (nbytes == 0);
		DTRACEK ("avail_size = %d, rest = %d\n", nbytes, rest);
		sz_to_end = size_to_end_of_cur_buffer (&u->rx_data_buf);
		
		bsp_enable_irq (spw_rx_data_irq (u->port));
		
		if (nbytes > sz_to_end) nbytes = sz_to_end;
		if (nbytes > rest) nbytes = rest;
		memcpy (buf, pdata, nbytes);
		rest -= nbytes;
		completed += nbytes;
	
		bsp_disable_irq (spw_rx_data_irq (u->port));
		move_reader_p (&u->rx_data_buf, nbytes);
	}
	bsp_enable_irq (spw_rx_data_irq (u->port));

	move_reader_p (&u->rx_desc_buf, sizeof (spw_descriptor_t));
	
	u->rx_eop++;
	u->rx_bytes += completed;

	DTRACEK ("=== spw_read exit, channel = %d, completed = %d\n", u->port, completed);
	rw_args->bytes_moved = completed;
    
	return RTEMS_SUCCESSFUL;
}

rtems_device_driver mcspw_write(
	rtems_device_major_number major,
	rtems_device_minor_number minor,
	void *arg
)
{
    DTRACEK("Invoked, major=0x%x, minor=0x%x\n", major, minor) ;
	
	spw_t *u = &spw_channel [minor];
	unsigned nbytes, completed = 0;
    rtems_libio_rw_args_t *rw_args = (rtems_libio_rw_args_t*)arg ;
    size_t size = rw_args->count ;
    char *buf = rw_args->buffer ;
	
	DTRACEK ("=== channel = %d, size = %d\n", u->port, size);

	if (! spw_connected (u)) {
		DTRACEK ("spw_write: channel %d not connected, wait for connection\n", u->port);
		//MC_SWIC_MODE_CR(u->port) |= MC_SWIC_LINK_MASK;
		spw_start (u);
		if (u->nonblock) {
			return 0;
		}
		
		// if (wait_event_interruptible (u->stwq, u->connected)) {
		if (rtems_semaphore_obtain (u->stws, RTEMS_WAIT, RTEMS_NO_TIMEOUT)) {
			return RTEMS_UNSATISFIED ;
		}
		
		DTRACEK ("spw_write: channel %d, waiting for connection finished\n", u->port);
	}
	
	DTRACEK ("MASKR2 = %08X, prev tx_desc = %08X\n", MC_MASKR2, *((unsigned *) u->tx_desc));
	
	// Если занят канал TX DMA, ...
	while (MC_SWIC_TX_DATA_RUN (u->port) & 1) {

		// При неблокирующем вводе-выводе, если канал TX DMA занят, сразу выходим 
		if (u->nonblock) {
			return 0;
		}

		// При блокирующем вводе-выводе ждем сигнала о завершении
		// текущей передачи TX DMA. Сигнал приходит от обработчика 
		// прерывания по окончанию передачи TX DMA.
		DTRACEK ("Waiting for TX DATA DMA free, channel %d\n", u->port);
		DTRACEK ("RX DATA CSR = %08X\n", MC_SWIC_RX_DATA_RUN (!u->port));
		
		bsp_enable_irq (spw_tx_data_irq (u->port));
		// if (wait_event_interruptible (u->txwq, !(MC_SWIC_TX_DATA_RUN (u->port) & 1))) {
        if (rtems_semaphore_obtain (u->txws, RTEMS_WAIT, RTEMS_NO_TIMEOUT)) {
			bsp_disable_irq (spw_tx_data_irq (u->port));
			return RTEMS_UNSATISFIED ;
		}
		bsp_disable_irq (spw_tx_data_irq (u->port));
		
		DTRACEK ("Waiting for TX DMA done\n");
		u->txdma_waits++;
	}

	// Настраиваем DMA на выдачу пакета
	u->tx_desc->size = size;
	u->tx_desc->type = EOP;
	u->tx_desc->valid = 1;
	u->tx_desc->unused = 0;
	MC_SWIC_TX_DESC_IR(u->port) = mips_virt_to_phys ((unsigned) u->tx_desc); // Адрес начала буфера
	MC_SWIC_TX_DESC_CSR(u->port) = MC_DMA_CSR_WCX(0) | MC_DMA_RUN; // 1 8-байтовое слово
	
	while (size > 0) {
		nbytes = size;
		if (nbytes > SPW_TX_DATA_BUFSZ) nbytes = SPW_TX_DATA_BUFSZ;
		
		memcpy (u->tx_data_buffer, buf + completed, nbytes);	// Приходится копировать из-за проблем в DMA
		
		MC_SWIC_TX_DATA_IR(u->port) = mips_virt_to_phys ((unsigned) u->tx_data_buffer);
		DTRACEK ("tp1, TX_DATA_CSR = %08X, TX_DESC_CSR = %08X, channel %d\n", 
			MC_SWIC_TX_DATA_RUN (u->port), MC_SWIC_TX_DESC_RUN (u->port), u->port);
		DTRACEK ("transmit: TX_DATA_IR = %08X, TX_DESC_IR = %08X, channel %d\n", 
			MC_SWIC_TX_DATA_IR (u->port), MC_SWIC_TX_DESC_IR (u->port), u->port);		
		MC_SWIC_TX_DATA_CSR(u->port) = MC_DMA_CSR_IM | MC_DMA_CSR_WCX(((nbytes + 7) >> 3) - 1)  | MC_DMA_RUN;
		
		DTRACEK ("tp2, TX_DATA_CSR = %08X, TX_DESC_CSR = %08X, channel %d\n", 
			MC_SWIC_TX_DATA_RUN (u->port), MC_SWIC_TX_DESC_RUN (u->port), u->port);

		completed += nbytes;
		size -= nbytes;
		if (size > 0) {
			bsp_enable_irq (spw_tx_data_irq (u->port));
			//if (wait_event_interruptible (u->txwq, !(MC_SWIC_TX_DATA_RUN (u->port) & 1))) {
            if (rtems_semaphore_obtain (u->txws, RTEMS_WAIT, RTEMS_NO_TIMEOUT)) {
				bsp_disable_irq (spw_tx_data_irq (u->port));
				return RTEMS_UNSATISFIED ;
			}
			bsp_disable_irq (spw_tx_data_irq (u->port));
		}
	}
	
	u->tx_packets++;
	u->tx_bytes += completed;

	DTRACEK ("=== spw_write exit, channel = %d, completed = %d\n", u->port, completed);
	rw_args->bytes_moved = completed;
    
	return RTEMS_SUCCESSFUL;
}

rtems_device_driver mcspw_control(
	rtems_device_major_number major,
	rtems_device_minor_number minor,
	void *arg
)
{
    DTRACEK("Invoked, major=0x%x, minor=0x%x\n", major, minor) ;
	rtems_libio_ioctl_args_t *ioarg = (rtems_libio_ioctl_args_t *) arg;
    unsigned int *data = ioarg->buffer;
    unsigned int cmd = ioarg->command ;
    
    unsigned int chan_num = minor ;
	spw_t *u = &spw_channel [chan_num];
	//unsigned freq_mult_mhz;

	switch (cmd) {
	// Получение значения частоты приема
	case SPW_GET_RX_SPEED:
		//freq_mult_mhz = MC_FREQUENCY_MULTIPLIER * MC_QUARTZ_CLOCK_FREQ;
		//ioarg->ioctl_return = ((MC_SWIC_RX_SPEED (u->port) * freq_mult_mhz / 1000000) >> 7);
        ioarg->ioctl_return = (MC_SWIC_RX_SPEED (u->port) * CPU_CLOCK_RATE_MHZ * 8 / 1024);
        break ;

	// Получение значения частоты передачи
	case SPW_GET_TX_SPEED:
		ioarg->ioctl_return = (MC_SWIC_TX_SPEED (u->port) & MC_SWIC_TX_SPEED_PRM_MASK) * 5;
        break ;

	// Установка значения частоты передачи
	case SPW_SET_TX_SPEED:
        DTRACEK("SPW_SET_SPEED request, data[0]=%i\n", data[0]) ;
		if (data[0] < 5 || data[0] > 400) return RTEMS_UNSATISFIED;
		spw_set_tx_speed (u, data[0]);
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
    
    case SPW_IS_CONNECTED:
        DTRACEK("SPW_IS_CONNECTED request, spw_connected() return 0x%X\n", spw_connected(u)) ;
        ioarg->ioctl_return = spw_connected(u) ;
        break ;
        
    case SPW_RX_IS_EMPTY:
        ioarg->ioctl_return = u->rx_data_buf.half[u->rx_data_buf.reader_idx].empty ;
        break ;
    
    case SPW_WAIT_TIME_CODE:
        if (rtems_semaphore_obtain(u->time, RTEMS_WAIT, RTEMS_NO_TIMEOUT)) {
			return RTEMS_UNSATISFIED ;
		}
        
    case SPW_GET_TIME_CODE:
        ioarg->ioctl_return = MC_SWIC_RX_CODE(u->port) & MC_SWIC_TIME_CODE ;
        break ;
        
	default:
		return RTEMS_NOT_IMPLEMENTED ;
	}
    
	return RTEMS_SUCCESSFUL;
}

//
// Обработчик прерывания по завершению приёма данных
//
static void spw_dma_rx_data_ih (void *dev_id)
{
	spw_t *u = dev_id;
	
	// Снимаем признак прерывания
	MC_SWIC_RX_DATA_CSR (u->port);
	
    // Temporarily disabled
	start_rx_dma_if_needed (&u->rx_data_buf);
	
	rtems_semaphore_release( u->rxdataws );
}

//
// Обработчик прерывания по приёму дескриптора
//
static void spw_dma_rx_desc_ih (void *dev_id)
{
	spw_t *u = dev_id;
#ifdef DEBUG
	spw_descriptor_t *pdesc = (spw_descriptor_t *) u->rx_desc_buf.half[u->rx_desc_buf.reader_idx].reader_p;
#endif
	
#ifdef BSP_ENABLE_CPU_CACHE
    cpu_cache_invalidate_data() ;
#endif
    
    DTRACEK("Descriptor has been received, pdesc=0x%x, type=0x%x\n", pdesc,  pdesc->type) ;
    
	// Снимаем признак прерывания
	MC_SWIC_RX_DESC_CSR (u->port);
    
	rtems_semaphore_release (u->rxdescws);
}

//
// Обработчик прерывания по завершению выдачи данных
//
static void spw_dma_tx_data_ih (void *dev_id)
{
	spw_t *u = dev_id;
	
    DTRACEK("Data transmitted, channel=%i\n", u->port) ;
    
	// Снимаем признак прерывания
	MC_SWIC_TX_DATA_CSR (u->port);
		
	rtems_semaphore_release (u->txws);
}

//
// Обработчик прерывания по установке соединения
//
static void spw_connected_ih (void *dev_id)
{
	spw_t *u = dev_id;
	
	DTRACEK ("Connected, channel: %d, STATUS = %08X\n", u->port, MC_SWIC_STATUS (u->port));
	
	if (MC_SWIC_STATUS (u->port) & 0xE) {
		MC_SWIC_STATUS (u->port) |= 0xF;
		DTRACEK ("Error detected, waiting reconnection\n");
		return ;
	}
    
    MC_SWIC_STATUS (u->port) |= MC_SWIC_CONNECTED;      // clear IRQ
    
	
    /* Fix: DMA appear to be run when program is restarted, but actually it is not.
    * So, start dma in any case.
    */
    //if ( !(SWIC_DMA_RUN (u->rx_desc_buf.dma_chan))) start_dma (&u->rx_desc_buf);
    //if ( !(SWIC_DMA_RUN (u->rx_data_buf.dma_chan))) start_dma (&u->rx_data_buf);
    start_dma (&u->rx_desc_buf);
    start_dma (&u->rx_data_buf);
	
    spw_set_tx_speed (u, u->speed);
	
    rtems_semaphore_release (u->stws);
    
    // Закрываем прерывание, иначе оно будет возникать по приему каждого пакета
    MC_SWIC_MODE_CR(u->port) &= ~MC_SWIC_LINK_MASK;
	
    u->connected = 1 ;
}

// Handle time codes
static void spw_time_ih(void *dev_id) {
    spw_t *u = dev_id ;
    
    DTRACEK("SWIC STATUS=0x%x\n", MC_SWIC_STATUS (u->port) ) ;
    DTRACEK ("Time code event: RX_CODE%i=0x%x\n", u->port, MC_SWIC_RX_CODE(u->port));
    
    MC_SWIC_STATUS (u->port) |= MC_SWIC_GOT_TIME ;      // clear IRQ
    MC_SWIC_STATUS (u->port) |= MC_SWIC_IRQ_TIM ;
    
    rtems_semaphore_release( u->time );
}

static void spw_start (spw_t *u)
{
	u->connected = 0;
	// Включение частоты
	MC_CLKEN |= MC_CLKEN_SWIC(u->port);
	
	// Сброс контроллера
	MC_SWIC_MODE_CR(u->port) = SWIC_RESET;
	
	// Запись тайминга для того, чтобы SWIC отрабатывал временные интер-
	// валы в соответствии со стандартом SpaceWire
	//MC_SWIC_MODE_CR(u->port) = MC_SWIC_TIMING_WR_EN;
	//MC_SWIC_TX_SPEED(u->port) = MC_SWIC_TIMING(KHZ/10000);
	
	MC_SWIC_MODE_CR(u->port) = SWIC_START;

	// Начальные установки и пуск
	MC_SWIC_TX_SPEED(u->port) = MC_SWIC_TX_SPEED_PRM(CONFIG_MULTICORE_SWIC_START_SPEED / 5) | MC_SWIC_PLL_TX_EN | MC_SWIC_LVDS_EN;
	//MC_SWIC_TX_SPEED(u->port) = MC_SWIC_TX_SPEED_CON(SPW_START_SPEED / 5) | 
	//	MC_SWIC_TX_SPEED_PRM(u->speed / 5) | MC_SWIC_PLL_TX_EN | MC_SWIC_LVDS_EN;

	//mdelay (20);
	DTRACEK ("TX_SPEED(%d) = %08X\n", u->port, MC_SWIC_TX_SPEED(u->port));

	MC_SWIC_MODE_CR(u->port) = MC_SWIC_LinkStart | MC_SWIC_AutoStart | 
		MC_SWIC_WORK_TYPE | MC_SWIC_LINK_MASK | MC_SWIC_TCODE_MASK | MC_SWIC_TIM_MASK ;
		
	//MC_SWIC_MODE_CR(u->port) = MC_SWIC_LinkStart | MC_SWIC_AutoStart | 
	//	MC_SWIC_WORK_TYPE | MC_SWIC_LINK_MASK | MC_SWIC_AUTO_TX_SPEED;
        
	// Сброс всех признаков прерываний
	MC_SWIC_STATUS(u->port) = SWIC_RESET_ALL_IRQS;

	// Сброс счетчиков принятых пакетов
	MC_SWIC_CNT_RX_PACK(u->port) = 0;
	MC_SWIC_CNT_RX0_PACK(u->port) = 0;
}

//
// Запуск канала DMA для указанного двойного буфера
//
static inline void start_dma (double_buffer *pdbuf)
{
	one_buffer *pcur;
	
	DTRACEK ("start_dma 0x%04X, dma_idx = %d\n", pdbuf->dma_chan, pdbuf->dma_idx);
	
	pcur = &pdbuf->half[pdbuf->dma_idx];
	pcur->empty = 0;
	DTRACEK ("pcur->chain_addr = %08X\n", pcur->chain_addr);
	if (pcur->chain_addr) {
		DTRACEK ("First CSR in chain: %08X\n", ((dma_params_t *) pcur->chain_addr)->csr);
		SWIC_DMA_CP (pdbuf->dma_chan) = mips_virt_to_phys (pcur->chain_addr) | 1;
	} else {
		DTRACEK ("pcur->buf = %08X\n", pcur->buf);
		SWIC_DMA_IR (pdbuf->dma_chan) = mips_virt_to_phys ((unsigned) pcur->buf);
		SWIC_DMA_CSR (pdbuf->dma_chan) = MC_DMA_CSR_WN(0) | MC_DMA_CSR_IM |
			MC_DMA_CSR_WCX((pdbuf->size >> 3) - 1) | MC_DMA_CSR_RUN;
	}
}

//
// Запуск DMA, если соблюдены необходимые условия
//
static void start_rx_dma_if_needed (double_buffer *pdbuf)
{
	DTRACEK ("start_rx_dma_if_needed, SWIC_DMA_RUN(0x%04X) = %08X, second empty: %d, dma idx: %d, rd idx: %d\n", 
		pdbuf->dma_chan, SWIC_DMA_RUN (pdbuf->dma_chan), pdbuf->half[!pdbuf->dma_idx].empty,
		pdbuf->dma_idx, pdbuf->reader_idx);
	if ( !(SWIC_DMA_RUN (pdbuf->dma_chan) & 1) 	// Если канал DMA сейчас остановлен
	     && pdbuf->half[!pdbuf->dma_idx].empty)	// и соседний буфер пуст, то
	{
		SWIC_DMA_CSR (pdbuf->dma_chan);
		// Переключаем DMA на приём в соседний буфер
		pdbuf->dma_idx = !pdbuf->dma_idx;
		start_dma (pdbuf);
	}
}

void spw_set_tx_speed (spw_t *u, unsigned mbit_per_sec)
{
	u->speed = mbit_per_sec;
	MC_SWIC_TX_SPEED (u->port) = (MC_SWIC_TX_SPEED (u->port) & ~MC_SWIC_TX_SPEED_PRM_MASK) |
		MC_SWIC_TX_SPEED_PRM (mbit_per_sec / 5);
}

static inline int spw_connected (spw_t *u)
{
	return ((MC_SWIC_STATUS (u->port) & 0x30E0) == 0x30A0);
}

//
// Продвижение указателя читателя в двойном буфере
//
static void move_reader_p (double_buffer *pdbuf, int distance)
{
	one_buffer *pcur = &pdbuf->half[pdbuf->reader_idx];
	int size_to_end = size_to_end_of_cur_buffer (pdbuf);
	int aligned_distance = (distance + 7) & ~7;
	
	DTRACEK ("move_reader_p, distance = %d, size_to_end = %d\n", 
		aligned_distance, size_to_end);
	
	if (aligned_distance < size_to_end)
		pcur->reader_p += aligned_distance;
	else {
		pcur->empty = 1;
		pdbuf->reader_idx = !pdbuf->reader_idx;
		pcur = &pdbuf->half[pdbuf->reader_idx];
		pcur->reader_p = pcur->buf + (aligned_distance - size_to_end);
		if (pcur->chain_addr) {
			start_rx_dma_if_needed (pdbuf);
		}
	}
}

//
// Расчет доступной для чтения информации в указанном буфере @pbuf.
//
static inline unsigned avail_size (double_buffer *pdbuf)
{
	one_buffer *pcur = &pdbuf->half[pdbuf->reader_idx];
	
	int sz = SWIC_DMA_IR (pdbuf->dma_chan) - mips_virt_to_phys ((unsigned) pcur->reader_p);
	DTRACEK ("avail_size: sz = %d, DMA CSR = %08X, IR = %08X, addr = %08X\n", sz, SWIC_DMA_CSR (pdbuf->dma_chan),
		SWIC_DMA_IR (pdbuf->dma_chan), mips_virt_to_phys ((unsigned) pcur->reader_p));
	if (sz >= 0) return sz;
	else return (sz + SPW_RX_DATA_BUFSZ);
}

static inline unsigned size_to_end_of_cur_buffer (double_buffer *pdbuf)
{
	one_buffer *pcur = &pdbuf->half[pdbuf->reader_idx];
	return (pdbuf->size - (pcur->reader_p - pcur->buf)); 
}
