#ifndef CTCONTROL_H
#define CTCONTROL_H

/** @file ClockTamer control protocol API
 *
 */

enum CTResult {
    CTR_OK = 0,
    CTR_IO_ERROR,
    CTR_SYNTAX_ERROR,
    CTR_CMD_ERROR,
    CTR_FAILED,
    CTR_BAD_TUNING_RANGE,
    CTR_CANT_TUNE,

    CTR_INCORRECT_REPLY,
    CTR_LAST_MSG_NO /**< Always should be last in this enum */
};

enum CTMode {
    CTM_SPI,
    CTM_USB,
    CTM_AUTO
};

/**
 * @brief CTInit Initialize communication channel to ClockTamer
 * @param mode type of @ref CTMode
 * @return return type @ref CTResult
 */
uint8_t CTInit(uint8_t mode);

/**
 * @brief CTSetOutput Set output frequency
 * @param freq  frequency in Hertz
 * @return return type @ref CTResult
 */
uint8_t CTSetOutput(uint32_t freq);


/**
 * @brief CTSetOsc Set oscillator frequency
 * @param osc frequency in Hertz
 * @return return type @ref CTResult
 */
uint8_t CTSetOsc(uint32_t osc);


/**
 * @brief CTEnableOutputs Enable/Disable outputs
 * @param outputs bit mask
 * @return return type @ref CTResult
 */
uint8_t CTEnableOutputs(uint8_t outputs);


/**
 * @brief CTStoreToEEPROM Save current configuration to EEPROM
 * @return return type @ref CTResult
 */
uint8_t CTStoreToEEPROM(void);


/**
 * @brief CTLoadFromEEPROM Load and set configuration from EEPROM
 * @return return type @ref CTResult
 */
uint8_t CTLoadFromEEPROM(void);


/**
 * @brief CTGetOsc Read oscillator frequency from CT
 * @param posc pointer to store oscillator frequency
 * @return return type @ref CTResult
 */
uint8_t CTGetOsc(uint32_t *posc);


uint8_t CTSetLed(uint8_t led);


char* CTVer(void);

char* CTHwi(void);


typedef enum tagSFTCmdType {
    SFT_ERROR,
    SFT_UNKNOWN,
    SFT_PIN_LOCK,
    SFT_SET_FREQ,
    SFT_COUNTED,
    SFT_END
} SFTCmdType;

typedef  void (*CTOnSelfTestEvent)(uint8_t type, uint32_t value, uint8_t res);

uint8_t CTSelfTest(CTOnSelfTestEvent event);

#endif // CTCONTROL_H
