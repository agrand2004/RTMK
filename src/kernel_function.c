#include "kernel_functions.h"
#include "linked_list.h"
#include "tcb_functions.h"

static void update_next_task(void) {
    listobj *head = list_get_front(ReadyList);
    NextTask = head->pTask;
}

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
    if (!tcb) {
        return FAIL;
    }
    tcb->Deadline = Ticks + deadline; // relative deadline (depending on the actual tick)
    tcb->PC = task_body; // set TCB PC to point at the taskbody
    tcb->SPSR = 0x21000000;
    // set up stack frame
    tcb->StackSeg [STACK_SIZE - 2] = 0x21000000;
    tcb->StackSeg [STACK_SIZE - 3] = (unsigned int) task_body;
    tcb->SP = &(tcb->StackSeg [STACK_SIZE - 9]); // set TCB’s SP to point to the correct cell in stack segment

    listobj *new_obj = (listobj *)malloc(sizeof(listobj));
    if (!new_obj) {
        free_TCB(tcb);
        return FAIL;
    }
    new_obj->pTask = tcb;
    if (KernelMode == INIT)
    {
        insert_deadline_in_list(ReadyList, new_obj);
        return OK;
    }
    isr_off();
    PreviousTask = NextTask;
    insert_deadline_in_list(ReadyList, new_obj);
    update_next_task();
    SwitchContext();
    return OK;
}

void run()
{
    Ticks = 0;
    KernelMode = RUNNING;

    update_next_task();

    LoadContext_In_Run();
}

void terminate(){
    isr_off();
    listobj *head = list_pop_front(ReadyList);
    TCB *tcb = head->pTask;

    update_next_task();

    switch_to_stack_of_next_task();

    free_TCB(tcb);
    free_listobj(head);
    LoadContext_In_Terminate();
}
