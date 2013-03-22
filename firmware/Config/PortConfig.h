/** \file
 *  \brief AVR Pin Configuration Header File
 *
 *  This header file is used to configure pins layout on the board
 *
 */

#ifndef _PORT_CONFIG_H_
#define _PORT_CONFIG_H_

	#ifdef __AVR_AT90USB1287__

#if USE_EEPROM
	    // EEPROM
		#define EE_DDR   DDRB
		#define EE_PORT  PORTB
        #define EE_CS0   PB5
        #define EE_CS1   PB6
        #define EE_CS2   PB4
        #define EE_CS3   PB7
#endif

        // Clock Tamer CS for SPI
        #define CTS_DDR   DDRB
        #define CTS_PORT  PORTB
        #define CTS_CS    PB0

        // Clock Tamer CS for Power and ISP
        #define CT_DDR    DDRC
        #define CT_PORT   PORTC
        #define CT_POWER  PC6
        #define CT_RESET  PC5

        #define USBV_DDR  DDRE
        #define USBV_PORT PORTE
        #define USBV_P    PE7

        // Generic SPI
        #define SPI_DDR   DDRB
        #define SPI_PORT  PORTB
        #define SPI_MISO  PB3
        #define SPI_MOSI  PB2
        #define SPI_SCK   PB1

		
		// Generic 4*20 LCD
		#define LCD_DATA_PORT  PORTA
		#define LCD_DATA_DDR   DDRA
		#define LCD_DATA_PIN   PINA

		#define LCD_CTRL_PORT  PORTE
		#define LCD_CTRL_DDR   DDRE
		#define LCD_CTRL_PIN   PINE

		#define LCD_CTRL_EN    PE5
		#define LCD_CTRL_RS    PE4
		#define LCD_CTRL_RD    PE0
		
		// LED
        #define LED_DDR   DDRB
        #define LED_PORT  PORTB
        #define LED_1     PB4
        #define LED_2     PB5
        #define LED_3     PB6
        #define LED_4     PB7
		
        // Control buttons
        #define CTRL_DDR    DDRD
        #define CTRL_PIN    PIND
        #define CTRL_PORT   PORTD
        #define CTRL_BTUP   PD0
        #define CTRL_BTDOWN PD1
        #define CTRL_BTOK   PD2
        #define CTRL_BTBACK PD3

        // Microware
        #define MW_DDR           DDRF
        #define MW_PORT          PORTF
        #define MW_CLK           PF0
        #define MW_DATA          PF1
        #define MW_LE_LMK0X0XX   PF2
        #define MW_LMKGOE        PF3


	#else

		#error Unsupported architecture for pinouts.

	#endif
#endif
