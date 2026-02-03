#include "kernel_functions.h"
#include "linked_list.h"
#include "tcb_functions.h"

// Global variables for the kernel
int Ticks;
int KernelMode;
TCB *PreviousTask, *NextTask;
list *ReadyList, *WaitingList, *TimerList;

void idle_task(void)
{
    while (1)
    {
    }
}

exception init_kernel(void)
{
    // Init ticks to zero
    Ticks = 0;

    // Create lists structures
    ReadyList = create_list();
    WaitingList = create_list();
    TimerList = create_list();
    if (!ReadyList || !WaitingList || !TimerList)
        return FAIL;

    // Create idle task
    create_task(idle_task, UINT_MAX);

    // set kernel mode to init
    KernelMode = INIT;
    return OK;
}

exception create_task(void (*task_body)(), uint deadline)
{
    // Allocate memory for TCB
    TCB *tcb = create_TCB();
    tcb->Deadline = Ticks + deadline; // To make a realtive deadline
    // TODO: set TCB PC to point at the taskbody
    // TODO: set up stack frame
    // TODO: set TCB’s SP to point to the correct cell in stack segment

    // TODO: create new_obj with TCB information
    if (KernelMode == INIT)
    {
        insert_deadline_in_list(ReadyList, new_obj);
        return OK;
    }
    else
    {
        // TODO: do the else statement
    }
    return OK;
}

void run()
{
    Ticks = 0;
    KernelMode = RUNNING;

    listobj *head = list_get_front(ReadyList);
    NextTask = head->pTask;
 

    LoadContext_In_Run();
}

void terminate(){
    isr_off();
    listobj *head = list_pop_front(ReadyList);
    TCB *tcb = head->pTask;

    listobj *head = list_get_front(ReadyList);
    NextTask = head->pTask;
    

    switch_to_stack_of_next_task();

    free_TCB(tcb);
    free_listobj(head);
    
    LoadContext_In_Terminate();
}