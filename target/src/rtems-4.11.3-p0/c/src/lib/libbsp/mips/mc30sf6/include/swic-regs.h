/*
 * MC-30SF6 board Input/Output macros
 * 
 * Author: Elvis,
 * Modified: 2019, Anton Ermakov
 */

#ifndef MCSPW_IO_H_
#define MCSPW_IO_H_

#define EOP     1
#define EEP     2

#define SWIC_RESET          	0x00000079
#define SWIC_START          	0x00000101
#define SWIC_RESET_ALL_IRQS 	0x0001d00f

// Макросы для вычисления номеров прерываний
#define IRQ_SHIFT	3
#define swic_err_irq(port_id)		(MC1892_IRQ_SW0_ERR + ((port_id) << IRQ_SHIFT))
#define swic_time_irq(port_id)		(MC1892_IRQ_SW0_TIME + ((port_id) << IRQ_SHIFT))
#define swic_link_irq(port_id)		(MC1892_IRQ_SW0_LINK + ((port_id) << IRQ_SHIFT))
#define swic_rx_data_irq(port_id)	(MC1892_IRQ_SW0_RX_DATA + ((port_id) << IRQ_SHIFT))
#define swic_rx_desc_irq(port_id)	(MC1892_IRQ_SW0_RX_DESC + ((port_id) << IRQ_SHIFT))
#define swic_tx_data_irq(port_id)	(MC1892_IRQ_SW0_TX_DATA + ((port_id) << IRQ_SHIFT))
#define swic_tx_desc_irq(port_id)	(MC1892_IRQ_SW0_TX_DESC + ((port_id) << IRQ_SHIFT))

// Доступ к регистрам любого канала SWIC DMA по его адресу
#define SWIC_DMA_CSR(addr)	MC_R ((addr) + 0x0)	// Регистр управления и состояния канала
#define SWIC_DMA_CP(addr)	MC_R ((addr) + 0x4)	// Регистр указателя цепочки канала
#define SWIC_DMA_IR(addr)	MC_R ((addr) + 0x8)	// Индексный регистр внешней памяти канала
#define SWIC_DMA_RUN(addr)	MC_R ((addr) + 0xC)	// Псевдорегистр управления состоянием бита RUN
                                                // According to documentation RUN - writing
                                                // END, DONE - reading

// Адреса каналов DMA
#define SWIC_RX_DESC_CHAN(port)	(0xA800)
#define SWIC_RX_DATA_CHAN(port)	(0xA840)
#define SWIC_TX_DESC_CHAN(port)	(0xA880)
#define SWIC_TX_DATA_CHAN(port)	(0xA8C0)

/*
 * Регистры канала DMA записи в память дескрипторов принимаемых пакетов SWIC
 */
#define MC_SWIC_RX_DESC_CSR(n)	MC_R (0xA800)	/* Регистр управления и состояния канала */
#define MC_SWIC_RX_DESC_CP(n)	MC_R (0xA804)	/* Регистр указателя цепочки канала */
#define MC_SWIC_RX_DESC_IR(n)	MC_R (0xA808)	/* Индексный регистр внешней памяти канала */
#define MC_SWIC_RX_DESC_RUN(n)	MC_R (0xA80C)	/* Псевдорегистр управления состоянием бита RUN */

/*
 * Регистры канала DMA записи в память принимаемых слов данных SWIC
 */
#define MC_SWIC_RX_DATA_CSR(n)	MC_R (0xA840)	/* Регистр управления и состояния канала */
#define MC_SWIC_RX_DATA_CP(n)	MC_R (0xA844)	/* Регистр указателя цепочки канала */
#define MC_SWIC_RX_DATA_IR(n)	MC_R (0xA848)	/* Индексный регистр внешней памяти канала */
#define MC_SWIC_RX_DATA_RUN(n)	MC_R (0xA84C)	/* Псевдорегистр управления состоянием бита RUN */

/*
 * Регистры канала DMA чтения из памяти дескрипторов передаваемых пакетов SWIC
 */
#define MC_SWIC_TX_DESC_CSR(n)	MC_R (0xA880)	/* Регистр управления и состояния канала */
#define MC_SWIC_TX_DESC_CP(n)	MC_R (0xA884)	/* Регистр указателя цепочки канала */
#define MC_SWIC_TX_DESC_IR(n)	MC_R (0xA888)	/* Индексный регистр внешней памяти канала */
#define MC_SWIC_TX_DESC_RUN(n)	MC_R (0xA88C)	/* Псевдорегистр управления состоянием бита RUN */

/*
 * Регистры канала DMA чтения из памяти передаваемых слов данных SWIC
 */
