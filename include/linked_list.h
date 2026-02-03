#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include "kernel_functions.h"

// Function prototypes for linked list operations

// Create a new list (return NULL if memory allocation fails)
list *create_list(void);

// Initialize an existing list
void init_list(list *l);

// Add an object to the end of the list
void list_push_back(list *l, listobj *obj);

// Remove an object from the list
void list_remove(list *l, listobj *obj);

// Insert an object in the list l before the object current
void list_insert_before(list *l, listobj *current, listobj *obj_to_insert);

// Remove an object from the head of the list and returns it, returns NULL if the list is empty
listobj *list_pop_front(list *l);

// Remove an object from the head of the list and returns it, returns NULL if the list is empty
listobj *list_get_front(list *l);

// Frees the memory of the entire list
void free_list(list *l);

// Frees the memoey of a listobj
void free_listobj(listobj *lobj);

#endif /* LINKED_LIST_H */