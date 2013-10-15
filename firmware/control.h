#ifndef CONTROL_H
#define CONTROL_H

void CtrlInit(void);

/**
 * @brief CtrlUpdate Udpate contol state, should be called periodically
 */
void CtrlUpdate(void);

/**
 * @brief CtrlClear Clear all events
 */
void CtrlClear(void);

uint8_t CtrlIsUpPressed(void);
uint8_t CtrlIsDownPressed(void);
uint8_t CtrlIsOkPressed(void);
uint8_t CtrlIsBackPressed(void);


#endif // CONTROL_H
