/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  �ļ�����sc_task.c
 *
 *  ����ʱ��: 2014��12��16��10:23:53
 *  ��    ��: Larry
 *  ��    ��: ÿһ��Ⱥ�������ʵ��
 *  �޸���ʷ:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* include public header files */
#include <dos.h>
#include <esl.h>

/* include private header files */
#include "sc_def.h"
#include "sc_res.h"
#include "sc_debug.h"
#include "sc_db.h"
#include "bs_pub.h"
#include "sc_pub.h"
#include "sc_http_api.h"

/* define marcos */

/* define enums */

/* define structs */


/* �����б� refer to struct tagTaskCB*/
SC_TASK_CB           *g_pstTaskList  = NULL;
pthread_mutex_t      g_mutexTaskList = PTHREAD_MUTEX_INITIALIZER;

U32 sc_task_call_result_make_call_before(U32 ulCustomerID, U32 ulTaskID, S8 *szCallingNum, S8 *szCalleeNum, U32 ulSIPRspCode)
{
    SC_DB_MSG_CALL_RESULT_ST *pstCallResult     = NULL;

    pstCallResult = dos_dmem_alloc(sizeof(SC_DB_MSG_CALL_RESULT_ST));
    if (DOS_ADDR_INVALID(pstCallResult))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_TASK), "Analysis call result for task: %u, SIP Code:%u", ulTaskID, ulSIPRspCode);

    dos_memzero(pstCallResult, sizeof(SC_DB_MSG_CALL_RESULT_ST));
    pstCallResult->ulCustomerID = ulCustomerID;
    pstCallResult->ulTaskID = ulTaskID;

    /* ���к��� */
    if (DOS_ADDR_VALID(szCallingNum))
    {
        dos_snprintf(pstCallResult->szCaller, sizeof(pstCallResult->szCaller), "%s", szCallingNum);
    }

    /* ���к��� */
    if (DOS_ADDR_VALID(szCalleeNum))
    {
        dos_snprintf(pstCallResult->szCallee, sizeof(pstCallResult->szCallee), "%s", szCalleeNum);
    }

    pstCallResult->ulAnswerTimeStamp = time(NULL);
    switch (ulSIPRspCode)
    {
        case CC_ERR_SC_CALLER_NUMBER_ILLEGAL:
        case CC_ERR_SC_CALLEE_NUMBER_ILLEGAL:
            pstCallResult->ulResult = CC_RST_CALLING_NUM_INVALID;
            break;

        default:
            pstCallResult->ulResult = CC_RST_CONNECT_FAIL;
            break;
    }

    pstCallResult->stMsgTag.ulMsgType = SC_MSG_SAVE_CALL_RESULT;

    return sc_send_msg2db((SC_DB_MSG_TAG_ST *)pstCallResult);
}

U16 sc_task_transform_errcode_from_sc2sip(U32 ulErrcode)
{
    U16 usErrcodeSC = CC_ERR_SIP_UNDECIPHERABLE;

    if (ulErrcode >= CC_ERR_BUTT)
    {
        DOS_ASSERT(0);
        return usErrcodeSC;
    }

    if (ulErrcode < 1000 && ulErrcode > 99)
    {
        /* 1000����Ϊsip�����룬����Ҫת�� */
        return ulErrcode;
    }

    switch (ulErrcode)
    {
        case CC_ERR_NORMAL_CLEAR:
            usErrcodeSC = CC_ERR_SIP_SUCC;
            break;
        case CC_ERR_NO_REASON:
            usErrcodeSC = CC_ERR_SIP_BUSY_EVERYWHERE;
            break;
        case CC_ERR_SC_SERV_NOT_EXIST:
        case CC_ERR_SC_NO_SERV_RIGHTS:
        case CC_ERR_SC_USER_DOES_NOT_EXIST:
        case CC_ERR_SC_CUSTOMERS_NOT_EXIST:
            usErrcodeSC = CC_ERR_SIP_FORBIDDEN;
            break;
        case CC_ERR_SC_USER_OFFLINE:
        case CC_ERR_SC_USER_HAS_BEEN_LEFT:
        case CC_ERR_SC_PERIOD_EXCEED:
        case CC_ERR_SC_RESOURCE_EXCEED:
            usErrcodeSC = CC_ERR_SIP_TEMPORARILY_UNAVAILABLE;
            break;
        case CC_ERR_SC_USER_BUSY:
            usErrcodeSC = CC_ERR_SIP_BUSY_HERE;
            break;
        case CC_ERR_SC_CB_ALLOC_FAIL:
        case CC_ERR_SC_MEMORY_ALLOC_FAIL:
            usErrcodeSC = CC_ERR_SIP_INTERNAL_SERVER_ERROR;
            break;
        case CC_ERR_SC_IN_BLACKLIST:
        case CC_ERR_SC_CALLER_NUMBER_ILLEGAL:
        case CC_ERR_SC_CALLEE_NUMBER_ILLEGAL:
            usErrcodeSC = CC_ERR_SIP_NOT_FOUND;
            break;
        case CC_ERR_SC_NO_ROUTE:
        case CC_ERR_SC_NO_TRUNK:
            break;
        case CC_ERR_SC_MESSAGE_TIMEOUT:
        case CC_ERR_SC_AUTH_TIMEOUT:
        case CC_ERR_SC_QUERY_TIMEOUT:
            usErrcodeSC = CC_ERR_SIP_REQUEST_TIMEOUT;
            break;
        case CC_ERR_SC_CONFIG_ERR:
        case CC_ERR_SC_MESSAGE_PARAM_ERR:
        case CC_ERR_SC_MESSAGE_SENT_ERR:
        case CC_ERR_SC_MESSAGE_RECV_ERR:
        case CC_ERR_SC_CLEAR_FORCE:
        case CC_ERR_SC_SYSTEM_ABNORMAL:
        case CC_ERR_SC_SYSTEM_BUSY:
        case CC_ERR_SC_SYSTEM_MAINTAINING:
            usErrcodeSC = CC_ERR_SIP_SERVICE_UNAVAILABLE;
            break;
        case CC_ERR_BS_NOT_EXIST:
        case CC_ERR_BS_EXPIRE:
        case CC_ERR_BS_FROZEN:
        case CC_ERR_BS_LACK_FEE:
        case CC_ERR_BS_PASSWORD:
        case CC_ERR_BS_RESTRICT:
        case CC_ERR_BS_OVER_LIMIT:
        case CC_ERR_BS_TIMEOUT:
        case CC_ERR_BS_LINK_DOWN:
        case CC_ERR_BS_SYSTEM:
        case CC_ERR_BS_MAINTAIN:
        case CC_ERR_BS_DATA_ABNORMAL:
        case CC_ERR_BS_PARAM_ERR:
        case CC_ERR_BS_NOT_MATCH:
            usErrcodeSC = CC_ERR_SIP_PAYMENT_REQUIRED;
            break;
        default:
            DOS_ASSERT(0);
            break;
    }

    return usErrcodeSC;
}

