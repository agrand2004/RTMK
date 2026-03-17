#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "../../include/kernel_functions.h"
#include "../../include/linked_list.h"
#include "../../include/tcb_functions.h"
#include "../../include/timing_functions.h"

static int g_tests_run = 0;
static int g_tests_failed = 0;

static int g_isr_off_calls = 0;
static int g_switch_context_calls = 0;

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

/* -----------------------------------------------------------------------
 * Placeholder stubs
 * ----------------------------------------------------------------------- */
void isr_off(void) { g_isr_off_calls++; }
void isr_on(void) {}

void SwitchContext(void) { g_switch_context_calls++; }
void LoadContext_In_Run(void) {}
void switch_to_stack_of_next_task(void) {}
void LoadContext_In_Terminate(void) {}

/* -----------------------------------------------------------------------
 * Helpers
 * ----------------------------------------------------------------------- */
static void reset_counters(void)
{
    g_isr_off_calls = 0;
    g_switch_context_calls = 0;
}

static listobj *make_node(uint deadline_val)
{
    listobj *node = (listobj *)calloc(1, sizeof(listobj));
    if (!node)
    {
        perror("calloc");
        exit(2);
    }
    node->pTask = create_TCB();
    if (!node->pTask)
    {
        perror("create_TCB");
        exit(2);
    }
    node->pTask->Deadline = deadline_val;
    return node;
}

static void free_list_nodes(list *l)
{
    listobj *it = l->pHead;
    while (it)
    {
        listobj *next = it->pNext;
        free_TCB(it->pTask);
        free_listobj(it);
        it = next;
    }
    l->pHead = l->pTail = NULL;
}

static void kernel_setup(void)
{
    /* Build minimal kernel state without calling init_kernel (which adds an
       idle task) so tests control the exact list contents. */
    if (ReadyList)
        free_list_nodes(ReadyList), free(ReadyList);
    if (WaitingList)
        free_list_nodes(WaitingList), free(WaitingList);
    if (TimerList)
        free_list_nodes(TimerList), free(TimerList);

    ReadyList = create_list();
    WaitingList = create_list();
    TimerList = create_list();

    PreviousTask = NULL;
    NextTask = NULL;
    Ticks = 0;
    KernelMode = RUNNING;
    reset_counters();
}

static int count_nodes(list *l)
{
    int n = 0;
    listobj *it = l ? l->pHead : NULL;
    while (it)
    {
        n++;
        it = it->pNext;
    }
    return n;
}

/* -----------------------------------------------------------------------
 * Tests for set_ticks / ticks
 * ----------------------------------------------------------------------- */
static void test_set_and_get_ticks(void)
{
    kernel_setup();

    set_ticks(0);
    EXPECT(ticks() == 0, "ticks() should return 0 after set_ticks(0)");

    set_ticks(42);
    EXPECT(ticks() == 42, "ticks() should return 42 after set_ticks(42)");

    set_ticks(1000);
    EXPECT(ticks() == 1000, "ticks() should return 1000 after set_ticks(1000)");
}

/* -----------------------------------------------------------------------
 * Tests for deadline / set_deadline
 * ----------------------------------------------------------------------- */
static void test_deadline_returns_next_task_deadline(void)
{
    listobj *n1;

    kernel_setup();
    n1 = make_node(500);
    insert_deadline_in_list(ReadyList, n1);
    NextTask = n1->pTask;

    EXPECT(deadline() == 500, "deadline() should return NextTask->Deadline");
}

static void test_set_deadline_reschedules(void)
{
    listobj *n1, *n2, *n3;

    kernel_setup();

    n1 = make_node(10);
    n2 = make_node(20);
    n3 = make_node(30);
    insert_deadline_in_list(ReadyList, n1);
    insert_deadline_in_list(ReadyList, n2);
    insert_deadline_in_list(ReadyList, n3);

    /* Simulate that n1 is the currently running task */
    NextTask = n1->pTask;
    reset_counters();

    /* Push n1's deadline to 25 — it should slot between n2 and n3 */
    set_deadline(25);
    EXPECT(g_isr_off_calls == 1, "set_deadline should call isr_off");
    EXPECT(g_switch_context_calls == 1, "set_deadline should call SwitchContext");
    EXPECT(count_nodes(ReadyList) == 3, "ReadyList should still have 3 nodes after set_deadline");
    EXPECT(ReadyList->pHead->pTask->Deadline == 20, "n2 should now be first (deadline 20)");
    EXPECT(ReadyList->pHead->pNext->pTask->Deadline == 25, "rescheduled task should be second (deadline 25)");
    EXPECT(ReadyList->pTail->pTask->Deadline == 30, "n3 should remain last (deadline 30)");

    free_list_nodes(ReadyList);
    free_list_nodes(TimerList);
    free_list_nodes(WaitingList);
}

