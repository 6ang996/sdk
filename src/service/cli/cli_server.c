/*
 *            (C) Copyright 2014, ����Ѷͨ . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  cli_server.c
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: ʵ�������з�������ع���
 *     History:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* include the dos header files */
#include <dos.h>

/* include system header file */
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <pthread.h>
#include <dos/dos_types.h>
#include <dos/dos_string.h>
#include <dos/dos_cli.h>
#include <dos.h>
#include <errno.h>

/* include provate header file */
#include "cli_server.h"
#include "../telnetd/telnetd.h"


#if INCLUDE_DEBUG_CLI_SERVER

/* ����������ӽ����� */
#define MAX_PROCESS_NUM        DOS_PROCESS_MAX_NUM

/* ��������ͻ��� */
#define MAX_SEND_BUFF          512 * 3

/* ����������󳤶� */
#define MAX_CMD_LENGTH         512

/* ��������������������������������� */
#define MAX_KEY_WORDS          16

/* ����Զ��������ǰ׺ */
static S8 g_szRemoteCMDPrefix[] = "/";

/* �����߳̾�� */
static pthread_t g_pthCliServer;

/* ������ */
static pthread_mutex_t g_mutexCliServer = PTHREAD_MUTEX_INITIALIZER;

/* server��socket */
static S32 g_lSrvSocket = -1;

/* �߳��Ƿ����ڵȴ��˳� */
static BOOL g_bCliSrvWaitingExit = 0;

/* �߳������б� */
static PROCESS_INFO_NODE_ST *g_pstProcessList[MAX_PROCESS_NUM] = { NULL };

/* �ⲿ���� */
extern COMMAND_GROUP_ST g_stCmdRootGrp[];


/**
 * �����������
 * ������S32 cli_enter_log_mode(U32 ulIndex, S32 argc, S8 **argv)
 * ���ܣ�telnet�ͻ��˽���log��ӡģʽ�������
 * ������
 *      U32 ulIndex��telnet�ͻ��˵ı��
 *      S32 argc����������
 *      S8 **argv�������б�
 * ����ֵ���ɹ�����0��ʧ�ܷ��أ�1
 */
S32 cli_enter_log_mode(U32 ulIndex, S32 argc, S8 **argv)
{
    telnet_set_mode(ulIndex, CLIENT_LEVEL_LOG);
    return 0;
}

/**
 * �����������
 * ������S32 cli_cmd_exit(U32 ulIndex, S32 argc, S8 **argv)
 * ���ܣ�telnet�ͻ��˽���log��ӡģʽ�������
 * ������
 *      U32 ulIndex��telnet�ͻ��˵ı��
 *      S32 argc����������
 *      S8 **argv�������б�
 * ����ֵ���ɹ�����0��ʧ�ܷ��أ�1
 */
S32 cli_cmd_exit(U32 ulIndex, S32 argc, S8 **argv)
{
    telnet_set_exit(ulIndex);
    return 0;
}


/**
 * �����������
 * ������S32 cli_exit_log_mode(U32 ulIndex, S32 argc, S8 **argv)
 * ���ܣ�telnet�ͻ����˳�log��ӡģʽ�������
 * ������
 *      U32 ulIndex��telnet�ͻ��˵ı��
 *      S32 argc����������
 *      S8 **argv�������б�
 * ����ֵ���ɹ�����0��ʧ�ܷ��أ�1
 */
S32 cli_exit_log_mode(U32 ulIndex, S32 argc, S8 **argv)
{
    telnet_set_mode(ulIndex, CLIENT_LEVEL_CONFIG);
    return 0;
}

/**
 * ������cli_server_process_print(U32 ulIndex, S32 argc, S8 **argv)
 * ���ܣ���ӡע���ϵĽ���
 * ������
 *      U32 ulIndex��telnet�ͻ��˵ı��
 *      S32 argc����������
 *      S8 **argv�������б�
 * ����ֵ���ɹ�����0��ʧ�ܷ��أ�1
 */
S32 cli_server_process_print(U32 ulIndex, S32 argc, S8 **argv)
{
    S8 szBuff[512];
    U32 ulLength, i;

    ulLength = snprintf(szBuff, sizeof(szBuff), "%-32s%-16s%-10s\r\n", "Name", "Version", "Status");
    telnet_send_data(ulIndex, MSG_TYPE_CMD_RESPONCE, szBuff, ulLength);

    for (i = 0; i < MAX_PROCESS_NUM; i++)
    {
        if (g_pstProcessList[i]->bVaild)
        {
            ulLength = snprintf(szBuff
                        , sizeof(szBuff)
                        , "%-32s%-16s%-10s\r\n"
                        , g_pstProcessList[i]->szProcessName
                        , g_pstProcessList[i]->szProcessVersion
                        , g_pstProcessList[i]->bActive ? "Active" : "Inactive");
            telnet_send_data(ulIndex, MSG_TYPE_CMD_RESPONCE, szBuff, ulLength);
        }
    }

    return 0;
}

