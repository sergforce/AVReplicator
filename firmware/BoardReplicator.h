/** \file
 *
 *  Header file for BoardReplicator.c.
 */

#ifndef _BOARDREPLICATOR_H_
#define _BOARDREPLICATOR_H_

	/* Includes: */
		#include <avr/io.h>
		#include <avr/wdt.h>
		#include <avr/power.h>
		#include <avr/interrupt.h>
//		#include <string.h>
//		#include <stdio.h>

//		#include "Descriptors.h"

		#include <LUFA/Drivers/USB/USB.h>

		#include "lcd_text.h"
        #include "control.h"
		
	/* Function Prototypes: */
		void SetupHardware(void);

		void EVENT_USB_Device_ConfigurationChanged(void);
		void EVENT_USB_Device_ControlRequest(void);

#endif