#define MC_SWIC_TX_DATA_CSR(n)	MC_R (0xA8C0)	/* Регистр управления и состояния канала */
#define MC_SWIC_TX_DATA_CP(n)	MC_R (0xA8C4)	/* Регистр указателя цепочки канала */
#define MC_SWIC_TX_DATA_IR(n)	MC_R (0xA8C8)	/* Индексный регистр внешней памяти канала */
#define MC_SWIC_TX_DATA_RUN(n)	MC_R (0xA8CC)	/* Псевдорегистр управления состоянием бита RUN */

/*
 * Регистры SpaceWire. There is 1x SpaceWire unit + 2x SpaceWire ports(trencievers).
 */
#define MC_SWIC_HW_VER(n)	MC_R (0xA000)	/* Регистр аппаратной версии контроллера */
#define MC_SWIC_STATUS(n)	MC_R (0xA08C + ((n) << 2))	/* Регистр состояния [6]*/
#define MC_SWIC_RX_CODE(n)	MC_R (0xA03C)	/* FIXME Регистр принятого управляющего символа */
#define MC_SPW_MODE_CR(n)	MC_R (0xA0A4 + ((n) << 2))	/* Регистр управления режимом работы [6] */
#define MC_GSPW_MODE_CR(n)	MC_R (0xA0AC + ((n) << 2))	/* Регистр управления режимом работы [4] */
#define MC_SWIC_TX_SPEED(n)	MC_R (0xA0BC + ((n) << 2))	/* Регистр управления скоростью передачи [2] */
#define MC_GSPW_TX_SPEED(n) MC_R (0xA0C4 + ((n) << 2))  /* GigaSpaceWire TX speed [4] */
#define MC_SWIC_TX_CODE(n)	MC_R (0xA038)	/* Регистр передаваемого управляющего символа */
#define MC_SWIC_RX_SPEED(n)	MC_R (0xA0D4 + ((n) << 2))	/* Регистр измерителя скорости приема  [2] */
#define MC_SWIC_CNT_RX_PACK(n)	MC_R (0xC01C + ((n) << 12))	/* Регистр счетчика принятых пакетов ненулевой длины */
#define MC_SWIC_CNT_RX0_PACK(n)	MC_R (0xC020 + ((n) << 12))	/* Регистр счетчика принятых пакетов нулевой длины */
#define MC_SWIC_ISR_L(n)	MC_R (0xA040)	/* Регистр кодов распределенных прерываний (младшая часть) */
#define MC_SWIC_ISR_H(n)	MC_R (0xA044)	/* Регистр кодов распределенных прерываний (старшая часть) */
#define MC_GSPW_PMA_MODE(n) MC_R (0xA114 + ((n) << 2))  /* GigaSpaceWire PMA mode speed [4] */
#define MC_ROUTING_TABLE    MC_RA (0xA400)  /* SpaceWire routing table [16] */
#define MC_GSPW_ADG         MC_RA (0xA0EC)  /* GigaSpaceWire ADG[6] */
#define MC_MODE_R(n)        MC_R (0xA00C + ((n) << 2))  /* MODE_R[2] working mode of GigaSpwr[0-1] */
#define MC_STATE_R          MC_R (0xA014)   /* State of commutator's block. */

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
#define MC_SPW_LinkDisabled     0x00000001  /* Установка LinkDisabled для блока DS-кодирования */
#define MC_SPW_AutoStart        0x00000002  /* Установка AutoStart для блока DS-кодирования */
#define MC_SPW_LinkStart        0x00000004  /* Установка LinkStart для блока DS-кодирования */
#define MC_SPW_BdsReset         0x00000008
#define MC_SPW_CodecLoopback    0x00000010
#define MC_SPW_LvdsLoopback     0x00000020

/* MODE_R */
#define MC_GSPW_DMA_ENABLED     0x800000
#define MC_GSPW_DMA_CLR_FIFO    0x1000000   

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

/* Values of MC_STATE_R register */
#define MC_STATE_R_SPW_CONNECTED        0x7E0
#define MC_STATE_R_SPW_ERRORED          0x1F800
#define MC_STATE_R_PORT_CONNECTED       0x7E0000
#define MC_STATE_R_PORT_ERRORED         0x1F800000
//#define MC_STATE_R_SPW_ERRORED            0x7E0
//#define MC_STATE_R_SPW_CONNECTED          0x1F800
//#define MC_STATE_R_PORT_ERRORED           0x7E0000
//#define MC_STATE_R_PORT_CONNECTED         0x1F800000


#endif      // MCSPW_IO_H_