U32 sc_preview_task_call_result(SC_SRV_CB *pstSCB, U32 ulLegNo, U32 ulSIPRspCode)
{
    SC_DB_MSG_CALL_RESULT_ST *pstCallResult     = NULL;
    SC_LEG_CB                *pstCallingLegCB   = NULL;
    SC_LEG_CB                *pstCalleeLegCB    = NULL;
    SC_LEG_CB                *pstHungupLegCB    = NULL;
    SC_AGENT_NODE_ST         *pstAgentCall      = NULL;
    SC_TASK_CB               *pstTCB            = NULL;

    if (DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstHungupLegCB = sc_lcb_get(ulLegNo);
    if (DOS_ADDR_INVALID(pstHungupLegCB))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if (0 == pstSCB->stAutoPreview.ulTaskID || U32_BUTT == pstSCB->stAutoPreview.ulTaskID)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstTCB = sc_tcb_find_by_taskid(pstSCB->stAutoPreview.ulTaskID);
    if (DOS_ADDR_INVALID(pstTCB))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstCalleeLegCB = sc_lcb_get(pstSCB->stAutoPreview.ulCalleeLegNo);
    if (DOS_ADDR_INVALID(pstCalleeLegCB))
    {
        pstCallingLegCB = sc_lcb_get(pstSCB->stAutoPreview.ulCallingLegNo);
        if (DOS_ADDR_INVALID(pstCallingLegCB))
        {
            return DOS_FAIL;
        }

        /* ��ϯ���峬ʱ */
        pstCallResult = dos_dmem_alloc(sizeof(SC_DB_MSG_CALL_RESULT_ST));
        if (DOS_ADDR_INVALID(pstCallResult))
        {
            DOS_ASSERT(0);

            return DOS_FAIL;
        }

        sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_TASK), "Analysis call result for task: %u, SIP Code:%u", pstSCB->stAutoCall.ulTaskID, ulSIPRspCode);

        dos_memzero(pstCallResult, sizeof(SC_DB_MSG_CALL_RESULT_ST));
        pstCallResult->ulCustomerID = pstSCB->ulCustomerID; /* �ͻ�ID,Ҫ��ȫ����,������10λ,���λС��4 */

        /* ��ϯID,Ҫ��ȫ����,������10λ,���λС��4 */
        if (U32_BUTT != pstSCB->stAutoPreview.ulAgentID)
        {
            pstCallResult->ulAgentID = pstSCB->stAutoPreview.ulAgentID;
        }
        else
        {
            pstCallResult->ulAgentID = 0;
        }
        pstCallResult->ulTaskID = pstSCB->stAutoPreview.ulTaskID;       /* ����ID,Ҫ��ȫ����,������10λ,���λС��4 */

        pstAgentCall = sc_agent_get_by_id(pstSCB->stAutoPreview.ulAgentID);
        if (DOS_ADDR_VALID(pstAgentCall)
            && DOS_ADDR_VALID(pstAgentCall->pstAgentInfo))
        {
            dos_snprintf(pstCallResult->szAgentNum, sizeof(pstCallResult->szAgentNum), "%s", pstAgentCall->pstAgentInfo->szEmpNo);
        }

        /* ���к��� */
        if ('\0' != pstCallingLegCB->stCall.stNumInfo.szOriginalCalling[0])
        {
            dos_snprintf(pstCallResult->szCaller, sizeof(pstCallResult->szCaller), "%s", pstCallingLegCB->stCall.stNumInfo.szOriginalCalling);
        }

        /* ���к��� */
        if ('\0' != pstCallingLegCB->stCall.stNumInfo.szOriginalCallee[0])
        {
            dos_snprintf(pstCallResult->szCallee, sizeof(pstCallResult->szCallee), "%s", pstCallingLegCB->stCall.stNumInfo.szOriginalCallee);
        }

        /* ����ʱ��:�ӷ�����е��յ����� */
        pstCallResult->ulPDDLen = pstCallingLegCB->stCall.stTimeInfo.ulRingTime - pstCallingLegCB->stCall.stTimeInfo.ulStartTime;
        pstCallResult->ulStartTime = pstCallingLegCB->stCall.stTimeInfo.ulStartTime;
        pstCallResult->ulRingTime = pstCallingLegCB->stCall.stTimeInfo.ulRingTime;                  /* ����ʱ��,��λ:�� */
        pstCallResult->ulAnswerTimeStamp = pstCallingLegCB->stCall.stTimeInfo.ulAnswerTime;         /* Ӧ��ʱ��� */
        pstCallResult->ulFirstDTMFTime = pstCallingLegCB->stCall.stTimeInfo.ulDTMFStartTime;        /* ��һ�����β���ʱ��,��λ:�� */
        pstCallResult->ulIVRFinishTime = pstCallingLegCB->stCall.stTimeInfo.ulIVREndTime;           /* IVR�������ʱ��,��λ:�� */

        /* ����ʱ��,��λ:�� */
        pstCallResult->ulTimeLen = 0;

        if (pstSCB->stIncomingQueue.ulEnqueuTime != 0)
        {
            if (pstSCB->stIncomingQueue.ulDequeuTime != 0)
            {
                pstCallResult->ulWaitAgentTime = pstSCB->stIncomingQueue.ulDequeuTime - pstSCB->stIncomingQueue.ulEnqueuTime;
            }
            else
            {
                pstCallResult->ulWaitAgentTime = time(NULL) - pstSCB->stIncomingQueue.ulEnqueuTime;
            }
        }

        pstCallResult->ulHoldCnt = pstSCB->stHold.ulHoldCount;                  /* ���ִ��� */
        //pstCallResult->ulHoldTimeLen = pstSCB->usHoldTotalTime;              /* ������ʱ��,��λ:�� */
        //pstCallResult->usTerminateCause = pstSCB->usTerminationCause;           /* ��ֹԭ�� */
        if (ulLegNo == pstSCB->stAutoPreview.ulCallingLegNo)
        {
            pstCallResult->ucReleasePart = SC_CALLING;
        }
        else
        {
            pstCallResult->ucReleasePart = SC_CALLEE;
        }

        pstCallResult->ulResult = CC_RST_AGENT_NO_ANSER;
        goto proc_finished;
    }

    pstCallResult = dos_dmem_alloc(sizeof(SC_DB_MSG_CALL_RESULT_ST));
    if (DOS_ADDR_INVALID(pstCallResult))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_TASK), "Analysis call result for task: %u, SIP Code:%u", pstSCB->stAutoCall.ulTaskID, ulSIPRspCode);

    if (ulSIPRspCode >= CC_ERR_SC_SERV_NOT_EXIST)
    {
         ulSIPRspCode = sc_task_transform_errcode_from_sc2sip(ulSIPRspCode);
    }

    dos_memzero(pstCallResult, sizeof(SC_DB_MSG_CALL_RESULT_ST));
    pstCallResult->ulCustomerID = pstSCB->ulCustomerID; /* �ͻ�ID,Ҫ��ȫ����,������10λ,���λС��4 */

    /* ��ϯID,Ҫ��ȫ����,������10λ,���λС��4 */
    if (U32_BUTT != pstSCB->stAutoPreview.ulAgentID)
    {
        pstCallResult->ulAgentID = pstSCB->stAutoPreview.ulAgentID;
    }
    else
    {
        pstCallResult->ulAgentID = 0;
    }
    pstCallResult->ulTaskID = pstSCB->stAutoPreview.ulTaskID;       /* ����ID,Ҫ��ȫ����,������10λ,���λС��4 */

    if (pstCalleeLegCB->ulCBNo != ulLegNo)
    {
        pstCalleeLegCB->stCall.stTimeInfo.ulByeTime = pstHungupLegCB->stCall.stTimeInfo.ulByeTime;
    }

    /* ��ϯ����(����) */
    pstAgentCall = sc_agent_get_by_id(pstSCB->stAutoPreview.ulAgentID);

    if (DOS_ADDR_VALID(pstAgentCall)
        && DOS_ADDR_VALID(pstAgentCall->pstAgentInfo))
    {
        dos_snprintf(pstCallResult->szAgentNum, sizeof(pstCallResult->szAgentNum), "%s", pstAgentCall->pstAgentInfo->szEmpNo);
    }

    /* ���к��� */
    if ('\0' != pstCalleeLegCB->stCall.stNumInfo.szOriginalCalling[0])
    {
        dos_snprintf(pstCallResult->szCaller, sizeof(pstCallResult->szCaller), "%s", pstCalleeLegCB->stCall.stNumInfo.szOriginalCalling);
    }

    /* ���к��� */
    if ('\0' != pstCalleeLegCB->stCall.stNumInfo.szOriginalCallee[0])
    {
        dos_snprintf(pstCallResult->szCallee, sizeof(pstCallResult->szCallee), "%s", pstCalleeLegCB->stCall.stNumInfo.szOriginalCallee);
    }

    /* ����ʱ��:�ӷ�����е��յ����� */
    if (0 == pstCalleeLegCB->stCall.stTimeInfo.ulRingTime || 0 == pstCalleeLegCB->stCall.stTimeInfo.ulStartTime)
    {
        pstCallResult->ulPDDLen = 0;
    }
    else
    {
        pstCallResult->ulPDDLen = pstCalleeLegCB->stCall.stTimeInfo.ulRingTime - pstCalleeLegCB->stCall.stTimeInfo.ulStartTime;
    }
    pstCallResult->ulStartTime = pstCalleeLegCB->stCall.stTimeInfo.ulStartTime;
    pstCallResult->ulRingTime = pstCalleeLegCB->stCall.stTimeInfo.ulRingTime;                  /* ����ʱ��,��λ:�� */
    pstCallResult->ulAnswerTimeStamp = pstCalleeLegCB->stCall.stTimeInfo.ulAnswerTime;         /* Ӧ��ʱ��� */
    pstCallResult->ulFirstDTMFTime = pstCalleeLegCB->stCall.stTimeInfo.ulDTMFStartTime;        /* ��һ�����β���ʱ��,��λ:�� */
    pstCallResult->ulIVRFinishTime = pstCalleeLegCB->stCall.stTimeInfo.ulIVREndTime;           /* IVR�������ʱ��,��λ:�� */

    /* ����ʱ��,��λ:�� */
    if (0 == pstCalleeLegCB->stCall.stTimeInfo.ulByeTime || 0 == pstCalleeLegCB->stCall.stTimeInfo.ulAnswerTime)
    {
        pstCallResult->ulTimeLen = 0;
    }
    else
    {
        pstCallResult->ulTimeLen = pstCalleeLegCB->stCall.stTimeInfo.ulByeTime - pstCalleeLegCB->stCall.stTimeInfo.ulAnswerTime;
    }

    if (pstSCB->stIncomingQueue.ulEnqueuTime != 0)
    {
        if (pstSCB->stIncomingQueue.ulDequeuTime != 0)
        {
            pstCallResult->ulWaitAgentTime = pstSCB->stIncomingQueue.ulDequeuTime - pstSCB->stIncomingQueue.ulEnqueuTime;
        }
        else
        {
            pstCallResult->ulWaitAgentTime = time(NULL) - pstSCB->stIncomingQueue.ulEnqueuTime;
        }
    }

    pstCallResult->ulHoldCnt = pstSCB->stHold.ulHoldCount;                  /* ���ִ��� */
    //pstCallResult->ulHoldTimeLen = pstSCB->usHoldTotalTime;              /* ������ʱ��,��λ:�� */
    //pstCallResult->usTerminateCause = pstSCB->usTerminationCause;           /* ��ֹԭ�� */
    if (ulLegNo == pstSCB->stAutoPreview.ulCallingLegNo)
    {
        pstCallResult->ucReleasePart = SC_CALLING;
    }
    else
    {
        pstCallResult->ucReleasePart = SC_CALLEE;
    }

    pstCallingLegCB = sc_lcb_get(pstSCB->stAutoPreview.ulCallingLegNo);

    pstCallResult->ulResult = CC_RST_BUTT;

    if (CC_ERR_SIP_SUCC == ulSIPRspCode
        || CC_ERR_NORMAL_CLEAR == ulSIPRspCode)
    {
        /* ��ϯȫæ */
        if (pstSCB->stIncomingQueue.ulEnqueuTime != 0
            && pstSCB->stIncomingQueue.ulDequeuTime == 0)
        {
            pstCallResult->ulAnswerTimeStamp = pstCallResult->ulStartTime ? pstCallResult->ulStartTime : time(NULL);
            pstCallResult->ulResult = CC_RST_AGNET_BUSY;
            goto proc_finished;
        }

        if (pstCallResult->ulAnswerTimeStamp == 0)
        {
            /* δ���� */
            pstCallResult->ulAnswerTimeStamp = pstCallResult->ulRingTime;
            pstCallResult->ulResult = CC_RST_NO_ANSWER;
            goto proc_finished;
        }

        if (SC_CALLEE == pstCallResult->ucReleasePart)
        {
            pstCallResult->ulResult = CC_RST_CUSTOMER_HANGUP;
        }
        else
        {
            pstCallResult->ulResult = CC_RST_AGENT_HANGUP;
        }
    }
    else
    {
        switch (ulSIPRspCode)
        {
            case CC_ERR_SIP_NOT_FOUND:
                pstCallResult->ulResult = CC_RST_NOT_FOUND;
                break;

            case CC_ERR_SIP_TEMPORARILY_UNAVAILABLE:
                pstCallResult->ulAnswerTimeStamp = pstCallResult->ulRingTime;
                pstCallResult->ulResult = CC_RST_REJECTED;
                break;

            case CC_ERR_SIP_BUSY_HERE:
                if (pstCallResult->ucReleasePart == SC_CALLING)
                {
                    pstCallResult->ulResult = CC_RST_AGENT_NO_ANSER;
                }
                else
                {
                    pstCallResult->ulResult = CC_RST_BUSY;
                }
                break;

            case CC_ERR_SIP_REQUEST_TIMEOUT:
            case CC_ERR_SIP_REQUEST_TERMINATED:
                if (DOS_ADDR_INVALID(pstCalleeLegCB))
                {
                    pstCallResult->ulResult = CC_RST_NO_ANSWER;
                }
                else
                {
                    pstCallResult->ulResult = CC_RST_AGENT_NO_ANSER;
                }
                break;

            case CC_ERR_SC_CALLEE_NUMBER_ILLEGAL:
                pstCallResult->ulResult = CC_RST_CALLING_NUM_INVALID;
                break;

            default:
                pstCallResult->ulAnswerTimeStamp = pstCallResult->ulStartTime ? pstCallResult->ulStartTime : time(NULL);
                pstCallResult->ulResult = CC_RST_CONNECT_FAIL;
                break;
        }
    }

