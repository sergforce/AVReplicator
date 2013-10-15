#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

#include <PortConfig.h>

#include "font16x8.h"
#include "font16x8.c"

static uint8_t g_row, g_col;
//#define _nop() do { __asm__ __volatile__ ("nop\r\n nop\r\n nop\r\n nop\r\n nop\r\n nop\r\n nop\r\n nop\r\n nop\r\n nop\r\n"); } while (0)

#define _nop() _delay_us(20)

#define CMD_TURNON_DISPLAY  0x3f
#define CMD_SET_START_LINE  0xc0
#define CMD_SET_PAGE        0xb8
#define CMD_SET_ADDR        0x40


#define LCD_CTRL_MASK  ((1<<LCD_GRA_CTRL_EN)|(1<<LCD_GRA_CTRL_RS)|(1<<LCD_GRA_CTRL_RD)|\
    (1 << LCD_GRA_CTRL_CS1)|(1 << LCD_GRA_CTRL_CS0))

static void lcd_select_cs0(void)
{
    LCD_GRA_CTRL_PORT &= ~(1 << LCD_GRA_CTRL_CS1);
    _nop();
    LCD_GRA_CTRL_PORT |= (1 << LCD_GRA_CTRL_CS0);
    _nop();
}

static void lcd_select_cs1(void)
{
    LCD_GRA_CTRL_PORT &= ~(1 << LCD_GRA_CTRL_CS0);
    _nop();
    LCD_GRA_CTRL_PORT |= (1 << LCD_GRA_CTRL_CS1);
    _nop();
}

static void lcd_write_byte(uint8_t ch)
{
    // Note RS must be select outside
    LCD_GRA_CTRL_PORT &= ~(1 << LCD_GRA_CTRL_RD);
    _nop();
    LCD_GRA_CTRL_PORT |= (1 << LCD_GRA_CTRL_EN);
    _nop();
    LCD_GRA_DATA_PORT = ch;
    _nop();
    LCD_GRA_CTRL_PORT &= ~(1 << LCD_GRA_CTRL_EN);
    _nop();
}

static void lcd_write_cmd(uint8_t cmd)
{
    LCD_GRA_CTRL_PORT &= ~(1 << LCD_GRA_CTRL_RS);
    _nop();
    lcd_write_byte(cmd);
    _delay_ms(2);
}

static void lcd_write_data(uint8_t data)
{
    LCD_GRA_CTRL_PORT |= (1 << LCD_GRA_CTRL_RS);
    _nop();
    lcd_write_byte(data);
    _delay_us(40);
}



static void render_char(unsigned char c)
{

    // Check in font
    if (c > FONT_ASCII_END || c < FONT_ASCII_START) {
        c = 0;
    } else {
        c-= FONT_ASCII_START;
    }

    uint8_t i;
    uint8_t addr;

    //Select CHIP side    
    if (g_col > 8) {
        lcd_select_cs1();
        addr = (g_col - 8) << 3;
    } else {
        lcd_select_cs0();
        addr = (g_col) << 3;
    }

    //Set ADDR
    ///////////////////////////////////////////////////////////////////////////////
    lcd_write_cmd(CMD_SET_ADDR | addr);

    uint8_t page = g_row << 1;

    // Load char in memory
    int8_t side = 15;
    for (side = 15; side > 0; side -= 8, ++page) {
        uint8_t fdata[8];
        for (i = 0; i < 8; i++) {
            fdata[i] = pgm_read_byte(FONT_DATA + c + (uint16_t)(side - i) * FONT_SYMBOLS);
        }
        uint8_t cdata[8];
        //uint8_t j;

        // Select RAM PAGE
        ///////////////////////////////////////////////////////////////////////////////
        lcd_write_cmd(CMD_SET_PAGE | page);

        uint8_t k;
        uint8_t l;
        for (l = 1, i = 0; i < 8; i++, l<<=1) {
            cdata[i] = 0;

            //for (j = 0x80, k = 0; j != 0; k++, j>>=1) {
            for (k = 0; k < 8; k++) {
                cdata[i] |= fdata[k] & l;
            }
        }

        // Write Bytes
        /////////////////////////////////////////////////////////////////////////////
        for (i = 0; i < 8; i++) {
            lcd_write_data(cdata[i]);
        }
    }

}

void LCDInit(void)
{
    g_row = g_col = 0;

    // SET DIRECTIONS
    LCD_GRA_DATA_DDR = 0xff;
    LCD_GRA_DATA_PORT = 0;
    LCD_GRA_CTRL_DDR = LCD_CTRL_MASK;
    LCD_GRA_CTRL_PORT &= ~(LCD_CTRL_MASK);

    // RESET
    //LCD_GRA_CTRL_PORT
    for (;;) {
    _delay_ms(10);

    lcd_select_cs0(); lcd_write_cmd(CMD_TURNON_DISPLAY);
    lcd_select_cs1(); lcd_write_cmd(CMD_TURNON_DISPLAY);

    _delay_ms(100);

    LCDClear();

    _delay_ms(600);
    }
}

void LCDPutChar(char c)
{
    render_char(c);

    // Move "Cursor"
    if (++g_col > 15) {
        g_col = 0;
        if (++g_row > 3) {
            g_row = 0;
        }
    }
}


static void lcd_clear_chip_memory(void)
{
    lcd_write_cmd(CMD_SET_ADDR);
    for (uint8_t page = 0; page < 8; ++page) {
        lcd_write_cmd(CMD_SET_PAGE | page);

        for (uint8_t addr = 0; addr < 64; ++addr) {
            lcd_write_data(0xCC);
        }
    }
}

void LCDClear(void)
{
    lcd_select_cs0(); lcd_clear_chip_memory();
    lcd_select_cs1(); lcd_clear_chip_memory();
}

void LCDSetPos(uint8_t row, uint8_t col)
{
    g_row = row & 0x3;
    g_col = col & 0xf;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////

const char* LCDPutsBig(const char *p, uint8_t start_row)
{
    char c, i;
    for(i=0; i < 64; i++) {
        if ((i & 0x0f) == 0) {
            LCDSetPos(((i >> 4) + start_row) & 0x3, 0);
        }

        c = *p++;
        if (c) {
            LCDPutChar(c);
        } else {
            return 0;
        }
    }

    return p;
}

void LCDPuts(const char *p)
{
    char c, i;
    for(i=0; i < 32; i++, p++) {
        c = *p;
        if (c) {
            LCDPutChar(c);
        } else {
            return;
        }
    }
}

void LCDPuts_P(const char *p)
{
    char c, i;
    for(i=0; i < 32; i++, p++) {
        c = pgm_read_byte(p);
        if (c) {
            LCDPutChar(c);
        } else {
            return;
        }
    }
}




