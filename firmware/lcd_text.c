#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

#include <PortConfig.h>
#include "lcd_text.h"

#define LCD_CTRL_MASK  ((1<<LCD_CTRL_EN)|(1<<LCD_CTRL_RS)|(1<<LCD_CTRL_RD))

#define _nop() do { __asm__ __volatile__ ("nop"); } while (0)

static void    lcd_write_byte(uint8_t ch);
static uint8_t lcd_read_byte(void);
static void    lcd_write_cmd(uint8_t cmd);
static void    lcd_write_data(uint8_t data);


void lcd_write_byte(uint8_t ch)
{
    // Note RS must be select outside
    LCD_CTRL_PORT &= ~(1 << LCD_CTRL_RD);
_delay_us(1);
	LCD_CTRL_PORT |= (1 << LCD_CTRL_EN);
_delay_us(1);
    LCD_DATA_PORT = ch;
_delay_us(1);
    LCD_CTRL_PORT &= ~(1 << LCD_CTRL_EN);
_delay_us(1);
}

uint8_t lcd_read_byte(void)
{
    uint8_t res;

    // Note RS must be select outside
    LCD_CTRL_PORT |= (1 << LCD_CTRL_RD);

    // Enter input mode
    LCD_DATA_DDR = 0;

    // Read data
    LCD_CTRL_PORT |= (1 << LCD_CTRL_EN);
	_nop();
    res = LCD_DATA_PIN;
    LCD_CTRL_PORT &= ~(1 << LCD_CTRL_EN);

    // Enter write mode
    LCD_DATA_DDR = 0xff;
    return res;
}


void lcd_write_cmd(uint8_t cmd)
{
    LCD_CTRL_PORT &= ~(1 << LCD_CTRL_RS);
	_delay_us(1);
    lcd_write_byte(cmd);
    _delay_us(138);
}

void lcd_write_data(uint8_t data)
{
    LCD_CTRL_PORT |= (1 << LCD_CTRL_RS);
    _delay_us(1);
	lcd_write_byte(data);
    _delay_us(144);
}

/////////////////////////////////////////////////////////////////////////
// External interfacse
//
void LCDInit(void)
{
    // HW init
    LCD_DATA_DDR = 0xff;
    LCD_CTRL_DDR |= LCD_CTRL_MASK;
	LCD_CTRL_PORT &= ~(LCD_CTRL_MASK);
	
_delay_us(1);
	
    LCD_CTRL_PORT &= ~(1 << LCD_CTRL_RS);

    _delay_ms(15);

    lcd_write_byte(0x3f);

    _delay_ms(5);

    lcd_write_byte(0x3f);

    _delay_us(110);

    lcd_write_byte(0x3f);

    _delay_us(140);

	lcd_write_byte(0x38); _delay_us(140);
	lcd_write_byte(0x08); _delay_us(140);
	lcd_write_byte(0x01); _delay_us(140);
	lcd_write_byte(0x06); _delay_us(140);
	
    //lcd_write_byte(0x3f); _delay_us(140);
    //lcd_write_byte(0x08); _delay_us(140);
    //lcd_write_byte(0x01); _delay_us(140);
    //lcd_write_byte(0x06); _delay_us(140);
	
}


void LCDClear(void)
{
    lcd_write_cmd(0x01);
    lcd_write_cmd(0x02);
	lcd_write_cmd(0x0F);
}

void LCDSetPos(uint8_t row, uint8_t col)
{
    lcd_write_cmd(0x80 | (row << 6) | (col & 0x1f));
}

void LCDPuts(const uint8_t *p)
{
    char c, i;
    for(i=0; i < 32; i++, p++) {
        c = *p;
        if (c) {
            lcd_write_data(c);
        } else {
            return;
        }
    }
}

void LCDPuts_P(const uint8_t *p)
{
    char c, i;
    for(i=0; i < 32; i++, p++) {
        c = pgm_read_byte(p);
        if (c) {
            lcd_write_data(c);
        } else {
            return;
        }
    }
}


