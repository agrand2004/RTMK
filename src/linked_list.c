#include "linked_list.h"

list *create_list(void)
{
    list *l = (list *)malloc(sizeof(list));
    if (!l)
        return NULL;
    init_list(l);
    return l;
}

void init_list(list *l)
{
    l->pHead = NULL;
    l->pTail = NULL;
}

void list_push_back(list *l, listobj *obj)
{
    obj->pNext = NULL;
    obj->pPrevious = l->pTail;

    if (l->pTail)
        l->pTail->pNext = obj;
    else
        l->pHead = obj;

    l->pTail = obj;
}

void list_remove(list *l, listobj *obj)
{
    if (obj->pPrevious)
    {
        obj->pPrevious->pNext = obj->pNext;
    }
    else
    {
        l->pHead = obj->pNext;
    }

    if (obj->pNext)
    {
        obj->pNext->pPrevious = obj->pPrevious;
    }
    else
    {
        l->pTail = obj->pPrevious;
    }

    obj->pNext = obj->pPrevious = NULL;
}

listobj *list_pop_front(list *l)
{
    if (!l->pHead)
        return NULL;

    listobj *obj = l->pHead;
    list_remove(l, obj);
    return obj;
}

listobj *list_get_front(list *l)
{
    if (!l->pHead)
        return NULL;

    listobj *obj = l->pHead;
    return obj;
}

void free_list(list *l)
{
    listobj *it = l->pHead;
    while (it)
    {
        listobj *next = it->pNext;
        listobj_destroy(it);
        it = next;
    }
    free(l);
}

void free_listobj(listobj *lobj)
{
    free(lobj);
}
