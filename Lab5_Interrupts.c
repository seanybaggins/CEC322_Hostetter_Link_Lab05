/* Purpose: Use Interrupts for functionalities used in previous labs and
 * add the comparator peripheral functionality.
 * Class: CEC322
 * Authors: Sean Link and Andrew Hostetter
 * Date: 2/20/2018
 */
// Base includes with the timers Examples
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "grlib/grlib.h"
#include "drivers/cfal96x64x16.h"

// Necessary for blinking LED
#include "drivers/LED/LED.h"

// Necessary for lower case string conversion
#include <ctype.h>

// Necessary for Buttons
#include "drivers/buttons/buttons.h"

// Necessary for blinking LED
#include "driverlib/gpio.h"

// Necessary for ADC comparator
#include "drivers/comparator/comparator.h"

// Necessary for printing to the OLED
#include "drivers/OLED/displays.h"

// Necessary for the functionality of the UART
#include "drivers/UART/personalUART.h"
//****************************************************************************
// Defines and typedefs
//****************************************************************************

#define DEBOUNCE_DELAY 200000

// Necessary for blinking light clock
#define FIVE_PERCENT_CYCLE_ON 20000
#define NIENTYFIVE_PERCENT_CYCLE_OFF 380000

//****************************************************************************
// Globals
//****************************************************************************
tContext sContext;
uint32_t timesCrossed1Point6Volts = 0;
uint8_t menuSelection = '\0';
Button leftButton, rightButton, upButton, downButton, selectButton;
uint32_t buttonCounter = 0;
uint32_t globalFreeRunningCounter = 0;

//****************************************************************************
// Interrupts
//****************************************************************************
void IntButtons(void) {
    GPIOIntClear(BUTTONS_GPIO_BASE, ALL_BUTTONS);

    // Debounce
    static uint32_t lastButtonPressStamp = 0;
    if ((globalFreeRunningCounter - lastButtonPressStamp) < DEBOUNCE_DELAY)
        return;

    // Clear OLED
    clearBlack();

    // Reading in button
    int32_t buttonPressed = GPIOPinRead(BUTTONS_GPIO_BASE, ALL_BUTTONS);

    // Displaying information
    switch (~buttonPressed & ALL_BUTTONS) {
        case (LEFT_BUTTON) :
            leftButton.timesPressed++;
            displayInfoOnBoard("Left %d", leftButton.timesPressed, 35, DISPLAY_NUMBER);
            break;
        case (RIGHT_BUTTON) :
            rightButton.timesPressed++;
            displayInfoOnBoard("Right %d", rightButton.timesPressed, 35, DISPLAY_NUMBER);
            break;
        case (UP_BUTTON) :
            upButton.timesPressed++;
            displayInfoOnBoard("Up %d", upButton.timesPressed, 35, DISPLAY_NUMBER);
            break;
        case (DOWN_BUTTON) :
            downButton.timesPressed++;
            displayInfoOnBoard("Down %d", downButton.timesPressed, 35, DISPLAY_NUMBER);
            break;
        case (SELECT_BUTTON) :
            selectButton.timesPressed++;
            displayInfoOnBoard("Select %d", selectButton.timesPressed, 35, DISPLAY_NUMBER);
            break;
        default :
            displayInfoOnBoard("Undefined", -1, 35, DISPLAY_NUMBER);
            break;

    }
    buttonCounter++;
    lastButtonPressStamp = globalFreeRunningCounter;
    displayInfoOnBoard("count %d", buttonCounter, 50, DISPLAY_NUMBER);
}

void IntComp0(void) {
    ComparatorIntClear(COMP_BASE, 0);
    clearBlack();
    timesCrossed1Point6Volts++;
    displayInfoOnBoard("#Crossed %d", timesCrossed1Point6Volts, 25, DISPLAY_NUMBER);
}


void IntUART0(void) {
    uint32_t ui32Status;

    // Get the interrupt status.
    ui32Status = UARTIntStatus(UART0_BASE, true);

    // Clear the asserted interrupts.
    UARTIntClear(UART0_BASE, ui32Status);

    // Get the character from the UART buffer
    while(UARTCharsAvail(UART0_BASE)) {
        menuSelection = tolower((uint8_t)UARTCharGetNonBlocking(UART0_BASE));
    }
}


//****************************************************************************
// Main function
//****************************************************************************
int
main(void)
{
    // Enable lazy stacking for interrupt handlers.  This allows floating-point
    // instructions to be used within interrupt handlers, but at the expense of
    // extra stack usage.
    FPULazyStackingEnable();

    // Set the clocking to run directly from the crystal.
    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    // Initialize the display driver.
    CFAL96x64x16Init();

    // Disabling all Interrupts
    IntMasterDisable();

    // Initialize the graphics context and find the middle X coordinate.
    GrContextInit(&sContext, &g_sCFAL96x64x16);
    GrContextFontSet(&sContext, g_psFontFixed6x8);

    //*************************************************************************
    // Configuration
    //*************************************************************************
    ButtonsInit();

    setupUART();

    setupLED();

    setupComparator();

    //************************************************************************
    // Enable processor interrupts.
    //************************************************************************
    IntMasterEnable();

    //************************************************************************
    // Initializing Variables
    //************************************************************************
    uint32_t blinkingLightCounter = 0;
    bool exitProgram = false;
    bool enableLED = 1;
    leftButton.timesPressed = 0;
    rightButton.timesPressed = 0;
    upButton.timesPressed = 0;
    downButton.timesPressed = 0;
    selectButton.timesPressed = 0;

    //************************************************************************
    // starting functional calls and main while loop
    //************************************************************************

    // Displaying Splash Screen
    diplaySplashOnOLED();

    // Displaying UART Menu
    printMainMenu();


    while(exitProgram == false)
    {
        globalFreeRunningCounter++;
        // Blinking the LED
        if (blinkingLightCounter %
                (FIVE_PERCENT_CYCLE_ON + NIENTYFIVE_PERCENT_CYCLE_OFF) <=
                FIVE_PERCENT_CYCLE_ON && enableLED == 1) {
            GPIOPinWrite(GPIO_PORTG_BASE, GPIO_PIN_2, GPIO_PIN_2);
        }
        else {
            GPIOPinWrite(GPIO_PORTG_BASE, GPIO_PIN_2, 0);
        }

        //********************************************************************
        // Functional calls dependent on menu selection
        //********************************************************************

        switch(menuSelection) {
            case 't' :
                enableLED = !enableLED;
                break;
            case 's' :
                IntMasterDisable();
                clearBlack();
                diplaySplashOnOLED();
                IntMasterEnable();
                break;
            case 'q' :
                exitProgram = true;
                break;
        }
        menuSelection = '\0';
        blinkingLightCounter++;
    }
    clearBlack();
}


//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif
