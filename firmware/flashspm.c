#include <avr/boot.h>
#include "flashspm.h"


BOOTSPM_SECTION uint8_t SPMWritePage(uint32_t addr, const uint8_t* data, uint16_t size)
{
    if (size > SPM_PAGESIZE)
        return 1;
    if (addr >= BOOTSPM_SIZE)
        return 2;

    uint16_t bytes;
    addr += BOOTSPM_START_ADDR;
    for (bytes = 0; bytes < size; bytes += 2, data += 2) {
        boot_page_fill(addr + bytes, *((uint16_t*)data));
    }

    boot_page_erase(addr);
    boot_spm_busy_wait();

    boot_page_write(addr);
    boot_spm_busy_wait();

    boot_rww_enable();
    return 0;
}

void SPMRead(uint32_t addr, uint8_t* data, uint16_t size)
{
    addr += BOOTSPM_START_ADDR;
    uint16_t bytes;
    for (bytes = 0; bytes < size; bytes += 2, data += 2) {
        *((uint16_t*)data) = pgm_read_word_far(addr + bytes);
    }
}
