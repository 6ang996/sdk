/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  �ļ���: sc_event_process.c
 *
 *  ����ʱ��: 2015��1��5��16:18:41
 *  ��    ��: Larry
 *  ��    ��: ����FS���ķ������ĸ����¼�
 *  �޸���ʷ:
 */
#include <switch.h>
#include <dos.h>
#include <esl.h>
#include <sys/time.h>
#include <pthread.h>
#include "sc_pub.h"
#include "sc_task_pub.h"
#include "sc_debug.h"
#include "sc_event_process.h"

typedef struct tagSCEventProcessHandle
{
    esl_handle_t        stHandle;                /*  esl ��� */
    pthread_t           pthID;

    BOOL                blIsESLRunning;          /* ESL�Ƿ��������� */
    BOOL                blIsWaitingExit;         /* �����Ƿ����ڵȴ��˳� */
}SC_EP_HANDLE_ST;

extern U32   g_ulTaskTraceAll;


SC_EP_HANDLE_ST        *g_pstHandle = NULL;
U32 g_ulESLDebugLevel = ESL_LOG_LEVEL_NOTICE;


extern SC_CCB_ST *sc_ccb_hash_tables_find(S8 *pszUUID);

U32 sc_ep_call_verify(S8 *pszUUID)
{
    SC_CCB_ST *pstCCB = NULL;

    SC_TRACE_IN(pszUUID, 0, 0, 0);

    if (DOS_ADDR_INVALID(pszUUID))
    {
        DOS_ASSERT(0);

        SC_CCB_SET_STATUS(pstCCB, SC_CCB_REPORT);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pstCCB = sc_ccb_hash_tables_find(pszUUID);
    if (!SC_CCB_IS_VALID(pstCCB))
    {
        DOS_ASSERT(0);

        SC_CCB_SET_STATUS(pstCCB, SC_CCB_REPORT);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    SC_CCB_SET_STATUS(pstCCB, SC_CCB_AUTH);

    /* @TODO ��֤ */

    SC_CCB_SET_STATUS(pstCCB, SC_CCB_EXEC);

    return DOS_SUCC;
}

U32 sc_get_trunk_id(S8 *pszIPAddr, S8 *pszGatewayName)
{
    SC_TRACE_IN(pszIPAddr, pszGatewayName, 0, 0);

    /* ע������ط�Ӧ��ʹ�� && ���ţ�ֻҪ��һ���Ϸ��ͺ� */
    if (DOS_ADDR_INVALID(pszGatewayName) && DOS_ADDR_INVALID(pszIPAddr))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return U32_BUTT;
    }

    /* @TODO: ȥ���ݿ��ѯ */

    SC_TRACE_OUT();
    return U32_BUTT;
}

U32 sc_get_custom_id(S8 *pszNumber, U32 ulNumType)
{
    SC_TRACE_IN(pszNumber, ulNumType, 0, 0);

    if (DOS_ADDR_INVALID(pszNumber) || ulNumType >= SC_NUM_TYPE_BUTT)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return U32_BUTT;
    }

    SC_TRACE_OUT();
    return U32_BUTT;
}

