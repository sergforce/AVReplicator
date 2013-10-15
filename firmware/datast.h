#ifndef DATAST_H
#define DATAST_H

/**
 * @file Data structure file
 *
 * |BlockHeader|FirmwareId|...program blocks...|...eeprom blocks...|FirmwareId|....
 *
 */

//BM00
#define BLOCK_MAGIC                  0x30304D42UL
#define MAX_FIRMWARE                 4


#ifdef USE_EEPROM
#define BLOCK_SIZE                   256
#define TOTAL_MEM_SIZE               EEPROM_TOTAL_SIZE
#else
#define BLOCK_SIZE                   SPM_PAGESIZE
#define TOTAL_MEM_SIZE               BOOTSPM_SIZE
#endif



#ifdef USE_EEPROM
#define write_block_page(x,y,z)      eemem_write_page(x,y,z)
#define read_block(x,y,z)            eemem_read_page(x,y,z)
#else
#define write_block_page(x,y,z)      SPMWritePage(x,y,z)
#define read_block(x,y,z)            SPMRead(x,y,z)
#endif

/**
 * @brief The BlockHeader struct
 *
 * Must be placed at 4 offset in memory.
 * Should be written as @ref BLOCK_SIZE bytes
 *
 */
struct BlockHeader
{
    uint32_t magic;          /**< BLOCK_MAGIC */
    uint32_t firmwareOff[4]; /**< Offset of FirmwareId structure */
    uint32_t freeOffset;     /**< pointer to free offset */
};

#define FIF_USE_LOCKS  0x01
#define FIF_USE_FUSES  0x02

/**
 * @brief The FirmwareId struct
 *
 * Must be placed at @ref BLOCK_SIZE allign in memory.
 * Should be written as @ref BLOCK_SIZE bytes
 *
 */
struct FirmwareId
{
    uint32_t offset;      /**< Offset of this structure in memory. To check this structure is valid */
    char name[16];        /**< Name of firmware */

    uint16_t programSize; /**< Program size in bytes */
    uint16_t eepromSize;  /**< EEPROM size in bytes */

    uint8_t flags;        /**< Flags for FUSES and LOCK bits */

    uint8_t locks;
    uint8_t fuse;
    uint8_t fuseHi;
    uint8_t fuseEx;
};


enum HwReason {
    HR_ENTER_PROG = 1,
    HR_PROG_FLASH    = 0x12,
    HR_PROG_EEPROM   = 0x14,
    HR_PORG_FUSES    = 0x16,
    HR_VERIFY_FLASH  = 0x22,
    HR_VERIFY_EEPROM = 0x24,
    HR_VERIFY_FUSES  = 0x26,
    HR_READ_FLASH    = 0x32,
    HR_READ_EEPROM   = 0x34,
    HR_READ_FUSES    = 0x36,
};

/**
 * Callback
 * return non-zero means break all operations
 */
typedef int8_t (*CALLBACK_FLASH_FN)(int8_t reason, int8_t progress);


/**
 * @brief EDSInit  Initialize global flash structure
 */
void EDSInit(void);

/**
 * @brief EDSClear Clear all frimware
 */
void EDSClear(void);


enum AppendFirmwareErrors {
    AFE_OK = 0,
    AFE_NO_MEM,
    AFE_FAILED_ENTER_ISP,
    AFE_FAILED_SIGNATURE,
    AFE_FAILED_PROG_FLASH,
    AFE_FAILED_PROG_EEPROM,
    AFE_FAILED_PROG_FUSE,
    AFE_FAILED_PROG_LOCK
};

/**
 * @brief EDSAppendFirmwareFromDevice Read firmware from device and store it, also select it as default
 * @param fn
 */
uint8_t EDSAppendFirmwareFromDevice(CALLBACK_FLASH_FN fn);

/**
 * @brief EDSAppendDl  Update firmware from USB
 * @param pckdata      raw control data (I/O) must be at least 8 bytes for write available
 * @param pcksize      packet size
 * @return 0 - ok, no reply; < 0 failed -- firmware discarded; > 0 ok, reply size
 */
uint8_t EDSAppendDl(uint8_t *pckdata, uint8_t pcksize);


uint8_t EDSGetFirmwareCount(void);

/**
 * @brief EDSSelectFirmware Select firmware to work with
 * @param index 0-based firmare index
 * @return pointer to structure in success, 0 on failure
 */
struct FirmwareId* EDSSelectFirmware(uint8_t index);

uint8_t EDSFlashFirmware(CALLBACK_FLASH_FN fn);

#endif // DATAST_H
