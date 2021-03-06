/*
 * personalUART.c
 *
 *  Created on: Feb 20, 2018
 *      Author: Sean Link and Andrew Hostetter
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "driverlib/interrupt.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "personalUART.h"
#include "driverlib/gpio.h"


void setupUART(void) {
    // Turn on the UART
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);      // For UART

    // Wait until the peripheral is fully turned on
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0) ||
            !SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA));

    // Disable send and receive functionality
    UARTDisable(UART0_BASE);

    // Assign pin configuration and allow for console communication
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    // Configuring speed, bits, and parity
    UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 115200,
                                (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                                 UART_CONFIG_PAR_NONE));
    // Enalbing the interrupt
    IntEnable(INT_UART0);
    UARTIntClear(UART0_BASE, UART_INT_RX);
    UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);


    UARTEnable(UART0_BASE);
}

// Purpose: Send a message to the Console display
void UARTSend(const uint8_t *pui8Buffer)
{
    // Loop while there are more characters to send.
    for(uint32_t index = 0; index < strlen((const char *)pui8Buffer); index++)
    {
        // Write the next character to the UART.
        UARTCharPut(UART0_BASE, pui8Buffer[index]);
    }
}

// Purpose: Print the main menu to the UART Console
// Note to grader: I am keeping the multiple successive calls
// beacuse it reads better and I do not want to have one long string that
// goes past the 80th character. I think it is justified having slightly less
// Efficient code if the code reads better and the code in question is user driven
// and does not need to run as fast as possible.
void printMainMenu(void) {
    UARTSend("\r\n\nT - Toggle the LED\r\n");
    UARTSend("S - Splash Screen (2s)\r\n");
    UARTSend("Q - Quit Program \n\r");
}

