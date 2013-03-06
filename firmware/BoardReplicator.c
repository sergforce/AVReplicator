
/** \file
 *
 *  Main source file for the BoardReplicator. This file contains the main tasks of
 *  the project and is responsible for the initial application hardware configuration.
 */

#include "BoardReplicator.h"
#include "datast.h"
#include <PortConfig.h>
#include "spiio.h"
#include "ctcontrol.h"

#include "datast_usb.h"

#include "ui_text.h"
#include "lmk0x0xx.h"
#include "leds.h"

/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
 
int main(void)
{
	// LED INIT
    //LED_DDR = (1 << LED_1) | (1 << LED_2) | (1 << LED_3) | (1 << LED_4);
    //LED_PORT = 0;
    //LED_PORT |= (1 << LED_1);
	
	SetupHardware();
    LEDs_SetAllLEDs(LEDS_LED1);

    sei();

    LCDClear();
    LCDSetPos(0,0);
    LCDPuts_P(PSTR("Initializing..."));

    _delay_ms(3000);

    UIStart();

    for (;;)
    {
        USB_ExtraHost();
        USB_USBTask();
        UITask();
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

    LMKInit();

    CTInit(CTM_AUTO);

    EDSInit();

    LEDs_Init();

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

    USBControlRequest();

    //For now just light LED
    if(((USB_ControlRequest.bmRequestType & CONTROL_REQTYPE_TYPE) == REQTYPE_CLASS) &&
            ((USB_ControlRequest.bmRequestType & CONTROL_REQTYPE_RECIPIENT) == REQREC_DEVICE))  {

        if ((USB_ControlRequest.bmRequestType & CONTROL_REQTYPE_DIRECTION) == REQDIR_HOSTTODEVICE) {
            switch(USB_ControlRequest.bRequest)
            {
            case 0:
                // Light LED
                /* marks the command as "accepted" by the
                application, so that LUFA does not process it: */
                Endpoint_ClearSETUP();
                /* mark the whole request as successful: */
                Endpoint_ClearStatusStage();

                //LED_PORT |= (1 << LED_1);
                LEDs_SetAllLEDs(LEDS_LED1);
                break;
            case 1:
                // Light off LED
                /* marks the command as "accepted" by the
                application, so that LUFA does not process it: */
                Endpoint_ClearSETUP();
                /* mark the whole request as successful: */
                Endpoint_ClearStatusStage();

                //LED_PORT &= ~(1 << LED_1);
                LEDs_ClearAllLEDs(LEDS_LED1);
                break;
            }
        }
    }
}

void EVENT_USB_UIDChange(void)
{
    //LED_PORT ^= (1 << LED_4);
    LEDs_ToggleLEDs(LEDS_LED4);
}
