/**
 *            (C) Copyright 2014, ����Ѷͨ . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  hb_server.c
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: ���ļ�����������������ز���������
 *     History:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* include dos header files */
#include <dos.h>

#if INCLUDE_BH_SERVER

/* include system header file */
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/sem.h>

/* include private header file */
#include "heartbeat.h"

/* define global varables */
/* �Եȴ��˳���־������ */
static pthread_mutex_t      g_HBMutex = PTHREAD_MUTEX_INITIALIZER;

/* �߳�id */
static pthread_t            g_pthIDHBTask;

/* ���ӽ��̿��ƿ� */
static PROCESS_INFO_ST      *g_pstProcessInfo[DOS_PROCESS_MAX_NUM];

/* �Զ˵�ַ */
static struct sockaddr_un   g_stServerAddr;  /* �Զ˵�ַ */

/* ����socket */
static S32                  g_lSocket = -1;

/* �ȴ��˳���־ */
static S32                  g_lHBTaskWaitingExit = 0;

/* �������ݻ��� */
static S8                   g_szRecvBuf[MAX_BUFF_LENGTH];

/* �����ⲿ�ͻ�����Ϣ�߳� */
static pthread_t            g_pthSendMsgTask;


/* �ȴ��ⲿ��Ϣ���ź��� */
sem_t g_stSem;

/**
 * ����: hb_get_max_timeout()
 * ����: ��ȡ���ʱʱ��
 * ����:
 * ����ֵ
 */
S32 hb_get_max_timeout()
{
    U32 ulHBSendInterval = 0, ulHBMaxFailCnt;
    ulHBSendInterval = config_hh_get_send_interval();
    if (ulHBSendInterval < DEFAULT_HB_INTERVAL_MIN
        || ulHBSendInterval > DEFAULT_HB_INTERVAL_MAX)
    {
        ulHBSendInterval = DEFAULT_HB_INTERVAL_MIN;
    }

    ulHBMaxFailCnt = config_hb_get_max_fail_cnt();
    if (ulHBMaxFailCnt < DEFAULT_HB_FAIL_CNT_MIN
        || ulHBMaxFailCnt < DEFAULT_HB_FAIL_CNT_MAX)
    {
        ulHBMaxFailCnt = DEFAULT_HB_FAIL_CNT_MIN;
    }

    return ulHBSendInterval * ulHBMaxFailCnt;

}

VOID hb_recv_timeout(U64 uLParam)
{
    PROCESS_INFO_ST *pstProcessInfo = NULL;

    if (uLParam >= DOS_PROCESS_MAX_NUM)
    {
        hb_logr_warning("Heartbeat recv timeout, but with an error param \"%lu\".", uLParam);
        DOS_ASSERT(0);
        return;
    }

    pstProcessInfo = g_pstProcessInfo[uLParam];
    pstProcessInfo->ulHBFailCnt++;
    pstProcessInfo->hTmrHBTimeout = NULL;

    dos_printf("Process lost.");
}

/**
 * ������S32 bh_process_list(U32 ulIndex, S32 argc, S8 **argv)
 * ���ܣ������лص���������ӡ�ͻ����б�
 * ������
 *      U32 ulIndex���ͻ��˱�ʾ
 *      S32 argc����������
 *      S8 **argv��������
 * ����ֵ��
 *      �ɹ�����0��ʧ�ܷ��أ�1
 */
S32 bh_process_list(U32 ulIndex, S32 argc, S8 **argv)
{
    U32 i, ulTotal, ulActive;
    S8 szBuff[258];

    snprintf(szBuff, sizeof(szBuff)
                    , "\r\n%-32s%-16s%-10s%-10s%-10s\r\n"
                    , "Name"
                    , "Version"
                    , "Status"
                    , "Total HB"
                    , "Fail HB");
    cli_out_string(ulIndex, szBuff);

    ulTotal = 0;
    ulActive = 0;
    for (i=0; i<DOS_PROCESS_MAX_NUM; i++)
    {
        if (g_pstProcessInfo[i]->ulVilad)
        {
            snprintf(szBuff, sizeof(szBuff)
                                , "%-32s%-16s%-10s%-10d%-10d\r\n"
                                , g_pstProcessInfo[i]->szProcessName
                                , g_pstProcessInfo[i]->szProcessVersion
                                , g_pstProcessInfo[i]->ulActive ? "Active" : "Inactive"
                                , g_pstProcessInfo[i]->ulHBCnt
                                , g_pstProcessInfo[i]->ulHBFailCnt);
            cli_out_string(ulIndex, szBuff);
            ulTotal++;
        }
    }

    snprintf(szBuff, sizeof(szBuff)
                        , "-----------------------------------------------------------------------------------------\r\n");
    cli_out_string(ulIndex, szBuff);
    snprintf(szBuff, sizeof(szBuff)
                            , "Total:%d,   Active:%d \r\n"
                            , ulTotal, ulActive);
    cli_out_string(ulIndex, szBuff);

    return 0;
}


