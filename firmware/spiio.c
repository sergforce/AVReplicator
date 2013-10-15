#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

#include <PortConfig.h>
#include "spiio.h"


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


#define _nop() do { __asm__ __volatile__ ("nop"); } while (0)

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

// F/128
void spi_slow(void)
{
  SPSR = 0;
  SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR1) | (1<<SPR0);
}

void SPIInit(void)
{
    SPI_DDR |= (1 << SPI_MOSI) | (1 << SPI_SCK);

#if USE_EEPROM
    //Memory CSs
    EE_DDR |= (1 << EE_CS0) | (1 << EE_CS1) | (1 << EE_CS2) | (1 << EE_CS3);
    EE_PORT |= (1 << EE_CS0) | (1 << EE_CS1) | (1 << EE_CS2) | (1 << EE_CS3);
#endif

    // ClockTamer ISP
    CT_DDR |= (1 << CT_POWER) | (1 << CT_RESET);
    CT_PORT |= ((1 << CT_POWER) | (1 << CT_RESET));

    // ClockTamer SPI CS
    CTS_DDR |= (1 << CTS_CS);
    CTS_PORT |= (1 << CTS_CS);

    spi_slow();
}

static inline uint8_t transferSPI(uint8_t data)
{
    SEND_SPI(data);
    WAIT_FOR_SPI();
    return READ_SPI();
}

#ifdef USE_EEPROM
//////////////////////////////////////////////////////////////////////////
// GENERIC EEPROM COMMANDS
//

static inline void eespi_write_enable(void)
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

static void eespi_read_page(uint16_t addr, uint8_t *data, uint8_t count)
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
    _nop();
}

static void eespi_unselect(uint32_t addr)
{
    uint8_t chip = addr >> EEPROM_SIZE_BITS;
    switch (chip) {
    case 0:   EE_PORT |=  (1 << EE_CS0); break;
    case 1:   EE_PORT |=  (1 << EE_CS1); break;
    case 2:   EE_PORT |=  (1 << EE_CS2); break;
    case 3:   EE_PORT |=  (1 << EE_CS3); break;
    }
    _nop();
}

void eemem_write_page(uint32_t addr, const uint8_t *data, uint8_t count)
{
    eespi_select(addr);
    eespi_write_enable();
    eespi_unselect(addr);

    eespi_select(addr);
    eespi_wrtie_page((uint16_t)(addr & (EEPROM_SIZE - 1)), data, count);
    eespi_unselect(addr);

    eespi_select(addr);
    eespi_wait_for_write();
    eespi_unselect(addr);
}

void eemem_read_page(uint32_t addr, uint8_t *data, uint8_t count)
{
    eespi_select(addr);
    eespi_read_page((uint16_t)(addr & (EEPROM_SIZE - 1)), data, count);
    eespi_unselect(addr);
}

#endif


//////////////////////////////////////////////////////////////////////////
// AVR SPI Functions
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
#define AVRISP_READ_FUSE_EX()       avrisp_transfer(0x50, 0x08, 0xFF, 0xFF)

#define AVRISP_READ_CAL()           avrisp_transfer(0x31, 0x00, 0x00, 0xFF)

#define AVRISP_POLL_RDY()           avrisp_transfer(0xf0, 0x00, 0xFF, 0xFF)



