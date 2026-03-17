#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../include/kernel_functions.h"
#include "../../include/linked_list.h"
#include "../../include/tcb_functions.h"

/* Not in any header — defined in mailbox.c */
bool is_mailbox_empty(mailbox *mBox);

static int g_tests_run = 0;
static int g_tests_failed = 0;

static int g_isr_off_calls = 0;
static int g_switch_context_calls = 0;

#define EXPECT(condition, message)                              \
    do                                                          \
    {                                                           \
        g_tests_run++;                                          \
        if (!(condition))                                       \
        {                                                       \
            g_tests_failed++;                                   \
            printf("[FAIL] %s (line %d)\n", message, __LINE__); \
        }                                                       \
    } while (0)

/* -----------------------------------------------------------------------
 * Placeholder stubs
 * ----------------------------------------------------------------------- */
void isr_off(void) { g_isr_off_calls++; }
void isr_on(void) {}

void SwitchContext(void) { g_switch_context_calls++; }
void LoadContext_In_Run(void) {}
void switch_to_stack_of_next_task(void) {}
void LoadContext_In_Terminate(void) {}

/* -----------------------------------------------------------------------
 * Helpers
 * ----------------------------------------------------------------------- */
static void reset_counters(void)
{
    g_isr_off_calls = 0;
    g_switch_context_calls = 0;
}

static listobj *make_node(uint abs_deadline)
{
    listobj *node = (listobj *)calloc(1, sizeof(listobj));
    if (!node)
    {
        perror("calloc node");
        exit(2);
    }
    node->pTask = create_TCB();
    if (!node->pTask)
    {
        perror("create_TCB");
        exit(2);
    }
    node->pTask->Deadline = abs_deadline;
    return node;
}

static void free_list_nodes(list *l)
{
    listobj *it = l->pHead;
    while (it)
    {
        listobj *next = it->pNext;
        if (it->pTask)
            free_TCB(it->pTask);
        free_listobj(it);
        it = next;
    }
    l->pHead = l->pTail = NULL;
}

static void kernel_setup(void)
{
    if (ReadyList)
    {
        free_list_nodes(ReadyList);
        free(ReadyList);
    }
    if (WaitingList)
    {
        free_list_nodes(WaitingList);
        free(WaitingList);
    }
    if (TimerList)
    {
        free_list_nodes(TimerList);
        free(TimerList);
    }

    ReadyList = create_list();
    WaitingList = create_list();
    TimerList = create_list();

    PreviousTask = NULL;
    NextTask = NULL;
    Ticks = 0;
    KernelMode = RUNNING;
    reset_counters();
}

/* Add a task to ReadyList and update NextTask */
static listobj *add_ready_task(uint abs_deadline)
{
    listobj *node = make_node(abs_deadline);
    insert_deadline_in_list(ReadyList, node);
    NextTask = ReadyList->pHead->pTask;
    return node;
}

/* Manually plant a pre-built message into a mailbox (for test setup only).
 * Mirrors mailbox_push_back + bookkeeping without touching any static
 * internal function. */
static msg *plant_msg(mailbox *mBox, exception type,
                      void *data, size_t data_size,
                      listobj *block_node)
{
    msg *m = (msg *)calloc(1, sizeof(msg));
    if (!m)
    {
        perror("calloc msg");
        exit(2);
    }
    m->pData = (char *)malloc(data_size ? data_size : 1);
    if (!m->pData)
    {
        perror("malloc msg data");
        exit(2);
    }
    if (data && data_size)
        memcpy(m->pData, data, data_size);
    m->Status = type;
    m->pBlock = block_node;

    /* link into mailbox */
    m->pPrevious = mBox->pTail;
    if (mBox->pTail)
        mBox->pTail->pNext = m;
    else
        mBox->pHead = m;
    mBox->pTail = m;
    mBox->nMessages++;
    if (block_node)
        mBox->nBlockedMsg++;
    return m;
}

