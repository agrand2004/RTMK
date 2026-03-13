#pragma once

#include "led_functions.h"
#include "button_functions.h"
#include "kernel_functions.h"

#define LED_PIN_1 1
#define LED_PIN_2 2
#define LED_PIN_3 3
#define LED_PIN_4 4
#define BUTTON_1_PIN 14
#define BUTTON_2_PIN 0
#define task_1_deadline 10000
#define task_2_deadline 10000
#define task_3_deadline 10000
#define task_4_deadline 10000

extern mailbox *input_events;

void setup();
void turn_on_led(int index);
void turn_off_led(int index);
void flash_led(int index);
void compute_primes();
void Task_1();
void Task_2();
void Task_3();
void Task_4();
