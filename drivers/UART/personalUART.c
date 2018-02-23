/*
 * personalUART.c
 *
 *  Created on: Feb 20, 2018
 *      Author: Sean
 */
/*
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"

#include "driverlib/uart.h"
#include "driverlib/rom.h"
#include "grlib/grlib.h"
#include "drivers/cfal96x64x16.h"
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
