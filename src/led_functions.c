#include "led_functions.h"

void initLed(int portNumber, int pinNumber)
{
    // Enable the peripheral clock for PIOC
    AT91C_BASE_PMC->PMC_PCER = (1 << portNumber);

    // Configure PC14 as output (LED on Arduino Due)
    AT91C_BASE_PIOC->PIO_OER = (1 << pinNumber); // Enable output on PC14
    AT91C_BASE_PIOC->PIO_PER = (1 << pinNumber); // Enable PIO control on PC14
}

void setLed(int state, int pinNumber)
{
    if (state)
    {
        AT91C_BASE_PIOC->PIO_SODR = (1 << pinNumber); // Set Output Data Register - LED ON
    }
    else
    {
        AT91C_BASE_PIOC->PIO_CODR = (1 << pinNumber); // Clear Output Data Register - LED OFF
    }
}