/**
 * ������VOID hb_process_lost(PROCESS_INFO_ST *pstProcessInfo)
 * ���ܣ����ֿͻ��������Ͽ�
 * ������
 *      PROCESS_INFO_ST *pstProcessInfo: ���̿��ƿ�
 * ����ֵ��
 *      �ɹ�����0��ʧ�ܷ��أ�1
 */
VOID hb_process_lost(PROCESS_INFO_ST *pstProcessInfo)
{
    S32 lTreatmentNo;

    if (!pstProcessInfo)
    {
        DOS_ASSERT(0);
        return;
    }

    lTreatmentNo = config_hb_get_treatment();
    pstProcessInfo->ulActive = DOS_FALSE;
    if (lTreatmentNo < 0)
    {
        lTreatmentNo = 0;
    }

    hb_logr_error("Process \"%s\" lost. Treate as %d", pstProcessInfo->szProcessName, lTreatmentNo);
}


/**
 * ������PROCESS_INFO_ST *hb_alloc_process()
 * ���ܣ�������̿��ƿ麯��
 * ������
 * ����ֵ���ɹ����ؽ��̿��ƿ飬ʧ�ܷ��أ�1
 */
PROCESS_INFO_ST *hb_alloc_process()
{
    S32 i;

    for (i=0; i<DOS_PROCESS_MAX_NUM; i++)
    {
        if (!g_pstProcessInfo[i]->ulVilad)
        {
            break;
        }
    }

    if (DOS_PROCESS_MAX_NUM <= i)
    {
        DOS_ASSERT(0);
        return NULL;
    }

    return g_pstProcessInfo[i];
}

/**
 * ������PROCESS_INFO_ST *hb_alloc_process()
 * ���ܣ�������̿��ƿ麯��
 * ������
 * ����ֵ���ɹ����ؽ��̿��ƿ飬ʧ�ܷ��أ�1
 */
PROCESS_INFO_ST *hb_find_process(HEARTBEAT_DATA_ST *pstHBData)
{
    S32 i;

    for (i=0; i<DOS_PROCESS_MAX_NUM; i++)
    {
        if (dos_strcmp(pstHBData->szProcessName, g_pstProcessInfo[i]->szProcessName) == 0
                && dos_strcmp(pstHBData->szProcessVersion, g_pstProcessInfo[i]->szProcessVersion) == 0)
        {
            return g_pstProcessInfo[i];
        }
    }

    return NULL;
}

/**
 *  ������S32 hb_mod_msg_proc(VOID *pMsg, U32 uiLen, struct sockaddr_un *pstServerAddr,S32 lSocket)
 *  ���ܣ��ú������յ��ļ�ؽ��̵����ݽ��зַ�
 *  ������
 *      VOID *pMsg����Ϣ����
 *      U32 uiLen����Ϣ����
 *      struct sockaddr_un *pstServerAddr: �Զ˵�ַ
 *      S32 lSocket������socket
 *  ����ֵ���ɹ�����0.ʧ�ܷ��أ�1
 */
