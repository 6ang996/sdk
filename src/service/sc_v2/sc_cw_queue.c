/**
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  �ļ���: sc_cw_queue.c
 *
 *  ����ʱ��: 2015��6��1��20:00:36
 *  ��    ��: Larry
 *  ��    ��: ʵ�ֺ��еȴ�����
 *  �޸���ʷ:
 *
 * �ȴ����й������
 * 1. �̶�1���Ӽ��һ�Σ�����п��Ժ��е���ϯ�����ͨ��ϯ
 * 2. ����к��н��������Ҫ����֪ͨ��ģ��
 * 3. �ڴ��еĽṹ
 *  g_stCWQMngt
 *    | --- queue1 --- call1 --- call2 --- call3 --- call4 ...
 *    | --- queue2 --- call1 --- call2 --- call3 --- call4 ...
 *    | --- ......
 *  ��ȫ�ֱ���g_stCWQMngt������ϯ����У�ÿ����ϯ����ά��������ϯ��ĺ��ж���
 *
 */


#include <dos.h>
#include <esl.h>
#include "sc_def.h"
#include "sc_debug.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif /* __cplusplus */
#endif /* __cplusplus */

/* �������������еȴ����еĹ������� */
DLL_S               g_stCWQMngt;
pthread_t           g_pthCWQMngt;
pthread_mutex_t     g_mutexCWQMngt = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t      g_condCWQMngt  = PTHREAD_COND_INITIALIZER;

/*  �����̱߳�־ */
static BOOL         g_blCWQWaitingExit = DOS_FALSE;
static BOOL         g_blCWQRunning  = DOS_FALSE;

/*  */
/**
 * �ڲ�����������ϯ����в�����ϯ��
 * ����:
 *      VOID *pParam : ������������һ��U32�����ݣ���ʾ��ϯ��ID
 *      DLL_NODE_S *pstNode : ��ǰ�ڵ�
 * ����ֵ: 0��ʾ�ҵ�һ���ڵ㣬����ֵ��ʾû���ҵ�
 */
