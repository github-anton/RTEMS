#ifndef LIBBSP_MIPS_MCT03PEM_MCSPW_H
#define LIBBSP_MIPS_MCT03PEM_MCSPW_H

#include <rtems/io.h>

#ifdef __cplusplus
extern "C" {
#endif

// Получение значения частоты приема
#define SPW_GET_RX_SPEED            1

// Получение значения частоты передачи
#define SPW_GET_TX_SPEED            2

// Установка значения частоты передачи
#define SPW_SET_TX_SPEED            3
	
// Получение количества успешно принятых пакетов (EOP)
#define SPW_GET_STAT_RX_EOP         4
	
// Получение количества принятых пакетов с ошибкой (EEP)
#define SPW_GET_STAT_RX_EEP         5

// Получение количества принятых байт
#define SPW_GET_STAT_RX_BYTES	    6
	
// Получение количества переданных пакетов
#define SPW_GET_STAT_TX_PACKETS     7

// Получение количества переданных байт
#define SPW_GET_STAT_TX_BYTES	    8

// Получение количества ожиданий освобождения DMA передачи
#define SPW_GET_STAT_TX_DMA_WAITS	9
		
// Сброс статистики
#define SPW_RESET_STAT		        12

// Get connection status
#define SPW_IS_CONNECTED            13

// Get DMA buffer status
#define SPW_RX_IS_EMPTY             14
    
#define SPW_GET_TIME_CODE           15
    
#define SPW_WAIT_TIME_CODE          16
    
rtems_device_driver mcspw_initialize(rtems_device_major_number major, rtems_device_minor_number minor, void *arg);
rtems_device_driver mcspw_open(rtems_device_major_number major, rtems_device_minor_number minor, void *arg);
rtems_device_driver mcspw_close(rtems_device_major_number major, rtems_device_minor_number minor, void *arg);
rtems_device_driver mcspw_read(rtems_device_major_number major, rtems_device_minor_number minor, void *arg);
rtems_device_driver mcspw_write(rtems_device_major_number major, rtems_device_minor_number minor, void *arg);
rtems_device_driver mcspw_control(rtems_device_major_number major, rtems_device_minor_number minor, void *arg);

#ifdef __cplusplus
}
#endif

#define MCSPW_DRIVER_TABLE_ENTRY { mcspw_initialize, mcspw_open, mcspw_close, mcspw_read, mcspw_write, mcspw_control }

#endif
