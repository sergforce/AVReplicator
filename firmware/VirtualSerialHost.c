#include "VirtualSerialHost.h"

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

#if 0
int main(void)
{
	SetupHardware();

	puts_P(PSTR(ESC_FG_CYAN "CDC Host Demo running.\r\n" ESC_FG_WHITE));

	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
	sei();

	for (;;)
	{
		CDCHost_Task();

		CDC_Host_USBTask(&VirtualSerial_CDC_Interface);
		USB_USBTask();
	}
}

/** Task to manage an enumerated USB CDC device once connected, to print received data
 *  from the device to the serial port.
 */
void CDCHost_Task(void)
{
	if (USB_HostState != HOST_STATE_Configured)
	  return;

	if (CDC_Host_BytesReceived(&VirtualSerial_CDC_Interface))
	{
		/* Echo received bytes from the attached device through the USART */
		int16_t ReceivedByte = CDC_Host_ReceiveByte(&VirtualSerial_CDC_Interface);
		if (!(ReceivedByte < 0))
		  putchar(ReceivedByte);
	}
}

#endif


/** Event handler for the USB_DeviceAttached event. This indicates that a device has been attached to the host, and
 *  starts the library USB task to begin the enumeration and USB management process.
 */
void EVENT_USB_Host_DeviceAttached(void)
{
    //puts_P(PSTR("Device Attached.\r\n"));
    //LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);
}

/** Event handler for the USB_DeviceUnattached event. This indicates that a device has been removed from the host, and
 *  stops the library USB task management process.
 */
void EVENT_USB_Host_DeviceUnattached(void)
{
    //puts_P(PSTR("\r\nDevice Unattached.\r\n"));
    //LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
}

/** Event handler for the USB_DeviceEnumerationComplete event. This indicates that a device has been successfully
 *  enumerated by the host and is now ready to be used by the application.
 */
void EVENT_USB_Host_DeviceEnumerationComplete(void)
{
    //LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);

	uint16_t ConfigDescriptorSize;
	uint8_t  ConfigDescriptorData[512];

	if (USB_Host_GetDeviceConfigDescriptor(1, &ConfigDescriptorSize, ConfigDescriptorData,
	                                       sizeof(ConfigDescriptorData)) != HOST_GETCONFIG_Successful)
	{
        //puts_P(PSTR("Error Retrieving Configuration Descriptor.\r\n"));
        //LEDs_SetAllLEDs(LEDMASK_USB_ERROR);
		return;
	}

	if (CDC_Host_ConfigurePipes(&VirtualSerial_CDC_Interface,
	                            ConfigDescriptorSize, ConfigDescriptorData) != CDC_ENUMERROR_NoError)
	{
        //puts_P(PSTR("Attached Device Not a Valid CDC Class Device.\r\n"));
        //LEDs_SetAllLEDs(LEDMASK_USB_ERROR);
		return;
	}

	if (USB_Host_SetDeviceConfiguration(1) != HOST_SENDCONTROL_Successful)
	{
        //puts_P(PSTR("Error Setting Device Configuration.\r\n"));
        //LEDs_SetAllLEDs(LEDMASK_USB_ERROR);
		return;
	}
/*
	VirtualSerial_CDC_Interface.State.LineEncoding.BaudRateBPS = 9600;
	VirtualSerial_CDC_Interface.State.LineEncoding.CharFormat  = CDC_LINEENCODING_OneStopBit;
	VirtualSerial_CDC_Interface.State.LineEncoding.ParityType  = CDC_PARITY_None;
	VirtualSerial_CDC_Interface.State.LineEncoding.DataBits    = 8;
	
	if (CDC_Host_SetLineEncoding(&VirtualSerial_CDC_Interface))
	{
        puts_P(PSTR("Error Setting Device Line Encoding.\r\n"));
        LEDs_SetAllLEDs(LEDMASK_USB_ERROR);
		return;	
	}
*/

//	puts_P(PSTR("CDC Device Enumerated.\r\n"));
//	LEDs_SetAllLEDs(LEDMASK_USB_READY);
}

/** Event handler for the USB_HostError event. This indicates that a hardware error occurred while in host mode. */
void EVENT_USB_Host_HostError(const uint8_t ErrorCode)
{
	USB_Disable();

//	printf_P(PSTR(ESC_FG_RED "Host Mode Error\r\n"
//	                         " -- Error Code %d\r\n" ESC_FG_WHITE), ErrorCode);

//	LEDs_SetAllLEDs(LEDMASK_USB_ERROR);
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
}

