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
 
 /**
  * @brief LCDPutsBig Output long string (up to 64 bytes to screen)
  * @param p string
  * @return NULL if whole string has been displayed, otherwise pointer to remaining part
  */
 const char *LCDPutsBig(const char *p);

 void LCDPutChar(char c);


#endif // LCD__TEXT_H
