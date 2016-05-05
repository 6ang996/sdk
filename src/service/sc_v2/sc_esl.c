/**
 * @file : sc_esl.c
 *
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 * SCģ����շ���ESL����ĵط�
 *
 * @date: 2016��1��9��
 * @arthur: Larry
 */

#ifdef __cplusplus
extern "C" {
#endif /* End of __cplusplus */

#include <dos.h>
#include <esl.h>
#include "sc_def.h"
#include "sc_su.h"
#include "sc_debug.h"
#include "sc_publish.h"
#include "sc_http_api.h"


#define SC_ESL_EVENT_LIST \
            "BACKGROUND_JOB " \
            "CHANNEL_PARK " \
            "CHANNEL_CREATE " \
            "CHANNEL_ANSWER " \
            "CHANNEL_PROGRESS " \
            "CHANNEL_PROGRESS_MEDIA " \
            "CHANNEL_HANGUP_COMPLETE " \
            "CHANNEL_HOLD " \
            "CHANNEL_UNHOLD " \
            "PLAYBACK_START " \
            "PLAYBACK_STOP " \
            "RECORD_START " \
            "RECORD_STOP " \
            "DTMF " \
            "SESSION_HEARTBEAT "


/** ����ESL�¼��߳� */
pthread_t             g_stESLEventRecv;

/** ����ESL�¼��߳� */
pthread_t             g_stESLEventProc;


/** ҵ���Ӳ��ϱ�ʱ����Ϣ���� */
DLL_S                 g_stESLEventQueue;

/** ҵ���Ӳ��ϱ�ʱ����Ϣ������ */
pthread_mutex_t       g_mutexESLEventQueue = PTHREAD_MUTEX_INITIALIZER;

/** ҵ���Ӳ��ϱ�ʱ����Ϣ������������ */
pthread_cond_t        g_condESLEventQueue = PTHREAD_COND_INITIALIZER;

/** esl ��� */
esl_handle_t          g_stESLRecvHandle;

/** esl ��� */
esl_handle_t          g_stESLSendHandle;


/** �Ƿ�Ҫ���˳� */
BOOL                  g_blESLEventRunning = DOS_FALSE;


/**
 * ���¼������е�xml�����ļ�
 *
 * @return �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 */
U32 sc_esl_reloadxml()
{
    sc_esl_execute_cmd("api reloadxml\r\n", NULL, 0);

    return DOS_SUCC;
}

/**
 * ���¼������е�xml�����ļ�
 *
 * @param �������ݵ�����Դ����
 *
 * @return �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 */
U32 sc_esl_update_gateway(U32 ulAction, U32 ulID)
{
    S8 szBuff[100] = { 0, };

    switch (ulAction)
    {
        case SC_API_CMD_ACTION_GATEWAY_ADD:
            sc_esl_execute_cmd("bgapi sofia profile external rescan", NULL, 0);
            break;

        case SC_API_CMD_ACTION_GATEWAY_DELETE:
            dos_snprintf(szBuff, sizeof(szBuff), "bgapi sofia profile external killgw %u", ulID);
            sc_esl_execute_cmd(szBuff, NULL, 0);
            break;

        case SC_API_CMD_ACTION_GATEWAY_UPDATE:
        case SC_API_CMD_ACTION_GATEWAY_SYNC:
            dos_snprintf(szBuff, sizeof(szBuff), "bgapi sofia profile external killgw %u", ulID);
            sc_esl_execute_cmd(szBuff, NULL, 0);
            sc_esl_execute_cmd("bgapi sofia profile external rescan", NULL, 0);
            break;
    }

    sc_esl_execute_cmd("bgapi reloadacl reloadxml", NULL, 0);

    return DOS_SUCC;
}



/**
 * ����ESL���� @a pszApp������Ϊ @a pszArg������Ϊ @a pszUUID
 *
 * @param const S8 *pszApp: ִ�е�����
 * @param const S8 *pszArg: �������
 * @param const S8 *pszUUID: channel��UUID
 *
 * @return �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 *
 * @note ���ú�����ִ������ʱ��������ֵ�ǰ����Ѿ�ʧȥ���ӣ�������������ESL������, ����ֻ������1�Σ���������ٴ�ʧ�ܣ���ִ��ʧ��
 */
U32 sc_esl_execute(const S8 *pszApp, const S8 *pszArg, const S8 *pszUUID)
{
    U32 ulRet = DOS_FAIL;
    S32 lCount = 20;
    S8  szCMD[512] = {0};

    if (DOS_ADDR_INVALID(pszApp)
        || DOS_ADDR_INVALID(pszUUID))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    while (1)
    {
        if (!g_stESLSendHandle.connected)
        {
            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_ESL), "ELS for event connection has been down, re-connect. %s", __FUNCTION__);
            ulRet = esl_connect_timeout(&g_stESLSendHandle, "127.0.0.1", 8021, NULL, "ClueCon", 100);
            if (ESL_SUCCESS != ulRet)
            {
                sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_ESL), "ELS for send event re-connect fail, return code:%d, Msg:%s. ", ulRet, g_stESLSendHandle.err);

                if (lCount-- < 0)
                {
                    dos_snprintf(szCMD, sizeof(szCMD), "Esl re-connect fail 20. The system will be shutdown with 2 seconds");
                    dos_log(LOG_LEVEL_EMERG, LOG_TYPE_RUNINFO, szCMD);
                    dos_task_delay(2000);
                    exit(0);
                }
                dos_task_delay(1000);
                continue;
            }

            g_stESLSendHandle.event_lock = 1;
        }

        break;
    }

    if (ESL_SUCCESS != esl_execute(&g_stESLSendHandle, pszApp, pszArg, pszUUID))
    {
        sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_ESL), "ESL execute command fail. Result:%d, APP: %s, ARG : %s, UUID: %s"
                        , ulRet
                        , pszApp
                        , DOS_ADDR_VALID(pszArg) ? pszArg : "NULL"
                        , DOS_ADDR_VALID(pszUUID) ? pszUUID : "NULL");

        return DOS_FAIL;
    }

    sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ESL), "ESL execute command SUCC. APP: %s, Param: %s, UUID: %s"
                    , pszApp
                    , DOS_ADDR_VALID(pszArg) ? pszArg : "NULL"
                    , pszUUID);

    return DOS_SUCC;
}


