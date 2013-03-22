#include "ui_text.h"
#include <avr/pgmspace.h>
#include <string.h>
#include <stdlib.h>
#include "datast.h"

#include "lmk0x0xx.h"
#include "freqmes.h"

#include "uint64_ops.h"

#define LCD_HEIGHT   4
#define LCD_WIDTH   16


typedef void (*on_event_t)(void);

struct WidgetEvents {
    on_event_t on_up;
    on_event_t on_down;
    on_event_t on_ok;
    on_event_t on_back;

    on_event_t on_spin_up;
    on_event_t on_spin_down;
    on_event_t on_spin_push;

    on_event_t on_task;   /**< call periodically from time to time on active widget */
};
struct WidgetData {
    void *pointer;
    uint8_t data[18];
};


/**
 * @brief active_widget
 *
 * Read from program memory into RAM, overrided each time widget changes
 */
static struct WidgetEvents active_widget;
static struct WidgetData   active_widget_data;


void UITask(void)
{
    if (active_widget.on_task)
        active_widget.on_task();

    CtrlUpdate();
    if (CtrlIsUpPressed()) {
        if (active_widget.on_up)
            active_widget.on_up();
    } else if (CtrlIsDownPressed()) {
        if (active_widget.on_down)
            active_widget.on_down();
    } else if (CtrlIsBackPressed()) {
        if (active_widget.on_back)
            active_widget.on_back();
    } else if (CtrlIsOkPressed()) {
        if (active_widget.on_ok)
            active_widget.on_ok();
    }
}

/////////////////////////////////////////////////////////////////////////////////////
// MENU Widget
struct MenuItem {
    char Text[14];
    on_event_t on_event;
};

struct Menu {
    uint8_t items;
    uint8_t def_item;
    on_event_t on_back;

    struct MenuItem mitms[0];
};

void UI_Menu_Draw(const struct Menu *menu_p);
#define UI_MENU_POS     0
#define UI_MENU_ITEMS   1

void UI_Menu_EventUp(void)
{
    if (active_widget_data.data[UI_MENU_POS] < active_widget_data.data[UI_MENU_ITEMS] - 1) {
        ++active_widget_data.data[UI_MENU_POS];
        UI_Menu_Draw(active_widget_data.pointer);
    }

}
void UI_Menu_EventDown(void)
{
    if (active_widget_data.data[UI_MENU_POS] > 0) {
        --active_widget_data.data[UI_MENU_POS];
        UI_Menu_Draw(active_widget_data.pointer);
    }
}

void UI_Menu_EventOk(void)
{
    on_event_t event = (on_event_t)pgm_read_word(&((const struct Menu *)active_widget_data.pointer)->
            mitms[active_widget_data.data[UI_MENU_POS]].on_event);
    if (event)
        event();
}

void UI_Menu_EventBack(void)
{
    on_event_t event = (on_event_t)pgm_read_word(&((const struct Menu *)active_widget_data.pointer)->on_back);
    if (event)
        event();
}

#define UI_MIN(x,y)   ((x) > (y) ? (y) : (x))
void UI_Menu_Draw(const struct Menu *menu_p)
{
    int8_t start_pos = active_widget_data.data[UI_MENU_POS];
    if (start_pos + LCD_HEIGHT > (uint8_t)active_widget_data.data[UI_MENU_ITEMS]) {
        start_pos = (int8_t)active_widget_data.data[UI_MENU_ITEMS] - LCD_HEIGHT;
        if (start_pos < 0) {
            start_pos = 0;
        }
    }
    uint8_t i;
    LCDClear();
    for (i = 0; i < UI_MIN(LCD_HEIGHT, active_widget_data.data[UI_MENU_ITEMS]); i++) {
        LCDSetPos(i, 0);
        if (i + start_pos == active_widget_data.data[UI_MENU_POS]) {
            LCDPutChar('>');
        } else {
            LCDPutChar(' ');
        }
        uint8_t j = start_pos + i;
        LCDPutChar('1' + j);
        LCDPutChar(' ');
        LCDPuts_P(&menu_p->mitms[j].Text[0]);
    }
}

void UI_Menu_Enter(const struct Menu *menu_p)
{
    //Setup menu event handlers
    active_widget.on_back = UI_Menu_EventBack;
    active_widget.on_ok   = UI_Menu_EventOk;
    active_widget.on_up   = UI_Menu_EventUp;
    active_widget.on_down = UI_Menu_EventDown;

    active_widget.on_spin_up   = UI_Menu_EventUp;
    active_widget.on_spin_down = UI_Menu_EventDown;
    active_widget.on_spin_push = UI_Menu_EventOk;
    active_widget.on_task      = 0;

    //Cache values
    active_widget_data.pointer = (void*)menu_p;
    active_widget_data.data[UI_MENU_POS] = pgm_read_byte(&menu_p->def_item);
    active_widget_data.data[UI_MENU_ITEMS] = pgm_read_byte(&menu_p->items);

    //Draw Menu
    UI_Menu_Draw(menu_p);
}

///////////////////////////////////
// Simple message view UI clas

void UI_WaitForOk_EventBack(void)
{
    uint8_t pos = active_widget_data.data[UI_MENU_POS];
    UI_Menu_Enter((const struct Menu *)active_widget_data.pointer);
    active_widget_data.data[UI_MENU_POS] = pos;
    UI_Menu_Draw((const struct Menu *)active_widget_data.pointer);
}

