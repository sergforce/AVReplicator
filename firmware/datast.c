#include <avr/io.h>

#include "spiio.h"
#include "flashspm.h"
#include "datast.h"

#ifdef USE_EEPROM
#define write_block_page(x,y,z)      eemem_write_page(x,y,z)
#define read_block(x,y,z)            eemem_read_page(x,y,z)
#define BLOCK_SIZE                   256
#define TOTAL_MEM_SIZE               EEPROM_TOTAL_SIZE
#else
#define write_block_page(x,y,z)      SPMWritePage(x,y,z)
#define read_block(x,y,z)            SPMRead(x,y,z)
#define BLOCK_SIZE                   SPM_PAGESIZE
#define TOTAL_MEM_SIZE               BOOTSPM_SIZE
#endif


static struct BlockHeader s_head;
static struct FirmwareId  s_fwactive;


void EDSInit(void)
{
    read_block(0, (uint8_t*)&s_head, sizeof(s_head));
    if (s_head.freeOffset > TOTAL_MEM_SIZE ||
        s_head.magic != BLOCK_MAGIC) {
        // Incorrect data header, clear it

        EDSClear();
        return;
    }
}

void EDSClear(void)
{
    uint8_t i;
    s_head.magic = BLOCK_MAGIC;
    for (i = 0; i < 4; i++)
        s_head.firmwareOff[i] = 0;
    s_head.freeOffset = BLOCK_SIZE;

    write_block_page(0, (uint8_t*)&s_head, sizeof(s_head));
}



uint8_t EDSAppendFirmwareFromDevice(CALLBACK_FLASH_FN fn)
{
    if (s_head.freeOffset +
            AVRISP_MEGA32U2_FLASHSIZE +
            AVRISP_MEGA32U2_EESIZE +
            sizeof(struct FirmwareId) > TOTAL_MEM_SIZE)
        return AFE_NO_MEM;

    if (avrisp_enter() != 0)
        return AFE_FAILED_ENTER_ISP;

    uint8_t errcode = AFE_OK;

    if (avrisp_readsignature() != AVRISP_MEGA32U2_ID) {
        errcode = AFE_FAILED_SIGNATURE;
        goto error_isp;
    }

    s_fwactive.offset = s_head.freeOffset;
    s_fwactive.flags = 0;
    s_fwactive.fuse = avrisp_read_fuse();
    s_fwactive.fuseHi = avrisp_read_fuse_high();
    s_fwactive.fuseEx = avrisp_read_fuse_ex();
    s_fwactive.locks = 0;
    s_fwactive.programSize = AVRISP_MEGA32U2_FLASHSIZE;
    s_fwactive.eepromSize = AVRISP_MEGA32U2_EESIZE;
    s_fwactive.name[0] = 'F';
    s_fwactive.name[1] = '0';
    s_fwactive.name[2] = 0;

    // Read program
    // uint16_t eeblocks = AVRISP_MEGA32U2_FLASHSIZE / EESPI_PAGE_SIZE;
    uint32_t dataoffset = s_fwactive.offset + BLOCK_SIZE;
    uint16_t addr;
    uint8_t tmpdata[BLOCK_SIZE];

    for (addr = 0; addr < AVRISP_MEGA32U2_FLASHSIZE; addr += BLOCK_SIZE, dataoffset += BLOCK_SIZE) {
        avrisp_read_program(addr/2, tmpdata, BLOCK_SIZE/2);
        write_block_page(dataoffset, tmpdata, BLOCK_SIZE);
    }

    // Read eeprom
    for (addr = 0; addr < AVRISP_MEGA32U2_EESIZE; addr += BLOCK_SIZE, dataoffset += BLOCK_SIZE) {
        avrisp_read_eeprom(addr, tmpdata, BLOCK_SIZE);
        write_block_page(dataoffset, tmpdata, BLOCK_SIZE);
    }

    // Store data to storage
    write_block_page(s_fwactive.offset, (uint8_t*)&s_fwactive, sizeof(s_fwactive));

    // Update Header
    addr = dataoffset & (BLOCK_SIZE - 1);
    if (addr) {
        dataoffset &= ~((uint32_t)BLOCK_SIZE - 1);
        dataoffset += BLOCK_SIZE;
    }
    s_head.freeOffset = dataoffset;

    uint8_t i;
    for (i = 0; i < 4; i++) {
        if (s_head.firmwareOff[i] == 0) break;
    }

    if (i == 4) {
        errcode = AFE_NO_MEM;
    } else {
        s_head.firmwareOff[i] = s_fwactive.offset;
        write_block_page(0, (uint8_t*)&s_head, sizeof(s_head));
    }

error_isp:
    avrisp_leave();
    return errcode;
}
