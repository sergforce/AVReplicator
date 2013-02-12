#include <avr/io.h>
#include <util/delay.h>

#include <PortConfig.h>
#include "lmk0x0xx.h"


#define LMK0X0XX_WRITE(x)  write_reg_LMK0X0XX( \
  (uint8_t)((x) >> 24), \
  (uint8_t)((x) >> 16), \
  (uint8_t)((x) >> 8), \
  (uint8_t)((x)))


static void MicrowireLatchLMK0X0XX()
{
    MW_PORT |= (1 << MW_LE_LMK0X0XX);
    MW_PORT |= (1 << MW_LE_LMK0X0XX);
   //_delay_us(1);
    MW_PORT &= ~(1 << MW_LE_LMK0X0XX);
    MW_PORT &= ~(1 << MW_LE_LMK0X0XX);
   //_delay_us(1);
}



static void MicrowireWriteByte(uint8_t data)
{
   uint8_t i;

   for (i = 8; i != 0; i--)
   {
      MW_PORT &=  ~(1 << MW_CLK);  //Toggle CLK low

      // Use this because of perfomance impact on an embedded device
      if ((data & 0x80) != 0)
         MW_PORT |=  (1 << MW_DATA);   //Set DATA bit
      else
         MW_PORT &=  ~(1 << MW_DATA);  //Reset DATA bit

      data <<= 1;

      MW_PORT |=  (1 << MW_CLK);  //Toggle CLK high
   }

   MW_PORT &=  ~(1 << MW_DATA);
   MW_PORT &=  ~(1 << MW_CLK);
}


static void write_reg_LMK0X0XX(uint8_t f1, uint8_t f2, uint8_t f3, uint8_t f4)
{
        MicrowireWriteByte(f1);
        MicrowireWriteByte(f2);
        MicrowireWriteByte(f3);
        MicrowireWriteByte(f4);
        MicrowireLatchLMK0X0XX();
}


void LMKInit(void)
{
    MW_DDR |=  (1 << MW_DATA) | (1 << MW_CLK) | (1 << MW_LE_LMK0X0XX) | (1 << MW_LMKGOE);
    MW_PORT &= ~((1 << MW_DATA) | (1 << MW_CLK) | (1 << MW_LE_LMK0X0XX) | (1 << MW_LMKGOE));
}


void LMKSetInput(uint8_t no)
{
    if (no) {
        LMK0X0XX_WRITE(0x4800000E);
    } else {
        LMK0X0XX_WRITE(0x6800000E);
    }
}

void LMKSetDiv(uint8_t div)
{
    write_reg_LMK0X0XX(MAKE_LMK_HH(), MAKE_LMK_HL( ((div > 0) ? 1 : 0), 1), MAKE_LMK_LH(div), MAKE_LMK_LL(0, 0));

    //////////////////////////////////
    LMK0X0XX_WRITE(0x00022A09);
    LMKSetInput(0); // defauts to output 0
}

void LMKEnable(uint8_t en)
{
    if (en) {
        MW_PORT |= (1 << MW_LMKGOE);
        _delay_ms(1);

        LMK0X0XX_WRITE(0x80000100);

        _delay_ms(1);

        uint8_t i;
        for (i = 0; i < 8; i++) {
            write_reg_LMK0X0XX(MAKE_LMK_HH(), MAKE_LMK_HL(0, 0), MAKE_LMK_LH(1), MAKE_LMK_LL(0, i));
        }


        LMK0X0XX_WRITE(0x00022A09);
        LMKSetInput(0); // defauts to output 0
    } else {
        MW_PORT &= ~(1 << MW_LMKGOE);
    }
}