static S32 sc_cwq_find_agentgrp(VOID *pParam, DLL_NODE_S *pstNode)
{
    SC_CWQ_NODE_ST *pstCWQNode = NULL;

    if (DOS_ADDR_INVALID(pParam) || DOS_ADDR_INVALID(pstNode) || DOS_ADDR_INVALID(pstNode->pHandle))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstCWQNode = pstNode->pHandle;

    if (*(U32 *)pParam != pstCWQNode->ulAgentGrpID)
    {
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/**
 * ����: sc_cwq_add_call
 * ����: ��pstSCB��ָ��ĺ�����ӵ����ж���
 * ����:
 *      SC_SCB_ST *pstSCB : ���п��ƿ�
 *      U32 ulAgentGrpID : ��ϯ��ID
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 * !!! ���ulAgentGrpID��ָ����grp�����ڣ������´���
 * !!! �ú������� 0 �� U32_BUTT Ϊ�Ƿ�ID
 */
U32 sc_cwq_add_call(SC_SRV_CB *pstSCB, U32 ulAgentGrpID, S8 *szCaller, BOOL bIsAddHead)
{
    SC_CWQ_NODE_ST *pstCWQNode = NULL;
    DLL_NODE_S     *pstDLLNode = NULL;
    SC_INCOMING_CALL_NODE_ST *pstCallNode = NULL;

    if (DOS_ADDR_INVALID(pstSCB) || DOS_ADDR_INVALID(szCaller))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if (0 == ulAgentGrpID || U32_BUTT == ulAgentGrpID)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstDLLNode = dll_find(&g_stCWQMngt, (VOID *)&ulAgentGrpID, sc_cwq_find_agentgrp);
    if (DOS_ADDR_INVALID(pstDLLNode))
    {
        pstDLLNode = dos_dmem_alloc(sizeof(DLL_NODE_S));
        if (DOS_ADDR_INVALID(pstDLLNode))
        {
            DOS_ASSERT(0);

            return DOS_FAIL;
        }
        DLL_Init_Node(pstDLLNode);
        pstDLLNode->pHandle = NULL;

        pthread_mutex_lock(&g_mutexCWQMngt);
        if (bIsAddHead)
        {
            DLL_Add_Head(&g_stCWQMngt, pstDLLNode);
        }
        else
        {
            DLL_Add(&g_stCWQMngt, pstDLLNode);
        }
        pthread_mutex_unlock(&g_mutexCWQMngt);
    }

    pstCWQNode = pstDLLNode->pHandle;
    if (DOS_ADDR_INVALID(pstCWQNode))
    {
        pstCWQNode = dos_dmem_alloc(sizeof(SC_CWQ_NODE_ST));
        if (DOS_ADDR_INVALID(pstCWQNode))
        {
            DOS_ASSERT(0);

            return DOS_FAIL;
        }
        pstCWQNode->ulAgentGrpID = ulAgentGrpID;
        pstCWQNode->ulStartWaitingTime = 0;
        DLL_Init(&pstCWQNode->stCallWaitingQueue);
        pthread_mutex_init(&pstCWQNode->mutexCWQMngt, NULL);

        pstDLLNode->pHandle = pstCWQNode;
    }

    pstDLLNode = dos_dmem_alloc(sizeof(DLL_NODE_S));
    if (DOS_ADDR_INVALID(pstDLLNode))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    DLL_Init_Node(pstDLLNode);

    pstCallNode = (SC_INCOMING_CALL_NODE_ST *)dos_dmem_alloc(sizeof(SC_INCOMING_CALL_NODE_ST));
    if (DOS_ADDR_INVALID(pstCallNode))
    {
        DOS_ASSERT(0);
        dos_dmem_free(pstDLLNode);

        return DOS_FAIL;
    }

    pstCallNode->pstSCB = pstSCB;
    dos_strncpy(pstCallNode->szCaller, szCaller, SC_NUM_LENGTH-1);
    pstCallNode->szCaller[SC_NUM_LENGTH-1] = '\0';

    pstDLLNode->pHandle = pstCallNode;
    //pstSCB->ulInQueueTime = time(NULL);

    pthread_mutex_lock(&pstCWQNode->mutexCWQMngt);
    if (bIsAddHead)
    {
        DLL_Add_Head(&pstCWQNode->stCallWaitingQueue, pstDLLNode);
    }
    else
    {
        DLL_Add(&pstCWQNode->stCallWaitingQueue, pstDLLNode);
    }
    pthread_mutex_unlock(&pstCWQNode->mutexCWQMngt);

    pthread_mutex_lock(&g_mutexCWQMngt);
    pthread_cond_signal(&g_condCWQMngt);
    pthread_mutex_unlock(&g_mutexCWQMngt);

    return DOS_SUCC;
}

/**
 * ����: sc_cwq_del_call
 * ����: ��pstSCB��ָ��ĺ��дӺ��ж�����ɾ����
 * ����:
 *      SC_SCB_ST *pstSCB : ���п��ƿ�
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 */
U32 sc_cwq_del_call(SC_SRV_CB *pstSCB)
{
    DLL_NODE_S                  *pstDLLNode         = NULL;
    DLL_NODE_S                  *pstDLLNode1        = NULL;
    SC_CWQ_NODE_ST              *pstCWQNode         = NULL;
    SC_INCOMING_CALL_NODE_ST    *pstIncomingNode    = NULL;

    if (DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    DLL_Scan(&g_stCWQMngt, pstDLLNode, DLL_NODE_S*)
    {
        if (DOS_ADDR_INVALID(pstDLLNode))
        {
            break;
        }

        pstCWQNode = pstDLLNode->pHandle;
        if (DOS_ADDR_INVALID(pstCWQNode))
        {
            continue;
        }

        pthread_mutex_lock(&pstCWQNode->mutexCWQMngt);
        DLL_Scan(&pstCWQNode->stCallWaitingQueue, pstDLLNode1, DLL_NODE_S*)
        {
            if (DOS_ADDR_INVALID(pstDLLNode1))
            {
                break;
            }

            pstIncomingNode = pstDLLNode1->pHandle;
            if (DOS_ADDR_INVALID(pstIncomingNode))
            {
                continue;
            }

            if (pstIncomingNode->pstSCB == pstSCB)
            {
#if 0
                /* ������֮�󣬼���ȴ�ʱ�� */
                pstSCB->ulInQueueTime = time(NULL) - pstSCB->ulInQueueTime;
#endif
                dll_delete(&pstCWQNode->stCallWaitingQueue, pstDLLNode1);
                DLL_Init_Node(pstDLLNode1);

                dos_dmem_free(pstDLLNode1);
                pstDLLNode1= NULL;
                break;
            }
        }
        pthread_mutex_unlock(&pstCWQNode->mutexCWQMngt);
    }

    return DOS_SUCC;
}

VOID *sc_cwq_runtime(VOID *ptr)
{
    struct timespec         stTimeout;
    DLL_NODE_S              *pstDLLNode  = NULL;
    DLL_NODE_S              *pstDLLNode1 = NULL;
    SC_CWQ_NODE_ST          *pstCWQNode  = NULL;
    SC_SRV_CB               *pstSCB      = NULL;
    SC_MSG_EVT_LEAVE_CALLQUE_ST  stEvtLeaveCallque;
    SC_INCOMING_CALL_NODE_ST *pstCallNode = NULL;
    SC_AGENT_NODE_ST        *pstAgentNode = NULL;

    g_blCWQWaitingExit = DOS_FALSE;
    g_blCWQRunning  = DOS_TRUE;

    while (1)
    {
        if (g_blCWQWaitingExit)
        {
            break;
        }

        pthread_mutex_lock(&g_mutexCWQMngt);
        stTimeout.tv_sec = time(0) + 1;
        stTimeout.tv_nsec = 0;
        pthread_cond_timedwait(&g_condCWQMngt, &g_mutexCWQMngt, &stTimeout);
        pthread_mutex_unlock(&g_mutexCWQMngt);

        if (g_blCWQWaitingExit)
        {
            break;
        }

        DLL_Scan(&g_stCWQMngt, pstDLLNode, DLL_NODE_S*)
        {
            if (DOS_ADDR_INVALID(pstDLLNode))
            {
                DOS_ASSERT(0);
                break;
            }

            pstCWQNode = pstDLLNode->pHandle;
            if (DOS_ADDR_INVALID(pstCWQNode))
            {
                DOS_ASSERT(0);
                continue;
            }

            pthread_mutex_lock(&pstCWQNode->mutexCWQMngt);
            DLL_Scan(&pstCWQNode->stCallWaitingQueue, pstDLLNode1, DLL_NODE_S*)
            {
                if (DOS_ADDR_INVALID(pstDLLNode1))
                {
                    DOS_ASSERT(0);
                    break;
                }

                pstCallNode = pstDLLNode1->pHandle;
                if (DOS_ADDR_INVALID(pstCallNode))
                {
                    DOS_ASSERT(0);
                    continue;
                }

                pstSCB = pstCallNode->pstSCB;
#if 0
                /* ����һ��󣬲Ž�ͨ��ϯ������ȴ�����û�в��ţ�uuid_break���ܽ���ֹͣ�� */
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_ACD), "now : %u, ulEnqueuTime : %u, ulDequeuTime : %u", time(NULL), pstSCB->stIncomingQueue.ulEnqueuTime, pstSCB->stIncomingQueue.ulDequeuTime);
                if (time(NULL) - pstSCB->stIncomingQueue.ulEnqueuTime < 3)
                {
                    continue;
                }
#endif
                /* ��ȡһ����ϯ */
                pstAgentNode = sc_agent_select_by_grpid(pstCWQNode->ulAgentGrpID, pstCallNode->szCaller);
                if (DOS_ADDR_INVALID(pstAgentNode)
                    || DOS_ADDR_INVALID(pstAgentNode->pstAgentInfo))
                {
                    pstCWQNode->ulStartWaitingTime = time(0);

                    sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_ACD), "The group %u has no idel agent. scbNo(%u)", pstCWQNode->ulAgentGrpID, pstSCB->ulSCBNo);
                    break;
                }

                pstAgentNode->pstAgentInfo->bSelected = DOS_TRUE;

                dll_delete(&pstCWQNode->stCallWaitingQueue, pstDLLNode1);

                DLL_Init_Node(pstDLLNode1);
                dos_dmem_free(pstDLLNode1);
                pstDLLNode1 = NULL;
                dos_dmem_free(pstCallNode);
                pstCallNode = NULL;

                /* ������Ϣ�����ڿ���ת��ϯ�� */
                stEvtLeaveCallque.stMsgTag.ulMsgType = SC_EVT_LEACE_CALL_QUEUE;
                stEvtLeaveCallque.stMsgTag.ulSCBNo = pstSCB->ulSCBNo;
                stEvtLeaveCallque.stMsgTag.usInterErr = SC_LEAVE_CALL_QUE_SUCC;
                stEvtLeaveCallque.ulSCBNo = pstSCB->ulSCBNo;
                stEvtLeaveCallque.pstAgentNode = pstAgentNode;

                sc_log(pstAgentNode->pstAgentInfo->bTraceON, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_ACD), "The group %u send leave call queue. AgentID(%d), scbNo(%u)", pstCWQNode->ulAgentGrpID, pstAgentNode->pstAgentInfo, pstSCB->ulSCBNo);

                if (sc_send_event_leave_call_queue_rsp(&stEvtLeaveCallque) != DOS_SUCC)
                {
                    /* TODO ������Ϣʧ�� */
                }
            }
            pthread_mutex_unlock(&pstCWQNode->mutexCWQMngt);
        }
    }

    g_blCWQRunning  = DOS_FALSE;

    return NULL;
}

U32 sc_cwq_init()
{
    DLL_Init(&g_stCWQMngt);

    return DOS_SUCC;
}

U32 sc_cwq_start()
{
    if (g_blCWQRunning)
    {
        DOS_ASSERT(0);

        return DOS_SUCC;
    }

    if (pthread_create(&g_pthCWQMngt, NULL, sc_cwq_runtime, NULL) < 0)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    return DOS_SUCC;
}

U32 sc_cwq_stop()
{
    g_blCWQWaitingExit = DOS_TRUE;

    return DOS_SUCC;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */


