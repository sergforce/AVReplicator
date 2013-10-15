#include <LUFA/Drivers/USB/USB.h>

#include "datast.h"
#include "datast_usb.h"
#include "flashspm.h"

extern struct BlockHeader s_head;
extern struct FirmwareId  s_fwactive;

static uint8_t Endpoint_Write_Control_PEStream_LE (const uint32_t Buffer, uint16_t Length)
{
    uint32_t DataStream     = Buffer;
    bool     LastPacketFull = false;

    if (Length > USB_ControlRequest.wLength)
        Length = USB_ControlRequest.wLength;
    else if (!(Length))
        Endpoint_ClearIN();

    while (Length || LastPacketFull)  {
        uint8_t USB_DeviceState_LCL = USB_DeviceState;

        if (USB_DeviceState_LCL == DEVICE_STATE_Unattached)
            return ENDPOINT_RWCSTREAM_DeviceDisconnected;
        else if (USB_DeviceState_LCL == DEVICE_STATE_Suspended)
            return ENDPOINT_RWCSTREAM_BusSuspended;
        else if (Endpoint_IsSETUPReceived())
            return ENDPOINT_RWCSTREAM_HostAborted;
        else if (Endpoint_IsOUTReceived())
            break;

        if (Endpoint_IsINReady()) {
            uint16_t BytesInEndpoint = Endpoint_BytesInEndpoint();

            while (Length && (BytesInEndpoint < USB_Device_ControlEndpointSize)) {
                Endpoint_Write_16_LE(pgm_read_word_far(DataStream));
                DataStream += 2;
                Length -= 2;
                BytesInEndpoint += 2;

                //Endpoint_Write_8(pgm_read_byte_far(DataStream));
                //DataStream++;
                //Length--;
                //BytesInEndpoint++;
            }

            LastPacketFull = (BytesInEndpoint == USB_Device_ControlEndpointSize);
            Endpoint_ClearIN();
        }
    }

    while (!(Endpoint_IsOUTReceived())) {
        uint8_t USB_DeviceState_LCL = USB_DeviceState;

        if (USB_DeviceState_LCL == DEVICE_STATE_Unattached)
            return ENDPOINT_RWCSTREAM_DeviceDisconnected;
        else if (USB_DeviceState_LCL == DEVICE_STATE_Suspended)
            return ENDPOINT_RWCSTREAM_BusSuspended;
    }

    return ENDPOINT_RWCSTREAM_NoError;
}



static uint16_t AlignToBlock(uint16_t sz)
{
    uint16_t rem = sz & (BLOCK_SIZE - 1);
    if (rem) {
        sz &= ~((uint16_t)BLOCK_SIZE - 1);
        sz += BLOCK_SIZE;
    }
    return sz;
}

void USBControlReadFirmware(uint8_t fid, uint16_t start_block)
{
    if (EDSSelectFirmware(fid)) {
        Endpoint_ClearSETUP();


        uint32_t max_size = BLOCK_SIZE + AlignToBlock(s_fwactive.programSize) + AlignToBlock(s_fwactive.eepromSize);
        max_size -= start_block * BLOCK_SIZE;

        if (Endpoint_Write_Control_PEStream_LE(s_fwactive.offset + (uint32_t)start_block * BLOCK_SIZE + BOOTSPM_START_ADDR, max_size) == ENDPOINT_RWCSTREAM_NoError) {
            Endpoint_ClearOUT();
        }

    } else {
        // Mark transaction unsuccessful
        //while (!Endpoint_IsINReady());
        //Endpoint_
    }
}

void USBControlClearAll(void)
{
    EDSClear();

    Endpoint_ClearSETUP();
    /* mark the whole request as successful: */
    Endpoint_ClearStatusStage();
}

static uint32_t CheckNewFirmware(uint8_t* blk)
{
    struct FirmwareId *pfw = (struct FirmwareId *)blk;

    if (s_head.freeOffset +
            pfw->eepromSize +
            pfw->programSize +
            BLOCK_SIZE > TOTAL_MEM_SIZE)
        return 0;

    uint8_t i;
    for (i = 0; i < MAX_FIRMWARE; i++) {
        if (s_head.firmwareOff[i] == 0) {
            break;
        }
    }

    if (i == MAX_FIRMWARE)
        return 0; //no space

    s_fwactive = *pfw;
    s_fwactive.offset = s_head.freeOffset;
    return s_fwactive.offset;
}

void USBControlWriteFirmware(uint16_t idx)
{
    Endpoint_ClearSETUP();

    uint8_t tmpblk[BLOCK_SIZE];
    uint8_t *tmpptr = tmpblk;
    uint16_t length = USB_ControlRequest.wLength;

    uint32_t addr;

    if (idx > 0) {
        //firmware has been accepted
        addr = s_fwactive.offset + (idx) * BLOCK_SIZE;
    } else {
        // Determine start address after first block
        addr = 0;
    }

    if (!(length)) {
        Endpoint_ClearOUT();

        if (idx == 0x0fff) {
            // Write header
            write_block_page(s_fwactive.offset, (uint8_t*)&s_fwactive, sizeof(s_fwactive));
            uint8_t i;
            for (i = 0; i < MAX_FIRMWARE; i++) {
                if (s_head.firmwareOff[i] == 0) break;
            }
            s_head.firmwareOff[i] = s_fwactive.offset;
            s_head.freeOffset += s_fwactive.eepromSize +
                    s_fwactive.programSize + BLOCK_SIZE; // FIXME: Not aligned to block size
            // Update header block
            write_block_page(0, (uint8_t*)&s_head, sizeof(s_head));
        }
    }

    while (length)
    {
        uint8_t USB_DeviceState_LCL = USB_DeviceState;

        if (USB_DeviceState_LCL == DEVICE_STATE_Unattached)
            goto transfer_error;
          //return ENDPOINT_RWCSTREAM_DeviceDisconnected;
        else if (USB_DeviceState_LCL == DEVICE_STATE_Suspended)
            goto transfer_error;
          //return ENDPOINT_RWCSTREAM_BusSuspended;
        else if (Endpoint_IsSETUPReceived())
            goto transfer_error;
          //return ENDPOINT_RWCSTREAM_HostAborted;

        if (Endpoint_IsOUTReceived())
        {
            while (length && Endpoint_BytesInEndpoint())
            {
                *(tmpptr++) = Endpoint_Read_8();
                length--;

                if (tmpptr == tmpblk + BLOCK_SIZE) {
                    tmpptr = tmpblk;
                    // Process new block

                    if (addr == 0) {
                        //Pprocess header
                        addr = CheckNewFirmware(tmpblk);
                        if (addr == 0) {
                            goto transfer_error;
                        }
                    }

                    write_block_page(addr, tmpblk, BLOCK_SIZE);
                    addr += BLOCK_SIZE;
                }
            }

            Endpoint_ClearOUT();
        }
    }

    if (tmpptr != tmpblk) {
        //Incomplete block
        goto transfer_error;
    }

    // ack transaction
    while (!(Endpoint_IsINReady()))
    {
        uint8_t USB_DeviceState_LCL = USB_DeviceState;

        if (USB_DeviceState_LCL == DEVICE_STATE_Unattached)
            goto transfer_error;
          //return ENDPOINT_RWCSTREAM_DeviceDisconnected;
        else if (USB_DeviceState_LCL == DEVICE_STATE_Suspended)
            goto transfer_error;
          //return ENDPOINT_RWCSTREAM_BusSuspended;
    }
    Endpoint_ClearIN();

    // Ok
transfer_error:
    ;
}



void USBControlRequest(void)
{
    if(((USB_ControlRequest.bmRequestType & CONTROL_REQTYPE_TYPE) == REQTYPE_CLASS) &&
            ((USB_ControlRequest.bmRequestType & CONTROL_REQTYPE_RECIPIENT) == REQREC_DEVICE))  {

        if ((USB_ControlRequest.bmRequestType & CONTROL_REQTYPE_DIRECTION) == REQDIR_HOSTTODEVICE) {
            switch(USB_ControlRequest.bRequest)
            {
            case USBCT_CLEAR:
                USBControlClearAll();
                break;
            case USBCT_WRITEFW:
                USBControlWriteFirmware(USB_ControlRequest.wIndex);
                break;
            }
        } else {
            switch(USB_ControlRequest.bRequest)
            {
            case USBCT_READFW:
                USBControlReadFirmware(USB_ControlRequest.wIndex>>12, USB_ControlRequest.wIndex & 0x0fff);
                break;
            }
        }
    }
}
