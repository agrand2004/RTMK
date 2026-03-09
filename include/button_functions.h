#pragma once

#include "at91sam3x8.h"

void initButton(int portNumber, int pinNumber);
void readButton(unsigned int *buttonState, int portNumber, int pinNumber);
