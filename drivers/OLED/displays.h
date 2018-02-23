/*
 * displays.h
 *
 *  Created on: Feb 20, 2018
 *      Author: Sean
 */

#ifndef DRIVERS_DISPLAYS_H_
#define DRIVERS_DISPLAYS_H_

void diplayInfoOnBoard(uint8_t* formatString,uint32_t ADCValue,
                          uint32_t yLocationOnDisplay, DisplayMode displayMode);
void clearBlack(void);
void UARTSend(const uint8_t *pui8Buffer);
void printMainMenu(void);
void diplaySplashOnOLED(void);

#endif /* DRIVERS_DISPLAYS_H_ */
