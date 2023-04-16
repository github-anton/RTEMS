#include <bsp/multicore.h>
#include <bsp/cpu.h>

#define MC_CSR		MC_R (0x4008)	/* Управление и состояние */

/*
 * System register CSR flags
 */
#define MC_CSR_FM           0x00000001  /* Fixed mapping */
#define MC_CSR_TST_CACHE    0x00000800  /* random access to cache */
#define MC_CSR_FLUSH_I      0x00001000  /* instruction cache invalidate */
#define MC_CSR_FLUSH_D      0x00004000  /* data cache invalidate */

void cpu_cache_invalidate(unsigned cache_flag) ;

void flush_i_cache(void) {
    cpu_cache_invalidate ( MC_CSR_FLUSH_I ) ;
}

void flush_d_cache(void) {
    cpu_cache_invalidate ( MC_CSR_FLUSH_D ) ;
}

void clear_cache(void) {
    flush_i_cache() ;
    flush_d_cache() ;
}

void cpu_cache_invalidate(unsigned cache_flag) {
    int i ;
    MC_CSR = MC_CSR | cache_flag ;
    for (i = 0; i < 10; i++) {
        asm("nop") ;
    }
}