proc_finished:

    if (pstCallResult->ulAnswerTimeStamp == 0)
    {
        pstCallResult->ulAnswerTimeStamp = time(NULL);
    }

    if (CC_RST_BUTT == pstCallResult->ulResult)
    {
        pstCallResult->ulResult = CC_RST_CONNECT_FAIL;
    }

    pstCallResult->stMsgTag.ulMsgType = SC_MSG_SAVE_CALL_RESULT;
    return sc_send_msg2db((SC_DB_MSG_TAG_ST *)pstCallResult);
}

U32 sc_task_call_result(SC_SRV_CB *pstSCB, U32 ulLegNo, U32 ulSIPRspCode, U32 ulStatus)
{
    SC_DB_MSG_CALL_RESULT_ST *pstCallResult     = NULL;
    SC_LEG_CB                *pstCallingLegCB   = NULL;
    SC_LEG_CB                *pstCalleeLegCB    = NULL;
    SC_LEG_CB                *pstHungupLegCB    = NULL;
    SC_AGENT_NODE_ST         *pstAgentCall      = NULL;
    SC_TASK_CB               *pstTCB            = NULL;

    if (DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstHungupLegCB = sc_lcb_get(ulLegNo);
    if (DOS_ADDR_INVALID(pstHungupLegCB))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if (0 == pstSCB->stAutoCall.ulTaskID || U32_BUTT == pstSCB->stAutoCall.ulTaskID)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstTCB = sc_tcb_find_by_taskid(pstSCB->stAutoCall.ulTaskID);
    if (DOS_ADDR_INVALID(pstTCB))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstCallingLegCB = sc_lcb_get(pstSCB->stAutoCall.ulCallingLegNo);
    if (DOS_ADDR_INVALID(pstCallingLegCB))
    {
        return DOS_FAIL;
    }

    pstCallResult = dos_dmem_alloc(sizeof(SC_DB_MSG_CALL_RESULT_ST));
    if (DOS_ADDR_INVALID(pstCallResult))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_log(pstSCB->bTrace, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_TASK), "Analysis call result for task: %u, SIP Code:%u", pstSCB->stAutoCall.ulTaskID, ulSIPRspCode);

    if (ulSIPRspCode >= CC_ERR_SC_SERV_NOT_EXIST)
    {
         ulSIPRspCode = sc_task_transform_errcode_from_sc2sip(ulSIPRspCode);
    }

    dos_memzero(pstCallResult, sizeof(SC_DB_MSG_CALL_RESULT_ST));
    pstCallResult->ulCustomerID = pstSCB->ulCustomerID; /* �ͻ�ID,Ҫ��ȫ����,������10λ,���λС��4 */

    /* ��ϯID,Ҫ��ȫ����,������10λ,���λС��4 */
    if (U32_BUTT != pstSCB->stAutoCall.ulAgentID)
    {
        pstCallResult->ulAgentID = pstSCB->stAutoCall.ulAgentID;
    }
    else
    {
        pstCallResult->ulAgentID = 0;
    }
    pstCallResult->ulTaskID = pstSCB->stAutoCall.ulTaskID;       /* ����ID,Ҫ��ȫ����,������10λ,���λС��4 */

    if (pstCallingLegCB->ulCBNo != ulLegNo)
    {
        pstCallingLegCB->stCall.stTimeInfo.ulByeTime = pstHungupLegCB->stCall.stTimeInfo.ulByeTime;
    }

    /* ��ϯ����(����) */
    pstAgentCall = sc_agent_get_by_id(pstSCB->stAutoCall.ulAgentID);

    if (DOS_ADDR_VALID(pstAgentCall)
        && DOS_ADDR_VALID(pstAgentCall->pstAgentInfo))
    {
        dos_snprintf(pstCallResult->szAgentNum, sizeof(pstCallResult->szAgentNum), "%s", pstAgentCall->pstAgentInfo->szEmpNo);
    }

    /* ���к��� */
    if ('\0' != pstCallingLegCB->stCall.stNumInfo.szOriginalCalling[0])
    {
        dos_snprintf(pstCallResult->szCaller, sizeof(pstCallResult->szCaller), "%s", pstCallingLegCB->stCall.stNumInfo.szOriginalCalling);
    }

    /* ���к��� */
    if ('\0' != pstCallingLegCB->stCall.stNumInfo.szOriginalCallee[0])
    {
        dos_snprintf(pstCallResult->szCallee, sizeof(pstCallResult->szCallee), "%s", pstCallingLegCB->stCall.stNumInfo.szOriginalCallee);
    }

    /* ����ʱ��:�ӷ�����е��յ����� */
    if (0 == pstCallingLegCB->stCall.stTimeInfo.ulRingTime || 0 == pstCallingLegCB->stCall.stTimeInfo.ulStartTime)
    {
        pstCallResult->ulPDDLen = 0;
    }
    else
    {
        pstCallResult->ulPDDLen = pstCallingLegCB->stCall.stTimeInfo.ulRingTime - pstCallingLegCB->stCall.stTimeInfo.ulStartTime;
    }
    pstCallResult->ulStartTime = pstCallingLegCB->stCall.stTimeInfo.ulStartTime;
    pstCallResult->ulRingTime = pstCallingLegCB->stCall.stTimeInfo.ulRingTime;                  /* ����ʱ��,��λ:�� */
    pstCallResult->ulAnswerTimeStamp = pstCallingLegCB->stCall.stTimeInfo.ulAnswerTime;         /* Ӧ��ʱ��� */
    pstCallResult->ulFirstDTMFTime = pstCallingLegCB->stCall.stTimeInfo.ulDTMFStartTime;        /* ��һ�����β���ʱ��,��λ:�� */
    pstCallResult->ulIVRFinishTime = pstCallingLegCB->stCall.stTimeInfo.ulIVREndTime;           /* IVR�������ʱ��,��λ:�� */

    /* ����ʱ��,��λ:�� */
    if (0 == pstCallingLegCB->stCall.stTimeInfo.ulByeTime || 0 == pstCallingLegCB->stCall.stTimeInfo.ulAnswerTime)
    {
        pstCallResult->ulTimeLen = 0;
    }
    else
    {
        pstCallResult->ulTimeLen = pstCallingLegCB->stCall.stTimeInfo.ulByeTime - pstCallingLegCB->stCall.stTimeInfo.ulAnswerTime;
    }

    if (pstSCB->stIncomingQueue.ulEnqueuTime != 0)
    {
        if (pstSCB->stIncomingQueue.ulDequeuTime != 0)
        {
            pstCallResult->ulWaitAgentTime = pstSCB->stIncomingQueue.ulDequeuTime - pstSCB->stIncomingQueue.ulEnqueuTime;
        }
        else
        {
            pstCallResult->ulWaitAgentTime = time(NULL) - pstSCB->stIncomingQueue.ulEnqueuTime;
        }
    }

    pstCallResult->ulHoldCnt = pstSCB->stHold.ulHoldCount;                 /* ���ִ��� */
    //pstCallResult->ulHoldTimeLen = pstSCB->usHoldTotalTime;              /* ������ʱ��,��λ:�� */
    //pstCallResult->usTerminateCause = pstSCB->usTerminationCause;        /* ��ֹԭ�� */
    if (ulLegNo == pstSCB->stAutoCall.ulCallingLegNo)
    {
        pstCallResult->ucReleasePart = SC_CALLING;
    }
    else
    {
        pstCallResult->ucReleasePart = SC_CALLEE;
    }

    pstCalleeLegCB = sc_lcb_get(pstSCB->stAutoCall.ulCalleeLegNo);

    pstCallResult->ulResult = CC_RST_BUTT;

    if (CC_ERR_SIP_SUCC == ulSIPRspCode
        || CC_ERR_NORMAL_CLEAR == ulSIPRspCode)
    {
        /* ��ϯȫæ */
        if (pstSCB->stIncomingQueue.ulEnqueuTime != 0
            && pstSCB->stIncomingQueue.ulDequeuTime == 0)
        {
            pstCallResult->ulAnswerTimeStamp = pstCallResult->ulStartTime ? pstCallResult->ulStartTime : time(NULL);
            pstCallResult->ulResult = CC_RST_AGNET_BUSY;
            goto proc_finished;
        }

        if (pstCallResult->ulAnswerTimeStamp == 0
            && DOS_ADDR_INVALID(pstCalleeLegCB))
        {
            /* δ���� */
            pstCallResult->ulAnswerTimeStamp = pstCallResult->ulRingTime;
            pstCallResult->ulResult = CC_RST_NO_ANSWER;
            goto proc_finished;
        }

        if (pstTCB->ucMode != SC_TASK_MODE_DIRECT4AGETN)
        {
            /* �п��ܷ���ȷʵû�н������ͻ��Ͱ�����,����Ӧ�����ȴ��� */
            if (pstCallResult->ulFirstDTMFTime
                && DOS_ADDR_INVALID(pstCalleeLegCB))
            {
                pstCallResult->ulResult = CC_RST_HANGUP_AFTER_KEY;
                goto proc_finished;
            }

            /* ��������ʱ�Ҷ� */
            if (0 == pstCallResult->ulIVRFinishTime)
            {
                pstCallResult->ulAnswerTimeStamp = pstCallResult->ulRingTime;
                pstCallResult->ulResult = CC_RST_HANGUP_WHILE_IVR;
                goto proc_finished;
            }

            /* �����Ѿ������ˣ����Һ���û���ڶ��У�˵�������Ѿ���ת����ϯ�� */
            if (pstCallResult->ulIVRFinishTime && DOS_ADDR_VALID(pstCalleeLegCB))
            {
                /* ANSWERΪ0��˵����ϯû�н�ͨ�ȴ���ϯʱ �Ҷϵ� */
                if (DOS_ADDR_VALID(pstCalleeLegCB)
                    && !pstCalleeLegCB->stCall.stTimeInfo.ulAnswerTime)
                {
                    if (SC_CALLEE == pstCallResult->ucReleasePart)
                    {
                        pstCallResult->ulResult = CC_RST_AGENT_NO_ANSER;
                    }
                    else
                    {
                        pstCallResult->ulResult = CC_RST_HANGUP_NO_ANSER;
                    }

                    goto proc_finished;
                }
            }
        }

        if (SC_CALLEE == pstCallResult->ucReleasePart)
        {
            pstCallResult->ulResult = CC_RST_AGENT_HANGUP;
        }
        else
        {
            if (pstTCB->ucMode != SC_TASK_MODE_AUDIO_ONLY
                && ulStatus == SC_AUTO_CALL_EXEC2)
            {
                /* ���⴦����ϯ���в�ͬ����� */
                pstCallResult->ulResult = CC_RST_HANGUP_NO_ANSER;
            }
            else
            {
                pstCallResult->ulResult = CC_RST_CUSTOMER_HANGUP;
            }
        }
    }
    else
    {
        switch (ulSIPRspCode)
        {
            case CC_ERR_SIP_NOT_FOUND:
                pstCallResult->ulResult = CC_RST_NOT_FOUND;
                break;

            case CC_ERR_SIP_TEMPORARILY_UNAVAILABLE:
                pstCallResult->ulAnswerTimeStamp = pstCallResult->ulRingTime;
                pstCallResult->ulResult = CC_RST_REJECTED;
                break;

            case CC_ERR_SIP_BUSY_HERE:
                pstCallResult->ulAnswerTimeStamp = pstCallResult->ulStartTime;
                pstCallResult->ulResult = CC_RST_BUSY;
                break;

            case CC_ERR_SIP_REQUEST_TIMEOUT:
            case CC_ERR_SIP_REQUEST_TERMINATED:
                if (DOS_ADDR_INVALID(pstCalleeLegCB))
                {
                    pstCallResult->ulResult = CC_RST_NO_ANSWER;
                }
                else
                {
                    pstCallResult->ulResult = CC_RST_AGENT_NO_ANSER;
                }
                break;

            case CC_ERR_SC_CALLEE_NUMBER_ILLEGAL:
                pstCallResult->ulResult = CC_RST_CALLING_NUM_INVALID;
                break;

            default:
                pstCallResult->ulAnswerTimeStamp = pstCallResult->ulStartTime ? pstCallResult->ulStartTime : time(NULL);
                pstCallResult->ulResult = CC_RST_CONNECT_FAIL;
                break;
        }
    }

proc_finished:

    if (pstCallResult->ulAnswerTimeStamp == 0)
    {
        pstCallResult->ulAnswerTimeStamp = time(NULL);
    }

    if (CC_RST_BUTT == pstCallResult->ulResult)
    {
        pstCallResult->ulResult = CC_RST_CONNECT_FAIL;
    }

    pstCallResult->stMsgTag.ulMsgType = SC_MSG_SAVE_CALL_RESULT;
    return sc_send_msg2db((SC_DB_MSG_TAG_ST *)pstCallResult);
}

