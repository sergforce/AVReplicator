

#include <avr/io.h>

#include "lmk0x0xx.h"

static uint16_t CounterHValue;
static uint16_t CounterLValue;


static uint32_t counted_prev;
static uint32_t counted;


ISR(TIMER0_OVF_vect, ISR_BLOCK)
{
    // Self clock timer
    uint32_t a = ((uint32_t)CounterHValue << 16) | CounterLValue;
    if (TIFR1 & (1<< TOV1)) {
        a += 0x10000;
    }
    counted_prev = counted;
    counted = counted_prev;
}

ISR(TIMER1_OVF_vect, ISR_BLOCK)
{
    // External clock timer
    CounterHValue++;
}


enum FreqMeasureType {
    FMT_COARSE = 0, /**< 4M  clocks (0.5 sec) */
    FMT_NORMAL,     /**< 16M clocks (2 sec)  */
    FMT_PRECISE     /**< 64M clocks (8 sec) */
};

void FreqStartMeasure(uint8_t type)
{
    counted = counted_prev = 0;
    CounterHValue = CounterLValue = 0;

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


    TIMSK1 = (1 << TOIE1);
    TIMSK3 = (1 << TOIE3);

}


void FreqStopMeasure(void)
{
    TCCR3B = 0;
    TCCR1B = 0;
}




