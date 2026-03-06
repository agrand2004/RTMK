#include "io_functions.h"
#include "kernel_functions.h"

#define LED_PIN_1 1
#define LED_PIN_2 2
#define LED_PIN_3 3
#define LED_PIN_4 4
#define BUTTON_1_PIN 14
#define BUTTON_2_PIN 0


static const int task_1_deadline = 1000;
static const int task_2_deadline = 1000;
static const int task_3_deadline = 1000;
static const int task_4_deadline = 1000;

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

void Task_1() {
    while(1) {
        turn_on_led(1);
        compute_primes();
        for (int i = 0; i < 3; i++) {
            flash_led(1);
        }
        exception r = wait(8000);
        set_deadline(task_1_deadline + ticks());
    }
}

void Task_2() {
    while(1) {
        turn_on_led(2);
        compute_primes();
        for (int i = 0; i < 3; i++) {
            flash_led(2);
        }
        exception r = wait(8000);
        set_deadline(task_2_deadline + ticks());
    }
}

void Task_3() {
    while(1) {
        turn_on_led(3);
        compute_primes();
        for (int i = 0; i < 3; i++) {
            flash_led(3);
        }
        exception r = wait(8000);
        set_deadline(task_3_deadline + ticks());
    }
}

void Task_4() {
    int buttonState = 0;
    while (1) {
        readButton(&buttonState, BUTTON_PIN);
        if (buttonState == 1) {
            flash_led(4);
        }
        exception r = wait(10);
        set_deadline(task_4_deadline + ticks());
    }
}
