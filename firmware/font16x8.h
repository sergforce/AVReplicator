
#define FONT_WIFTH   8
#define FONT_HEIGHT 16

#define FONT_ASCII_START   32
#define FONT_ASCII_END     127

#define FONT_SYMBOLS       (FONT_ASCII_END + 1 - FONT_ASCII_START)

extern const unsigned char FONT_DATA[] PROGMEM;
