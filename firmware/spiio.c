
#include <avr/ports.h>
#include <avr/io.h>
#include <avr/delay.h>


////////////////////////////////////////////////////////////
// PORT CONFIGURATION
//

#define EE_DDR   DDRC
#define EE_PORT  PORTC
#define EE_CS0    0 //PC0
#define EE_CS1    0 //PC0
#define EE_CS2    0 //PC0
#define EE_CS3    0 //PC0


#define CT_DDR   DDRC
#define CT_PORT  PORTC
#define CT_CS    1 //PC1
#define CT_RESET 2 //PC2


#define DDR_SPI  DDRB
#define DD_MOSI  3 //PB3
#define DD_SCK   5 //PB5

//
// END OF PORT CONFIGURATION
/////////////////////////////////////////////////////////////


#define WAIT_FOR_SPI()   while(!(SPSR & (1<<SPIF)))
#define SEND_SPI(x)      SPDR = (x)
#define READ_SPI()       SPDR


//EEPROM GENERAL
#define EESPI_WREN  0x06
#define EESPI_WRDI  0x04
#define EESPI_RDSR  0x05
#define EESPI_WRSR  0x01
#define EESPI_READ  0x03
#define EESPI_WRITE 0x02

#define EESPI_SR_nRDY 0x01
#define EESPI_SR_WEN  0x02

#define EESPI_SELECT()      EE_PORT &= ~(1 << EE_CS)
#define EESPI_UNSELECT()    EE_PORT |=  (1 << EE_CS)

#define AVRSPI_RESET_DOWN() CT_PORT &= ~(1 << CT_RESET)
#define AVRSPI_RESET_UP()   CT_PORT |=  (1 << CT_RESET)




//////////////////////////////////////////////////////////////////////////
// GENERIC SPI COMMANDS
//
#include "spiio.h"

// F/2
void spi_fast(void)
{
  SPSR = (1<<SPI2X);
  SPCR = (1<<SPE) | (1<<MSTR);
}

// F/64
void spi_slow(void)
{
  SPSR = 0;
  SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR1);
}

static inline uint8_t transferSPI(uint8_t data)
{
    SEND_SPI(data);
    WAIT_FOR_SPI();
    return READ_SPI();
}

//////////////////////////////////////////////////////////////////////////
// GENERIC EEPROM COMMANDS
//

static inline void eespi_write_enable()
{
    transferSPI(EESPI_WREN);
}

static void eespi_wrtie_page(uint16_t addr, const uint8_t *data, uint8_t count)
{
  //transferSPI(EESPI_WREN);

  transferSPI(EESPI_WRITE);
  transferSPI(addr>>8);

  SEND_SPI(addr);

  for (; count != 0; --count) {
    uint8_t d = *(data++);
    WAIT_FOR_SPI();
    READ_SPI();
    SEND_SPI(d);
  }

  WAIT_FOR_SPI();
  READ_SPI();
}

static void eespi_read(uint16_t addr, uint8_t *data, uint8_t count)
{
  transferSPI(EESPI_READ);
  transferSPI(addr>>8);
  transferSPI(addr);

  for (; count != 0; --count) {
    *data++ = transferSPI(0xFF);
  }
}

static void eespi_wait_for_write(void)
{
  uint8_t flags;
  do {
      transferSPI(EESPI_RDSR);
      flags = transferSPI(0xFF);
  }
  while (flags & EESPI_SR_nRDY);
}

////////////////////////////////////////////////////////////////////
// Chip select program
static void eespi_select(uint32_t addr)
{
    uint8_t chip = addr >> EEPROM_SIZE_BITS;
    switch (chip) {
    case 0:  EE_PORT &= ~(1 << EE_CS0); break;
    case 1:  EE_PORT &= ~(1 << EE_CS1); break;
    case 2:  EE_PORT &= ~(1 << EE_CS2); break;
    case 3:  EE_PORT &= ~(1 << EE_CS3); break;
    }
}

static void eespi_unselect(uint32_t addr)
{
    uint8_t chip = addr >> EEPROM_SIZE_BITS;
    switch (chip) {
    case 0:   EE_PORT |=  (1 << EE_CS); break;
    case 1:   EE_PORT |=  (1 << EE_CS); break;
    case 2:   EE_PORT |=  (1 << EE_CS); break;
    case 3:   EE_PORT |=  (1 << EE_CS); break;
    }
}

void eemem_write_page(uint32_t addr, const uint8_t *data, uint8_t count)
{
    eespi_select(addr);
    eespi_write_enable();
    eespi_unselect(addr);

    eespi_select(addr);
    eespi_wrtie_page(addr & (EEPROM_SIZE - 1), data, count);
    eespi_unselect(addr);

    eespi_select(addr);
    eespi_wait_for_write();
    eespi_unselect(addr);
}

