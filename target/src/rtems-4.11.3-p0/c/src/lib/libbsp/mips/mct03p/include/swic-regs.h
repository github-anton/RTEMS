/*
 * Hardware register defines for Elvees NVCom-01 microcontroller.
 *
 * Copyright (C) 2010 Serge Vakulenko, <serge@vak.ru>
 *
 * This file is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You can redistribute this file and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software Foundation;
 * either version 2 of the License, or (at your discretion) any later version.
 * See the accompanying file "COPYING.txt" for more details.
 *
 * As a special exception to the GPL, permission is granted for additional
 * uses of the text contained in this file.  See the accompanying file
 * "COPY-UOS.txt" for details.
 * 
 * MCT-03P I/O routines
 * Modified 2019 by Anton Ermakov
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

#endif      // MCSPW_IO_H_