/**
 * ִ��ESL���� @a pszCmd, ���������Ӧ����JOB-UUID���ݣ����� @a pszUUID
 * ��ΪNULL���ͽ�JOB-UUID������ @a pszUUID �У�@a ulLenghtΪ @a pszUUID �ĳ���
 *
 * @param const S8 *pszCmd:
 * @param S8 *pszUUID: ����������洢BGAPI��UUID
 * @param U32 ulLenght: UUID���泤��
 *
 * @return �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 *
 * @note ���ú�����ִ������ʱ��������ֵ�ǰ����Ѿ�ʧȥ���ӣ�������������ESL������
 *
 * @note �����Ҫ��ȡJOB-UUID��@a ulLenght ������� @def SC_UUID_LENGTH
 */
U32 sc_esl_execute_cmd(const S8 *pszCmd, S8 *pszUUID, U32 ulLenght)
{
    U32 ulRet = DOS_FAIL;
    S8  *pszUUIDStart;
    S32 lCount = 20;
    S8  szCMD[512] = {0};
    S8  szReplyText[1024] = { 0 };

    if (DOS_ADDR_INVALID(pszCmd))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    /* �ж��Ƿ���Ҫ���� */
    while (1)
    {
        if (!g_stESLSendHandle.connected)
        {
            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_ESL), "ELS for event connection has been down, re-connect. %s", __FUNCTION__);
            ulRet = esl_connect_timeout(&g_stESLSendHandle, "127.0.0.1", 8021, NULL, "ClueCon", 100);
            if (ESL_SUCCESS != ulRet)
            {
                sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_ESL), "ELS for send event re-connect fail, return code:%d, Msg:%s. ", ulRet, g_stESLSendHandle.err);

                if (lCount-- < 0)
                {
                    dos_snprintf(szCMD, sizeof(szCMD), "Esl re-connect fail 20. The system will be shutdown with 2 seconds");
                    dos_log(LOG_LEVEL_EMERG, LOG_TYPE_RUNINFO, szCMD);
                    dos_task_delay(2000);
                    exit(0);
                }
                dos_task_delay(1000);
                continue;
            }

            g_stESLSendHandle.event_lock = 1;
            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_ESL), "ELS for event connect Back to Normal. %s", __FUNCTION__);
        }

        break;
    }

    /* ���Ͳ��������� */
    if (ESL_SUCCESS != esl_send_recv(&g_stESLSendHandle, pszCmd))
    {
        sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_ESL), "ESL execute command fail. Result:%d, CMD: %s"
                        , ulRet
                        , pszCmd);

        return DOS_FAIL;
    }

    /** ��ȡ��Ӧ */
    if (DOS_ADDR_INVALID(g_stESLSendHandle.last_sr_reply)|| dos_strnicmp(g_stESLSendHandle.last_sr_reply, "-ERR", 4) == 0)
    {
        sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_ESL), "ESL execute command fail. reply text: %s. CMD: %s", g_stESLSendHandle.last_sr_reply, pszCmd);

        return DOS_FAIL;
    }

    dos_strncpy(szReplyText, g_stESLSendHandle.last_sr_reply, sizeof(szReplyText));
    szReplyText[sizeof(szReplyText) - 1] = '\0';

    if (DOS_ADDR_VALID(pszUUID) && ulLenght > SC_UUID_LENGTH)
    {
        pszUUIDStart = dos_strstr(szReplyText, "Job-UUID: ");
        if (DOS_ADDR_VALID(pszUUIDStart))
        {
            pszUUIDStart += dos_strlen("Job-UUID: ");
            dos_snprintf(pszUUID, ulLenght, "%s", pszUUIDStart);
        }
        else
        {
            pszUUID[0] = '\0';
        }
    }

    sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ESL), "ESL execute command SUCC. CMD: %s", pszCmd);

    return DOS_SUCC;
}

/**
 * ����ESL�¼�����Ҫ�Ƿַ�����
 */
VOID sc_esl_event_process(esl_event_t *pstEvent)
{
    S8    *pszUUID = NULL;
    U32   ulRet = DOS_FAIL;
    SC_LEG_CB *pstLegCB = NULL;

    if (DOS_ADDR_INVALID(pstEvent))
    {
        DOS_ASSERT(0);
        return;
    }

    sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ESL), "Processing esl event. Event: %s(%d). channel name: %s"
                          , esl_event_get_header(pstEvent, "Event-Name")
                          , pstEvent->event_id
                          , esl_event_get_header(pstEvent, "Channel-Name"));


    if (ESL_EVENT_CHANNEL_CREATE != pstEvent->event_id
        && ESL_EVENT_BACKGROUND_JOB != pstEvent->event_id)
    {
        pszUUID = esl_event_get_header(pstEvent, "Caller-Unique-ID");
        if (DOS_ADDR_INVALID(pszUUID))
        {
            DOS_ASSERT(0);
            return;
        }

        pstLegCB = sc_lcb_hash_find(pszUUID);
        if (DOS_ADDR_INVALID(pstLegCB))
        {
            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ESL), "Recv event without lcb. UUID: %s", pszUUID);
            return;
        }

        if (!pstLegCB->bValid)
        {
            sc_lcb_hash_delete(pszUUID);

            DOS_ASSERT(0);
            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ESL), "Recv event with an invalid lcb.");
            return;
        }
    }


    switch (pstEvent->event_id)
    {
        case ESL_EVENT_BACKGROUND_JOB:
            ulRet = sc_esl_event_background_job(pstEvent);
            break;

        case ESL_EVENT_CHANNEL_PARK:
            ulRet = sc_esl_event_park(pstEvent, pstLegCB);
            break;

        case ESL_EVENT_CHANNEL_CREATE:
            ulRet = sc_esl_event_create(pstEvent);
            break;

        case ESL_EVENT_CHANNEL_ANSWER:
            ulRet = sc_esl_event_answer(pstEvent, pstLegCB);
            break;

        case ESL_EVENT_CHANNEL_PROGRESS:
            ulRet = sc_esl_event_progress(pstEvent, pstLegCB);
            break;

        case ESL_EVENT_CHANNEL_PROGRESS_MEDIA:
            ulRet = sc_esl_event_progress(pstEvent, pstLegCB);
            break;

        case ESL_EVENT_CHANNEL_HANGUP_COMPLETE:
            ulRet = sc_esl_event_hangup(pstEvent, pstLegCB);
            break;

        case ESL_EVENT_CHANNEL_HOLD:
            ulRet = sc_esl_event_hold(pstEvent, pstLegCB);
            break;

        case ESL_EVENT_CHANNEL_UNHOLD:
            ulRet = sc_esl_event_unhold(pstEvent, pstLegCB);
            break;

        case ESL_EVENT_PLAYBACK_START:
            ulRet = sc_esl_event_playback_start(pstEvent, pstLegCB);
            break;

        case ESL_EVENT_PLAYBACK_STOP:
            ulRet = sc_esl_event_playback_stop(pstEvent, pstLegCB);
            break;

        case ESL_EVENT_RECORD_START:
            ulRet = sc_esl_event_record_start(pstEvent, pstLegCB);
            break;

        case ESL_EVENT_RECORD_STOP:
            ulRet = sc_esl_event_record_stop(pstEvent, pstLegCB);
            break;

        case ESL_EVENT_DTMF:
            ulRet = sc_esl_event_dtmf(pstEvent, pstLegCB);
            break;

        case ESL_EVENT_SESSION_HEARTBEAT:
            ulRet = sc_esl_event_session_heartbeat(pstEvent, pstLegCB);
            break;

        default:
            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_ESL), "Useless esl event. %s(%d)"
                          , esl_event_get_header(pstEvent, "Event-Name")
                          , pstEvent->event_id);
            break;
    }

    sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ESL), "Processed esl event. Event: %s(%d). channel name: %s. Result: %s"
                          , esl_event_get_header(pstEvent, "Event-Name")
                          , pstEvent->event_id
                          , esl_event_get_header(pstEvent, "Channel-Name")
                          , DOS_SUCC == ulRet ? "succ" : "FAIL");

}

