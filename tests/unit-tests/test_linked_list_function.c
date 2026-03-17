#include <stdio.h>
#include <stdlib.h>

#include "../../include/linked_list.h"

static int g_tests_run = 0;
static int g_tests_failed = 0;

#define EXPECT(condition, message)                                              \
    do                                                                          \
    {                                                                           \
        g_tests_run++;                                                          \
        if (!(condition))                                                       \
        {                                                                       \
            g_tests_failed++;                                                   \
            printf("[FAIL] %s (line %d)\n", message, __LINE__);              \
        }                                                                       \
    } while (0)

static listobj *new_listobj(void)
{
    listobj *obj = (listobj *)calloc(1, sizeof(listobj));
    if (!obj)
    {
        printf("Fatal: allocation failed for listobj\n");
        exit(2);
    }
    return obj;
}

static void test_create_and_init_list(void)
{
    list *l = create_list();
    EXPECT(l != NULL, "create_list should allocate a list");
    if (!l)
        return;

    EXPECT(l->pHead == NULL, "newly created list head should be NULL");
    EXPECT(l->pTail == NULL, "newly created list tail should be NULL");

    l->pHead = (listobj *)0x1;
    l->pTail = (listobj *)0x2;
    init_list(l);
    EXPECT(l->pHead == NULL, "init_list should reset head to NULL");
    EXPECT(l->pTail == NULL, "init_list should reset tail to NULL");

    free(l);
}

static void test_push_back_and_get_front(void)
{
    list l;
    listobj *a = new_listobj();
    listobj *b = new_listobj();

    init_list(&l);

    EXPECT(list_get_front(&l) == NULL, "list_get_front should return NULL on empty list");

    list_push_back(&l, a);
    EXPECT(l.pHead == a, "push_back on empty list should set head");
    EXPECT(l.pTail == a, "push_back on empty list should set tail");
    EXPECT(a->pPrevious == NULL, "first element previous should be NULL");
    EXPECT(a->pNext == NULL, "first element next should be NULL");
    EXPECT(list_get_front(&l) == a, "list_get_front should return first element");
    EXPECT(l.pHead == a && l.pTail == a, "list_get_front should not modify list");

    list_push_back(&l, b);
    EXPECT(l.pHead == a, "push_back should keep original head");
    EXPECT(l.pTail == b, "push_back should update tail");
    EXPECT(a->pNext == b, "old tail next should point to new tail");
    EXPECT(b->pPrevious == a, "new tail previous should point to old tail");
    EXPECT(b->pNext == NULL, "new tail next should be NULL");

    free_listobj(a);
    free_listobj(b);
}

static void test_pop_front_cases(void)
{
    list l;
    listobj *a = new_listobj();
    listobj *b = new_listobj();
    listobj *popped;

    init_list(&l);

    popped = list_pop_front(&l);
    EXPECT(popped == NULL, "list_pop_front should return NULL on empty list");

    list_push_back(&l, a);
    popped = list_pop_front(&l);
    EXPECT(popped == a, "list_pop_front should return the only element");
    EXPECT(l.pHead == NULL && l.pTail == NULL, "after popping only element list should be empty");
    EXPECT(a->pNext == NULL && a->pPrevious == NULL, "popped element links should be cleared");

    list_push_back(&l, a);
    list_push_back(&l, b);
    popped = list_pop_front(&l);
    EXPECT(popped == a, "pop_front should return old head in multi-element list");
    EXPECT(l.pHead == b, "head should move to next element");
    EXPECT(l.pTail == b, "tail should remain last element");
    EXPECT(b->pPrevious == NULL, "new head previous should be NULL");
    EXPECT(a->pNext == NULL && a->pPrevious == NULL, "removed head links should be cleared");

    free_listobj(a);
    free_listobj(b);
}

