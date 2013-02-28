#ifndef LEDS_H
#define LEDS_H

#include "Config/PortConfig.h"

#define  LEDS_LED1       (1 << LED_1)
#define  LEDS_LED2       (1 << LED_2)
#define  LEDS_LED3       (1 << LED_3)
#define  LEDS_LED4       (1 << LED_4)


#define LEDs_Init()     LED_DDR |= (LEDS_LED1 | LEDS_LED2 | LEDS_LED3 | LEDS_LED4)
#define LEDs_Disable()  LED_DDR &= (LEDS_LED1 | LEDS_LED2 | LEDS_LED3 | LEDS_LED4)

#define LEDs_ClearAllLEDs(x)  LED_PORT &= (x)
#define LEDs_SetAllLEDs(x)    LED_PORT |= (x)
#define LEDs_ToggleLEDs(x)    LED_PORT ^= (x)


#endif // LEDS_H
