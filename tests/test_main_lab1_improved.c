/*
 * Improved integration test for lab 1 kernel behavior.
 *
 * Focus:
 * 1) init_kernel list state
 * 2) create_task TCB/stack initialization in INIT mode
 * 3) runtime create_task preemption behavior in RUNNING mode
 * 4) terminate path under repeated task execution
 *
 * This file keeps the original intent from test_main_lab1.c
 * but uses clearer variable names and explicit failure codes.
 */

#include "system_sam3x.h"
#include "at91sam3x8.h"
#include "kernel_functions.h"

#define MAIN_CREATION_COUNT 10
#define RUNTIME_CREATION_COUNT 10

#define BASE_RELATIVE_DEADLINE 1000U
#define RUNTIME_RELATIVE_DEADLINE 100000U

/* Failure codes to identify where the test stopped */
#define FAIL_INIT_KERNEL_RETURN 10U
#define FAIL_INIT_READYLIST_SIZE 11U
#define FAIL_INIT_WAITINGLIST_SIZE 12U
#define FAIL_INIT_TIMERLIST_SIZE 13U

#define FAIL_CREATE_FROM_MAIN_RETURN 20U
#define FAIL_STACK_POINTER_RANGE 21U
#define FAIL_STACKED_PC_MISMATCH 22U
#define FAIL_STACKED_SPSR_MISMATCH 23U
#define FAIL_ZERO_PC 24U
#define FAIL_DEADLINE_MISMATCH 25U

#define FAIL_RUNTIME_CREATE_RETURN 30U
#define FAIL_RUNTIME_NO_IMMEDIATE_PROGRESS 31U
#define FAIL_RUNTIME_DEPTH_MISMATCH 32U
#define FAIL_RUNTIME_TASK_SETUP 33U

static volatile unsigned int g_failure_code = 0U;

/* Stage gates: multiplied together so any failure forces 0 */
static volatile unsigned int g_init_stage_ok = OK;
static volatile unsigned int g_create_stage_ok = OK;
static volatile unsigned int g_runtime_step_ok = OK;
static volatile unsigned int g_runtime_stage_ok = OK;

static void fail_and_halt(unsigned int failure_code)
{
    g_failure_code = failure_code;
    while (1)
    {
        /* Failure state */
    }
}

static void assert_or_fail(int condition, unsigned int failure_code)
{
    if (!condition)
    {
        fail_and_halt(failure_code);
    }
}

void task_terminates_immediately(void)
{
    terminate();
}

void task_runtime_recursive_creator(void)
{
    exception create_result;
    unsigned int runtime_entries_before_create;
    static unsigned int runtime_creations_left = RUNTIME_CREATION_COUNT;
    static unsigned int runtime_entry_count = 0;

    runtime_entry_count++;
    runtime_creations_left--;

    if (runtime_creations_left > 0)
    {
        runtime_entries_before_create = runtime_entry_count;
        create_result = create_task(
            task_runtime_recursive_creator,
            RUNTIME_RELATIVE_DEADLINE - 10U * runtime_entry_count);

        if (create_result == OK)
        {
            /*
             * If create_task preempts correctly, the tighter-deadline task should
             * run immediately and increase runtime_entry_count before we continue.
             */
            if (runtime_entry_count <= runtime_entries_before_create)
            {
                g_runtime_step_ok = FAIL;
                fail_and_halt(FAIL_RUNTIME_NO_IMMEDIATE_PROGRESS);
            }
            else
            {
                g_runtime_step_ok = OK;
            }
        }
        else
        {
            g_runtime_step_ok = FAIL;
            fail_and_halt(FAIL_RUNTIME_CREATE_RETURN);
        }
    }
    else
    {
        if (runtime_entry_count == RUNTIME_CREATION_COUNT)
        {
            g_runtime_step_ok = OK;
        }
        else
        {
            g_runtime_step_ok = FAIL;
            fail_and_halt(FAIL_RUNTIME_DEPTH_MISMATCH);
        }
    }

    g_runtime_stage_ok = g_runtime_stage_ok * g_runtime_step_ok;
    terminate();
}

void task_final_verdict(void)
{
    if (g_runtime_stage_ok == OK)
    {
        while (1)
        {
            /* PASS state */
        }
    }
    else
    {
        fail_and_halt(FAIL_RUNTIME_TASK_SETUP);
    }
}

