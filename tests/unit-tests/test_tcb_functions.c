#include <stdio.h>
#include <stdlib.h>

#include "../../include/tcb_functions.h"

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

static listobj *new_node_with_deadline(uint deadline)
{
    listobj *node = (listobj *)calloc(1, sizeof(listobj));
    if (!node)
    {
        printf("Fatal: allocation failed for listobj\n");
        exit(2);
    }

    node->pTask = create_TCB();
    if (!node->pTask)
    {
        printf("Fatal: create_TCB failed for test node\n");
        free(node);
        exit(2);
    }

    node->pTask->Deadline = deadline;
    return node;
}

static void free_nodes_in_list(list *l)
{
    listobj *it = l->pHead;
    while (it)
    {
        listobj *next = it->pNext;
        free_TCB(it->pTask);
        free_listobj(it);
        it = next;
    }
    l->pHead = NULL;
    l->pTail = NULL;
}

static void expect_list_deadlines(list *l, const uint *expected, int n_expected, const char *label)
{
    listobj *it = l->pHead;
    int i = 0;

    while (it && i < n_expected)
    {
        EXPECT(it->pTask != NULL, label);
        if (it->pTask)
        {
            EXPECT(it->pTask->Deadline == expected[i], label);
        }
        it = it->pNext;
        i++;
    }

    EXPECT(i == n_expected, label);
    EXPECT(it == NULL, label);
}

static void test_create_and_free_tcb(void)
{
    TCB *tcb = create_TCB();
    EXPECT(tcb != NULL, "create_TCB should return a valid pointer");
    if (tcb)
    {
        tcb->Deadline = 1234;
        EXPECT(tcb->Deadline == 1234, "created TCB should be writable");
        free_TCB(tcb);
        EXPECT(1, "free_TCB should free allocated TCB without crashing");
    }
}

static void test_insert_empty_list(void)
{
    list l;
    listobj *n1 = new_node_with_deadline(42);

    init_list(&l);
    insert_deadline_in_list(&l, n1);

    EXPECT(l.pHead == n1, "insert into empty list should set head");
    EXPECT(l.pTail == n1, "insert into empty list should set tail");
    EXPECT(n1->pPrevious == NULL, "single node previous should be NULL");
    EXPECT(n1->pNext == NULL, "single node next should be NULL");

    free_nodes_in_list(&l);
}

static void test_insert_before_head(void)
{
    list l;
    listobj *n50 = new_node_with_deadline(50);
    listobj *n10 = new_node_with_deadline(10);
    uint expected[] = {10, 50};

    init_list(&l);
    insert_deadline_in_list(&l, n50);
    insert_deadline_in_list(&l, n10);

    EXPECT(l.pHead == n10, "smaller deadline should become new head");
    EXPECT(l.pTail == n50, "tail should stay as largest deadline");
    EXPECT(n10->pPrevious == NULL, "head previous should be NULL");
    EXPECT(n10->pNext == n50, "new head next should point to old head");
    EXPECT(n50->pPrevious == n10, "old head previous should point to new head");

    expect_list_deadlines(&l, expected, 2, "list should be sorted after head insertion");
    free_nodes_in_list(&l);
}

static void test_insert_middle_and_tail(void)
{
    list l;
    listobj *n10 = new_node_with_deadline(10);
    listobj *n30 = new_node_with_deadline(30);
    listobj *n50 = new_node_with_deadline(50);
    listobj *n40 = new_node_with_deadline(40);
    listobj *n80 = new_node_with_deadline(80);
    uint expected[] = {10, 30, 40, 50, 80};

    init_list(&l);
    insert_deadline_in_list(&l, n10);
    insert_deadline_in_list(&l, n30);
    insert_deadline_in_list(&l, n50);

    insert_deadline_in_list(&l, n40);
    EXPECT(n30->pNext == n40, "middle insert should update previous node next");
    EXPECT(n40->pPrevious == n30, "middle insert previous link should be correct");
    EXPECT(n40->pNext == n50, "middle insert next link should be correct");
    EXPECT(n50->pPrevious == n40, "middle insert should update next node previous");

    insert_deadline_in_list(&l, n80);
    EXPECT(l.pTail == n80, "largest deadline should become tail");
    EXPECT(n80->pNext == NULL, "tail next should be NULL");

    expect_list_deadlines(&l, expected, 5, "list should be sorted after middle and tail insertions");
    free_nodes_in_list(&l);
}

static void test_equal_deadline_insertion(void)
{
    list l;
    listobj *n10 = new_node_with_deadline(10);
    listobj *n30_a = new_node_with_deadline(30);
    listobj *n50 = new_node_with_deadline(50);
    listobj *n30_b = new_node_with_deadline(30);
    uint expected[] = {10, 30, 30, 50};

    init_list(&l);
    insert_deadline_in_list(&l, n10);
    insert_deadline_in_list(&l, n30_a);
    insert_deadline_in_list(&l, n50);
    insert_deadline_in_list(&l, n30_b);

    EXPECT(n30_a->pNext == n30_b, "equal deadline node should be inserted after existing equal node");
    EXPECT(n30_b->pPrevious == n30_a, "equal deadline node previous link should be correct");
    EXPECT(n30_b->pNext == n50, "equal deadline node should still be before larger deadline");

    expect_list_deadlines(&l, expected, 4, "list should keep sorted order with equal deadlines");
    free_nodes_in_list(&l);
}

int main(void)
{
    printf("Running tcb_functions unit tests...\n");

    test_create_and_free_tcb();
    test_insert_empty_list();
    test_insert_before_head();
    test_insert_middle_and_tail();
    test_equal_deadline_insertion();

    printf("Tests run: %d\n", g_tests_run);
    printf("Tests failed: %d\n", g_tests_failed);

    if (g_tests_failed == 0)
    {
        printf("All tcb_functions tests passed.\n");
        return 0;
    }

    printf("Some tcb_functions tests failed.\n");
    return 1;
}