void UI_WaitForOk_Enter(void)
{
    active_widget.on_back = UI_WaitForOk_EventBack;
    active_widget.on_ok   = UI_WaitForOk_EventBack;
    active_widget.on_up   = 0;
    active_widget.on_down = 0;

    active_widget.on_spin_up   = 0;
    active_widget.on_spin_down = 0;
    active_widget.on_spin_push = UI_WaitForOk_EventBack;

    active_widget.on_task      = 0;
}



///////////////////////////////////////////////////////////////////////////////////////
// User specific UI

struct MainMenu {
    struct Menu      menu;
    struct MenuItem  it_prog_tst;
    struct MenuItem  it_program;
    struct MenuItem  it_test;
    struct MenuItem  it_version;
    struct MenuItem  it_freqtest;
    struct MenuItem  it_interfaces;
    struct MenuItem  it_ext_test;
    struct MenuItem  it_all_test;
    struct MenuItem  it_ctboard;
};

void OnInfo(void)
{
    char err = ' ';
    uint8_t no = 0;
    LCDClear();
    if ((no = CTSetLed(1))) {
        err = 'L';
        goto io_error;
    }

    char *pd = CTHwi();
    if (!pd) {
        err = 'H';
        goto io_error;
    }

    LCDPutsBig(pd, 0);
    pd = CTVer();
    if (!pd) {
        err = 'V';
        goto io_error;
    }
    LCDPutsBig(pd, 2);
    UI_WaitForOk_Enter();
    return;

io_error:
    LCDSetPos(3,0);
    LCDPuts_P(PSTR("IO Error: "));
    LCDPutChar(err);
    LCDPutChar('0' + no);
    UI_WaitForOk_Enter();
}

//                  Maximum message      "0123456789012345"
static const char afe_ok[] PROGMEM     = "Ok";
static const char afe_no_mem[] PROGMEM = "No free memory";
static const char afe_isp[] PROGMEM    = "Can't enter ISP";
static const char afe_sig[] PROGMEM    = "Incorrect sign.";
static const char afe_unkn[] PROGMEM   = "Unknown error";

static const char afe_fprog[] PROGMEM  = "Failed flash";
static const char afe_fee[] PROGMEM    = "Failed eeprom";
static const char afe_ff[] PROGMEM     = "Failed fuses";

static const char afe_nofw[] PROGMEM   = "No firmwares";

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

int8_t CALLBACK_OnProgram(int8_t reason, int8_t progress)
{
    LCDSetPos(2,0);
    switch (reason & 0xF0) {
    case (HR_PROG_FLASH & 0xF0):   LCDPuts_P(PSTR("Write  ")); break;
    case (HR_VERIFY_FLASH & 0xF0): LCDPuts_P(PSTR("Verify ")); break;
    }
    switch (reason & 0x0F) {
    case (HR_PROG_FLASH & 0x0F):   LCDPuts_P(PSTR("FLASH ")); break;
    case (HR_PROG_EEPROM & 0x0F):  LCDPuts_P(PSTR("EEPROM")); break;
    case (HR_PORG_FUSES & 0x0F):   LCDPuts_P(PSTR("FUSES ")); break;
    }
    LCD_PrintU16(progress);
    LCDPutChar('%');
    return 0;
}

uint8_t ProgramClockTamer(void)
{
    LCDClear();
    struct FirmwareId* p = EDSSelectFirmware(0);
    LCDSetPos(0,0);
    LCDPuts_P(PSTR("Flashing..."));

    if (p) {
        LCDSetPos(1,0);
        LCDPuts(p->name);

        LCDSetPos(3,0);
        uint8_t res = EDSFlashFirmware(CALLBACK_OnProgram);
        DisplayError(res);

        return res;
    } else {
        LCDSetPos(3,0);
        LCDPuts_P(afe_nofw);

        return 1;
    }
}

void OnProgram(void)
{
    ProgramClockTamer();
    UI_WaitForOk_Enter();
}


#define FM_PREV        2
#define FM_DIV        (FM_PREV + 4)
#define FM_MES        (FM_DIV + 1)
#define FM_MODE_CNT   (FM_MES + 1)
#define FM_AVG_VALUE  (FM_MODE_CNT + 1)
#define FM_AVG_COUNT  (FM_AVG_VALUE + 1)

void LCD_PrintFreq(uint32_t freq)
{
    uint32_t start = 1000000000;
    uint8_t separator = 2;
    uint8_t started = 0;

    while (start > 0) {
        uint8_t digit = freq / start;
        if (!started && digit > 0) {
            started = 1;
        }
        if (started || start == 1) {
            LCDPutChar('0' + digit);
        } else {
            LCDPutChar(' ');
        }
        if (separator == 2 && start > 1) {
            LCDPutChar(started ? '\'' : ' ');
            separator = 0;
        } else {
            separator++;
        }

        freq -= start * digit;
        start /= 10;
    }
}

