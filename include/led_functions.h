#ifndef LED_FUNCTION_H
#define LED_FUNCTION_H

#include "at91sam3x8.h"

// TODO: remove those one or add the other
#define LED_PIN 14 // Pin number for the LED (PB14)

void initLed(int portNumber, int pinNumber);

void setLed(int state, int pinNumber);

#endif // LED_FUNCTION_H