
/** \file
 *
 *  Main source file for the BoardReplicator. This file contains the main tasks of
 *  the project and is responsible for the initial application hardware configuration.
 */

#include "BoardReplicator.h"
#include <PortConfig.h>

/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
 
const char st_hello[] PROGMEM = "Hellow, World!5432";


int main(void)
{
	// LED INIT
    LED_DDR = (1 << LED_1) | (1 << LED_2) | (1 << LED_3) | (1 << LED_4);
	LED_PORT = 0;
	LED_PORT |= (1 << LED_1);	
	
	SetupHardware();

	sei();

	LCDClear();
	LCDPuts_P(st_hello);

//	LCDSetPos(1,0); LCDPuts_P(st_prg);
//	LCDSetPos(2,0); LCDPuts_P(st_test);
//	LCDSetPos(3,0); LCDPuts_P(st_gen);


    uint8_t startPos = 0;

	for (;;)
	{	
        //USB_USBTask();

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
	USB_Init();

	LCDInit();
	
    CtrlInit();

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

