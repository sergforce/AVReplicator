#include <avr/io.h>
#include <avr/delay.h>


#define LCD_DATA_PORT  PORTA
#define LCD_DATA_DDR   DDRA
#define LCD_DATA_PIN   PINA

#define LCD_CTRL_PORT  PORTE
#define LCD_CTRL_DDR   DDRE
#define LCD_CTRL_PIN   PINE

#define LCD_CTRL_EN    PE5
#define LCD_CTRL_RS    PE4
#define LCD_CTRL_RD    PE0

#define LCD_CTRL_MASK  ((1<<LCD_CTRL_EN)|(1<<LCD_CTRL_RS)|(1<<LCD_CTRL_RD))


static void    lcd_write_byte(uint8_t ch);
static uint8_t lcd_read_byte(void);
static void    lcd_write_cmd(uint8_t cmd);
static void    lcd_write_data(uint8_t data);


void lcd_write_byte(uint8_t ch)
{
    // Note RS must be select outside
    LCD_CTRL_PORT &= ~(1 << LCD_CTRL_RD);
    LCD_CTRL_PORT |= (1 << LCD_CTRL_EN);
    LCD_DATA_PORT = ch;
    LCD_CTRL_PORT &= ~(1 << LCD_CTRL_EN);
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
    res = LCD_DATA_PIN;
    LCD_CTRL_PORT &= ~(1 << LCD_CTRL_EN);

    // Enter write mode
    LCD_DATA_DDR = 0xff;
    return res;
}


void lcd_write_cmd(uint8_t cmd)
{
    LCD_CTRL_PORT &= ~(1 << LCD_CTRL_RS);
    lcd_write_byte(cmd);
    delay_us(38);
}

void lcd_write_data(uint8_t data)
{
    LCD_CTRL_PORT |= (1 << LCD_CTRL_RS);
    lcd_write_byte(data);
    delay_us(44);
}

/////////////////////////////////////////////////////////////////////////
// External interfacse
//
void LCDInit(void)
{
    // HW init
    LCD_DATA_DDR = 0xff;
    LCD_CTRL_DDR |= LCD_CTRL_MASK;
    LCD_CTRL_PORT &= ~(1 << LCD_CTRL_RS);

    delay_ms(15);

    lcd_write_byte(0x3f);

    delay_ms(5);

    lcd_write_byte(0x3f);

    delay_us(110);

    lcd_write_byte(0x3f);

    delay_us(40);

    lcd_write_byte(0x3f); delay_us(40);
    lcd_write_byte(0x08); delay_us(40);
    lcd_write_byte(0x01); delay_us(40);
    lcd_write_byte(0x06); delay_us(40);

}


void LCDClear(void)
{
    lcd_write_cmd(0x01);
    lcd_write_cmd(0x02);
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

void LCDPutsP(const uint8_t __progmem* p)
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


