#pragma once

#include "at91sam3x8.h"

// TODO: remove those one or add the other
#define BUTTON_PIN 1 // Pin number for the button (PD1)
#define ID_PIOD 16 // Peripheral ID for PIOD

void initButton(int portNumber, int pinNumber);
void readButton(unsigned int *buttonState, int pinNumber);
