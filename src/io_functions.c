#include "io_functions.h"
#include "kernel_functions.h"

static const int task_1_deadline = 1000;
static const int task_2_deadline = 1000;
static const int task_3_deadline = 1000;
static const int task_4_deadline = 1000;

void setup()
{
    // Init all the leds
    // Init the buttons
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
