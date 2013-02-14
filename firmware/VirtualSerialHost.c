#include "VirtualSerialHost.h"
#include <PortConfig.h>

#include "lcd_text.h"

/** LUFA CDC Class driver interface configuration and state information. This structure is
 *  passed to all CDC Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_CDC_Host_t VirtualSerial_CDC_Interface =
	{
		.Config =
			{
				.DataINPipe             =
					{
						.Address        = (PIPE_DIR_IN  | 1),
						.Banks          = 1,
					},
				.DataOUTPipe            =
					{
						.Address        = (PIPE_DIR_OUT | 2),
						.Banks          = 1,
					},
				.NotificationPipe       =
					{
						.Address        = (PIPE_DIR_IN  | 3),
						.Banks          = 1,
					},
			},
	};

void USB_ExtraHost(void)
{
    CDC_Host_USBTask(&VirtualSerial_CDC_Interface);
}

int16_t USB_ClockTamer_RecvLine(char* cmd, uint8_t max_reply)
{
    uint32_t timeout = 300000; // 3 sec
    char* start_ptr = cmd;

    while (max_reply > 0 && timeout > 0) {
        if (USB_HostState != HOST_STATE_Configured)
          return -1; //Error

        while (CDC_Host_BytesReceived(&VirtualSerial_CDC_Interface)) {
            int16_t rb = CDC_Host_ReceiveByte(&VirtualSerial_CDC_Interface);
            if (!(rb < 0)) {
                if (rb == '\r')
                    continue;
                if (rb == '\n') {
                    *(cmd++) = 0;
                    return cmd - start_ptr - 1;
                }
                *(cmd++) = rb;
            }
        }

        CDC_Host_USBTask(&VirtualSerial_CDC_Interface);
        USB_USBTask();
        _delay_us(10);
        --timeout;
    }

    //Timed out
    *(cmd++) = 0;
    return cmd - start_ptr - 1;
}

int16_t USB_ClockTamer_Send(char *cmd, uint8_t max_reply)
{
    uint8_t res;
    res = CDC_Host_SendString(&VirtualSerial_CDC_Interface, cmd);
    if (res != PIPE_READYWAIT_NoError)
        return -1;

    res = CDC_Host_SendByte(&VirtualSerial_CDC_Interface, '\n');
    if (res != PIPE_READYWAIT_NoError)
        return -1;

    res = CDC_Host_Flush(&VirtualSerial_CDC_Interface);
    if (res != PIPE_READYWAIT_NoError)
        return -1;

    return USB_ClockTamer_RecvLine(cmd, max_reply);
}

/** Event handler for the USB_DeviceAttached event. This indicates that a device has been attached to the host, and
 *  starts the library USB task to begin the enumeration and USB management process.
 */
void EVENT_USB_Host_DeviceAttached(void)
{
    //puts_P(PSTR("Device Attached.\r\n"));
    //LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);
    //LCDPuts_P(PSTR("HostAttached"));
    LED_PORT |= (1 << LED_3);
}

/** Event handler for the USB_DeviceUnattached event. This indicates that a device has been removed from the host, and
 *  stops the library USB task management process.
 */
void EVENT_USB_Host_DeviceUnattached(void)
{
    //puts_P(PSTR("\r\nDevice Unattached.\r\n"));
    //LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
    LED_PORT &= ~(1 << LED_3);
}

volatile uint8_t USBCommState;

/** Event handler for the USB_DeviceEnumerationComplete event. This indicates that a device has been successfully
 *  enumerated by the host and is now ready to be used by the application.
 */
