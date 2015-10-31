/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  �ļ���
 *
 *  ����ʱ��:
 *  ��    ��:
 *  ��    ��:
 *  �޸���ʷ:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>
#include <sc_pub.h>
#include <esl.h>
#include "sc_def.h"
#include "sc_debug.h"
#include "sc_acd_def.h"
#include "sc_acd.h"
#include "sc_db.h"

#define SC_MAX_SQL_LEN  1024

static BOOL             g_blExitFlag = DOS_TRUE;
static DLL_S            g_stDBRequestQueue;
static pthread_t        g_pthreadDB;
static pthread_mutex_t  g_mutexDBRequestQueue = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t   g_condDBRequestQueue  = PTHREAD_COND_INITIALIZER;

extern DB_HANDLE_ST         *g_pstSCDBHandle;

static U32 sc_db_save_call_result(SC_DB_MSG_TAG_ST *pstMsg)
{
    SC_DB_MSG_CALL_RESULT_ST *pstCallResult;
    S8                       szSQL[SC_MAX_SQL_LEN];

    if (DOS_ADDR_INVALID(pstMsg))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstCallResult = (SC_DB_MSG_CALL_RESULT_ST *)pstMsg;

    sc_logr_debug(SC_DB, "Save call result. Customer:%u, Task: %u, Agent:%s(%u), Caller:%s, Callee:%s, TerminateCause: %u, Result: %u"
                    , pstCallResult->ulCustomerID, pstCallResult->ulTaskID, pstCallResult->szAgentNum, pstCallResult->ulAgentID
                    , pstCallResult->szCaller, pstCallResult->szCallee, pstCallResult->usTerminateCause, pstCallResult->ulResult);

    dos_snprintf(szSQL, sizeof(szSQL),
                    "INSERT INTO tbl_calltask_result(`id`,`customer_id`,`task_id`,`caller`,`callee`,`agent_num`,`pdd_len`,"
                    "`ring_times`,`answer_time`,`ivr_end_time`,`dtmf_time`,`wait_agent_times`,`time_len`,"
                    "`hold_cnt`,`hold_times`,`release_part`,`terminate_cause`,`result`, `agent_id`) VALUES(NULL, %u, %u, "
                    "\"%s\", \"%s\", \"%s\", %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u)"
                  , pstCallResult->ulCustomerID, pstCallResult->ulTaskID, pstCallResult->szCaller
                  , pstCallResult->szCallee, pstCallResult->szAgentNum, pstCallResult->ulPDDLen
                  , pstCallResult->ulRingTime, pstCallResult->ulAnswerTimeStamp, pstCallResult->ulIVRFinishTime
                  , pstCallResult->ulFirstDTMFTime, pstCallResult->ulWaitAgentTime, pstCallResult->ulTimeLen
                  , pstCallResult->ulHoldCnt, pstCallResult->ulHoldTimeLen, pstCallResult->ucReleasePart
                  , pstCallResult->usTerminateCause, pstCallResult->ulResult, pstCallResult->ulAgentID);

    if (db_query(g_pstSCDBHandle, szSQL, NULL, NULL, 0) != DOS_SUCC)
    {
        sc_logr_error(SC_DB, "%s", "Save call result FAIL.");

        return DOS_FAIL;
    }

    return DOS_SUCC;
}

static VOID sc_db_request_proc(SC_DB_MSG_TAG_ST *pstMsg)
{
    U32 ulResult;
    if (DOS_ADDR_INVALID(pstMsg))
    {
        DOS_ASSERT(0);
        return;
    }

    sc_logr_debug(SC_DB, "Process db request. Type: %u", pstMsg->ulMsgType);

    switch (pstMsg->ulMsgType)
    {
        case SC_MSG_SAVE_CALL_RESULT:
            ulResult = sc_db_save_call_result(pstMsg);
            break;

        default:
            sc_logr_notice(SC_DB, "Invalid msg type(%u)", pstMsg->ulMsgType);
            break;
    }

    dos_dmem_free(pstMsg);
    pstMsg = NULL;

    sc_logr_debug(SC_DB, "Process db request finished. Result: %s", DOS_SUCC == ulResult ? "succ" : "FAIL");
}

static VOID *sc_db_runtime(VOID *ptr)
{
    DLL_NODE_S       *pstDLLNode = NULL;
    SC_DB_MSG_TAG_ST *pstMsg     = NULL;

    g_blExitFlag = DOS_FALSE;

    while (1)
    {
        if (g_blExitFlag)
        {
            sc_logr_info(SC_DB, "%s", "Request exit.");
            break;
        }

        pthread_mutex_lock(&g_mutexDBRequestQueue);
        pthread_cond_wait(&g_condDBRequestQueue, &g_mutexDBRequestQueue);
        pthread_mutex_unlock(&g_mutexDBRequestQueue);

        if (0 == DLL_Count(&g_stDBRequestQueue))
        {
            continue;
        }

        while (1)
        {
            if (0 == DLL_Count(&g_stDBRequestQueue))
            {
                break;
            }

            pthread_mutex_lock(&g_mutexDBRequestQueue);
            pstDLLNode = dll_fetch(&g_stDBRequestQueue);
            pthread_mutex_unlock(&g_mutexDBRequestQueue);

            if (DOS_ADDR_INVALID(pstDLLNode))
            {
                sc_logr_error(SC_DB, "%s", "DB request queue error.");
                break;
            }

            if (DOS_ADDR_INVALID(pstDLLNode->pHandle))
            {
                sc_logr_error(SC_DB, "%s", "DB request is empty.");
                break;
            }

            pstMsg = (SC_DB_MSG_TAG_ST *)pstDLLNode->pHandle;

            DLL_Init_Node(pstDLLNode);
            dos_dmem_free(pstDLLNode);
            pstDLLNode = NULL;

            sc_db_request_proc(pstMsg);
        }
    }

    return NULL;
}

U32 sc_send_msg2db(SC_DB_MSG_TAG_ST *pstMsg)
{
    DLL_NODE_S  *pstNode;

    if (DOS_ADDR_INVALID(pstMsg))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstNode = dos_dmem_alloc(sizeof(DLL_NODE_S));
    if (DOS_ADDR_INVALID(pstNode))
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_DB, "%s", "Send msg to db fail. Alloc memory fail");
        return DOS_FAIL;
    }

    DLL_Init_Node(pstNode);
    pstNode->pHandle = pstMsg;

    pthread_mutex_lock(&g_mutexDBRequestQueue);
    DLL_Add(&g_stDBRequestQueue, pstNode);
    pthread_cond_signal(&g_condDBRequestQueue);
    pthread_mutex_unlock(&g_mutexDBRequestQueue);

    return DOS_SUCC;
}

U32 sc_db_init()
{
    DLL_Init(&g_stDBRequestQueue);

    return DOS_SUCC;
}

U32 sc_db_start()
{
    if (pthread_create(&g_pthreadDB, NULL, sc_db_runtime, NULL) < 0)
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_DB, "%s", "Start the DB task FAIL.");
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

U32 sc_db_stop()
{
    g_blExitFlag = DOS_TRUE;

    return DOS_SUCC;
}


#ifdef __cplusplus
}
#endif /* __cplusplus */