/**
 * ����ESL�¼�����
 */
VOID *sc_esl_process_runtime(VOID *ptr)
{
    struct timespec     stTimeout;
    DLL_NODE_S          *pstListNode = NULL;
    esl_event_t         *pstEvent   = NULL;
    SC_PTHREAD_MSG_ST   *pstPthreadMsg = NULL;

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    pstPthreadMsg = dos_pthread_cb_alloc();
    if (DOS_ADDR_VALID(pstPthreadMsg))
    {
        pstPthreadMsg->ulPthID = pthread_self();
        pstPthreadMsg->func = sc_esl_process_runtime;
        pstPthreadMsg->pParam = ptr;
        dos_strcpy(pstPthreadMsg->szName, "sc_esl_process_runtime");
    }

    for (;;)
    {
        if (DOS_ADDR_VALID(pstPthreadMsg))
        {
            pstPthreadMsg->ulLastTime = time(NULL);
        }

        pthread_mutex_lock(&g_mutexESLEventQueue);
        stTimeout.tv_sec = time(0) + 1;
        stTimeout.tv_nsec = 0;
        pthread_cond_timedwait(&g_condESLEventQueue, &g_mutexESLEventQueue, &stTimeout);
        pthread_mutex_unlock(&g_mutexESLEventQueue);

        while (1)
        {
            if (DLL_Count(&g_stESLEventQueue) <= 0)
            {
                break;
            }

            pthread_mutex_lock(&g_mutexESLEventQueue);
            pstListNode = dll_fetch(&g_stESLEventQueue);
            pthread_mutex_unlock(&g_mutexESLEventQueue);
            if (DOS_ADDR_INVALID(pstListNode))
            {
                DOS_ASSERT(0);
                break;
            }

            if (DOS_ADDR_INVALID(pstListNode->pHandle))
            {
                DOS_ASSERT(0);
                continue;
            }

            pstEvent = (esl_event_t*)pstListNode->pHandle;

            pstListNode->pHandle = NULL;
            DLL_Init_Node(pstListNode)
            dos_dmem_free(pstListNode);
#if 1
            /* �о���־�е�࣬���Դ���һ�� */
            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ESL), "Recv ESL event. %s(%d), Channel Name: %s"
                            , esl_event_get_header(pstEvent, "Event-Name")
                            , pstEvent->event_id
                            , esl_event_get_header(pstEvent, "Channel-Name"));
#endif
            sc_esl_event_process(pstEvent);

            esl_event_destroy(&pstEvent);
            pstEvent = NULL;
        }
    }
}

/**
 * �̺߳���������esl�¼�
 */