U32 sc_task_get_mode(U32 ulTCBNo)
{
    if (ulTCBNo > SC_MAX_TASK_NUM)
    {
        DOS_ASSERT(0);

        return U32_BUTT;
    }

    if (!g_pstTaskList[ulTCBNo].ucValid)
    {
        DOS_ASSERT(0);

        return U32_BUTT;
    }

    return g_pstTaskList[ulTCBNo].ucMode;
}

U32 sc_task_get_playcnt(U32 ulTCBNo)
{
    if (ulTCBNo > SC_MAX_TASK_NUM)
    {
        DOS_ASSERT(0);

        return 0;
    }

    if (!g_pstTaskList[ulTCBNo].ucValid)
    {
        DOS_ASSERT(0);

        return 0;
    }

    return g_pstTaskList[ulTCBNo].ucAudioPlayCnt;
}

S8 *sc_task_get_audio_file(U32 ulTCBNo)
{
    if (ulTCBNo > SC_MAX_TASK_NUM)
    {
        DOS_ASSERT(0);

        return NULL;
    }

    if (!g_pstTaskList[ulTCBNo].ucValid)
    {
        DOS_ASSERT(0);

        return NULL;
    }

    return g_pstTaskList[ulTCBNo].szAudioFileLen;
}

U32 sc_task_get_agent_queue(U32 ulTCBNo)
{
    if (ulTCBNo > SC_MAX_TASK_NUM)
    {
        DOS_ASSERT(0);

        return U32_BUTT;
    }

    if (!g_pstTaskList[ulTCBNo].ucValid)
    {
        DOS_ASSERT(0);

        return U32_BUTT;
    }

    return g_pstTaskList[ulTCBNo].ulAgentQueueID;
}

VOID sc_task_update_calledcnt(U64 ulArg)
{
    SC_DB_MSG_TAG_ST    *pstMsg     = NULL;
    SC_TASK_CB          *pstTCB     = NULL;
    S8                  szSQL[512]  = { 0 };

    pstTCB = (SC_TASK_CB *)ulArg;
    if (DOS_ADDR_INVALID(pstTCB))
    {
        return;
    }

    if (pstTCB->ulCalledCountLast == pstTCB->ulCalledCount)
    {
        return;
    }

    pstTCB->ulCalledCountLast = pstTCB->ulCalledCount;

    dos_snprintf(szSQL, sizeof(szSQL), "UPDATE tbl_calltask SET calledcnt=%u WHERE id=%u", pstTCB->ulCalledCount, pstTCB->ulTaskID);

    pstMsg = (SC_DB_MSG_TAG_ST *)dos_dmem_alloc(sizeof(SC_DB_MSG_TAG_ST));
    if (DOS_ADDR_INVALID(pstMsg))
    {
        DOS_ASSERT(0);

        return;
    }
    pstMsg->ulMsgType = SC_MSG_SAVE_TASK_CALLED_COUNT;
    pstMsg->szData = dos_dmem_alloc(dos_strlen(szSQL) + 1);
    if (DOS_ADDR_INVALID(pstMsg->szData))
    {
        DOS_ASSERT(0);
        dos_dmem_free(pstMsg->szData);

        return;
    }

    dos_strcpy(pstMsg->szData, szSQL);

    sc_send_msg2db(pstMsg);

    return;
}


/*
 * ����: SC_TEL_NUM_QUERY_NODE_ST *sc_task_get_callee(SC_TASK_CB_ST *pstTCB)
 * ����: ��ȡ���к���
 * ����:
 *      SC_TASK_CB_ST *pstTCB: ������ƿ�
 * ����ֵ: �ɹ����ر��к�����ƿ�ָ��(�Ѿ��������ˣ�����ʹ����֮��Ҫ�ͷ���Դ)�����򷵻�NULL
 * ���øú���֮����������˺Ϸ�ֵ����Ҫ�ͷŸ���Դ
 */
