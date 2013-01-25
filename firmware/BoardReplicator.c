
/** \file
 *
 *  Main source file for the BoardReplicator. This file contains the main tasks of
 *  the project and is responsible for the initial application hardware configuration.
 */

#include "BoardReplicator.h"
#include "datast.h"
#include <PortConfig.h>
#include "spiio.h"



/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
 
const char st_hello[] PROGMEM = "Hellow, World!5432";


//           Maximum message      "0123456789012345"
const char afe_ok[] PROGMEM     = "Ok";
const char afe_no_mem[] PROGMEM = "No free memory";
const char afe_isp[] PROGMEM    = "Can't enter ISP";
const char afe_sig[] PROGMEM    = "Incorrect sign.";
const char afe_unkn[] PROGMEM   = "Unknown error";


int main(void)
{
	// LED INIT
    LED_DDR = (1 << LED_1) | (1 << LED_2) | (1 << LED_3) | (1 << LED_4);
	LED_PORT = 0;
	LED_PORT |= (1 << LED_1);	
	
	SetupHardware();

	sei();    

	LCDClear();

    uint8_t startPos = 0;

	for (;;)
	{	
        USB_USBTask();

		LED_PORT |= (1 << LED_3);
        _delay_ms(10);
		LED_PORT &= ~(1 << LED_3);
        _delay_ms(10);


        CtrlUpdate();
        uint8_t update = 0;

        if (CtrlIsUpPressed()) {
            startPos += 0x10;
            update = 1;
        } else if (CtrlIsDownPressed()) {
            startPos -= 0x10;
            update = 1;
        }
        if (CtrlIsOkPressed()) {
            LED_PORT ^= (1 << LED_4);
        }
        if (CtrlIsBackPressed()) {
            LED_PORT ^= (1 << LED_2);

            LCDSetPos(3,0);
            switch (EDSAppendFirmwareFromDevice(0)) {
            case AFE_OK:               LCDPuts_P(afe_ok); break;
            case AFE_NO_MEM:           LCDPuts_P(afe_no_mem); break;
            case AFE_FAILED_ENTER_ISP: LCDPuts_P(afe_isp); break;
            case AFE_FAILED_SIGNATURE: LCDPuts_P(afe_sig); break;
            default:                   LCDPuts_P(afe_unkn); break;
            }
        }

        if (update) {
            uint8_t q,k;
            for (q=1; q<4; q++) {
                LCDSetPos(q,0);
                for (k=0; k < 16; k++) {
                    char sym[2];
                    sym[0] = startPos + 0x10 * q + k;
                    sym[1] = 0;

                    LCDPuts(sym);
                }
            }
        }
	}
}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void)
{
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);

	/* Hardware Initialization */
    USB_Init(USB_MODE_UID);

	LCDInit();
	
    CtrlInit();

    SPIInit();

    EDSInit();
	XMCRA = 0;
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
//	CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
//	CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
}

void EVENT_USB_UIDChange(void)
{
    LED_PORT ^= (1 << LED_4);
}