int8_t avrisp_enter(void)
{
  int8_t rep, i;
  for (i = 0; i < 16; i++) {
    AVRSPI_RESET_DOWN(); AVRSPI_RESET_DOWN();

    _delay_ms(50+30*i);

    transferSPI(AVRISP_ENTER_B0);
    transferSPI(AVRISP_ENTER_B1);
    rep = transferSPI(0xFF);
    transferSPI(0xFF);

    if (rep != AVRISP_ENTER_B1) {
      AVRSPI_RESET_UP(); AVRSPI_RESET_UP();

      _delay_ms(1+10*i);

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

static void wait_rdy(void)
{
    uint8_t b;
    do {
        b = AVRISP_POLL_RDY();
    } while ((b & 1) == 1);
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
    wait_rdy();
}

void avrisp_read_eeprom(uint16_t addr, uint8_t* data, uint16_t count)
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
    wait_rdy();
}

void avrisp_chip_erase(void)
{
    AVRISP_CHIP_ERASE();

    //Wait for finalizing
    wait_rdy();
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
    AVRISP_WRITE_FUSE(fuse);
    wait_rdy();
}

void avrisp_write_fuse_high(uint8_t fuse)
{
    AVRISP_WRITE_FUSE_HIGH(fuse);
    wait_rdy();
}

void avrisp_write_fuse_ex(uint8_t fuse)
{
    AVRISP_WRITE_FUSE_EX(fuse);
    wait_rdy();
}


//ClockTamer

#define CLOCKTAMER_SELECT()    do { SPCR |= (1<<CPOL); _nop(); _nop(); CTS_PORT &= ~(1 << CTS_CS); } while (0)
#define CLOCKTAMER_UNSELECT()  do { CTS_PORT |=  (1 << CTS_CS);_nop(); _nop(); SPCR &= ~(1<<CPOL);   } while (0)


uint8_t clocktamer_get_replyln(char* reply, uint8_t max_reply)
{
    uint8_t cmd;
    uint16_t i;
    uint16_t j;
#define MAX_ITER_J  60000

    // Wait for maximum of 0.5 sec
    for (i = 0; i < 50000; i++) {
        _delay_us(10);

        CLOCKTAMER_SELECT();
        cmd = transferSPI(0xFF);
        CLOCKTAMER_UNSELECT();

        _delay_us(20);

        if (cmd != 0xFF && cmd != 0x00) {
            //Start of reply
            uint8_t rep;
            for (rep = 0; rep < max_reply; ) {
                *reply++ = cmd;
                rep++;

                if (cmd == '\r') {
                    *(--reply) = 0; //Terminate string
                    rep--;
                } else if (cmd == '\n' /*|| cmd == '\r'*/) {//End of message
                    *(--reply) = 0; //Terminate string
                    rep--;
                    goto end_of_reply;
                }

                CLOCKTAMER_SELECT();
                cmd = transferSPI(0xFF);
                CLOCKTAMER_UNSELECT();
                _delay_us(20);

                if (cmd == 0xFF || cmd == 0) {
                    for (j = 0; j < MAX_ITER_J; j++) {
                        _delay_us(20);
                        CLOCKTAMER_SELECT();
                        cmd = transferSPI(0xFF);
                        CLOCKTAMER_UNSELECT();
                        if (cmd != 0xFF && cmd != 0x00)
                            break;
                    }
                    if (j == MAX_ITER_J)
                        goto reply_trunc;//end_of_reply;
                }
            }

reply_trunc:
            //Message truncated
            *reply = 0xFF; //Message not end flag
end_of_reply:
            return rep;
        }
        CLOCKTAMER_UNSELECT();
    }

    CLOCKTAMER_UNSELECT();
    return 0; //No message has been received
}

uint8_t clocktamer_sendcmd_p(const char* cmd_p, char* reply, uint8_t max_reply)
{
    CLOCKTAMER_SELECT();

    char cmd;
    while ((cmd = pgm_read_byte(cmd_p++))) {
        transferSPI(cmd);
        //_delay_us(100);
    }
    transferSPI('\n');
    CLOCKTAMER_UNSELECT();

    return clocktamer_get_replyln(reply, max_reply);
}

uint8_t clocktamer_sendcmd(char* cmd, uint8_t max_reply)
{
    CLOCKTAMER_SELECT();
    char *iocmd = cmd;
    char c;
    while ((c = *(iocmd++))) {
        transferSPI(c);
        //_delay_us(100);
    }
    transferSPI('\n');
    CLOCKTAMER_UNSELECT();

    return clocktamer_get_replyln(cmd, max_reply);
}

void clocktamer_reset(void)
{
    CT_DDR |= (1 << CT_RESET);
    AVRSPI_RESET_DOWN();
    _delay_ms(250);
    AVRSPI_RESET_UP();
}

void clocktamer_dfubit_set(void)
{
    CTS_PORT &= ~(1 << CTS_CS);

    SPCR = 0;

    _nop();

    //Turn off SPI to get rid of phantom power
    //SPI_DDR &= ~((1 << SPI_MOSI) | (1 << SPI_SCK));
    SPI_PORT &= ~((1 << SPI_MISO) | (1 << SPI_MOSI) | (1 << SPI_SCK));


    CT_DDR &= ~(1 << CT_RESET);
    CT_PORT &= ~(1 << CT_RESET);

}

void clocktamer_dfubit_clear(void)
{
    //SPI_DDR |= (1 << SPI_MOSI) | (1 << SPI_SCK);
    CT_DDR |= (1 << CT_RESET);
    CT_PORT |= (1 << CT_RESET);

    CTS_PORT |= (1 << CTS_CS);

    spi_slow();
}