/* Free any messages left in a mailbox after a test (leak-safe cleanup) */
static void drain_mailbox(mailbox *mBox)
{
    msg *it = mBox->pHead;
    while (it)
    {
        msg *next = it->pNext;
        if (it->pData)
            free(it->pData);
        free(it);
        it = next;
    }
    mBox->pHead = mBox->pTail = NULL;
    mBox->nMessages = 0;
    mBox->nBlockedMsg = 0;
}

/* -----------------------------------------------------------------------
 * create_mailbox
 * ----------------------------------------------------------------------- */
static void test_create_mailbox_basic(void)
{
    mailbox *mBox = create_mailbox(10, sizeof(int));
    EXPECT(mBox != NULL, "create_mailbox should return a non-NULL pointer");
    if (!mBox)
        return;

    EXPECT(mBox->pHead == NULL, "new mailbox head should be NULL");
    EXPECT(mBox->pTail == NULL, "new mailbox tail should be NULL");
    EXPECT(mBox->nMessages == 0, "new mailbox message count should be 0");
    EXPECT(mBox->nBlockedMsg == 0, "new mailbox blocked count should be 0");
    EXPECT(mBox->nMaxMessages == 10, "nMaxMessages should match argument");
    EXPECT((size_t)mBox->nDataSize == sizeof(int), "nDataSize should match argument");

    free(mBox);
}

static void test_create_mailbox_small(void)
{
    mailbox *mBox = create_mailbox(1, 1);
    EXPECT(mBox != NULL, "create_mailbox(1,1) should succeed");
    EXPECT(mBox->nMaxMessages == 1, "nMaxMessages should be 1");
    EXPECT(mBox->nDataSize == 1, "nDataSize should be 1");
    if (mBox)
        free(mBox);
}

/* -----------------------------------------------------------------------
 * remove_mailbox
 * ----------------------------------------------------------------------- */
static void test_remove_mailbox_null(void)
{
    EXPECT(remove_mailbox(NULL) == FAIL, "remove_mailbox(NULL) should return FAIL");
}

static void test_remove_mailbox_with_messages(void)
{
    mailbox *mBox = create_mailbox(5, sizeof(int));
    int val = 42;
    plant_msg(mBox, SENDER, &val, sizeof(int), NULL);

    EXPECT(remove_mailbox(mBox) == NOT_EMPTY,
           "remove_mailbox should return NOT_EMPTY when messages are queued");

    drain_mailbox(mBox);
    free(mBox);
}

static void test_remove_mailbox_with_blocked(void)
{
    mailbox *mBox = create_mailbox(5, sizeof(int));
    /* nBlockedMsg > 0 without nMessages covers the other branch */
    mBox->nBlockedMsg = 1;

    EXPECT(remove_mailbox(mBox) == NOT_EMPTY,
           "remove_mailbox should return NOT_EMPTY when blocked senders/receivers exist");

    mBox->nBlockedMsg = 0;
    free(mBox);
}

static void test_remove_mailbox_empty(void)
{
    mailbox *mBox = create_mailbox(5, sizeof(int));
    EXPECT(remove_mailbox(mBox) == SUCCESS,
           "remove_mailbox should return SUCCESS for an empty mailbox");
    /* mBox is freed by remove_mailbox — do not access it again */
}

/* -----------------------------------------------------------------------
 * is_mailbox_empty
 * ----------------------------------------------------------------------- */
static void test_is_mailbox_empty(void)
{
    mailbox *mBox = create_mailbox(5, sizeof(int));
    EXPECT(is_mailbox_empty(mBox) == TRUE,
           "is_mailbox_empty should be TRUE on fresh mailbox");

    int val = 1;
    plant_msg(mBox, SENDER, &val, sizeof(int), NULL);
    EXPECT(is_mailbox_empty(mBox) == FALSE,
           "is_mailbox_empty should be FALSE after a message is added");

    drain_mailbox(mBox);
    free(mBox);
}

/* -----------------------------------------------------------------------
 * send_no_wait
 * ----------------------------------------------------------------------- */