void LCD_PrintU16(uint16_t val)
{
    uint16_t start = 10000;
    uint8_t started = 0;

    while (start > 0) {
        uint8_t digit = val / start;
        if (!started && digit > 0) {
            started = 1;
        }
        if (started) {
            LCDPutChar('0' + digit);
        }

        val -= start * digit;
        start /= 10;
    }
    if (!started) {
        LCDPutChar('0');
    }
}

static void FrequencyMeterReset(void)
{
    FreqStopMeasure();
    LMKSetDiv(active_widget_data.data[FM_DIV]);
    active_widget_data.data[FM_MES] = 0;
    *((uint32_t*)&active_widget_data.data[FM_PREV]) = 0;
    active_widget_data.data[FM_AVG_COUNT] = 0;
    *((uint64_t*)&active_widget_data.data[FM_AVG_VALUE]) = 0;
    FreqStartMeasure(active_widget_data.data[FM_MODE_CNT]);
}

uint8_t FreqLogAvg(uint8_t val)
{
    uint8_t i = 8;
    for (;val !=0; val <<= 1, --i) {
        if (val & 0x80)
            return i;
    }
    return i;
}

static inline uint64_t uint64_shiftr_round (uint64_t v, uint8_t count)
{
    v = uint64_shiftr(v, count - 1);
    if (v & 1) {
        v++;
    }
    return uint64_sr(v);
}

uint8_t FrequencyMeter_CheckNew(void)
{
    uint8_t mes = FreqGetMes();
    uint8_t m =   active_widget_data.data[FM_MES];
    if (mes == m)
        return 0;

    active_widget_data.data[FM_MES] = mes;
    return 1;
}

void FrequencyMeter_DispFreq(uint32_t* pfreq, uint32_t* pdelta)
{
    uint32_t prev = *((uint32_t*)&active_widget_data.data[FM_PREV]);
    uint32_t cur = FreqGetTicks();

    int32_t delta = cur - prev;
    *((uint32_t*)&active_widget_data.data[FM_PREV]) = cur;

    //Show new value
    uint32_t freq = FreqCalculate(cur, active_widget_data.data[FM_DIV]);
    //LCDClear();
    LCDSetPos(1,1);
    LCD_PrintFreq(freq);
    if (pfreq) *pfreq = freq;

    LCDSetPos(2,0);
    uint8_t sign = 0;
    if (delta < 0) {
        delta = -delta;
        sign = 1;
        LCDPutChar('-');
    } else if (delta > 0) {
        LCDPutChar('+');
    } else {
        LCDPutChar(' ');
    }
    uint32_t dfreq = FreqCalculate((uint32_t)delta, active_widget_data.data[FM_DIV]);
    LCD_PrintFreq(dfreq);

    if (pdelta) *pdelta = dfreq;
}

void TASK_FrequencyMeter(void)
{
    if (!FrequencyMeter_CheckNew())
        return;

    uint32_t freq;
    FrequencyMeter_DispFreq(&freq, 0);

    if (active_widget_data.data[FM_MES] < 2)
        return;

    // Check for change DIV
    uint16_t p = (freq >> 21);  // 2mhz
    /*if (p == 0) {
        ++p;
    } else */
    if (p > 255) {
        p = 255;
    }

    if ((p < active_widget_data.data[FM_DIV] - 1) &&
            (p != active_widget_data.data[FM_DIV])) {
        goto __update_counter;
    } else if (p > active_widget_data.data[FM_DIV]) {
        goto __update_counter;
    }
#if 0
    // Calculate average value
    if ((uint32_t)delta < *((uint32_t*)&active_widget_data.data[FM_PREV]) >> 10) {
        uint8_t bits = FreqLogAvg(p);
        if (active_widget_data.data[FM_AVG_COUNT] == 0) {
            active_widget_data.data[FM_AVG_COUNT]++;

            *((uint64_t*)&active_widget_data.data[FM_AVG_VALUE]) =
                    uint64_shiftl(cur, bits);

            LCDSetPos(3,8);
            LCDPuts_P(PSTR("AD:"));
            LCD_PrintU16(bits);

        } else if (bits > 0) {
            if (active_widget_data.data[FM_AVG_COUNT] < 255)  {
                active_widget_data.data[FM_AVG_COUNT]++;
            }

            *((uint64_t*)&active_widget_data.data[FM_AVG_VALUE]) =
                    *((uint64_t*)&active_widget_data.data[FM_AVG_VALUE]) -
                    uint64_shiftr_round(*((uint64_t*)&active_widget_data.data[FM_AVG_VALUE]), bits)
                    //uint64_mul32(uint64_shiftr(*((uint64_t*)&active_widget_data.data[FM_AVG_VALUE]), bits),
                    //              ((uint16_t)1 << bits) - 1)
                    //uint64_shiftr(uint64_mul32(*((uint64_t*)&active_widget_data.data[FM_AVG_VALUE]),
                    //              ((uint16_t)1 << bits) - 1), bits)
                    + cur;


            freq = uint64_shiftr(FreqCalculate64(
                                    *((uint64_t*)&active_widget_data.data[FM_AVG_VALUE]),
                                    active_widget_data.data[FM_DIV]),
                                 bits);
            LCDSetPos(0,0);
            LCDPutChar('A');

            LCD_PrintFreq(freq);

        }
    } else {
        LCDSetPos(0,0);
        LCDPuts_P(PSTR("--------------  "));

        active_widget_data.data[FM_AVG_COUNT] = 0;
    }
#endif
    return;
__update_counter:
    LCDSetPos(3,0);
    LCDPuts_P(PSTR("DIV:   "));
    LCDSetPos(3,4);
    LCD_PrintU16(p);

    active_widget_data.data[FM_DIV] = p;
    FrequencyMeterReset();

    LCDSetPos(0,0);
    LCDPuts_P(PSTR("--------------  "));
   /* FreqStopMeasure();

    LMKSetDiv(p);
    active_widget_data.data[FM_DIV] = p;
    active_widget_data.data[FM_MES] = 0;
    *((uint32_t*)&active_widget_data.data[FM_PREV]) = 0;

    FreqStartMeasure(active_widget_data.data[FM_MODE_CNT]);
    */
}

