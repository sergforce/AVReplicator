#ifndef UI_TEXT_H
#define UI_TEXT_H

#include "VirtualSerialHost.h"
#include "spiio.h"
#include "ctcontrol.h"
#include "lcd_text.h"
#include "control.h"

/**
 * @file File contsning User interface
 */


/**
 * @brief UITask function process user interaction.
 *
 * Should be called periodically
 */
void UITask(void);

void UIStart(void);
#endif // UI_TEXT_H
