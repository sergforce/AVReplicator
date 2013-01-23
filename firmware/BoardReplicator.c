
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

const char st_prg[] PROGMEM = "\xf5Programs";
const char st_test[] PROGMEM = "\xf6Test";
const char st_gen[] PROGMEM = "\xf7Generator";

int main(void)
{
	// LED INIT
	LED_DDR = (1 << LED_1) | (1 << LED_2) | (1 << LED_3);
	LED_PORT = 0;
	LED_PORT |= (1 << LED_1);	
	
	SetupHardware();

	sei();

	LCDClear();
	LCDPuts_P(st_hello);

	LCDSetPos(1,0); LCDPuts_P(st_prg);
	LCDSetPos(2,0); LCDPuts_P(st_test);
	LCDSetPos(3,0); LCDPuts_P(st_gen);
	
	for (;;)
	{
	
		//USB_USBTask();
		LED_PORT |= (1 << LED_3);
		_delay_ms(100);
		_delay_ms(100);
		_delay_ms(100);
		_delay_ms(100);
		_delay_ms(100);
		LED_PORT &= ~(1 << LED_3);
		_delay_ms(100);
		_delay_ms(100);
		_delay_ms(100);
		_delay_ms(100);
		_delay_ms(100);		
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