VOID *sc_esl_recv_runtime(VOID *ptr)
{
    U32         ulRet;
    BOOL        bFirstConn = DOS_TRUE;
    DLL_NODE_S  *pstDLLNode = NULL;
    esl_event_t *pstEvent   = NULL;
    //SC_PTHREAD_MSG_ST   *pstPthreadMsg = NULL;

    g_blESLEventRunning = DOS_TRUE;

#if 0
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    pstPthreadMsg = sc_pthread_cb_alloc();
    if (DOS_ADDR_VALID(pstPthreadMsg))
    {
        pstPthreadMsg->ulPthID = pthread_self();
        pstPthreadMsg->func = sc_esl_recv_runtime;
        pstPthreadMsg->pParam = ptr;
        dos_strcpy(pstPthreadMsg->szName, "sc_esl_recv_runtime");
    }
#endif

    for (;;)
    {
        /* ����˳���־�����ϣ���׼���˳��� */
        if (!g_blESLEventRunning)
        {
            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_ESL), "%s", "Event process exit flag has been set. the task will be exit.");
            break;
        }

        /*
           * ��������Ƿ�����
           * ������Ӳ���������׼������
           **/
        if (!g_stESLRecvHandle.connected)
        {
            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_ESL), "ELS for event connection has been down, re-connect. %s", __FUNCTION__);
            g_stESLRecvHandle.event_lock = 1;
            ulRet = esl_connect(&g_stESLRecvHandle, "127.0.0.1", 8021, NULL, "ClueCon");
            if (ESL_SUCCESS != ulRet)
            {
                sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_ESL), "ELS for event re-connect fail, return code:%d, Msg:%s. Will be retry after 1 second.", ulRet, g_stESLRecvHandle.err);

                sleep(1);
                continue;
            }

            esl_global_set_default_logger(ESL_LOG_LEVEL_WARNING);
            esl_events(&g_stESLRecvHandle, ESL_EVENT_TYPE_PLAIN, SC_ESL_EVENT_LIST);

            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_ESL), "ELS for event connect Back to Normal. %s", __FUNCTION__);
        }

        //if (DOS_ADDR_VALID(pstPthreadMsg))
        //{
        //    pstPthreadMsg->ulLastTime = time(NULL);
        //}

        if (bFirstConn)
        {
            bFirstConn = DOS_FALSE;
        }

        ulRet = esl_recv_event(&g_stESLRecvHandle, 1, NULL);
        if (ESL_FAIL == ulRet)
        {
            DOS_ASSERT(0);

            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_ESL), "%s", "ESL Recv event fail, continue.");

            esl_disconnect(&g_stESLRecvHandle);
            continue;
        }

        pstEvent = g_stESLRecvHandle.last_ievent;
        if (DOS_ADDR_INVALID(pstEvent))
        {
            DOS_ASSERT(0);

            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_ESL), "%s", "ESL get event fail, continue.");
            continue;
        }

        sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ESL), "recv esl event %s(%d)."
                        , esl_event_get_header(pstEvent, "Event-Name")
                        , pstEvent->event_id);

        pstDLLNode = (DLL_NODE_S *)dos_dmem_alloc(sizeof(DLL_NODE_S));
        if (DOS_ADDR_INVALID(pstDLLNode))
        {
            DOS_ASSERT(0);

            sc_log(DOS_FALSE, SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_ESL), "ESL recv thread recv event %s(%d). Alloc memory fail. Drop"
                            , esl_event_get_header(pstEvent, "Event-Name")
                            , pstEvent->event_id);

            continue;
        }

        pthread_mutex_lock(&g_mutexESLEventQueue);

        DLL_Init_Node(pstDLLNode);
        pstDLLNode->pHandle = NULL;
        esl_event_dup((esl_event_t **)(&pstDLLNode->pHandle), pstEvent);

        DLL_Add(&g_stESLEventQueue, pstDLLNode);

        pthread_cond_signal(&g_condESLEventQueue);
        pthread_mutex_unlock(&g_mutexESLEventQueue);

    }

    return NULL;
}

/**
 * ��ʼ��ESLģ��
 *
 * @return �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 */
U32 sc_esl_init()
{
    DLL_Init(&g_stESLEventQueue);
    dos_memzero(&g_stESLSendHandle, sizeof(esl_handle_t));
    dos_memzero(&g_stESLRecvHandle, sizeof(esl_handle_t));

    return DOS_SUCC;
}

/**
 * ����ESL�¼���������߳�
 *
 * @return �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 */
U32 sc_esl_start()
{
    if (pthread_create(&g_stESLEventRecv, NULL, sc_esl_process_runtime, NULL) < 0)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if (pthread_create(&g_stESLEventRecv, NULL, sc_esl_recv_runtime, NULL) < 0)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/**
 * ֹͣESL�¼���������߳�
 *
 * @return �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 *
 * @note �ú������ز���ȷ�������̶߳�������
 */
U32 sc_esl_stop()
{
    g_blESLEventRunning = DOS_FALSE;

    return DOS_SUCC;
}

#ifdef __cplusplus
}
#endif /* End of __cplusplus */

