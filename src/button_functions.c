#include "button_functions.h"

void initButton(int portNumber, int pinNumber)
{
    // Enable the peripheral clock for PIOD
    AT91C_BASE_PMC->PMC_PCER = (1 << portNumber);

    // Configure PD1 as input (Button on Arduino Due)
    AT91C_BASE_PIOD->PIO_ODR = (1 << pinNumber); // Disable output
    AT91C_BASE_PIOD->PIO_PER = (1 << pinNumber); // Enable PIO control
    // Enable pull-up resistor on PD1
    AT91C_BASE_PIOD->PIO_PPUER = (1 << pinNumber);

    initButtonInterrupt(portNumber, pinNumber);
}

// init the NVIC to enable PIOD interrupts on button press
void initButtonInterrupt(int portNumber, int pinNumber)
{
    // Enable hardware input filter (glitch filter)
    // This provides basic hardware debouncing
    AT91C_BASE_PIOD->PIO_IFER = (1 << pinNumber); // Enable input filter

    // Enable interrupt for this pin
    // Interrupt will trigger on any change (press or release)
    AT91C_BASE_PIOD->PIO_IER = (1 << pinNumber); // Enable interrupt on PD1

    // Enable PIOD interrupt in NVIC (Nested Vectored Interrupt Controller)
    NVIC_EnableIRQ(portNumber); // Enable PIOD interrupt in NVIC
}

void readButton(unsigned int *buttonState, int pinNumber)
{
    // Read the state of PD1
    if (AT91C_BASE_PIOD->PIO_PDSR & (1 << pinNumber))
    {
        *buttonState = 0; // Button not pressed
    }
    else
    {
        *buttonState = 1; // Button pressed
    }
}