void main(void)
{
    unsigned int create_index;
    unsigned int *stack_pointer;
    unsigned int *stack_low_bound;
    unsigned int *stack_high_bound;
    unsigned int *stacked_pc_address;
    unsigned int *stacked_spsr_address;
    listobj *created_task_node = 0;
    exception result;

    SystemInit();
    SysTick_Config(100000);
    SCB->SHP[((uint32_t)(SysTick_IRQn) & 0xFU) - 4U] = (0xE0U);
    isr_off();

    g_init_stage_ok = OK;

    result = init_kernel();
    if (result != OK)
    {
        g_init_stage_ok = FAIL;
        fail_and_halt(FAIL_INIT_KERNEL_RETURN);
    }

    if (ReadyList->pHead != ReadyList->pTail)
    {
        g_init_stage_ok = FAIL;
        fail_and_halt(FAIL_INIT_READYLIST_SIZE);
    }
    if (WaitingList->pHead != WaitingList->pTail)
    {
        g_init_stage_ok = FAIL;
        fail_and_halt(FAIL_INIT_WAITINGLIST_SIZE);
    }
    if (TimerList->pHead != TimerList->pTail)
    {
        g_init_stage_ok = FAIL;
        fail_and_halt(FAIL_INIT_TIMERLIST_SIZE);
    }

    assert_or_fail(g_init_stage_ok == OK, FAIL_INIT_KERNEL_RETURN);

    g_create_stage_ok = OK;
    for (create_index = 1U; create_index <= MAIN_CREATION_COUNT; create_index++)
    {
        result = create_task(task_terminates_immediately, BASE_RELATIVE_DEADLINE + create_index);
        if (result != OK)
        {
            g_create_stage_ok = FAIL;
            fail_and_halt(FAIL_CREATE_FROM_MAIN_RETURN);
        }

        if (create_index == 1U)
        {
            created_task_node = ReadyList->pHead;
        }
        else
        {
            created_task_node = created_task_node->pNext;
        }

        stack_pointer = created_task_node->pTask->SP;
        stack_low_bound = &(created_task_node->pTask->StackSeg[0]);
        stack_high_bound = &(created_task_node->pTask->StackSeg[STACK_SIZE - 1]);

        if (stack_low_bound >= stack_pointer || stack_pointer >= stack_high_bound)
        {
            g_create_stage_ok = FAIL;
            fail_and_halt(FAIL_STACK_POINTER_RANGE);
        }

        stacked_pc_address = (unsigned int *)((unsigned int)stack_pointer + 24U);
        stacked_spsr_address = (unsigned int *)((unsigned int)stack_pointer + 28U);

        if ((unsigned int)created_task_node->pTask->PC != (unsigned int)(*stacked_pc_address))
        {
            g_create_stage_ok = FAIL;
            fail_and_halt(FAIL_STACKED_PC_MISMATCH);
        }

        if ((unsigned int)created_task_node->pTask->SPSR != (unsigned int)(*stacked_spsr_address))
        {
            g_create_stage_ok = FAIL;
            fail_and_halt(FAIL_STACKED_SPSR_MISMATCH);
        }

        if ((unsigned int)created_task_node->pTask->PC == 0U)
        {
            g_create_stage_ok = FAIL;
            fail_and_halt(FAIL_ZERO_PC);
        }

        if ((unsigned int)created_task_node->pTask->Deadline !=
            (unsigned int)(BASE_RELATIVE_DEADLINE + create_index))
        {
            g_create_stage_ok = FAIL;
            fail_and_halt(FAIL_DEADLINE_MISMATCH);
        }
    }

    assert_or_fail(g_create_stage_ok == OK, FAIL_CREATE_FROM_MAIN_RETURN);

    g_runtime_stage_ok = 1U;
    g_runtime_step_ok = 0U;

    result = create_task(task_runtime_recursive_creator, RUNTIME_RELATIVE_DEADLINE);
    if (result != OK)
    {
        fail_and_halt(FAIL_RUNTIME_TASK_SETUP);
    }

    result = create_task(task_final_verdict, 8U * RUNTIME_RELATIVE_DEADLINE);
    if (result != OK)
    {
        fail_and_halt(FAIL_RUNTIME_TASK_SETUP);
    }

    run();

    fail_and_halt(FAIL_RUNTIME_TASK_SETUP);
}
