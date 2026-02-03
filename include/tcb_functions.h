#pragma once

#include "kernel_functions.h"

TCB *create_TCB();
void init_TCB(TCB *tcb);

// Insert a list object into a list (depending on the deadline of the objects)
void insert_deadline_in_list(list *l, listobj *obj_to_insert);

void free_TCB(TCB *tcb);