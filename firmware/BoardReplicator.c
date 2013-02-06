
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

const char afe_fprog[] PROGMEM  = "Failed flash";
const char afe_fee[] PROGMEM    = "Failed eeprom";
const char afe_ff[] PROGMEM     = "Failed fuses";


const char afe_nofw[] PROGMEM   = "No firmwares";

//const char ct_ver[] PROGMEM = "HWI";
const char ct_ver[] PROGMEM = "SET,STS,AUT";

void DisplayError(uint8_t code)
{
    switch (code) {
    case AFE_OK:               LCDPuts_P(afe_ok); break;
    case AFE_NO_MEM:           LCDPuts_P(afe_no_mem); break;
    case AFE_FAILED_ENTER_ISP: LCDPuts_P(afe_isp); break;
    case AFE_FAILED_SIGNATURE: LCDPuts_P(afe_sig); break;
    case AFE_FAILED_PROG_FLASH:LCDPuts_P(afe_fprog); break;
    case AFE_FAILED_PROG_EEPROM:LCDPuts_P(afe_fee); break;
    case AFE_FAILED_PROG_FUSE: LCDPuts_P(afe_ff); break;
    default:                   LCDPuts_P(afe_unkn); break;
    }
}

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


    UIStart();

    for (;;)
    {
        USB_ExtraHost();
        USB_USBTask();
        UITask();
    }

#ifdef OLD_CODE
    for (;;)
	{	
        USB_ExtraHost();

        USB_USBTask();

        //LED_PORT |= (1 << LED_3);
        //_delay_ms(10);
        //LED_PORT &= ~(1 << LED_3);
        //_delay_ms(10);


        CtrlUpdate();
        uint8_t update = 0;

        if (CtrlIsUpPressed()) {
            startPos += 0x10;
            update = 1;
        } else if (CtrlIsDownPressed()) {
            //startPos -= 0x10;
            //update = 1;

            CTSetLed(0);


            LCDClear();
            char *pd = CTHwi();
            if (pd) {
                LCDPutsBig(pd);
            }
#if 0
            char d[65];
            uint8_t len = clocktamer_sendcmd_p(ct_ver, d, sizeof(d));
            if (len > 0) {
               d[len] = 0;

               LCDClear();
               //LCDSetPos(1,0);
               //LCDPuts(d);
               LCDPutsBig(d);


               uint8_t k;
               char *pos = d;
               int8_t fl = 64;

               for (k = 0; k < 50; k++) {
                   len = clocktamer_get_replyln(pos, (uint8_t)fl);
                   if (len > 3) {
                       if (pos[len] == 0) {
                         LCDClear();
                         LCDPutsBig(d);
                         pos = d;
                         fl = 64;
                       } else {
                         pos += len;
                         fl -= len;

                         if (fl <= 0) {
                             LCDClear();
                             LCDPutsBig(d);
                             pos = d;
                             fl = 64;
                         } else {
                             LCDClear();
                             LCDPutsBig(d);

                         }

                       }
                   }
               }
            }
#endif
        }
        if (CtrlIsOkPressed()) {
            LED_PORT ^= (1 << LED_4);

            LCDSetPos(3,0);
            struct FirmwareId* p = EDSSelectFirmware(0);
            if (p) {
                DisplayError(EDSFlashFirmware(0));
            } else {
                LCDPuts_P(afe_nofw);
            }
        }
        if (CtrlIsBackPressed()) {
            LED_PORT ^= (1 << LED_2);

            LCDSetPos(3,0);
            DisplayError(EDSAppendFirmwareFromDevice(0));
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
#endif
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

                LED_PORT |= (1 << LED_1);
                break;
            case 1:
                // Light off LED
                /* marks the command as "accepted" by the
                application, so that LUFA does not process it: */
                Endpoint_ClearSETUP();
                /* mark the whole request as successful: */
                Endpoint_ClearStatusStage();

                LED_PORT &= ~(1 << LED_1);
                break;
            }
        }
    }
}

void EVENT_USB_UIDChange(void)
{
    LED_PORT ^= (1 << LED_4);
}
