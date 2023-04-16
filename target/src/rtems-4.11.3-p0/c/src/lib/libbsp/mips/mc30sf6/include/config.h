#ifndef BSP_MULTICORE_CONFIG_H
#define BSP_MULTICORE_CONFIG_H

#define     CONFIG_MC30SF6

/* 
 * Taken from mcLinux mc30sf6 .config
*/
#define CONFIG_MULTICORE_CLOCKFREQ_MHZ          10      /* G1 quartz, XTI input.    */

/*
*   Taken from mcLinux mct03p .config
*/
#define CONFIG_MULTICORE_SWIC_DEF_WORK_SPEED    200
#define CONFIG_MULTICORE_SWIC_START_SPEED       10
#define CONFIG_MULTICORE_SWIC_QTY				1

#define CONFIG_MULTICORE_MFBSP_QTY				4
#define CONFIG_MULTICORE_LPORT_RATE_MHZ         20       // PLC needs 20MHz.

#if 1
    #define CONFIG_ENABLE_CPU_CACHE
#endif

#endif
