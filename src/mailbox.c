#include "kernel_functions.h"
#include "linked_list.h"
#include "tcb_functions.h"

static void mailbox_remove_msg(mailbox *l, msg *obj)
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

static void mailbox_push_back(mailbox *l, msg *obj)
{
    obj->pNext = NULL;
    obj->pPrevious = l->pTail;

    if (l->pTail)
        l->pTail->pNext = obj;
    else
        l->pHead = obj;

    l->pTail = obj;
}

mailbox *create_mailbox(uint nMessages, uint nDataSize)
{
    mailbox *mBox = (mailbox *)malloc(sizeof(mailbox));
    if (!mBox)
    {
        return NULL;
    }
    mBox->pHead = NULL;
    mBox->pTail = NULL;
    mBox->nDataSize = nDataSize;
    mBox->nMaxMessages = nMessages;
    mBox->nMessages = 0;
    mBox->nBlockedMsg = 0;

    return mBox;
}

exception remove_mailbox(mailbox *mBox)
{
    if (mBox == NULL)
    {
        return FAIL;
    }
    if (mBox->nMessages > 0 || mBox->nBlockedMsg > 0)
    {
        return NOT_EMPTY;
    }

    free(mBox);
    return SUCCESS;
}

msg *get_msg_by_type(mailbox *mBox, exception type)
{
    msg *currentMsg = mBox->pHead;
    // if (mBox->nBlockedMsg != 0) //? remove this ???
    // {
        for (; currentMsg; currentMsg = currentMsg->pNext)
        {
            if (currentMsg->Status == type)
            {
                return currentMsg;
            }
        }
    // }
    return NULL;
}

exception send_wait(mailbox *mBox, void *pData)
{
    if (mBox == NULL || pData == NULL)
    {
        return FAIL;
    }
    isr_off();

    msg *currentMsg = get_msg_by_type(mBox, RECEIVER);
    listobj *currentTask = list_get_front(ReadyList);
    if (currentMsg != NULL)
    {
        memcpy(currentMsg->pData, pData, mBox->nDataSize);
        mailbox_remove_msg(mBox, currentMsg);
        mBox->nMessages--;
        mBox->nBlockedMsg--;
        update_previous_task();
        list_remove(WaitingList, currentMsg->pBlock);
        insert_deadline_in_list(ReadyList, currentMsg->pBlock);
        update_next_task();
    }
    else
    {
        currentMsg = (msg *)malloc(sizeof(msg));
        if (!currentMsg)
        {
            isr_on();
            return FAIL;
        }
        currentMsg->pData = (char *)malloc(mBox->nDataSize);
        if (!currentMsg->pData)
        {
            free(currentMsg);
            isr_on();
            return FAIL;
        }
        memcpy(currentMsg->pData, pData, mBox->nDataSize);
        currentMsg->Status = SENDER;
        currentMsg->pBlock = list_get_front(ReadyList);
        mailbox_push_back(mBox, currentMsg);
        mBox->nMessages++;
        mBox->nBlockedMsg++;
        update_previous_task();
        list_remove(ReadyList, currentMsg->pBlock);
        list_push_back(WaitingList, currentMsg->pBlock);
        update_next_task();
    }
    SwitchContext();
    if (isDeadlineReached(currentTask->pTask))
    {
        isr_off();

        mailbox_remove_msg(mBox, currentMsg);
        mBox->nMessages--;
        mBox->nBlockedMsg--;
        free(currentMsg->pData);
        free(currentMsg);

        isr_on();
        return DEADLINE_REACHED;
    }

    return OK;
}

