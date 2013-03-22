#ifndef FLASHSPM_H
#define FLASHSPM_H

#include <avr/pgmspace.h>

// 44 kb of storage size
#define BOOTSPM_START_ADDR    0x10000
#define BOOTSPM_SIZE          0x0B000

#define USE_DFU_API

#ifdef USE_DFU_API
#define SPMW_SECTION
#else
#define BOOTSPM_SECTION  __attribute__ ((section (".bootspm")))
#define SPMW_SECTION     BOOTSPM_SECTION
#endif


SPMW_SECTION uint8_t SPMWritePage(uint32_t addr, const uint8_t* data, uint16_t size);
void SPMRead(uint32_t addr, uint8_t* data, uint16_t size);


#endif // FLASHSPM_H
