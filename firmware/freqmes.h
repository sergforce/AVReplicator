#ifndef FREQMES_H
#define FREQMES_H



enum FreqMeasureType {
    FMT_COARSE = 0, /**< 4M  clocks (0.5 sec) */
    FMT_NORMAL,     /**< 16M clocks (2 sec)  */
    FMT_PRECISE     /**< 64M clocks (8 sec) */
};


void FreqStartMeasure(uint8_t type);

void FreqStopMeasure(void);

uint32_t FreqGetTicks(void);

uint32_t FreqCalculate(uint32_t counted, uint8_t div);

uint64_t FreqCalculate64(uint64_t counted, uint8_t div);

uint8_t FreqGetMes(void);

#endif // FREQMES_H