SC_TEL_NUM_QUERY_NODE_ST *sc_task_get_callee(SC_TASK_CB *pstTCB)
{
    SC_TEL_NUM_QUERY_NODE_ST *pstCallee = NULL;
    list_t                   *pstList = NULL;
    U32                      ulCount = 0;

    if (DOS_ADDR_INVALID(pstTCB))
    {
        DOS_ASSERT(0);
        return NULL;
    }

    if (dos_list_is_empty(&pstTCB->stCalleeNumQuery))
    {
        ulCount = sc_task_load_callee(pstTCB);
        sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_TASK), "Load callee number for task %d. Load result : %d", pstTCB->ulTaskID, ulCount);
    }

    if (dos_list_is_empty(&pstTCB->stCalleeNumQuery))
    {
        pstTCB->ucTaskStatus = SC_TASK_STOP;

        sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_TASK), "Task %d has finished. or there is no callees.", pstTCB->ulTaskID);

        return NULL;
    }

    while (1)
    {
        if (dos_list_is_empty(&pstTCB->stCalleeNumQuery))
        {
            break;
        }

        pstList = dos_list_fetch(&pstTCB->stCalleeNumQuery);
        if (!pstList)
        {
            continue;
        }

        pstCallee = dos_list_entry(pstList, SC_TEL_NUM_QUERY_NODE_ST, stLink);
        if (!pstCallee)
        {
            continue;
        }

        break;
    }

    pstCallee->stLink.next = NULL;
    pstCallee->stLink.prev = NULL;

    sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_TASK), "Select callee %s for new call.", pstCallee->szNumber);

    return pstCallee;
}


/*
 * ����: U32 sc_task_make_call(SC_TASK_CB_ST *pstTCB)
 * ����: ����ҵ����ƿ飬����������ӵ�������ģ�飬�ȴ�����
 * ����:
 *      SC_TASK_CB_ST *pstTCB: ������ƿ�
 * ����ֵ: �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
U32 sc_task_make_call(SC_TASK_CB *pstTCB)
{
    SC_TEL_NUM_QUERY_NODE_ST    *pstCallee                  = NULL;
    S8                          szCaller[SC_NUM_LENGTH]     = {0};
    SC_SRV_CB                   *pstSCB                     = NULL;
    SC_LEG_CB                   *pstLegCB                   = NULL;
    U32                         ulErrNo                     = CC_ERR_NO_REASON;
    BOOL                        bIsTrace                    = DOS_FALSE;

    if (DOS_ADDR_INVALID(pstTCB))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    bIsTrace = pstTCB->bTraceON;
    if (!bIsTrace)
    {
        bIsTrace = sc_customer_get_trace(pstTCB->ulCustomID);
    }

    pstCallee = sc_task_get_callee(pstTCB);
    if (!pstCallee)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* ֻҪȡ���˱��к��룬��Ӧ�ü�һ */
    pstTCB->ulCalledCount++;

    if (!bIsTrace)
    {
        bIsTrace = sc_trace_check_callee(pstCallee->szNumber);
    }

    /* ��������й��ʳ�; */
    if (pstCallee->szNumber[0] == '0'
        && pstCallee->szNumber[1] == '0')
    {
        /* ���ʱ�����к�����00��ͷ����ֹ���� */
        sc_log(bIsTrace, SC_LOG_SET_FLAG(LOG_LEVEL_WARNING, SC_MOD_EVENT, SC_LOG_DISIST), "callee is %s. Not alloc call", pstCallee->szNumber);
        ulErrNo = CC_ERR_SC_CALLEE_NUMBER_ILLEGAL;
        goto make_call_file;
    }

    /* �ж��Ƿ��ں������� */
    if (!sc_black_list_check(pstTCB->ulCustomID, pstCallee->szNumber))
    {
        sc_log(bIsTrace, SC_LOG_SET_FLAG(LOG_LEVEL_WARNING, SC_MOD_EVENT, SC_LOG_DISIST), "The destination is in black list. %s", pstCallee->szNumber);
        ulErrNo = CC_ERR_SC_CALLEE_NUMBER_ILLEGAL;
        goto make_call_file;
    }

    if (pstTCB->ucMode != SC_TASK_MODE_CALL_AGNET_FIRST)
    {
        /* �Ⱥ�����ϯ��ģʽ�����޻�����к��� */
        if (sc_get_number_by_callergrp(pstTCB->ulCallerGrpID, szCaller, SC_NUM_LENGTH) != DOS_SUCC)
        {
            sc_log(bIsTrace, SC_LOG_SET_FLAG(LOG_LEVEL_NOTIC, SC_MOD_TASK, SC_LOG_DISIST), "Get caller from caller group(%u) FAIL.", pstTCB->ulCallerGrpID);
            ulErrNo = CC_ERR_SC_CALLER_NUMBER_ILLEGAL;
            goto make_call_file;
        }

        if (!bIsTrace)
        {
            bIsTrace = sc_trace_check_caller(szCaller);
        }
    }

    /* ����һ��scb��leg */
    pstSCB = sc_scb_alloc();
    if (DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        goto make_call_file;
    }

    pstSCB->bTrace = bIsTrace;

    pstLegCB = sc_lcb_alloc();
    if (DOS_ADDR_INVALID(pstLegCB))
    {
        DOS_ASSERT(0);
        goto make_call_file;
    }

    if (pstTCB->ucMode == SC_TASK_MODE_CALL_AGNET_FIRST)
    {
        /* �Ⱥ��пͻ� */
        pstSCB->stAutoPreview.stSCBTag.bValid = DOS_TRUE;
        pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stAutoPreview.stSCBTag;
        pstSCB->stAutoPreview.ulCalleeLegNo = pstLegCB->ulCBNo;
        pstSCB->stAutoPreview.ulTaskID = pstTCB->ulTaskID;
        pstSCB->stAutoPreview.ulTcbID = pstTCB->usTCBNo;
        pstSCB->ulCustomerID = pstTCB->ulCustomID;
    }
    else
    {
        pstSCB->stAutoCall.stSCBTag.bValid = DOS_TRUE;
        pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stAutoCall.stSCBTag;
        pstSCB->stAutoCall.ulCallingLegNo = pstLegCB->ulCBNo;
        pstSCB->stAutoCall.ulTaskID = pstTCB->ulTaskID;
        pstSCB->stAutoCall.ulTcbID = pstTCB->usTCBNo;
        pstSCB->stAutoCall.ulKeyMode = pstTCB->ucMode;
        pstSCB->ulCustomerID = pstTCB->ulCustomID;

    }

    pstLegCB->stCall.bValid = DOS_TRUE;
    pstLegCB->ulSCBNo = pstSCB->ulSCBNo;

    dos_snprintf(pstLegCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstLegCB->stCall.stNumInfo.szOriginalCallee), pstCallee->szNumber);
    dos_snprintf(pstLegCB->stCall.stNumInfo.szOriginalCalling, sizeof(pstLegCB->stCall.stNumInfo.szOriginalCalling), szCaller);

    pstLegCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND;
    sc_scb_set_service(pstSCB, BS_SERV_AUTO_DIALING);

    /* ��֤ */
    if (pstTCB->ucMode == SC_TASK_MODE_CALL_AGNET_FIRST)
    {
        pstSCB->stAutoPreview.stSCBTag.usStatus = SC_AUTO_PREVIEW_AUTH;
    }
    else
    {
        pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_AUTH;
    }

    sc_log_digest_print_only(pstSCB, "Task(%u) callee : %s, caller : %s.", pstTCB->ulTaskID, pstCallee->szNumber, szCaller);

    if (sc_send_usr_auth2bs(pstSCB, pstLegCB) != DOS_SUCC)
    {
        goto make_call_file;
    }

    return DOS_SUCC;

make_call_file:
    if (DOS_ADDR_VALID(pstSCB))
    {
        sc_scb_free(pstSCB);
        pstSCB = NULL;
    }
    if (DOS_ADDR_VALID(pstLegCB))
    {
        sc_lcb_free(pstLegCB);
        pstLegCB = NULL;
    }
    sc_task_call_result_make_call_before(pstTCB->ulCustomID, pstTCB->ulTaskID, szCaller, pstCallee->szNumber, ulErrNo);
    return DOS_FAIL;
}

U32 sc_task_mngt_query_task(U32 ulTaskID, U32 ulCustomID)
{
    SC_DB_MSG_TASK_STATUS_ST    *pstDBTaskStatus    = NULL;
    SC_TASK_CB                  *pstTCB             = NULL;

    if (0 == ulTaskID || U32_BUTT == ulTaskID)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_DATA;
    }

    if (0 == ulCustomID || U32_BUTT == ulCustomID)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_USR;
    }

    pstTCB = sc_tcb_find_by_taskid(ulTaskID);
    if (!pstTCB)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_DATA;
    }

    pstDBTaskStatus = dos_dmem_alloc(sizeof(SC_DB_MSG_TASK_STATUS_ST));
    if (DOS_ADDR_INVALID(pstDBTaskStatus))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    dos_memzero(pstDBTaskStatus, sizeof(SC_DB_MSG_TASK_STATUS_ST));
    pstDBTaskStatus->ulTaskID = pstTCB->ulTaskID;
    /* ��ϯ���� */
    sc_agent_group_stat_by_id(pstTCB->ulAgentQueueID, &pstDBTaskStatus->ulTotalAgent, NULL, &pstDBTaskStatus->ulIdleAgent, NULL);
    /* �Ѿ����еĺ����� */
    pstDBTaskStatus->ulCalledCount = pstTCB->ulCalledCount;
    /* ��ǰ������ */
    pstDBTaskStatus->ulCurrentConcurrency = pstTCB->ulCurrentConcurrency;
    /* ��󲢷��� */
    pstDBTaskStatus->ulMaxConcurrency = pstTCB->ulMaxConcurrency;

    pstDBTaskStatus->stMsgTag.ulMsgType = SC_MSG_SACE_TASK_STATUS;

    sc_send_msg2db((SC_DB_MSG_TAG_ST *)pstDBTaskStatus);

    return SC_HTTP_ERRNO_SUCC;
}

