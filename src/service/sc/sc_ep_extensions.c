/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  �ļ���: sc_ep_extensions.c
 *
 *  ����ʱ��: 2015��6��18��15:25:18
 *  ��    ��: Larry
 *  ��    ��: ����FS���ķ������ĸ����¼�
 *  �޸���ʷ:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>
#include <esl.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <bs_pub.h>
#include <libcurl/curl.h>
#include "sc_def.h"
#include "sc_debug.h"
#include "sc_acd_def.h"
#include "sc_ep.h"
#include "sc_acd_def.h"

#define SC_EXT_EVENT_LIST "custom sofia::register sofia::unregister"


SC_EP_HANDLE_ST  *g_pstExtMngtHangle  = NULL;
DLL_S            g_stExtMngtMsg;
pthread_mutex_t  g_mutexExtMngtMsg = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t   g_condExtMngtMsg  = PTHREAD_COND_INITIALIZER;
pthread_t        g_pthExtMngtProcTask;

extern DB_HANDLE_ST         *g_pstSCDBHandle;

U32 sc_ep_update_sip_IP(U32 ulPublicIP, U32 ulPrivateIP, U32 ulSipID)
{
    S8 szSQL[512] = { 0 };

    dos_snprintf(szSQL, sizeof(szSQL), "UPDATE tbl_sip SET public_net=%d, private_net=%d WHERE id=%u", ulPublicIP, ulPrivateIP, ulSipID);

    return db_query(g_pstSCDBHandle, szSQL, NULL, NULL, NULL);
}

VOID* sc_ep_ext_mgnt(VOID *ptr)
{
    struct timespec     stTimeout;
    DLL_NODE_S          *pstListNode = NULL;
    esl_event_t         *pstEvent    = NULL;
    S8                  *pUserID     = NULL;
    S8                  *pVal        = NULL;
    S8                  szPrivateIP[17] = {0};
    U32                 ulPublicIP   = 0;
    U32                 ulPrivateIP  = 0;
    U32                 ulSipID      = 0;
    U32                 ulResult     = 0;
    SC_STATUS_TYPE_EN   enStatus;

    for (;;)
    {
        pthread_mutex_lock(&g_mutexExtMngtMsg);
        stTimeout.tv_sec = time(0) + 1;
        stTimeout.tv_nsec = 0;
        pthread_cond_timedwait(&g_condExtMngtMsg, &g_mutexExtMngtMsg, &stTimeout);
        pthread_mutex_unlock(&g_mutexExtMngtMsg);

        while (1)
        {
            if (DLL_Count(&g_stExtMngtMsg) <= 0)
            {
                break;
            }

            pthread_mutex_lock(&g_mutexExtMngtMsg);

            pstListNode = dll_fetch(&g_stExtMngtMsg);
            if (DOS_ADDR_INVALID(pstListNode))
            {
                DOS_ASSERT(0);

                pthread_mutex_unlock(&g_mutexExtMngtMsg);
                continue;
            }

            pthread_mutex_unlock(&g_mutexExtMngtMsg);

            pstEvent = (esl_event_t *)pstListNode->pHandle;
            pstListNode->pHandle = NULL;
            if (pstEvent)
            {
                /* TODO ά��ACDģ������ϯ����Ӧ��SIP�ֻ���״̬ */
                pUserID = esl_event_get_header(pstEvent, "username");
                if (esl_strlen_zero(pUserID))
                {
                     sc_logr_debug(SC_ACD, "%s", "Not get userid");

                     goto end;
                }

                /* ά��SIP�ֻ���״̬ */
                pVal = esl_event_get_header(pstEvent, "Event-Subclass");
                if (esl_strlen_zero(pVal))
                {
                     sc_logr_debug(SC_ACD, "%s", "Not get userid");

                     goto end;
                }

                if (!dos_strstr(pVal, "unregister"))
                {
                    /* register */
                    enStatus = SC_STATUS_TYPE_REGISTER;
                }
                else
                {
                    /* unregister */
                    enStatus = SC_STATUS_TYPE_UNREGISTER;
                }

                ulResult = sc_ep_update_sip_status(pUserID, enStatus, &ulSipID);
                if (ulResult != DOS_SUCC)
                {
                    sc_logr_debug(SC_ACD, "%s", "update sip status fail");

                    goto end;
                }

                /* ��SIP��IP��ַ�浽��tbl_sip�� */
                pVal = esl_event_get_header(pstEvent, "network-ip");
                if (esl_strlen_zero(pVal))
                {
                    sc_logr_debug(SC_ACD, "%s", "Not get network-ip");
                    ulPublicIP = 0;
                }
                else
                {
                    inet_pton(AF_INET, pVal, (VOID *)(&ulPublicIP));
                }

                pVal = esl_event_get_header(pstEvent, "contact");
                if (esl_strlen_zero(pVal))
                {
                    sc_logr_debug(SC_ACD, "%s", "Not get contact");
                    ulPrivateIP = 0;
                }
                else
                {
                    if (1 == dos_sscanf(pVal, "%*[^@]@%[^:]", szPrivateIP))
                    {
                        inet_pton(AF_INET, szPrivateIP, (VOID *)(&ulPrivateIP));
                    }
                    else
                    {
                        sc_logr_debug(SC_ACD, "%s", "Not get private IP from contact");
                        ulPrivateIP = 0;
                    }
                }

                ulResult = sc_ep_update_sip_IP(ulPublicIP, ulPrivateIP, ulSipID);
                if(DB_ERR_SUCC != ulResult)
                {
                    sc_logr_debug(SC_ACD, "%s", "update sip db fail, userid is %s", pUserID);
                }
end:
                esl_event_destroy(&pstEvent);
                pstEvent = NULL;
            }
            dos_dmem_free(pstListNode);
        }
    }
}


