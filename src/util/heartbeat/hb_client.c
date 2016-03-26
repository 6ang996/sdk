/**
 *            (C) Copyright 2014, ����Ѷͨ . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  hb_client.c
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: ���ļ���������������صĲ�������
 *     History:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* include dos header files */
#include <dos.h>

#if INCLUDE_BH_CLIENT

/* include system header file */
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

/* include private header file */
#include "heartbeat.h"

/* define local macros */
#define MAX_BUFF_LENGTH 1024

/* define global varables */

/* �߳�������Ҫ����߳��˳���ʾ���߳̿��ƿ� */
static pthread_mutex_t g_HBMutex = PTHREAD_MUTEX_INITIALIZER;

/* �߳̿��ƿ� */
static PROCESS_INFO_ST g_stProcessInfo;

/* �߳�id */
static pthread_t       g_pthIDHBTask;

/* �̵߳ȴ��˳���ʾ */
static S32             g_lHBTaskWaitingExit = 0;

/* �߳̽���buff */
static S8              g_szRecvBuf[MAX_BUFF_LENGTH];

/* socket����״̬��ʾ */
static BOOL            g_bIsConnectOK = DOS_FALSE;

/* ������� */
static U32                  g_ulHBSendInterval = DEFAULT_HB_INTERVAL_MIN;

/* ����ʧ�ܴ��� */
static U32                  g_ulHBMaxFailCnt = DEFAULT_HB_FAIL_CNT_MIN;

PROCESS_INFO_ST *g_pstDebugProcInfo = &g_stProcessInfo;

/**
 * ������U32 hb_get_max_fail_cnt()
 * ���ܣ���ȡ�������ͼ��
 * ������
 * ����ֵ���������ͼ��
 */
U32 hb_get_max_send_interval()
{
    return g_ulHBSendInterval;
}


/**
 * ������VOID hb_heartbeat_interval_timeout(U64 uLParam)
 * ���ܣ��������ͼ����ʱ
 * ������
 *      U64 uLParam�����̿��ƿ���
 * ����ֵ��null
 */
VOID hb_heartbeat_interval_timeout(U64 uLParam)
{
    PROCESS_INFO_ST *pstProcessInfo = &g_stProcessInfo;

    pstProcessInfo->ulHBCnt++;

    hb_logr_debug("Send heartbeat to the HB server.");

    hb_send_heartbeat(pstProcessInfo);
}

/**
 * ������VOID hb_heartbeat_recv_timeout(U64 uLParam)
 * ���ܣ��������ճ�ʱ�ص�����
 * ������
 *      U64 uLParam�����̿��ƿ���
 * ����ֵ��null
 */
VOID hb_heartbeat_recv_timeout(U64 uLParam)
{
    PROCESS_INFO_ST *pstProcessInfo = NULL;

    if (uLParam >= DOS_PROCESS_MAX_NUM)
    {
        hb_logr_warning("Heartbeat recv timeout, but with an error param \"%lu\".", uLParam);
        DOS_ASSERT(0);
        return;
    }

    pstProcessInfo = &g_stProcessInfo;
    pstProcessInfo->ulHBFailCnt++;
    pstProcessInfo->hTmrRecvTimeout = NULL;

    hb_logr_debug("Heartbeat recv timeout. process \"%s\"", pstProcessInfo->szProcessName);

    if (pstProcessInfo->ulHBFailCnt >= g_ulHBMaxFailCnt)
    {
        pstProcessInfo->ulStatus = PROCESS_HB_INIT;
        hb_send_reg(pstProcessInfo);
    }
}


/**
 * ������VOID hb_reg_timeout(U64 uLProcessCB)
 * ���ܣ������ͻ���ע�ᳬʱ
 *      ע�������ڸú������������ò���������������
 * ������
 *      U64 uLProcessCB��ָ�����̿��ƿ�ñ��
 * ����ֵ��
 */
VOID hb_reg_timeout(U64 uLProcessCB)
{
    /* ��ʱ����λ */
    g_stProcessInfo.hTmrRegInterval = NULL;

    hb_logr_warning("%s", "Heartbeat client register timerout. Resend register msg.");

    /* �ط�ע�� */
    hb_send_reg(&g_stProcessInfo);
}


/**
 * ������VOID hb_set_connect_flg(BOOL bConnStatus)
 * ���ܣ�����socket����״̬��ʾ
 * ������
 *      BOOL bConnStatus������״̬��true��false
 * ����ֵ��
 */
VOID hb_set_connect_flg(BOOL bConnStatus)
{
    g_bIsConnectOK = bConnStatus;
}


/**
 *  ������S32 hb_client_msg_proc(VOID *pMsg, U32 ulLen)
 *  ���ܣ��ú������յ���ؽ�ʱ���ã���Ҫ������Ϣ���Ͷ����ݽ��зַ�
 *  ������
 *      VOID *pMsg����Ϣ����
 *      U32 uiLen����Ϣ����
 *  ����ֵ���ɹ�����0.ʧ�ܷ��أ�1
 */