/*
 * ����: VOID *sc_task_runtime(VOID *ptr)
 * ����: ��������������߳�������
 * ����:
 */
VOID *sc_task_runtime(VOID *ptr)
{
    SC_TEL_NUM_QUERY_NODE_ST *pstCallee = NULL;
    SC_TASK_CB      *pstTCB        = NULL;
    list_t          *pstList       = NULL;
    U32             ulTaskInterval = 0;
    S32             lResult        = 0;
    U32             ulMinInterval  = 0;
    U32             blStopFlag     = DOS_FALSE;
    BOOL            blPauseFlag    = DOS_FALSE;

    if (!ptr)
    {
        sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_TASK), "%s", "Fail to start the thread for task, invalid parameter");
        pthread_exit(0);
    }

    pstTCB = (SC_TASK_CB *)ptr;
    if (DOS_ADDR_INVALID(pstTCB))
    {
        sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_TASK), "%s", "Start task without pointer a TCB.");
        return NULL;
    }

    pstTCB->ucTaskStatus = SC_TASK_WORKING;

    if (sc_serv_ctrl_check(pstTCB->ulCustomID
                                , BS_SERV_AUTO_DIALING
                                , SC_SRV_CTRL_ATTR_TASK_MODE
                                , pstTCB->ucMode
                                , SC_SRV_CTRL_ATTR_INVLID
                                , U32_BUTT))
    {
        sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_TASK), "Service not allow.(TaskID:%u) ", pstTCB->ulTaskID);

        goto finished;
    }

    /* ����һ����ʱ�������Ѿ����й��ĺ�����������ʱд�����ݿ��� */
    lResult = dos_tmr_start(&pstTCB->pstTmrHandle
                            , SC_TASK_UPDATE_DB_TIMER * 1000
                            , sc_task_update_calledcnt
                            , (U64)pstTCB
                            , TIMER_NORMAL_LOOP);
    if (lResult < 0)
    {
        sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_TASK), "Start timer update task(%u) calledcnt FAIL", pstTCB->ulTaskID);
    }

    /*
       ����ط���ŵ���˼��: �������ʱ��Ϊ5000ms����Ҫ����5000ms�ڲ�������ϯ��Ҫ�ĺ���ȫ����������
       �����Ҫ����һ��������е�ʱ����
       ���Ǻ��м����С��20ms
       ����1000ms���һ��
       ����ǲ���Ҫ��ϯ�ĺ��У�����20CPS�����ȷ���(��ȻҪ������Ͳ�����)
    */
    if (pstTCB->usSiteCount * pstTCB->ulCallRate)
    {
        ulMinInterval = 5000 / ceil(1.0 * (pstTCB->usSiteCount * pstTCB->ulCallRate) / 10);
        ulMinInterval = (ulMinInterval < 20) ? 20 : (ulMinInterval > 1000) ? 1000 : ulMinInterval;
    }
    else
    {
        ulMinInterval = 1000 / 20;
    }

    sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_TASK), "Start run task(%u), Min interval: %ums", pstTCB->ulTaskID, ulMinInterval);

    while (1)
    {
        if (0 == ulTaskInterval)
        {
            ulTaskInterval = ulMinInterval;
        }
        dos_task_delay(ulTaskInterval);
        ulTaskInterval = 0;

        if (!pstTCB->ucValid)
        {
            return NULL;
        }

        /* ���ݵ�ǰ��������ȷ��������еļ���������ǰ�����Ѿ���������״̬����Ҫǿ�Ƶ������ */
        if (!sc_task_check_can_call(pstTCB))
        {
            /* ���ܻ�ǳ��죬�Ͳ�Ҫ��ӡ�� */
            /*sc_logr_debug(NULL, SC_TASK, "Cannot make call for reach the max concurrency. Task : %u.", pstTCB->ulTaskID);*/
            continue;
        }

        /* �����ͣ�˾ͼ����ȴ� */
        if (SC_TASK_PAUSED == pstTCB->ucTaskStatus)
        {
            /* ��һ�� ��ͣ ʱ�����������񣬵ȴ�20s */
            if (pstTCB->ulCurrentConcurrency != 0 || !blPauseFlag)
            {
                blPauseFlag = DOS_TRUE;
                sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_TASK), "Cannot make call for paused status. Task : %u.", pstTCB->ulTaskID);
                ulTaskInterval = 20000;
                continue;
            }

            sc_task_mngt_query_task(pstTCB->ulTaskID, pstTCB->ulCustomID);
            break;
        }
        blPauseFlag = DOS_FALSE;

        /* �����ֹͣ�ˣ��ͼ�⻹��û�к��У�����к��У��͵ȴ����ȴ�û�к���ʱ�˳����� */
        if (SC_TASK_STOP == pstTCB->ucTaskStatus)
        {
            /* ��һ�� SC_TASK_STOP ʱ�����������񣬵ȴ�20s */
            if (pstTCB->ulCurrentConcurrency != 0 || !blStopFlag)
            {
                blStopFlag = DOS_TRUE;
                sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_TASK), "Cannot make call for stoped status. Task : %u, CurrentConcurrency : %u.", pstTCB->ulTaskID, pstTCB->ulCurrentConcurrency);
                ulTaskInterval = 20000;
                continue;
            }

            /* ��������ˣ��˳���ѭ�� */
            break;
        }
        blStopFlag = DOS_FALSE;

        /* ��鵱ǰ�Ƿ��������ʱ��� */
        if (sc_task_check_can_call_by_time(pstTCB) != DOS_TRUE)
        {
            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_TASK), "Cannot make call for invalid time period. Task : %u. %d", pstTCB->ulTaskID, pstTCB->usTCBNo);
            ulTaskInterval = 20000;
            continue;
        }

        /* ��⵱ʱ�����Ƿ���Է������ */
        if (sc_task_check_can_call_by_status(pstTCB) != DOS_TRUE)
        {
            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_TASK), "Cannot make call for system busy. Task : %u.", pstTCB->ulTaskID);
            continue;
        }
#if 1
        /* ������� */
        if (sc_task_make_call(pstTCB))
        {
            /* ���ͺ���ʧ�� */
            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_TASK), "%s", "Make call fail.");

        }
#endif
    }

finished:
    sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_TASK), "Task %d finished.", pstTCB->ulTaskID);

    /* �ͷ������Դ */
    if (DOS_ADDR_VALID(pstTCB->pstTmrHandle))
    {
        dos_tmr_stop(&pstTCB->pstTmrHandle);
        pstTCB->pstTmrHandle = NULL;
    }

    while (1)
    {
        if (dos_list_is_empty(&pstTCB->stCalleeNumQuery))
        {
            break;
        }

        pstList = dos_list_fetch(&pstTCB->stCalleeNumQuery);
        if (DOS_ADDR_INVALID(pstList))
        {
            break;
        }

        pstCallee = dos_list_entry(pstList, SC_TEL_NUM_QUERY_NODE_ST, stLink);
        if (DOS_ADDR_INVALID(pstCallee))
        {
            continue;
        }

        dos_dmem_free(pstCallee);
        pstCallee = NULL;
    }

#if 0
    if (pstTCB->pstCallerNumQuery)
    {
        dos_dmem_free(pstTCB->pstCallerNumQuery);
        pstTCB->pstCallerNumQuery = NULL;
    }
#endif

    pthread_mutex_destroy(&pstTCB->mutexTaskList);

    /* Ⱥ����������󣬽����еı��к�����������Ϊ���к���������� */
    pstTCB->bThreadRunning = DOS_FALSE;
    sc_task_update_calledcnt((U64)pstTCB);
    sc_task_save_status(pstTCB->ulTaskID, blStopFlag ? SC_TASK_STATUS_DB_STOP : SC_TASK_STATUS_DB_PAUSED, NULL);

    sc_tcb_free(pstTCB);
    pstTCB = NULL;

    return NULL;
}

/*
 * ����: U32 sc_task_start(SC_TASK_CB_ST *pstTCB)
 * ����: �������л�����
 * ����:
 *      SC_TASK_CB_ST *pstTCB: ������ƿ�
 * �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 */
U32 sc_task_start(SC_TASK_CB *pstTCB)
{
    if (!pstTCB
        || !pstTCB->ucValid)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if (pstTCB->bThreadRunning)
    {
        sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_TASK), "Task %u already running.", pstTCB->ulTaskID);
    }
    else
    {
        if (pthread_create(&pstTCB->pthID, NULL, sc_task_runtime, pstTCB) < 0)
        {
            DOS_ASSERT(0);

            pstTCB->bThreadRunning = DOS_FALSE;

            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_TASK), "Start task %d faild", pstTCB->ulTaskID);

            return DOS_FAIL;
        }

        pstTCB->bThreadRunning = DOS_TRUE;
    }

    sc_task_save_status(pstTCB->ulTaskID, SC_TASK_STATUS_DB_START, NULL);

    sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_TASK), "Start task %d finished.", pstTCB->ulTaskID);

    if (g_stSysStat.ulCallTaskNum < U32_BUTT)
    {
        g_stSysStat.ulCallTaskNum++;
    }
    else
    {
        DOS_ASSERT(0);
    }

    return DOS_SUCC;
}

/*
 * ����: U32 sc_task_stop(SC_TASK_CB_ST *pstTCB)
 * ����: ֹͣ���л�����
 * ����:
 *      SC_TASK_CB_ST *pstTCB: ������ƿ�
 * �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 */