U32 sc_ep_parse_event(esl_event_t *pstEvent, SC_CCB_ST *pstCCB)
{
    S8         *pszUUID      = NULL;
    S8         *pszCaller    = NULL;
    S8         *pszCallee    = NULL;
    S8         *pszANI       = NULL;
    S8         *pszType      = NULL;
    S8         *pszCallSrc   = NULL;
    S8         *pszTrunkIP   = NULL;
    S8         *pszGwName    = NULL;
    S8         *pszCallDirection = NULL;
    S8         *pszUserIdentity  = NULL;
    U32        ulUserIdentity    = U32_BUTT;

    SC_TRACE_IN(pstEvent, pstCCB, 0, 0);

    if (DOS_ADDR_INVALID(pstEvent) || DOS_ADDR_INVALID(pstCCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_SUCC;
    }

    /* ��ESL EVENT�л�ȥ��غ�����Ϣ */
    /*
     * 1. PSTN����
     *   ����:
     *       Call Direction: Inbound;
     *       Profile Name:   external
     *   ��ȡ����Ϣ:
     *       �Զ�IP����gateway name��ȡ���е�����ID
     *       ��������Ϣ, ��������Ϣ
     * 2. ������PSTN
     *   ����:
     *       Call Direction: outbount;
     *       Profile Name:   external;
     *   ��ȡ����Ϣ:
     *       �Զ�IP����gateway name��ȡ���е�����ID
     *       ��������Ϣ, ��������Ϣ
     *       ��ȡ�û���Ϣ��ʾ
     * 3. �ڲ�����
     *   ����:
     *       Call Direction: Inbound;
     *       Profile Name:   internal;
     *   ��ȡ����Ϣ:
     *       ��������Ϣ, ��������Ϣ
     *       ��ȡ�û���Ϣ��ʾ
     */
    pszUUID = esl_event_get_header(pstEvent, "Caller-Unique-ID");
    if (DOS_ADDR_INVALID(pszUUID))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pszCallSrc = esl_event_get_header(pstEvent, "variable_sofia_profile_name");
    if (DOS_ADDR_INVALID(pszCallSrc))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pszCallDirection = esl_event_get_header(pstEvent, "Call-Direction");
    if (DOS_ADDR_INVALID(pszCallDirection))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pszGwName = esl_event_get_header(pstEvent, "variable_sip_gateway_name");
    pszTrunkIP = esl_event_get_header(pstEvent, "Caller-Network-Addr");
    pszCaller = esl_event_get_header(pstEvent, "Caller-Caller-ID-Number");
    pszCallee = esl_event_get_header(pstEvent, "Caller-Destination-Number");
    pszANI    = esl_event_get_header(pstEvent, "Caller-ANI");
    if (DOS_ADDR_INVALID(pszCaller) || DOS_ADDR_INVALID(pszCallee) || DOS_ADDR_INVALID(pszANI))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    /* ���������д��CCB�� */
    pthread_mutex_lock(&pstCCB->mutexCCBLock);

    if (0 == dos_strnicmp(pszCallDirection, "outbound", dos_strlen("outbound")))
    {
        pstCCB->aucServiceType[0] = SC_SERV_OUTBOUND_CALL;
    }
    else
    {
        pstCCB->aucServiceType[0] = SC_SERV_INBOUND_CALL;
    }

    if (0 == dos_strnicmp(pszCallSrc, "external", dos_strlen("external")))
    {
        pstCCB->aucServiceType[1] = SC_SERV_EXTERNAL_CALL;
        pstCCB->ulTrunkID = sc_get_trunk_id(pszTrunkIP, pszGwName);
        if (U32_BUTT == pstCCB->ulTrunkID)
        {
            DOS_ASSERT(0);
        }
    }
    else
    {
        pstCCB->aucServiceType[1] = SC_SERV_INTERNAL_CALL;
        pstCCB->ulTrunkID = U32_BUTT;
    }

    /* ����Ǻ�������ͨ����������������˻���Ϣ */
    if (SC_SERV_OUTBOUND_CALL == pstCCB->aucServiceType[0])
    {
        pszUserIdentity = esl_event_get_header(pstEvent, "variable_user_identity");
        if (dos_atoul(pszUserIdentity, &ulUserIdentity) < 0)
        {
            DOS_ASSERT(0);
            pthread_mutex_unlock(&pstCCB->mutexCCBLock);
            SC_TRACE_OUT();
            return DOS_FAIL;
        }

        pstCCB->ulCustomID = ulUserIdentity;
    }
    else
    {
        /*
         * ������ⲿ����ĺ�����Ҫ���ݱ��к���(�ͻ�����)�����ҵ�ǰ���ں����Ǹ��ͻ�
         * �����SIP�ֻ�����������Ҫͨ�����к���(SIP�ֻ�)���ҵ�ǰ���Ǹ��ͻ�
         */
        if (SC_SERV_EXTERNAL_CALL == pstCCB->aucServiceType[1])
        {
            pstCCB->ulCustomID = sc_get_custom_id(pszCallee, SC_NUM_TYPE_OUTLINE);
        }
        else
        {
            pstCCB->ulCustomID = sc_get_custom_id(pszCaller, SC_NUM_TYPE_USERID);
        }
    }

    dos_strncpy(pstCCB->szCalleeNum, pszCallee, sizeof(pstCCB->szCalleeNum));
    pstCCB->szCalleeNum[sizeof(pstCCB->szCalleeNum) -1] = '\0';
    dos_strncpy(pstCCB->szCallerNum, pszCaller, sizeof(pstCCB->szCallerNum));
    pstCCB->szCallerNum[sizeof(pstCCB->szCallerNum) -1] = '\0';
    dos_strncpy(pstCCB->szANINum, pszANI, sizeof(pstCCB->szANINum));
    pstCCB->szANINum[sizeof(pstCCB->szANINum) -1] = '\0';
    dos_strncpy(pstCCB->szUUID, pszUUID, sizeof(pstCCB->szUUID));
    pstCCB->szUUID[sizeof(pstCCB->szUUID) -1] = '\0';
    pthread_mutex_unlock(&pstCCB->mutexCCBLock);

    SC_TRACE_OUT();
    return DOS_SUCC;
}

U32 sc_ep_channel_create_proc(esl_event_t *pstEvent, esl_handle_t *pstHandle, SC_CCB_ST *pstCCB)
{
    U32         ulStatus;
    U32         ulRet = DOS_SUCC;

    SC_TRACE_IN(pstEvent, pstHandle, pstCCB, 0);

    if (DOS_ADDR_INVALID(pstEvent)
        || DOS_ADDR_INVALID(pstHandle)
        || DOS_ADDR_INVALID(pstCCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_SUCC;
    }

    sc_call_trace(pstCCB, "Start process event %s.", esl_event_get_header(pstEvent, "Event-Name"));


    ulStatus = pstCCB->usStatus;
    switch (ulStatus)
    {
        case SC_CCB_IDEL:
            /* û��BREAK */
        case SC_CCB_INIT:
            /* ����״̬��INIT */
            SC_CCB_SET_STATUS(pstCCB, SC_CCB_INIT);

            /* ֪ͨ����̣߳�CCB�Լ���ȫ��hash���� */
            sc_ccb_syn_post(pstCCB->szUUID);

            ulRet = DOS_SUCC;
            break;
        case SC_CCB_AUTH:
        case SC_CCB_EXEC:
        case SC_CCB_REPORT:
            /* @todo report the call status */
            break;
        case SC_CCB_RELEASE:
            /* @TODO: Release the CCB */
        default:
            DOS_ASSERT(0);
            ulRet = DOS_FAIL;
            break;
    }

    sc_call_trace(pstCCB, "Finished to process %s event.", esl_event_get_header(pstEvent, "Event-Name"));

    SC_TRACE_OUT();
    return ulRet;
}

U32 sc_ep_channel_destroy_proc(esl_event_t *pstEvent, esl_handle_t *pstHandle, SC_CCB_ST *pstCCB)
{
    S8 *pszUUID;
    U32 ulStatus, ulRet = DOS_FAIL;

    SC_TRACE_IN(pstEvent, pstHandle, pstCCB, 0);

    if (DOS_ADDR_INVALID(pstEvent)
        || DOS_ADDR_INVALID(pstHandle)
        || DOS_ADDR_INVALID(pstCCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_SUCC;
    }

    if (!pstCCB)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_SUCC;
    }

    sc_call_trace(pstCCB, "Start process event %s.", esl_event_get_header(pstEvent, "Event-Name"));

    /* @TODO: �����Ҫ����Ҫ�ϱ��� */
    ulStatus = pstCCB->usStatus;
    switch (ulStatus)
    {
        case SC_CCB_IDEL:
        case SC_CCB_INIT:
        case SC_CCB_AUTH:
        case SC_CCB_EXEC:
            /* @TODO �ϱ�ҵ�� */
        case SC_CCB_REPORT:
        case SC_CCB_RELEASE:
            /* ������ ����ͳһ�ͷ���Դ */
            break;
        default:
            DOS_ASSERT(0);
            ulRet = DOS_FAIL;
            break;
    }


    pszUUID = esl_event_get_header(pstEvent, "Caller-Unique-ID");
    if (!pszUUID || '\0' == pszUUID[0])
    {
        DOS_ASSERT(0);
        goto process_fail;
    }
    sc_ccb_hash_tables_delete(pszUUID);
    //pthread_mutex_destroy(&pstCCB->mutexCCBLock);
    sc_ccb_free(pstCCB);
    pstCCB = NULL;
    sc_call_trace(pstCCB, "Finished to process %s event.", esl_event_get_header(pstEvent, "Event-Name"));

    SC_TRACE_OUT();
    return DOS_SUCC;

process_fail:
    sc_logr_error("Can not get the UUID for the event.", esl_event_get_header(pstEvent, "Event-Name"));
    return DOS_FAIL;
}
#if 0
U32 sc_ep_channel_exec_proc(esl_event_t *pstEvent, esl_handle_t *pstHandle, SC_CCB_ST *pstCCB)
{
    U32 ulStatus, ulRet = DOS_FAIL;

    SC_TRACE_IN(pstEvent, pstHandle, pstCCB, 0);

    if (DOS_ADDR_INVALID(pstEvent)
        || DOS_ADDR_INVALID(pstHandle)
        || DOS_ADDR_INVALID(pstCCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_SUCC;
    }

    sc_call_trace(pstCCB, "Start process event %s.", esl_event_get_header(pstEvent, "Event-Name"));

    ulStatus = pstCCB->usStatus;
    switch (ulStatus)
    {
        case SC_CCB_IDEL:
            /* û��BREAK */
        case SC_CCB_INIT:
        case SC_CCB_AUTH:
            DOS_ASSERT(0);

            sc_logr_info("Unauthed call while exec.");

            /* ��ֹ���� */

            ulRet = DOS_FAIL;
            break;
        case SC_CCB_EXEC:
        case SC_CCB_REPORT:
        case SC_CCB_RELEASE:
        default:
            DOS_ASSERT(0);
            ulRet = DOS_FAIL;
            break;
    }


    sc_call_trace(pstCCB, "Finished to process %s event.", esl_event_get_header(pstEvent, "Event-Name"));

    SC_TRACE_OUT();
    return DOS_SUCC;

}

U32 sc_ep_channel_exec_complete_proc(esl_event_t *pstEvent, esl_handle_t *pstHandle, SC_CCB_ST *pstCCB)
{
    SC_TRACE_IN(pstEvent, pstHandle, pstCCB, 0);

    if (DOS_ADDR_INVALID(pstEvent)
        || DOS_ADDR_INVALID(pstHandle)
        || DOS_ADDR_INVALID(pstCCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_SUCC;
    }

    sc_call_trace(pstCCB, "Start process event %s.", esl_event_get_header(pstEvent, "Event-Name"));

    sc_call_trace(pstCCB, "Finished to process %s event.", esl_event_get_header(pstEvent, "Event-Name"));

    SC_TRACE_OUT();
    return DOS_SUCC;

}
#endif
U32 sc_ep_channel_hungup_proc(esl_event_t *pstEvent, esl_handle_t *pstHandle, SC_CCB_ST *pstCCB)
{
    U32 ulStatus, ulRet = DOS_SUCC;

    SC_TRACE_IN(pstEvent, pstHandle, pstCCB, 0);

    if (DOS_ADDR_INVALID(pstEvent)
        || DOS_ADDR_INVALID(pstHandle)
        || DOS_ADDR_INVALID(pstCCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_SUCC;
    }

    sc_call_trace(pstCCB, "Start process event %s.", esl_event_get_header(pstEvent, "Event-Name"));

    SC_CCB_SET_STATUS(pstCCB, SC_CCB_REPORT);
    /* @TODO: �ϱ����� */

    ulStatus = pstCCB->usStatus;
    switch (ulStatus)
    {
        case SC_CCB_IDEL:
        case SC_CCB_INIT:
        case SC_CCB_AUTH:
            DOS_ASSERT(0);

            /* �쳣���� */

            break;
        case SC_CCB_EXEC:
        case SC_CCB_REPORT:
            /* @todo  �Ʒ���� */
            SC_CCB_SET_STATUS(pstCCB, SC_CCB_RELEASE);
            break;
        case SC_CCB_RELEASE:
            DOS_ASSERT(0);

            /* �쳣���� */

            break;
        default:
            DOS_ASSERT(0);
            ulRet = DOS_FAIL;
            break;
    }


    sc_call_trace(pstCCB, "Finished to process %s event.", esl_event_get_header(pstEvent, "Event-Name"));

    SC_TRACE_OUT();
    return DOS_SUCC;

}

U32 sc_ep_channel_hungup_complete_proc(esl_event_t *pstEvent, esl_handle_t *pstHandle, SC_CCB_ST *pstCCB)
{
    U32 ulStatus, ulRet = DOS_SUCC;

    SC_TRACE_IN(pstEvent, pstHandle, pstCCB, 0);

    if (DOS_ADDR_INVALID(pstEvent)
        || DOS_ADDR_INVALID(pstHandle)
        || DOS_ADDR_INVALID(pstCCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_SUCC;
    }

    sc_call_trace(pstCCB, "Start process event %s.", esl_event_get_header(pstEvent, "Event-Name"));

    ulStatus = pstCCB->usStatus;
    switch (ulStatus)
    {
        case SC_CCB_IDEL:
        case SC_CCB_INIT:
        case SC_CCB_AUTH:
            DOS_ASSERT(0);

            /* �쳣���� */

            break;
        case SC_CCB_EXEC:
        case SC_CCB_REPORT:
            /* @todo  �Ʒ���� */
            SC_CCB_SET_STATUS(pstCCB, SC_CCB_REPORT);
            break;
        case SC_CCB_RELEASE:
            DOS_ASSERT(0);

            /* �쳣���� */

            break;
        default:
            DOS_ASSERT(0);
            ulRet = DOS_FAIL;
            break;
    }


    sc_call_trace(pstCCB, "Finished to process %s event.", esl_event_get_header(pstEvent, "Event-Name"));

    SC_TRACE_OUT();
    return DOS_SUCC;

}


U32 sc_ep_channel_answer_proc(esl_event_t *pstEvent, esl_handle_t *pstHandle, SC_CCB_ST *pstCCB)
{
    U32 ulStatus, ulRet = DOS_SUCC;

    SC_TRACE_IN(pstEvent, pstHandle, pstCCB, 0);

    if (DOS_ADDR_INVALID(pstEvent)
        || DOS_ADDR_INVALID(pstHandle)
        || DOS_ADDR_INVALID(pstCCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_SUCC;
    }

    sc_call_trace(pstCCB, "Start process event %s.", esl_event_get_header(pstEvent, "Event-Name"));
    ulStatus = pstCCB->usStatus;
    switch (ulStatus)
    {
        case SC_CCB_IDEL:
        case SC_CCB_INIT:
        case SC_CCB_AUTH:
            DOS_ASSERT(0);

            /* �쳣���� */

            break;
        case SC_CCB_EXEC:
            /* @TODO : ��ʼ�Ʒ� */
            break;
        case SC_CCB_REPORT:
        case SC_CCB_RELEASE:
        default:
            DOS_ASSERT(0);
            ulRet = DOS_FAIL;
            break;
    }

    sc_call_trace(pstCCB, "Finished to process %s event.", esl_event_get_header(pstEvent, "Event-Name"));

    SC_TRACE_OUT();
    return DOS_SUCC;

}

U32 sc_ep_dtmf_proc(esl_event_t *pstEvent, esl_handle_t *pstHandle, SC_CCB_ST *pstCCB)
{
    SC_TRACE_IN(pstEvent, pstHandle, pstCCB, 0);

    if (DOS_ADDR_INVALID(pstEvent)
        || DOS_ADDR_INVALID(pstHandle)
        || DOS_ADDR_INVALID(pstCCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_SUCC;
    }

    sc_call_trace(pstCCB, "Start process event %s.", esl_event_get_header(pstEvent, "Event-Name"));



    sc_call_trace(pstCCB, "Finished to process %s event.", esl_event_get_header(pstEvent, "Event-Name"));

    SC_TRACE_OUT();
    return DOS_SUCC;

}


U32 sc_ep_channel_media_proc(esl_event_t *pstEvent, esl_handle_t *pstHandle, SC_CCB_ST *pstCCB)
{
    U32 ulStatus, ulRet = DOS_SUCC;

    SC_TRACE_IN(pstEvent, pstHandle, pstCCB, 0);

    if (DOS_ADDR_INVALID(pstEvent)
        || DOS_ADDR_INVALID(pstHandle)
        || DOS_ADDR_INVALID(pstCCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_SUCC;
    }

    sc_call_trace(pstCCB, "Start process event %s.", esl_event_get_header(pstEvent, "Event-Name"));
    ulStatus = pstCCB->usStatus;
    switch (ulStatus)
    {
        case SC_CCB_IDEL:
        case SC_CCB_INIT:
        case SC_CCB_AUTH:
            DOS_ASSERT(0);

            /* �쳣���� */

            break;
        case SC_CCB_EXEC:
            /* @TODO : ��ʼ�Ʒ� */
            break;
        case SC_CCB_REPORT:
        case SC_CCB_RELEASE:
        default:
            DOS_ASSERT(0);
            ulRet = DOS_FAIL;
            break;
    }

    sc_call_trace(pstCCB, "Finished to process %s event.", esl_event_get_header(pstEvent, "Event-Name"));

    SC_TRACE_OUT();
    return DOS_SUCC;

}

U32 sc_ep_channel_media_complete_proc(esl_event_t *pstEvent, esl_handle_t *pstHandle, SC_CCB_ST *pstCCB)
{
    SC_TRACE_IN(pstEvent, pstHandle, pstCCB, 0);

    if (DOS_ADDR_INVALID(pstEvent)
        || DOS_ADDR_INVALID(pstHandle)
        || DOS_ADDR_INVALID(pstCCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_SUCC;
    }

    sc_call_trace(pstCCB, "Start process event %s.", esl_event_get_header(pstEvent, "Event-Name"));

    sc_call_trace(pstCCB, "Finished to process %s event.", esl_event_get_header(pstEvent, "Event-Name"));

    SC_TRACE_OUT();
    return DOS_SUCC;
}

U32 sc_ep_session_heartbeat(esl_event_t *pstEvent, esl_handle_t *pstHandle, SC_CCB_ST *pstCCB)
{
    U32 ulStatus, ulRet = DOS_SUCC;

    SC_TRACE_IN(pstEvent, pstHandle, pstCCB, 0);

    if (DOS_ADDR_INVALID(pstEvent)
        || DOS_ADDR_INVALID(pstHandle)
        || DOS_ADDR_INVALID(pstCCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_SUCC;
    }

    sc_call_trace(pstCCB, "Start process event %s.", esl_event_get_header(pstEvent, "Event-Name"));
    ulStatus = pstCCB->usStatus;
    switch (ulStatus)
    {
        case SC_CCB_IDEL:
        case SC_CCB_INIT:
        case SC_CCB_AUTH:
            DOS_ASSERT(0);

            /* �쳣���� */

            break;
        case SC_CCB_EXEC:
            /* @TODO : �м�Ʒ� */
            break;
        case SC_CCB_REPORT:
        case SC_CCB_RELEASE:
        default:
            DOS_ASSERT(0);
            ulRet = DOS_FAIL;
            break;
    }

    sc_call_trace(pstCCB, "Finished to process %s event.", esl_event_get_header(pstEvent, "Event-Name"));

    SC_TRACE_OUT();
    return DOS_SUCC;

}

U32 sc_ep_process(esl_event_t *pstEvent, esl_handle_t *pstHandle)
{
    S8                     *pszUUID = NULL;
    S8                     *pszCallerSource = NULL;
    S8                     *pszCCBNo  = NULL;
    SC_CCB_ST              *pstCCB    = NULL;
    U32                    ulCCBNo;
    U32                    ulRet = DOS_FAIL;
    S8                     szBuffCmd[128] = { 0 };

    SC_TRACE_IN(pstEvent, pstHandle, 0, 0);

    if (DOS_ADDR_INVALID(pstEvent) || DOS_ADDR_INVALID(pstHandle))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    /* ��ȡ�¼���UUID */
    pszUUID = esl_event_get_header(pstEvent, "Caller-Unique-ID");
    if (DOS_ADDR_INVALID(pszUUID))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    /* ���ĳ���¼�ȷʵû��Caller-Source ?*/
    pszCallerSource = esl_event_get_header(pstEvent, "Caller-Source");
    if (DOS_ADDR_INVALID(pszCallerSource))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    /*
     * ����Ǵ���ͨ���ͷ�Ϊ�������
     * 1.���������mod_loopback��˵�����Զ�������У�����ط���Ҫͨ��һЩ������ȡCCBָ��
     * 2.���������Ҫ����CCB
     */
    if (ESL_EVENT_CHANNEL_CREATE == pstEvent->event_id)
    {
        if (0 == dos_stricmp(pszCallerSource, "mod_loopback"))
        {
            pszCCBNo = esl_event_get_header(pstEvent, "variable_ccb_number");
            if (DOS_ADDR_INVALID(pszCCBNo)
                || dos_atoul(pszCCBNo, &ulCCBNo) < 0)
            {
                DOS_ASSERT(0);
                sc_logr_error("%s", "Invalid CHANNEL_CREATE event. With no CCB No");

                SC_CCB_SET_STATUS(pstCCB, SC_CCB_REPORT);

                SC_TRACE_OUT();
                return DOS_FAIL;
            }

            pstCCB = sc_ccb_get(ulCCBNo);
            if (!SC_CCB_IS_VALID(pstCCB))
            {
                DOS_ASSERT(0);
                sc_logr_error("Invalid CHANNEL_CREATE event. With no valid CCB(%d)", ulCCBNo);

                SC_CCB_SET_STATUS(pstCCB, SC_CCB_REPORT);

                SC_TRACE_OUT();
                return DOS_FAIL;
            }
        }
        else
        {
            pstCCB = sc_ccb_alloc();
            if (!pstCCB)
            {
                DOS_ASSERT(0);
                sc_logr_error("%s", "Alloc CCB FAIL.");

                SC_CCB_SET_STATUS(pstCCB, SC_CCB_REPORT);

                SC_TRACE_OUT();
                return DOS_FAIL;
            }

            sc_ccb_hash_tables_add(pszUUID, pstCCB);

            /* ��ͨ�����ñ��� */
            dos_snprintf(szBuffCmd
                         , sizeof(szBuffCmd) - 1
                         , "api uuid_setvar %s ccb_number %d\r\n"
                         , pszUUID
                         , pstCCB->usCCBNo);

            esl_send(pstHandle, szBuffCmd);


            ulRet = sc_ep_parse_event(pstEvent, pstCCB);
            if (ulRet != DOS_SUCC)
            {
                DOS_ASSERT(0);
                sc_logr_error("%s", "Parse the param from the event fail.");

                SC_CCB_SET_STATUS(pstCCB, SC_CCB_REPORT);
            }
        }
    }
    else
    {
        /* ͨ������ʱ�Ѿ��������˸ñ��� */
#if 0
        pszCCBNo = esl_event_get_header(pstEvent, "variable_ccb_number");
        if (DOS_ADDR_INVALID(pszCCBNo)
            || dos_atoul(pszCCBNo, &ulCCBNo) < 0)
        {
            DOS_ASSERT(0);
            sc_logr_error("%s", "Invalid CHANNEL_CREATE event. With no CCB No");

            SC_CCB_SET_STATUS(pstCCB, SC_CCB_REPORT);

            SC_TRACE_OUT();
            return DOS_FAIL;
        }
#endif

        pstCCB = sc_ccb_get(ulCCBNo);
        if (!SC_CCB_IS_VALID(pstCCB))
        {
            DOS_ASSERT(0);
            sc_logr_error("Invalid CHANNEL_CREATE event. With no valid CCB(%d)", ulCCBNo);

            SC_TRACE_OUT();
            return DOS_FAIL;
        }

#if 0
        /* �������CHANNEL CREATE�¼�������Ҫͨ��UUID����ȡCCB */
        pstCCB = sc_ccb_hash_tables_find(pszUUID);
        if (NULL == pstCCB)
        {
            DOS_ASSERT(0);

            sc_logr_info("Recv event %s, but the call with the UUID %s has not handeled yet! The call will be killed."
                            , esl_event_get_body(pstEvent)
                            , pszUUID);
            SC_TRACE_OUT();
            return DOS_FAIL;
        }
#endif
    }

    sc_logr_info("Start process event: %s(%d)", esl_event_get_header(pstEvent, "Event-Name"), pstEvent->event_id);

    switch (pstEvent->event_id)
    {
        /* ��ȡ����״̬ */
        case ESL_EVENT_CHANNEL_CREATE:
            ulRet = sc_ep_channel_create_proc(pstEvent, pstHandle, pstCCB);
            break;
        case ESL_EVENT_CHANNEL_DESTROY:
            ulRet = sc_ep_channel_destroy_proc(pstEvent, pstHandle, pstCCB);
            break;
#if 0
        case ESL_EVENT_CHANNEL_EXECUTE:
            ulRet = sc_ep_channel_exec_proc(pstEvent, pstHandle, pstCCB);
            break;
        case ESL_EVENT_CHANNEL_EXECUTE_COMPLETE:
            ulRet = sc_ep_channel_exec_complete_proc(pstEvent, pstHandle, pstCCB);
            break;
#endif
        case ESL_EVENT_CHANNEL_HANGUP:
            ulRet = sc_ep_channel_hungup_proc(pstEvent, pstHandle, pstCCB);
            break;
        case ESL_EVENT_CHANNEL_HANGUP_COMPLETE:
            ulRet = sc_ep_channel_hungup_complete_proc(pstEvent, pstHandle, pstCCB);
            break;
        case ESL_EVENT_CHANNEL_ANSWER:
            ulRet = sc_ep_channel_answer_proc(pstEvent, pstHandle, pstCCB);
            break;
        case ESL_EVENT_DTMF:
            ulRet = sc_ep_dtmf_proc(pstEvent, pstHandle, pstCCB);
            break;
        case ESL_EVENT_CHANNEL_PROGRESS:
        case ESL_EVENT_CHANNEL_PROGRESS_MEDIA:
            ulRet = sc_ep_channel_media_proc(pstEvent, pstHandle, pstCCB);
            break;
        case ESL_EVENT_SESSION_HEARTBEAT:
            ulRet = sc_ep_session_heartbeat(pstEvent, pstHandle, pstCCB);
            break;
        default:
            //DOS_ASSERT(0);
            ulRet = DOS_SUCC;

            sc_logr_info("Recv unhandled event: %s(%d)", esl_event_get_header(pstEvent, "Event-Name"), pstEvent->event_id);
            break;
    }

    sc_logr_info("Process finished event: %s(%d). Result:%s"
                    , esl_event_get_header(pstEvent, "Event-Name")
                    , pstEvent->event_id
                    , (DOS_SUCC == ulRet) ? "Successfully" : "FAIL");

    SC_TRACE_OUT();
    return ulRet;
}


VOID*  sc_ep_runtime(VOID *ptr)
{
    U32                  ulRet = ESL_FAIL;
    S8                   *pszIsLoopback = NULL;

    while(1)
    {
        /* ����˳���־�����ϣ���׼���˳��� */
        if (g_pstHandle->blIsWaitingExit)
        {
            sc_logr_notice("%s", "Dialer exit flag has been set. the task will be exit.");
            break;
        }

        /*
         * ��������Ƿ�����
         * ������Ӳ���������׼������
         **/
        if (!g_pstHandle->blIsESLRunning)
        {
            sc_logr_notice("%s", "ELS for event connection has been down, re-connect.");
            ulRet = esl_connect(&g_pstHandle->stHandle, "127.0.0.1", 8021, NULL, "ClueCon");
            if (ESL_SUCCESS != ulRet)
            {
                esl_disconnect(&g_pstHandle->stHandle);
                sc_logr_notice("ELS for event re-connect fail, return code:%d, Msg:%s. Will be retry after 1 second.", ulRet, g_pstHandle->stHandle.err);

                sleep(1);
                continue;
            }

            g_pstHandle->blIsESLRunning = DOS_TRUE;

            esl_global_set_default_logger(g_ulESLDebugLevel);
            esl_events(&g_pstHandle->stHandle, ESL_EVENT_TYPE_PLAIN, SC_EP_EVENT_LIST);

            sc_logr_notice("%s", "ELS for event connect Back to Normal.");
        }

        esl_global_set_default_logger(g_ulESLDebugLevel);
        ulRet = esl_recv_event(&g_pstHandle->stHandle, 1, NULL);
        if (ESL_FAIL == ulRet)
        {
            continue;
        }

        if (!g_pstHandle->stHandle.last_ievent)
        {
            continue;
        }

        esl_event_t *pstEvent = g_pstHandle->stHandle.last_ievent;
        if (!pstEvent)
        {
            continue;
        }

        /* ������loopback�ĺ��� */
        /* ���ﱾӦ��ʹ��is_loopback����������������������һЩʱ����û�б����ã����¹��˲�ȫ */
        pszIsLoopback = esl_event_get_header(pstEvent, "Channel-Name");
        if (pszIsLoopback
            && 0 == dos_strnicmp(pszIsLoopback, "loopback", dos_strlen("loopback")))
        {
            continue;
        }

        sc_logr_info("ESL recv event: %s(%d), CCB No:%s", esl_event_get_header(pstEvent, "Event-Name"), pstEvent->event_id, esl_event_get_header(pstEvent, "variable_ccb_no"));

        ulRet = sc_ep_process(pstEvent, &g_pstHandle->stHandle);
        if (ulRet != DOS_SUCC)
        {
            DOS_ASSERT(0);
        }
    }


}

U32 sc_ep_init()
{
    SC_TRACE_IN(0, 0, 0, 0);

    g_pstHandle = dos_dmem_alloc(sizeof(SC_EP_HANDLE_ST));
    if (!g_pstHandle)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    dos_memzero(g_pstHandle, sizeof(SC_EP_HANDLE_ST));
    g_pstHandle->blIsESLRunning = DOS_FALSE;
    g_pstHandle->blIsWaitingExit = DOS_FALSE;

    SC_TRACE_OUT();
    return DOS_SUCC;
}

U32 sc_ep_start()
{
    SC_TRACE_IN(0, 0, 0, 0);

    if (pthread_create(&g_pstHandle->pthID, NULL, sc_ep_runtime, NULL) < 0)
    {
        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    SC_TRACE_OUT();
    return DOS_SUCC;
}

U32 sc_ep_shutdown()
{
    SC_TRACE_IN(0, 0, 0, 0);

    g_pstHandle->blIsWaitingExit = DOS_TRUE;

    SC_TRACE_OUT();
    return DOS_SUCC;
}

