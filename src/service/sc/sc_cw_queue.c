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
#include "sc_acd_def.h"
#include "sc_acd.h"

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
 * ����: sc_cwq_find_agentgrp
 * ����: �ڲ�����������ϯ����в�����ϯ��
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
#if 0
/**
 * ����: sc_cwq_find_call
 * ����: �ڲ��������ں��ж����в����ض��ĺ���
 * ����:
 *      VOID *pParam : ������������һ��SCB��ָ��
 *      DLL_NODE_S *pstNode : ��ǰ�ڵ�
 * ����ֵ: 0��ʾ�ҵ�һ���ڵ㣬����ֵ��ʾû���ҵ�
 */
static S32 sc_cwq_find_call(VOID *pParam, DLL_NODE_S *pstNode)
{
    SC_SCB_ST *pstSCB = NULL;

    if (DOS_ADDR_INVALID(pParam) || DOS_ADDR_INVALID(pstNode) || DOS_ADDR_INVALID(pstNode->pHandle))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstSCB = pstNode->pHandle;

    if ((SC_SCB_ST *)pParam != pstSCB)
    {
        return DOS_FAIL;
    }

    return DOS_SUCC;
}
#endif

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
U32 sc_cwq_add_call(SC_SCB_ST *pstSCB, U32 ulAgentGrpID)
{
    SC_CWQ_NODE_ST *pstCWQNode = NULL;
    DLL_NODE_S     *pstDLLNode = NULL;

    if (DOS_ADDR_INVALID(pstSCB))
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
        DLL_Add(&g_stCWQMngt, pstDLLNode);
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
    pstDLLNode->pHandle = pstSCB;
    pstSCB->ulInQueueTime = time(NULL);

    pthread_mutex_lock(&pstCWQNode->mutexCWQMngt);
    DLL_Add(&pstCWQNode->stCallWaitingQueue, pstDLLNode);
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
U32 sc_cwq_del_call(SC_SCB_ST *pstSCB)
{
    DLL_NODE_S              *pstDLLNode  = NULL;
    DLL_NODE_S              *pstDLLNode1 = NULL;
    SC_CWQ_NODE_ST          *pstCWQNode  = NULL;
    SC_SCB_ST               *pstSCB1     = NULL;

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

            pstSCB1 = pstDLLNode1->pHandle;
            if (DOS_ADDR_INVALID(pstSCB1))
            {
                continue;
            }

            if (pstSCB1 == pstSCB)
            {
                /* ������֮�󣬼���ȴ�ʱ�� */
                pstSCB->ulInQueueTime = time(NULL) - pstSCB->ulInQueueTime;

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
    SC_SCB_ST               *pstSCB      = NULL;
    BOOL                    blHasIdelAgent = DOS_FALSE;
    S8                      szAPPParam[512]  = {0};

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

                pstSCB = pstDLLNode1->pHandle;
                if (DOS_ADDR_INVALID(pstSCB))
                {
                    DOS_ASSERT(0);
                    continue;
                }

                /* ����һ��󣬲Ž�ͨ��ϯ������ȴ�����û�в��ţ�uuid_break���ܽ���ֹͣ�� */
                if (time(NULL) - pstSCB->ulInQueueTime < 3)
                {
                    continue;
                }

                if (sc_acd_query_idel_agent(pstCWQNode->ulAgentGrpID, &blHasIdelAgent) != DOS_SUCC
                    || !blHasIdelAgent)
                {
                    pstCWQNode->ulStartWaitingTime = time(0);

                    sc_logr_info(SC_ACD, "The group %u has no idel agent. (%d)", pstCWQNode->ulAgentGrpID, blHasIdelAgent);
                    break;
                }

                dll_delete(&pstCWQNode->stCallWaitingQueue, pstDLLNode1);

                DLL_Init_Node(pstDLLNode1);
                dos_dmem_free(pstDLLNode1);
                pstDLLNode1 = NULL;

                dos_snprintf(szAPPParam, sizeof(szAPPParam), "bgapi uuid_break %s all\r\n", pstSCB->szUUID);
                sc_ep_esl_execute_cmd(szAPPParam);

                /* �Ż����� */
                sc_ep_esl_execute("set", "instant_ringback=true", pstSCB->szUUID);
                sc_ep_esl_execute("set", "transfer_ringback=tone_stream://%(1000,4000,450);loops=-1", pstSCB->szUUID);

                if (sc_ep_call_agent_by_grpid(pstSCB, pstCWQNode->ulAgentGrpID) != DOS_SUCC)
                {
                    DOS_ASSERT(0);
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

    pthread_detach(g_pthCWQMngt);

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