S32 hb_msg_proc(VOID *pMsg, U32 uiLen, struct sockaddr_un *pstServerAddr, S32 lAddrLen, S32 lSocket)
{
    HEARTBEAT_DATA_ST stHBData;
    PROCESS_INFO_ST *pstProcessInfo;
    S32 lResult;

    if (!pMsg || uiLen < sizeof(HEARTBEAT_DATA_ST))
    {
        DOS_ASSERT(0);
        hb_logr_warning("%s", "Heartbeat server recv invalid msg.");
        return -1;
    }

    if (!pstServerAddr)
    {
        DOS_ASSERT(0);
        hb_logr_warning("%s", "Heartbeat server recv msg with unknow address.");
        return -1;
    }

    memcpy((VOID *)&stHBData, pMsg, sizeof(HEARTBEAT_DATA_ST));

    /* ���ҽ���, ���û�У���Ҫ���һ�� */
    pstProcessInfo = hb_find_process(&stHBData);
    if (!pstProcessInfo)
    {
        hb_logr_info("%s", "New process register, alloc ccb.");
        pstProcessInfo = hb_alloc_process(&stHBData);

        pstProcessInfo->ulVilad = DOS_TRUE;
        pstProcessInfo->ulActive = DOS_TRUE;
        pstProcessInfo->ulHBCnt = 0;
        pstProcessInfo->ulStatus = PROCESS_HB_INIT;
        pstProcessInfo->hTmrHBTimeout = NULL;


        dos_strncpy(pstProcessInfo->szProcessName, stHBData.szProcessName, sizeof(pstProcessInfo->szProcessName));
        pstProcessInfo->szProcessName[sizeof(pstProcessInfo->szProcessName) - 1] = '\0';

        dos_strncpy(pstProcessInfo->szProcessVersion, stHBData.szProcessVersion, sizeof(pstProcessInfo->szProcessVersion));
        pstProcessInfo->szProcessVersion[sizeof(pstProcessInfo->szProcessVersion) -1] = '\0';
    }

    if (!pstProcessInfo)
    {
        DOS_ASSERT(0);
        hb_logr_warning("%s", "Cannot alloc process info block.");
        return -1;
    }

    /* �п��ܶԷ������ˣ�����Ҫ����һ�� */
    pstProcessInfo->lSocket = lSocket;
    dos_memcpy(&pstProcessInfo->stPeerAddr, pstServerAddr, sizeof(struct sockaddr_un));
    pstProcessInfo->ulPeerAddrLen = lAddrLen;

    hb_logr_debug("Heartbeat server recv msg. Cmd:%d, Length:%d, Process name:%s, Process version:%s, ID:%d, %p"
                    , stHBData.ulCommand, uiLen
                    , pstProcessInfo->szProcessName
                    , pstProcessInfo->szProcessVersion
                    , pstProcessInfo->ulProcessCBNo
                    , pstProcessInfo);

    /* �ַ���Ϣ */
    switch (stHBData.ulCommand)
    {
        case HEARTBEAT_DATA_REG:
            lResult = hb_reg_proc(pstProcessInfo);
            break;
        case HEARTBEAT_DATA_REG_RESPONSE:
            DOS_ASSERT(0);
            lResult = -1;
            break;
        case HEARTBEAT_DATA_UNREG:
            lResult = hb_unreg_proc(pstProcessInfo);
            break;
        case HEARTBEAT_DATA_UNREG_RESPONSE:
            DOS_ASSERT(0);
            lResult = -1;
            break;
        case HEARTBEAT_DATA_HB:
            lResult = hb_heartbeat_proc(pstProcessInfo);
            break;
        case HEARTBEAT_WARNING_SEND:
            lResult = hb_recv_external_warning(pMsg);
            break;
        case HEARTBEAT_WARNING_SEND_RESPONSE:
            DOS_ASSERT(0);
            break;
        default:
            DOS_ASSERT(0);
            lResult = -1;
            break;
    }

    return lResult;
}



/**
 *  ������VOID *heartbeat_task(VOID *ptr)
 *  ���ܣ�����ס�̺߳���
 *  ������
 *  ����ֵ���ɹ�����0.ʧ�ܷ��أ�1
 */
VOID *heartbeat_task(VOID *ptr)
{
    S32 lRet, lMaxFd;
    fd_set stFdset;
    struct timeval stTimeout={1, 0};
    struct sockaddr_un stServerAddr;
    socklen_t lAddrLen;

    if (g_lSocket < 0)
    {
        DOS_ASSERT(0);
        pthread_exit(0);
    }

    while (1)
    {
        FD_ZERO(&stFdset);
        FD_SET(g_lSocket, &stFdset);
        lMaxFd = g_lSocket + 1;
        stTimeout.tv_sec = 1;
        stTimeout.tv_usec = 0;

        lRet = select(lMaxFd, &stFdset, NULL, NULL, &stTimeout);
        if (-1 == lRet)
        {
            perror("Error happened when select return.");
            g_lSocket = -1;
            hb_logr_warning("%s", "Exception occurred in heartbeat client tast.");
            break;
        }
        else if (0 == lRet)
        {
            continue;
        }

        g_szRecvBuf[0] = '\0';
        lAddrLen = sizeof(stServerAddr);
        lRet = recvfrom(g_lSocket
                , g_szRecvBuf
                , sizeof(g_szRecvBuf)
                , 0
                , (struct sockaddr *)&stServerAddr
                , &lAddrLen);
        if (lRet < 0)
        {
            g_lSocket = -1;
            DOS_ASSERT(0);
            break;
        }

        hb_msg_proc(g_szRecvBuf, lRet, &stServerAddr, lAddrLen, g_lSocket);
    }

    DOS_ASSERT(0);
    hb_logr_warning("%s", "Heartbeat server will be exited. The recv socket failed.s");

    return 0;
}


S32 hb_read_mod_config()
{
    return 0;
}

/**
 *  ������S32 heartbeat_init()
 *  ���ܣ���ʼ������ģ��
 *  ������
 *  ����ֵ���ɹ�����0.ʧ�ܷ��أ�1
 */
