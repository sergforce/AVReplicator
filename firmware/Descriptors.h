/*
             LUFA Library
     Copyright (C) Dean Camera, 2012.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2012  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaim all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *
 *  Header file for Descriptors.c.
 */

#ifndef _DESCRIPTORS_H_
#define _DESCRIPTORS_H_

	/* Includes: */
		#include <LUFA/Drivers/USB/USB.h>

        /** Descriptor type value for a DFU class functional descriptor. */
        #define DTYPE_BRFunctional               0x41

        typedef struct
        {
            USB_Descriptor_Header_t Header; /**< Standard descriptor header structure */

            uint16_t                TransferSize; /**< Maximum number of bytes the device can accept
                                                   *  from the host in a transaction
                                                   */
        } USB_Descriptor_BR_Functional_t;

        typedef struct
        {
            USB_Descriptor_Configuration_Header_t Config;

            // DFU Interface
            USB_Descriptor_Interface_t            BR_Interface;
            USB_Descriptor_BR_Functional_t        BR_Functional;
        } USB_Descriptor_Configuration_t;

/* Function Prototypes: */
    uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
                                        const uint8_t wIndex,
                                        const void** const DescriptorAddress)
                                        ATTR_WARN_UNUSED_RESULT ATTR_NON_NULL_PTR_ARG(3);
#endif

