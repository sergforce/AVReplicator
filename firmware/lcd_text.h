#ifndef LCD__TEXT_H
#define LCD__TEXT_H

/**
 * @file Character-based LCD ops
 *
 */
 
 void LCDInit(void);
 
 void LCDClear(void);
 
 void LCDSetPos(uint8_t row, uint8_t col);
 
 void LCDPuts(const char *p);
 
 void LCDPuts_P(const char *p);
 

#endif // LCD__TEXT_H