static void test_send_no_wait_null_args(void)
{
    mailbox *mBox = create_mailbox(5, sizeof(int));
    int val = 1;
    EXPECT(send_no_wait(NULL, &val) == FAIL, "send_no_wait(NULL, data) should return FAIL");
    EXPECT(send_no_wait(mBox, NULL) == FAIL, "send_no_wait(mBox, NULL) should return FAIL");
    free(mBox);
}

static void test_send_no_wait_stores_message(void)
{
    kernel_setup();
    mailbox *mBox = create_mailbox(5, sizeof(int));
    int val = 99;

    exception ret = send_no_wait(mBox, &val);

    EXPECT(ret == OK, "send_no_wait should return OK when no receiver waiting");
    EXPECT(mBox->nMessages == 1, "mailbox should contain one message after send_no_wait");
    EXPECT(mBox->pHead != NULL, "mailbox pHead should not be NULL after send_no_wait");
    EXPECT(mBox->pHead->Status == SENDER, "stored message status should be SENDER");
    EXPECT(mBox->pHead->pBlock == NULL,
           "no-wait sender message pBlock should be NULL");
    if (mBox->pHead && mBox->pHead->pData)
        EXPECT(*(int *)mBox->pHead->pData == 99, "stored message should contain the sent value");

    drain_mailbox(mBox);
    free(mBox);
    free_list_nodes(ReadyList);
    free_list_nodes(WaitingList);
    free_list_nodes(TimerList);
}

static void test_send_no_wait_full_drops_oldest(void)
{
    kernel_setup();
    mailbox *mBox = create_mailbox(1, sizeof(int)); /* capacity = 1 */
    int first = 10, second = 20;

    send_no_wait(mBox, &first);
    EXPECT(mBox->nMessages == 1, "mailbox should have 1 message after first send");

    /* This send should evict the first message (full) */
    send_no_wait(mBox, &second);
    EXPECT(mBox->nMessages == 1, "mailbox should still have 1 message after overflow eviction");
    if (mBox->pHead && mBox->pHead->pData)
        EXPECT(*(int *)mBox->pHead->pData == second,
               "mailbox should contain the newest message after overflow");

    drain_mailbox(mBox);
    free(mBox);
    free_list_nodes(ReadyList);
    free_list_nodes(WaitingList);
    free_list_nodes(TimerList);
}

static void test_send_no_wait_delivers_to_waiting_receiver(void)
{
    kernel_setup();
    mailbox *mBox = create_mailbox(5, sizeof(int));

    /* Plant a waiting RECEIVER with a node in WaitingList */
    listobj *receiver_node = make_node(9999);
    list_push_back(WaitingList, receiver_node);
    msg *recv_msg = plant_msg(mBox, RECEIVER, NULL, sizeof(int), receiver_node);
    (void)recv_msg;

    /* A running sender in ReadyList */
    add_ready_task(9999);
    reset_counters();

    int val = 77;
    exception ret = send_no_wait(mBox, &val);

    EXPECT(ret == OK, "send_no_wait should return OK when receiver is waiting");
    EXPECT(mBox->nMessages == 0, "mailbox should be empty after direct delivery");
    EXPECT(mBox->nBlockedMsg == 0, "blocked count should be 0 after delivery");
    EXPECT(g_switch_context_calls == 1,
           "send_no_wait should call SwitchContext after unblocking a receiver");

    /* Receiver node should now be in ReadyList */
    int found = 0;
    listobj *it = ReadyList->pHead;
    while (it)
    {
        if (it == receiver_node)
        {
            found = 1;
            break;
        }
        it = it->pNext;
    }
    EXPECT(found, "receiver node should be moved to ReadyList after delivery");

    drain_mailbox(mBox);
    free(mBox);
    free_list_nodes(ReadyList);
    free_list_nodes(WaitingList);
    free_list_nodes(TimerList);
}

/* -----------------------------------------------------------------------
 * receive_no_wait
 * ----------------------------------------------------------------------- */
