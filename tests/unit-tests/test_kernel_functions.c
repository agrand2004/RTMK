#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "../../include/kernel_functions.h"
#include "../../include/linked_list.h"
#include "../../include/tcb_functions.h"

static int g_tests_run = 0;
static int g_tests_failed = 0;

static int g_isr_off_calls = 0;
static int g_switch_context_calls = 0;
static int g_load_run_calls = 0;
static int g_switch_stack_calls = 0;
static int g_load_terminate_calls = 0;

#define EXPECT(condition, message)                              \
    do                                                          \
    {                                                           \
        g_tests_run++;                                          \
        if (!(condition))                                       \
        {                                                       \
            g_tests_failed++;                                   \
            printf("[FAIL] %s (line %d)\n", message, __LINE__); \
        }                                                       \
    } while (0)

/* Placeholder stubs for hardware/context functions used by kernel_function.c */
void isr_off(void)
{
    g_isr_off_calls++;
}

void isr_on(void)
{
}

void SwitchContext(void)
{
    g_switch_context_calls++;
}

void LoadContext_In_Run(void)
{
    g_load_run_calls++;
}

void switch_to_stack_of_next_task(void)
{
    g_switch_stack_calls++;
}

void LoadContext_In_Terminate(void)
{
    g_load_terminate_calls++;
}

static void task_a(void)
{
}

static void task_b(void)
{
}

static void reset_mock_counters(void)
{
    g_isr_off_calls = 0;
    g_switch_context_calls = 0;
    g_load_run_calls = 0;
    g_switch_stack_calls = 0;
    g_load_terminate_calls = 0;
}

static int count_nodes(list *l)
{
    int count = 0;
    listobj *it = l ? l->pHead : NULL;
    while (it)
    {
        count++;
        it = it->pNext;
    }
    return count;
}

static void free_kernel_list(list **plist)
{
    listobj *it;
    if (!plist || !*plist)
        return;

    it = (*plist)->pHead;
    while (it)
    {
        listobj *next = it->pNext;
        if (it->pTask)
            free_TCB(it->pTask);
        free_listobj(it);
        it = next;
    }

    free(*plist);
    *plist = NULL;
}

static void reset_kernel_state(void)
{
    free_kernel_list(&ReadyList);
    free_kernel_list(&WaitingList);
    free_kernel_list(&TimerList);
    PreviousTask = NULL;
    NextTask = NULL;
    Ticks = 0;
    KernelMode = INIT;
}

static void test_init_kernel(void)
{
    exception ret = init_kernel();
    EXPECT(ret == OK, "init_kernel should return OK");
    EXPECT(Ticks == 0, "init_kernel should reset Ticks to 0");
    EXPECT(KernelMode == INIT, "init_kernel should set KernelMode to INIT");
    EXPECT(ReadyList != NULL, "ReadyList should be allocated");
    EXPECT(WaitingList != NULL, "WaitingList should be allocated");
    EXPECT(TimerList != NULL, "TimerList should be allocated");

    if (ReadyList)
    {
        EXPECT(ReadyList->pHead != NULL, "ReadyList should contain idle task");
        EXPECT(ReadyList->pHead == ReadyList->pTail, "ReadyList should contain one idle task initially");
        if (ReadyList->pHead && ReadyList->pHead->pTask)
            EXPECT(ReadyList->pHead->pTask->Deadline == UINT_MAX, "idle task deadline should be UINT_MAX");
    }

    reset_kernel_state();
}

static void test_create_task_init_mode_sorting(void)
{
    listobj *first;
    listobj *second;
    listobj *third;

    EXPECT(init_kernel() == OK, "init_kernel should succeed before create_task tests");

    EXPECT(create_task(task_a, 50) == OK, "create_task should succeed in INIT mode");
    EXPECT(create_task(task_b, 10) == OK, "create_task should succeed with smaller deadline");

    EXPECT(count_nodes(ReadyList) == 3, "ReadyList should contain 2 tasks + idle");

    first = ReadyList->pHead;
    second = first ? first->pNext : NULL;
    third = second ? second->pNext : NULL;

    EXPECT(first != NULL && second != NULL && third != NULL, "ReadyList should contain three linked nodes");
    if (first && second && third)
    {
        EXPECT(first->pTask->Deadline == 10, "first task should have earliest deadline");
        EXPECT(second->pTask->Deadline == 50, "second task should have next deadline");
        EXPECT(third->pTask->Deadline == UINT_MAX, "idle task should remain last");
    }

    reset_kernel_state();
}

static void test_run_and_create_task_running_mode(void)
{
    TCB *old_next;

    EXPECT(init_kernel() == OK, "init_kernel should succeed before run test");
    EXPECT(create_task(task_a, 20) == OK, "create_task should add task before run");

    reset_mock_counters();
    run();

    EXPECT(KernelMode == RUNNING, "run should set KernelMode to RUNNING");
    EXPECT(Ticks == 0, "run should reset Ticks to 0");
    EXPECT(g_load_run_calls == 1, "run should call LoadContext_In_Run placeholder");
    EXPECT(NextTask == ReadyList->pHead->pTask, "run should set NextTask from ReadyList head");

    old_next = NextTask;
    EXPECT(create_task(task_b, 5) == OK, "create_task should succeed in RUNNING mode");
    EXPECT(g_isr_off_calls == 1, "create_task in RUNNING mode should call isr_off once");
    EXPECT(g_switch_context_calls == 1, "create_task in RUNNING mode should call SwitchContext once");
    EXPECT(PreviousTask == old_next, "create_task should set PreviousTask to old NextTask");
    EXPECT(NextTask->Deadline == 5, "NextTask should become new earliest deadline task");

    reset_kernel_state();
}


static void test_small_helpers(void)
{
    TCB tcb;

    Ticks = 100;
    tcb.Deadline = 80;
    EXPECT(isDeadlineReached(&tcb) == TRUE, "isDeadlineReached should be true when ticks > deadline");

    tcb.Deadline = 100;
    EXPECT(isDeadlineReached(&tcb) == TRUE, "isDeadlineReached should be true when ticks == deadline");

    tcb.Deadline = 120;
    EXPECT(isDeadlineReached(&tcb) == FALSE, "isDeadlineReached should be false when ticks < deadline");

    NextTask = &tcb;
    PreviousTask = NULL;
    update_previous_task();
    EXPECT(PreviousTask == NextTask, "update_previous_task should copy NextTask into PreviousTask");
}

int main(void)
{
    printf("Running kernel_function unit tests...\n");

    reset_kernel_state();
    reset_mock_counters();

    test_init_kernel();
    test_create_task_init_mode_sorting();
    test_run_and_create_task_running_mode();
    test_small_helpers();

    reset_kernel_state();

    printf("Tests run: %d\n", g_tests_run);
    printf("Tests failed: %d\n", g_tests_failed);

    if (g_tests_failed == 0)
    {
        printf("All kernel_function tests passed.\n");
        return 0;
    }

    printf("Some kernel_function tests failed.\n");
    return 1;
}