exception receive_wait(mailbox *mBox, void *pData)
{
    if (mBox == NULL || pData == NULL)
    {
        return FAIL;
    }
    isr_off();

    msg *currentMsg = get_msg_by_type(mBox, SENDER);
    listobj *currentTask = list_get_front(ReadyList);

    if (currentMsg)
    {
        memcpy(pData, currentMsg->pData, mBox->nDataSize);
        mailbox_remove_msg(mBox, currentMsg);
        mBox->nMessages--;

        if (currentMsg->pBlock != NULL)
        {
            mBox->nBlockedMsg--;
            update_previous_task();
            list_remove(WaitingList, currentMsg->pBlock);
            insert_deadline_in_list(ReadyList, currentMsg->pBlock);
            update_next_task();
        }
        else
        {
            free(currentMsg->pData);
            free(currentMsg);
        }
    }
    else
    {
        currentMsg = (msg *)malloc(sizeof(msg));
        if (!currentMsg)
        {
            isr_on();
            return FAIL;
        }
        currentMsg->pData = (char *)malloc(mBox->nDataSize);
        if (!currentMsg->pData)
        {
            free(currentMsg);
            isr_on();
            return FAIL;
        }
        currentMsg->Status = RECEIVER;
        currentMsg->pBlock = list_get_front(ReadyList);
        mailbox_push_back(mBox, currentMsg);
        mBox->nMessages++;
        mBox->nBlockedMsg++;
        update_previous_task();
        list_remove(ReadyList, currentMsg->pBlock);
        list_push_back(WaitingList, currentMsg->pBlock);
        update_next_task();
    }
    SwitchContext();
    if (isDeadlineReached(currentTask->pTask))
    {
        isr_off();

        mailbox_remove_msg(mBox, currentMsg);
        mBox->nMessages--;
        mBox->nBlockedMsg--;
        free(currentMsg->pData);
        free(currentMsg);

        isr_on();
        return DEADLINE_REACHED;
    }
    return OK;
}

exception send_no_wait(mailbox *mBox, void *pData)
{
    if (mBox == NULL || pData == NULL)
    {
        return FAIL;
    }
    isr_off();

    msg *currentMsg = get_msg_by_type(mBox, RECEIVER);

    if (currentMsg)
    {
        memcpy(currentMsg->pData, pData, mBox->nDataSize);
        mailbox_remove_msg(mBox, currentMsg);
        mBox->nMessages--;
        mBox->nBlockedMsg--;
        update_previous_task();
        list_remove(WaitingList, currentMsg->pBlock);
        insert_deadline_in_list(ReadyList, currentMsg->pBlock);
        update_next_task();
        SwitchContext();
    }
    else
    {
        currentMsg = (msg *)malloc(sizeof(msg));
        if (!currentMsg)
        {
            isr_on();
            return FAIL;
        }
        currentMsg->pData = (char *)malloc(mBox->nDataSize);
        if (!currentMsg->pData)
        {
            free(currentMsg);
            isr_on();
            return FAIL;
        }
        memcpy(currentMsg->pData, pData, mBox->nDataSize);
        currentMsg->Status = SENDER;
        currentMsg->pBlock = NULL;
        // ? do we have to do the same thing in the other functions when adding to the mailbox ???
        // Yes we need to
        if (mBox->nMessages == mBox->nMaxMessages)
        {
            mailbox_remove_msg(mBox, mBox->pHead);
            if (mBox->pHead->pBlock != NULL)
            {
                mBox->nBlockedMsg--;
                // * we don't need to put the waiting task back to the ready list because it will be done in the check_timer_list function when the task will be unblocked by the timeout
            }
            free(mBox->pHead->pData);
            free(mBox->pHead);
            mBox->nMessages--;
        }
        mailbox_push_back(mBox, currentMsg);
        mBox->nMessages++;
    }
    return OK;
}

int receive_no_wait(mailbox *mBox, void *pData)
{
    if (mBox == NULL || pData == NULL)
    {
        return FAIL;
    }
    isr_off();

    msg *currentMsg = get_msg_by_type(mBox, SENDER);
    if (currentMsg)
    {
        memcpy(pData, currentMsg->pData, mBox->nDataSize);
        mailbox_remove_msg(mBox, currentMsg);
        mBox->nMessages--;

        if (currentMsg->pBlock != NULL)
        {
            mBox->nBlockedMsg--;
            update_previous_task();
            list_remove(WaitingList, currentMsg->pBlock);
            insert_deadline_in_list(ReadyList, currentMsg->pBlock);
            update_next_task();
            SwitchContext();
        }
        else
        {
            free(currentMsg->pData);
            free(currentMsg);
        }
        return OK;
    }
    return FAIL;
}

// int no_messages(mailbox *mBox);