static void test_receive_no_wait_null_args(void)
{
    mailbox *mBox = create_mailbox(5, sizeof(int));
    int buf = 0;
    EXPECT(receive_no_wait(NULL, &buf) == FAIL, "receive_no_wait(NULL, buf) should return FAIL");
    EXPECT(receive_no_wait(mBox, NULL) == FAIL, "receive_no_wait(mBox, NULL) should return FAIL");
    free(mBox);
}

static void test_receive_no_wait_empty_mailbox(void)
{
    kernel_setup();
    mailbox *mBox = create_mailbox(5, sizeof(int));
    int buf = 0;

    EXPECT(receive_no_wait(mBox, &buf) == FAIL,
           "receive_no_wait should return FAIL when no sender is present");

    free(mBox);
    free_list_nodes(ReadyList);
    free_list_nodes(WaitingList);
    free_list_nodes(TimerList);
}

static void test_receive_no_wait_nonblocking_sender(void)
{
    kernel_setup();
    mailbox *mBox = create_mailbox(5, sizeof(int));

    /* Plant a no-wait SENDER message (pBlock = NULL) */
    int sent_val = 55;
    plant_msg(mBox, SENDER, &sent_val, sizeof(int), NULL);
    add_ready_task(9999);
    reset_counters();

    int buf = 0;
    exception ret = receive_no_wait(mBox, &buf);

    EXPECT(ret == OK, "receive_no_wait should return OK with a non-blocking sender");
    EXPECT(buf == 55, "receive_no_wait should copy the sent value into the buffer");
    EXPECT(mBox->nMessages == 0, "mailbox should be empty after receiving the only message");
    EXPECT(g_switch_context_calls == 0,
           "receive_no_wait with non-blocking sender should NOT call SwitchContext");

    drain_mailbox(mBox);
    free(mBox);
    free_list_nodes(ReadyList);
    free_list_nodes(WaitingList);
    free_list_nodes(TimerList);
}

static void test_receive_no_wait_blocked_sender(void)
{
    kernel_setup();
    mailbox *mBox = create_mailbox(5, sizeof(int));

    /* Plant a blocked SENDER message (pBlock != NULL, sender in WaitingList) */
    listobj *sender_node = make_node(9999);
    list_push_back(WaitingList, sender_node);
    int sent_val = 33;
    plant_msg(mBox, SENDER, &sent_val, sizeof(int), sender_node);

    /* A receiver running in ReadyList + one more so ReadyList stays non-empty */
    add_ready_task(9999);
    add_ready_task(9998);
    reset_counters();

    int buf = 0;
    exception ret = receive_no_wait(mBox, &buf);

    EXPECT(ret == OK, "receive_no_wait should return OK with a blocked sender");
    EXPECT(buf == 33, "receive_no_wait should copy data when sender was blocked");
    EXPECT(mBox->nMessages == 0, "mailbox should be empty after receive");
    EXPECT(mBox->nBlockedMsg == 0, "blocked count should be 0 after unblocking sender");
    EXPECT(g_switch_context_calls == 1,
           "receive_no_wait should call SwitchContext after unblocking a blocked sender");

    /* Sender node should now be in ReadyList */
    int found = 0;
    listobj *it = ReadyList->pHead;
    while (it)
    {
        if (it == sender_node)
        {
            found = 1;
            break;
        }
        it = it->pNext;
    }
    EXPECT(found, "sender node should be moved to ReadyList");

    drain_mailbox(mBox);
    free(mBox);
    free_list_nodes(ReadyList);
    free_list_nodes(WaitingList);
    free_list_nodes(TimerList);
}

/* -----------------------------------------------------------------------
 * send_wait
 * ----------------------------------------------------------------------- */
static void test_send_wait_null_args(void)
{
    mailbox *mBox = create_mailbox(5, sizeof(int));
    int val = 1;
    EXPECT(send_wait(NULL, &val) == FAIL, "send_wait(NULL, data) should return FAIL");
    EXPECT(send_wait(mBox, NULL) == FAIL, "send_wait(mBox, NULL) should return FAIL");
    free(mBox);
}

static void test_send_wait_delivers_to_waiting_receiver(void)
{
    kernel_setup();
    mailbox *mBox = create_mailbox(5, sizeof(int));

    /* Waiting receiver: node in WaitingList, RECEIVER msg in mailbox */
    listobj *receiver_node = make_node(9999);
    list_push_back(WaitingList, receiver_node);
    int recv_buf = 0;
    msg *recv_msg = plant_msg(mBox, RECEIVER, &recv_buf, sizeof(int), receiver_node);
    (void)recv_msg;

    /* Running sender in ReadyList */
    add_ready_task(9999);
    reset_counters();

    int val = 42;
    exception ret = send_wait(mBox, &val);

    EXPECT(ret == OK, "send_wait should return OK when receiver is already waiting");
    EXPECT(mBox->nMessages == 0, "mailbox should be empty after direct delivery");
    EXPECT(mBox->nBlockedMsg == 0, "blocked count should be 0 after delivery");
    EXPECT(g_switch_context_calls == 1, "send_wait should call SwitchContext");

    /* Receiver should be back in ReadyList */
    int found = 0;
    listobj *it = ReadyList->pHead;
    while (it)
    {
        if (it == receiver_node)
        {
            found = 1;
            break;
        }
        it = it->pNext;
    }
    EXPECT(found, "receiver node should be moved to ReadyList after data delivery");

    /* Data should have been written into the receiver's buffer */
    EXPECT(*(int *)recv_msg->pData == 42,
           "receiver message pData should contain the sent value");

    /* cleanup — recv_msg was removed from mailbox by send_wait but not freed */
    if (recv_msg->pData)
        free(recv_msg->pData);
    free(recv_msg);
    drain_mailbox(mBox);
    free(mBox);
    free_list_nodes(ReadyList);
    free_list_nodes(WaitingList);
    free_list_nodes(TimerList);
}

static void test_send_wait_no_receiver_blocks_sender(void)
{
    kernel_setup();
    mailbox *mBox = create_mailbox(5, sizeof(int));

    /* Two tasks: sender (head, running) + idle with larger deadline */
    listobj *sender = add_ready_task(9999);
    add_ready_task(99999);
    reset_counters();

    int val = 7;
    exception ret = send_wait(mBox, &val);

    EXPECT(ret == OK, "send_wait should return OK when no deadline was missed");
    EXPECT(mBox->nMessages == 1, "mailbox should contain the pending message");
    EXPECT(mBox->nBlockedMsg == 1, "blocked count should be 1 for the waiting sender");
    EXPECT(mBox->pHead != NULL && mBox->pHead->Status == SENDER,
           "queued message should have SENDER status");
    EXPECT(g_switch_context_calls == 1, "send_wait should call SwitchContext");

    /* sender node should now be in WaitingList */
    int found = 0;
    listobj *it = WaitingList->pHead;
    while (it)
    {
        if (it == sender)
        {
            found = 1;
            break;
        }
        it = it->pNext;
    }
    EXPECT(found, "sender node should be moved to WaitingList");

    drain_mailbox(mBox);
    free(mBox);
    free_list_nodes(ReadyList);
    free_list_nodes(WaitingList);
    free_list_nodes(TimerList);
}

static void test_send_wait_deadline_reached(void)
{
    kernel_setup();
    Ticks = 100;
    mailbox *mBox = create_mailbox(5, sizeof(int));

    /* Sender whose deadline has already expired */
    listobj *sender = make_node(50); /* Deadline=50 < Ticks=100 */
    insert_deadline_in_list(ReadyList, sender);
    listobj *idle = make_node(9999);
    insert_deadline_in_list(ReadyList, idle);
    NextTask = ReadyList->pHead->pTask;
    reset_counters();

    int val = 3;
    exception ret = send_wait(mBox, &val);

    EXPECT(ret == DEADLINE_REACHED,
           "send_wait should return DEADLINE_REACHED when sender deadline expired");
    EXPECT(mBox->nMessages == 0,
           "mailbox should be empty after deadline-reached cleanup");
    EXPECT(mBox->nBlockedMsg == 0,
           "blocked count should be 0 after deadline-reached cleanup");

    drain_mailbox(mBox);
    free(mBox);
    free_list_nodes(ReadyList);
    free_list_nodes(WaitingList);
    free_list_nodes(TimerList);
}

/* -----------------------------------------------------------------------
 * receive_wait
 * ----------------------------------------------------------------------- */
static void test_receive_wait_null_args(void)
{
    mailbox *mBox = create_mailbox(5, sizeof(int));
    int buf = 0;
    EXPECT(receive_wait(NULL, &buf) == FAIL, "receive_wait(NULL, buf) should return FAIL");
    EXPECT(receive_wait(mBox, NULL) == FAIL, "receive_wait(mBox, NULL) should return FAIL");
    free(mBox);
}

static void test_receive_wait_nonblocking_sender(void)
{
    kernel_setup();
    mailbox *mBox = create_mailbox(5, sizeof(int));

    /* No-wait SENDER message (pBlock = NULL) */
    int sent_val = 88;
    plant_msg(mBox, SENDER, &sent_val, sizeof(int), NULL);

    add_ready_task(9999);
    reset_counters();

    int buf = 0;
    exception ret = receive_wait(mBox, &buf);

    EXPECT(ret == OK, "receive_wait should return OK for a non-blocking sender");
    EXPECT(buf == 88, "receive_wait should copy the sender's value into the buffer");
    EXPECT(mBox->nMessages == 0, "mailbox should be empty after receiving the only message");
    EXPECT(g_switch_context_calls == 0,
           "receive_wait with non-blocking sender should NOT call SwitchContext");

    drain_mailbox(mBox);
    free(mBox);
    free_list_nodes(ReadyList);
    free_list_nodes(WaitingList);
    free_list_nodes(TimerList);
}

static void test_receive_wait_blocked_sender(void)
{
    kernel_setup();
    mailbox *mBox = create_mailbox(5, sizeof(int));

    /* Blocked SENDER: node in WaitingList, SENDER msg in mailbox */
    listobj *sender_node = make_node(9999);
    list_push_back(WaitingList, sender_node);
    int sent_val = 64;
    plant_msg(mBox, SENDER, &sent_val, sizeof(int), sender_node);

    /* Running receiver + idle */
    add_ready_task(9999);
    add_ready_task(9998);
    reset_counters();

    int buf = 0;
    exception ret = receive_wait(mBox, &buf);

    EXPECT(ret == OK, "receive_wait should return OK when blocked sender is present");
    EXPECT(buf == 64, "receive_wait should copy value from blocked sender");
    EXPECT(mBox->nMessages == 0, "mailbox should be empty after unblocking sender");
    EXPECT(mBox->nBlockedMsg == 0, "blocked count should be 0 after unblocking sender");
    EXPECT(g_switch_context_calls == 1, "receive_wait should call SwitchContext");

    /* Sender should be back in ReadyList */
    int found = 0;
    listobj *it = ReadyList->pHead;
    while (it)
    {
        if (it == sender_node)
        {
            found = 1;
            break;
        }
        it = it->pNext;
    }
    EXPECT(found, "sender node should be moved to ReadyList after being unblocked");

    drain_mailbox(mBox);
    free(mBox);
    free_list_nodes(ReadyList);
    free_list_nodes(WaitingList);
    free_list_nodes(TimerList);
}

