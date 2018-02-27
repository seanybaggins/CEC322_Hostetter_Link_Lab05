/*
 * personalUART.h
 *
 *  Created on: Feb 20, 2018
 *      Author: Sean
 */

#ifndef DRIVERS_UART_PERSONALUART_H_
#define DRIVERS_UART_PERSONALUART_H_

void setupUART(void);
void IntUART0(void);
void UARTSend(const uint8_t *pui8Buffer);
void printMainMenu(void);

#endif /* DRIVERS_UART_PERSONALUART_H_ */