U32 sc_task_stop(SC_TASK_CB *pstTCB)
{
    if (!pstTCB)
    {
        DOS_ASSERT(0);


        return DOS_FAIL;
    }

    if (!pstTCB->ucValid)
    {
        DOS_ASSERT(0);

        sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_TASK), "Cannot stop the task. TCB Valid:%d, TCB Status: %d", pstTCB->ucValid, pstTCB->ucTaskStatus);

        return DOS_FAIL;
    }

    sc_task_save_status(pstTCB->ulTaskID, SC_TASK_STATUS_DB_STOP, NULL);

    pthread_mutex_lock(&pstTCB->mutexTaskList);
    pstTCB->ucTaskStatus = SC_TASK_STOP;
    pthread_mutex_unlock(&pstTCB->mutexTaskList);
    
    g_stSysStat.ulCallTaskNum--;
    return DOS_SUCC;
}

/*
 * ����: U32 sc_task_continue(SC_TASK_CB_ST *pstTCB)
 * ����: �ָ����л�����
 * ����:
 *      SC_TASK_CB_ST *pstTCB: ������ƿ�
 * �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 */
U32 sc_task_continue(SC_TASK_CB *pstTCB)
{
    if (!pstTCB)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if (!pstTCB->ucValid
        || (pstTCB->ucTaskStatus != SC_TASK_PAUSED && pstTCB->ucTaskStatus != SC_TASK_STOP))
    {
        sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_TASK), "Cannot continue the task. TCB Valid:%d, TCB Status: %d", pstTCB->ucValid, pstTCB->ucTaskStatus);

        return DOS_FAIL;
    }

    pthread_mutex_lock(&pstTCB->mutexTaskList);
    pstTCB->ucTaskStatus = SC_TASK_WORKING;
    pthread_mutex_unlock(&pstTCB->mutexTaskList);

    /* ��ʼ���� */
    return sc_task_start(pstTCB);
}

/*
 * ����: U32 sc_task_pause(SC_TASK_CB_ST *pstTCB)
 * ����: ��ͣ���л�����
 * ����:
 *      SC_TASK_CB_ST *pstTCB: ������ƿ�
 * �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 */
U32 sc_task_pause(SC_TASK_CB *pstTCB)
{
    if (!pstTCB)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if (!pstTCB->ucValid
        || pstTCB->ucTaskStatus != SC_TASK_WORKING)
    {
        sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_TASK), "Cannot stop the task. TCB Valid:%d, TCB Status: %d", pstTCB->ucValid, pstTCB->ucTaskStatus);

        return DOS_FAIL;
    }

    sc_task_save_status(pstTCB->ulTaskID, SC_TASK_STATUS_DB_PAUSED, NULL);

    pthread_mutex_lock(&pstTCB->mutexTaskList);
    pstTCB->ucTaskStatus = SC_TASK_PAUSED;
    pthread_mutex_unlock(&pstTCB->mutexTaskList);

    g_stSysStat.ulCallTaskNum--;

    return DOS_SUCC;
}

U32 sc_task_concurrency_add(U32 ulTCBNo)
{
    if (ulTCBNo >= SC_MAX_TASK_NUM)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if (!g_pstTaskList[ulTCBNo].ucValid)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pthread_mutex_lock(&g_pstTaskList[ulTCBNo].mutexTaskList);
    g_pstTaskList[ulTCBNo].ulCurrentConcurrency++;
    if (g_pstTaskList[ulTCBNo].ulCurrentConcurrency > g_pstTaskList[ulTCBNo].ulMaxConcurrency)
    {
        DOS_ASSERT(0);
    }
    pthread_mutex_unlock(&g_pstTaskList[ulTCBNo].mutexTaskList);

    return DOS_SUCC;
}

U32 sc_task_concurrency_minus(U32 ulTCBNo)
{
    if (ulTCBNo >= SC_MAX_TASK_NUM)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    /* �����������󣬻���һͨ�绰����ͨ���У�
        ����ͬ�绰�Ҷ�ʱ���������������������
        �� ulCurrentConcurrency ��Ϊ1�� �����������ԶҲ�����˳���
    if (!g_pstTaskMngtInfo->pstTaskList[ulTCBNo].ucValid)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }
    */

    pthread_mutex_lock(&g_pstTaskList[ulTCBNo].mutexTaskList);
    if (g_pstTaskList[ulTCBNo].ulCurrentConcurrency > 0)
    {
        g_pstTaskList[ulTCBNo].ulCurrentConcurrency--;
    }
    else
    {
        DOS_ASSERT(0);
    }
    pthread_mutex_unlock(&g_pstTaskList[ulTCBNo].mutexTaskList);

    return DOS_SUCC;
}

/*
 * ����: U32 sc_task_mngt_continue_task(U32 ulTaskID, U32 ulCustomID)
 * ����: ����һ���Ѿ���ͣ������
 * ����:
 *      U32 ulTaskID   : ����ID
 *      U32 ulCustomID : ���������ͻ�ID
 * ����ֵ
 *      ����HTTP API������
 * �ú�����������
 **/
U32 sc_task_mngt_continue_task(U32 ulTaskID, U32 ulCustomID)
{
    SC_TASK_CB *pstTCB = NULL;
    U32 ulRet = DOS_FAIL;

    if (0 == ulTaskID || U32_BUTT == ulTaskID)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_DATA;
    }

    if (0 == ulCustomID)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_USR;
    }

    pstTCB = sc_tcb_find_by_taskid(ulTaskID);
    if (!pstTCB)
    {
        /* ��ͣ������һ�������ˣ�����Ҳ���Ӧ�����¼��� */
        if (sc_task_load(ulTaskID) != DOS_SUCC)
        {
            DOS_ASSERT(0);
            return SC_HTTP_ERRNO_INVALID_DATA;
        }

        pstTCB = sc_tcb_find_by_taskid(ulTaskID);
        if (!pstTCB)
        {
            DOS_ASSERT(0);
            return SC_HTTP_ERRNO_INVALID_DATA;
        }

        ulRet = sc_task_load_callee(pstTCB);
        if (DOS_SUCC != ulRet)
        {
            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_TASK), "SC Task Load Callee FAIL.(TaskID:%u, usNo:%u)", ulTaskID, pstTCB->usTCBNo);
            return SC_HTTP_ERRNO_INVALID_DATA;
        }
        sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_TASK), "SC Task Load callee SUCC.(TaskID:%u, usNo:%u)", ulTaskID, pstTCB->usTCBNo);
    }

    if (pstTCB->ucTaskStatus == SC_TASK_INIT)
    {
        /* ��Ҫ�������� */
        if (DOS_SUCC != sc_task_load(ulTaskID))
        {
            DOS_ASSERT(0);

            return SC_HTTP_ERRNO_CMD_EXEC_FAIL;
        }
    }

    /* ��ȡ����������󲢷��� */
    pstTCB->usSiteCount = sc_agent_group_agent_count(pstTCB->ulAgentQueueID);
    pstTCB->ulMaxConcurrency = ceil(1.0 * (pstTCB->usSiteCount * pstTCB->ulCallRate) / 10);
    if (0 == pstTCB->ulMaxConcurrency)
    {
        pstTCB->ulMaxConcurrency = SC_MAX_TASK_MAX_CONCURRENCY;
    }

    if (pstTCB->ucTaskStatus != SC_TASK_PAUSED
        && pstTCB->ucTaskStatus != SC_TASK_STOP)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_TASK_STATUS;
    }

    sc_task_continue(pstTCB);

    return SC_HTTP_ERRNO_SUCC;
}


/*
 * ����: U32 sc_task_mngt_start_task(U32 ulTaskID, U32 ulCustomID)
 * ����: ��ͣһ�������е�����
 * ����:
 *      U32 ulTaskID   : ����ID
 *      U32 ulCustomID : ���������ͻ�ID
 * ����ֵ
 *      ����HTTP API������
 * �ú�����������
 **/
U32 sc_task_mngt_pause_task(U32 ulTaskID, U32 ulCustomID)
{
    SC_TASK_CB *pstTCB = NULL;

    if (0 == ulTaskID || U32_BUTT == ulTaskID)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_DATA;
    }

    if (0 == ulCustomID)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_USR;
    }

    pstTCB = sc_tcb_find_by_taskid(ulTaskID);
    if (!pstTCB)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_DATA;
    }

    if (pstTCB->ucTaskStatus != SC_TASK_WORKING)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_TASK_STATUS;
    }

    sc_task_pause(pstTCB);

    return SC_HTTP_ERRNO_SUCC;

}

U32 sc_task_mngt_delete_task(U32 ulTaskID, U32 ulCustomID)
{
    SC_TASK_CB *pstTCB = NULL;

    if (0 == ulTaskID || U32_BUTT == ulTaskID)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_DATA;
    }

    if (0 == ulCustomID || U32_BUTT == ulCustomID)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_USR;
    }

    pstTCB = sc_tcb_find_by_taskid(ulTaskID);
    if (!pstTCB)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_DATA;
    }

    sc_task_stop(pstTCB);
    return SC_HTTP_ERRNO_SUCC;

}


/*
 * ����: U32 sc_task_mngt_start_task(U32 ulTaskID, U32 ulCustomID)
 * ����: ����һ���µ�����
 * ����:
 *      U32 ulTaskID   : ����ID
 *      U32 ulCustomID : ���������ͻ�ID
 * ����ֵ
 *      ����HTTP API������
 * �ú�����������
 **/