static void test_insert_before_cases(void)
{
    list l;
    listobj *a = new_listobj();
    listobj *b = new_listobj();
    listobj *c = new_listobj();
    listobj *x = new_listobj();
    listobj *y = new_listobj();

    init_list(&l);
    list_push_back(&l, a);
    list_push_back(&l, b);
    list_push_back(&l, c);

    list_insert_before(&l, b, x);
    EXPECT(l.pHead == a, "inserting before middle should not change head");
    EXPECT(l.pTail == c, "inserting before middle should not change tail");
    EXPECT(a->pNext == x, "node before insertion point should link to inserted node");
    EXPECT(x->pPrevious == a, "inserted node previous should be old previous");
    EXPECT(x->pNext == b, "inserted node next should be current node");
    EXPECT(b->pPrevious == x, "current previous should be inserted node");

    list_insert_before(&l, a, y);
    EXPECT(l.pHead == y, "inserting before head should update head");
    EXPECT(y->pPrevious == NULL, "new head previous should be NULL");
    EXPECT(y->pNext == a, "new head next should be old head");
    EXPECT(a->pPrevious == y, "old head previous should point to new head");

    free_listobj(y);
    free_listobj(a);
    free_listobj(x);
    free_listobj(b);
    free_listobj(c);
}

static void test_remove_cases(void)
{
    list l;
    listobj *a = new_listobj();
    listobj *b = new_listobj();
    listobj *c = new_listobj();
    listobj *d = new_listobj();

    init_list(&l);
    list_push_back(&l, a);
    list_push_back(&l, b);
    list_push_back(&l, c);
    list_push_back(&l, d);

    list_remove(&l, b);
    EXPECT(l.pHead == a, "remove middle should keep head");
    EXPECT(l.pTail == d, "remove middle should keep tail");
    EXPECT(a->pNext == c, "remove middle should bypass removed node (next)");
    EXPECT(c->pPrevious == a, "remove middle should bypass removed node (previous)");
    EXPECT(b->pNext == NULL && b->pPrevious == NULL, "removed node links should be cleared");

    list_remove(&l, a);
    EXPECT(l.pHead == c, "remove head should update head");
    EXPECT(c->pPrevious == NULL, "new head previous should be NULL");
    EXPECT(a->pNext == NULL && a->pPrevious == NULL, "removed head links should be cleared");

    list_remove(&l, d);
    EXPECT(l.pTail == c, "remove tail should update tail");
    EXPECT(c->pNext == NULL, "new tail next should be NULL");
    EXPECT(d->pNext == NULL && d->pPrevious == NULL, "removed tail links should be cleared");

    list_remove(&l, c);
    EXPECT(l.pHead == NULL && l.pTail == NULL, "remove last element should empty list");
    EXPECT(c->pNext == NULL && c->pPrevious == NULL, "removed last element links should be cleared");

    free_listobj(a);
    free_listobj(b);
    free_listobj(c);
    free_listobj(d);
}

static void test_free_helpers(void)
{
    list *l_empty = create_list();
    EXPECT(l_empty != NULL, "create_list should work for free_list empty test");
    if (l_empty)
    {
        free_list(l_empty);
    }

    list *l = create_list();
    listobj *a = new_listobj();
    listobj *b = new_listobj();
    listobj *c = new_listobj();

    EXPECT(l != NULL, "create_list should work for free_list populated test");
    if (l)
    {
        list_push_back(l, a);
        list_push_back(l, b);
        list_push_back(l, c);
        free_list(l);
    }
    else
    {
        free_listobj(a);
        free_listobj(b);
        free_listobj(c);
    }

    listobj *single = new_listobj();
    free_listobj(single);
    EXPECT(1, "free_listobj should free standalone list object without crashing");
}

int main(void)
{
    printf("Running linked list unit tests...\n");

    test_create_and_init_list();
    test_push_back_and_get_front();
    test_pop_front_cases();
    test_insert_before_cases();
    test_remove_cases();
    test_free_helpers();

    printf("Tests run: %d\n", g_tests_run);
    printf("Tests failed: %d\n", g_tests_failed);

    if (g_tests_failed == 0)
    {
        printf("All linked list tests passed.\n");
        return 0;
    }

    printf("Some linked list tests failed.\n");
    return 1;
}