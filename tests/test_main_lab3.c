#include "system_sam3x.h"
#include "at91sam3x8.h"
#include "kernel_functions.h"
#include "io_functions.h"

mailbox *input_events;
mailbox *message_mailbox;

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

// * Code for first part
// void Task_2() {
//     while(1) {
//         turn_on_led(2);
//         compute_primes();
//         for (int i = 0; i < 3; i++) {
//             flash_led(2);
//         }
//         exception r = wait(8000);
//         set_deadline(task_2_deadline + ticks());
//     }
// }

void Task_2() {
    bool pressed = 1;
    while(1) {
        turn_on_led(2);
        pressed = 1;
        exception i_e = receive_wait(input_events, &pressed);
        if (i_e == DEADLINE_REACHED) {
            turn_off_led(2);
        } else {
            for (int i = 0; i < 3; i++) {
                flash_led(2);
            }
        }
        exception r = wait(8000);
        set_deadline(task_2_deadline + ticks());
    }
}

// * Code for third part: task 2 sends a message to task 3 when the button is pressed
// void Task_2() {
//     bool pressed = 1;
//     while(1) {
//         turn_on_led(2);
//         pressed = 1;
//         exception i_e = receive_wait(input_events, &pressed);
//         if (i_e == DEADLINE_REACHED) {
//             turn_off_led(2);
//         } else {
//             for (int i = 0; i < 3; i++) {
//                 flash_led(2);
//             }
//             send_no_wait(message_mailbox, 0);
//         }
//         exception r = wait(8000);
//         set_deadline(task_2_deadline + ticks());
//     }
// }

// * Code for first and second part
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

// * Code for the third part: task 3 waits for a message from task 2 before executing
// void Task_3() {
//     while(1) {
//         exception message_received = receive_wait(message_mailbox, NULL);
//         if (message_received == OK) {
//             turn_on_led(3);
//             compute_primes();
//         }
//         for (int i = 0; i < 3; i++) {
//             flash_led(3);
//         }
//         exception r = wait(8000);
//         set_deadline(task_3_deadline + ticks());
//     }
// }

void Task_4() {
    int buttonState = 0;
    while (1) {
        readButton(&buttonState, AT91C_ID_PIOD, BUTTON_2_PIN);
        if (buttonState == 1) {
            flash_led(4);
        }
        exception r = wait(10);
        set_deadline(task_4_deadline + ticks());
    }
}


int main()
{
    SystemInit();
    SysTick_Config(83999);
    exception retVal = init_kernel();
    input_events = create_mailbox(1, sizeof(bool));
    message_mailbox = create_mailbox(1, sizeof(int));
    retVal = create_task( Task_1,  task_1_deadline);
    if ( retVal !=  OK ) { while(1) { /* no use going further */  } }
    retVal = create_task( Task_2,  task_2_deadline);
    if ( retVal !=  OK ) { while(1) { /* no use going further */  } }
    retVal = create_task( Task_3,  task_3_deadline);
    if ( retVal !=  OK ) { while(1) { /* no use going further */  } }
    retVal = create_task( Task_4,  task_4_deadline);
    if ( retVal !=  OK ) { while(1) { /* no use going further */  } }
    setup();
    run();
}