void eemem_read_page(uint32_t addr, uint8_t *data, uint8_t count)
{
    eespi_select(addr);
    eespi_read_page(addr & (EEPROM_SIZE - 1), data, count);
    eespi_unselect(addr);
}



//////////////////////////////////////////////////////////////////////////
// AVR SPI Functions

int8_t avrisp_enter(void)
{
  int8_t rep, i;
  for (i = 0; i < 16; i++) {
    AVRSPI_RESET_DOWN(); AVRSPI_RESET_DOWN();

    _delay_ms(50);

    transferSPI(AVRISP_ENTER_B0);
    transferSPI(AVRISP_ENTER_B1);
    rep = transferSPI(0xFF);
    transferSPI(0xFF);

    if (rep != AVRISP_ENTER_B1) {
      AVRSPI_RESET_UP(); AVRSPI_RESET_UP();

      _delay_ms(100);

    } else {
      return 0;
    }
  }
  return -1;
}

static int8_t avrisp_transfer(int8_t a, int8_t b, int8_t c, int8_t d)
{
    transferSPI(a);
    transferSPI(b);
    transferSPI(c);
    return transferSPI(d);
}

#define AVRISP_MEGA32U2_ID   0x8A951E
#define AVRISP_ENTER_B0    0xAC
#define AVRISP_ENTER_B1    0x53


//#define AVRISP_PROG_ENABLE()        avrisp_transfer()
#define AVRISP_CHIP_ERASE()         avrisp_transfer(0xAC, 0x80, 0xff, 0xff)
//#define AVRISP_LOAD_EXTADDR()        avrisp_transfer()
#define AVRISP_READ_PROG_H(b,c)     avrisp_transfer(0x28, b, c, 0xFF)
#define AVRISP_READ_PROG_L(b,c)     avrisp_transfer(0x20, b, c, 0xFF)
#define AVRISP_LOAD_PROG_H(b,i)     avrisp_transfer(0x48, 0xFF, b, i)
#define AVRISP_LOAD_PROG_L(b,i)     avrisp_transfer(0x40, 0xFF, b, i)
#define AVRISP_WRITE_PROG(a,b)      avrisp_transfer(0x4C, a, b, 0xFF)

#define AVRISP_READ_EEPROM(a,b)     avrisp_transfer(0xA0, a, b, 0xFF)
#define AVRISP_WRITE_EEPROM_SINGLE(a,b,i)     avrisp_transfer(0xC0, a, b, i)
#define AVRISP_LOAD_EEPROM(b,i)     avrisp_transfer(0xC1, 0x00, b, i)
#define AVRISP_WRITE_EEPROM(a,b)    avrisp_transfer(0xC2, a, b, 0xFF)

#define AVRISP_READ_LOCK()          avrisp_transfer(0x58, 0x00, 0xFF, 0xFF)
#define AVRISP_WRITE_LOCK(i)        avrisp_transfer(0xAC, 0xFF, 0xFF, 0xC0 | i)

#define AVRISP_READ_SIGNATURE(b)    avrisp_transfer(0x30, 0x00, b, 0xFF)

#define AVRISP_WRITE_FUSE(i)        avrisp_transfer(0xAC, 0xA0, 0xFF, i)
#define AVRISP_WRITE_FUSE_HIGH(i)   avrisp_transfer(0xAC, 0xA8, 0xFF, i)
#define AVRISP_WRITE_FUSE_EX(i)     avrisp_transfer(0xAC, 0xA4, 0xFF, i)
#define AVRISP_READ_FUSE()          avrisp_transfer(0x50, 0x00, 0xFF, 0xFF)
#define AVRISP_READ_FUSE_HIGH()     avrisp_transfer(0x58, 0x08, 0xFF, 0xFF)
#define AVRISP_READ_FUSE_EX()       avrisp_transfer(0x50, 0x04, 0xFF, 0xFF)

#define AVRISP_READ_CAL()           avrisp_transfer(0x31, 0x00, 0x00, 0xFF)

#define AVRISP_POLL_RDY()           avrisp_transfer(0xf0, 0x00, 0xFF, 0xFF)


int32_t avrisp_readsignature(void)
{
  union {
    int32_t a;
    uint8_t b[4];
  } ret;

  ret.b[0] = AVRISP_READ_SIGNATURE(0);
  ret.b[1] = AVRISP_READ_SIGNATURE(1);
  ret.b[2] = AVRISP_READ_SIGNATURE(2);
  ret.b[3] = 0;

  return ret.a;
}

void avrisp_leave(void)
{
   AVRSPI_RESET_UP(); AVRSPI_RESET_UP();
}