void EVENT_USB_Host_DeviceEnumerationComplete(void)
{
    //LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);
    //LCDPuts_P(PSTR("0"));

	uint16_t ConfigDescriptorSize;
	uint8_t  ConfigDescriptorData[512];

	if (USB_Host_GetDeviceConfigDescriptor(1, &ConfigDescriptorSize, ConfigDescriptorData,
	                                       sizeof(ConfigDescriptorData)) != HOST_GETCONFIG_Successful)
	{
        //puts_P(PSTR("Error Retrieving Configuration Descriptor.\r\n"));
        //LEDs_SetAllLEDs(LEDMASK_USB_ERROR);
        //LCDPuts_P(PSTR("ErrorConfDiscr"));
        USBCommState = USBH_ERR_CONF_DISCRIPTOR;
		return;
	}

    if (USBCommState != USBH_MODE_DFU) {
	if (CDC_Host_ConfigurePipes(&VirtualSerial_CDC_Interface,
	                            ConfigDescriptorSize, ConfigDescriptorData) != CDC_ENUMERROR_NoError)
	{
        //puts_P(PSTR("Attached Device Not a Valid CDC Class Device.\r\n"));
        //LEDs_SetAllLEDs(LEDMASK_USB_ERROR);
        //LCDPuts_P(PSTR("ErrorCPipes"));
        USBCommState = USBH_ERR_CONF_PIPES;
		return;
	}
    }

	if (USB_Host_SetDeviceConfiguration(1) != HOST_SENDCONTROL_Successful)
	{
        //puts_P(PSTR("Error Setting Device Configuration.\r\n"));
        //LEDs_SetAllLEDs(LEDMASK_USB_ERROR);
        //LCDPuts_P(PSTR("ErrorSetConf"));
        USBCommState = USBH_ERR_CONF_DEVICE;
		return;
	}

    if (USBCommState == USBH_MODE_DFU) {
        USBCommState = USBH_CONFIGURED_DFU;
        return;
    }

	VirtualSerial_CDC_Interface.State.LineEncoding.BaudRateBPS = 9600;
	VirtualSerial_CDC_Interface.State.LineEncoding.CharFormat  = CDC_LINEENCODING_OneStopBit;
	VirtualSerial_CDC_Interface.State.LineEncoding.ParityType  = CDC_PARITY_None;
	VirtualSerial_CDC_Interface.State.LineEncoding.DataBits    = 8;
	
	if (CDC_Host_SetLineEncoding(&VirtualSerial_CDC_Interface))
	{
        //puts_P(PSTR("Error Setting Device Line Encoding.\r\n"));
        //LEDs_SetAllLEDs(LEDMASK_USB_ERROR);
        //LCDPuts_P(PSTR("ErrorSetLine"));
        USBCommState = USBH_ERR_CDC_SETLINE;
		return;	
	}

    USBCommState = USBH_CONFIGURED_CDC;
//    LCDPuts_P(PSTR("Enumerated"));
//	puts_P(PSTR("CDC Device Enumerated.\r\n"));
//	LEDs_SetAllLEDs(LEDMASK_USB_READY);
}

void LCD_PrintU16(uint16_t val);
/** Event handler for the USB_HostError event. This indicates that a hardware error occurred while in host mode. */
void EVENT_USB_Host_HostError(const uint8_t ErrorCode)
{
	USB_Disable();
    LCDClear();
    LCDSetPos(0,0);
    LCDPuts_P(PSTR("Host Mode Error!"));
    LCDSetPos(1,0);
    LCDPuts_P(PSTR("Unattach device"));
    LCDSetPos(2,0);
    LCDPuts_P(PSTR(" and reset the "));
    LCDSetPos(3,0);
    LCDPuts_P(PSTR("board.HALTED"));
    LCD_PrintU16(ErrorCode);

	for(;;);
}

/** Event handler for the USB_DeviceEnumerationFailed event. This indicates that a problem occurred while
 *  enumerating an attached USB device.
 */
void EVENT_USB_Host_DeviceEnumerationFailed(const uint8_t ErrorCode,
                                            const uint8_t SubErrorCode)
{
//	printf_P(PSTR(ESC_FG_RED "Dev Enum Error\r\n"
//	                         " -- Error Code %d\r\n"
//	                         " -- Sub Error Code %d\r\n"
//	                         " -- In State %d\r\n" ESC_FG_WHITE), ErrorCode, SubErrorCode, USB_HostState);

    //LEDs_SetAllLEDs(LEDMASK_USB_ERROR);

    //LCDPuts_P(PSTR(" EnumFailed"));
    USBCommState = USBH_ERR_ENUM_FAILED;
}