U32 sc_task_mngt_start_task(U32 ulTaskID, U32 ulCustomID)
{
    SC_TASK_CB *pstTCB = NULL;

    if (0 == ulTaskID || U32_BUTT == ulTaskID)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_DATA;
    }

    if (0 == ulCustomID || U32_BUTT == ulCustomID)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_USR;
    }

    if (DOS_SUCC != sc_task_load(ulTaskID))
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_CMD_EXEC_FAIL;
    }

    pstTCB = sc_tcb_find_by_taskid(ulTaskID);
    if (DOS_ADDR_INVALID(pstTCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* ��ȡ����������󲢷��� */
    pstTCB->usSiteCount = sc_agent_group_agent_count(pstTCB->ulAgentQueueID);
    pstTCB->ulMaxConcurrency = ceil(1.0 * (pstTCB->usSiteCount * pstTCB->ulCallRate) / 10);
    if (0 == pstTCB->ulMaxConcurrency)
    {
        pstTCB->ulMaxConcurrency = SC_MAX_TASK_MAX_CONCURRENCY;
    }

    if (sc_task_start(pstTCB) != DOS_SUCC)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_CMD_EXEC_FAIL;
    }

    return SC_HTTP_ERRNO_SUCC;
}


/*
 * ����: U32 sc_task_mngt_stop_task(U32 ulTaskID, U32 ulCustomID)
 * ����: ֹͣһ�������е�����
 * ����:
 *      U32 ulTaskID   : ����ID
 *      U32 ulCustomID : ���������ͻ�ID
 * ����ֵ
 *      ����HTTP API������
 * �ú�����������
 **/
U32 sc_task_mngt_stop_task(U32 ulTaskID, U32 ulCustomID)
{
    SC_TASK_CB *pstTCB = NULL;

    if (0 == ulTaskID || U32_BUTT == ulTaskID)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_DATA;
    }

    if (0 == ulCustomID || U32_BUTT == ulCustomID)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_USR;
    }

    pstTCB = sc_tcb_find_by_taskid(ulTaskID);
    if (!pstTCB)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_DATA;
    }

    if (pstTCB->ucTaskStatus == SC_TASK_STOP)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_TASK_STATUS;
    }

    sc_task_stop(pstTCB);

    return SC_HTTP_ERRNO_SUCC;
}


/*
 * ���� : sc_task_write_stat
 * ���� : ������������״̬
 * ���� :
 * ����ֵ: �ɹ�����DOS_SUCC,���򷵻�DOS_FAIL
 */
U32 sc_task_write_stat(U32 ulType, VOID *ptr)
{
    U32 i;

    for (i=0; i<SC_MAX_TASK_NUM; i++)
    {
        if (g_pstTaskList[i].ucValid)
        {
            sc_task_mngt_query_task(g_pstTaskList[i].ulTaskID, g_pstTaskList[i].ulCustomID);
        }
    }

    return DOS_SUCC;
}


/*
 * ����: VOID sc_task_mngt_cmd_process(SC_TASK_CTRL_CMD_ST *pstCMD)
 * ����: ����������ƣ����п���API
 * ����:
 *      SC_TASK_CTRL_CMD_ST *pstCMD: API��������
 * ����ֵ
 * �ú�����������
 **/
U32 sc_task_mngt_cmd_proc(U32 ulAction, U32 ulCustomerID, U32 ulTaskID)
{
    U32 ulRet = SC_HTTP_ERRNO_INVALID_REQUEST;

    sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_TASK), "Process CMD, Action:%u, Task: %u, CustomID: %u"
                    , ulAction, ulTaskID, ulCustomerID);

    switch (ulAction)
    {
        case SC_API_CMD_ACTION_ADD:
        {
            /* DO Nothing */
            ulRet = SC_HTTP_ERRNO_SUCC;
            break;
        }
        case SC_API_CMD_ACTION_UPDATE:
        {
            /* �������û�б����ص��ڴ棬�Ͳ�Ҫ������ */
            if (sc_tcb_find_by_taskid(ulTaskID))
            {
                //if (sc_task_and_callee_load(ulTaskID) != DOS_SUCC)
                //{
                //    ulRet = SC_HTTP_ERRNO_INVALID_DATA;
                //}

                if (sc_task_load(ulTaskID) != DOS_SUCC)
                {
                    ulRet = SC_HTTP_ERRNO_INVALID_DATA;
                }

                ulRet = SC_HTTP_ERRNO_SUCC;
            }
            else
            {
                ulRet = SC_HTTP_ERRNO_SUCC;
            }
            break;
        }
        case SC_API_CMD_ACTION_START:
        {
            ulRet = sc_task_mngt_start_task(ulTaskID, ulCustomerID);
            break;
        }
        case SC_API_CMD_ACTION_STOP:
        {
            ulRet = sc_task_mngt_stop_task(ulTaskID, ulCustomerID);
            break;
        }
        case SC_API_CMD_ACTION_CONTINUE:
        {
            ulRet = sc_task_mngt_continue_task(ulTaskID, ulCustomerID);
            break;
        }
        case SC_API_CMD_ACTION_PAUSE:
        {
            ulRet = sc_task_mngt_pause_task(ulTaskID, ulCustomerID);
            break;
        }
        case SC_API_CMD_ACTION_DELETE:
        {
            ulRet = sc_task_mngt_delete_task(ulTaskID, ulCustomerID);
            break;
        }
        case SC_API_CMD_ACTION_STATUS:
        {
            ulRet = sc_task_mngt_query_task(ulTaskID, ulCustomerID);
            break;
        }
        default:
        {
            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_TASK), "Action templately not support. ACTION: %d", ulAction, ulAction);
            break;
        }
    }

    sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_TASK), "CMD Process finished. Action: %u, ErrCode:%u"
                    , ulAction, ulRet);

    return ulRet;
}


/*
 * ����: U32 sc_task_mngt_start()
 * ����: �������п���ģ�飬ͬʱ�����Ѿ������صĺ�������
 * ����:
 * ����ֵ: �ɹ�����DOS_SUCC�� ʧ�ܷ���DOS_FAIL
 **/
U32 sc_task_mngt_start()
{
    SC_TASK_CB    *pstTCB = NULL;
    U32              ulIndex;

    for (ulIndex = 0; ulIndex < SC_MAX_TASK_NUM; ulIndex++)
    {
        pstTCB = &g_pstTaskList[ulIndex];

        if (pstTCB->ucValid && SC_TCB_HAS_VALID_OWNER(pstTCB))
        {
            if (DOS_SUCC != sc_task_and_callee_load(pstTCB->ulTaskID))
            {
                sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_TASK), "Task init fail. Custom ID: %d, Task ID: %d", pstTCB->ulCustomID, pstTCB->ulTaskID);
                sc_tcb_free(pstTCB);
                continue;
            }
            
            if (pstTCB->ucMode != SC_TASK_MODE_AUDIO_ONLY)
            {
                /* ��ȡ����������󲢷��� */
                pstTCB->usSiteCount = sc_agent_group_agent_count(pstTCB->ulAgentQueueID);
                pstTCB->ulMaxConcurrency = ceil(1.0 * (pstTCB->usSiteCount * pstTCB->ulCallRate) / 10);
            }
            
            if (0 == pstTCB->ulMaxConcurrency)
            {
                pstTCB->ulMaxConcurrency = SC_MAX_TASK_MAX_CONCURRENCY;
            }

            if (pstTCB->ucTaskStatus != SC_TASK_WORKING)
            {
                continue;
            }

            if (sc_task_start(pstTCB) != DOS_SUCC)
            {
                sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_TASK), "Task start fail. Custom ID: %d, Task ID: %d", pstTCB->ulCustomID, pstTCB->ulTaskID);

                sc_tcb_free(pstTCB);
                continue;
            }
        }
    }

    sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_TASK), "Start call task mngt service finished.");

    return DOS_SUCC;
}


/*
 * ����: U32 sc_task_mngt_init()
 * ����: ��ʼ�����й���ģ��
 * ����:
 * ����ֵ: �ɹ�����DOS_SUCC�� ʧ�ܷ���DOS_FAIL
 **/
U32 sc_task_mngt_init()
{
    SC_TASK_CB      *pstTCB = NULL;
    U32             ulIndex = 0;

    /* ��ʼ�����п��ƿ��������ƿ� */
    g_pstTaskList = (SC_TASK_CB *)dos_smem_alloc(sizeof(SC_TASK_CB) * SC_MAX_TASK_NUM);
    if (!g_pstTaskList)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }
    dos_memzero(g_pstTaskList, sizeof(SC_TASK_CB) * SC_MAX_TASK_NUM);
    for (ulIndex = 0; ulIndex < SC_MAX_TASK_NUM; ulIndex++)
    {
        pstTCB = &g_pstTaskList[ulIndex];
        pstTCB->usTCBNo = ulIndex;
        pthread_mutex_init(&pstTCB->mutexTaskList, NULL);
        pstTCB->ucTaskStatus = SC_TASK_BUTT;
        pstTCB->ulTaskID = U32_BUTT;
        pstTCB->ulCustomID = U32_BUTT;

        dos_list_init(&pstTCB->stCalleeNumQuery);
    }

    /* ����Ⱥ������ */
    if (sc_task_mngt_load_task() != DOS_SUCC)
    {
        sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_TASK), "Load call task fail.");
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/*
 * ����: U32 sc_task_mngt_shutdown()
 * ����: ֹͣ�������ģ��
 * ����:
 * ����ֵ: �ɹ�����DOS_SUCC�� ʧ�ܷ���DOS_FAIL
 **/
U32 sc_task_mngt_stop()
{
    return DOS_SUCC;
}


#ifdef __cplusplus
}
#endif /* __cplusplus */


