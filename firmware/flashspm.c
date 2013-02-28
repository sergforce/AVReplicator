#include <avr/boot.h>
#include "flashspm.h"


#define BOOTLOADER_API_TABLE_SIZE          32
#define BOOTLOADER_API_TABLE_START         ((FLASHEND + 1UL) - BOOTLOADER_API_TABLE_SIZE)
#define BOOTLOADER_API_CALL(Index)         (void*)((BOOTLOADER_API_TABLE_START + (Index * 2)) / 2)

/*
 *  void    (*BootloaderAPI_ErasePage)(uint32_t Address)               = BOOTLOADER_API_CALL(0);
 *  void    (*BootloaderAPI_WritePage)(uint32_t Address)               = BOOTLOADER_API_CALL(1);
 *  void    (*BootloaderAPI_FillWord)(uint32_t Address, uint16_t Word) = BOOTLOADER_API_CALL(2);
 *  uint8_t (*BootloaderAPI_ReadSignature)(uint16_t Address)           = BOOTLOADER_API_CALL(3);
 *  uint8_t (*BootloaderAPI_ReadFuse)(uint16_t Address)                = BOOTLOADER_API_CALL(4);
 *  uint8_t (*BootloaderAPI_ReadLock)(void)                            = BOOTLOADER_API_CALL(5);
 *  void    (*BootloaderAPI_WriteLock)(uint8_t LockBits)               = BOOTLOADER_API_CALL(6);
 */

SPMW_SECTION uint8_t SPMWritePage(uint32_t addr, const uint8_t* data, uint16_t size)
{
    if (size > SPM_PAGESIZE)
        return 1;
    if (addr >= BOOTSPM_SIZE)
        return 2;

#ifdef USE_DFU_API
    void    (*BootloaderAPI_ErasePage)(uint32_t Address)               = BOOTLOADER_API_CALL(0);
    void    (*BootloaderAPI_WritePage)(uint32_t Address)               = BOOTLOADER_API_CALL(1);
    void    (*BootloaderAPI_FillWord)(uint32_t Address, uint16_t Word) = BOOTLOADER_API_CALL(2);
#endif
    uint16_t bytes;
    addr += BOOTSPM_START_ADDR;
    for (bytes = 0; bytes < size; bytes += 2, data += 2) {
#ifdef USE_DFU_API
        BootloaderAPI_FillWord(addr + bytes, *((uint16_t*)data));
#else
        boot_page_fill(addr + bytes, *((uint16_t*)data));
#endif
    }

#ifdef USE_DFU_API
    BootloaderAPI_ErasePage(addr);
    BootloaderAPI_WritePage(addr);
#else
    boot_page_erase(addr);
    boot_spm_busy_wait();

    boot_page_write(addr);
    boot_spm_busy_wait();

    boot_rww_enable();
#endif
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