S32 hb_client_msg_proc(VOID *pMsg, U32 ulLen)
{
    HEARTBEAT_DATA_ST stHBData;
    S32 lResult = -1;

    if (!pMsg || ulLen <= 0)
    {
        DOS_ASSERT(0);
        hb_logr_warning("%s", "Heartbeat client recv invalid msg.");
        return -1;
    }

    memcpy((VOID *)&stHBData, pMsg, sizeof(HEARTBEAT_DATA_ST));

    hb_logr_debug("Recv hb server msg. Name:%s, Version:%s, CMD:%d"
            , stHBData.szProcessName, stHBData.szProcessVersion, stHBData.ulCommand);

    switch (stHBData.ulCommand)
    {
        case HEARTBEAT_DATA_REG:
            DOS_ASSERT(0);
            lResult = -1;
            break;
        case HEARTBEAT_DATA_REG_RESPONSE:
            lResult = hb_reg_response_proc(&g_stProcessInfo);
            break;
        case HEARTBEAT_DATA_UNREG:
            DOS_ASSERT(0);
            lResult = -1;
            break;
        case HEARTBEAT_DATA_UNREG_RESPONSE:
            lResult = hb_unreg_response_proc(&g_stProcessInfo);
            break;
        case HEARTBEAT_DATA_HB:
            lResult = hb_heartbeat_proc(&g_stProcessInfo);
            break;
        case HEARTBEAT_WARNING_SEND:
            break;
        case HEARTBEAT_WARNING_SEND_RESPONSE:
            break;
        case HEARTBEAT_SYS_REBOOT:
            break;
        default:
            DOS_ASSERT(0);
            lResult = -1;
            break;
    }

    hb_logr_debug("HB server msg processed. Name:%s, Version:%s, CMD:%d"
            , stHBData.szProcessName, stHBData.szProcessVersion, stHBData.ulCommand);

    return lResult;
}

/**
 * ������S32 hb_client_reconn()
 * ���ܣ��ͻ����������ӷ�����
 * ������
 * ����ֵ���ɹ�����0���ǲ����أ�1
 */
S32 hb_client_reconn()
{
    S8 szBuffSockPath[256];

    dos_memzero(&g_stProcessInfo.stPeerAddr, sizeof(g_stProcessInfo.stPeerAddr));
    snprintf(szBuffSockPath, sizeof(szBuffSockPath), "%s/var/run/socket/moniter.sock", dos_get_sys_root_path());
    g_stProcessInfo.stPeerAddr.sun_family = AF_UNIX;
    dos_strcpy(g_stProcessInfo.stPeerAddr.sun_path, szBuffSockPath);
    g_stProcessInfo.ulPeerAddrLen = offsetof(struct sockaddr_un, sun_path) + strlen(szBuffSockPath);

    if (connect(g_stProcessInfo.lSocket, (struct sockaddr *)&g_stProcessInfo.stPeerAddr, g_stProcessInfo.ulPeerAddrLen) < 0)
    {
        hb_logr_info("Cannot connect to heartbeat server.(%d)", errno);
        return DOS_FAIL;
    }

    return DOS_SUCC;
}


/**
 *  ������S32 heartbeat_task()
 *  ���ܣ�����ס�̺߳���
 *  ������
 *  ����ֵ���ɹ�����0.ʧ�ܷ��أ�1
 */
VOID *hb_client_task(VOID *ptr)
{
    S32 lRet, lMaxFd;
    fd_set stFdset;
    struct timeval stTimeout={1, 0};

    if (g_stProcessInfo.lSocket < 0)
    {
        DOS_ASSERT(0);
        pthread_exit(0);
    }


    hb_logr_debug("%s", "Start to register to the heartbeat server.");

    while (1)
    {
        /* ���ӵ������� */
        if (!g_bIsConnectOK)
        {
            hb_logr_debug("%s", "Waiting to connect to the heartbeat server.");
            sleep(1);
            if (hb_client_reconn() != DOS_SUCC)
            {
                continue;
            }
            hb_logr_debug("%s", "Connect to the heartbeat server OK.");

            g_bIsConnectOK = DOS_TRUE;

            hb_send_reg(&g_stProcessInfo);
        }

        /* �������� */
        FD_ZERO(&stFdset);
        FD_SET(g_stProcessInfo.lSocket, &stFdset);
        lMaxFd = g_stProcessInfo.lSocket + 1;
        stTimeout.tv_sec = 1;
        stTimeout.tv_usec = 0;

        lRet = select(lMaxFd, &stFdset, NULL, NULL, &stTimeout);
        if (-1 == lRet)
        {
            perror("Error happened when select return.");
            g_stProcessInfo.lSocket = -1;
            hb_logr_warning("%s", "Exception occurred in heartbeat client tast.");
            break;
        }
        else if (0 == lRet)
        {
            continue;
        }

        g_szRecvBuf[0] = '\0';
        lRet = recvfrom(g_stProcessInfo.lSocket
                , g_szRecvBuf
                , sizeof(g_szRecvBuf)
                , 0
                , (struct sockaddr *)&g_stProcessInfo.stPeerAddr
                , &g_stProcessInfo.ulPeerAddrLen);
        if (lRet < 0)
        {
            g_bIsConnectOK = DOS_FALSE;
        }

        hb_client_msg_proc(g_szRecvBuf, lRet);
    }

    return 0;
}