typedef union u16_sep
{
  uint16_t a;
  uint8_t b[2];
} u16_sep_t;


void avrisp_read_program(uint16_t addr, uint8_t* data, uint8_t count)
{
  u16_sep_t q;
  q.a = addr;
  // AVR-gcc is "little endian"

  for (;count != 0; --count, q.a++) {
    *(data++) = AVRISP_READ_PROG_L(q.b[1], q.b[0]); //Low byte
    *(data++) = AVRISP_READ_PROG_H(q.b[1], q.b[0]); //High byte
  }
}

void avrisp_flash_page(uint16_t addr, const uint8_t* data, uint8_t count)
{
    uint8_t b;
    for (b = 0; count != 0; --count, b++) {
      AVRISP_LOAD_PROG_L(b, *(data++)); //Low byte
      AVRISP_LOAD_PROG_H(b, *(data++)); //High byte
    }

    u16_sep_t q;
    q.a = addr;
    AVRISP_WRITE_PROG(q.b[1], q.b[0]);

    //Wait for finalizing
    do {
        b = AVRISP_POLL_RDY();
    } while (b & 1 == 1);
}

void avrisp_read_eeprom(uint16_t addr, uint8_t* data, uint8_t count)
{
    u16_sep_t q;
    q.a = addr;
    for (;count != 0; --count, q.a++) {
      *(data++) = AVRISP_READ_EEPROM(q.b[1], q.b[0]);
    }
}

void avrisp_write_eeprom_page(uint16_t addr, uint8_t* data, uint8_t count)
{
    uint8_t b;
    for (b = 0; count != 0; --count, b++) {
        AVRISP_LOAD_EEPROM(b, *(data++));
    }

    u16_sep_t q;
    q.a = addr;
    AVRISP_WRITE_EEPROM(q.b[1], q.b[0]);

    //Wait for finalizing
    do {
        b = AVRISP_POLL_RDY();
    } while (b & 1 == 1);
}

void avrisp_chip_erase(void)
{
    AVRISP_CHIP_ERASE();
}

uint8_t avrisp_read_fuse(void)
{
    return AVRISP_READ_FUSE();
}

uint8_t avrisp_read_fuse_high(void)
{
    return AVRISP_READ_FUSE_HIGH();
}

uint8_t avrisp_read_fuse_ex(void)
{
    return AVRISP_READ_FUSE_EX();
}

void avrisp_write_fuse(uint8_t fuse)
{
    return AVRISP_WRITE_FUSE(fuse);
}

void avrisp_write_fuse_high(uint8_t fuse)
{
    return AVRISP_WRITE_FUSE_HIGH(fuse);
}

void avrisp_write_fuse_ex(uint8_t fuse)
{
    return AVRISP_WRITE_FUSE_EX(fuse);
}

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

void copy_from_ct_to_ee(void);

#include <SPI.h>




void hardware_init(void)
{
  // For EEPORM
  EE_DDR |= (1 << EE_CS);
  EE_PORT |= (1 << EE_CS);

  // For ClockTamer
  CT_DDR |= (1 << CT_CS) | (1 << CT_RESET);
  CT_PORT |= (1 << CT_CS) | (1 << CT_RESET);

  //SPI Init
  DDR_SPI = (1<<DD_MOSI) | (1<<DD_SCK);
  // Enable SPI, Master, set clock rate fck/2
  spi_fast();

//  SPSR = (1<<SPI2X);
//  SPCR = (1<<SPE) | (1<<MSTR);

//  SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR1);
}

void setup() {

  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  hardware_init();

  Serial.println("SETUP");

  SPI.begin();

  spi_slow();
  int8_t rep = avrisp_enter();
  if (rep == AVRISP_ENTER_B1) {
      Serial.println("ISP OK");

      int32_t sig = avrisp_readsignature();

      if (sig == AVRISP_MEGA32U2_ID) {
         Serial.println("MEGA32U2 Detected");

      } else {
         Serial.print("Incorrect ISP id:");
         Serial.print(sig, HEX);
      }

  } else {
      Serial.println("ISP SYNC FAILED");
  }
  avrisp_leave();

  spi_fast();
}

uint8_t data[16] = "TEST data1";
uint8_t data2[17] = "!!!!!!!!!!!!!!";

void loop() {

  hardware_init();

  Serial.println("Prepare");
  eespi_wrtie_page(0, data, 16);
  Serial.println("Written data");
  eespi_wait_for_write();
  Serial.println("Ready");
  eespi_read(0, data2, 16);
  Serial.println("Data:");

  Serial.println((const char*)data2);


  // prints title with ending line break
  for (;;);


}
