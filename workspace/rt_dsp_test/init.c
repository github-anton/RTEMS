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
#include "libdsp.h"

/* forward declarations to avoid warnings */
extern "C" {
rtems_task Init(rtems_task_argument argument);
 // Адрес начала программы DSP-ядра
extern int Start_DSP;
// входные параметры - числа, которые DSP перемножает и
// параллельно находит максимальное из них
extern int InA;
extern int InB;
// результат
extern int OutC;
extern int OutD;
};

const char rtems_test_name[] = "DSP Test";

rtems_task Init(
  rtems_task_argument ignored
)
{
    rtems_test_begin();
    
    int OutputC;
    int OutputD;

    int InputA=5;
    int InputB=2;
    
    dsp_regs *pDSP0 = (dsp_regs*)(0x18480000 + 0xa000000) ;
    common_regs *pDSP0_CSR = (common_regs*)(0x18481000 + 0xa0000000) ;

// обнуляем управляющие регистры ядра DSP0
// Регистр управления и состояния DCSR
    pDSP0->DCSR  = 0;
// Регистр состояния SR
    pDSP0->SR     = 0;

// в программный счетчик ядра DSP0 кладем адрес начала программы DSP
// адрес для DSP0 получаем из адреса программы DSP в адресном пространстве
// CPU-ядра, вычитая адрес начала PRAM0 - памяти программ ядра DSP0.
// Аналогично получаем адреса переменных - только вычитаем адрес
// памяти данных DSP0.
// сдвиг вправо на два байта == деление на 4 - так как у DSP-ядер
// словная адресация, а в CPU-ядре - байтовая.
    pDSP0->PC=((unsigned int)&Start_DSP - 0xb8440000)>>2;

    pDSP0->A0 =((unsigned int)&InA - 0xb8400000)>>2;
    pDSP0->A1 =((unsigned int)&InB - 0xb8400000)>>2;
    pDSP0->A2 =((unsigned int)&OutC- 0xb8400000)>>2;
    pDSP0->A3 =((unsigned int)&OutD- 0xb8400000)>>2;

// записываем значения входных переменных в память DSP-ядра
    InA=InputA;
    InB=InputB;

// Запускаем ядро DSP0 на исполнение записью бита DCSR0[14]
    pDSP0->DCSR = 0x4000;
// Когда ядро DSP0 закончит программу - оно выполнит инструкцию STOP,
// заботливо оставленную нами в программе для DSP-ядра.
// После выполнения данной инструкции DSP-ядро останавливается и
// выставляет бит QSTR_DSP[3]. Если при этом в единице будет бит
// MASKR_DSP[3], и будут разрешены прерывания от внутренних устройств
// процессора - будет прерывание. Но поскольку это
// базовый простой пример - сделали просто опрос.
    while( !(pDSP0_CSR->QSTR & (1<<3)) ) ;

// забираем результат из памяти DSP-ядра в память CPU-ядра
    OutputC=OutC;
    OutputD=OutD;
    
    printf("OutputC=%d, OutputD=%d\n", OutputC, OutputD) ;
    
    rtems_test_end();
    exit( 0 );
}


#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_APPLICATION_EXTRA_DRIVERS MCSPW_DRIVER_TABLE_ENTRY

#define CONFIGURE_MAXIMUM_TASKS            3
#define CONFIGURE_USE_DEVFS_AS_BASE_FILESYSTEM

#define CONFIGURE_RTEMS_INIT_TASKS_TABLE
// If CPU cahce is disabled CONFIGURE_MICROSECONDS_PER_TICK should be not less than 1000
#define CONFIGURE_MICROSECONDS_PER_TICK 1000

#define CONFIGURE_INITIAL_EXTENSIONS RTEMS_TEST_INITIAL_EXTENSION

#define CONFIGURE_UNIFIED_WORK_AREAS
#define CONFIGURE_UNLIMITED_OBJECTS             // operators new and delete really need it!
#define CONFIGURE_FILESYSTEM_DEVFS
#define CONFIGURE_LIBIO_MAXIMUM_FILE_DESCRIPTORS 8

#define CONFIGURE_INIT
#include <rtems/confdefs.h>