void BACK_FrequencyMeter(void)
{
    FreqStopMeasure();
    LMKEnable(0);

    UI_WaitForOk_EventBack();
}

void UP_FrequencyMeter(void)
{
    if (active_widget_data.data[FM_MODE_CNT] < FMT_PRECISE) {
        ++active_widget_data.data[FM_MODE_CNT];
        FrequencyMeterReset();
        /*
        FreqStopMeasure();
        active_widget_data.data[FM_MES] = 0;
        FreqStartMeasure(active_widget_data.data[FM_MODE_CNT]);
        *((uint32_t*)&active_widget_data.data[FM_PREV]) = 0;
        */
    }
}
void DOWN_FrequencyMeter(void)
{
    if (active_widget_data.data[FM_MODE_CNT] > FMT_COARSE) {
        --active_widget_data.data[FM_MODE_CNT];
        FrequencyMeterReset();
        /*
        FreqStopMeasure();
        active_widget_data.data[FM_MES] = 0;
        FreqStartMeasure(active_widget_data.data[FM_MODE_CNT]);
        *((uint32_t*)&active_widget_data.data[FM_PREV]) = 0;
        */
    }
}

void OnFrequencyMeter(void)
{
    active_widget.on_back = BACK_FrequencyMeter;
    active_widget.on_ok   = BACK_FrequencyMeter;
    active_widget.on_up   = UP_FrequencyMeter;
    active_widget.on_down = DOWN_FrequencyMeter;
    active_widget.on_spin_up   = 0;
    active_widget.on_spin_down = 0;
    active_widget.on_spin_push = BACK_FrequencyMeter;
    active_widget.on_task = TASK_FrequencyMeter;

    LCDClear();
    LCDSetPos(0, 0);
    LCDPuts_P(PSTR("Calculating..."));

    LMKEnable(1);
    //LMKSetInput(0);
    //active_widget_data.data[FM_MES] = 0;
    active_widget_data.data[FM_MODE_CNT] = FMT_NORMAL;
    active_widget_data.data[FM_DIV] = 255;

    //*((uint32_t*)&active_widget_data.data[FM_PREV]) = 0;
    //LMKSetDiv(active_widget_data.data[FM_DIV]); //The most available divider
    //FreqStartMeasure(active_widget_data.data[FM_MODE_CNT]);

    FrequencyMeterReset();
}

//////////////////////////////////////////////////////////////////////////
// UI Test Menu
//////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////
// Code for selftesting
////////////////////////////////////////////////////////////

void CALLBACK_CTOnSelfTestEvent(uint8_t type, uint32_t value, uint8_t res)
{
    switch (type) {
    case SFT_PIN_LOCK:
        LCDSetPos(3, 0);
        LCDPutChar('P');
        LCDPutChar(' ');
        goto print_status;
        break;
    case SFT_SET_FREQ:
        LCDSetPos(2, 0);
        LCDPuts_P(PSTR("C-------------"));
        LCDSetPos(1, 0);
        LCDPutChar('F');
        LCD_PrintFreq(value);
        goto print_status;
        break;
    case SFT_COUNTED:
        LCDSetPos(2, 1);
        LCD_PrintFreq(value);

print_status:
        if (res == CTR_OK) {
            LCDPutChar('O'); LCDPutChar('K');
        } else if (res == CTR_FAILED){
            LCDPutChar('#'); LCDPutChar('#');
        } else {
            LCDPutChar('?'); LCDPutChar('?');
        }
        break;

    }
}

extern char g_ctrecvbuffer[];

uint8_t TestSelf(void)
{
    LCDClear();

    uint16_t ret = CTSelfTest(CALLBACK_CTOnSelfTestEvent);
    uint8_t res = ret & 0xFF;
    LCDSetPos(3, 0);
    if (res == CTR_FAILED) {
        LCDPuts_P(PSTR("SELF FAILED: "));
        LCD_PrintU16(ret >> 8);
    } else if (res == CTR_OK) {
        LCDPuts_P(PSTR("*SELF PASSED*"));
        return 0;
    } else if (res == CTR_IO_ERROR) {
        LCDPuts_P(PSTR("IO ERROR"));
    } else {
        LCDPuts_P(PSTR("UNKNOWN ERROR: "));
        LCDPutChar('0' + res);

        LCDPutsBig(g_ctrecvbuffer, 0);
    }

    return 1;
}

void OnTest(void)
{
    TestSelf();
    UI_WaitForOk_Enter();
}

////////////////////////////////////////////////////////////
// Code for Interface testing
////////////////////////////////////////////////////////////
const char sp_clocktamer[] PROGMEM = "ClockTamer";

uint8_t TST_CheckVer(void)
{
    char *data;
    data = CTVer();
    if (!data) {
        LCDPuts_P(PSTR("EIO"));
    } else if (strncmp_P(data, sp_clocktamer, sizeof(sp_clocktamer) - 1)) {
        LCDPuts_P(PSTR("EDAT"));
    } else {
        LCDPuts_P(PSTR("Ok"));
        return 0;
    }

    return 1;
}

uint8_t TST_USBHost(uint8_t pipes)
{
    switch (USBCommState) {
    case USBH_MODE_CDC:
    case USBH_MODE_DFU:
        LCDPuts_P(PSTR("ETO"));
        break;
    case USBH_ERR_ENUM_FAILED:    LCDPuts_P(PSTR("EENM"));       break;
    case USBH_ERR_CONF_DISCRIPTOR:LCDPuts_P(PSTR("ECND"));       break;
    case USBH_ERR_CONF_PIPES:     LCDPuts_P(PSTR("ECNP"));       break;
    case USBH_ERR_CONF_DEVICE:    LCDPuts_P(PSTR("ECNE"));       break;
    case USBH_ERR_CDC_SETLINE:    LCDPuts_P(PSTR("ESL"));        break;
    case USBH_CONFIGURED_CDC:
        if (pipes == USBH_MODE_CDC)
            return 0;
        LCDPuts_P(PSTR("Pdfu"));
        break;
    case USBH_CONFIGURED_DFU:
        if (pipes == USBH_MODE_DFU)
            return 0;
        LCDPuts_P(PSTR("Pcdc"));
        break;
    }

    return 1;
}

void TST_InitUSB(uint8_t mode)
{
    USBCommState = mode;
    USB_Init(USB_MODE_Host);
    _delay_ms(100);
    CTPower(1 << CTM_USB);
    CTInit(CTM_USB);
    //sei();

    clocktamer_reset();

    uint8_t i;
    for (i = 0; i < 100; i++) {
        if (USBCommState != mode) {
            return;
        }
        uint16_t j;
        for (j = 0; j < 1000; j++) {
            if (mode == USBH_MODE_CDC) {
                USB_ExtraHost();
            }
            USB_USBTask();
            _delay_us(100);
        }
    }

}

uint8_t TST_DFU(void)
{
    uint8_t cnt = 5;
    for(;cnt!=0;cnt--) {
    char state = 0xff;

    USB_ControlRequest.bmRequestType = REQDIR_DEVICETOHOST | (REQTYPE_CLASS | REQREC_INTERFACE);
    USB_ControlRequest.bRequest = 0x05; //DFU_GETSTATE
    USB_ControlRequest.wValue = 0;
    USB_ControlRequest.wIndex = 0;
    USB_ControlRequest.wLength = 1;

    switch (USB_Host_SendControlRequest(&state)) {
    case HOST_SENDCONTROL_Successful:
        if (state == 2) { /* DFU IDLE */
            LCDPuts_P(PSTR("Ok"));
            return 0;
        }
        LCDPuts_P(PSTR("Est"));
        break;
    case HOST_SENDCONTROL_DeviceDisconnected:
        LCDPuts_P(PSTR("EDIS"));
        break;
    case HOST_SENDCONTROL_PipeError:
        LCDPuts_P(PSTR("EPIP"));
        break;
    case HOST_SENDCONTROL_SetupStalled:
        LCDPuts_P(PSTR("ESTL"));
        break;
    case HOST_SENDCONTROL_SoftwareTimeOut:
        if (cnt > 1)
            continue;

        LCDPuts_P(PSTR("ESTO"));
        break;
    }
    return 1;
    }

    // Should not be here
    return 1;
}

uint8_t TestProcedureInterfaces(void)
{
    uint8_t err = 0;

    CTPower(0);
    USB_USBTask();
    USB_Host_VBUS_Manual_Off();
    USB_Host_VBUS_Auto_Off();
    USB_Disable();

    LCDClear();
    _delay_ms(500);

    // Check SPI
    LCDSetPos(1, 0);
    LCDPuts_P(PSTR("Testing SPI "));
    CTPower(1 << CTM_SPI);
    CTInit(CTM_SPI);
    _delay_ms(1000);

    err = TST_CheckVer();

    // Check USB
    LCDSetPos(2, 0);
    LCDPuts_P(PSTR("Testing USB "));

    TST_InitUSB(USBH_MODE_CDC);
    if (!TST_USBHost(USBH_MODE_CDC)) {
        err += TST_CheckVer();
    } else {
        err++;
    }

    CTPower(0);
    USB_USBTask();
    USB_Host_VBUS_Manual_Off();
    USB_Host_VBUS_Auto_Off();
    USB_Disable();

    clocktamer_dfubit_set();
    _delay_ms(1000);

    // Check Bootloader
    LCDSetPos(3, 0);
    LCDPuts_P(PSTR("Testing DFU "));

    TST_InitUSB(USBH_MODE_DFU);
    if (!TST_USBHost(USBH_MODE_DFU)) {
        // Test control request
        err += TST_DFU();
    } else {
        err++;
    }

    clocktamer_dfubit_clear();
    CTPower(0);
    USB_USBTask();
    USB_Host_VBUS_Manual_Off();
    USB_Host_VBUS_Auto_Off();
    USB_Disable();

    _delay_ms(1000);

    // Rolling back
    USB_Disable();
    CTInit(CTM_AUTO);
    CTPower((1 << CTM_USB) | (1 << CTM_SPI));
    _delay_ms(100);
    USB_Init(USB_MODE_UID);

    return err;
}


void OnTestInterfaces(void)
{
    TestProcedureInterfaces();
    UI_WaitForOk_Enter();
}


////////////////////////////////////////////////////////////
// Code for external frequency testing
////////////////////////////////////////////////////////////
void PrintClockTamerError(uint8_t err)
{
    switch (err) {
    case CTR_OK:                LCDPuts_P(PSTR(" OK "));  break;
    case CTR_IO_ERROR:          LCDPuts_P(PSTR("E_io"));  break;
    case CTR_SYNTAX_ERROR:      LCDPuts_P(PSTR("Estx"));  break;
    case CTR_CMD_ERROR:         LCDPuts_P(PSTR("Ecmd"));  break;
    case CTR_FAILED:            LCDPuts_P(PSTR("Efld"));  break;
    case CTR_BAD_TUNING_RANGE:  LCDPuts_P(PSTR("Ebtr"));  break;
    case CTR_CANT_TUNE:         LCDPuts_P(PSTR("Ecnt"));  break;
    case CTR_INCORRECT_REPLY:   LCDPuts_P(PSTR("Einc"));  break;
    default:                    LCDPuts_P(PSTR("Eunk"));  break;
    }
}
void TestProcedureTestExtFreq_GetFreqs(uint32_t* min, uint32_t* max)
{
    uint32_t freq_min = (uint32_t)(-1);
    uint32_t freq_max = 0;

    //Collect stat for 10 seconds
    uint8_t i;
    for (i = 0; i < 200; i++) {
        _delay_ms(100);
        if (!FrequencyMeter_CheckNew())
            continue;

        uint32_t freq;
        FrequencyMeter_DispFreq(&freq, 0);

        if (active_widget_data.data[FM_MES] < 2)
            continue;

        if (freq < freq_min) {
            freq_min = freq;
        }
        if (freq_max < freq) {
            freq_max = freq;
        }

        //print progress
        LCDSetPos(3,1);
        LCDPuts_P(PSTR("Progress: "));
        LCD_PrintU16(i>>1);
        LCDPutChar('%');
    }

    *min = freq_min;
    *max = freq_max;
}

uint8_t TestProcedureTestExtFreq(uint32_t freq)
{
    uint8_t err = 0;
    uint8_t ret;

    ret = CTSetOutput(freq);
    if (ret != CTR_OK)
        goto print_eror_and_exit;

    //Print header
    LCDClear();
    LCDSetPos(0,0);
    LCDPuts_P(PSTR("SET"));
    LCD_PrintFreq(freq);

    //Wait to setle frequency
    _delay_ms(100);


    active_widget_data.data[FM_MODE_CNT] = FMT_NORMAL;
    active_widget_data.data[FM_DIV]      = 255;
    uint8_t j;

    LCDSetPos(3,0);
    LCDPutChar('A');

    for (j = 0; j < 2; j++) {
        //Initialize counter
        LMKEnable(1);
        _delay_us(100);
        LCDSetPos(1,14); LCDPutChar(' '); LCDPutChar(' ');
        LCDSetPos(2,14); LCDPutChar(' '); LCDPutChar(' ');
        FrequencyMeterReset();

        uint32_t freq_min, freq_max;
        TestProcedureTestExtFreq_GetFreqs(&freq_min, &freq_max);

        uint32_t delta;
        uint16_t reldelta;

        delta = freq_max - freq_min;
        if (delta > freq) {
            reldelta = 9999;
        } else {
            reldelta = (delta * 1000) / freq_min;
        }

        FreqStopMeasure();
        LMKEnable(0);

        LCDSetPos(1,0);
        LCDPuts_P(PSTR("MAX"));
        LCD_PrintFreq(freq_max);
        LCDSetPos(2,0);
        LCDPuts_P(PSTR("MIN"));
        LCD_PrintFreq(freq_min);

        LCDSetPos(3,0);
        LCDPuts_P(PSTR("R: "));
        LCD_PrintU16(reldelta);

        if (freq > freq_max) {
            freq_max = freq;
        }
        if (freq < freq_min) {
            freq_max = freq;
        }
        delta = freq_max - freq_min;
        uint16_t relsdelta;
        if (delta > freq)  {
            relsdelta = 9999;
        } else {
            relsdelta = (delta * 1000) / freq_min;
        }

        LCDPuts_P(PSTR(" Q: "));
        LCD_PrintU16(relsdelta);

        LCDPutChar(' ');
        LCDPutChar(' ');

        if (!((reldelta < 9) && (relsdelta < 9))) {
            goto measure_error;
        }

        // Set optimized devider and count more precise

        LCDSetPos(3,0);
        LCDPutChar('B');
        uint16_t p = (freq_max >> 21);  // 2mhz
        if (p > 255) {
            p = 255;
        }
        active_widget_data.data[FM_DIV]      = p;


        USB_ExtraHost();
        USB_USBTask();
    }

    uint16_t tmp;
    for (tmp = 0; tmp < 20000; tmp++) {
        _delay_us(90);

        USB_ExtraHost();
        USB_USBTask();
    }
    return 0;

measure_error:
    return 100;

print_eror_and_exit:
    PrintClockTamerError(err);
    return 200;
}

uint32_t TestProcedure_freqs[] PROGMEM = {
    10000000,
    19600000,
    20480000,
    30770000,
    38200000,
    40960000,
    52000000,
    64000000,
    92000000
};


void DoPause(void)
{
    _delay_ms(100);

    uint8_t i;
    for (i = 0; i < 255; i++) {
        USB_ExtraHost();
        USB_USBTask();
    }
}


uint8_t TestProcedureAllTest(void)
{
    uint8_t i;
    for (i = 0; i < sizeof(TestProcedure_freqs) / sizeof(uint32_t); i++) {
        uint32_t freq = pgm_read_dword((uint32_t*)TestProcedure_freqs + i);
        uint8_t  ret = TestProcedureTestExtFreq(freq);

        if (ret) {
            return ret;
        }
        DoPause();
    }
    return 0;
}

void OnExtFreq(void)
{
    TestProcedureAllTest();
    UI_WaitForOk_Enter();
}



void OnAverallCTTest(void)
{
    uint8_t res;

    // Interface test
    res = TestProcedureInterfaces();
    if (res) goto testing_end;

    DoPause();

    // Selftesting
    res = TestSelf();
    if (res) goto testing_end;

    DoPause();

    // External testing
    res = TestProcedureAllTest();
    if (res) goto testing_end;

    // Everything is ok
    LCDClear();
    LCDSetPos(0,0);
    LCDPuts_P(PSTR("Testing done: OK"));

testing_end:
    UI_WaitForOk_Enter();
}

void OnProgramAndTest(void)
{
    uint8_t res = ProgramClockTamer();
    if (res) goto program_end;

    // Reset ClockTamer
    clocktamer_reset();

    DoPause();

    OnAverallCTTest();
    return;

program_end:
    UI_WaitForOk_Enter();
}

/////////////////////////////////////////////////////////
// UI CT BOARD
/////////////////////////////////////////////////////////

enum CTBRDState {
    CTBR_NONE = 0,
    CTBR_DORDER1,// = 0x11,
    CTBR_DORDER2,// = 0x12,
    CTBR_DORDER3,// = 0x13,
    CTBR_DORDER4,// = 0x14,
    CTBR_DORDER5,// = 0x15,
    CTBR_DORDER6,// = 0x16,
    CTBR_DORDER7,// = 0x17,
    CTBR_DORDER8,// = 0x18,
    CTBR_DORDER9,// = 0x19,
    CTBR_OUT0,// = 0x20,
    CTBR_OUT1,// = 0x21,
    CTBR_OUT3,// = 0x23,
    CTBR_OUT4,// = 0x24,
    CTBR_OUT5,// = 0x25,
    CTBR_OUT6,// = 0x26,
    CTBR_OUT7,// = 0x27,
    CTBR_SET,//  = 0x30,
    CTBR_SAVE// = 0x40
};

#define CTBR_M_STATE        2
#define CTBR_M_FREQ         3
#define CTBR_M_OUT          7

static void CTBR_UpdateUI(void)
{
    LCDClear();
    LCDSetPos(0,1);
    LCD_PrintFreq(*(uint32_t*)(&active_widget_data.data[CTBR_M_FREQ]));

    LCDSetPos(2,0);
    uint8_t mout = active_widget_data.data[CTBR_M_OUT];
    uint8_t pos;
    for (pos = 1; pos != 0x00; pos<<=1) {
        if (pos == 0x4) {
            LCDPutChar('x');
        } else {
            LCDPutChar((mout & pos) ? '1' : '0');
        }
        LCDPutChar(' ');
    }

    LCDSetPos(3,0);
    LCDPuts_P(PSTR(" Set  Save"));

    switch (active_widget_data.data[CTBR_M_STATE]) {
    case CTBR_NONE:
        return;
    case CTBR_SET:  LCDSetPos(3,0); LCDPutChar('>'); break;
    case CTBR_SAVE: LCDSetPos(3,5); LCDPutChar('>'); break;
    default:
        if (active_widget_data.data[CTBR_M_STATE] < CTBR_OUT0) {
            uint8_t dig = active_widget_data.data[CTBR_M_STATE] - CTBR_DORDER1;
            if (dig > 5) {
                dig += 2;
            } else if (dig > 2) {
                dig ++;
            }
            LCDSetPos(1, 13 - dig); LCDPutChar('^'); break;
        } else {
            pos = active_widget_data.data[CTBR_M_STATE] - CTBR_OUT0;
            if (pos > 1) ++pos;
            pos <<= 1;
            LCDSetPos(1, pos); LCDPutChar('_'); break;
        }
    }
}

void UP_OnCTBRUp(void)
{
    if (++active_widget_data.data[CTBR_M_STATE] > CTBR_SAVE)
        active_widget_data.data[CTBR_M_STATE] = CTBR_NONE;

    CTBR_UpdateUI();
}

void DOWN_OnCTBRDown(void)
{
    if (active_widget_data.data[CTBR_M_STATE]-- == CTBR_NONE)
        active_widget_data.data[CTBR_M_STATE] = CTBR_SAVE;

    CTBR_UpdateUI();
}

static uint32_t Order10(uint8_t o)
{
    uint32_t res = 1;
    while (--o) {
        res *= 10;
    }
    return res;
}

static void CTBR_UpdateOuts(void)
{
    uint8_t bit = active_widget_data.data[CTBR_M_STATE] - CTBR_OUT0;
    if (bit > 1) ++bit;
    active_widget_data.data[CTBR_M_OUT] ^= (1 << bit);

    LCDSetPos(3, 11);
    PrintClockTamerError(CTStoreToEEPROM());
}

void OK_OnCTBROk(void)
{
    switch (active_widget_data.data[CTBR_M_STATE]) {
    case CTBR_NONE:
        return;
    case CTBR_SET:
        LCDSetPos(3, 11);
        PrintClockTamerError(CTEnableOutputs(active_widget_data.data[CTBR_M_OUT]));
        PrintClockTamerError(CTSetOutput(*(uint32_t*)(&active_widget_data.data[CTBR_M_FREQ])));
        return;
    case CTBR_SAVE:
        LCDSetPos(3, 11);
        PrintClockTamerError(CTStoreToEEPROM());
        return;
    }

    if (active_widget_data.data[CTBR_M_STATE] < CTBR_OUT0) {
        uint32_t orv = Order10(active_widget_data.data[CTBR_M_STATE] - CTBR_NONE);
        uint8_t digit = (*(uint32_t*)(&active_widget_data.data[CTBR_M_FREQ]) / orv) % 10;
        if (digit == 9) {
            *(uint32_t*)(&active_widget_data.data[CTBR_M_FREQ]) -= 9*orv;
        } else {
            *(uint32_t*)(&active_widget_data.data[CTBR_M_FREQ]) += orv;
        }
    } else {
        CTBR_UpdateOuts();
    }
    CTBR_UpdateUI();
}


void BACK_OnCTBRBack(void)
{
    switch (active_widget_data.data[CTBR_M_STATE]) {
    case CTBR_NONE:
        UI_WaitForOk_EventBack();
        return;
    case CTBR_SET:
    case CTBR_SAVE:
        active_widget_data.data[CTBR_M_STATE] = CTBR_NONE;
        CTBR_UpdateUI();
        return;
    }

    if (active_widget_data.data[CTBR_M_STATE] < CTBR_OUT0) {
        uint32_t orv = Order10(active_widget_data.data[CTBR_M_STATE] - CTBR_NONE);
        uint8_t digit = (*(uint32_t*)(&active_widget_data.data[CTBR_M_FREQ]) / orv) % 10;
        if (digit == 0) {
            *(uint32_t*)(&active_widget_data.data[CTBR_M_FREQ]) += 9*orv;
        } else {
            *(uint32_t*)(&active_widget_data.data[CTBR_M_FREQ]) -= orv;
        }
    } else {
        CTBR_UpdateOuts();
    }
    CTBR_UpdateUI();
}

void OnClockTamerFreqset(void)
{
    active_widget.on_back = BACK_OnCTBRBack;
    active_widget.on_ok   = OK_OnCTBROk;
    active_widget.on_up   = UP_OnCTBRUp;
    active_widget.on_down = DOWN_OnCTBRDown;
    active_widget.on_spin_up   = 0;
    active_widget.on_spin_down = 0;
    active_widget.on_spin_push = 0;
    active_widget.on_task = 0;

    LCDClear();
    LCDSetPos(0,0);
    active_widget_data.data[CTBR_M_STATE] = CTBR_NONE;
    uint8_t res = CTGetOutput((uint32_t*)&active_widget_data.data[CTBR_M_FREQ]);
    if (res) {
        goto _on_error;
    }

    res = CTGetPorts(&active_widget_data.data[CTBR_M_OUT]);
    if (res) {
        goto _on_error;
    }

    CTBR_UpdateUI();
    return;

_on_error:
    LCDPuts_P(PSTR("Error: "));
    PrintClockTamerError(res);
    UI_WaitForOk_Enter();
    return;
}

static struct MainMenu sp_main_menu PROGMEM = {
    .menu = { .items = 9, .def_item = 0, .on_back = 0 },
    .it_prog_tst = { .Text = "Program&Test", .on_event = OnProgramAndTest},
    .it_program =  { .Text = "Only Program", .on_event = OnProgram},
    .it_test    =  { .Text = "SelfTest",     .on_event = OnTest},
    .it_version =  { .Text = "Info",         .on_event = OnInfo},
    .it_freqtest=  { .Text = "Freq Meter",   .on_event = OnFrequencyMeter},
    .it_interfaces={ .Text = "Iface test",   .on_event = OnTestInterfaces},
    .it_ext_test = { .Text = "Ext Freq",     .on_event = OnExtFreq},
    .it_all_test = { .Text = "All tests",    .on_event = OnAverallCTTest},
    .it_ctboard =  { .Text = "CT Board",     .on_event = OnClockTamerFreqset},
};


void UIStart(void)
{
    UI_Menu_Enter(&sp_main_menu.menu);
}
