#ifndef TCB_FUNCTIONS_H
#define TCB_FUNCTIONS_H

#include "kernel_functions.h"
#include "linked_list.h"

TCB *create_TCB();

// Insert a list object into a list (depending on the deadline of the objects)
void insert_deadline_in_list(list *l, listobj *obj_to_insert);

void free_TCB(TCB *tcb);

#endif /* TCB_FUNCTIONS_H */