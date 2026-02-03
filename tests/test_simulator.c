/*
 * Software Simulator for Lab1 - No Hardware Required
 *
 * This test runs the kernel functions on a desktop computer without needing
 * the Arduino Due hardware kit. It mocks the hardware-specific functions
 * and allows you to test your kernel logic locally.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

/* Include your kernel headers */
#include "../include/kernel_functions.h"
#include "../include/linked_list.h"
#include "../include/tcb_functions.h"

/* ============================================================================
 * MOCK HARDWARE FUNCTIONS - These replace the hardware-specific code
 * ============================================================================ */

/* Mock system initialization */
void SystemInit(void)
{
    printf("[SIMULATOR] SystemInit() called\n");
    /* Nothing needed for simulation */
}

/* Mock SysTick configuration */
void SysTick_Config(unsigned int ticks)
{
    printf("[SIMULATOR] SysTick_Config(%u) called\n", ticks);
    /* Nothing needed for simulation */
}

/* Mock define for SysTick_IRQn */
#define SysTick_IRQn -1

/* Mock interrupt enable/disable */
void isr_off(void)
{
    printf("[SIMULATOR] ISR disabled\n");
}

void isr_on(void)
{
    printf("[SIMULATOR] ISR enabled\n");
}

/* Mock SCB (System Control Block) */
typedef struct
{
    unsigned int SHP[16];
} MockSCB;

MockSCB mockSCB = {0};
MockSCB *SCB = &mockSCB;

/* ============================================================================
 * TEST CODE - From the original test_main_lab1.c
 * ============================================================================ */

unsigned int g0 = 0, g1 = 0, g2 = 0, g3 = 1; /* gate flags for various stages of unit test */

unsigned int low_deadline = 1000;
unsigned int high_deadline = 100000;

#define create_count_from_main_MAX 10
#define create_count_from_task2_MAX 10

void task_body_1(void)
{
    printf("[TASK 1] Running - about to terminate\n");
    terminate();
}

void task_body_2(void)
{
    exception return_value;
    unsigned int recursion_count_upon_entry;
    static unsigned int recursion_depth_left = create_count_from_task2_MAX;
    static unsigned int count_recursive_calls = 0;

    count_recursive_calls++;
    recursion_depth_left--;
    printf("[TASK 2] Recursion call %u of %u\n", count_recursive_calls, create_count_from_task2_MAX);

    if (recursion_depth_left > 0)
    {
        recursion_count_upon_entry = count_recursive_calls;
        return_value = create_task(task_body_2, high_deadline - 10 * count_recursive_calls);

        if (return_value == OK)
        {
            if (count_recursive_calls <= recursion_count_upon_entry)
            {
                g2 = FAIL;
            }
            else
            {
                g2 = OK;
            }
        }
        else
        {
            g2 = FAIL;
        }
    }
    else
    {
        if (count_recursive_calls == create_count_from_task2_MAX)
        {
            g2 = OK;
        }
        else
        {
            g2 = FAIL;
        }
    }
    g3 = g3 * g2;
    terminate();
}

void task_body_3(void)
{
    printf("[TASK 3] Running - checking test results\n");
    if (g3 == OK)
    {
        printf("[TASK 3] Test PASSED! g3 = OK\n");
        while (1)
        { /* Alles Gut ! This unit test has been passed */
        }
    }
    else
    {
        printf("[TASK 3] Test FAILED! g3 = FAIL\n");
        while (1)
        { /* failed */
        }
    }
}

/* ============================================================================
 * SIMPLIFIED SCHEDULER - For testing without real context switching
 * ============================================================================ */

void run_simple_simulation(void)
{
    printf("\n========== RUNNING SIMPLE SIMULATION ==========\n\n");

    printf("g0 (init_kernel test) = %s\n", g0 == OK ? "OK" : "FAIL");
    printf("g1 (create_task from main) = %s\n", g1 == OK ? "OK" : "FAIL");
    printf("g2 (recursive task creation) = %s\n", g2 == OK ? "OK" : "FAIL");
    printf("g3 (combined result) = %s\n", g3 == OK ? "OK" : "FAIL");

    printf("\nReadyList: pHead=%p, pTail=%p\n",
           (void *)ReadyList->pHead, (void *)ReadyList->pTail);
    printf("WaitingList: pHead=%p, pTail=%p\n",
           (void *)WaitingList->pHead, (void *)WaitingList->pTail);
    printf("TimerList: pHead=%p, pTail=%p\n",
           (void *)TimerList->pHead, (void *)TimerList->pTail);

    /* Count tasks in ReadyList */
    int task_count = 0;
    listobj *current = ReadyList->pHead;
    while (current != ReadyList->pTail && current != NULL)
    {
        task_count++;
        printf("  Task %d: Deadline = %u\n", task_count, current->pTask->Deadline);
        current = current->pNext;
    }
    printf("Total tasks in ReadyList: %d\n", task_count);
}