static void test_set_deadline_to_tail(void)
{
    listobj *n1, *n2, *n3;

    kernel_setup();

    /* set_deadline always pops the HEAD (the currently-running task by RTOS
       convention). n1 is the running task (head). Stretching its deadline to
       35 should push it after both n2 and n3. */
    n1 = make_node(10);
    n2 = make_node(20);
    n3 = make_node(30);
    insert_deadline_in_list(ReadyList, n1);
    insert_deadline_in_list(ReadyList, n2);
    insert_deadline_in_list(ReadyList, n3);

    NextTask = n1->pTask;  /* n1 is the running (head) task */

    set_deadline(35);
    EXPECT(count_nodes(ReadyList) == 3, "node count should stay the same after set_deadline");
    EXPECT(ReadyList->pHead->pTask->Deadline == 20, "n2 should become head after deadline extension");
    EXPECT(ReadyList->pHead->pNext->pTask->Deadline == 30, "n3 should be second");
    EXPECT(ReadyList->pTail->pTask->Deadline == 35, "rescheduled task should be at the tail");

    free_list_nodes(ReadyList);
    free_list_nodes(TimerList);
    free_list_nodes(WaitingList);
}

/* -----------------------------------------------------------------------
 * Tests for wait
 * ----------------------------------------------------------------------- */
static void test_wait_moves_task_to_timer_list(void)
{
    listobj *n1, *n2;

    kernel_setup();

    n1 = make_node(100);
    n2 = make_node(200);
    insert_deadline_in_list(ReadyList, n1);
    insert_deadline_in_list(ReadyList, n2);
    NextTask = n1->pTask;
    reset_counters();

    /* wait(5): n1 should leave ReadyList and land in TimerList with nTCnt=5 */
    wait(5);

    EXPECT(g_isr_off_calls == 1, "wait should call isr_off");
    EXPECT(g_switch_context_calls == 1, "wait should call SwitchContext");
    EXPECT(count_nodes(ReadyList) == 1, "ReadyList should have one remaining task");
    EXPECT(count_nodes(TimerList) == 1, "TimerList should have one task after wait");
    EXPECT(TimerList->pHead->nTCnt == 5, "waiting task nTCnt should match requested ticks");
    EXPECT(TimerList->pHead->pTask == n1->pTask, "waiting task should be the one that called wait");

    free_list_nodes(ReadyList);
    free_list_nodes(TimerList);
    free_list_nodes(WaitingList);
}

static void test_wait_returns_ok_when_no_deadline_overrun(void)
{
    listobj *n1, *n2;
    exception ret;

    kernel_setup();
    /* Ticks=0, deadline far in the future => not reached */
    n1 = make_node(9999);
    n2 = make_node(9999);
    insert_deadline_in_list(ReadyList, n1);
    insert_deadline_in_list(ReadyList, n2);
    NextTask = n1->pTask;

    ret = wait(3);
    EXPECT(ret == OK, "wait should return OK when deadline is not reached");

    free_list_nodes(ReadyList);
    free_list_nodes(TimerList);
    free_list_nodes(WaitingList);
}

static void test_wait_returns_deadline_reached_on_overrun(void)
{
    listobj *n1, *n2;
    exception ret;

    kernel_setup();
    Ticks = 500;
    /* deadline already passed => isDeadlineReached is true */
    n1 = make_node(100);
    n2 = make_node(9999);
    insert_deadline_in_list(ReadyList, n2); /* n1 has smaller deadline: insert sorted */
    insert_deadline_in_list(ReadyList, n1);
    NextTask = n1->pTask;

    ret = wait(3);
    EXPECT(ret == DEADLINE_REACHED, "wait should return DEADLINE_REACHED when deadline already passed");

    free_list_nodes(ReadyList);
    free_list_nodes(TimerList);
    free_list_nodes(WaitingList);
}

/* -----------------------------------------------------------------------
 * Tests for TimerInt / check_timer_list / check_waiting_list
 * ----------------------------------------------------------------------- */
static void test_timerInt_increments_ticks(void)
{
    kernel_setup();
    Ticks = 0;
    TimerInt();
    EXPECT(Ticks == 1, "TimerInt should increment Ticks by 1");
    TimerInt();
    EXPECT(Ticks == 2, "TimerInt should increment Ticks to 2 on second call");
}

