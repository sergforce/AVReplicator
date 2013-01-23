#ifndef SPIIO_H
#define SPIIO_H


// PAGE SIZES (for ATMEGA32U2)
#define AVRISP_MEGA32U2_PAGESIZE         64
#define AVRISP_MEGA32U2_PAGESIZE_BYTES   (AVRISP_MEGA32U2_PAGESIZE*2)


// EEPROM size (for AT25256B)
#define EESPI_PAGE_SIZE                  64
#define EEPROM_SIZE_BITS                 15
#define EEPROM_SIZE                      (1U<<EEPROM_SIZE_BITS)





void eemem_write_page(uint32_t addr, const uint8_t *data, uint8_t count);
void eemem_read_page(uint32_t addr, uint8_t *data, uint8_t count);


/**
 * @brief avrisp_leave Leave AVR ISP mode
 */
void avrisp_leave(void);

/**
 * @brief avrisp_enter Enter AVR ISP mode
 * @return 0 - OK, 255 - Error, can't sync SPI
 */
int8_t avrisp_enter(void);

/**
 * @brief avrisp_readsignature
 * @return return AVR chip signature
 */
int32_t avrisp_readsignature(void);

/**
 * @brief avrisp_read_program_page
 * @param addr of AVR flash memeory (word counted)
 * @param data pointer to write
 * @param count of program words
 */
void avrisp_read_program(uint16_t addr, uint8_t* data, uint8_t count);

/**
 * @brief avrisp_flash_page
 * @param addr of AVR flash memeory (word counted), must be aligned to AVR flash page
 * @param data to write
 * @param count of program words
 */
void avrisp_flash_page(uint16_t addr, const uint8_t* data, uint8_t count);

/**
 * @brief avrisp_read_eeprom
 * @param addr of EEPROM memory
 * @param data pointer to write
 * @param count of bytes
 */
void avrisp_read_eeprom(uint16_t addr, uint8_t* data, uint8_t count);

/**
 * @brief avrisp_write_eeprom_page
 * @param addr of EEPROM memory
 * @param data to write
 * @param count of bytes
 */
void avrisp_write_eeprom_page(uint16_t addr, uint8_t* data, uint8_t count);

/**
 * @brief avrisp_chip_erase Erase FLASH & EEPROM
 */
void avrisp_chip_erase(void);

/**
 * @brief avrisp_read_fuse
 * @return FUSE byte
 */
uint8_t avrisp_read_fuse(void);

/**
 * @brief avrisp_read_fuse_high
 * @return FUSE HIGH byte
 */
uint8_t avrisp_read_fuse_high(void);

/**
 * @brief avrisp_read_fuse_ex
 * @return FUSE EXTENDED byte
 */
uint8_t avrisp_read_fuse_ex(void);


void avrisp_write_fuse(uint8_t fuse);
void avrisp_write_fuse_high(uint8_t fuse);
void avrisp_write_fuse_ex(uint8_t fuse);

#endif // SPIIO_H