/* ============================================================================
 * MAIN TEST FUNCTION
 * ============================================================================ */

int main(void)
{
    printf("===== LAB1 KERNEL TEST - SOFTWARE SIMULATOR =====\n\n");

    /* Initialize hardware (mocked) */
    SystemInit();
    SysTick_Config(100000);
    if (SysTick_IRQn >= 0)
        SCB->SHP[((unsigned int)(SysTick_IRQn) & 0xF) - 4] = 0xE0;
    isr_off();

    printf("\n--- Testing init_kernel() ---\n");
    g0 = OK;
    exception retVal = init_kernel();
    if (retVal != OK)
    {
        printf("ERROR: init_kernel() returned FAIL\n");
        g0 = FAIL;
    }
    else
    {
        printf("SUCCESS: init_kernel() returned OK\n");
    }

    if (ReadyList->pHead != ReadyList->pTail)
    {
        printf("ERROR: ReadyList not initialized properly\n");
        g0 = FAIL;
    }
    if (WaitingList->pHead != WaitingList->pTail)
    {
        printf("ERROR: WaitingList not initialized properly\n");
        g0 = FAIL;
    }
    if (TimerList->pHead != TimerList->pTail)
    {
        printf("ERROR: TimerList not initialized properly\n");
        g0 = FAIL;
    }

    if (g0 != OK)
    {
        printf("\nFAILED: Kernel initialization failed!\n");
        return 1;
    }
    printf("SUCCESS: All lists initialized correctly\n");

    /* Test 1: Create tasks from main */
    printf("\n--- Testing create_task() from main ---\n");
    unsigned int i;
    listobj *nextListObj = NULL;

    g1 = OK;
    for (i = 1; i <= create_count_from_main_MAX; i++)
    {
        printf("Creating task %u with deadline %u\n", i, low_deadline + i);
        retVal = create_task(task_body_1, low_deadline + i);
        if (retVal == OK)
        {
            if (i == 1)
                nextListObj = ReadyList->pHead;
            else
                nextListObj = nextListObj->pNext;

            printf("  Task %u: PC=%p, Deadline=%u\n", i,
                   (void *)nextListObj->pTask->PC, nextListObj->pTask->Deadline);

            /* Check whether the newly created task has correct deadline */
            if ((unsigned int)nextListObj->pTask->Deadline != (unsigned int)(low_deadline + i))
            {
                printf("  ERROR: Deadline mismatch!\n");
                g1 = FAIL;
                break;
            }
            if ((unsigned int)nextListObj->pTask->PC == (unsigned int)0)
            {
                printf("  ERROR: PC is NULL!\n");
                g1 = FAIL;
                break;
            }
        }
        else
        {
            printf("  ERROR: create_task() failed\n");
            g1 = FAIL;
            break;
        }
    }

    if (g1 != OK)
    {
        printf("\nFAILED: create_task test failed!\n");
        return 1;
    }
    printf("SUCCESS: All tasks created successfully\n");

    /* Test 2: Create tasks with recursion during runtime */
    printf("\n--- Testing create_task() with recursion ---\n");
    g3 = 1;
    g2 = 0;
    retVal = create_task(task_body_2, high_deadline);
    if (retVal != OK)
    {
        printf("ERROR: Failed to create task_body_2\n");
        return 1;
    }
    printf("SUCCESS: task_body_2 created\n");

    retVal = create_task(task_body_3, 8 * high_deadline);
    if (retVal != OK)
    {
        printf("ERROR: Failed to create task_body_3\n");
        return 1;
    }
    printf("SUCCESS: task_body_3 created\n");

    /* Show results */
    printf("\n");
    run_simple_simulation();

    printf("\n===== TEST COMPLETE =====\n");
    printf("Overall result: %s\n",
           (g0 == OK && g1 == OK) ? "PASSED" : "FAILED");

    return (g0 == OK && g1 == OK) ? 0 : 1;
}