/**
 * ����: VOID* sc_ep_runtime(VOID *ptr)
 * ����: ESL�¼������߳�������
 * ����:
 * ����ֵ:
 */
VOID* sc_ep_ext_runtime(VOID *ptr)
{
    U32                  ulRet = ESL_FAIL;
    DLL_NODE_S           *pstDLLNode = NULL;
    // �жϵ�һ�������Ƿ�ɹ�
    static BOOL bFirstConnSucc = DOS_FALSE;

    for (;;)
    {
        /* ����˳���־�����ϣ���׼���˳��� */
        if (g_pstExtMngtHangle->blIsWaitingExit)
        {
            sc_logr_notice(SC_ESL, "%s", "Event process exit flag has been set. the task will be exit.");
            break;
        }

        /*
         * ��������Ƿ�����
         * ������Ӳ���������׼������
         **/
        if (!g_pstExtMngtHangle->blIsESLRunning)
        {
            sc_logr_notice(SC_ESL, "%s", "ELS for event connection has been down, re-connect.");
            g_pstExtMngtHangle->stRecvHandle.event_lock = 1;
            ulRet = esl_connect(&g_pstExtMngtHangle->stRecvHandle, "127.0.0.1", 8021, NULL, "ClueCon");
            if (ESL_SUCCESS != ulRet)
            {
                esl_disconnect(&g_pstExtMngtHangle->stRecvHandle);
                sc_logr_notice(SC_ESL, "ELS for event re-connect fail, return code:%d, Msg:%s. Will be retry after 1 second.", ulRet, g_pstExtMngtHangle->stRecvHandle.err);

                sleep(1);
                continue;
            }

            g_pstExtMngtHangle->blIsESLRunning = DOS_TRUE;
            g_pstExtMngtHangle->ulESLDebugLevel = ESL_LOG_LEVEL_INFO;
            esl_global_set_default_logger(g_pstExtMngtHangle->ulESLDebugLevel);
            esl_events(&g_pstExtMngtHangle->stRecvHandle, ESL_EVENT_TYPE_PLAIN, SC_EXT_EVENT_LIST);

            sc_logr_notice(SC_ESL, "%s", "ELS for event connect Back to Normal.");
        }

        if (!bFirstConnSucc)
        {
            bFirstConnSucc = DOS_TRUE;
            /* ��Ҫ�����⴦����ѯ���еķֻ� */
        }

        ulRet = esl_recv_event(&g_pstExtMngtHangle->stRecvHandle, 1, NULL);
        if (ESL_FAIL == ulRet)
        {
            sc_logr_info(SC_ESL, "%s", "ESL Recv event fail, continue.");
            g_pstExtMngtHangle->blIsESLRunning = DOS_FALSE;
            continue;
        }

        esl_event_t *pstEvent = g_pstExtMngtHangle->stRecvHandle.last_ievent;
        if (DOS_ADDR_INVALID(pstEvent))
        {
            sc_logr_info(SC_ESL, "%s", "ESL get event fail, continue.");
            g_pstExtMngtHangle->blIsESLRunning = DOS_FALSE;
            continue;
        }

        pstDLLNode = dos_dmem_alloc(sizeof(DLL_NODE_S));
        if (DOS_ADDR_INVALID(pstDLLNode))
        {
            DOS_ASSERT(0);

            continue;
        }

        pthread_mutex_lock(&g_mutexExtMngtMsg);

        DLL_Init_Node(pstDLLNode);
        pstDLLNode->pHandle = NULL;
        esl_event_dup((esl_event_t **)(&pstDLLNode->pHandle), pstEvent);
        DLL_Add(&g_stExtMngtMsg, pstDLLNode);

        pthread_cond_signal(&g_condExtMngtMsg);
        pthread_mutex_unlock(&g_mutexExtMngtMsg);
    }

    /* @TODO �ͷ���Դ */
    return NULL;
}

/* ��ʼ���¼�����ģ�� */
U32 sc_ep_ext_init()
{    SC_TRACE_IN(0, 0, 0, 0);

    g_pstExtMngtHangle = dos_dmem_alloc(sizeof(SC_EP_HANDLE_ST));
    if (DOS_ADDR_INVALID(g_pstExtMngtHangle))
    {
        DOS_ASSERT(0);

        goto init_fail;
    }

    dos_memzero(g_pstExtMngtHangle, sizeof(SC_EP_HANDLE_ST));
    g_pstExtMngtHangle->blIsESLRunning = DOS_FALSE;
    g_pstExtMngtHangle->blIsWaitingExit = DOS_FALSE;

    DLL_Init(&g_stExtMngtMsg);

    SC_TRACE_OUT();
    return DOS_SUCC;

init_fail:

    return DOS_FAIL;
}

/* �����¼�����ģ�� */
U32 sc_ep_ext_start()
{
    SC_TRACE_IN(0, 0, 0, 0);

    if (pthread_create(&g_pthExtMngtProcTask, NULL, sc_ep_ext_mgnt, NULL) < 0)
    {
        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    if (pthread_create(&g_pstExtMngtHangle->pthID, NULL, sc_ep_ext_runtime, NULL) < 0)
    {
        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    SC_TRACE_OUT();
    return DOS_SUCC;
}

/* ֹͣ�¼�����ģ�� */
U32 sc_ep_ext_shutdown()
{
    SC_TRACE_IN(0, 0, 0, 0);

    g_pstExtMngtHangle->blIsWaitingExit = DOS_TRUE;

    SC_TRACE_OUT();
    return DOS_SUCC;
}


#ifdef __cplusplus
}
#endif /* __cplusplus */


