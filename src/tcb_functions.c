#include "tcb_functions.h"

TCB *create_TCB()
{
    TCB *tcb = (TCB *)malloc(sizeof(TCB));
    if (!tcb)
        return NULL;
    return tcb;
}

void insert_deadline_in_list(list *l, listobj *obj_to_insert)
{
    listobj *current = l->pHead;
    uint obj_deadline = obj_to_insert->pTask->Deadline;

    while (current)
    {
        if (obj_deadline < current->pTask->Deadline)
        {
            // Insert before current
            list_insert_before(l, current, obj_to_insert);
            return;
        }
        current = current->pNext;
    }
    // Insert at the end
    list_push_back(l, obj_to_insert);
}

void free_TCB(TCB *tcb)
{
    free(tcb);
}
