#ifndef FLASHSPM_H
#define FLASHSPM_H

#include <avr/pgmspace.h>

// 44 kb of storage size
#define BOOTSPM_SECTION __attribute__ ((section (".bootspm")))
#define BOOTSPM_START_ADDR    0x14000
#define BOOTSPM_SIZE          0x0B000


BOOTSPM_SECTION uint8_t SPMWritePage(uint32_t addr, const uint8_t* data, uint16_t size);
void SPMRead(uint32_t addr, uint8_t* data, uint16_t size);


#endif // FLASHSPM_H