/**
 *  ������S32 heartbeat_init()
 *  ���ܣ���ʼ������ģ��
 *  ������
 *  ����ֵ���ɹ�����0.ʧ�ܷ��أ�1
 */
S32 hb_client_init()
{
    S8 szBuffSockPath[256] = { 0 };
    S8 szBuffCMD[256] = { 0 };
    g_lHBTaskWaitingExit = 0;
    S32 lAddrLen;
    struct sockaddr_un stLocalAddr;
    struct timeval stTimeout={2, 0};

    memset((VOID*)&g_stProcessInfo, 0, sizeof(g_stProcessInfo));
    g_stProcessInfo.ulPeerAddrLen = 0;
    g_stProcessInfo.ulStatus = PROCESS_HB_INIT;
    g_stProcessInfo.hTmrRegInterval = NULL;
    strncpy(g_stProcessInfo.szProcessName, dos_get_process_name(), sizeof(g_stProcessInfo.szProcessName));
    g_stProcessInfo.szProcessName[sizeof(g_stProcessInfo.szProcessName) - 1] = '\0';
    strncpy(g_stProcessInfo.szProcessVersion, dos_get_process_version(), sizeof(g_stProcessInfo.szProcessVersion));
    g_stProcessInfo.szProcessVersion[sizeof(g_stProcessInfo.szProcessVersion) - 1] = '\0';

    g_stProcessInfo.hTmrRecvTimeout = NULL;
    g_stProcessInfo.hTmrSendInterval = NULL;
    g_stProcessInfo.hTmrRegInterval = NULL;
    g_stProcessInfo.ulHBFailCnt = 0;

    g_stProcessInfo.lSocket = -1;
    g_stProcessInfo.lSocket = socket (AF_UNIX, SOCK_DGRAM, 0);
    if (g_stProcessInfo.lSocket < 0)
    {
        perror ("create socket failed");
        return -1;
    }

    /* ��socket���ó�ʱ */
    setsockopt(g_stProcessInfo.lSocket, SOL_SOCKET, SO_SNDTIMEO,(const char*)&stTimeout,sizeof(stTimeout));

    /* ���Ŀ¼����������ھ�Ҫ���� */
    snprintf(szBuffCMD, sizeof(szBuffCMD), "mkdir -p %s/var/run/socket", dos_get_sys_root_path());
    system(szBuffCMD);

    dos_memzero(&stLocalAddr, sizeof(stLocalAddr));
    stLocalAddr.sun_family = AF_UNIX;
    snprintf(szBuffSockPath, sizeof(szBuffSockPath), "%s/var/run/socket/moniter-%s.sock"
                , dos_get_sys_root_path()
                , dos_get_process_name());
    dos_strcpy(stLocalAddr.sun_path, szBuffSockPath);
    lAddrLen = offsetof(struct sockaddr_un, sun_path) + dos_strlen(stLocalAddr.sun_path);
    unlink(szBuffSockPath);

    if(bind(g_stProcessInfo.lSocket, (struct sockaddr*)&stLocalAddr, lAddrLen) < 0)
    {
        dos_printf("Cannot bind server addr.(%d)\n", errno);
        close(g_stProcessInfo.lSocket);
        g_stProcessInfo.lSocket = -1;
        unlink(szBuffSockPath);
        return DOS_FAIL;
    }

    g_ulHBSendInterval = config_hh_get_send_interval();
    if (g_ulHBSendInterval < DEFAULT_HB_INTERVAL_MIN
        || g_ulHBSendInterval > DEFAULT_HB_INTERVAL_MAX)
    {
        g_ulHBSendInterval = DEFAULT_HB_INTERVAL_MIN;
    }

    g_ulHBMaxFailCnt = config_hb_get_max_fail_cnt();
    if (g_ulHBMaxFailCnt < DEFAULT_HB_FAIL_CNT_MIN
        || g_ulHBMaxFailCnt < DEFAULT_HB_FAIL_CNT_MAX)
    {
        g_ulHBMaxFailCnt = DEFAULT_HB_FAIL_CNT_MIN;
    }

    return 0;
}


/**
 *  ������S32 heartbead_start()
 *  ���ܣ����������߳�
 *  ������
 *  ����ֵ���ɹ�����0.ʧ�ܷ��أ�1
 */
S32 hb_client_start()
{
    S32 iResult = 0;

    iResult = pthread_create(&g_pthIDHBTask, NULL, hb_client_task, NULL);
    if (iResult < 0)
    {
        return -1;
    }

    return 0;
}


/**
 *  ������S32 heartbead_stop()
 *  ���ܣ�ֹͣ�����߳�
 *  ������
 *  ����ֵ���ɹ�����0.ʧ�ܷ��أ�1
 */
S32 hb_client_stop()
{
    pthread_mutex_lock(&g_HBMutex);
    g_lHBTaskWaitingExit = 1;
    pthread_mutex_unlock(&g_HBMutex);

    return 0;
}

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

