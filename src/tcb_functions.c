#include "tcb_functions.h"

TCB *create_TCB()
{
    TCB *tcb = (TCB *)malloc(sizeof(TCB));
    if (!tcb)
        return NULL;
    init_TCB(tcb);
    return tcb;
}

void insert_deadline_in_list(list *l, listobj *obj_to_insert)
{
    listobj *current = l->pHead;
    uint obj_deadline = obj_to_insert->pTask->Deadline;

    while (current)
    {
        if (current->pTask->Deadline < obj_deadline)
        {
            // Insert before current
            obj_to_insert->pNext = current;
            obj_to_insert->pPrevious = current->pPrevious;
            if (current->pPrevious)
                current->pPrevious->pNext = obj_to_insert;
            else
                l->pHead = obj_to_insert;
            current->pPrevious = obj_to_insert;
            return;
        }
        current = current->pNext;
    }
    // Insert at the end
    obj_to_insert->pNext = NULL;
    obj_to_insert->pPrevious = l->pTail;
    if (l->pTail)
        l->pTail->pNext = obj_to_insert;
    else
        l->pHead = obj_to_insert;
    l->pTail = obj_to_insert;
}

void free_TCB(TCB *tcb)
{
    free(tcb);
}
