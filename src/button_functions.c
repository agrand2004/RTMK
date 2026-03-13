#include "button_functions.h"
#include "core_cm3.h"

void initButton(int portNumber, int pinNumber)
{
    // Enable the peripheral clock for the corresponding port
    AT91C_BASE_PMC->PMC_PCER = (1 << portNumber);

    // Configure PD1 as input (Button on Arduino Due)
    if (portNumber == AT91C_ID_PIOA) {
        AT91C_BASE_PIOA->PIO_ODR = (1 << pinNumber); // Disable output
        AT91C_BASE_PIOA->PIO_PER = (1 << pinNumber); // Enable PIO control
        // Enable pull-up resistor on PA14
        AT91C_BASE_PIOA->PIO_PPUER = (1 << pinNumber);
        initButtonInterrupt(AT91C_ID_PIOA, pinNumber);
    } else if (portNumber == AT91C_ID_PIOD) {
        AT91C_BASE_PIOD->PIO_ODR = (1 << pinNumber); // Disable output
        AT91C_BASE_PIOD->PIO_PER = (1 << pinNumber); // Enable PIO control
        // Enable pull-up resistor on PD1
        AT91C_BASE_PIOD->PIO_PPUER = (1 << pinNumber);
    }
}

// init the NVIC to enable PIOD interrupts on button press
void initButtonInterrupt(int portNumber, int pinNumber)
{
    // Enable hardware input filter (glitch filter)
    // This provides basic hardware debouncing
    if (portNumber == AT91C_ID_PIOA) {
        AT91C_BASE_PIOA->PIO_IFER = (1 << pinNumber); // Enable input filter
        // Enable interrupt for this pin
        // Interrupt will trigger on any change (press or release)
        AT91C_BASE_PIOA->PIO_IER = (1 << pinNumber); // Enable interrupt on PA14
        // Enable PIOA interrupt in NVIC (Nested Vectored Interrupt Controller)
        NVIC_SetPriority(portNumber, 1);
        *AT91C_PIOA_ISR;
        NVIC_EnableIRQ(portNumber); // Enable PIOA interrupt in NVIC
    } else if (portNumber == AT91C_ID_PIOD) {
        AT91C_BASE_PIOD->PIO_IFER = (1 << pinNumber); // Enable input filter
        // Enable interrupt for this pin
        // Interrupt will trigger on any change (press or release)
        AT91C_BASE_PIOD->PIO_IER = (1 << pinNumber); // Enable interrupt on PD1

        // Enable PIOD interrupt in NVIC (Nested Vectored Interrupt Controller)
        NVIC_EnableIRQ(portNumber); // Enable interrupt in NVIC on the correct port
    }
}

void readButton(unsigned int *buttonState,int portNumber, int pinNumber)
{
    // Read the state of the button
    if (portNumber == AT91C_ID_PIOA) {
        if (AT91C_BASE_PIOA->PIO_PDSR & (1 << pinNumber)) {
            *buttonState = 0; // Button not pressed
        } else {
            *buttonState = 1; // Button pressed
        }
    } else if (portNumber == AT91C_ID_PIOD) {
        if (AT91C_BASE_PIOD->PIO_PDSR & (1 << pinNumber)) {
            *buttonState = 0; // Button not pressed
        } else {
            *buttonState = 1; // Button pressed
        }
    }
}

void PIOA_Handler(void)
{
    ButtonHandler(AT91C_ID_PIOA);
}

void PIOD_Handler(void)
{
    ButtonHandler(AT91C_ID_PIOD);
}