static void test_receive_wait_no_sender_blocks_receiver(void)
{
    kernel_setup();
    mailbox *mBox = create_mailbox(5, sizeof(int));

    /* Two tasks: receiver (head, running) + idle with larger deadline */
    listobj *receiver = add_ready_task(9999);
    add_ready_task(99999);
    reset_counters();

    int buf = 0;
    exception ret = receive_wait(mBox, &buf);

    EXPECT(ret == OK, "receive_wait should return OK when no deadline miss");
    EXPECT(mBox->nMessages == 1, "mailbox should have the pending RECEIVER message");
    EXPECT(mBox->nBlockedMsg == 1, "blocked count should be 1");
    EXPECT(mBox->pHead != NULL && mBox->pHead->Status == RECEIVER,
           "queued message should have RECEIVER status");
    EXPECT(g_switch_context_calls == 1, "receive_wait should call SwitchContext");

    /* Receiver node should be in WaitingList */
    int found = 0;
    listobj *it = WaitingList->pHead;
    while (it)
    {
        if (it == receiver)
        {
            found = 1;
            break;
        }
        it = it->pNext;
    }
    EXPECT(found, "receiver node should be in WaitingList while blocking");

    drain_mailbox(mBox);
    free(mBox);
    free_list_nodes(ReadyList);
    free_list_nodes(WaitingList);
    free_list_nodes(TimerList);
}

static void test_receive_wait_deadline_reached(void)
{
    kernel_setup();
    Ticks = 200;
    mailbox *mBox = create_mailbox(5, sizeof(int));

    /* Receiver whose deadline already passed */
    listobj *receiver = make_node(100); /* Deadline=100 < Ticks=200 */
    insert_deadline_in_list(ReadyList, receiver);
    listobj *idle = make_node(9999);
    insert_deadline_in_list(ReadyList, idle);
    NextTask = ReadyList->pHead->pTask;
    reset_counters();

    int buf = 0;
    exception ret = receive_wait(mBox, &buf);

    EXPECT(ret == DEADLINE_REACHED,
           "receive_wait should return DEADLINE_REACHED when receiver deadline expired");
    EXPECT(mBox->nMessages == 0,
           "mailbox should be empty after deadline-reached cleanup");
    EXPECT(mBox->nBlockedMsg == 0,
           "blocked count should be 0 after deadline-reached cleanup");

    drain_mailbox(mBox);
    free(mBox);
    free_list_nodes(ReadyList);
    free_list_nodes(WaitingList);
    free_list_nodes(TimerList);
}

/* -----------------------------------------------------------------------
 * main
 * ----------------------------------------------------------------------- */
int main(void)
{
    printf("Running mailbox unit tests...\n");

    /* create_mailbox */
    test_create_mailbox_basic();
    test_create_mailbox_small();

    /* remove_mailbox */
    test_remove_mailbox_null();
    test_remove_mailbox_with_messages();
    test_remove_mailbox_with_blocked();
    test_remove_mailbox_empty();

    /* is_mailbox_empty */
    test_is_mailbox_empty();

    /* send_no_wait */
    test_send_no_wait_null_args();
    test_send_no_wait_stores_message();
    test_send_no_wait_full_drops_oldest();
    test_send_no_wait_delivers_to_waiting_receiver();

    /* receive_no_wait */
    test_receive_no_wait_null_args();
    test_receive_no_wait_empty_mailbox();
    test_receive_no_wait_nonblocking_sender();
    test_receive_no_wait_blocked_sender();

    /* send_wait */
    test_send_wait_null_args();
    test_send_wait_delivers_to_waiting_receiver();
    test_send_wait_no_receiver_blocks_sender();
    test_send_wait_deadline_reached();

    /* receive_wait */
    test_receive_wait_null_args();
    test_receive_wait_nonblocking_sender();
    test_receive_wait_blocked_sender();
    test_receive_wait_no_sender_blocks_receiver();
    test_receive_wait_deadline_reached();

    printf("Tests run: %d\n", g_tests_run);
    printf("Tests failed: %d\n", g_tests_failed);

    if (g_tests_failed == 0)
    {
        printf("All mailbox tests passed.\n");
        return 0;
    }

    printf("Some mailbox tests failed.\n");
    return 1;
}