S32 heartbeat_init()
{
    U32 i;
    S32 lRet;
    g_lHBTaskWaitingExit = 0;
    S8 szBuffSockPath[256] = { 0 };
    S8 szBuffCMD[256];
    PROCESS_INFO_ST *pstProcessMem = NULL;

    /* ��ʼ�����̿��ƿ� */
    pstProcessMem = (PROCESS_INFO_ST *)dos_smem_alloc(sizeof(PROCESS_INFO_ST) * DOS_PROCESS_MAX_NUM);
    dos_memzero((VOID*)pstProcessMem, sizeof(PROCESS_INFO_ST) * DOS_PROCESS_MAX_NUM);
    for (i=0; i<DOS_PROCESS_MAX_NUM; i++)
    {
        g_pstProcessInfo[i] = pstProcessMem + i;
        g_pstProcessInfo[i]->ulVilad = DOS_FALSE;
        g_pstProcessInfo[i]->ulActive = DOS_FALSE;
        g_pstProcessInfo[i]->ulProcessCBNo = i;
        g_pstProcessInfo[i]->ulHBCnt = 0;
        g_pstProcessInfo[i]->ulHBFailCnt = 0;
        g_pstProcessInfo[i]->ulPeerAddrLen = 0;
        g_pstProcessInfo[i]->ulStatus = PROCESS_HB_BUTT;
        g_pstProcessInfo[i]->hTmrHBTimeout = NULL;
    }

    if (DOS_SUCC == config_hb_init())
    {
        S32 ulRes;

        for (i = 0; i < DOS_PROCESS_MAX_NUM; i++)
        {
            ulRes = config_hb_get_process_list(i
                    , g_pstProcessInfo[i]->szProcessName
                    , sizeof(g_pstProcessInfo[i]->szProcessName)
                    , g_pstProcessInfo[i]->szProcessVersion
                    , sizeof(g_pstProcessInfo[i]->szProcessVersion));
            if (ulRes < 0)
            {
                g_pstProcessInfo[i]->szProcessName[0] = '\0';
                g_pstProcessInfo[i]->szProcessVersion[0] = '\0';
            }
            else
            {
                g_pstProcessInfo[i]->ulVilad = DOS_TRUE;
            }
        }

        config_hb_deinit();
    }

    /* ��ʼ��socket */
    g_lSocket = -1;
    g_lSocket = socket (AF_UNIX, SOCK_DGRAM, 0);
    if (g_lSocket < 0)
    {
        perror ("create socket failed");
        return -1;
    }

    snprintf(szBuffCMD, sizeof(szBuffCMD), "mkdir -p %s/var/run/socket", dos_get_sys_root_path());
    system(szBuffCMD);
    snprintf(szBuffSockPath, sizeof(szBuffSockPath), "%s/var/run/socket/moniter.sock"
                , dos_get_sys_root_path());
    unlink(szBuffSockPath);

    dos_memzero(&g_stServerAddr, sizeof(g_stServerAddr));
    g_stServerAddr.sun_family = AF_UNIX;
    dos_strcpy(g_stServerAddr.sun_path, szBuffSockPath);
    S32 lLen = offsetof (struct sockaddr_un, sun_path) + strlen(szBuffSockPath);
    lRet = bind(g_lSocket, (struct sockaddr*)&g_stServerAddr, lLen);
    if(lRet < 0)
    {
        hb_logr_error("Cannot bind server socket.(%d)\n", errno);
        close(g_lSocket);
        return DOS_FAIL;
    }

    return 0;
}


/**
 *  ������S32 heartbead_start()
 *  ���ܣ����������߳�
 *  ������
 *  ����ֵ���ɹ�����0.ʧ�ܷ��أ�1
 */
S32 heartbeat_start()
{
    S32 iResult = 0;

    /* ��ʼ��֪ͨ��Ϣ���ź��� */
    sem_init(&g_stSem, 0, 0);

    iResult = pthread_create(&g_pthIDHBTask, NULL, heartbeat_task, NULL);
    if (iResult < 0)
    {
        return -1;
    }

    iResult = pthread_create(&g_pthSendMsgTask, NULL, hb_external_warning_proc, NULL);
    if (iResult < 0)
    {
        return -1;
    }

    //pthread_join(g_pthIDHBTask, NULL);
    return 0;
}

/**
 *  ������S32 heartbead_stop()
 *  ���ܣ�ֹͣ�����߳�
 *  ������
 *  ����ֵ���ɹ�����0.ʧ�ܷ��أ�1
 */
S32 heartbeat_stop()
{
    pthread_mutex_lock(&g_HBMutex);
    g_lHBTaskWaitingExit = 1;
    pthread_mutex_unlock(&g_HBMutex);

    pthread_join(g_pthIDHBTask, NULL);

    return 0;
}

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

