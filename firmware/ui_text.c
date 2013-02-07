#include "ui_text.h"
#include <avr/pgmspace.h>
#include "datast.h"

#include "lmk0x0xx.h"
#include "freqmes.h"

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
    uint8_t data[8];
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
    char Text[12];
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
    struct MenuItem  it_program;
    struct MenuItem  it_test;
    struct MenuItem  it_version;
    struct MenuItem  it_freqtest;
    struct MenuItem  it_ctboard;
};

void OnInfo(void)
{
    LCDClear();
    if (!CTSetLed(1)) {
        goto io_error;
    }

    char *pd = CTHwi();
    if (!pd) {
        goto io_error;
    }

    LCDPutsBig(pd, 0);
    pd = CTVer();
    if (!pd) {
        goto io_error;
    }
    LCDPutsBig(pd, 2);
    UI_WaitForOk_Enter();
    return;

io_error:
    LCDSetPos(3,0);
    LCDPuts_P(PSTR("IO Error"));
    UI_WaitForOk_Enter();
}

void DisplayError(uint8_t code);
extern char afe_nofw[] PROGMEM;

void OnProgram(void)
{
    LCDSetPos(3,0);
    struct FirmwareId* p = EDSSelectFirmware(0);
    if (p) {
        DisplayError(EDSFlashFirmware(0));
    } else {
        LCDPuts_P(afe_nofw);
    }
    UI_WaitForOk_Enter();
}


#define FM_PREV   2
#define FM_DIV    (FM_PREV + 4)
#define FM_MES    (FM_DIV + 1)

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
        if (started) {
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

void TASK_FrequencyMeter(void)
{
    uint8_t mes = FreqGetMes();
    uint8_t m =   active_widget_data.data[FM_MES];
    if (mes == m)
        return;

    active_widget_data.data[FM_MES] = mes;

    uint32_t prev = *((uint32_t*)&active_widget_data.data[FM_PREV]);
    uint32_t cur = FreqGetTicks();

    ////////LCDSetPos(1,1);
    ////////LCD_PrintFreq(cur);
    ////////LCDSetPos(3,1);
    ////////LCD_PrintFreq(prev);
    ///////*((uint32_t*)&active_widget_data.data[FM_PREV]) = cur;
    /////// return;

    //if (prev == cur)
    //    return;

    int32_t delta = cur - prev;
    *((uint32_t*)&active_widget_data.data[FM_PREV]) = cur;


    //Show new value
    uint32_t freq = FreqCalculate(cur, active_widget_data.data[FM_DIV]);
    //LCDClear();
    LCDSetPos(1,1);
    LCD_PrintFreq(freq);

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
    freq = FreqCalculate((uint32_t)delta, active_widget_data.data[FM_DIV]);
    LCD_PrintFreq(freq);
}

void BACK_FrequencyMeter(void)
{
    FreqStopMeasure();
    LMKEnable(0);

    UI_WaitForOk_EventBack();
}

void OnFrequencyMeter(void)
{
    active_widget.on_back = BACK_FrequencyMeter;
    active_widget.on_ok   = BACK_FrequencyMeter;
    active_widget.on_up   = 0;
    active_widget.on_down = 0;
    active_widget.on_spin_up   = 0;
    active_widget.on_spin_down = 0;
    active_widget.on_spin_push = BACK_FrequencyMeter;
    active_widget.on_task = TASK_FrequencyMeter;

    LCDClear();
    LCDSetPos(0, 0);
    LCDPuts_P(PSTR("Calculating..."));

    LMKEnable(1);
    //LMKSetInput(0);

    *((uint32_t*)&active_widget_data.data[FM_PREV]) = 0;
    active_widget_data.data[FM_DIV] = 255;
    LMKSetDiv(active_widget_data.data[FM_DIV]); //The most available divider

    active_widget_data.data[FM_MES] = 0;
    FreqStartMeasure(FMT_NORMAL);
}

static struct MainMenu sp_main_menu PROGMEM = {
    .menu = { .items = 4, .def_item = 0, .on_back = 0 },
    .it_program = { .Text = "Program",    .on_event = OnProgram},
    .it_test    = { .Text = "Test",       .on_event = 0},
    .it_version = { .Text = "Info",       .on_event = OnInfo},
    .it_freqtest= { .Text = "Freq Meter", .on_event = OnFrequencyMeter},
    .it_ctboard = { .Text = "CT Board",   .on_event = 0}
};


void UIStart(void)
{
    UI_Menu_Enter(&sp_main_menu.menu);
}
