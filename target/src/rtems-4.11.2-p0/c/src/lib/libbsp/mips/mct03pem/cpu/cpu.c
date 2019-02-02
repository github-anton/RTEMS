#include <bsp/cpu.h>
#include <bsp.h>

#define MC_CSR		MC_R (0x4008)	/* Управление и состояние */

/*
 * System register CSR flags
 */
#define MC_CSR_FM       0x00000001  /* Fixed mapping */
#define MC_CSR_TST_CACHE    0x00000800  /* random access to cache */
#define MC_CSR_FLUSH_I      0x00001000  /* instruction cache invalidate */
#define MC_CSR_FLUSH_D      0x00004000  /* data cache invalidate */

void cpu_cache_invalidate_data(void) {
    int i ;
    MC_CSR = MC_CSR | MC_CSR_FLUSH_D ;
    for (i = 0; i < 10; i++) {
        asm("nop") ;
    }
}
