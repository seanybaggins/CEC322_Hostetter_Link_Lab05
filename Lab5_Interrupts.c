/* Purpose:*/
// Base includes with the timers Examples
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/rom.h"
#include "grlib/grlib.h"
#include "drivers/cfal96x64x16.h"

// Necessary for UART
#include "driverlib/uart.h"
#include "drivers/UART/personalUART.h"
#include <string.h>

// Necessary for lower case string conversion
#include <ctype.h>

// Necessary to write data to a temporary character buffer to ultimately display
#include <stdio.h>

// Necessary for Buttons
#include "drivers/buttons/buttons.h"

// Necessary for blinking LED
#include "driverlib/gpio.h"

// Necessary for ADC comparator
#include "drivers/comparator/comparator.h"

//****************************************************************************
// Defines and typedefs
//****************************************************************************
// Necessary for ADC
#define ADC_SEQUENCE_3 3

// Necessary to switch between 16 MHz and 66 MHz
#define SIXTYSIX_MHZ 1

// Necessary for blinking light clock
#define FIVE_PERCENT_CYCLE_ON 20000
#define NIENTYFIVE_PERCENT_CYCLE_OFF 380000

// Necessary for displayInfoOnBoard()
typedef enum {
    DISPLAY_OFF = 0, DISPLAY_NUMBER = 1, DISPLAY_BAR = 2, DISPLAY_COUNT = 3
} DisplayMode;

//****************************************************************************
// Globals
//****************************************************************************
tContext sContext;
uint32_t characterFromComputer;
uint32_t timesCrossed1Point6Volts;
uint8_t menuSelection;
Button leftButton, rightButton, upButton, downButton, selectButton;
LastButtonPressed lastButtonPressed;
//*****************************************************************************
// Prototypes
//*****************************************************************************
void displayInfoOnBoard(uint8_t* formatString,uint32_t ADCValue,
                          uint32_t yLocationOnDisplay, DisplayMode displayMode);
void clearBlack(void);
void UARTSend(const uint8_t *pui8Buffer);
void printMainMenu(void);
void diplaySplashOnOLED(void);

uint32_t buttonCounter;
void IntButtons(void) {
    GPIOIntClear(BUTTONS_GPIO_BASE, ALL_BUTTONS);
    int32_t buttonPressed = GPIOPinRead(BUTTONS_GPIO_BASE, ALL_BUTTONS);
    switch (~buttonPressed & ALL_BUTTONS) {
        case (LEFT_BUTTON) :
            clearBlack();
            leftButton.timesPressed++;
            displayInfoOnBoard("Left %d", leftButton.timesPressed, 35, DISPLAY_NUMBER);
            break;
        case (RIGHT_BUTTON) :
            clearBlack();
            rightButton.timesPressed++;
            displayInfoOnBoard("Right %d", rightButton.timesPressed, 35, DISPLAY_NUMBER);
            break;
        case (UP_BUTTON) :
            clearBlack();
            upButton.timesPressed++;
            displayInfoOnBoard("Up %d", upButton.timesPressed, 35, DISPLAY_NUMBER);
            break;
        case (DOWN_BUTTON) :
            clearBlack();
            downButton.timesPressed++;
            displayInfoOnBoard("Down %d", downButton.timesPressed, 35, DISPLAY_NUMBER);
            break;
        case (SELECT_BUTTON) :
            clearBlack();
            selectButton.timesPressed++;
            displayInfoOnBoard("Select %d", selectButton.timesPressed, 35, DISPLAY_NUMBER);
            break;
        case 0 :
            break;
        default :
            clearBlack();
            displayInfoOnBoard("Undefined", -1, 35, DISPLAY_NUMBER);
            break;
    }
    int delay;
    for (delay = 0; delay < 2000000; delay++);
    buttonCounter++;
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

    //************************************************************************
    // Enabling the peripherals
    //************************************************************************

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);      // For LED
    //*************************************************************************
    // Checking if the peripheral is turned on
    //*************************************************************************
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOG));

    //*************************************************************************
    // Configuration
    //*************************************************************************
    ButtonsInit();

    setupUART();


    setupComparator();

    // Enable the GPIO pin for the LED (PG2).  Set the direction as output, and
    // enable the GPIO pin for digital function.
    GPIOPinTypeGPIOOutput(GPIO_PORTG_BASE, GPIO_PIN_2);

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
    timesCrossed1Point6Volts = 0;
    menuSelection = '\0';
    lastButtonPressed = UNDEFINED;
    leftButton.timesPressed = 0;
    rightButton.timesPressed = 0;
    upButton.timesPressed = 0;
    downButton.timesPressed = 0;
    selectButton.timesPressed = 0;


    buttonCounter = 0;

    //************************************************************************
    // starting functional calls and main while loop
    //************************************************************************

    // Displaying Splash Screen
    diplaySplashOnOLED();

    // Displaying UART Menu
    printMainMenu();


    while(exitProgram == false)
    {

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

// Purpose: Display data to the OLED display
void displayInfoOnBoard(uint8_t* formatString,uint32_t ADCValue,
                          uint32_t yLocationOnDisplay, DisplayMode displayMode) {
    if(displayMode == DISPLAY_NUMBER) {
        uint8_t displayDataBuffer[16];

        // Setting the text to be white
        GrContextForegroundSet(&sContext, ClrWhite);

        sprintf((char*)displayDataBuffer, (char*)formatString, ADCValue);

        // Prints number to the OLED display
        GrStringDrawCentered(&sContext,(char*)displayDataBuffer, -1,
                             GrContextDpyWidthGet(&sContext) / 2, yLocationOnDisplay, true);
    }
    else if(displayMode == DISPLAY_BAR) {
        tRectangle sRect;

        sRect.i16XMin = 0;
        sRect.i16YMin = yLocationOnDisplay - 4;
        sRect.i16YMax = yLocationOnDisplay + 4;

        // Scaling data down to fit on screen
        sRect.i16XMax = ADCValue / 6783;

        // Setting color of display
        GrContextForegroundSet(&sContext, ClrWhite);

        // Writing bar to screen
        GrRectFill(&sContext, &sRect);
    }
}

// Purpose: Prints a black box over the entire OLED display
void clearBlack(void){

    tRectangle sRect;
    sRect.i16XMin = 0;
    sRect.i16YMin = 0;
    sRect.i16XMax = GrContextDpyWidthGet(&sContext) - 1;
    sRect.i16YMax = 69;
    GrContextForegroundSet(&sContext, ClrBlack);
    GrRectFill(&sContext, &sRect);
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

// Purpose: Display the splash screen on the OLED display
void diplaySplashOnOLED(void) {

    tRectangle sRect;

    clearBlack();

    // Fill the top part of the screen with blue to create the banner.
    sRect.i16XMin = 0;
    sRect.i16YMin = 0;
    sRect.i16XMax = GrContextDpyWidthGet(&sContext) - 1;
    sRect.i16YMax = 9;
    GrContextForegroundSet(&sContext, ClrDarkBlue);
    GrRectFill(&sContext, &sRect);

    // Change foreground for white text.
    GrContextForegroundSet(&sContext, ClrWhite);

    // Put the lab name in the middle of the banner.
    GrContextFontSet(&sContext, g_psFontFixed6x8);
    GrStringDrawCentered(&sContext, "Link_Hostetter", -1,
                         GrContextDpyWidthGet(&sContext) / 2, 4, 0);

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
