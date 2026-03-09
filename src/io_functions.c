#include "io_functions.h"
#include "kernel_functions.h"

void setup()
{
    // Init the leds
    initLed(AT91C_ID_PIOC, LED_PIN_1); // LED1 on PC1
    initLed(AT91C_ID_PIOC, LED_PIN_2); // LED2 on PC2
    initLed(AT91C_ID_PIOC, LED_PIN_3); // LED3 on PC3
    initLed(AT91C_ID_PIOC, LED_PIN_4); // LED4 on PC4
    // Init the buttons
    initButton(AT91C_ID_PIOA, BUTTON_1_PIN);
    initButton(AT91C_ID_PIOD, BUTTON_2_PIN);
}

void turn_on_led(int index)
{
    setLed(1, index);
}

void turn_off_led(int index)
{
    setLed(0, index);
}

void flash_led(int index)
{
    turn_on_led(index);
    for (int i = 0; i < 8000; i++)
        for (int j = 0; j < 100; j++)
            ;
    turn_off_led(index);
}

void compute_primes()
{
    volatile long long x, y, n=4000;
    bool isprime;
    for(x = 2; x < n; x++) {
        isprime = 1;
        for(y=2; y <= x; y++) {
            if((x % y) == 0) {
                isprime = 0;
                break;
            }
        }
    }
}

void ButtonHandler(int port)
{
    unsigned int status;
    int buttonState;

    if (port == AT91C_ID_PIOA) {
        status = AT91C_BASE_PIOA->PIO_ISR;
    } else {
        status = AT91C_BASE_PIOD->PIO_ISR;
    }
    if (status & (1 << BUTTON_1_PIN)) {
        readButton(&buttonState, AT91C_ID_PIOA, BUTTON_1_PIN);
        if (buttonState) {
            send_no_wait(input_events, &buttonState);
        }
    }
}
