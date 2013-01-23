#ifndef DATAST_H
#define DATAST_H

/**
 * @file Data structure file
 *
 * |BlockHeader|FirmwareId|...program blocks...|...eeprom blocks...|FirmwareId|....
 *
 */

#define BLOCK_SIZE 256

/**
 * @brief The BlockHeader struct
 *
 * Must be placed at zero offset in memory.
 * Should be written as @ref BLOCK_SIZE bytes
 *
 */
struct BlockHeader
{
    uint16_t firmwareOff[4]; /**< Offset of FirmwareId structure */
    uint16_t freeOffset;     /**< pointer to free offset */
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
    uint16_t offset;      /**< Offset of this structure in memory. To check this structure is valid */
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
    HR_PROG_FLASH,
    HR_PROG_EEPROM,
    HR_PORG_FUSES,
    HR_VERIFY_FLASH,
    HR_VERIFY_EEPROM,
    HR_VERIFY_FUSES,
    HR_READ_FLASH,
    HR_READ_EEPROM,
    HR_READ_FUSES
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

/**
 * @brief EDSAppendFirmwareFromDevice Read firmware from device and store it, also select it as default
 * @param fn
 */
void EDSAppendFirmwareFromDevice(CALLBACK_FLASH_FN fn);

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
