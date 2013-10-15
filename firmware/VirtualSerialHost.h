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

int16_t USB_ClockTamer_RecvLine(char* cmd, uint8_t max_reply);
int16_t USB_ClockTamer_Send(char* cmd, uint8_t max_reply);

void USB_ExtraHost(void);

void EVENT_USB_Host_HostError(const uint8_t ErrorCode);
void EVENT_USB_Host_DeviceAttached(void);
void EVENT_USB_Host_DeviceUnattached(void);
void EVENT_USB_Host_DeviceEnumerationFailed(const uint8_t ErrorCode,
                                            const uint8_t SubErrorCode);
void EVENT_USB_Host_DeviceEnumerationComplete(void);

enum USBHostStatus {
    USBH_MODE_CDC = 0,
    USBH_MODE_DFU = 1,

    USBH_ERR_ENUM_FAILED,

    USBH_ERR_CONF_DISCRIPTOR,
    USBH_ERR_CONF_PIPES,
    USBH_ERR_CONF_DEVICE,
    USBH_ERR_CDC_SETLINE,

    USBH_CONFIGURED_CDC,
    USBH_CONFIGURED_DFU

};

extern volatile uint8_t USBCommState;

#endif