static void test_timerInt_moves_expired_timer_task_to_ready(void)
{
    listobj *idle, *waiting;

    kernel_setup();

    /* Put an idle task in ReadyList so update_next_task doesn't crash */
    idle = make_node(UINT_MAX);
    insert_deadline_in_list(ReadyList, idle);
    NextTask = idle->pTask;

    /* A task that is waiting for 1 tick */
    waiting = make_node(9999);
    waiting->nTCnt = 1;
    list_push_back(TimerList, waiting);

    TimerInt(); /* nTCnt goes to 0 → task should move to ReadyList */

    EXPECT(count_nodes(TimerList) == 0, "TimerList should be empty after timer expires");
    EXPECT(count_nodes(ReadyList) == 2, "expired task should be moved to ReadyList");

    free_list_nodes(ReadyList);
    free_list_nodes(TimerList);
    free_list_nodes(WaitingList);
}

static void test_timerInt_keeps_non_expired_timer_task(void)
{
    listobj *idle, *waiting;

    kernel_setup();

    idle = make_node(UINT_MAX);
    insert_deadline_in_list(ReadyList, idle);
    NextTask = idle->pTask;

    /* A task waiting for 5 ticks — should stay */
    waiting = make_node(9999);
    waiting->nTCnt = 5;
    list_push_back(TimerList, waiting);

    TimerInt(); /* nTCnt decremented to 4, not yet expired */

    EXPECT(count_nodes(TimerList) == 1, "TimerList should still have the task (not expired)");
    EXPECT(TimerList->pHead->nTCnt == 4, "nTCnt should be decremented by 1 each TimerInt");
    EXPECT(count_nodes(ReadyList) == 1, "ReadyList should only contain the idle task");

    free_list_nodes(ReadyList);
    free_list_nodes(TimerList);
    free_list_nodes(WaitingList);
}

static void test_timerInt_moves_deadline_overrun_waiting_task_to_ready(void)
{
    listobj *idle, *blocked;

    kernel_setup();
    Ticks = 0;

    idle = make_node(UINT_MAX);
    insert_deadline_in_list(ReadyList, idle);
    NextTask = idle->pTask;

    /* A task in WaitingList whose deadline has already passed */
    blocked = make_node(1); /* deadline=1 */
    list_push_back(WaitingList, blocked);

    TimerInt(); /* Ticks becomes 1 → isDeadlineReached(blocked) => true */

    EXPECT(count_nodes(WaitingList) == 0, "WaitingList should be empty after deadline overrun");
    EXPECT(count_nodes(ReadyList) == 2, "overdue blocked task should move to ReadyList");
    EXPECT(blocked->pMessage == NULL, "pMessage should be cleared for woken blocked task");

    free_list_nodes(ReadyList);
    free_list_nodes(TimerList);
    free_list_nodes(WaitingList);
}

static void test_timerInt_keeps_non_expired_waiting_task(void)
{
    listobj *idle, *blocked;

    kernel_setup();
    Ticks = 0;

    idle = make_node(UINT_MAX);
    insert_deadline_in_list(ReadyList, idle);
    NextTask = idle->pTask;

    /* Deadline far in the future — should stay in WaitingList */
    blocked = make_node(9999);
    list_push_back(WaitingList, blocked);

    TimerInt();

    EXPECT(count_nodes(WaitingList) == 1, "WaitingList task with future deadline should not move");
    EXPECT(count_nodes(ReadyList) == 1, "ReadyList should still have only the idle task");

    free_list_nodes(ReadyList);
    free_list_nodes(TimerList);
    free_list_nodes(WaitingList);
}

/* -----------------------------------------------------------------------
 * main
 * ----------------------------------------------------------------------- */
int main(void)
{
    printf("Running timing_functions unit tests...\n");

    test_set_and_get_ticks();
    test_deadline_returns_next_task_deadline();
    test_set_deadline_reschedules();
    test_set_deadline_to_tail();
    test_wait_moves_task_to_timer_list();
    test_wait_returns_ok_when_no_deadline_overrun();
    test_wait_returns_deadline_reached_on_overrun();
    test_timerInt_increments_ticks();
    test_timerInt_moves_expired_timer_task_to_ready();
    test_timerInt_keeps_non_expired_timer_task();
    test_timerInt_moves_deadline_overrun_waiting_task_to_ready();
    test_timerInt_keeps_non_expired_waiting_task();

    printf("Tests run: %d\n", g_tests_run);
    printf("Tests failed: %d\n", g_tests_failed);

    if (g_tests_failed == 0)
    {
        printf("All timing_functions tests passed.\n");
        return 0;
    }

    printf("Some timing_functions tests failed.\n");
    return 1;
}
