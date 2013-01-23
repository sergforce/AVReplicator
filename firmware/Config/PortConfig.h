/** \file
 *  \brief AVR Pin Configuration Header File
 *
 *  This header file is used to configure pins layout on the board
 *
 */

#ifndef _PORT_CONFIG_H_
#define _PORT_CONFIG_H_

	#ifdef __AVR_AT90USB1287__

	    // EEPROM
		#define EE_DDR   DDRB
		#define EE_PORT  PORTB
		#define EE_CS0   PB4
		#define EE_CS1   PB5
		#define EE_CS2   PB6
		#define EE_CS3   PB7

        // Clock Tamer Header
		#define CT_DDR   DDRC
		#define CT_PORT  PORTC
		#define CT_CS    PC1
		#define CT_RESET PC2

        // Generic SPI
		#define DDR_SPI  DDRB
		#define DD_MOSI  PB3
		#define DD_SCK   PB1		

		
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
		#define LED_DDR   DDRF
		#define LED_PORT  PORTF
		#define LED_1     PF4
		#define LED_2     PF5
		#define LED_3     PF6
		#define LED_4     PF7
		
	#else

		#error Unsupported architecture for pinouts.

	#endif
#endif
