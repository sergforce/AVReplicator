/** \file
 *
 *  Header file for VirtualSerialHost.c.
 */

#ifndef _VIRTUALSERIAL_HOST_H_
#define _VIRTUALSERIAL_HOST_H_

	/* Includes: */
		#include <avr/io.h>
		#include <stdio.h>

		#include <LUFA/Drivers/USB/USB.h>

		void CDCHost_Task(void);

		void EVENT_USB_Host_HostError(const uint8_t ErrorCode);
		void EVENT_USB_Host_DeviceAttached(void);
		void EVENT_USB_Host_DeviceUnattached(void);
		void EVENT_USB_Host_DeviceEnumerationFailed(const uint8_t ErrorCode,
		                                            const uint8_t SubErrorCode);
		void EVENT_USB_Host_DeviceEnumerationComplete(void);

#endif

