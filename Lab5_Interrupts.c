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
#include "driverlib/timer.h"
#include "driverlib/rom.h"
#include "grlib/grlib.h"
#include "drivers/cfal96x64x16.h"

// Necessary for UART
#include "driverlib/uart.h"
#include <string.h>

// Necessary for lower case string conversion
#include <ctype.h>

// Necessary to write data to a temporary character buffer to ultimately display
#include <stdio.h>

// Necessary for Buttons
#include "drivers/buttons.h"

// Necessary for blinking LED
#include "driverlib/gpio.h"

// Necessary for ADC comparator
#include "driverlib/comp.c"
#include "driverlib/comp.h"


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
static tContext sContext;
static uint32_t countsPerSecond;
static uint32_t characterFromComputer;
static bool enableCounter;
static DisplayMode servicedDisplay;
static DisplayMode requestedDisplay;

//*****************************************************************************
// Prototypes
//*****************************************************************************
void diplayADCInfoOnBoard(uint8_t* formatString,uint32_t ADCValue,
                          uint32_t yLocationOnDisplay, DisplayMode displayMode);
void clearBlack(void);
void UARTSend(const uint8_t *pui8Buffer);
void printMainMenu(void);
void diplaySplashOnOLED(void);


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
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);     // For UART
    SysCtlPeripheralEnable(SYSCTL_PERIPH_COMP0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);      // For LED

    //*************************************************************************
    // Checking if the peripheral is turned on
    //*************************************************************************
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0) ||
            !SysCtlPeripheralReady(SYSCTL_PERIPH_COMP0) ||
            !SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA) ||
            !SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOG));

    //*************************************************************************
    // Configuration
    //*************************************************************************
    // Undergoing on set up steps for the buttons
    ButtonsInit();

    // UART for 115,200, 8-N-1 operation.
    // Configure the UART for 115,200, 8-N-1 operation.
    UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 115200,
                            (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                             UART_CONFIG_PAR_NONE));

    // Configuring the internal reference for the comparator
    ComparatorRefSet(COMP_BASE, COMP_REF_1_65V);

    ComparatorConfigure(COMP_BASE, 0, COMP_TRIG_NONE | COMP_INT_BOTH |
                        COMP_ASRCP_REF | COMP_OUTPUT_NORMAL);

    // Enable the GPIO pin for the LED (PG2).  Set the direction as output, and
    // enable the GPIO pin for digital function.
    GPIOPinTypeGPIOOutput(GPIO_PORTG_BASE, GPIO_PIN_2);

    //************************************************************************
    // Enable processor interrupts.
    //************************************************************************
    IntMasterEnable();

    //
    // Enable the timers.
    //
    TimerEnable(TIMER0_BASE, TIMER_A);
    TimerEnable(TIMER1_BASE, TIMER_A);

    //************************************************************************
    // Initializing Variables
    //************************************************************************
    uint32_t blinkingLightCounter = 0;
    countsPerSecond = 0;
    servicedDisplay = DISPLAY_NUMBER;
    requestedDisplay = DISPLAY_NUMBER;
    bool enableLED = 1;
    enableCounter = 0;

    //************************************************************************
    // starting functional calls and main while loop
    //************************************************************************

    // Displaying Splash Screen
    diplaySplashOnOLED();

    // Displaying UART Menu
    printMainMenu();

    // Clearing interrupt flag for the ADC
    ADCIntClear(ADC0_BASE, ADC_SEQUENCE_3);

    // Start reading the ADC
    ADCProcessorTrigger(ADC0_BASE, ADC_SEQUENCE_3);

    while(tolower(characterFromComputer != 'q'))
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
        switch(tolower(characterFromComputer)) {
            case 't' :
                enableLED = !enableLED;
                break;
            case 's' :
                IntMasterDisable();
                clearBlack();
                diplaySplashOnOLED();
                IntMasterEnable();
                break;
            case 'c' :
                enableCounter = !enableCounter;
                break;
            case '1' :
                // Toggle the display
                requestedDisplay = (requestedDisplay + 1) % DISPLAY_COUNT;
                break;
            case '2' :
                // Toggle the display
                servicedDisplay = (servicedDisplay + 1) % DISPLAY_COUNT;
                break;
        }
        // Changing charter to null manually in case the interrupt does
        // not trigger in time
        characterFromComputer = '\0';

        blinkingLightCounter++;
    }
    clearBlack();
}

// Purpose: Display data to the OLED display
void diplayADCInfoOnBoard(uint8_t* formatString,uint32_t ADCValue,
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

// Print the main menu to the UART Console
void printMainMenu(void) {
    UARTSend("\r\n\nT - Toggle the LED\r\n");
    UARTSend("S - Splash Screen (2s)\r\n");
    UARTSend("1 - Toggle Data Display Requested\n\r");
    UARTSend("2 - Toggle Data Display Serviced\n\r");
    UARTSend("C - Toggle Count Display\n\r");
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
