#include <avr/io.h>
#include <avr/interrupt.h>
#include "freqmes.h"
#include "lmk0x0xx.h"

#define BIND static
#include "uint64_ops.h"


static uint16_t CounterHValue;
//static uint16_t CounterLValue;


static uint32_t counted_prev;
static uint32_t counted;

static uint8_t measures;

ISR(TIMER3_OVF_vect, ISR_BLOCK)
{
    // Self clock timer
    uint8_t b;
    //uint32_t a = TCNT1;
    //a |= (uint32_t)CounterHValue << 16;

    union {
        uint16_t u16[2];
        uint32_t u32;
    } a;

    a.u16[0] = TCNT1;
    a.u16[1] = CounterHValue;

    //uint32_t a = ((uint32_t)CounterHValue << 16) | ((uint16_t) ((b = TCNT1H)) << 8) | (uint8_t) TCNT1L;
 //   if ((TIFR1 & (1<< TOV1)) && ((b & 0xf0) == 0)) {
 //       a += 0x10000;
 //   }
    counted_prev = counted;
    counted = a.u32;
    //counted = counted_prev;
    measures++;
}

ISR(TIMER1_OVF_vect, ISR_BLOCK)
{
    // External clock timer
    CounterHValue++;
}

uint32_t FreqGetTicks(void)
{
    cli();
    uint32_t val = counted - counted_prev;
    sei();
    return val;
}

uint8_t FreqGetMes(void)
{
    return measures;
}

uint64_t FreqCalculate(uint32_t counted, uint8_t div)
{
#define COUNTER_BITS  16
    uint8_t fmt_bits;
    if ((TCCR3B & ((1 << CS31) | (1 << CS30))) == ((1 << CS31) | (1 << CS30))) {
        //type = FMT_COARSE;
        //fmt_div = 64;
        fmt_bits = COUNTER_BITS + 6;
    } else if ((TCCR3B & ((1 << CS32))) == ((1 << CS32))) {
        //type = FMT_NORMAL;
        //fmt_div = 256;
        fmt_bits = COUNTER_BITS + 8;
    } else {
        //type = FMT_PRECISE;
        //fmt_div = 1024;
        fmt_bits = COUNTER_BITS + 10;
    }

    uint16_t divx = (div == 0) ? 1 : (uint16_t)div << 1;
    return uint64_shiftr(uint64_mul32(counted, (uint32_t)divx * F_CPU),
                         fmt_bits);
}

void FreqStartMeasure(uint8_t type)
{
    measures = 0;
    counted = 0;
    CounterHValue = 0; //CounterLValue = 0;

    TCNT3L = 0;
    TCNT3H = 0;
    switch (type) {
    case FMT_COARSE:  TCCR3B = (1 << CS31) | (1 << CS30); break;             //Freq/64
    case FMT_NORMAL:  TCCR3B = (1 << CS32);               break;             //Freq/256
    default:          TCCR3B = (1 << CS32) | (1 << CS30); break;             //Freq/1024
    }


    TCNT1L = 0;
    TCNT1H = 0;
    TCCR1B = (1 << CS12) | (1 << CS11) | (1 << CS10);      //External T1 Source
    //TCCR1B = (1 << CS32) | (1 << CS30);  Self clock for testing


    TIMSK1 = (1 << TOIE1);
    TIMSK3 = (1 << TOIE3);

}


void FreqStopMeasure(void)
{
    TCCR3B = 0;
    TCCR1B = 0;
}