/**
 * ������S32 cli_find_active_process(S8 *pszName)
 * ���ܣ���������ΪpszName�����Ҵ��ڼ���״̬���߳�
 * ������
 *      S8 *pszName���̵߳�����
 * ����ֵ���ɹ������߳̿��ƿ�ı�ţ�ʧ�ܷ��أ�1
 */
PROCESS_INFO_NODE_ST * cli_server_find_active_process(S8 *pszName)
{
    S32 i;

    if (!pszName || '\0' == pszName[0])
    {
        DOS_ASSERT(0);
        return NULL;
    }

    for (i=0; i<MAX_PROCESS_NUM; i++)
    {
        cli_logr_debug("%s,%d,%d\n", g_pstProcessList[i]->szProcessName, g_pstProcessList[i]->bVaild, g_pstProcessList[i]->bActive);
        if (g_pstProcessList[i]->bVaild
            && g_pstProcessList[i]->bActive
            && dos_stricmp(pszName, g_pstProcessList[i]->szProcessName) == 0)
        {
            return g_pstProcessList[i];
        }
    }

    return NULL;
}


/**
 * ������S32 cli_server_send_reg_rsp(PROCESS_INFO_NODE_ST *pstProcess)
 * ���ܣ�cli server ��process����ע����Ӧ
 * ������
 *      PROCESS_INFO_NODE_ST *pstProcess���߳̿��ƿ�
 * ����ֵ���ɹ�����0��ʧ�ܷ��أ�1
 */
S32 cli_server_send_reg_rsp2process(PROCESS_INFO_NODE_ST *pstProcess)
{
    CLI_MSG_HEADER *pstMsgHeader;
    MSG_UNIT_ST    *pstMsgCmd;
    U32 ulMsgLen = 0, ulRet = 0;
    U8 szSendBuff[MAX_SEND_BUFF];

    if (!pstProcess)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* �����Ϣͷ */
    pstMsgHeader = (CLI_MSG_HEADER *)szSendBuff;
    pstMsgHeader->usClientIndex = INVALID_CLIENT_INDEX;
    pstMsgHeader->usProteVersion = 0;
    ulMsgLen += sizeof(CLI_MSG_HEADER);

    /* ��鳤�� */
    if (sizeof(szSendBuff) - ulMsgLen < sizeof(MSG_UNIT_ST))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* �����Ϣ�� */
    pstMsgCmd = (MSG_UNIT_ST *)pstMsgHeader->pszData;
    pstMsgCmd->usLength = 0;
    pstMsgCmd->usType = MSG_TYPE_PROCESS_REG_RSP;
    ulMsgLen += sizeof(MSG_UNIT_ST) + pstMsgCmd->usLength;

    /* �����Ϣ���� */
    pstMsgHeader->usLength = ulMsgLen - sizeof(CLI_MSG_HEADER);

    ulRet = sendto(g_lSrvSocket, szSendBuff, ulMsgLen, 0, (struct sockaddr *)&pstProcess->stClientAddr, pstProcess->uiClientAddrLen);
    if (ulRet < 0)
    {
        pstProcess->bActive = DOS_FALSE;
    }

    return DOS_SUCC;
}

/**
 * ������S32 cli_send_cmd2process(U32 ulClientIndex, PROCESS_INFO_NODE_ST *pstProcess, S8 *pszBuffer, U32 ulLength)
 * ���ܣ�cli server ��process��������
 * ������
 *      U32 ulClientIndex��telnet�ͻ��˱��
 *      PROCESS_INFO_NODE_ST *pstProcess:���̿��ƿ�
 *      S8 *pszBuffer������buff
 *      U32 ulLength��buff����
 * ����ֵ���ɹ�����0��ʧ�ܷ��أ�1
 */
S32 cli_server_send_cmd2process(U32 ulClientIndex
                    , PROCESS_INFO_NODE_ST *pstProcess
                    , S8 *pszBuffer
                    , U32 ulLength)
{
    CLI_MSG_HEADER *pstMsgHeader;
    MSG_UNIT_ST    *pstMsgCmd;
    U8             szSendBuff[MAX_SEND_BUFF];
    U32            ulMsgLen = 0;
    S32            lRet = 0;

    if (!pstProcess || !pstProcess->bVaild)
    {
        DOS_ASSERT(0);
        cli_logr_warning("%s", "Request send data to process. But have not special a valid process.");
        return DOS_FAIL;
    }

    if (!pstProcess || !pstProcess->bActive)
    {
        DOS_ASSERT(0);
        cli_logr_warning("%s", "Request send data to process. But given an inactive process.");
        return DOS_FAIL;
    }

    if (!pszBuffer || ulLength <= 0)
    {
        DOS_ASSERT(0);
        cli_logr_warning("%s", "Request send data to process. But given an empty buffer.");
        return DOS_FAIL;
    }

    if (g_lSrvSocket < 0)
    {
        DOS_ASSERT(0);
        cli_logr_warning("%s", "Request send data to process. But it seems the process is no longer active.");
        g_lSrvSocket = -1;
        return DOS_FAIL;
    }

    /* ��鳤�� */
    pstMsgHeader = (CLI_MSG_HEADER *)szSendBuff;
    ulMsgLen += sizeof(CLI_MSG_HEADER);
    if (sizeof(szSendBuff) - ulMsgLen < sizeof(MSG_UNIT_ST))
    {
        cli_logr_warning("%s", "Request send data to process. Buffer not enough.");
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* ��д���Ԫ */
    pstMsgCmd = (MSG_UNIT_ST *)pstMsgHeader->pszData;
    pstMsgCmd->usType = MSG_TYPE_CMD;
    pstMsgCmd->usLength = ulLength;
    ulMsgLen += sizeof(MSG_UNIT_ST);
    if (sizeof(szSendBuff) - ulMsgLen < ulLength)
    {
        cli_logr_warning("%s", "Request send data to process. Buffer not enough.");
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    dos_memcpy(pstMsgCmd->pszData, pszBuffer, ulLength);
    ulMsgLen += ulLength;

    /* ��д��Ϣͷ */
    pstMsgHeader->usClientIndex = (U16)ulClientIndex;
    pstMsgHeader->usProteVersion = 0;
    pstMsgHeader->usLength = sizeof(MSG_UNIT_ST) + pstMsgCmd->usLength;

    lRet = sendto(g_lSrvSocket, szSendBuff, ulMsgLen, 0, (struct sockaddr *)&pstProcess->stClientAddr, pstProcess->uiClientAddrLen);
    if (lRet < 0)
    {
        cli_logr_warning("Request send data to process. Send fail.(%d)", errno);
        DOS_ASSERT(0);
        pstProcess->bActive = DOS_FALSE;
        return DOS_FAIL;
    }
    else
    {
        pstProcess->bActive = DOS_TRUE;
    }

    return DOS_SUCC;
}

/**
 * ������S32 cli_server_send_broadcast_cmd(U32 U32 ulIndex, , S8 *pszCmd, U32 ulBuff)
 * ���ܣ������м���Ľ��̷�������
 * ������
 *      S8 *pszCmd �� �����ַ���
 *      U32 ullength�������ַ������ȣ���Я����������
 * ����ֵ���ɹ�����0��ʧ�ܷ��أ�1
 */
S32 cli_server_send_broadcast_cmd(U32 ulIndex, S8 *pszCmd, U32 ullength)
{
    U32 i=0;

    if (!pszCmd || '\0' == pszCmd[0] || 0 == ullength)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    for (i=0; i<MAX_PROCESS_NUM; i++)
    {
        if (g_pstProcessList[i]
            && g_pstProcessList[i]->bVaild
            && g_pstProcessList[i]->bActive)
        {
            cli_server_send_cmd2process(ulIndex, g_pstProcessList[i], pszCmd, ullength);
        }
    }

    return DOS_SUCC;
}

/**
 * ������S32 cli_server_reg_proc(S8 *pszName, S8 *pszVersion, struct sockaddr_un *pstAddr, U32 ulSockLen)
 * ���ܣ��������ע����Ϣ
 * ������
 *      S8 *pszName��������
 *      S8 *pszVersion���汾��
 * ����ֵ���ɹ�����0��ʧ�ܷ��أ�1
 */
S32 cli_server_reg_proc(S8 *pszName, S8 *pszVersion, struct sockaddr_un *pstAddr, U32 ulSockLen)
{
    S32 i;

    if (!pszName || '\0' == pszName[0])
    {
        cli_logr_info("Recv register msg, but without process name.", pszName);
        return -1;
    }

    if (!pstAddr || 0 == ulSockLen)
    {
        DOS_ASSERT(0);
        return -1;
    }

    cli_logr_debug("Processing process register message. Process:%s", pszName);

    /* ��ָ���Ľ����Ƿ���ע����,���ע������Ҫ���¶Զ˵�ַ */
    for (i = 0; i < MAX_PROCESS_NUM; i++)
    {
        if (g_pstProcessList[i]->bVaild
                && dos_strcmp(g_pstProcessList[i]->szProcessName, pszName) == 0)
        {
            break;
        }
    }

    /* ���û��ע��͸�������ƿ飬����ʼ�� */
    if (i >= MAX_PROCESS_NUM)
    {
        cli_logr_info("New process registe. Name:%s", pszName);

        for (i=0; i<MAX_PROCESS_NUM; i++)
        {
            if (!g_pstProcessList[i]->bVaild)
            {
                dos_memcpy((VOID *)&g_pstProcessList[i]->stClientAddr, pstAddr, sizeof(g_pstProcessList[i]->stClientAddr));
                g_pstProcessList[i]->uiClientAddrLen = ulSockLen;
                break;
            }
        }

        /* û���ҵ������Ǿ;ܾ�ע���� */
        if (i >= MAX_PROCESS_NUM)
        {
            cli_logr_info("Cannot find a cb for the new process. Name:%s", pszName);
            return -1;
        }

        /* ��ʼ�����̿��ƿ� */
        g_pstProcessList[i]->bVaild  = DOS_TRUE;
        dos_strncpy(g_pstProcessList[i]->szProcessName, pszName, sizeof(g_pstProcessList[i]->szProcessName));
        g_pstProcessList[i]->szProcessName[sizeof(g_pstProcessList[i]->szProcessName) - 1] = '\0';
        if (pszVersion && '\0' != pszVersion[0])
        {
            dos_strncpy(g_pstProcessList[i]->szProcessVersion, pszVersion, sizeof(g_pstProcessList[i]->szProcessVersion));
            g_pstProcessList[i]->szProcessVersion[sizeof(g_pstProcessList[i]->szProcessVersion) - 1] = '\0';
        }
    }

    if (!g_pstProcessList[i]->bActive)
    {
        g_pstProcessList[i]->bActive = DOS_TRUE;
        cli_logr_info("Process \"%s\" register successfully.", pszName);
    }

    cli_server_send_reg_rsp2process(g_pstProcessList[i]);

    dos_memcpy((VOID *)&g_pstProcessList[i]->stClientAddr, pstAddr, sizeof(g_pstProcessList[i]->stClientAddr));
    g_pstProcessList[i]->uiClientAddrLen = ulSockLen;

    cli_logr_debug("Process registe message processed. Process:%s", pszName);

    return 0;
}

/**
 * ������S32 cli_server_reg_proc(S8 *pszName, S8 *pszVersion)
 * ���ܣ��������ȡ��ע����Ϣ
 * ������
 *      S8 *pszName��������
 *      S8 *pszVersion���汾��
 * ����ֵ���ɹ�����0��ʧ�ܷ��أ�1
 */
S32 cli_server_unreg_proc()
{
    return 0;
}

/**
 * ������S32 cli_server_msg_process(VOID *_pMsg, S32 ulLen, struct sockaddr_un *pstAddr, U32 ulSockLen)
 * ���ܣ�����������̷�����������Ϣ
 * ������
 *      VOID *_pMsg����Ϣ��
 *      S32 ulLen����Ϣ����
 *      struct sockaddr_un *pstAddr�����յ�ַ
 *      U32 ulSockLen�����յ�ַ����
 * ����ֵ���ɹ�����0��ʧ�ܷ��أ�1
 */
S32 cli_server_msg_process(VOID *_pMsg, S32 ulLen, struct sockaddr_un *pstAddr, U32 ulAddrLen)
{
    CLI_MSG_HEADER *pstMsgHeader = NULL;
    MSG_UNIT_ST    *pstMsgCmd = NULL;
    S8 *pszName = NULL, *pszVersion = NULL;
    BOOL bHasRegCmd = DOS_FALSE;
    U32 ulLength;
    S32 lRet = 0;

    if (!_pMsg || ulLen <= 0)
    {
        DOS_ASSERT(0);
        return -1;
    }
#if 1
    if (!pstAddr || ulAddrLen <= 0)
    {
        DOS_ASSERT(0);
        cli_logr_warning("%s(%p, %d)", "Recv msg with invalid address.", pstAddr, ulAddrLen);
        return -1;
    }
#endif
    if (ulLen < sizeof(CLI_MSG_HEADER))
    {
        DOS_ASSERT(0);
        cli_logr_warning("Recv msg with invalid length(%d).", ulLen);
        return -1;
    }

    /* ��ȡ��Ϣͷ */
    pstMsgHeader = (CLI_MSG_HEADER*)_pMsg;
    if (pstMsgHeader->usLength <= 0)
    {
        cli_logr_info("%s", "Recv msg without data.");
        return 0;
    }

    /* �����ȡ��Ϣ�� */
    ulLength = 0;
    pstMsgCmd = (MSG_UNIT_ST *)pstMsgHeader->pszData;
    while (ulLength <= pstMsgHeader->usLength
            && pstMsgCmd)
    {
        if (pstMsgHeader->usLength - ulLength < sizeof(MSG_UNIT_ST) + pstMsgCmd->usLength)
        {
            cli_logr_info("%s(%d,%d,%d)", "Recv msg, but data is not cpmplete."
                        , pstMsgHeader->usLength, ulLength, pstMsgCmd->usLength);
            break;
        }

        switch (pstMsgCmd->usType)
        {
            case MSG_TYPE_PROCESS_REG:
                bHasRegCmd = DOS_TRUE;

                cli_logr_info("%s", "Recv regisrte frame.");
                break;
            case MSG_TYPE_PROCESS_NAME:
                pszName = (S8 *)pstMsgCmd->pszData;
                pszName[pstMsgCmd->usLength - 1] = '\0';

                cli_logr_info("Recv process name frame, name:%s", pszName);
                break;
            case MSG_TYPE_PROCESS_VERSION:
                pszVersion = (S8 *)pstMsgCmd->pszData;
                pszVersion[pstMsgCmd->usLength - 1] = '\0';

                cli_logr_info("Recv process version frame, version:%s", pszVersion);
                break;
            case MSG_TYPE_PROCESS_REG_RSP:
                DOS_ASSERT(0);
                cli_logr_info("%s", "Control center template cannot support this cmd.");
                break;
            case MSG_TYPE_PROCESS_UNREG:
                lRet = cli_server_unreg_proc();
                break;
            case MSG_TYPE_PROCESS_UNREG_RESPONCE:
                DOS_ASSERT(0);
                cli_logr_info("%s", "Control center template cannot support this cmd.");
                break;
            case MSG_TYPE_CMD:
                DOS_ASSERT(0);
                cli_logr_info("%s", "Control center template cannot support this cmd.");
                break;
            case MSG_TYPE_CMD_RESPONCE:
                lRet = telnet_send_data(pstMsgHeader->usClientIndex, MSG_TYPE_CMD_RESPONCE, (S8 *)pstMsgCmd->pszData, pstMsgCmd->usLength);
                break;
            case MSG_TYPE_LOG:
                lRet = telnet_send_data(pstMsgHeader->usClientIndex, MSG_TYPE_LOG, (S8 *)pstMsgCmd->pszData, pstMsgCmd->usLength);
                break;
            default:
                DOS_ASSERT(0);
                break;
        }

        /* ��ȡ��һ�����Ԫ */
        ulLength += sizeof(MSG_UNIT_ST) + pstMsgCmd->usLength;
        if (pstMsgHeader->usLength == ulLength)
        {
            break;
        }
        pstMsgCmd = _pMsg + sizeof(CLI_MSG_HEADER) + ulLength;
    }



    /* �鿴�Ƿ���ע�ᱨ�ģ�����о���Ҫ����һ��ע�� */
    if (DOS_TRUE == bHasRegCmd
        && pszName && '\0' != pszName[0])
    {
        lRet = cli_server_reg_proc(pszName, pszVersion, pstAddr, ulAddrLen);
    }

    return lRet;
}


/**
 * ������VOID *cli_server_main_loop(VOID *p)
 * ���ܣ�cli server�̺߳���
 * ������
 * ����ֵ��
 */
VOID *cli_server_main_loop(VOID *p)
{
    struct sockaddr_un stClientAddr;
    socklen_t sockClientAddrLen;
    S8 szRecvBuf[MAX_RECV_BUFF] = { 0 };
    fd_set  stFDSet;
    S32 lRet = 0, lMaxFd = 0;
    struct timeval stTimeout={1, 0};

    pthread_mutex_lock(&g_mutexCliServer);
    g_bCliSrvWaitingExit = 0;
    pthread_mutex_unlock(&g_mutexCliServer);


    while (1)
    {
        /* �����쳣 */
        if (g_lSrvSocket < 0)
        {
            dos_log(LOG_LEVEL_ERROR, LOG_TYPE_RUNINFO, "Unexpection cli server socket status. exit.\n");
            break;
        }

        //printf("\nActive\n");

        /* �����˳� */
        pthread_mutex_lock(&g_mutexCliServer);
        if (g_bCliSrvWaitingExit)
        {
            pthread_mutex_unlock(&g_mutexCliServer);
            break;
        }
        pthread_mutex_unlock(&g_mutexCliServer);

        /* ׼��select */
        FD_ZERO(&stFDSet);
        FD_SET(g_lSrvSocket, &stFDSet);
        lMaxFd = g_lSrvSocket + 1;
        szRecvBuf[0] = '\0';
        stTimeout.tv_sec = 1;
        stTimeout.tv_usec = 0;

        lRet = select(lMaxFd, &stFDSet, NULL, NULL, &stTimeout);
        if (lRet < 0)
        {
            DOS_ASSERT(0);
            cli_logr_warning("%s", "Cli server select fail. exit.");
            break;
        }
        else if (0 == lRet)
        {
            continue;
        }

        /* �������� */
        sockClientAddrLen = sizeof(stClientAddr);
        dos_memzero((VOID *)&stClientAddr, sockClientAddrLen);
        lRet = recvfrom(g_lSrvSocket, szRecvBuf, sizeof(szRecvBuf)
                , MSG_DONTWAIT, (struct sockaddr *)&stClientAddr, &sockClientAddrLen);
        if (EAGAIN == errno || EWOULDBLOCK == errno)
        {
            continue;
        }

        /* �쳣�ˣ��˳� */
        if (lRet < 0)
        {
            cli_logr_error("", "Unexpection error while select returned. exit.\n");
            DOS_ASSERT(0);
            break;
        }

        cli_server_msg_process(szRecvBuf, lRet, &stClientAddr, sockClientAddrLen);
    }

    if (g_lSrvSocket > 0)
    {
        close(g_lSrvSocket);
        g_lSrvSocket = -1;
    }

    pthread_mutex_lock(&g_mutexCliServer);
    g_bCliSrvWaitingExit = 1;
    pthread_mutex_unlock(&g_mutexCliServer);

    cli_logr_debug("%s", "Cli server task exited.");

    return NULL;
}


/**
 * ������S32 cli_server_init()
 * ���ܣ���ʼ��cli server�߳�
 * ������
 * ����ֵ���ɹ�����0��ʧ�ܷ��أ�1
 */
S32 cli_server_init()
{
    S32 iRet, i, lLength;
    PROCESS_INFO_NODE_ST *pstProcessMem;
    struct sockaddr_un stSrvAddr;
    S8 szBuffSockPath[256] = { 0 };
    S8 szBuffCMD[256];

    /* ��ʼ�����пͻ��� */
    pstProcessMem = (PROCESS_INFO_NODE_ST *)dos_smem_alloc(sizeof(PROCESS_INFO_NODE_ST) * MAX_PROCESS_NUM);
    if (!pstProcessMem)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    dos_memzero(pstProcessMem, sizeof(PROCESS_INFO_NODE_ST) * MAX_PROCESS_NUM);
    for (i=0; i<MAX_PROCESS_NUM; i++)
    {
        g_pstProcessList[i] = pstProcessMem + i;
        g_pstProcessList[i]->ulIndex = i;
        g_pstProcessList[i]->bVaild = DOS_FALSE;
        g_pstProcessList[i]->bActive = DOS_FALSE;
    }

    /* ��ʼ������״̬ */
    g_bCliSrvWaitingExit = 0;

    /* ��ʼ��socket�ļ����·�� */
    snprintf(szBuffCMD, sizeof(szBuffCMD), "mkdir -p %s/var/run/socket", dos_get_sys_root_path());
    system(szBuffCMD);
    snprintf(szBuffSockPath, sizeof(szBuffSockPath), "%s/var/run/socket/cli-srv.sock", dos_get_sys_root_path());
    unlink(szBuffSockPath);

    /* ��ʼ��socket */
    g_lSrvSocket = socket(AF_UNIX, SOCK_DGRAM, 0);
    if(g_lSrvSocket < 0)
    {
        //dos_log(LOG_LEVEL_ERROR, LOG_TYPE_RUNINFO, "Cannot create communication socket.\n");
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* ��ʼ�����󶨷�������ַ */
    dos_memzero(&stSrvAddr, sizeof(stSrvAddr));
    stSrvAddr.sun_family=AF_UNIX;
    dos_strcpy(stSrvAddr.sun_path, szBuffSockPath);
    lLength = offsetof(struct sockaddr_un, sun_path) + strlen(szBuffSockPath);
    iRet = bind(g_lSrvSocket, (struct sockaddr*)&stSrvAddr, lLength);
    if(iRet < 0)
    {
        cli_logr_error("%s", "Cannot bind server address.\n");
        close(g_lSrvSocket);
        g_lSrvSocket = -1;
        unlink(szBuffSockPath);
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/**
 * ������S32 cli_server_init()
 * ���ܣ�����cli server�߳�
 * ������
 * ����ֵ���ɹ�����0��ʧ�ܷ��أ�1
 */
S32 cli_server_start()
{
    S32 iRet = 0;

    iRet = pthread_create(&g_pthCliServer, NULL, cli_server_main_loop, NULL);
    if (iRet != 0)
    {
        return DOS_FAIL;
    }

    //pthread_join(g_pthCliServer, NULL);

    return DOS_SUCC;
}

/**
 * ������S32 cli_server_init()
 * ���ܣ�ֹͣcli server�߳�
 * ������
 * ����ֵ���ɹ�����0��ʧ�ܷ��أ�1
 */
S32 cli_server_stop()
{
    S8 szBuffSockPath[256] = { 0 };

    pthread_mutex_lock(&g_mutexCliServer);
    g_bCliSrvWaitingExit = 1;
    pthread_mutex_unlock(&g_mutexCliServer);

    cli_logr_debug("%s", "Cli server will be stopped later.");

    pthread_join(g_pthCliServer, NULL);

    if (g_lSrvSocket > 0)
    {
        close(g_lSrvSocket);
    }

    /* ɾ��Linux socket�ļ� */
    snprintf(szBuffSockPath, sizeof(szBuffSockPath), "%s/run/socket/%s.sock", dos_get_sys_root_path(), dos_get_process_name());
    unlink(szBuffSockPath);

    return DOS_SUCC;
}


/**
 * ������COMMAND_ST *cli_server_cmd_find(COMMAND_GROUP_ST *pstCurrentGroup, S32 argc, S8 **argv)
 * ���ܣ����ұ�������
 * ������
 *      struct tagCommand *pstCurrentGroup, ��ǰ�����
 *      S32 argc, ��������
 *      S8 **argv �����б�
 * ����ֵ��
 *      �ɹ���������ڵ㣬ʧ�ܷ���null
 */
COMMAND_ST *cli_server_cmd_find(COMMAND_GROUP_ST *pstCurrentGroup, S32 argc, S8 **argv)
{
    S32 iCnt;

    if (!argv || 0 == argc)
    {
        DOS_ASSERT(0);
        return NULL;
    }

    for (iCnt=0; iCnt<pstCurrentGroup->ulSize; iCnt++)
    {
        if (dos_stricmp(pstCurrentGroup->pstCmdSet[iCnt].pszCommand, argv[0]) == 0)
        {
            break;
        }
    }

    if (iCnt >= pstCurrentGroup->ulSize)
    {
        cli_logr_debug("Cannot find the cmd %s: ", argv[0]);
        return NULL;
    }

    return &pstCurrentGroup->pstCmdSet[iCnt];
}

/**
 * ������S32 cli_server_analyse_cmd(U32 ulClientIndex, U32 ulMode, S8 *szBuffer, U32 ulLength)
 * ���ܣ�����telnet���͹���������
 * ������
 *      U32 ulClientIndex��telnet�ͻ��˱��
 *      U32 ulMode����ǰtelnet�ͻ��˴���ʲôģʽ
 *      S8 *szBuffer�������
 *      U32 ulLength������泤��
 * ����ֵ��
 *      �ɹ�����0��ʧ�ܷ��أ�1
 */
S32 cli_server_cmd_analyse(U32 ulClientIndex, U32 ulMode, S8 *szBuffer, U32 ulLength)
{
    COMMAND_GROUP_ST *pstCurrentGroup = NULL;
    COMMAND_ST *pstCurrentCmd = NULL;
    S8 *pszKeyWord[MAX_KEY_WORDS] = { 0 };
    S8 szErrorMsg[128], szCMDBak[MAX_CMD_LENGTH];
    S32 iKeyCnt = 0, iRet = 0;
    S8 *pWord = NULL, *pRealCMD = NULL;
    S32 i=0, lRet = 0;

    dos_strncpy(szCMDBak, szBuffer, MAX_CMD_LENGTH);
    szCMDBak[MAX_CMD_LENGTH - 1] = '\0';

    if (!szBuffer || !ulLength)
    {
        return -1;
    }

    logo_debug("Cli Server", "CMD", DOS_TRUE, "Control Panel Recv CMD:%s", szBuffer);

    /* ��ȡ���� */
    pWord = strtok(szBuffer, " ");
    while (pWord)
    {
        pszKeyWord[iKeyCnt] = dos_dmem_alloc(dos_strlen(pWord) + 1);
        if (!pszKeyWord[iKeyCnt])
        {
            logr_warning("%s", "Alloc fail.");
            iRet = -1;
            goto finished;
        }

        dos_strcpy(pszKeyWord[iKeyCnt], pWord);
        iKeyCnt++;
        pWord = strtok(NULL, " ");
        if (NULL == pWord)
        {
            break;
        }
    }

    if (iKeyCnt <= 0)
    {
        goto finished;
    }

    cli_logr_debug("%d, %s, %s. %s", iKeyCnt, pszKeyWord[0], pszKeyWord[1], szCMDBak);

    /* �ȴ��������� */
    if (dos_strnicmp(g_szRemoteCMDPrefix, pszKeyWord[0], dos_strlen(g_szRemoteCMDPrefix)) == 0)
    {
        /* �������� */
        /* �ͻ����Ƿ�����ȷ��ģʽ */
        if (ulMode > cli_cmdset_get_group_num())
        {
            iRet = -1;
            goto finished;
        }

        /* ��ȡ��ǰģʽ����� */
        pstCurrentGroup = &g_stCmdRootGrp[ulMode];
        if (!pstCurrentGroup)
        {
            iRet = -1;
            goto finished;
        }

        /* �������� */
        pstCurrentCmd = cli_server_cmd_find(pstCurrentGroup, iKeyCnt, pszKeyWord);
        if (!pstCurrentCmd)
        {
            snprintf(szErrorMsg, sizeof(szErrorMsg), "Cannot find the command: %s\r\n", szBuffer);
            iRet = -1;
            goto finished;
        }

        /* ִ������ */
        if (pstCurrentCmd->func)
        {
            iRet = pstCurrentCmd->func(ulClientIndex, iKeyCnt, pszKeyWord);
        }

        cli_logr_debug("Cli Server", "CMD", DOS_TRUE, "Control Panel Exec Local CMD:\"%s\"", szCMDBak);

        goto finished;
    }
    /* �Ƿ���н����� */
    else if (iKeyCnt > 1
        && dos_strnicmp(g_szRemoteCMDPrefix, pszKeyWord[0], dos_strlen(g_szRemoteCMDPrefix)) != 0)
    {
        PROCESS_INFO_NODE_ST *pstProcess;
        /* �����Ƿ��иý��̣�����о�ֱ�ӽ�����͵����̣����û�м����������Ƿ��Ǳ������� */
        pstProcess = cli_server_find_active_process(pszKeyWord[0]);
        if (pstProcess
            || dos_strcmp(pszKeyWord[0], "all") == 0)
        {
            if (pstProcess)
            {
                pRealCMD = dos_strstr(szCMDBak, pstProcess->szProcessName);
                pRealCMD += dos_strlen(pstProcess->szProcessName);

                lRet = cli_server_send_cmd2process(ulClientIndex
                                        , pstProcess
                                        , pRealCMD
                                        , dos_strlen(pRealCMD) + 1);
            }
            else
            {
                pRealCMD = dos_strstr(szCMDBak, "all");
                pRealCMD += dos_strlen("all");

                lRet = cli_server_send_broadcast_cmd(ulClientIndex, pRealCMD, dos_strlen(pRealCMD) + 1);
            }

            cli_logr_debug("Cli Server", "CMD", DOS_TRUE, "Control Panel Send CMD:\"%s\" to process \"%s\"", szCMDBak, pszKeyWord[0]);

            /* ������� */
            if (lRet < 0)
            {
                snprintf(szErrorMsg, sizeof(szErrorMsg), "Send msg to remote pthread failed.\r\n");
                iRet = -1;
            }

            goto finished;
        }

        snprintf(szErrorMsg, sizeof(szErrorMsg), "Cannot find the process you are looking for(%s).\r\n", pszKeyWord[0]);
        iRet = -1;
        goto finished;
    }


finished:
    for (i=0; i<iKeyCnt; i++)
    {
        if (pszKeyWord[i])
        {
            dos_dmem_free(pszKeyWord[i]);
        }
    }

    if (iRet < 0)
    {
        telnet_send_data(ulClientIndex, MSG_TYPE_CMD, szErrorMsg, dos_strlen(szErrorMsg));
    }

    return iRet;
}

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
