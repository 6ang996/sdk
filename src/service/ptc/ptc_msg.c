#ifdef  __cplusplus
extern "C" {
#endif

/* include sys header files */
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/time.h>
#include <semaphore.h>
#include <time.h>
#include <signal.h>
/* include provate header file */
#include <pt/dos_sqlite3.h>
#include <pt/ptc.h>
#include "ptc_msg.h"
#include "ptc_web.h"
#include "ptc_telnet.h"

sem_t g_SemPtcRecv;
U32  g_ulHDTimeoutCount = 0;                /* ������ʱ���� */
BOOL g_bIsOnLine = DOS_FALSE;               /* �豸�Ƿ����� */
DOS_TMR_ST g_stHBTmrHandle    = NULL;       /* ������ʱ�� */
DOS_TMR_ST g_stACKTmrHandle   = NULL;       /* ��½������������Ӧ�Ķ�ʱ�� */
PT_CC_CB_ST *g_pstPtcSend     = NULL;       /* ���ͻ�����ptc�ڵ� */
PT_CC_CB_ST *g_pstPtcRecv     = NULL;       /* ���ջ�����ptc�ڵ� */
list_t   g_stPtcNendRecvNode;       /* ��Ҫ���յİ�����Ϣ���� */
list_t   g_stPtcNendSendNode;       /* ��Ҫ���͵İ�����Ϣ���� */
list_t   g_stPtcReSendNode;                 /* ��Ҫ���͵İ�����Ϣ���� */
PTC_SERV_MSG_ST g_stServMsg;                /* ���ptc��Ϣ��ȫ�ֱ��� */
pthread_mutex_t g_mutexPtcSendPthread   = PTHREAD_MUTEX_INITIALIZER;       /* �����߳��� */
pthread_cond_t  g_condPtcSend           = PTHREAD_COND_INITIALIZER;        /* ������������ */
pthread_mutex_t g_mutexPtcRecvPthread   = PTHREAD_MUTEX_INITIALIZER;       /* �����߳��� */
pthread_cond_t  g_condPtcRecv           = PTHREAD_COND_INITIALIZER;        /* ������������ */
pthread_mutex_t g_mutexPtcReSendPthread = PTHREAD_MUTEX_INITIALIZER;       /* �����߳��� */
pthread_cond_t  g_condPtcReSend         = PTHREAD_COND_INITIALIZER;        /* ������������ */
S32 g_ulUdpSocket               = 0;
U32 g_ulPtcNendSendNodeCount    = 0;
U32 g_ulSendTimeSleep           = 20;
U32 g_ulConnectPtsCount         = 0;
S32 g_lHBTimeInterval           = 0;         /* ns */
PT_PTC_TYPE_EN g_enPtcType = PT_PTC_TYPE_LINUX;
struct timeval g_stHBStartTime;
S8 *g_pPackageBuff = NULL;
U32 g_ulPackageLen = 0;
U32 g_ulReceivedLen = 0;
U32 g_usCrc = 0;
PTC_CLIENT_CB_ST g_astPtcConnects[PTC_STREAMID_MAX_COUNT];
U32 g_ulSend2PtsDelay = PTC_SEND2PTS_SLEEP_DEFAULT;
U32 g_ulSendDataCount = 0;
U32 g_ulResendCount   = 0;
U32 g_ulSendNormal2PtsDelay = 0;


void ptc_send2pts(S8 *pstData, U32 ulLen)
{
    S32 lResult = 0;

    if (DOS_ADDR_INVALID(pstData))
    {
        return;
    }

    if (g_ulUdpSocket <= 0)
    {
        g_ulUdpSocket = ptc_create_udp_socket(PTC_SOCKET_CACHE);
    }

    lResult = sendto(g_ulUdpSocket, pstData, ulLen, 0, (struct sockaddr*)&g_pstPtcSend->stDestAddr, sizeof(g_pstPtcSend->stDestAddr));
    if (lResult < 0)
    {
        close(g_ulUdpSocket);
        g_ulUdpSocket = -1;
        g_ulUdpSocket = ptc_create_udp_socket(PTC_SOCKET_CACHE);
    }
}

S8 *ptc_get_current_time()
{
    time_t ulCurrTime = 0;

    ulCurrTime = time(NULL);
    return ctime(&ulCurrTime);
}

void ptc_init_ptc_client_cb(PTC_CLIENT_CB_ST *pstPtcClientCB)
{
    if (DOS_ADDR_INVALID(pstPtcClientCB))
    {
        return;
    }

    pstPtcClientCB->ulStreamID = 0;
    pstPtcClientCB->lSocket = -1;
    pstPtcClientCB->enDataType = PT_DATA_BUTT;
    dos_list_init(&pstPtcClientCB->stRecvBuffList);
    pstPtcClientCB->ulBuffNodeCount = 0;
    pstPtcClientCB->lLastUseTime = time(NULL);
    pstPtcClientCB->pstSendStreamNode = NULL;
    pthread_mutex_init(&pstPtcClientCB->pMutexPthread, NULL);
    if (DOS_ADDR_VALID(pstPtcClientCB->pszBuff))
    {
        dos_dmem_free(pstPtcClientCB->pszBuff);
        pstPtcClientCB->pszBuff = NULL;
    }
    pstPtcClientCB->ulBuffLen = 0;
    pstPtcClientCB->enState = PTC_CLIENT_LEISURE;
}

void ptc_init_all_ptc_client_cb()
{
    S32 i = 0;

    for (i=0; i<PTC_STREAMID_MAX_COUNT; i++)
    {
        ptc_init_ptc_client_cb(&g_astPtcConnects[i]);
    }
}

void ptc_delete_all_client_cb()
{
    S32 i = 0;

    for (i=0; i<PTC_STREAMID_MAX_COUNT; i++)
    {
        if (g_astPtcConnects[i].enState == PTC_CLIENT_USING)
        {
            ptc_delete_client_by_index(&g_astPtcConnects[i]);
        }
    }
}

/**
 * ������S32 ptc_get_socket_by_streamID(U32 ulStreamID)
 * ���ܣ�
 *      1.ͨ��streamID��ö�Ӧ��socket
 * ����
 *      U64 ulStreamID ��
 * ����ֵ����
 */
S32 ptc_get_socket_by_streamID(U32 ulStreamID)
{
    S32 i = 0;

    for (i=0; i<PTC_STREAMID_MAX_COUNT; i++)
    {
        if (g_astPtcConnects[i].enState == PTC_CLIENT_USING
            && g_astPtcConnects[i].ulStreamID == ulStreamID)
        {
            return g_astPtcConnects[i].lSocket;
        }
    }

    return DOS_FAIL;
}

/**
 * ������S32 ptc_get_streamID_by_socket(S32 lSocket, PT_DATA_TYPE_EN enDataType)
 * ���ܣ�
 *      1.ͨ��lSocket �� enDataType ��ö�Ӧ��streamID
 * ����
 * ����ֵ����
 */
S32 ptc_get_streamID_by_socket(S32 lSocket, PT_DATA_TYPE_EN enDataType)
{
    S32 i = 0;

    for (i=0; i<PTC_STREAMID_MAX_COUNT; i++)
    {
        if (g_astPtcConnects[i].enDataType == enDataType
            && g_astPtcConnects[i].enState == PTC_CLIENT_USING
            && g_astPtcConnects[i].lSocket == lSocket)
        {
            return g_astPtcConnects[i].ulStreamID;
        }
    }

    return DOS_FAIL;
}

S32 ptc_get_client_node_array_by_socket(S32 lSocket, PT_DATA_TYPE_EN enDataType)
{
    S32 i = 0;

    for (i=0; i<PTC_STREAMID_MAX_COUNT; i++)
    {
        if (g_astPtcConnects[i].enDataType == enDataType
            && g_astPtcConnects[i].enState == PTC_CLIENT_USING
            && g_astPtcConnects[i].lSocket == lSocket)
        {
            return i;
        }
    }

    return DOS_FAIL;
}

/**
 * ������S32 ptc_add_client(U32 ulStreamID, S32 lSocket, PT_DATA_TYPE_EN enDataType)
 * ���ܣ�
 *      1.
 * ����
 * ����ֵ����
 */
S32 ptc_add_client(U32 ulStreamID, S32 lSocket, PT_DATA_TYPE_EN enDataType, PT_STREAM_CB_ST *pstRecvStreamNode)
{
    S32 i       = 0;
    S32 j       = 0;
    U32 ulIndex = 0;
    static U32 ulStartIndex = 0;
    PT_STREAM_CB_ST *pstRecvStreamHead = NULL;
    PT_STREAM_CB_ST *pstSendStreamHead = NULL;

    ulStartIndex++;
    if (ulStartIndex > PTC_STREAMID_MAX_COUNT - 1)
    {
        ulStartIndex = 0;
    }

    for (i=0; i<PTC_STREAMID_MAX_COUNT; i++)
    {
        ulIndex = ulStartIndex + i;
        if (ulIndex >= PTC_STREAMID_MAX_COUNT)
        {
            ulIndex -= PTC_STREAMID_MAX_COUNT;
        }

        if (PTC_CLIENT_LEISURE == g_astPtcConnects[ulIndex].enState)
        {
            ptc_delete_client_by_index(&g_astPtcConnects[ulIndex]);

            /* ����recv stream�ĵ�ַ�����send stream�ĵ�ַ */
            pstRecvStreamHead = g_pstPtcRecv->pstStreamHead;
            pstSendStreamHead = g_pstPtcSend->pstStreamHead;
            j = pstRecvStreamNode - pstRecvStreamHead;
            if (j < PTC_STREAMID_MAX_COUNT)
            {
                 g_astPtcConnects[ulIndex].pstSendStreamNode = &pstSendStreamHead[j];
            }
            else
            {
                g_astPtcConnects[ulIndex].pstSendStreamNode = NULL;
            }

            dos_list_init(&g_astPtcConnects[ulIndex].stRecvBuffList);
            g_astPtcConnects[ulIndex].ulStreamID = ulStreamID;
            g_astPtcConnects[ulIndex].lSocket = lSocket;
            g_astPtcConnects[ulIndex].enDataType = enDataType;
            g_astPtcConnects[ulIndex].enState = PTC_CLIENT_USING;
            g_astPtcConnects[ulIndex].lLastUseTime = time(NULL);

            switch (enDataType)
            {
                case PT_DATA_WEB:
                    FD_SET(lSocket, &g_stWebFdSet);
                    g_ulWebSocketMax = g_ulWebSocketMax > lSocket ? g_ulWebSocketMax : lSocket;
                    break;
                case PT_DATA_CMD:
                    FD_SET(lSocket, &g_stCmdFdSet);
                    g_ulCmdSocketMax = g_ulCmdSocketMax > lSocket ? g_ulCmdSocketMax : lSocket;
                    break;
                default:
                    break;
            }

            return DOS_SUCC;
        }
    }

    return DOS_FAIL;
}

VOID ptc_delete_client_by_index(PTC_CLIENT_CB_ST *pstPtcClientCB)
{
    list_t *pstRecvBuffList = NULL;
    PTC_WEB_REV_MSG_HANDLE_ST *pstRecvBuffNode = NULL;

    if (DOS_ADDR_INVALID(pstPtcClientCB))
    {
        return;
    }
    pstPtcClientCB->enState = PTC_CLIENT_FREEING;

    if (pstPtcClientCB->lSocket > 0)
    {
        switch (pstPtcClientCB->enDataType)
        {
        case PT_DATA_WEB:
            FD_CLR(pstPtcClientCB->lSocket, &g_stWebFdSet);
            break;
        case PT_DATA_CMD:
            FD_CLR(pstPtcClientCB->lSocket, &g_stCmdFdSet);
            break;
        default:
            break;
        }
        close(pstPtcClientCB->lSocket);
    }

    pthread_mutex_lock(&pstPtcClientCB->pMutexPthread);
    pstPtcClientCB->ulBuffNodeCount = 0;

    while (1)
    {
        if (dos_list_is_empty(&pstPtcClientCB->stRecvBuffList))
        {
            break;
        }

        pstRecvBuffList = dos_list_fetch(&pstPtcClientCB->stRecvBuffList);
        if (DOS_ADDR_INVALID(pstRecvBuffList))
        {
            break;
        }

        pstRecvBuffNode = dos_list_entry(pstRecvBuffList, PTC_WEB_REV_MSG_HANDLE_ST, stList);
        if (DOS_ADDR_INVALID(pstRecvBuffList))
        {
            continue;
        }

        if (DOS_ADDR_VALID(pstRecvBuffNode->paRecvBuff))
        {
            dos_dmem_free(pstRecvBuffNode->paRecvBuff);
            pstRecvBuffNode->paRecvBuff = NULL;
        }
        if (DOS_ADDR_VALID(pstRecvBuffNode))
        {
            dos_dmem_free(pstRecvBuffNode);
            pstRecvBuffNode = NULL;
        }
    }

    pthread_mutex_unlock(&pstPtcClientCB->pMutexPthread);

    ptc_init_ptc_client_cb(pstPtcClientCB);
}

/**
 * ������S32 ptc_add_client(U32 ulStreamID, S32 lSocket, PT_DATA_TYPE_EN enDataType)
 * ���ܣ�
 *      1.
 * ����
 * ����ֵ����
 */
VOID ptc_delete_client_by_socket(S32 lSocket, PT_DATA_TYPE_EN enDataType)
{
    S32 i = 0;

    for (i=0; i<PTC_STREAMID_MAX_COUNT; i++)
    {
        if (g_astPtcConnects[i].enDataType == enDataType
            && g_astPtcConnects[i].enState == PTC_CLIENT_USING
            && g_astPtcConnects[i].lSocket == lSocket)
        {
            ptc_delete_client_by_index(&g_astPtcConnects[i]);

            return;
        }
    }
}

/**
 * ������S32 ptc_add_client(U32 ulStreamID, S32 lSocket, PT_DATA_TYPE_EN enDataType)
 * ���ܣ�
 *      1.
 * ����
 * ����ֵ����
 */
VOID ptc_delete_client_by_streamID(U32 ulStreamID)
{
    S32 i = 0;

    for (i=0; i<PTC_STREAMID_MAX_COUNT; i++)
    {
        if (g_astPtcConnects[i].enState == PTC_CLIENT_USING
            && g_astPtcConnects[i].ulStreamID == ulStreamID)
        {
            ptc_delete_client_by_index(&g_astPtcConnects[i]);

            return;
        }
    }
}

/**
 * ������S32 ptc_add_client(U32 ulStreamID, S32 lSocket, PT_DATA_TYPE_EN enDataType)
 * ���ܣ�
 *      1.
 * ����
 * ����ֵ����
 */
void ptc_set_cache_full_true(S32 lSocket, PT_DATA_TYPE_EN enDataType)
{
    S32 i = 0;

    for (i=0; i<PTC_STREAMID_MAX_COUNT; i++)
    {
        if (g_astPtcConnects[i].enDataType == enDataType
            && g_astPtcConnects[i].enState == PTC_CLIENT_USING
            && g_astPtcConnects[i].lSocket == lSocket)
        {
            if (PT_DATA_WEB == enDataType)
            {
                FD_CLR(lSocket, &g_stWebFdSet);
            }
        }
    }
}

/**
 * ������S32 ptc_add_client(U32 ulStreamID, S32 lSocket, PT_DATA_TYPE_EN enDataType)
 * ���ܣ�
 *      1.
 * ����
 * ����ֵ����
 */
void ptc_set_cache_full_false(U32 ulStreamID, PT_DATA_TYPE_EN enDataType)
{
    S32 i = 0;

    for (i=0; i<PTC_STREAMID_MAX_COUNT; i++)
    {
        if (g_astPtcConnects[i].enDataType == enDataType
            && g_astPtcConnects[i].enState == PTC_CLIENT_USING
            && g_astPtcConnects[i].ulStreamID == ulStreamID)
        {
            if (PT_DATA_WEB == enDataType)
            {
                FD_SET(g_astPtcConnects[i].lSocket, &g_stWebFdSet);
            }
        }
    }
}

S32 ptc_printf_recv_cache_msg(U32 ulIndex, S32 argc, S8 **argv)
{
    S32             i           = 0;
    U32             ulLen       = 0;
    S8              szBuff[PT_DATA_BUFF_512]     = {0};
    PT_STREAM_CB_ST *pstStreamHead  = NULL;
    PT_STREAM_CB_ST *pstStreamCB    = NULL;

    ulLen = snprintf(szBuff, sizeof(szBuff), "\r\n%-20s%-15s%-10s%-10s%-10s\r\n", "SN", "StreamID", "CurrSeq", "MaxSeq", "ConfirmSeq");
    cli_out_string(ulIndex, szBuff);

    pstStreamHead = g_pstPtcRecv->pstStreamHead;
    for (i=0; i<PTC_STREAMID_MAX_COUNT; i++)
    {
        pstStreamCB = &pstStreamHead[i];

        snprintf(szBuff, sizeof(szBuff), "%.16s%11d%10d%10d%10d\r\n", g_pstPtcRecv->aucID, pstStreamCB->ulStreamID, pstStreamCB->lCurrSeq, pstStreamCB->lMaxSeq, pstStreamCB->lConfirmSeq);
        cli_out_string(ulIndex, szBuff);
    }

    return 0;
}

S32 ptc_printf_send_cache_msg(U32 ulIndex, S32 argc, S8 **argv)
{
    S32             i           = 0;
    U32             ulLen       = 0;
    S8              szBuff[PT_DATA_BUFF_512]     = {0};
    PT_STREAM_CB_ST *pstStreamHead  = NULL;
    PT_STREAM_CB_ST *pstStreamCB    = NULL;

    ulLen = snprintf(szBuff, sizeof(szBuff), "\r\n%-20s%-15s%-10s%-10s%-10s\r\n", "SN", "StreamID", "CurrSeq", "MaxSeq", "ConfirmSeq");
    cli_out_string(ulIndex, szBuff);

    pstStreamHead = g_pstPtcSend->pstStreamHead;
    for (i=0; i<PTC_STREAMID_MAX_COUNT; i++)
    {
        pstStreamCB = &pstStreamHead[i];

        snprintf(szBuff, sizeof(szBuff), "%.16s%11d%10d%10d%10d\r\n", g_pstPtcSend->aucID, pstStreamCB->ulStreamID, pstStreamCB->lCurrSeq, pstStreamCB->lMaxSeq, pstStreamCB->lConfirmSeq);
        cli_out_string(ulIndex, szBuff);
    }

    return 0;
}


S32 ptc_create_udp_socket(U32 ulSocketCache)
{
    S32 lSocket = 0;
    S32 lRet = 0;

    while (1)
    {
        lSocket = socket(AF_INET, SOCK_DGRAM, 0);
        if (lSocket < 0)
        {
            perror("create socket error!");
            sleep(1);

            continue;
        }
        lRet = setsockopt(lSocket, SOL_SOCKET, SO_SNDBUF, (char*)&ulSocketCache, sizeof(ulSocketCache));
        if (lRet != 0)
        {
            logr_error("setsockopt error : %d", lRet);
            close(lSocket);
            sleep(1);

            continue;
        }

        lRet = setsockopt(lSocket, SOL_SOCKET, SO_RCVBUF, (char*)&ulSocketCache, sizeof(ulSocketCache));
        if (lRet != 0)
        {
            logr_error("setsockopt error : %d", lRet);
            close(lSocket);
            sleep(1);

            continue;
        }

        break;
    }

    return lSocket;
}

/**
 * ������VOID ptc_delete_send_stream_node(U32 ulStreamID, PT_DATA_TYPE_EN enDataType)
 * ���ܣ�
 *      1.ɾ�����ͻ�����,һ��stream�ڵ�
 * ����
 *      U32 ulStreamID ��stream id
 *      U32 enDataType ����Ϣ����
 * ����ֵ����
 */
VOID ptc_delete_send_stream_node_by_streamID(U32 ulStreamID)
{
    PT_STREAM_CB_ST *pstRecvStreamHead = g_pstPtcRecv->pstStreamHead;
    PT_STREAM_CB_ST *pstSendStreamHead = g_pstPtcSend->pstStreamHead;
    S32 i = 0;

    for (i=0; i<PTC_STREAMID_MAX_COUNT; i++)
    {
        if (pstRecvStreamHead[i].bIsValid && pstRecvStreamHead[i].ulStreamID == ulStreamID)
        {
            pt_delete_stream_node(&pstRecvStreamHead[i], 0);
            pt_delete_stream_node(&pstSendStreamHead[i], 1);
        }
    }
}

void ptc_delete_stream_node_by_recv(PT_STREAM_CB_ST *pstRecvStreamNode)
{
    PT_STREAM_CB_ST *pstRecvStreamHead = g_pstPtcRecv->pstStreamHead;
    PT_STREAM_CB_ST *pstSendStreamHead = g_pstPtcSend->pstStreamHead;
    S32 i = 0;

    if (DOS_ADDR_INVALID(pstRecvStreamNode))
    {
        return;
    }

    pt_delete_stream_node(pstRecvStreamNode, 0);

    i = pstRecvStreamHead - pstRecvStreamNode;
    if (i < PTC_STREAMID_MAX_COUNT)
    {
        pt_delete_stream_node(&pstSendStreamHead[i], 1);
    }
}

void ptc_init_all_stream()
{
    S32 i = 0;
    PT_STREAM_CB_ST *pstStreamRecvHead = g_pstPtcRecv->pstStreamHead;
    PT_STREAM_CB_ST *pstStreamSendHead = g_pstPtcSend->pstStreamHead;

    for (i=0; i<PTC_STREAMID_MAX_COUNT; i++)
    {
        pt_delete_stream_node(&pstStreamRecvHead[i], 0);
        pt_delete_stream_node(&pstStreamSendHead[i], 1);
    }

}

void ptc_free_stream_resoure(PT_DATA_TYPE_EN enDataType, PTC_CLIENT_CB_ST *pstPtcClientCB, U32 ulStreamID, S32 lExitType)
{
    pt_logr_debug("delete stream resource, streamID : %d", ulStreamID);
    ptc_send_exit_notify_to_pts(enDataType, ulStreamID, lExitType);
    ptc_delete_send_stream_node_by_streamID(ulStreamID);
    if (DOS_ADDR_VALID(pstPtcClientCB))
    {
        ptc_delete_client_by_index(pstPtcClientCB);
    }
    pt_logr_debug("after delete stream resource, streamID : %d", ulStreamID);
}

S32 ptc_get_udp_user_ip()
{
    S32 lSockfd = 0;
    S32 lError  = 0;
    struct sockaddr_in stServAddr;
    struct sockaddr_in stAddrCli;
    socklen_t lAddrLen = sizeof(struct sockaddr_in);
    S8 szLocalIp[PT_IP_ADDR_SIZE] = {0}; /* ���ʮ���� */
    S8 szIfrName[PT_DATA_BUFF_64] = {0};
    S8 szMac[PT_DATA_BUFF_64] = {0};

    bzero(&stServAddr, sizeof(stServAddr));
    stServAddr.sin_family = AF_INET;

    stServAddr.sin_port = g_stServMsg.usBaiduPort;
    stServAddr.sin_addr.s_addr = *(U32 *)(g_stServMsg.achBaiduIP);

    lSockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lSockfd < 0)
    {
        perror("create tcp socket");
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

ptc_connect:
    lError = connect(lSockfd,(struct sockaddr*)&stServAddr, sizeof(stServAddr));
    if (lError < 0)
    {
        perror("connect pts");
        sleep(2);
        /* ���� */
        goto ptc_connect;
    }

    lError = getsockname(lSockfd,(struct sockaddr*)&stAddrCli, &lAddrLen);
    if (lError != 0)
    {
        pt_logr_info("Get sockname Error!");
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    g_stServMsg.usLocalPort = dos_ntohs(stAddrCli.sin_port);
    dos_strcpy(szLocalIp, inet_ntoa(stAddrCli.sin_addr));
    inet_pton(AF_INET, szLocalIp, (VOID *)(g_stServMsg.achLocalIP));
    lError = ptc_get_ifr_name_by_ip(szLocalIp, lSockfd, szIfrName, PT_DATA_BUFF_64);
    if (lError != DOS_SUCC)
    {
        pt_logr_info("Get ifr name Error!");
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    lError = ptc_get_mac(szIfrName, lSockfd, szMac, PT_DATA_BUFF_64);
    if (lError != DOS_SUCC)
    {
        pt_logr_info("Get mac Error!");
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    dos_strncpy(g_stServMsg.szMac, szMac, PT_DATA_BUFF_64);
    logr_debug("local ip : %s, irf name : %s, mac : %s", szLocalIp, szIfrName, szMac);

    close(lSockfd);

    return DOS_SUCC;
}

/**
 * ������VOID ptc_data_lose(PT_MSG_TAG *pstMsgDes, S32 lShouldSeq)
 * ���ܣ�
 *      1.���Ͷ�������
 * ����
 *      PT_MSG_TAG *pstMsgDes ��������ʧ����������Ϣ
 * ����ֵ����
 */
VOID ptc_data_lose(PT_MSG_TAG *pstMsgDes)
{
    if (NULL == pstMsgDes)
    {
        pt_logr_debug("ptc data lose param is NULL");
        return;
    }

    pthread_mutex_lock(&g_mutexPtcSendPthread);

    BOOL bIsResend = DOS_FALSE;
    PT_CMD_EN enCmdValue = PT_CMD_RESEND;
    pt_need_send_node_list_insert(&g_stPtcNendSendNode, g_pstPtcSend->aucID, pstMsgDes, enCmdValue, bIsResend, DOS_FALSE);

    pthread_cond_signal(&g_condPtcSend);
    pthread_mutex_unlock(&g_mutexPtcSendPthread);

    return;
}

VOID ptc_send_exit_notify_to_pts(PT_DATA_TYPE_EN enDataType, U32 ulStreamID, S32 lSeq)
{
    PT_MSG_TAG stMsgDes;

    stMsgDes.enDataType = enDataType;
    dos_memcpy(stMsgDes.aucID, g_pstPtcSend->aucID, PTC_ID_LEN);
    stMsgDes.ulStreamID = dos_htonl(ulStreamID);
    stMsgDes.ExitNotifyFlag = DOS_TRUE;
    stMsgDes.lSeq = dos_htonl(lSeq);
    stMsgDes.enCmdValue = PT_CMD_NORMAL;
    stMsgDes.bIsEncrypt = DOS_FALSE;
    stMsgDes.bIsCompress = DOS_FALSE;

    ptc_send2pts((S8 *)&stMsgDes, sizeof(PT_MSG_TAG));
}

/**
 * ������VOID ptc_send_lost_data_req(U64 ulLoseMsg)
 * ���ܣ�
 *      1.�ش���ʧ�İ�
 * ����
 *      U64 ulLoseMsg ��
 * ����ֵ����
 */
VOID ptc_send_lost_data_req(U64 ulLoseMsg)
{
    if (0 == ulLoseMsg)
    {
        return;
    }

    S32 i = 0;
    U32 ulCount = 0;
    U32 ulArraySub = 0;
    PT_LOSE_BAG_MSG_ST *pstLoseMsg = (PT_LOSE_BAG_MSG_ST *)ulLoseMsg;
    PT_MSG_TAG *pstMsg = &pstLoseMsg->stMsg;
    PT_STREAM_CB_ST *pstStreamNode = pstLoseMsg->pstStreamNode;

    if (pstStreamNode->ulCountResend >= PT_RESEND_REQ_MAX_COUNT)
    {
        /* �����ط�PTC_RESEND_MAX_COUNT�Σ���δ�յ� */
        ptc_free_stream_resoure((PT_DATA_TYPE_EN)pstMsg->enDataType, NULL, pstMsg->ulStreamID, 0);    /* ֪ͨpts�ر�sockfd */
        dos_tmr_stop(&pstStreamNode->hTmrHandle);
        return;
    }
    else
    {
        pstStreamNode->ulCountResend++;
        for (i=pstStreamNode->lCurrSeq+1; i<pstStreamNode->lMaxSeq; i++)
        {
            ulArraySub = i & (PT_DATA_SEND_CACHE_SIZE - 1);
            if (0 == i)
            {
                /* �ж�0�Ű��Ƿ�ʧ */
                if (pstStreamNode->unDataQueHead.pstDataTcp[ulArraySub].ulLen == 0)
                {
                    ulCount++;
                    pstMsg->lSeq = i;
                    ptc_data_lose(pstMsg);
                }
            }
            else if (pstStreamNode->unDataQueHead.pstDataTcp[ulArraySub].lSeq != i)
            {
                ulCount++;
                pstMsg->lSeq = i;
                ptc_data_lose(pstMsg);  /*���Ͷ����ط�����*/
            }
        }
    }

    if (0 == ulCount)
    {
        if (NULL != pstStreamNode->hTmrHandle)
        {
            dos_tmr_stop(&pstStreamNode->hTmrHandle);
            pstStreamNode->hTmrHandle = NULL;
        }
        pstStreamNode->ulCountResend = 0;
    }
}

/**
 * ������S32 ptc_key_convert(S8 *szKey, S8 *szDest, S8 *szVersion)
 * ���ܣ�
 *      1.��½��֤
 * ����
 *      S8 *szKey     ��ԭʼֵ
 *      S8 *szDest    ��ת�����ֵ
 *      S8 *szVersion ��ptc�汾��
 * ����ֵ����
 */
S32 ptc_key_convert(S8 *szKey, S8 *szDest)
{
    if (NULL == szKey || NULL == szDest)
    {

        return DOS_FAIL;
    }

    S32 i = 0;

    //if (dos_strncmp(ptc_get_version(), "1.1", dos_strlen("1.1")) == 0)
    //{
    /* 1.1�汾��֤���� */
    for (i=0; i<PT_LOGIN_VERIFY_SIZE-1; i++)
    {
        szDest[i] = szKey[i]&0xA9;
    }
    szDest[PT_LOGIN_VERIFY_SIZE] = '\0';
    //}

    return DOS_SUCC;
}

/**
 * ������VOID ptc_hd_ack_timeout_callback(U64 lSockfd)
 * ���ܣ�
 *      1.������Ӧ�ĳ�ʱ����
 * ����
 *      U64 lSockfd ��udpͨ���׽���
 * ����ֵ����
 */
VOID ptc_hd_ack_timeout_callback()
{
    g_ulHDTimeoutCount++;
    pt_logr_debug("Heartbeat response timeout : %d", g_ulHDTimeoutCount);
    g_lHBTimeInterval = -1;
    if (g_ulHDTimeoutCount >= PTC_HB_TIMEOUT_COUNT_MAX)
    {
        /* ��������޷��յ�������Ӧ�����·��͵�½���� */
        logr_debug("can not connect PTS!!!");
        dos_tmr_stop(&g_stACKTmrHandle);
        g_stACKTmrHandle = NULL;
        ptc_init_all_stream();

        ptc_delete_all_client_cb();
        ptc_init_all_ptc_client_cb();

        g_bIsOnLine = DOS_FALSE;

        ptc_send_login2pts();

    }
}

/**
 * ������VOID ptc_send_hb_req(U64 lSockfd)
 * ���ܣ�
 *      1.������������
 * ����
 *      U64 lSockfd ��udpͨ���׽���
 * ����ֵ����
 */
VOID ptc_send_hb_req()
{
    PT_MSG_TAG *pstMsgDes = NULL;
    HEART_BEAT_RTT_TSG stVerRet;
    S8 acBuff[PT_DATA_BUFF_512] = {0};
    S32 lResult = 0;

    pstMsgDes = (PT_MSG_TAG *)acBuff;
    /* ������Ϣ������ֵ */
    pstMsgDes->enDataType = PT_DATA_CTRL;
    dos_memcpy(pstMsgDes->aucID, g_pstPtcSend->aucID, PTC_ID_LEN);
    pstMsgDes->ulStreamID = dos_htonl(PT_CTRL_HB_REQ);
    pstMsgDes->ExitNotifyFlag = DOS_FALSE;
    pstMsgDes->lSeq = 0;
    pstMsgDes->enCmdValue = PT_CMD_NORMAL;
    pstMsgDes->bIsEncrypt = DOS_FALSE;
    pstMsgDes->bIsCompress = DOS_FALSE;
    dos_memcpy(pstMsgDes->aulServIp, g_stServMsg.achLocalIP, IPV6_SIZE);
    /* �������ݸ�ֵ */
    pstMsgDes->usServPort = g_stServMsg.usLocalPort;

    stVerRet.lHBTimeInterval = g_lHBTimeInterval;
    dos_memcpy(acBuff+sizeof(PT_MSG_TAG), (VOID *)&stVerRet, sizeof(PT_CTRL_DATA_ST));

    gettimeofday(&g_stHBStartTime, NULL);

    ptc_send2pts(acBuff, sizeof(HEART_BEAT_RTT_TSG) + sizeof(PT_MSG_TAG));

    /* ������ʱ�����ж�������Ӧ�Ƿ�ʱ */
    pt_logr_debug("send hd to pts");
    lResult = dos_tmr_start(&g_stACKTmrHandle, PTC_WAIT_HB_ACK_TIME, ptc_hd_ack_timeout_callback, 0, TIMER_NORMAL_NO_LOOP);
    if (lResult < 0)
    {
        pt_logr_info("ptc_send_hb_req : start timer fail");
    }
}

/**
 * ������VOID ptc_send_login_req(U64 arg)
 * ���ܣ�
 *      1.���͵�½����
 * ����
 *      U64 arg ��udpͨ���׽���
 * ����ֵ����
 */
VOID ptc_send_login_req(U64 arg)
{
    S32 lResult = 0;
    PT_MSG_TAG *pstMsgDes = NULL;
    PT_CTRL_DATA_ST stVerRet;
    S8 acBuff[PT_DATA_BUFF_512] = {0};
    struct sockaddr_in stAddrCli;
    socklen_t lAddrLen = sizeof(struct sockaddr_in);
    S8 szDestIp[PT_IP_ADDR_SIZE] = {0};

    pstMsgDes = (PT_MSG_TAG *)acBuff;

    if (g_bIsOnLine)
    {
        /* �ѵ�½,�رն�ʱ������������ */
        dos_tmr_stop(&g_stHBTmrHandle);
        ptc_send_heartbeat2pts();

        return;
    }
    g_ulConnectPtsCount++;

    if (g_ulConnectPtsCount > 4)
    {
        g_ulConnectPtsCount = 0;
        if (g_pstPtcSend->stDestAddr.sin_addr.s_addr == *(U32 *)(g_stServMsg.achPtsMajorIP) && g_stServMsg.usPtsMajorPort == g_pstPtcSend->stDestAddr.sin_port)
        {
            /* ������ע�᲻�ϣ��л�pts�������� */
            if (g_stServMsg.usPtsMinorPort != 0)
            {
                g_pstPtcSend->stDestAddr.sin_addr.s_addr = *(U32 *)(g_stServMsg.achPtsMinorIP);
                g_pstPtcSend->stDestAddr.sin_port = g_stServMsg.usPtsMinorPort;
            }
        }
        else if (g_pstPtcSend->stDestAddr.sin_addr.s_addr == *(U32 *)(g_stServMsg.achPtsMinorIP) && g_stServMsg.usPtsMinorPort == g_pstPtcSend->stDestAddr.sin_port)
        {
            /* ������ע�᲻�ϣ��л��������� */
            if (g_stServMsg.usPtsMajorPort != 0)
            {
                g_pstPtcSend->stDestAddr.sin_addr.s_addr = *(U32 *)(g_stServMsg.achPtsMajorIP);
                g_pstPtcSend->stDestAddr.sin_port = g_stServMsg.usPtsMajorPort;
            }
        }
        else
        {
            /* ��������ע�᲻�ϣ��л��������� */
            if (g_stServMsg.usPtsMajorPort != 0)
            {
                g_pstPtcSend->stDestAddr.sin_addr.s_addr = *(U32 *)(g_stServMsg.achPtsMajorIP);
                g_pstPtcSend->stDestAddr.sin_port = g_stServMsg.usPtsMajorPort;
            }
        }
    }
    /* ��½��Ϣ������ֵ */
    pstMsgDes->enDataType = PT_DATA_CTRL;
    dos_memcpy(pstMsgDes->aucID, g_pstPtcSend->aucID, PTC_ID_LEN);
    pstMsgDes->ulStreamID = dos_htonl(PT_CTRL_LOGIN_REQ);
    pstMsgDes->ExitNotifyFlag = DOS_FALSE;
    pstMsgDes->lSeq = 0;
    pstMsgDes->enCmdValue = PT_CMD_NORMAL;
    pstMsgDes->bIsEncrypt = DOS_FALSE;
    pstMsgDes->bIsCompress = DOS_FALSE;
    //dos_memcpy(stMsgDes.aulServIp, g_stServMsg.achLocalIP, IPV6_SIZE);
    //stMsgDes.usServPort = g_stServMsg.usLocalPort;

    /* ��½���ݸ�ֵ */
    stVerRet.enCtrlType = PT_CTRL_LOGIN_REQ;

    dos_memcpy(acBuff+sizeof(PT_MSG_TAG), (VOID *)&stVerRet, sizeof(PT_CTRL_DATA_ST));

    if (dos_ntohs(g_pstPtcSend->stDestAddr.sin_port) != 0)
    {
        ptc_send2pts(acBuff, sizeof(PT_CTRL_DATA_ST) + sizeof(PT_MSG_TAG));

        inet_ntop(AF_INET, (void *)(&g_pstPtcSend->stDestAddr.sin_addr.s_addr), szDestIp, PT_IP_ADDR_SIZE);
        logr_debug("ptc send login request to pts : ip : %s:%u", szDestIp, dos_ntohs(g_pstPtcSend->stDestAddr.sin_port));
    }

    /* ��ȡptcһ��udp��˽���˿ں� -- sendto�󣬲��ܻ�ȡ�˿ں� */
    if (g_stServMsg.usLocalPort == 0)
    {
        lResult = getsockname(g_ulUdpSocket,(struct sockaddr*)&stAddrCli, &lAddrLen);
        if (lResult != 0)
        {
            pt_logr_info("Get sockname Error!");
            return;
        }
        g_stServMsg.usLocalPort = dos_ntohs(stAddrCli.sin_port);
    }
}

/**
 * ������VOID ptc_send_logout_req(U64 arg)
 * ���ܣ�
 *      1.�˳���½����
 * ����
 *      U64 arg ��udpͨ���׽���
 * ����ֵ����
 */
VOID ptc_send_logout_req()
{
    PT_MSG_TAG *pstMsgDes = NULL;
    PT_CTRL_DATA_ST stVerRet;
    S8 acBuff[PT_DATA_BUFF_512] = {0};

    pstMsgDes = (PT_MSG_TAG *)acBuff;

    pstMsgDes->enDataType = PT_DATA_CTRL;
    dos_memcpy(pstMsgDes->aucID, g_pstPtcSend->aucID, PTC_ID_LEN);
    pstMsgDes->ulStreamID = dos_htonl(PT_CTRL_LOGOUT);
    pstMsgDes->ExitNotifyFlag = DOS_FALSE;
    pstMsgDes->lSeq = 0;
    pstMsgDes->enCmdValue = PT_CMD_NORMAL;
    pstMsgDes->bIsEncrypt = DOS_FALSE;
    pstMsgDes->bIsCompress = DOS_FALSE;
    dos_memcpy(pstMsgDes->aulServIp, g_stServMsg.achLocalIP, IPV6_SIZE);
    pstMsgDes->usServPort = g_stServMsg.usLocalPort;

    stVerRet.enCtrlType = PT_CTRL_LOGOUT;
    dos_memcpy(acBuff+sizeof(PT_MSG_TAG), (VOID *)&stVerRet, sizeof(PT_CTRL_DATA_ST));

    ptc_send2pts(acBuff, sizeof(PT_CTRL_DATA_ST) + sizeof(PT_MSG_TAG));
}

/**
 * ������VOID ptc_send_confirm_msg(PT_MSG_TAG *pstMsgDes, U32 lConfirmSeq)
 * ���ܣ�
 *      1.����ȷ�Ͻ�����Ϣ
 * ����
 *
 * ����ֵ����
 */
VOID ptc_send_confirm_msg(PT_MSG_TAG *pstMsgDes, U32 lConfirmSeq)
{
    if (NULL == pstMsgDes)
    {
        pt_logr_debug("ptc_send_confirm_msg : param error");
        return;
    }

    S32 i = 0;

    pthread_mutex_lock(&g_mutexPtcSendPthread);

    BOOL bIsResend = DOS_FALSE;
    PT_CMD_EN enCmdValue = PT_CMD_CONFIRM;
    pstMsgDes->lSeq = lConfirmSeq;

    pt_logr_debug("send confirm data to pts: type = %d, stream = %d", pstMsgDes->enDataType,pstMsgDes->ulStreamID);
    for (i=0; i<PT_SEND_CONFIRM_COUNT; i++)
    {
        pt_need_send_node_list_insert(&g_stPtcNendSendNode, pstMsgDes->aucID, pstMsgDes, enCmdValue, bIsResend, DOS_FALSE);
    }
    pthread_cond_signal(&g_condPtcSend);
    pthread_mutex_unlock(&g_mutexPtcSendPthread);

}

/**
 * ������S32 ptc_save_into_send_cache(PT_CC_CB_ST *pstPtcNode, PT_MSG_TAG *pstMsgDes, S8 *acSendBuf, S32 lDataLen)
 *      1.��ӵ����ͻ���
 * ����
 *      PT_CC_CB_ST *pstPtcNode : ptc�����ַ
 *      PT_MSG_TAG *pstMsgDes   ��ͷ����Ϣ
 *      S8 *acSendBuf           ��������
 *      S32 lDataLen            �������ݳ���
 * ����ֵ��
 *      PT_SAVE_DATA_SUCC ��ӳɹ�
 *      PT_SAVE_DATA_FAIL ʧ��
 *      PT_NEED_CUT_PTHREAD ��64�������ȴ�ȷ�Ͻ�����Ϣ
 */
S32 ptc_save_into_send_cache(PT_MSG_TAG *pstMsgDes, S8 *acSendBuf, S32 lDataLen)
{
    if (DOS_ADDR_INVALID(g_pstPtcSend) || DOS_ADDR_INVALID(pstMsgDes) || DOS_ADDR_INVALID(acSendBuf))
    {
        return PT_SAVE_DATA_FAIL;
    }

    PT_STREAM_CB_ST *pstStreamListHead      = NULL;
    PT_STREAM_CB_ST *pstRecvStreamListHead  = NULL;
    PT_STREAM_CB_ST *pstStreamNode          = NULL;
    S32             lResult                 = 0;
    S32             i                       = 0;

    pstStreamListHead     = g_pstPtcSend->pstStreamHead;
    pstRecvStreamListHead = g_pstPtcRecv->pstStreamHead;
    pstStreamNode = pt_stream_queue_search(pstStreamListHead, pstMsgDes->ulStreamID);
    if (DOS_ADDR_INVALID(pstStreamNode))
    {
        if (pstMsgDes->enDataType == PT_DATA_CTRL)
        {
            /* ������ */
            i =  pt_stream_queue_get_node(pstRecvStreamListHead);
            if (i < 0 || i >= PTC_STREAMID_MAX_COUNT)
            {
                return PT_SAVE_DATA_FAIL;
            }
            pstStreamNode = &pstStreamListHead[i];
            /* ��ʼ����Ӧ�ķ��ͻ��� */
            pt_stream_node_init(pstStreamNode);
            pstStreamNode->bIsValid = DOS_TRUE;
            pstStreamNode->ulStreamID = pstMsgDes->ulStreamID;
            pstStreamNode->enDataType = (PT_DATA_TYPE_EN)pstMsgDes->enDataType;

            dos_memcpy(pstRecvStreamListHead->aulServIp, pstMsgDes->aulServIp, IPV6_SIZE);
            pstRecvStreamListHead->usServPort = pstMsgDes->usServPort;
            pstRecvStreamListHead->ulStreamID = pstMsgDes->ulStreamID;
            pstRecvStreamListHead->enDataType = (PT_DATA_TYPE_EN)pstMsgDes->enDataType;
        }
        else
        {
            return PT_SAVE_DATA_FAIL;
        }
    }

    /* �����ݲ��뵽data queue�� */
    lResult = pt_send_data_tcp_queue_insert(pstStreamNode, acSendBuf, lDataLen, PT_DATA_SEND_CACHE_SIZE, 0);
    if (lResult < 0)
    {
        return PT_SAVE_DATA_FAIL;
    }

    return lResult;
}

void ptc_send_login_rsp_to_pts(PT_CTRL_DATA_ST *pstVerRet)
{
    PT_MSG_TAG *pstMsgDes = NULL;
    S8 acBuff[PT_DATA_BUFF_512] = {0};

    pstMsgDes = (PT_MSG_TAG *)acBuff;

    pstMsgDes->enDataType = PT_DATA_CTRL;
    dos_memcpy(pstMsgDes->aucID, g_pstPtcSend->aucID, PTC_ID_LEN);
    pstMsgDes->ulStreamID = dos_htonl(PT_CTRL_LOGIN_RSP);
    pstMsgDes->ExitNotifyFlag = DOS_FALSE;
    pstMsgDes->lSeq = 0;
    pstMsgDes->enCmdValue = PT_CMD_NORMAL;
    pstMsgDes->bIsEncrypt = DOS_FALSE;
    pstMsgDes->bIsCompress = DOS_FALSE;

    dos_memcpy(pstMsgDes->aulServIp, g_stServMsg.achLocalIP, IPV6_SIZE);
    pstMsgDes->usServPort = g_stServMsg.usLocalPort;

    dos_memcpy(acBuff+sizeof(PT_MSG_TAG), pstVerRet, sizeof(PT_CTRL_DATA_ST));

    ptc_send2pts(acBuff,  sizeof(PT_CTRL_DATA_ST) + sizeof(PT_MSG_TAG));
}

/**
 * ������S32 ptc_save_into_cache(PT_MSG_TAG *pstMsgDes, S8 *acRecvBuf, S32 lDataLen)
 *      1.��ӵ����ջ���
 * ����
 *      PT_CC_CB_ST *pstPtcNode : ptc�����ַ
 *      PT_MSG_TAG *pstMsgDes   ��ͷ����Ϣ
 *      S8 *acRecvBuf           ��������
 *      S32 lDataLen            �������ݳ���
 *      PT_SAVE_DATA_SUCC ��ӳɹ�
 *      PT_SAVE_DATA_FAIL ʧ��
 *      PT_NEED_CUT_PTHREAD �����Ѵ���
 */
S32 ptc_save_into_recv_cache(S8 *acRecvBuf, S32 lDataLen)
{
   if (DOS_ADDR_INVALID(acRecvBuf))
    {
        return PT_SAVE_DATA_FAIL;
    }

    S32 i = 0;
    U32 ulArraySub = 0;
    S32 lResult = 0;
    U32 ulNextSendArraySub = 0;
    PT_STREAM_CB_ST    *pstRecvStreamHead   = NULL;
    PT_STREAM_CB_ST    *pstSendStreamHead   = NULL;
    PT_STREAM_CB_ST    *pstStreamNode       = NULL;
    PT_LOSE_BAG_MSG_ST *pstLoseMsg          = NULL;
    PT_DATA_TCP_ST     *pstDataQueue        = NULL;
    PT_MSG_TAG         *pstMsgDes           = (PT_MSG_TAG *)acRecvBuf;

    /* �жϷ��ͻ������Ƿ���� streamID */
    pstRecvStreamHead = g_pstPtcRecv->pstStreamHead;
    pstSendStreamHead = g_pstPtcSend->pstStreamHead;
    pstStreamNode = pt_stream_queue_search(pstRecvStreamHead, pstMsgDes->ulStreamID);
    if (DOS_ADDR_INVALID(pstStreamNode))
    {
        /* ������ */
        i =  pt_stream_queue_get_node(pstRecvStreamHead);
        if (i < 0 || i >= PTC_STREAMID_MAX_COUNT)
        {
            return PT_SAVE_DATA_FAIL;
        }
        pstStreamNode = &pstRecvStreamHead[i];
        /* ��ʼ����Ӧ�ķ��ͻ��� */
        pt_stream_node_init(&pstSendStreamHead[i]);
        pstSendStreamHead[i].bIsValid = DOS_TRUE;
        pstSendStreamHead[i].ulStreamID = pstMsgDes->ulStreamID;
        pstSendStreamHead[i].enDataType = (PT_DATA_TYPE_EN)pstMsgDes->enDataType;

        dos_memcpy(pstStreamNode->aulServIp, pstMsgDes->aulServIp, IPV6_SIZE);
        pstStreamNode->usServPort = pstMsgDes->usServPort;
        pstStreamNode->ulStreamID = pstMsgDes->ulStreamID;
        pstStreamNode->enDataType = (PT_DATA_TYPE_EN)pstMsgDes->enDataType;
    }

    /* �����ݲ��뵽data queue�� */
    lResult = pt_recv_data_tcp_queue_insert(pstStreamNode, pstMsgDes, acRecvBuf+sizeof(PT_MSG_TAG), lDataLen-sizeof(PT_MSG_TAG), PT_DATA_RECV_CACHE_SIZE);
    if (lResult < 0)
    {
        pt_logr_info("ptc_save_into_recv_cache : add data into recv cache fail");
        return PT_SAVE_DATA_FAIL;
    }

    /* ÿ64��������Ƿ������� */
    if (pstMsgDes->lSeq - pstStreamNode->lConfirmSeq >= PT_CONFIRM_RECV_MSG_SIZE)
    {
        i = pstStreamNode->lConfirmSeq + 1;
        for (i=0; i<PT_CONFIRM_RECV_MSG_SIZE; i++)
        {
            ulArraySub = (pstStreamNode->lConfirmSeq + 1 + i) & (PT_DATA_RECV_CACHE_SIZE - 1);  /*������,������������е�λ��*/
            if (pstStreamNode->unDataQueHead.pstDataTcp[ulArraySub].lSeq != pstStreamNode->lConfirmSeq + 1 + i)
            {
                break;
            }
        }

        if (i == PT_CONFIRM_RECV_MSG_SIZE)
        {
            /* ����ȷ�Ͻ�����Ϣ */
            pt_logr_debug("send make sure msg : %d", pstStreamNode->lConfirmSeq + PT_CONFIRM_RECV_MSG_SIZE);
            ptc_send_confirm_msg(pstMsgDes, pstStreamNode->lConfirmSeq + PT_CONFIRM_RECV_MSG_SIZE);
            pstStreamNode->lConfirmSeq = pstStreamNode->lConfirmSeq + PT_CONFIRM_RECV_MSG_SIZE;
        }
    }

    if (PT_NEED_CUT_PTHREAD == lResult)
    {
        /* �����Ѵ��� */
        return PT_NEED_CUT_PTHREAD;
    }
    else
    {
        /* �ж��Ƿ񶪰����洢���Ƿ��ǿ��Է��͵İ� */
        if (pstStreamNode->lCurrSeq + 1 == pstMsgDes->lSeq)
        {
            if (NULL != pstStreamNode->hTmrHandle)
            {
                /* ������ڶ�ʱ�����������Ϊ��ʧ����С�� */
                pstStreamNode->ulCountResend = 0;
            }
            return PT_SAVE_DATA_SUCC;
        }
        else if (pstMsgDes->lSeq < pstStreamNode->lCurrSeq + 1)
        {
            /* �ѷ��͹��İ� */
            pt_logr_info("stream id %d, seq is %d, currseq is %d", pstMsgDes->ulStreamID, pstMsgDes->lSeq, pstStreamNode->lCurrSeq);
            return PT_SAVE_DATA_FAIL;
        }
        else
        {
            ulNextSendArraySub = (pstStreamNode->lCurrSeq + 1) & (PT_DATA_RECV_CACHE_SIZE - 1);
            pstDataQueue = pstStreamNode->unDataQueHead.pstDataTcp;
            if (pstDataQueue[ulNextSendArraySub].lSeq == pstStreamNode->lCurrSeq + 1)
            {
                return PT_SAVE_DATA_SUCC;
            }

            if (pstMsgDes->lSeq == 0 && pstStreamNode->lCurrSeq == -1)
            {
                return PT_SAVE_DATA_SUCC;
            }

            /* ���������û�ж�ʱ���ģ�������ʱ�� */
            if (NULL == pstStreamNode->hTmrHandle)
            {
                pt_logr_debug("lose data, start timer, seq : %d", pstStreamNode->lCurrSeq + 1);
                if (pstStreamNode->pstLostParam == NULL)
                {
                    pstLoseMsg = (PT_LOSE_BAG_MSG_ST *)dos_dmem_alloc(sizeof(PT_LOSE_BAG_MSG_ST));
                    if (NULL == pstLoseMsg)
                    {
                        pt_logr_error("ptc_save_into_recv_cache : malloc");

                        return PT_SAVE_DATA_FAIL;
                    }

                    pstLoseMsg->stMsg = *pstMsgDes;
                    pstLoseMsg->pstStreamNode = pstStreamNode;

                    pstStreamNode->pstLostParam = pstLoseMsg;
                }
                pstStreamNode->ulCountResend = 0;
                ptc_send_lost_data_req((U64)pstStreamNode->pstLostParam);
                lResult = dos_tmr_start(&pstStreamNode->hTmrHandle, PT_SEND_LOSE_DATA_TIMER, ptc_send_lost_data_req, (U64)pstStreamNode->pstLostParam, TIMER_NORMAL_LOOP);
                if (PT_SAVE_DATA_FAIL == lResult)
                {
                    pt_logr_info("ptc_save_into_recv_cache : start timer fail");

                    return PT_SAVE_DATA_FAIL;
                }

            }

            pt_logr_info("ptc_save_into_recv_cache : time exited");

            return PT_SAVE_DATA_FAIL;
        }
    }
}

/**
 * ������BOOL ptc_upgrade(PT_DATA_TCP_ST *pstRecvDataTcp, PT_STREAM_CB_ST *pstStreamNode)
 * ���ܣ�
 *      1.����������������ptc
 * ����
 * ����ֵ�� DOS_FALSE   : break;
 *          DOS_TRUE    : continue;
 */
BOOL ptc_save_upgrade_package(S8 *szBuff, U32 ulLen)
{
    LOAD_FILE_TAG   *pstUpgrade = NULL;
    FILE            *pFileFd    = NULL;
    S32             lFd         = 0;
    U32             usCrc       = 0;
    S8 szServiceRoot[PT_DATA_BUFF_64] = {0};
    S8 szOldFile[PT_DATA_BUFF_128] = {0};
    S8 szNewFile[PT_DATA_BUFF_128] = {0};
    S8 szSystemCmd[PT_DATA_BUFF_256] = {0};

    if (DOS_ADDR_INVALID(g_pPackageBuff))
    {
        g_ulReceivedLen = 0;
        pstUpgrade = (LOAD_FILE_TAG *)szBuff;
        g_ulPackageLen = dos_ntohl(pstUpgrade->ulFileLength);
        g_usCrc = dos_ntohl(pstUpgrade->usCRC);
        if (g_ulPackageLen <= 0)
        {
            /* ʧ�� */
            logr_info("recv upgrade package fail");
        }
        else
        {
            logr_debug("start recv upgrade package");
            g_pPackageBuff = (S8 *)dos_dmem_alloc(g_ulPackageLen + sizeof(LOAD_FILE_TAG));
            if (NULL == g_pPackageBuff)
            {
                DOS_ASSERT(0);
                logr_debug("malloc fail\n");
                return DOS_FALSE;
            }
            else
            {
                dos_memzero(g_pPackageBuff, g_ulPackageLen + sizeof(LOAD_FILE_TAG));
            }
        }

        dos_memcpy(g_pPackageBuff+g_ulReceivedLen, szBuff, ulLen);
        g_ulReceivedLen += ulLen;

        return DOS_TRUE;
    }

    dos_memcpy(g_pPackageBuff+g_ulReceivedLen, szBuff, ulLen);
    g_ulReceivedLen += ulLen;

    if (ulLen < PT_RECV_DATA_SIZE)
    {
        /* ������ɣ�CRC��֤ */
        usCrc = load_calc_crc((U8 *)g_pPackageBuff + sizeof(LOAD_FILE_TAG), g_ulReceivedLen-sizeof(LOAD_FILE_TAG));
        printf("g_usCrc : %u, usCrc = %u, file len : %u, recv len : %u\n\n", g_usCrc, usCrc, g_ulPackageLen, g_ulReceivedLen);
        if (usCrc == g_usCrc)
        {
            logr_debug("recv upgrade package succ");
            if (config_get_service_root(szServiceRoot, sizeof(szServiceRoot)) == NULL)
            {
                logr_debug("get service root fail");
                return DOS_FAIL;
            }

            dos_snprintf(szOldFile, PT_DATA_BUFF_128, "%s/bin/ptcd_old", szServiceRoot);
            dos_snprintf(szNewFile, PT_DATA_BUFF_128, "%s/bin/ptcd", szServiceRoot);
            dos_snprintf(szSystemCmd, PT_DATA_BUFF_256, "mv %s %s", szNewFile, szOldFile);
            /* ��֤ͨ�������浽�ļ��� */
            system(szSystemCmd);

            pFileFd = fopen("/dipcc/bin/ptcd", "w");
            if (NULL == pFileFd)
            {
                pt_logr_info("create file file\n");
                dos_snprintf(szSystemCmd, PT_DATA_BUFF_256, "mv %s %s", szOldFile, szNewFile);
                system("szSystemCmd");
            }
            else
            {
                lFd = fileno(pFileFd);
                fchmod(lFd, 0777);
                fwrite(g_pPackageBuff+sizeof(LOAD_FILE_TAG), g_ulPackageLen-sizeof(LOAD_FILE_TAG), 1, pFileFd);
                fclose(pFileFd);
                pFileFd = NULL;
                ptc_send_exit_notify_to_pts(PT_DATA_CTRL, PT_CTRL_PTC_PACKAGE, 0);
                ptc_send_logout2pts();
                /* ptc �˳�����, �ȴ����� */
                //exit(0);
                kill(getpid(), SIGUSR1);
            }
            dos_dmem_free(g_pPackageBuff);
            g_pPackageBuff = NULL;
        }
        else
        {
            /* ��֤��ͨ�� */
            logr_info("upgrade package error : md5 error");
        }

        ptc_delete_send_stream_node_by_streamID(PT_CTRL_PTC_PACKAGE);
        ptc_send_exit_notify_to_pts(PT_DATA_CTRL, PT_CTRL_PTC_PACKAGE, 0);

        return DOS_FALSE;
    }

    return DOS_TRUE;
}

VOID ptc_recv_upgrade_package(PT_NEND_RECV_NODE_ST *pstNeedRecvNode)
{
    U32               ulArraySub       = 0;
    PT_STREAM_CB_ST   *pstStreamList   = NULL;
    PT_STREAM_CB_ST   *pstStreamNode   = NULL;
    PT_DATA_TCP_ST    stRecvDataTcp;
    S32               lResult          = 0;

     pstStreamList = g_pstPtcRecv->pstStreamHead;
    if (DOS_ADDR_INVALID(pstStreamList))
    {
        return;
    }

    pstStreamNode = pt_stream_queue_search(pstStreamList, pstNeedRecvNode->ulStreamID);
    if (NULL == pstStreamNode)
    {
        return;
    }

    if (NULL == pstStreamNode->unDataQueHead.pstDataTcp)
    {
        return;
    }
    while(1)
    {
        pstStreamNode->lCurrSeq++;
        ulArraySub = (pstStreamNode->lCurrSeq) & (PT_DATA_RECV_CACHE_SIZE - 1);
        stRecvDataTcp = pstStreamNode->unDataQueHead.pstDataTcp[ulArraySub];
        if (stRecvDataTcp.lSeq == pstStreamNode->lCurrSeq)
        {
            lResult = ptc_save_upgrade_package(stRecvDataTcp.szBuff, stRecvDataTcp.ulLen);
            if (!lResult)
            {
                break;
            }
            continue;
        }
        else
        {
            /* ���seqû�з��ͣ���һ */
            pstStreamNode->lCurrSeq--;
            break;
        }
    }

    return;
}

VOID ptc_update_pts_history_file(U8 aulServIp[IPV6_SIZE], U16 usServPort)
{
    S8 szPtsIp[PT_IP_ADDR_SIZE] = {0};
    FILE *pFileHandle = NULL;
    S32 lPtsHistoryConut = 0;
    S8 szPtsHistory[3][PT_DATA_BUFF_64];

    inet_ntop(AF_INET, (void *)(aulServIp), szPtsIp, PT_IP_ADDR_SIZE);
    pFileHandle = fopen(g_stServMsg.szHistoryPath, "r");
    if (NULL == pFileHandle)
    {
        return;
    }

    while (!feof(pFileHandle) && lPtsHistoryConut < 3)
    {
        fgets(szPtsHistory[lPtsHistoryConut], PT_DATA_BUFF_64, pFileHandle);
        printf("lPtsHistoryConut : %d, %s\n", lPtsHistoryConut, szPtsHistory[lPtsHistoryConut]);
        lPtsHistoryConut++;
    }
    fclose(pFileHandle);
    lPtsHistoryConut--;
    printf("switch pts count : %d\n", lPtsHistoryConut);
    pFileHandle = fopen(g_stServMsg.szHistoryPath, "w");
    if (NULL == pFileHandle)
    {
        return;
    }

    if (0 == lPtsHistoryConut)
    {
        fprintf(pFileHandle, "%s&%d\n", szPtsIp, dos_ntohs(usServPort));
    }
    else if (1 == lPtsHistoryConut)
    {
        fprintf(pFileHandle, "%s&%d\n%s", szPtsIp, dos_ntohs(usServPort), szPtsHistory[0]);
    }
    else
    {
        fprintf(pFileHandle, "%s&%d\n%s%s", szPtsIp, dos_ntohs(usServPort), szPtsHistory[0], szPtsHistory[1]);
    }
    fclose(pFileHandle);

    ptc_get_pts_history();
    return;
}

/**
 * ������void ptc_get_hb_time_interval(struct timeval stHBEndTime)
 * ���ܣ�
 *      1.����������������Ӧ֮���ʱ����
 * ����
 *
 * ����ֵ����
 */
void ptc_get_hb_time_interval(struct timeval stHBEndTime)
{
    S32 lSec = stHBEndTime.tv_sec - g_stHBStartTime.tv_sec;
    S32 lUsec = stHBEndTime.tv_usec - g_stHBStartTime.tv_usec;

    g_lHBTimeInterval = lSec * 1000 * 1000 + lUsec;
}

/**
 * ������VOID ptc_ctrl_msg_handle(U32 ulStreamID, S8 *pData)
 * ���ܣ�
 *      1.���������Ϣ
 * ����
 *
 * ����ֵ����
 */
VOID ptc_ctrl_msg_handle(S8 *pData, U32 lRecvLen)
{
    if (DOS_ADDR_INVALID(pData))
    {
        return;
    }
    PT_CTRL_DATA_ST *pstCtrlData = NULL;
    PT_CTRL_DATA_ST stVerRet;
    PT_CTRL_DATA_ST stCtrlData;
    PT_MSG_TAG      *pstMsgDes = (PT_MSG_TAG *)pData;
    struct timeval  stHBEndTime;
    S8  szDestKey[PT_LOGIN_VERIFY_SIZE]  = {0};
    S8  acBuff[sizeof(PT_CTRL_DATA_ST)]  = {0};
    S8  szPtcName[PT_PTC_NAME_LEN]       = {0};
    S8  szPtsIp[PT_IP_ADDR_SIZE]         = {0};
    S8  szPtsPort[PT_DATA_BUFF_16]       = {0};
    U8  paucIPAddr[IPV6_SIZE]            = {0};
    S32 lResult = 0;

    pstCtrlData = (PT_CTRL_DATA_ST *)(pData + sizeof(PT_MSG_TAG));
    switch (pstMsgDes->ulStreamID)
    {
    case PT_CTRL_LOGIN_RSP:
        /* ��½��֤ */
        pt_logr_debug("recv login response");
        lResult = ptc_get_udp_user_ip();  /* ��ȡ����ip */
        if (lResult != DOS_SUCC)
        {
            return;
        }

        lResult = ptc_key_convert(pstCtrlData->szLoginVerify, szDestKey);
        if (lResult < 0)
        {
            break;
        }

        /* ��ȡptc���� */
        lResult = config_get_ptc_name(szPtcName, PT_PTC_NAME_LEN);
        if (lResult != DOS_SUCC)
        {
            pt_logr_info("get ptc name fail\n");
            dos_memzero(stVerRet.szPtcName, PT_PTC_NAME_LEN);
        }
        else
        {
            dos_memcpy(stVerRet.szPtcName, szPtcName, PT_PTC_NAME_LEN);
        }

        /* ����key��pts */
        dos_memcpy(stVerRet.szLoginVerify, szDestKey, PT_LOGIN_VERIFY_SIZE);
        stVerRet.enCtrlType = PT_CTRL_LOGIN_RSP;
        stVerRet.ulLginVerSeq = dos_htonl(pstMsgDes->lSeq);
        stVerRet.enPtcType = g_enPtcType;
        dos_strcpy(stVerRet.achPtsMajorDomain, g_stServMsg.achPtsMajorDomain);
        dos_strcpy(stVerRet.achPtsMinorDomain, g_stServMsg.achPtsMinorDomain);
        stVerRet.usPtsMajorPort = g_stServMsg.usPtsMajorPort;
        stVerRet.usPtsMinorPort = g_stServMsg.usPtsMinorPort;
        ptc_get_version(stVerRet.szVersion);
        dos_strcpy(stVerRet.szPtsHistoryIp1, g_stServMsg.szPtsLasterIP1);
        dos_strcpy(stVerRet.szPtsHistoryIp2, g_stServMsg.szPtsLasterIP2);
        dos_strcpy(stVerRet.szPtsHistoryIp3, g_stServMsg.szPtsLasterIP3);
        stVerRet.usPtsHistoryPort1 = g_stServMsg.usPtsLasterPort1;
        stVerRet.usPtsHistoryPort2 = g_stServMsg.usPtsLasterPort2;
        stVerRet.usPtsHistoryPort3 = g_stServMsg.usPtsLasterPort3;
        dos_strncpy(stVerRet.szMac, g_stServMsg.szMac, PT_DATA_BUFF_64);
        dos_memcpy(acBuff, (VOID *)&stVerRet, sizeof(PT_CTRL_DATA_ST));

        ptc_send_login_rsp_to_pts(&stVerRet);

        break;
    case PT_CTRL_LOGIN_ACK:
        /* ��½��� */
        switch (pstCtrlData->ucLoginRes)
        {
            case PT_PTC_LOGIN_SUCC:
                /* �ɹ� */
                pt_logr_notic("ptc login succ");
                g_bIsOnLine = DOS_TRUE;
                g_ulHDTimeoutCount = 0;
                g_ulConnectPtsCount = 0;
                break;
            case PT_PTC_LOGIN_FAIL_LICENSE:
                pt_logr_notic("ptc login fail, the license do not allow");
                g_bIsOnLine = DOS_FALSE;
                break;
            case PT_PTC_LOGIN_FAIL_SN:
                pt_logr_notic("ptc login fail, request login ipcc SN format error");
                g_bIsOnLine = DOS_FALSE;
                break;
            case PT_PTC_LOGIN_FAIL_VERIFY:
                pt_logr_notic("ptc login fail, verify fail");
                g_bIsOnLine = DOS_FALSE;
                break;
            default:
                pt_logr_notic("ptc login fail, default");
                g_bIsOnLine = DOS_FALSE;
                break;
        }

        /* ֪ͨpts�������¼�Ļ��� */
        ptc_send_exit_notify_to_pts(PT_DATA_CTRL, PT_CTRL_LOGIN_RSP, 0);
        break;
    case PT_CTRL_HB_RSP:
        /* ������Ӧ���رն�ʱ�� */
        gettimeofday(&stHBEndTime, NULL);
        ptc_get_hb_time_interval(stHBEndTime);
        pt_logr_debug("recv from pts hb rsp");
        if (DOS_ADDR_VALID(g_stACKTmrHandle))
        {
            dos_tmr_stop(&g_stACKTmrHandle);
        }
        g_stACKTmrHandle = NULL;
        g_ulHDTimeoutCount = 0;
        break;
    case PT_CTRL_SWITCH_PTS:
        /* �л�pts */
        ptc_send_logout2pts();
        g_pstPtcSend->stDestAddr.sin_addr.s_addr = *(U32 *)(pstMsgDes->aulServIp);
        g_pstPtcSend->stDestAddr.sin_port = pstMsgDes->usServPort;

        /* д���ļ�,��¼ע���pts����ʷ��¼ */
        //if (*(U32 *)pstMsgDes->aulServIp != *(U32 *)g_stServMsg.achPtsMajorIP && *(U32 *)pstMsgDes->aulServIp != *(U32 *)g_stServMsg.achPtsMinorIP)
        ptc_update_pts_history_file(pstMsgDes->aulServIp, pstMsgDes->usServPort);
        ptc_send_login2pts();
        break;
    case PT_CTRL_PTS_MAJOR_DOMAIN:
        /* �޸������� */
        dos_memzero(&stCtrlData, sizeof(PT_CTRL_DATA_ST));
        stCtrlData.enCtrlType = PT_CTRL_PTS_MAJOR_DOMAIN;
        if (pstCtrlData->achPtsMajorDomain[0] != '\0')
        {
            /* �������� */
            if (pt_is_or_not_ip(pstCtrlData->achPtsMajorDomain))
            {
                if (DOS_SUCC == config_set_pts_major_domain(pstCtrlData->achPtsMajorDomain))
                {
                    if (!config_save())
                    {
                        /* ����ɹ� */
                        dos_strncpy(szPtsIp, pstCtrlData->achPtsMajorDomain, PT_IP_ADDR_SIZE);
                        inet_pton(AF_INET, szPtsIp, (VOID *)(g_stServMsg.achPtsMajorIP));
                        dos_strncpy(g_stServMsg.achPtsMajorDomain, pstCtrlData->achPtsMajorDomain, PT_DATA_BUFF_64-1);
                        dos_strncpy(stCtrlData.achPtsMajorDomain, pstCtrlData->achPtsMajorDomain, PT_DATA_BUFF_64-1);
                    }
                }
            }
            else
            {
                lResult = pt_DNS_analyze(pstCtrlData->achPtsMajorDomain, paucIPAddr);
                if (lResult <= 0)
                {
                    logr_info("1DNS fail");
                }
                else
                {
                    inet_ntop(AF_INET, (void *)(paucIPAddr), szPtsIp, PT_IP_ADDR_SIZE);
                    if (DOS_SUCC == config_set_pts_major_domain(pstCtrlData->achPtsMajorDomain))
                    {
                        if (!config_save())
                        {
                            /* ����ɹ� */
                            inet_pton(AF_INET, szPtsIp, (VOID *)(g_stServMsg.achPtsMajorIP));
                            dos_strncpy(g_stServMsg.achPtsMajorDomain, pstCtrlData->achPtsMajorDomain, PT_DATA_BUFF_64-1);
                            dos_strncpy(stCtrlData.achPtsMajorDomain, pstCtrlData->achPtsMajorDomain, PT_DATA_BUFF_64-1);
                        }
                    }
                }
            }
        }

        if (pstCtrlData->usPtsMajorPort != 0)
        {
            sprintf(szPtsPort, "%d", dos_ntohs(pstCtrlData->usPtsMajorPort));
            if (DOS_SUCC == config_set_pts_major_port(szPtsPort))
            {
                if (!config_save())
                {
                    g_stServMsg.usPtsMajorPort = pstCtrlData->usPtsMajorPort;
                    stCtrlData.usPtsMajorPort = pstCtrlData->usPtsMajorPort;
                }
            }
        }

        ptc_save_ctrl_msg_into_cache(PT_CTRL_PTS_MAJOR_DOMAIN, (S8 *)&stCtrlData, sizeof(PT_CTRL_DATA_ST));
        break;
    case PT_CTRL_PTS_MINOR_DOMAIN:
        /* �޸ı����� */
        dos_memzero(&stCtrlData, sizeof(PT_CTRL_DATA_ST));
        stCtrlData.enCtrlType = PT_CTRL_PTS_MINOR_DOMAIN;
        if (pstCtrlData->achPtsMinorDomain[0] != '\0')
        {
            /* �������� */
            if (pt_is_or_not_ip(pstCtrlData->achPtsMinorDomain))
            {
                if (DOS_SUCC == config_set_pts_minor_domain(pstCtrlData->achPtsMinorDomain))
                {
                    if (!config_save())
                    {
                        /* ����ɹ� */
                        dos_strncpy(szPtsIp, pstCtrlData->achPtsMinorDomain, PT_IP_ADDR_SIZE);
                        inet_pton(AF_INET, szPtsIp, (VOID *)(g_stServMsg.achPtsMajorIP));
                        dos_strncpy(g_stServMsg.achPtsMinorDomain, pstCtrlData->achPtsMinorDomain, PT_DATA_BUFF_64-1);
                        dos_strncpy(stCtrlData.achPtsMinorDomain, pstCtrlData->achPtsMinorDomain, PT_DATA_BUFF_64-1);
                    }
                }
            }
            else
            {
                lResult = pt_DNS_analyze(pstCtrlData->achPtsMinorDomain, paucIPAddr);
                if (lResult <= 0)
                {
                    logr_info("1DNS fail");
                }
                else
                {
                    inet_ntop(AF_INET, (void *)(paucIPAddr), szPtsIp, PT_IP_ADDR_SIZE);
                    if (DOS_SUCC == config_set_pts_minor_domain(pstCtrlData->achPtsMinorDomain))
                    {
                        if (!config_save())
                        {
                            /* ����ɹ� */
                            inet_pton(AF_INET, szPtsIp, (VOID *)(g_stServMsg.achPtsMajorIP));
                            dos_strncpy(g_stServMsg.achPtsMinorDomain, pstCtrlData->achPtsMinorDomain, PT_DATA_BUFF_64-1);
                            dos_strncpy(stCtrlData.achPtsMinorDomain, pstCtrlData->achPtsMinorDomain, PT_DATA_BUFF_64-1);
                        }
                    }
                }
            }
        }

        if (pstCtrlData->usPtsMinorPort != 0)
        {
            sprintf(szPtsPort, "%d", dos_ntohs(pstCtrlData->usPtsMinorPort));
            if (DOS_SUCC == config_set_pts_minor_port(szPtsPort))
            {
                if (!config_save())
                {
                    g_stServMsg.usPtsMinorPort = pstCtrlData->usPtsMinorPort;
                    stCtrlData.usPtsMinorPort = pstCtrlData->usPtsMinorPort;
                }
            }
        }

        ptc_save_ctrl_msg_into_cache(PT_CTRL_PTS_MINOR_DOMAIN, (S8 *)&stCtrlData, sizeof(PT_CTRL_DATA_ST));
        break;
    case PT_CTRL_PING:
        ptc_save_ctrl_msg_into_cache(PT_CTRL_PING_ACK, pData + sizeof(PT_MSG_TAG), lRecvLen - sizeof(PT_MSG_TAG));
        break;
    default:
        break;
    }

    return;
}

/**
 * ������S32 ptc_deal_with_confirm_msg(PT_MSG_TAG *pstMsgDes)
 * ���ܣ�
 *      1.������Ϣȷ����Ϣ
 * ����
 *
 * ����ֵ����
 */
S32 ptc_deal_with_confirm_msg(PT_MSG_TAG *pstMsgDes)
{
    PT_STREAM_CB_ST    *pstStreamListHead  = NULL;
    PT_STREAM_CB_ST    *pstStreamNode      = NULL;
    S32 lResult = DOS_FALSE;

    pstStreamListHead = g_pstPtcSend->pstStreamHead;
    pstStreamNode = pt_stream_queue_search(pstStreamListHead, pstMsgDes->ulStreamID);
    if (NULL == pstStreamNode)
    {
        return lResult;
    }

    if (pstStreamNode->lConfirmSeq < pstMsgDes->lSeq)
    {
        pstStreamNode->lConfirmSeq = pstMsgDes->lSeq;
        lResult = DOS_TRUE;
    }

    return lResult;
}

/**
 * ������S8 *ptc_get_version()
 * ���ܣ�
 *      1.��ȡ�汾��
 * ����
 * ����ֵ��
 */
S32 ptc_get_version(U8 *szVersion)
{
    if (NULL == szVersion)
    {
        return DOS_FALSE;
    }

    inet_pton(AF_INET, DOS_PROCESS_VERSION, (VOID *)szVersion);

    return DOS_SUCC;
}

/**
 * ������VOID ptc_send_heartbeat2pts(S32 lSockfd)
 * ���ܣ�
 *      1.������ʱ����������������
 * ����
 * ����ֵ��
 */
VOID ptc_send_heartbeat2pts()
{
    S32 lResult = 0;

    lResult = dos_tmr_start(&g_stHBTmrHandle, PTC_SEND_HB_TIME, ptc_send_hb_req, (U64)g_ulUdpSocket, TIMER_NORMAL_LOOP);
    if (lResult < 0)
    {
        pt_logr_info("ptc_send_heartbeat2pts : start timer fail");
    }
}

/**
 * ������VOID ptc_send_login2pts(S32 lSockfd)
 * ���ܣ�
 *      1.������ʱ�������͵�½����
 * ����
 * ����ֵ��
 */
VOID ptc_send_login2pts()
{
    S32 lResult = 0;
    ptc_send_login_req(0);
    lResult = dos_tmr_start(&g_stHBTmrHandle, PTC_SEND_HB_TIME, ptc_send_login_req, 0, TIMER_NORMAL_LOOP);
    if (lResult < 0)
    {
        pt_logr_info("ptc_send_login2pts : start timer fail");
    }
}

/**
 * ������VOID ptc_send_logout2pts(S32 lSockfd)
 * ���ܣ�
 *      1.������ʱ���������˳���½����
 * ����
 * ����ֵ��
 */
VOID ptc_send_logout2pts()
{
    g_bIsOnLine = DOS_FALSE;
    dos_tmr_stop(&g_stHBTmrHandle);
    ptc_send_logout_req();
}

BOOL ptc_cache_is_or_not_have_space(PT_STREAM_CB_ST *pstSendStreamNode)
{
    if (DOS_ADDR_INVALID(pstSendStreamNode))
    {
        return DOS_FALSE;
    }

    if (pstSendStreamNode->lMaxSeq - pstSendStreamNode->lConfirmSeq >= PT_DATA_SEND_CACHE_SIZE - 1)
    {
        return DOS_FALSE;
    }

    return DOS_TRUE;
}

/**
 * ������void ptc_ipcc_send_msg(PT_DATA_TYPE_EN enDataType, U32 ulStreamID, S8 *pcData, S32 lDataLen, U8 ExitNotifyFlag)
 * ���ܣ�
 *      1.������ݵ����ͻ�����
 *      2.�����ӳɹ�����֪ͨ�����̣߳���������
 * ����
 *      PT_DATA_TYPE_EN enDataType    ��data������
 *      U32 ulStreamID                ��stream ID
 *      S8 *pcData                    ��������
 *      S32 lDataLen                  �������ݳ���
 *      U8 ExitNotifyFlag             ��֪ͨ�Է���Ӧ�Ƿ����
 * ����ֵ����
 */
S32 ptc_save_msg_into_cache(PT_STREAM_CB_ST *pstStreamNode, PT_DATA_TYPE_EN enDataType, U32 ulStreamID, S8 *acSendBuf, S32 lDataLen)
{
    S32 lResult = 0;
    PT_MSG_TAG stMsgDes;
    BOOL bIsResend = DOS_FALSE;
    PT_CMD_EN enCmdValue = PT_CMD_NORMAL;
    PT_STREAM_CB_ST *pstStreamListHead      = NULL;

    if (DOS_ADDR_INVALID(acSendBuf))
    {
        return PT_SAVE_DATA_FAIL;
    }

    if (DOS_ADDR_INVALID(pstStreamNode))
    {
        pstStreamListHead = g_pstPtcSend->pstStreamHead;
        pstStreamNode = pt_stream_queue_search(pstStreamListHead, ulStreamID);
        if (DOS_ADDR_INVALID(pstStreamNode))
        {
            return PT_SAVE_DATA_FAIL;
        }
    }

      /* �����ݲ��뵽data queue�� */
    lResult = pt_send_data_tcp_queue_insert(pstStreamNode, acSendBuf, lDataLen, PT_DATA_SEND_CACHE_SIZE, 0);
    if (lResult < 0)
    {
        pt_logr_debug("save into cache fail");
        /* ��ӵ����ͻ���ʧ�� */
        return PT_SAVE_DATA_FAIL;
    }
    else
    {
        stMsgDes.ExitNotifyFlag = DOS_FALSE;
        stMsgDes.ulStreamID     = ulStreamID;
        stMsgDes.enDataType     = enDataType;
        /* ��ӵ�������Ϣ���� */
        pthread_mutex_lock(&g_mutexPtcSendPthread);
        if (NULL == pt_need_send_node_list_search(&g_stPtcNendSendNode, ulStreamID))
        {
            pt_need_send_node_list_insert(&g_stPtcNendSendNode, g_pstPtcSend->aucID, &stMsgDes, enCmdValue, bIsResend, DOS_FALSE);
            pt_logr_debug("save into send pts list succ, count : %d", g_ulPtcNendSendNodeCount);
        }
        pthread_cond_signal(&g_condPtcSend);
        pthread_mutex_unlock(&g_mutexPtcSendPthread);
    }

    usleep(10); /* ��֤�л��߳� */

    return lResult;
}

S32 ptc_save_ctrl_msg_into_cache(U32 ulStreamID, S8 *acSendBuf, S32 lDataLen)
{
    PT_STREAM_CB_ST *pstStreamListHead      = NULL;
    PT_STREAM_CB_ST *pstRecvStreamListHead  = NULL;
    PT_STREAM_CB_ST *pstStreamNode          = NULL;
    S32             lResult                 = 0;
    S32             i                       = 0;
    BOOL            bIsResend               = DOS_FALSE;
    PT_CMD_EN       enCmdValue              = PT_CMD_NORMAL;
    PT_MSG_TAG      stMsgDes;

    if (DOS_ADDR_INVALID(acSendBuf))
    {
        return PT_SAVE_DATA_FAIL;
    }

    pstStreamListHead     = g_pstPtcSend->pstStreamHead;
    pstRecvStreamListHead = g_pstPtcRecv->pstStreamHead;
    pstStreamNode = pt_stream_queue_search(pstStreamListHead, ulStreamID);
    if (DOS_ADDR_INVALID(pstStreamNode))
    {
         /* ������ */
        i =  pt_stream_queue_get_node(pstRecvStreamListHead);
        if (i < 0 || i >= PTC_STREAMID_MAX_COUNT)
        {
            return PT_SAVE_DATA_FAIL;
        }
        pstStreamNode = &pstStreamListHead[i];
        /* ��ʼ����Ӧ�ķ��ͻ��� */
        pt_stream_node_init(pstStreamNode);
        pstStreamNode->bIsValid = DOS_TRUE;
        pstStreamNode->ulStreamID = ulStreamID;
        pstStreamNode->enDataType = PT_DATA_CTRL;

        pstRecvStreamListHead->ulStreamID = ulStreamID;
        pstRecvStreamListHead->enDataType = PT_DATA_CTRL;
    }

      /* �����ݲ��뵽data queue�� */
    lResult = pt_send_data_tcp_queue_insert(pstStreamNode, acSendBuf, lDataLen, PT_DATA_SEND_CACHE_SIZE, 0);
    if (lResult < 0)
    {
        pt_logr_debug("save ctrl msg into cache fail");
        return PT_SAVE_DATA_FAIL;
    }
    else
    {
        stMsgDes.ExitNotifyFlag = DOS_FALSE;
        stMsgDes.ulStreamID     = ulStreamID;
        stMsgDes.enDataType     = PT_DATA_CTRL;
        /* ��ӵ�������Ϣ���� */
        pthread_mutex_lock(&g_mutexPtcSendPthread);
        if (NULL == pt_need_send_node_list_search(&g_stPtcNendSendNode, ulStreamID))
        {
            pt_need_send_node_list_insert(&g_stPtcNendSendNode, g_pstPtcSend->aucID, &stMsgDes, enCmdValue, bIsResend, DOS_FALSE);
            pt_logr_debug("save into send pts list succ, count : %d", g_ulPtcNendSendNodeCount);
        }
        pthread_cond_signal(&g_condPtcSend);
        pthread_mutex_unlock(&g_mutexPtcSendPthread);
    }

    return lResult;
}

/**
 * ������void *ptc_send_msg2pts(VOID *arg)
 * ���ܣ�
 *      1.���������߳�
 * ����
 *      VOID *arg :ͨ��ͨ�ŵ�sockfd
 * ����ֵ��void *
 */
VOID *ptc_send_msg2pts(VOID *arg)
{
    U32              ulArraySub       = 0;
    PT_STREAM_CB_ST  *pstStreamHead   = NULL;
    list_t           *pstNendSendList = NULL;
    PT_STREAM_CB_ST  *pstStreamNode   = NULL;
    PT_DATA_TCP_ST   *pstSendDataHead = NULL;
    PT_DATA_TCP_ST   stSendDataNode;
    S8 acBuff[PT_SEND_DATA_SIZE]      = {0};
    PT_MSG_TAG       *pstMsgDes       = (PT_MSG_TAG *)acBuff;
    PT_NEND_SEND_NODE_ST *pstNeedSendNode = NULL;
    PT_DATA_TCP_ST    stRecvDataTcp;
    struct timeval now;
    struct timespec timeout;
    U32 ulCount = 0;

    dos_list_init(&g_stPtcNendRecvNode);
    dos_list_init(&g_stPtcNendSendNode);
    dos_list_init(&g_stPtcReSendNode);

    while (1)
    {
        pthread_mutex_lock(&g_mutexPtcSendPthread);
        gettimeofday(&now, NULL);
        timeout.tv_sec = now.tv_sec + 1;
        timeout.tv_nsec = (now.tv_usec) * 1000;
        pthread_cond_timedwait(&g_condPtcSend, &g_mutexPtcSendPthread, &timeout);
        pthread_mutex_unlock(&g_mutexPtcSendPthread);

        while(1)
        {
            ulCount = g_ulPtcNendSendNodeCount;
            if (0 == g_ulPtcNendSendNodeCount)
            {
                break;
            }

            if (dos_list_is_empty(&g_stPtcNendSendNode))
            {
                g_ulPtcNendSendNodeCount = 0;
                break;
            }
            if (ulCount < 3)
            {
                pthread_mutex_lock(&g_mutexPtcSendPthread);
            }
            pstNendSendList = dos_list_fetch(&g_stPtcNendSendNode);
            if (DOS_ADDR_INVALID(pstNendSendList))
            {
                if (ulCount < 3)
                {
                    pthread_mutex_unlock(&g_mutexPtcSendPthread);
                }
                DOS_ASSERT(0);
                continue;
            }

            if (g_ulPtcNendSendNodeCount > 0)
            {
                g_ulPtcNendSendNodeCount--;
            }

            if (ulCount < 3)
            {
                pthread_mutex_unlock(&g_mutexPtcSendPthread);
            }
            dos_memzero(acBuff, PT_SEND_DATA_SIZE);
            pstNeedSendNode = dos_list_entry(pstNendSendList, PT_NEND_SEND_NODE_ST, stListNode);
            if (pstNeedSendNode->enCmdValue != PT_CMD_NORMAL || pstNeedSendNode->ExitNotifyFlag == DOS_TRUE)
            {
                /* ���� lost/resend/confirm/exitNotify��Ϣ */
                pstMsgDes->enDataType = pstNeedSendNode->enDataType;
                dos_memcpy(pstMsgDes->aucID, g_pstPtcSend->aucID, PTC_ID_LEN);
                pstMsgDes->ulStreamID = dos_htonl(pstNeedSendNode->ulStreamID);
                pstMsgDes->ExitNotifyFlag = pstNeedSendNode->ExitNotifyFlag;
                pstMsgDes->lSeq = dos_htonl(pstNeedSendNode->lSeqResend);
                pstMsgDes->enCmdValue = pstNeedSendNode->enCmdValue;
                pstMsgDes->bIsEncrypt = DOS_FALSE;
                pstMsgDes->bIsCompress = DOS_FALSE;
                dos_memcpy(pstMsgDes->aulServIp, g_stServMsg.achLocalIP, IPV6_SIZE);
                pstMsgDes->usServPort = g_stServMsg.usLocalPort;
                pt_logr_debug("ptc_send_msg2pts : lost/resend/confirm/exitNotify request, seq : %d", pstNeedSendNode->lSeqResend);

                ptc_send2pts((S8 *)pstMsgDes, sizeof(PT_MSG_TAG));

                dos_dmem_free(pstNeedSendNode);
                pstNeedSendNode = NULL;
                continue;
            }
            /* �������ݰ� */
            pstStreamHead = g_pstPtcSend->pstStreamHead;
            if (NULL == pstStreamHead)
            {
                dos_dmem_free(pstNeedSendNode);
                pstNeedSendNode = NULL;
                //pthread_mutex_unlock(&g_mutexSendCache);

                continue;
            }

            pstStreamNode = pt_stream_queue_search(pstStreamHead, pstNeedSendNode->ulStreamID);
            if(NULL == pstStreamNode)
            {
                pt_logr_info("ptc_send_msg2pts : stream node cann't found : %d", pstNeedSendNode->ulStreamID);
                dos_dmem_free(pstNeedSendNode);
                pstNeedSendNode = NULL;

                continue;
            }

            pstSendDataHead = pstStreamNode->unDataQueHead.pstDataTcp;
            if (NULL == pstSendDataHead)
            {
                pt_logr_info("ptc_send_msg2pts : data queue is NULL");
                dos_dmem_free(pstNeedSendNode);
                pstNeedSendNode = NULL;

                continue;
            }

            while(1)
            {
                /* ����data��ֱ�������� */
                pstStreamNode->lCurrSeq++;
                ulArraySub = (pstStreamNode->lCurrSeq) & (PT_DATA_SEND_CACHE_SIZE - 1);
                if (NULL == pstStreamNode->unDataQueHead.pstDataTcp)
                {
                    break;
                }
                stRecvDataTcp = pstStreamNode->unDataQueHead.pstDataTcp[ulArraySub];
                if (stRecvDataTcp.lSeq == pstStreamNode->lCurrSeq)
                {
                    stSendDataNode = pstSendDataHead[ulArraySub];
                    pstMsgDes->enDataType = pstNeedSendNode->enDataType;
                    dos_memcpy(pstMsgDes->aucID, g_pstPtcSend->aucID, PTC_ID_LEN);
                    pstMsgDes->ulStreamID = dos_htonl(pstNeedSendNode->ulStreamID);
                    pstMsgDes->ExitNotifyFlag = stSendDataNode.ExitNotifyFlag;
                    pstMsgDes->lSeq = dos_htonl(pstStreamNode->lCurrSeq);
                    pstMsgDes->enCmdValue = pstNeedSendNode->enCmdValue;
                    pstMsgDes->bIsEncrypt = DOS_FALSE;
                    pstMsgDes->bIsCompress = DOS_FALSE;
                    dos_memcpy(pstMsgDes->aulServIp, g_stServMsg.achLocalIP, IPV6_SIZE);
                    pstMsgDes->usServPort = g_stServMsg.usLocalPort;

                    dos_memcpy(acBuff+sizeof(PT_MSG_TAG), stSendDataNode.szBuff, stSendDataNode.ulLen);

                    ptc_send2pts(acBuff, stSendDataNode.ulLen + sizeof(PT_MSG_TAG));
                    g_ulSendDataCount++;
                    usleep(g_ulSend2PtsDelay + g_ulSendNormal2PtsDelay);

                    pt_logr_debug("send data to pts : length:%d, stream:%d, seq:%d", stRecvDataTcp.ulLen, pstNeedSendNode->ulStreamID, pstStreamNode->lCurrSeq);
                }
                else
                {
                    /* ���seqû�з��ͣ���һ */
                    pstStreamNode->lCurrSeq--;
                    break;
                }
            }

            dos_dmem_free(pstNeedSendNode);
            pstNeedSendNode = NULL;

        }
    }
    return NULL;
}

void *ptc_resend_msg2pts(void *arg)
{
    list_t           *pstNendSendList = NULL;
    S8 acBuff[PT_SEND_DATA_SIZE]      = {0};
    PT_MSG_TAG       *pstMsgDes       = (PT_MSG_TAG *)acBuff;
    PT_NEND_SEND_NODE_ST *pstNeedSendNode = NULL;
    struct timespec  timeout;
    U32              ulArraySub       = 0;
    U32              ulSendCount      = 0;
    PT_STREAM_CB_ST  *pstStreamHead   = NULL;
    PT_STREAM_CB_ST  *pstStreamNode   = NULL;
    PT_DATA_TCP_ST   *pstSendDataHead = NULL;
    PT_DATA_TCP_ST   stSendDataNode;

    while (1)
    {
        pthread_mutex_lock(&g_mutexPtcReSendPthread);
        timeout.tv_sec = time(0) + 1;
        timeout.tv_nsec = 0;
        pthread_cond_timedwait(&g_condPtcReSend, &g_mutexPtcReSendPthread, &timeout);
        pthread_mutex_unlock(&g_mutexPtcReSendPthread);

        while (1)
        {
            pthread_mutex_lock(&g_mutexPtcReSendPthread);
            if (dos_list_is_empty(&g_stPtcReSendNode))
            {
                pthread_mutex_unlock(&g_mutexPtcReSendPthread);
                break;
            }

            pstNendSendList = dos_list_fetch(&g_stPtcReSendNode);
            if (DOS_ADDR_INVALID(pstNendSendList))
            {
                DOS_ASSERT(0);
                pthread_mutex_unlock(&g_mutexPtcReSendPthread);

                break;
            }

            pstNeedSendNode = dos_list_entry(pstNendSendList, PT_NEND_SEND_NODE_ST, stListNode);
            pthread_mutex_unlock(&g_mutexPtcReSendPthread);

            /* �������ݰ� */
            pstStreamHead = g_pstPtcSend->pstStreamHead;
            if (DOS_ADDR_INVALID(pstStreamHead))
            {
                dos_dmem_free(pstNeedSendNode);
                pstNeedSendNode = NULL;

                continue;
            }

            pstStreamNode = pt_stream_queue_search(pstStreamHead, pstNeedSendNode->ulStreamID);
            if(DOS_ADDR_INVALID(pstStreamNode))
            {
                dos_dmem_free(pstNeedSendNode);
                pstNeedSendNode = NULL;

                continue;
            }

            pstSendDataHead = pstStreamNode->unDataQueHead.pstDataTcp;
            if (DOS_ADDR_INVALID(pstSendDataHead))
            {
                dos_dmem_free(pstNeedSendNode);
                pstNeedSendNode = NULL;

                continue;
            }

            /* Ҫ���ش��İ� */
            ulArraySub = pstNeedSendNode->lSeqResend & (PT_DATA_SEND_CACHE_SIZE - 1);  /* Ҫ���͵İ���data�����е��±� */
            if (pstSendDataHead[ulArraySub].lSeq != pstNeedSendNode->lSeqResend)
            {
                /* Ҫ���͵İ������� */
            }
            else
            {
                stSendDataNode = pstSendDataHead[ulArraySub];
                pstMsgDes->enDataType = pstNeedSendNode->enDataType;
                dos_memcpy(pstMsgDes->aucID, g_pstPtcSend->aucID, PTC_ID_LEN);
                pstMsgDes->ulStreamID = dos_htonl(pstNeedSendNode->ulStreamID);
                pstMsgDes->ExitNotifyFlag = stSendDataNode.ExitNotifyFlag;
                pstMsgDes->lSeq = dos_htonl(pstNeedSendNode->lSeqResend);
                pstMsgDes->enCmdValue = pstNeedSendNode->enCmdValue;
                pstMsgDes->bIsEncrypt = DOS_FALSE;
                pstMsgDes->bIsCompress = DOS_FALSE;
                dos_memcpy(pstMsgDes->aulServIp, g_stServMsg.achLocalIP, IPV6_SIZE);
                pstMsgDes->usServPort = g_stServMsg.usLocalPort;

                dos_memcpy(acBuff+sizeof(PT_MSG_TAG), stSendDataNode.szBuff, stSendDataNode.ulLen);

                /* �ش��ģ��������� */
                ulSendCount = PT_RESEND_RSP_COUNT;
                while (ulSendCount)
                {
                    g_ulSendDataCount++;
                    ptc_send2pts(acBuff, stSendDataNode.ulLen + sizeof(PT_MSG_TAG));
                    ulSendCount--;
                    usleep(g_ulSend2PtsDelay);
                }
            }
            dos_dmem_free(pstNeedSendNode);
            pstNeedSendNode = NULL;
        }
    }

    return NULL;
}

/**
 * ������void *ptc_recv_msg_from_amc(VOID *arg)
 * ���ܣ�
 *      1.���������߳�
 * ����
 *      VOID *arg :ͨ��ͨ�ŵ�sockfd
 * ����ֵ��void *
 */
VOID *ptc_recv_msg_from_pts(VOID *arg)
{
    S32         lRecvLen   = 0;
    S32         lResult    = 0;
    S32         lSaveIntoCacheRes = 0;
    U32         MaxFdp     = 0;
    PT_MSG_TAG *pstMsgDes  = NULL;
    struct sockaddr_in stClientAddr = g_pstPtcSend->stDestAddr;
    socklen_t          ulCliaddrLen = sizeof(stClientAddr);
    fd_set             ReadFds;
    struct timeval stTimeVal = {1, 0};
    struct timeval stTimeValCpy;

    S8 acRecvBuf[PT_SEND_DATA_SIZE]  = {0};
    struct timeval now;
    sem_init(&g_SemPtcRecv, 0, 1);              /* ��ʼ���ź��� */

    while (1)
    {
        if (g_ulUdpSocket < 0)
        {
            sleep(1);
            continue;
        }
        stTimeValCpy = stTimeVal;
        MaxFdp = g_ulUdpSocket;
        FD_ZERO(&ReadFds);
        FD_SET(g_ulUdpSocket, &ReadFds);

        lResult = select(MaxFdp + 1, &ReadFds, NULL, NULL, &stTimeValCpy);
        if (lResult < 0)
        {
            perror("ptc_recv_msg_from_pts : fail to select");
            DOS_ASSERT(0);
            sleep(1);
            continue;
        }
        else if (0 == lResult)
        {
            continue;
        }

        if (FD_ISSET(g_ulUdpSocket, &ReadFds))
        {
            lRecvLen = recvfrom(g_ulUdpSocket, acRecvBuf, PT_SEND_DATA_SIZE, 0, (struct sockaddr*)&stClientAddr, &ulCliaddrLen);
            if (lRecvLen < 0)
            {
                pt_logr_info("recvfrom fail ,create socket again");
                close(g_ulUdpSocket);
                g_ulUdpSocket = -1;
                g_ulUdpSocket = ptc_create_udp_socket(PTC_SOCKET_CACHE);

                continue;
            }

            sem_wait(&g_SemPtcRecv);

            /* ȡ����Ϣͷ������ */
            pstMsgDes = (PT_MSG_TAG *)acRecvBuf;
            /* �ֽ���ת�� */
            pstMsgDes->ulStreamID = dos_ntohl(pstMsgDes->ulStreamID);
            pstMsgDes->lSeq = dos_ntohl(pstMsgDes->lSeq);

            if (pstMsgDes->enDataType == PT_DATA_CTRL && pstMsgDes->ulStreamID != PT_CTRL_PTC_PACKAGE)
            {
                /* ������Ϣ */
                ptc_ctrl_msg_handle(acRecvBuf, lRecvLen);

                sem_post(&g_SemPtcRecv);
                continue;
            }

            if (pstMsgDes->enCmdValue == PT_CMD_RESEND)
            {
                /* �ش����� */
                pt_logr_debug("ptc recv resend req, seq : %d, stream : %d", pstMsgDes->lSeq, pstMsgDes->ulStreamID);
                BOOL bIsResend = DOS_TRUE;
                PT_CMD_EN enCmdValue = PT_CMD_NORMAL;
                g_ulResendCount++;
                pthread_mutex_lock(&g_mutexPtcReSendPthread);
                pt_resend_node_list_insert(&g_stPtcReSendNode, g_pstPtcSend->aucID, pstMsgDes, enCmdValue, bIsResend, DOS_FALSE);
                pthread_cond_signal(&g_condPtcReSend);
                pthread_mutex_unlock(&g_mutexPtcReSendPthread);

                sem_post(&g_SemPtcRecv);
                continue;
            }
            else if (pstMsgDes->enCmdValue == PT_CMD_CONFIRM)
            {
                /* ȷ�Ͻ��� */
                gettimeofday(&now, NULL);
                pt_logr_debug("make sure recv seq : %d", pstMsgDes->lSeq);
                lResult = ptc_deal_with_confirm_msg(pstMsgDes);
                if (lResult)
                {
                    if (PT_DATA_WEB == pstMsgDes->enDataType)
                    {
                        ptc_set_cache_full_false(pstMsgDes->ulStreamID, PT_DATA_WEB);
                        send(g_lPipeWRFd, "s", 1, 0);
                    }
                }

                sem_post(&g_SemPtcRecv);
                continue;
            }
            else if (pstMsgDes->ExitNotifyFlag == DOS_TRUE)
            {
                /* ��Ӧ���������streamID�ڵ� */
                pt_logr_debug("recv exit notify, stream : %d", pstMsgDes->ulStreamID);
                ptc_delete_send_stream_node_by_streamID(pstMsgDes->ulStreamID);
                ptc_delete_client_by_streamID(pstMsgDes->ulStreamID);

                sem_post(&g_SemPtcRecv);
                continue;
            }
            else
            {
                pt_logr_debug("ptc recv from pts stream : %d, seq : %d", pstMsgDes->ulStreamID, pstMsgDes->lSeq);
                lSaveIntoCacheRes = ptc_save_into_recv_cache(acRecvBuf, lRecvLen);
                if (lSaveIntoCacheRes < 0)
                {
                    sem_post(&g_SemPtcRecv);
                    continue;
                }

                pthread_mutex_lock(&g_mutexPtcRecvPthread);
                if (NULL == pt_need_recv_node_list_search(&g_stPtcNendRecvNode, pstMsgDes->ulStreamID))
                {
                    pt_need_recv_node_list_insert(&g_stPtcNendRecvNode, pstMsgDes);
                }
                pthread_cond_signal(&g_condPtcRecv);
                pthread_mutex_unlock(&g_mutexPtcRecvPthread);
            }

            if (lSaveIntoCacheRes == PT_NEED_CUT_PTHREAD)
            {
                usleep(10); /* �����̣߳�ִ�н��պ��� */
            }
        }
    }

    return NULL;
}

/**
 * ������VOID *ptc_send_msg2proxy(VOID *arg)
 * ���ܣ�
 *      1.�߳� ������Ϣ��proxy
 * ����
 * ����ֵ��
 */
VOID *ptc_send_msg2proxy(VOID *arg)
{
    list_t *pstNendRecvList = NULL;
    PT_NEND_RECV_NODE_ST *pstNeedRecvNode = NULL;
    struct timeval now;
    struct timespec timeout;

    while (1)
    {
        gettimeofday(&now, NULL);
        timeout.tv_sec = now.tv_sec + 1;
        timeout.tv_nsec = now.tv_usec * 1000;

        pthread_mutex_lock(&g_mutexPtcRecvPthread);
        sem_post(&g_SemPtcRecv);
        pthread_cond_timedwait(&g_condPtcRecv, &g_mutexPtcRecvPthread, &timeout);
        pthread_mutex_unlock(&g_mutexPtcRecvPthread);

        /* ѭ������g_pstPtsNendRecvNode�е�stream */

        DOS_LOOP_DETECT_INIT(lLoopMaxCount, DOS_DEFAULT_MAX_LOOP);

        while(1)
        {
            /* ��ֹ��ѭ�� */
            DOS_LOOP_DETECT(lLoopMaxCount)
            pthread_mutex_lock(&g_mutexPtcRecvPthread);
            if (dos_list_is_empty(&g_stPtcNendRecvNode))
            {
                pthread_mutex_unlock(&g_mutexPtcRecvPthread);
                break;
            }

            pstNendRecvList = dos_list_fetch(&g_stPtcNendRecvNode);
            if (DOS_ADDR_INVALID(pstNendRecvList))
            {
                pthread_mutex_unlock(&g_mutexPtcRecvPthread);
                DOS_ASSERT(0);
                continue;
            }
            pthread_mutex_unlock(&g_mutexPtcRecvPthread);

            pstNeedRecvNode = dos_list_entry(pstNendRecvList, PT_NEND_RECV_NODE_ST, stListNode);

            if (PT_DATA_CTRL == pstNeedRecvNode->enDataType)
            {
                if (PT_CTRL_PTC_PACKAGE == pstNeedRecvNode->ulStreamID)
                {
                    ptc_recv_upgrade_package(pstNeedRecvNode);
                }
                dos_dmem_free(pstNeedRecvNode);
                pstNeedRecvNode = NULL;
                continue;
            }
            else if (PT_DATA_WEB == pstNeedRecvNode->enDataType)
            {
                ptc_send_msg2web(pstNeedRecvNode);
                dos_dmem_free(pstNeedRecvNode);
                pstNeedRecvNode = NULL;
                continue;
            }
            else if (PT_DATA_CMD == pstNeedRecvNode->enDataType)
            {
                ptc_send_msg2cmd(pstNeedRecvNode);
                dos_dmem_free(pstNeedRecvNode);
                pstNeedRecvNode = NULL;
                continue;
            }
            else
            {
                dos_dmem_free(pstNeedRecvNode);
                pstNeedRecvNode = NULL;
            }
        }
    }

    return NULL;
}


void ptc_client_cb_examine()
{
    S32 i = 0;
    time_t lCurTime = time(NULL);
    U32 ulStreamID = 0;
    PT_DATA_TYPE_EN enDataType;
    S32 lSocket = 0;
    S32 lIndex  = 0;

    for (i=0; i<PTC_STREAMID_MAX_COUNT; i++)
    {
        if (g_astPtcConnects[i].enState == PTC_CLIENT_USING)
        {
            if (lCurTime - g_astPtcConnects[i].lLastUseTime > PTC_CONNECT_TIME_OUT)
            {
                /* ɾ�� */
                ulStreamID = g_astPtcConnects[i].ulStreamID;
                enDataType = g_astPtcConnects[i].enDataType;
                lSocket = g_astPtcConnects[i].lSocket;

                lIndex = ptc_get_client_node_array_by_socket(lSocket, enDataType);
                if (lIndex >= 0 && lIndex < PTC_STREAMID_MAX_COUNT)
                {
                    ptc_free_stream_resoure(enDataType, &g_astPtcConnects[lIndex], ulStreamID, 3);
                }
                else
                {
                    ptc_free_stream_resoure(enDataType, NULL, ulStreamID, 3);
                }
            }
        }
    }
}

void *ptc_terminal_dispose(void *arg)
{
    U32 ulCount = 0;
    U32 ulReSendOld = 0;
    U32 ulThresholds = 1000;
    U32 ulIncreaseCount = 0;            /* һ�������ӵĴ��� */
    U32 ulUpdateCount = 0;  /*  */
    U32 ulReduceCount = 1;

    while (1)
    {
        usleep(100 * 1000);

        ulCount += 100;
        if (ulCount >= PTC_DISPOSE_INTERVAL_TIME)
        {
            ulCount = 0;
            ptc_client_cb_examine();
        }
        if (ulCount % 1000 == 0)
        {
            if (ulIncreaseCount > 5 && ulUpdateCount >= 5)
            {
                ulThresholds = g_ulSend2PtsDelay - ulIncreaseCount * 1000 + 5000;
                if (ulThresholds >= 50 * 1000)
                {
                    ulThresholds = 49 * 1000;
                }
                ulUpdateCount = 0;
            }
            ulUpdateCount++;

            if (g_ulSendDataCount >= 20)
            {
                if (g_ulResendCount > 3)
                {
                    if (g_ulSend2PtsDelay < 50 * 1000)
                    {
                        g_ulSend2PtsDelay += 1000;
                    }
                }
                else if (0 == g_ulResendCount)
                {
                    if (ulThresholds > 1000)
                    {
                        g_ulSend2PtsDelay = ulThresholds;
                    }
                    else
                    {
                        if (g_ulSend2PtsDelay > 1000)
                        {
                            if (g_ulSend2PtsDelay > 1000)
                            {
                                g_ulSend2PtsDelay -= 1000;
                            }
                        }
                    }
                }
            }

            if (g_ulResendCount * g_ulSend2PtsDelay > 1000 * 1000)
            {
                g_ulSendNormal2PtsDelay = 1000 * 1000;
            }
            else
            {
                g_ulSendNormal2PtsDelay = 0;
            }

            if (ulReduceCount == 0 && (ulIncreaseCount > 5 || g_ulResendCount > 20))
            {
                ulReduceCount++;
                if (ulThresholds < 49 * 1000)
                {
                    ulThresholds += 1000;
                }
            }

            if (g_ulSend2PtsDelay == ulThresholds && g_ulResendCount == 0)
            {
                ulReduceCount++;
            }

            if (ulReduceCount >= 5)
            {
                /* ���ٷ�ֵ */
                ulReduceCount = 0;
                if (ulThresholds > 1000)
                {
                    ulThresholds -= 1000;
                }
            }
            //printf("%d, resend : %d, total : %d, ulThresholds : %d, %u\n", g_ulSend2PtsDelay, g_ulResendCount, g_ulSendDataCount, ulThresholds, time(NULL));
            g_ulSendDataCount = 0;
            g_ulResendCount = 0;
            ulIncreaseCount = 0;
        }
        else
        {
            if (g_ulResendCount - ulReSendOld >= 6)
            {
                if (g_ulSend2PtsDelay < 50 * 1000)
                {
                    ulIncreaseCount++;
                    g_ulSend2PtsDelay += 1000;
                }
            }
        }

        ulReSendOld = g_ulResendCount;
    }

    return NULL;
}

#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */

