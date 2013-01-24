#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

#include <PortConfig.h>
#include "control.h"

#define BT_COUNT  4
static struct ButtonState {
    uint8_t mask;    /**< Bit mask for soft trigger */
    uint8_t pressed; /**< 7th bit -- button down flag; 0:6 bits -- Number of registered events */
} s_btstate[BT_COUNT];

#define BT_UP   0
#define BT_DOWN 1
#define BT_OK   2
#define BT_BACK 3

// 6 same bits following each other
#define MASK_SEARCH 0x3f

#define CTRL_MASK  ((1 << CTRL_BTUP) | (1 << CTRL_BTDOWN) | (1 << CTRL_BTOK) | (1 << CTRL_BTBACK))
void CtrlInit(void)
{
    CTRL_DDR &= ~CTRL_MASK;
    CTRL_PORT |= CTRL_MASK; //Pull-up enable

    uint8_t i;
    for (i = 0; i < BT_COUNT; i++) {
        s_btstate[i].mask = 0;
        s_btstate[i].pressed = 0;
    }
}

void CtrlUpdate(void)
{
    uint8_t i;
    // Update mask
    for (i = 0; i < BT_COUNT; i++) {
        s_btstate[i].mask <<= 1;
    }
    if (CTRL_PIN & (1 << CTRL_BTUP))   s_btstate[BT_UP].mask |= 1;
    if (CTRL_PIN & (1 << CTRL_BTDOWN)) s_btstate[BT_DOWN].mask |= 1;
    if (CTRL_PIN & (1 << CTRL_BTOK))   s_btstate[BT_OK].mask |= 1;
    if (CTRL_PIN & (1 << CTRL_BTBACK)) s_btstate[BT_BACK].mask |= 1;

    // Detect events
    for (i = 0; i < BT_COUNT; i++) {
        if (s_btstate[i].pressed & 0x80) {
            // Now in pressed state, find release event
            if ((s_btstate[i].mask & MASK_SEARCH) == 0) {
                s_btstate[i].pressed ++;      // add event
                s_btstate[i].pressed &= 0x7f; // flush mask
            }
        } else {
            // In released event, fins press event
            if ((s_btstate[i].mask & MASK_SEARCH) == MASK_SEARCH) {
                s_btstate[i].pressed |= 0x80; // flush mask and add event
            }
        }
    }
}

void CtrlClear(void)
{
    uint8_t i;
    for (i = 0; i < BT_COUNT; i++) {
        s_btstate[i].pressed = 0;
    }
}

static uint8_t CtrlIsPressed(uint8_t bt)
{
    uint8_t cnt = s_btstate[bt].pressed & 0x7f;
    if (cnt > 0) {
        s_btstate[bt].pressed--;
    }
    return cnt;
}

uint8_t CtrlIsUpPressed(void)
{
    return CtrlIsPressed(BT_UP);
}

uint8_t CtrlIsDownPressed(void)
{
    return CtrlIsPressed(BT_DOWN);
}

uint8_t CtrlIsOkPressed(void)
{
    return CtrlIsPressed(BT_OK);
}

uint8_t CtrlIsBackPressed(void)
{
    return CtrlIsPressed(BT_BACK);
}
