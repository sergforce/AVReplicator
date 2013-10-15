#ifndef DATAST_USB_H
#define DATAST_USB_H

#define USBCT_READFW   0x10
#define USBCT_CLEAR    0x11
#define USBCT_WRITEFW  0x12


void USBControlReadFirmware(uint8_t fid, uint16_t start_block);

void USBControlClearAll(void);

/**
 * @brief USBControlWriteFirmware
 * @param idx  values 0 - start block; 0xffff - finishing block with zero size; otherwise data blocks
 *
 * Zero block must be always first with size of one block, followed by data blocks with size > 0 and finishing
 * with the flush block
 */
void USBControlWriteFirmware(uint16_t idx);


void USBControlRequest(void);

#endif // DATAST_USB_H
