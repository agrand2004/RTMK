#include "timing_functions.h"
#include "tcb_functions.h"

exception wait(uint nTicks)
{
    isr_off();
    update_previous_task();
    listobj *running_task = list_pop_front(ReadyList);
    list_push_back(TimerList, running_task);
    update_next_task();
    SwitchContext();
    if (isDeadlineReached(running_task->pTask))
    {
        return DEADLINE_REACHED;
    }
    return OK;
}

void set_ticks(uint nTicks)
{
    Ticks = nTicks;
}

uint ticks(void)
{
    return Ticks;
}

uint deadline(void)
{
    return NextTask->Deadline;
}

void set_deadline(uint deadline)
{
    isr_off();
    NextTask->Deadline = deadline;
    update_previous_task();

    // Reschedule the current task in the ReadyList based on its new deadline
    listobj *current = list_pop_front(ReadyList);
    insert_deadline_in_list(ReadyList, current);

    update_next_task();
    SwitchContext();
}

// Function to check the TimerList and move tasks to ReadyList if their timer has expired or their deadline has been reached
static void check_timer_list(void)
{
    listobj *current = TimerList->pHead;
    while (current)
    {
        current->nTCnt--;
        if (current->nTCnt <= 0 || isDeadlineReached(current->pTask))
        {
            listobj *to_move = current;
            current = current->pNext;
            update_previous_task();
            list_remove(TimerList, to_move);
            insert_deadline_in_list(ReadyList, to_move);
            update_next_task();
        }
        else
        {
            current = current->pNext;
        }
    }
}

// Function to check the WaitingList and move tasks to ReadyList if their deadline has been reached
static void check_waiting_list(void)
{
    listobj *current = WaitingList->pHead;
    while (current)
    {
        // THings we can do : sort the waiting list
        if (isDeadlineReached(current->pTask))
        {
            listobj *to_move = current;
            current = current->pNext;
            update_previous_task();
            list_remove(WaitingList, to_move);
            insert_deadline_in_list(ReadyList, to_move);
            // TODO: Clean up mailbox entry for to_move ?? -> the thing is that we don't have access to the mailbox from here
            to_move->pMessage = NULL; // ? Is there anything else to do ?
            update_next_task();
        }
        else
        {
            current = current->pNext;
        }
    }
}

void TimerInt()
{
    Ticks++;
    check_timer_list();
    check_waiting_list();
}
