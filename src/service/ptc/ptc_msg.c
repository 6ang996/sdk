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
/* include provate header file */
#include <pt/dos_sqlite3.h>
#include <pt/ptc.h>
#include "ptc_msg.h"

sem_t g_SemPtc;                             /* ���ƻ����ź��� */
sem_t g_SemPtcRecv;
U32  g_ulHDTimeoutCount = 0;                /* ������ʱ���� */
BOOL g_bIsOnLine = DOS_FALSE;               /* �豸�Ƿ����� */
DOS_TMR_ST g_stHBTmrHandle    = NULL;       /* ������ʱ�� */
DOS_TMR_ST g_stACKTmrHandle   = NULL;       /* ��½������������Ӧ�Ķ�ʱ�� */
PT_CC_CB_ST *g_pstPtcSend     = NULL;       /* ���ͻ�����ptc�ڵ� */
PT_CC_CB_ST *g_pstPtcRecv     = NULL;       /* ���ջ�����ptc�ڵ� */
list_t  *g_pstPtcNendRecvNode = NULL;       /* ��Ҫ���յİ�����Ϣ���� */
list_t  *g_pstPtcNendSendNode = NULL;       /* ��Ҫ���͵İ�����Ϣ���� */
PTC_SERV_MSG_ST g_stServMsg;                /* ���ptc��Ϣ��ȫ�ֱ��� */
pthread_mutex_t g_mutex_send  = PTHREAD_MUTEX_INITIALIZER;       /* �����߳��� */
pthread_cond_t  g_cond_send   = PTHREAD_COND_INITIALIZER;        /* ������������ */
pthread_mutex_t g_mutex_recv  = PTHREAD_MUTEX_INITIALIZER;       /* �����߳��� */
pthread_cond_t  g_cond_recv   = PTHREAD_COND_INITIALIZER;        /* ������������ */
U32 g_ulSendTimeSleep = 20;
static S32 g_lUdpSocket = 0;
U32 g_ulConnectPtsCount = 0;
PT_PTC_TYPE_EN g_enPtcType = PT_PTC_TYPE_IPCC;

S8 *ptc_get_current_time()
{
    time_t ulCurrTime = 0;

    ulCurrTime = time(NULL);
    return ctime(&ulCurrTime);
}

VOID ptc_send_pthread_mutex_lock(S8 *szFileName, U32 ulLine)
{
    printf("%s %d %.*s : send\n", szFileName, ulLine, PTC_TIME_SIZE, ptc_get_current_time());
    pthread_mutex_lock(&g_mutex_send);
    printf("%s %d %.*s : send lock\n", szFileName, ulLine, PTC_TIME_SIZE, ptc_get_current_time());
}

VOID ptc_send_pthread_mutex_unlock(S8 *szFileName, U32 ulLine)
{
    printf("%s %d %.*s : send unlock\n", szFileName, ulLine, PTC_TIME_SIZE, ptc_get_current_time());
    pthread_mutex_unlock(&g_mutex_send);
}

VOID ptc_send_pthread_cond_timedwait(struct timespec *timeout, S8 *szFileName, U32 ulLine)
{
    printf("%s %d %.*s : send wait\n", szFileName, ulLine, PTC_TIME_SIZE, ptc_get_current_time());
    pthread_cond_timedwait(&g_cond_send, &g_mutex_send, timeout); /* ����ʹ������ĺ��� */
    printf("%s %d %.*s : send lock\n", szFileName, ulLine, PTC_TIME_SIZE, ptc_get_current_time());
}

VOID ptc_recv_pthread_mutex_lock(S8 *szFileName, U32 ulLine)
{
    printf("%s %d %.*s : recv\n", szFileName, ulLine, PTC_TIME_SIZE, ptc_get_current_time());
    pthread_mutex_lock(&g_mutex_recv);
    printf("%s %d %.*s : recv lock\n", szFileName, ulLine, PTC_TIME_SIZE, ptc_get_current_time());
}

VOID ptc_recv_pthread_mutex_unlock(S8 *szFileName, U32 ulLine)
{
    printf("%s %d %.*s : recv unlock\n", szFileName, ulLine, PTC_TIME_SIZE, ptc_get_current_time());
    pthread_mutex_unlock(&g_mutex_recv);
}

VOID ptc_recv_pthread_cond_wait(S8 *szFileName, U32 ulLine)
{
    printf("%s %d %.*s : recv wait\n", szFileName, ulLine, PTC_TIME_SIZE, ptc_get_current_time());
    pthread_cond_wait(&g_cond_recv, &g_mutex_recv);
    printf("%s %d %.*s : recv wait in\n", szFileName, ulLine, PTC_TIME_SIZE, ptc_get_current_time());
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
VOID ptc_delete_send_stream_node(U32 ulStreamID, PT_DATA_TYPE_EN enDataType, BOOL bIsMutex)
{
    list_t *pstStreamListHead = NULL;
    PT_STREAM_CB_ST *pstStreamNode = NULL;

    if (bIsMutex)
    {
        #if PT_MUTEX_DEBUG
        ptc_send_pthread_mutex_lock(__FILE__, __LINE__);
        #else
        pthread_mutex_lock(&g_mutex_send);
        #endif
    }

    if (g_pstPtcSend != NULL)
    {
        pstStreamListHead = g_pstPtcSend->astDataTypes[enDataType].pstStreamQueHead;
        if (pstStreamListHead != NULL)
        {
            pstStreamNode = pt_stream_queue_search(pstStreamListHead, ulStreamID);
            if (pstStreamNode != NULL)
            {
                pstStreamListHead = pt_delete_stream_node(pstStreamListHead, &pstStreamNode->stStreamListNode, enDataType);
                g_pstPtcSend->astDataTypes[enDataType].pstStreamQueHead = pstStreamListHead;
             }
        }
    }

    if (bIsMutex)
    {
        #if PT_MUTEX_DEBUG
        ptc_send_pthread_mutex_unlock(__FILE__, __LINE__);
        #else
        pthread_mutex_unlock(&g_mutex_send);
        #endif
    }
}

/**
 * ������VOID ptc_delete_recv_stream_node(U32 ulStreamID, PT_DATA_TYPE_EN enDataType)
 * ���ܣ�
 *      1.ɾ�����ջ����У� һ��stream�ڵ�
 * ����
 *      U32 ulStreamID ��stream id
 *      U32 enDataType ����Ϣ����
 * ����ֵ����
 */
VOID ptc_delete_recv_stream_node(U32 ulStreamID, PT_DATA_TYPE_EN enDataType, BOOL bIsMutex)
{
    list_t *pstStreamListHead = NULL;
    PT_STREAM_CB_ST *pstStreamNode = NULL;

    if (bIsMutex)
    {
        #if PT_MUTEX_DEBUG
        ptc_recv_pthread_mutex_lock(__FILE__, __LINE__);
        #else
        pthread_mutex_lock(&g_mutex_recv);
        #endif
    }

    if (g_pstPtcRecv != NULL)
    {
        pstStreamListHead = g_pstPtcRecv->astDataTypes[enDataType].pstStreamQueHead;
        if (pstStreamListHead != NULL)
        {
            pstStreamNode = pt_stream_queue_search(pstStreamListHead, ulStreamID);
            if (pstStreamNode != NULL)
            {
                pstStreamListHead = pt_delete_stream_node(pstStreamListHead, &pstStreamNode->stStreamListNode, enDataType);
                g_pstPtcRecv->astDataTypes[enDataType].pstStreamQueHead = pstStreamListHead;
            }
        }
    }

    if (bIsMutex)
    {
        #if PT_MUTEX_DEBUG
        ptc_recv_pthread_mutex_unlock(__FILE__, __LINE__);
        #else
        pthread_mutex_unlock(&g_mutex_recv);
        #endif
    }
}

VOID ptc_get_udp_use_ip()
{
    S32 lSockfd = 0;
    S32 lError  = 0;
    struct sockaddr_in stServAddr;
    struct sockaddr_in stAddrCli;
    socklen_t lAddrLen = sizeof(struct sockaddr_in);
    S8 szLocalIp[PT_IP_ADDR_SIZE] = {0}; /* ���ʮ���� */
    S8 szIfrName[PT_DATA_BUFF_64] = {0};
    S8 szMac[PT_DATA_BUFF_64] = {0};

    stServAddr = g_pstPtcSend->stDestAddr;

    lSockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lSockfd < 0)
    {
        perror("create tcp socket");
        exit(DOS_FAIL);
    }

ptc_connect: lError = connect(lSockfd,(struct sockaddr*)&stServAddr,sizeof(stServAddr));
     if (lError < 0)
     {
        perror("connect pts");
        sleep(2);
        /* ���� */
        goto ptc_connect;
        exit(DOS_FAIL);
     }

    lError = getsockname(lSockfd,(struct sockaddr*)&stAddrCli, &lAddrLen);
    if (lError != 0)
    {
        pt_logr_info("Get sockname Error!");
        exit(DOS_FAIL);
    }
    g_stServMsg.usLocalPort = dos_ntohs(stAddrCli.sin_port);
    dos_strcpy(szLocalIp, inet_ntoa(stAddrCli.sin_addr));
    inet_pton(AF_INET, szLocalIp, (VOID *)(g_stServMsg.achLocalIP));
    lError = ptc_get_ifr_name_by_ip(szLocalIp, lSockfd, szIfrName, PT_DATA_BUFF_64);
    if (lError != DOS_SUCC)
    {
        pt_logr_info("Get ifr name Error!");
        exit(DOS_FAIL);
    }
    lError = ptc_get_mac(szIfrName, lSockfd, szMac, PT_DATA_BUFF_64);
    if (lError != DOS_SUCC)
    {
        pt_logr_info("Get mac Error!");
        exit(DOS_FAIL);
    }
    dos_strncpy(g_stServMsg.szMac, szMac, PT_DATA_BUFF_64);
    logr_debug("local ip : %s, irf name : %s, mac : %s", szLocalIp, szIfrName, szMac);

    close(lSockfd);
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

    #if PT_MUTEX_DEBUG
    ptc_send_pthread_mutex_lock(__FILE__, __LINE__);
    #else
    pthread_mutex_lock(&g_mutex_send);
    #endif
    BOOL bIsResend = DOS_FALSE;
    PT_CMD_EN enCmdValue = PT_CMD_RESEND;
    g_pstPtcNendSendNode = pt_need_send_node_list_insert(g_pstPtcNendSendNode, g_pstPtcSend->aucID, pstMsgDes, enCmdValue, bIsResend);

    pthread_cond_signal(&g_cond_send);
    #if PT_MUTEX_DEBUG
    ptc_send_pthread_mutex_unlock(__FILE__, __LINE__);
    #else
    pthread_mutex_unlock(&g_mutex_send);
    #endif

    return;
}

VOID ptc_send_exit_notify_to_pts(PT_DATA_TYPE_EN enDataType, U32 ulStreamID)
{
    PT_MSG_TAG stMsgDes;
    S8 acBuff[PT_DATA_BUFF_512] = {0};

    stMsgDes.enDataType = enDataType;
    dos_memcpy(stMsgDes.aucID, g_pstPtcSend->aucID, PTC_ID_LEN);
    stMsgDes.ulStreamID = dos_htonl(ulStreamID);
    stMsgDes.ExitNotifyFlag = DOS_TRUE;
    stMsgDes.lSeq = 0;
    stMsgDes.enCmdValue = PT_CMD_NORMAL;
    stMsgDes.bIsEncrypt = DOS_FALSE;
    stMsgDes.bIsCompress = DOS_FALSE;

    dos_memcpy(acBuff, (VOID *)&stMsgDes, sizeof(PT_MSG_TAG));

    sendto(g_lUdpSocket, acBuff, sizeof(PT_MSG_TAG), 0, (struct sockaddr*)&g_pstPtcSend->stDestAddr, sizeof(g_pstPtcSend->stDestAddr));
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
    list_t *pstStreamListHead = NULL;

    if (pstStreamNode->ulCountResend >= PT_RESEND_REQ_MAX_COUNT)
    {
        /* �����ط�PTC_RESEND_MAX_COUNT�Σ���δ�յ� */
        //ptc_save_msg_into_cache(pstMsg->enDataType, pstMsg->ulStreamID, "", 0, DOS_TRUE); /* ֪ͨpts�ر�sockfd */
        ptc_send_exit_notify_to_pts(pstMsg->enDataType, pstMsg->ulStreamID);
        /* ���ptc�У�stream����Դ */
        if (NULL != g_pstPtcRecv)
        {
            pstStreamListHead = g_pstPtcRecv->astDataTypes[pstMsg->enDataType].pstStreamQueHead;
            pt_delete_stream_node(pstStreamListHead, &pstStreamNode->stStreamListNode, pstMsg->enDataType);
            g_pstPtcRecv->astDataTypes[pstMsg->enDataType].pstStreamQueHead = pstStreamListHead;
        }
        ptc_delete_send_stream_node(pstMsg->ulStreamID, pstMsg->enDataType, DOS_FALSE);
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

    if (dos_strncmp(ptc_get_version(), "1.1", dos_strlen("1.1")) == 0)
    {
        /* 1.1�汾��֤���� */
        for (i=0; i<PT_LOGIN_VERIFY_SIZE-1; i++)
        {
            szDest[i] = szKey[i]&0xA9;
        }
        szDest[PT_LOGIN_VERIFY_SIZE] = '\0';
    }
    else
    {
        return DOS_FAIL;
    }

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
VOID ptc_hd_ack_timeout_callback(U64 lSockfd)
{
    g_ulHDTimeoutCount++;
    pt_logr_debug("Heartbeat response timeout : %d", g_ulHDTimeoutCount);
    if (g_ulHDTimeoutCount >= PTC_HB_TIMEOUT_COUNT_MAX)
    {
        /* ��������޷��յ�������Ӧ�����·��͵�½���� */
        ptc_send_login2pts(lSockfd);
        g_bIsOnLine = DOS_FALSE;
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
VOID ptc_send_hb_req(U64 lSockfd)
{
    PT_MSG_TAG stMsgDes;
    PT_CTRL_DATA_ST stVerRet;
    S8 acBuff[PT_DATA_BUFF_512] = {0};
    S32 lResult = 0;
    /* ������Ϣ������ֵ */
    stMsgDes.enDataType = PT_DATA_CTRL;
    dos_memcpy(stMsgDes.aucID, g_pstPtcSend->aucID, PTC_ID_LEN);
    stMsgDes.ulStreamID = dos_htonl(PT_CTRL_HB_REQ);
    stMsgDes.ExitNotifyFlag = DOS_FALSE;
    stMsgDes.lSeq = 0;
    stMsgDes.enCmdValue = PT_CMD_NORMAL;
    stMsgDes.bIsEncrypt = DOS_FALSE;
    stMsgDes.bIsCompress = DOS_FALSE;
    dos_memcpy(stMsgDes.aulServIp, g_stServMsg.achLocalIP, IPV6_SIZE);
    /* �������ݸ�ֵ */
    stMsgDes.usServPort = g_stServMsg.usLocalPort;
    dos_memcpy(acBuff, (VOID *)&stMsgDes, sizeof(PT_MSG_TAG));
    stVerRet.enCtrlType = PT_CTRL_HB_REQ;
    dos_memcpy(acBuff+sizeof(PT_MSG_TAG), (VOID *)&stVerRet, sizeof(PT_CTRL_DATA_ST));

    sendto(lSockfd, acBuff, sizeof(PT_CTRL_DATA_ST) + sizeof(PT_MSG_TAG), 0, (struct sockaddr*)&g_pstPtcSend->stDestAddr, sizeof(g_pstPtcSend->stDestAddr));
    /* ������ʱ�����ж�������Ӧ�Ƿ�ʱ */
    lResult = dos_tmr_start(&g_stACKTmrHandle, PTC_WAIT_HB_ACK_TIME, ptc_hd_ack_timeout_callback, lSockfd, TIMER_NORMAL_NO_LOOP);
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
    S32 lSockfd = (S32)arg;
    PT_MSG_TAG stMsgDes;
    PT_CTRL_DATA_ST stVerRet;
    S8 acBuff[PT_DATA_BUFF_512] = {0};
    struct sockaddr_in stAddrCli;
    socklen_t lAddrLen = sizeof(struct sockaddr_in);
    S8 szDestIp[PT_IP_ADDR_SIZE] = {0};

    if (g_bIsOnLine)
    {
        /* �ѵ�½,�رն�ʱ������������ */
        dos_tmr_stop(&g_stHBTmrHandle);
        ptc_send_heartbeat2pts(lSockfd);

        return;
    }
    g_ulConnectPtsCount++;
    printf("connect pts count : %d\n", g_ulConnectPtsCount);
    if (g_ulConnectPtsCount > 4)
    {
        g_ulConnectPtsCount = 0;
        if (g_pstPtcSend->stDestAddr.sin_addr.s_addr == *(U32 *)(g_stServMsg.achPtsMajorIP))
        {
            /* ������ע�᲻�ϣ��л�pts�������� */
            g_pstPtcSend->stDestAddr.sin_addr.s_addr = *(U32 *)(g_stServMsg.achPtsMinorIP);
            g_pstPtcSend->stDestAddr.sin_port = g_stServMsg.usPtsMinorPort;
        }
        else if (g_pstPtcSend->stDestAddr.sin_addr.s_addr == *(U32 *)(g_stServMsg.achPtsMinorIP))
        {
            /* ������ע�᲻�ϣ��л��������� */
            g_pstPtcSend->stDestAddr.sin_addr.s_addr = *(U32 *)(g_stServMsg.achPtsMajorIP);
            g_pstPtcSend->stDestAddr.sin_port = g_stServMsg.usPtsMajorPort;
        }
        else
        {
            /* ��������ע�᲻�ϣ��л��������� */
            g_pstPtcSend->stDestAddr.sin_addr.s_addr = *(U32 *)(g_stServMsg.achPtsMajorIP);
            g_pstPtcSend->stDestAddr.sin_port = g_stServMsg.usPtsMajorPort;
        }
    }
    /* ��½��Ϣ������ֵ */
    stMsgDes.enDataType = PT_DATA_CTRL;
    dos_memcpy(stMsgDes.aucID, g_pstPtcSend->aucID, PTC_ID_LEN);
    stMsgDes.ulStreamID = dos_htonl(PT_CTRL_LOGIN_REQ);
    stMsgDes.ExitNotifyFlag = DOS_FALSE;
    stMsgDes.lSeq = 0;
    stMsgDes.enCmdValue = PT_CMD_NORMAL;
    stMsgDes.bIsEncrypt = DOS_FALSE;
    stMsgDes.bIsCompress = DOS_FALSE;
    //dos_memcpy(stMsgDes.aulServIp, g_stServMsg.achLocalIP, IPV6_SIZE);
    //stMsgDes.usServPort = g_stServMsg.usLocalPort;
    dos_memcpy(acBuff, (VOID *)&stMsgDes, sizeof(PT_MSG_TAG));
    /* ��½���ݸ�ֵ */
    stVerRet.enCtrlType = PT_CTRL_LOGIN_REQ;
    dos_strncpy(stVerRet.szVersion, ptc_get_version(), PT_VERSION_LEN);

    dos_memcpy(acBuff+sizeof(PT_MSG_TAG), (VOID *)&stVerRet, sizeof(PT_CTRL_DATA_ST));

    inet_ntop(AF_INET, (void *)(&g_pstPtcSend->stDestAddr.sin_addr.s_addr), szDestIp, PT_IP_ADDR_SIZE);
    sendto(lSockfd, acBuff, sizeof(PT_CTRL_DATA_ST) + sizeof(PT_MSG_TAG), 0, (struct sockaddr*)&g_pstPtcSend->stDestAddr, sizeof(g_pstPtcSend->stDestAddr));
    logr_debug("ptc send login request to pts : ip : %s", szDestIp);

    /* ��ȡptcһ��udp��˽���˿ں� -- sendto�󣬲��ܻ�ȡ�˿ں� */
    if (g_stServMsg.usLocalPort == 0)
    {
        lResult = getsockname(lSockfd,(struct sockaddr*)&stAddrCli, &lAddrLen);
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
VOID ptc_send_logout_req(U64 arg)
{
    S32 lSockfd = (S32)arg;
    PT_MSG_TAG stMsgDes;
    PT_CTRL_DATA_ST stVerRet;
    S8 acBuff[PT_DATA_BUFF_512] = {0};

    stMsgDes.enDataType = PT_DATA_CTRL;
    dos_memcpy(stMsgDes.aucID, g_pstPtcSend->aucID, PTC_ID_LEN);
    stMsgDes.ulStreamID = dos_htonl(PT_CTRL_LOGOUT);
    stMsgDes.ExitNotifyFlag = DOS_FALSE;
    stMsgDes.lSeq = 0;
    stMsgDes.enCmdValue = PT_CMD_NORMAL;
    stMsgDes.bIsEncrypt = DOS_FALSE;
    stMsgDes.bIsCompress = DOS_FALSE;
    dos_memcpy(stMsgDes.aulServIp, g_stServMsg.achLocalIP, IPV6_SIZE);
    stMsgDes.usServPort = g_stServMsg.usLocalPort;

    dos_memcpy(acBuff, (VOID *)&stMsgDes, sizeof(PT_MSG_TAG));

    stVerRet.enCtrlType = PT_CTRL_LOGOUT;
    dos_memcpy(acBuff+sizeof(PT_MSG_TAG), (VOID *)&stVerRet, sizeof(PT_CTRL_DATA_ST));

    sendto(lSockfd, acBuff, sizeof(PT_CTRL_DATA_ST) + sizeof(PT_MSG_TAG), 0, (struct sockaddr*)&g_pstPtcSend->stDestAddr, sizeof(g_pstPtcSend->stDestAddr));
    pt_logr_debug("ptc send logout request to pts");
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

    #if PT_MUTEX_DEBUG
    ptc_send_pthread_mutex_lock(__FILE__, __LINE__);
    #else
    pthread_mutex_lock(&g_mutex_send);
    #endif
    BOOL bIsResend = DOS_FALSE;
    PT_CMD_EN enCmdValue = PT_CMD_CONFIRM;
    pstMsgDes->lSeq = lConfirmSeq;

    pt_logr_debug("send confirm data to pts: type = %d, stream = %d", pstMsgDes->enDataType,pstMsgDes->ulStreamID);
    for (i=0; i<PT_SEND_CONFIRM_COUNT; i++)
    {
        g_pstPtcNendSendNode = pt_need_send_node_list_insert(g_pstPtcNendSendNode, pstMsgDes->aucID, pstMsgDes, enCmdValue, bIsResend);
    }
    pthread_cond_signal(&g_cond_send);
    #if PT_MUTEX_DEBUG
    ptc_send_pthread_mutex_unlock(__FILE__, __LINE__);
    #else
    pthread_mutex_unlock(&g_mutex_send);
    #endif
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
    if (NULL == g_pstPtcSend || NULL == pstMsgDes || NULL == acSendBuf)
    {
        pt_logr_debug("ptc_save_into_send_cache : param error");
        return PT_SAVE_DATA_FAIL;
    }

    list_t          *pstStreamListHead  = NULL;
    PT_STREAM_CB_ST *pstStreamNode      = NULL;
    PT_DATA_TCP_ST  *pstDataQueue       = NULL;
    S32 lResult = 0;

    /* �ж�stream�Ƿ��ڽ��ն����д��ڣ��������ڣ�˵�����stream�Ѿ����� */
    if (pstMsgDes->enDataType != PT_DATA_CTRL)
    {
        #if PT_MUTEX_DEBUG
        ptc_recv_pthread_mutex_lock(__FILE__, __LINE__);
        #else
        pthread_mutex_lock(&g_mutex_recv);
        #endif
        pstStreamListHead = g_pstPtcRecv->astDataTypes[pstMsgDes->enDataType].pstStreamQueHead;
        if (NULL == pstStreamListHead)
        {
            #if PT_MUTEX_DEBUG
            ptc_recv_pthread_mutex_unlock(__FILE__, __LINE__);
            #else
            pthread_mutex_unlock(&g_mutex_recv);
            #endif
            return PT_SAVE_DATA_FAIL;
        }
        else
        {
            pstStreamNode = pt_stream_queue_search(pstStreamListHead, pstMsgDes->ulStreamID);
            if (NULL == pstStreamNode)
            {
                #if PT_MUTEX_DEBUG
                ptc_recv_pthread_mutex_unlock(__FILE__, __LINE__);
                #else
                pthread_mutex_unlock(&g_mutex_recv);
                #endif
                return PT_SAVE_DATA_FAIL;
            }
        }
        #if PT_MUTEX_DEBUG
        ptc_recv_pthread_mutex_unlock(__FILE__, __LINE__);
        #else
        pthread_mutex_unlock(&g_mutex_recv);
        #endif
    }

    #if PT_MUTEX_DEBUG
    ptc_send_pthread_mutex_lock(__FILE__, __LINE__);
    #else
    pthread_mutex_lock(&g_mutex_send);
    #endif
    pstStreamListHead = g_pstPtcSend->astDataTypes[pstMsgDes->enDataType].pstStreamQueHead;
    if (NULL == pstStreamListHead)
    {
        /* ����stream list head */
        pstStreamNode = pt_stream_node_create(pstMsgDes->ulStreamID);
        if (NULL == pstStreamNode)
        {
            /* stream node����ʧ�� */
            #if PT_MUTEX_DEBUG
            ptc_send_pthread_mutex_unlock(__FILE__, __LINE__);
            #else
            pthread_mutex_unlock(&g_mutex_send);
            #endif
            return PT_SAVE_DATA_FAIL;
        }

        pstStreamListHead = &(pstStreamNode->stStreamListNode);
        g_pstPtcSend->astDataTypes[pstMsgDes->enDataType].pstStreamQueHead = pstStreamListHead;
    }
    else
    {
        pstStreamNode = pt_stream_queue_search(pstStreamListHead, pstMsgDes->ulStreamID);
        if (NULL == pstStreamNode)
        {
            /* ����stream node */
            pstStreamNode = pt_stream_node_create(pstMsgDes->ulStreamID);
            if (NULL == pstStreamNode)
            {
                /* stream node����ʧ�� */
                #if PT_MUTEX_DEBUG
                ptc_send_pthread_mutex_unlock(__FILE__, __LINE__);
                #else
                pthread_mutex_unlock(&g_mutex_send);
                #endif
                return PT_SAVE_DATA_FAIL;
            }
            /* ���� stream list�� */
            pstStreamListHead = pt_stream_queue_insert(pstStreamListHead, &(pstStreamNode->stStreamListNode));
        }
    }

    pstDataQueue = pstStreamNode->unDataQueHead.pstDataTcp;
    if (NULL == pstDataQueue)
    {
        /* ����tcp data queue */
        pstDataQueue = pt_data_tcp_queue_create(PT_DATA_RECV_CACHE_SIZE);
        if (NULL == pstDataQueue)
        {
            /* create data queueʧ��,�ͷŵ�stream */
            pt_logr_info("ptc_save_into_send_cache : create data queue fail");
            pstStreamListHead = pt_delete_stream_node(pstStreamListHead, &pstStreamNode->stStreamListNode, pstMsgDes->enDataType);
            #if PT_MUTEX_DEBUG
            ptc_send_pthread_mutex_unlock(__FILE__, __LINE__);
            #else
            pthread_mutex_unlock(&g_mutex_send);
            #endif
            return PT_SAVE_DATA_FAIL;
        }

        pstStreamNode->unDataQueHead.pstDataTcp = pstDataQueue;
    }

    /* �����ݲ��뵽data queue�� */
    lResult = pt_send_data_tcp_queue_insert(pstStreamNode, acSendBuf, lDataLen, PT_DATA_SEND_CACHE_SIZE);
    if (lResult < 0)
    {
        #if PT_MUTEX_DEBUG
        ptc_send_pthread_mutex_unlock(__FILE__, __LINE__);
        #else
        pthread_mutex_unlock(&g_mutex_send);
        #endif
        return PT_SAVE_DATA_FAIL;
    }

    #if PT_MUTEX_DEBUG
    ptc_send_pthread_mutex_unlock(__FILE__, __LINE__);
    #else
    pthread_mutex_unlock(&g_mutex_send);
    #endif
    return lResult;
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
S32 ptc_save_into_recv_cache(PT_CC_CB_ST *pstPtcNode, S8 *acRecvBuf, S32 lDataLen, S32 lSockfd)
{
    if (NULL == pstPtcNode || NULL == acRecvBuf)
    {
        return PT_SAVE_DATA_FAIL;
    }

    S32 i = 0;
    U32 ulArraySub = 0;
    S32 lResult = 0;
    U32 ulNextSendArraySub = 0;
    list_t             *pstStreamListHead  = NULL;
    PT_STREAM_CB_ST    *pstStreamNode      = NULL;
    PT_LOSE_BAG_MSG_ST *pstLoseMsg         = NULL;
    PT_DATA_TCP_ST     *pstDataQueue       = NULL;
    PT_MSG_TAG         *pstMsgDes = (PT_MSG_TAG *)acRecvBuf;

    pstStreamListHead = pstPtcNode->astDataTypes[pstMsgDes->enDataType].pstStreamQueHead;
    if (NULL == pstStreamListHead)
    {
        /* ����stream list head */
        pstStreamNode = pt_stream_node_create(pstMsgDes->ulStreamID);
        if (NULL == pstStreamNode)
        {
            /* stream node����ʧ�� */
            pt_logr_info("ptc_save_into_recv_cache : create stream node fail");
            return PT_SAVE_DATA_FAIL;
        }
        dos_memcpy(pstStreamNode->aulServIp, pstMsgDes->aulServIp, IPV6_SIZE);
        pstStreamNode->usServPort = pstMsgDes->usServPort;

        pstStreamListHead = &(pstStreamNode->stStreamListNode);
        pstPtcNode->astDataTypes[pstMsgDes->enDataType].pstStreamQueHead = pstStreamListHead;
    }
    else
    {
        pstStreamNode = pt_stream_queue_search(pstStreamListHead, pstMsgDes->ulStreamID);
        if (NULL == pstStreamNode)
        {
            /* ����stream node */
            pstStreamNode = pt_stream_node_create(pstMsgDes->ulStreamID);
            if (NULL == pstStreamNode)
            {
                /* stream node����ʧ�� */
                pt_logr_info("ptc_save_into_recv_cache : create stream node fail");
                return PT_SAVE_DATA_FAIL;
            }
            dos_memcpy(pstStreamNode->aulServIp, pstMsgDes->aulServIp, IPV6_SIZE);
            pstStreamNode->usServPort = pstMsgDes->usServPort;
            /* ���� stream list�� */
            pstStreamListHead = pt_stream_queue_insert(pstStreamListHead, &(pstStreamNode->stStreamListNode));
        }
    }

    pstDataQueue = pstStreamNode->unDataQueHead.pstDataTcp;
    if (NULL == pstDataQueue)
    {
        /* ����tcp data queue */
        pstDataQueue = pt_data_tcp_queue_create(PT_DATA_RECV_CACHE_SIZE);
        if (NULL == pstDataQueue)
        {
            /* create data queueʧ�� */
            pt_logr_info("ptc_save_into_recv_cache : create tcp data queue fail");
            return PT_SAVE_DATA_FAIL;
        }

        pstStreamNode->unDataQueHead.pstDataTcp = pstDataQueue;
    }

    /* �����ݲ��뵽data queue�� */
    lResult = pt_recv_data_tcp_queue_insert(pstStreamNode, pstMsgDes, acRecvBuf+sizeof(PT_MSG_TAG), lDataLen-sizeof(PT_MSG_TAG), PT_DATA_RECV_CACHE_SIZE);
    if (lResult < 0)
    {
        pt_logr_debug("ptc_save_into_recv_cache : add data into recv cache fail");
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
            return PT_SAVE_DATA_FAIL;
        }
        else
        {
            if (pstStreamNode->lCurrSeq != -1)
            {
                ulNextSendArraySub = (pstStreamNode->lCurrSeq + 1) & (PT_DATA_RECV_CACHE_SIZE - 1);
                if (pstDataQueue[ulNextSendArraySub].lSeq == pstStreamNode->lCurrSeq + 1)
                {
                    return PT_SAVE_DATA_SUCC;
                }
            }
            else
            {
                if (pstMsgDes->lSeq == 0)
                {
                    return PT_SAVE_DATA_SUCC;
                }
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
                        perror("ptc_save_into_recv_cache : malloc");
                        return PT_SAVE_DATA_FAIL;
                    }

                    pstLoseMsg->stMsg = *pstMsgDes;
                    pstLoseMsg->pstStreamNode = pstStreamNode;
                    pstLoseMsg->lSocket = lSockfd;

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
            return PT_SAVE_DATA_FAIL;
        }
    }
}

VOID ptc_update_pts_history_file(U8 aulServIp[IPV6_SIZE], U16 usServPort)
{
    S8 szPtsIp[PT_IP_ADDR_SIZE] = {0};
    FILE *pFileHandle = NULL;
    S32 lPtsHistoryConut = 0;
    S8 szPtsHistory[3][PT_DATA_BUFF_64];

    inet_ntop(AF_INET, (void *)(aulServIp), szPtsIp, PT_IP_ADDR_SIZE);
    pFileHandle = fopen("pts_history", "r");
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
    pFileHandle = fopen("pts_history", "w");
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
 * ������VOID ptc_ctrl_msg_handle(U32 ulStreamID, S8 *pData)
 * ���ܣ�
 *      1.���������Ϣ
 * ����
 *
 * ����ֵ����
 */
VOID ptc_ctrl_msg_handle(PT_MSG_TAG *pstMsgDes, S8 *pData)
{
    if (NULL == pData)
    {
        return;
    }
    PT_CTRL_DATA_ST *pstCtrlData = NULL;
    S8 szDestKey[PT_LOGIN_VERIFY_SIZE] = {0};
    S32 lResult = 0;
    PT_CTRL_DATA_ST stVerRet;
    S8 acBuff[sizeof(PT_CTRL_DATA_ST)] = {0};
    S8 szPtcName[PT_PTC_NAME_LEN] = {0};
    S8 szPtsIp[PT_IP_ADDR_SIZE] = {0};
    S8 szPtsPort[PT_DATA_BUFF_16] = {0};
    U8 paucIPAddr[IPV6_SIZE] = {0};
    PT_CTRL_DATA_ST stCtrlData;

    pstCtrlData = (PT_CTRL_DATA_ST *)(pData + sizeof(PT_MSG_TAG));
    switch (pstCtrlData->enCtrlType)
    {
        case PT_CTRL_LOGIN_RSP:
            /* ��½��֤ */
            ptc_get_udp_use_ip();  /* ��ȡ����ip */
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
            dos_strncpy(stVerRet.szVersion, ptc_get_version(), PT_VERSION_LEN);
            dos_strcpy(stVerRet.szPtsHistoryIp1, g_stServMsg.szPtsLasterIP1);
            dos_strcpy(stVerRet.szPtsHistoryIp2, g_stServMsg.szPtsLasterIP2);
            dos_strcpy(stVerRet.szPtsHistoryIp3, g_stServMsg.szPtsLasterIP3);
            stVerRet.usPtsHistoryPort1 = g_stServMsg.usPtsLasterPort1;
            stVerRet.usPtsHistoryPort2 = g_stServMsg.usPtsLasterPort2;
            stVerRet.usPtsHistoryPort3 = g_stServMsg.usPtsLasterPort3;
            dos_strncpy(stVerRet.szMac, g_stServMsg.szMac, PT_DATA_BUFF_64);
            dos_memcpy(acBuff, (VOID *)&stVerRet, sizeof(PT_CTRL_DATA_ST));

            ptc_save_msg_into_cache(PT_DATA_CTRL, pstMsgDes->ulStreamID, acBuff, sizeof(PT_CTRL_DATA_ST));
            break;
        case PT_CTRL_LOGIN_ACK:
            /* ��½��� */
            if (pstCtrlData->ucLoginRes == DOS_TRUE)
            {
                /* �ɹ� */
                pt_logr_info("ptc login succ");
                g_bIsOnLine = DOS_TRUE;
                g_ulHDTimeoutCount = 0;
                g_ulConnectPtsCount = 0;
            }
            else
            {
                /* ʧ�� */
                pt_logr_info("login fail");
            }
            /* ֪ͨpts�������¼�Ļ��� */
            ptc_delete_send_stream_node(PT_CTRL_LOGIN_RSP, PT_DATA_CTRL, DOS_TRUE);
            ptc_send_exit_notify_to_pts(PT_DATA_CTRL, PT_CTRL_LOGIN_RSP);
            break;
        case PT_CTRL_HB_RSP:
            /* ������Ӧ���رն�ʱ�� */
            pt_logr_debug("recv from pts hb rsp");
            if (g_stACKTmrHandle != NULL)
            {
                dos_tmr_stop(&g_stACKTmrHandle);
            }
            g_stACKTmrHandle = NULL;
            g_ulHDTimeoutCount = 0;
            break;
        case PT_CTRL_SWITCH_PTS:
            /* �л�pts */
            ptc_send_logout2pts(g_lUdpSocket);
            g_pstPtcSend->stDestAddr.sin_addr.s_addr = *(U32 *)(pstMsgDes->aulServIp);
            g_pstPtcSend->stDestAddr.sin_port = pstMsgDes->usServPort;

            /* д���ļ�,��¼ע���pts����ʷ��¼ */
            //if (*(U32 *)pstMsgDes->aulServIp != *(U32 *)g_stServMsg.achPtsMajorIP && *(U32 *)pstMsgDes->aulServIp != *(U32 *)g_stServMsg.achPtsMinorIP)
            ptc_update_pts_history_file(pstMsgDes->aulServIp, pstMsgDes->usServPort);
            ptc_send_login2pts(g_lUdpSocket);
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
                    lResult = pt_DNS_resolution(pstCtrlData->achPtsMajorDomain, paucIPAddr);
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

            ptc_save_msg_into_cache(PT_DATA_CTRL, PT_CTRL_PTS_MAJOR_DOMAIN, (S8 *)&stCtrlData, sizeof(PT_CTRL_DATA_ST));
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
                    lResult = pt_DNS_resolution(pstCtrlData->achPtsMinorDomain, paucIPAddr);
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

            ptc_save_msg_into_cache(PT_DATA_CTRL, PT_CTRL_PTS_MINOR_DOMAIN, (S8 *)&stCtrlData, sizeof(PT_CTRL_DATA_ST));
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
    list_t             *pstStreamListHead  = NULL;
    PT_STREAM_CB_ST    *pstStreamNode      = NULL;

    #if PT_MUTEX_DEBUG
    ptc_send_pthread_mutex_lock(__FILE__, __LINE__);
    #else
    pthread_mutex_lock(&g_mutex_send);
    #endif
    pstStreamListHead = g_pstPtcSend->astDataTypes[pstMsgDes->enDataType].pstStreamQueHead;
    if (NULL == pstStreamListHead)
    {
        #if PT_MUTEX_DEBUG
        ptc_send_pthread_mutex_unlock(__FILE__, __LINE__);
        #else
        pthread_mutex_unlock(&g_mutex_send);
        #endif
        return DOS_FALSE;
    }

    pstStreamNode = pt_stream_queue_search(pstStreamListHead, pstMsgDes->ulStreamID);
    if (NULL == pstStreamNode)
    {
        #if PT_MUTEX_DEBUG
        ptc_send_pthread_mutex_unlock(__FILE__, __LINE__);
        #else
        pthread_mutex_unlock(&g_mutex_send);
        #endif
        return DOS_FALSE;
    }

    if (pstStreamNode->lConfirmSeq < pstMsgDes->lSeq)
    {
        pstStreamNode->lConfirmSeq = pstMsgDes->lSeq;
        //sem_getvalue(&g_SemPtc, &i);
        //pt_logr_info("sem post : %ld", i);
        sem_post(&g_SemPtc);
    }
    #if PT_MUTEX_DEBUG
    ptc_send_pthread_mutex_unlock(__FILE__, __LINE__);
    #else
    pthread_mutex_unlock(&g_mutex_send);
    #endif
    return DOS_TRUE;
}

/**
 * ������S8 *ptc_get_version()
 * ���ܣ�
 *      1.��ȡ�汾��
 * ����
 * ����ֵ��
 */
S8 *ptc_get_version()
{
    return PTC_VERSION;
}

/**
 * ������VOID ptc_send_heartbeat2pts(S32 lSockfd)
 * ���ܣ�
 *      1.������ʱ����������������
 * ����
 * ����ֵ��
 */
VOID ptc_send_heartbeat2pts(S32 lSockfd)
{
    S32 lResult = 0;

    lResult = dos_tmr_start(&g_stHBTmrHandle, PTC_SEND_HB_TIME, ptc_send_hb_req, (U64)lSockfd, TIMER_NORMAL_LOOP);
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
VOID ptc_send_login2pts(S32 lSockfd)
{
    S32 lResult = 0;
    ptc_send_login_req((U64)lSockfd);
    lResult = dos_tmr_start(&g_stHBTmrHandle, PTC_SEND_HB_TIME, ptc_send_login_req, (U64)lSockfd, TIMER_NORMAL_LOOP);
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
VOID ptc_send_logout2pts(S32 lSockfd)
{
    g_bIsOnLine = DOS_FALSE;
    dos_tmr_stop(&g_stHBTmrHandle);
    ptc_send_logout_req((U64)lSockfd);
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
VOID ptc_save_msg_into_cache(PT_DATA_TYPE_EN enDataType, U32 ulStreamID, S8 *pcData, S32 lDataLen)
{
    S32 lResult = 0;
    PT_MSG_TAG stMsgDes;
    struct timespec stSemTime;
    struct timeval now;
    if (enDataType < 0 || enDataType >= PT_DATA_BUTT)
    {
        pt_logr_debug("ptc_save_msg_into_cache : enDataType should in 0-%d: %d", PT_DATA_BUTT, enDataType);
        return;
    }
    else if (NULL == pcData)
    {
        pt_logr_debug("ptc_save_msg_into_cache : send data is NULL");
        return;
    }

    BOOL bIsResend = DOS_FALSE;
    PT_CMD_EN enCmdValue = PT_CMD_NORMAL;

    stMsgDes.ExitNotifyFlag = DOS_FALSE;
    stMsgDes.ulStreamID     = ulStreamID;
    stMsgDes.enDataType     = enDataType;

    lResult = ptc_save_into_send_cache(&stMsgDes, pcData, lDataLen);
    if (lResult < 0)
    {
        /* ��ӵ����ͻ���ʧ�� */
        return;
    }
    else
    {
        /* ��ӵ�������Ϣ���� */
        #if PT_MUTEX_DEBUG
        ptc_send_pthread_mutex_lock(__FILE__, __LINE__);
        #else
        pthread_mutex_lock(&g_mutex_send);
        #endif
        if (NULL == pt_need_send_node_list_search(g_pstPtcNendSendNode, ulStreamID))
        {
            g_pstPtcNendSendNode = pt_need_send_node_list_insert(g_pstPtcNendSendNode, g_pstPtcSend->aucID, &stMsgDes, enCmdValue, bIsResend);
        }
        pthread_cond_signal(&g_cond_send);
        #if PT_MUTEX_DEBUG
        ptc_send_pthread_mutex_unlock(__FILE__, __LINE__);
        #else
        pthread_mutex_unlock(&g_mutex_send);
        #endif
    }

    if (PT_NEED_CUT_PTHREAD == lResult)
    {
        gettimeofday(&now, NULL);
        stSemTime.tv_sec = now.tv_sec + PTC_WAIT_CONFIRM_TIMEOUT;
        stSemTime.tv_nsec = now.tv_usec * 1000;
        pt_logr_debug("wait for confirm msg");
        // sem_getvalue(&g_SemPtc, &i);
        sem_timedwait(&g_SemPtc, &stSemTime);
    }

    usleep(10); /* ��֤�л��߳� */
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
    S32              lSockfd          = *(S32 *)arg;
    U32              ulArraySub       = 0;
    U32              ulSendCount      = 0;
    list_t           *pstStreamHead   = NULL;
    list_t           *pstNendSendList = NULL;
    PT_STREAM_CB_ST  *pstStreamNode   = NULL;
    PT_DATA_TCP_ST   *pstSendDataHead = NULL;
    PT_DATA_TCP_ST   stSendDataNode;
    PT_MSG_TAG       stMsgDes;
    S8 acBuff[PT_SEND_DATA_SIZE]      = {0};
    PT_NEND_SEND_NODE_ST *pstNeedSendNode = NULL;
    PT_DATA_TCP_ST    stRecvDataTcp;
    struct timeval now;
    struct timespec timeout;
    S32 lResult = 0;
    sem_init (&g_SemPtc, 0, 0);  /* ��ʼ���ź��� */

    while (1)
    {
        gettimeofday(&now, NULL);
        timeout.tv_sec = now.tv_sec + 1;
        timeout.tv_nsec = (now.tv_usec) * 1000;

        #if PT_MUTEX_DEBUG
        ptc_send_pthread_mutex_lock(__FILE__, __LINE__);
        #else
        pthread_mutex_lock(&g_mutex_send);
        #endif
        #if PT_MUTEX_DEBUG
        ptc_send_pthread_cond_timedwait(&timeout, __FILE__, __LINE__); /* ����ʹ������ĺ��� */
        #else
        pthread_cond_timedwait(&g_cond_send, &g_mutex_send, &timeout);
        #endif

        if (NULL == g_pstPtcSend)
        {
            pt_logr_info("ptc send cache not init");
            continue;
        }

        pstNendSendList = g_pstPtcNendSendNode;
        DOS_LOOP_DETECT_INIT(lLoopMaxCount, DOS_DEFAULT_MAX_LOOP);

        while(1)
        {
            /* ��ֹ��ѭ�� */
            DOS_LOOP_DETECT(lLoopMaxCount)

            if (NULL == pstNendSendList)
            {
                break;
            }
            pstNeedSendNode = list_entry(pstNendSendList, PT_NEND_SEND_NODE_ST, stListNode);
            if (pstNendSendList == pstNendSendList->next)
            {
                pstNendSendList = NULL;
            }
            else
            {
                pstNendSendList = pstNendSendList->next;
                list_del(&pstNeedSendNode->stListNode);
            }

            dos_memzero(&stMsgDes, sizeof(PT_MSG_TAG));

            if (pstNeedSendNode->enCmdValue != PT_CMD_NORMAL || pstNeedSendNode->ExitNotifyFlag == DOS_TRUE)
            {
                /* ���� lost/resend/confirm/exitNotify��Ϣ */
                stMsgDes.enDataType = pstNeedSendNode->enDataType;
                dos_memcpy(stMsgDes.aucID, g_pstPtcSend->aucID, PTC_ID_LEN);
                stMsgDes.ulStreamID = dos_htonl(pstNeedSendNode->ulStreamID);
                stMsgDes.ExitNotifyFlag = pstNeedSendNode->ExitNotifyFlag;
                stMsgDes.lSeq = dos_htonl(pstNeedSendNode->lSeqResend);
                stMsgDes.enCmdValue = pstNeedSendNode->enCmdValue;
                stMsgDes.bIsEncrypt = DOS_FALSE;
                stMsgDes.bIsCompress = DOS_FALSE;
                dos_memcpy(stMsgDes.aulServIp, g_stServMsg.achLocalIP, IPV6_SIZE);
                stMsgDes.usServPort = g_stServMsg.usLocalPort;
                pt_logr_debug("ptc_send_msg2pts : lost/resend/confirm/exitNotify request, seq : %d", pstNeedSendNode->lSeqResend);
                sendto(lSockfd, (VOID *)&stMsgDes, sizeof(PT_MSG_TAG), 0,  (struct sockaddr*)&g_pstPtcSend->stDestAddr, sizeof(g_pstPtcSend->stDestAddr));

                dos_dmem_free(pstNeedSendNode);
                pstNeedSendNode = NULL;
                continue;
            }
            /* �������ݰ� */
            pstStreamHead = g_pstPtcSend->astDataTypes[pstNeedSendNode->enDataType].pstStreamQueHead;
            if (NULL == pstStreamHead)
            {
                pt_logr_info("ptc_send_msg2pts : stream list head is NULL, cann't get data package");
                dos_dmem_free(pstNeedSendNode);
                pstNeedSendNode = NULL;
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

            if (pstNeedSendNode->bIsResend)
            {
                /* Ҫ���ش��İ� */
                ulArraySub = pstNeedSendNode->lSeqResend & (PT_DATA_SEND_CACHE_SIZE - 1);  /* Ҫ���͵İ���data�����е��±� */
                if (pstSendDataHead[ulArraySub].lSeq != pstNeedSendNode->lSeqResend)
                {
                    /* Ҫ���͵İ������� */
                    pt_logr_info("ptc_send_msg2pts : need resend data is not exit : seq = %d ", pstNeedSendNode->lSeqResend);
                }
                else
                {
                    stSendDataNode = pstSendDataHead[ulArraySub];
                    stMsgDes.enDataType = pstNeedSendNode->enDataType;
                    dos_memcpy(stMsgDes.aucID, g_pstPtcSend->aucID, PTC_ID_LEN);
                    stMsgDes.ulStreamID = dos_htonl(pstNeedSendNode->ulStreamID);
                    stMsgDes.ExitNotifyFlag = stSendDataNode.ExitNotifyFlag;
                    stMsgDes.lSeq = dos_htonl(pstNeedSendNode->lSeqResend);
                    stMsgDes.enCmdValue = pstNeedSendNode->enCmdValue;
                    stMsgDes.bIsEncrypt = DOS_FALSE;
                    stMsgDes.bIsCompress = DOS_FALSE;
                    dos_memcpy(stMsgDes.aulServIp, g_stServMsg.achLocalIP, IPV6_SIZE);
                    stMsgDes.usServPort = g_stServMsg.usLocalPort;

                    dos_memcpy(acBuff, (VOID *)&stMsgDes, sizeof(PT_MSG_TAG));
                    dos_memcpy(acBuff+sizeof(PT_MSG_TAG), stSendDataNode.szBuff, stSendDataNode.ulLen);

                    /* �ش��ģ��������� */
                    ulSendCount = PT_RESEND_RSP_COUNT;
                    while (ulSendCount)
                    {
                        //usleep(g_ulSendTimeSleep);
                        lResult = sendto(lSockfd, acBuff, stSendDataNode.ulLen + sizeof(PT_MSG_TAG), 0, (struct sockaddr*)&g_pstPtcSend->stDestAddr, sizeof(g_pstPtcSend->stDestAddr));
                        ulSendCount--;
                        pt_logr_info("ptc_send_msg2pts : send resend data seq : %d, stream, result = %d", pstNeedSendNode->lSeqResend, pstNeedSendNode->ulStreamID, lResult);

                    }
                }
                dos_dmem_free(pstNeedSendNode);
                pstNeedSendNode = NULL;
                continue;
            }

            while(1)
            {
                 /* ����data��ֱ�������� */
                pstStreamNode->lCurrSeq++;
                ulArraySub = (pstStreamNode->lCurrSeq) & (PT_DATA_SEND_CACHE_SIZE - 1);
                stRecvDataTcp = pstStreamNode->unDataQueHead.pstDataTcp[ulArraySub];
                if (stRecvDataTcp.lSeq == pstStreamNode->lCurrSeq)
                {
                    stSendDataNode = pstSendDataHead[ulArraySub];
                    stMsgDes.enDataType = pstNeedSendNode->enDataType;
                    dos_memcpy(stMsgDes.aucID, g_pstPtcSend->aucID, PTC_ID_LEN);
                    stMsgDes.ulStreamID = dos_htonl(pstNeedSendNode->ulStreamID);
                    stMsgDes.ExitNotifyFlag = stSendDataNode.ExitNotifyFlag;
                    stMsgDes.lSeq = dos_htonl(pstStreamNode->lCurrSeq);
                    stMsgDes.enCmdValue = pstNeedSendNode->enCmdValue;
                    stMsgDes.bIsEncrypt = DOS_FALSE;
                    stMsgDes.bIsCompress = DOS_FALSE;
                    dos_memcpy(stMsgDes.aulServIp, g_stServMsg.achLocalIP, IPV6_SIZE);
                    stMsgDes.usServPort = g_stServMsg.usLocalPort;

                    dos_memcpy(acBuff, (VOID *)&stMsgDes, sizeof(PT_MSG_TAG));
                    dos_memcpy(acBuff+sizeof(PT_MSG_TAG), stSendDataNode.szBuff, stSendDataNode.ulLen);

                    //usleep(g_ulSendTimeSleep);
                    lResult = sendto(lSockfd, acBuff, stSendDataNode.ulLen + sizeof(PT_MSG_TAG), 0, (struct sockaddr*)&g_pstPtcSend->stDestAddr, sizeof(g_pstPtcSend->stDestAddr));
                    pt_logr_info("ptc_send_msg2pts : send data  length:%d, seq:%d, stream:%d, result:%d", stRecvDataTcp.ulLen, pstStreamNode->lCurrSeq, pstNeedSendNode->ulStreamID, lResult);
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

        g_pstPtcNendSendNode = pstNendSendList;
        #if PT_MUTEX_DEBUG
        ptc_send_pthread_mutex_unlock(__FILE__, __LINE__);
        #else
        pthread_mutex_unlock(&g_mutex_send);
        #endif
    }
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
    S32         lSockfd    = *(S32*)arg;
    S32         lRecvLen   = 0;
    S32         lResult    = 0;
    U32         MaxFdp     = lSockfd;
    PT_MSG_TAG *pstMsgDes  = NULL;
    struct sockaddr_in stClientAddr = g_pstPtcSend->stDestAddr;
    socklen_t          ulCliaddrLen = sizeof(stClientAddr);
    fd_set             ReadFdsCpio;
    fd_set             ReadFds;
    struct timeval stTimeVal = {1, 0};
    struct timeval stTimeValCpy;
    FD_ZERO(&ReadFds);
    FD_SET(lSockfd, &ReadFds);
    S8 acRecvBuf[PT_SEND_DATA_SIZE]  = {0};
    struct timeval now;
    sem_init(&g_SemPtcRecv, 0, 1);  /* ��ʼ���ź��� */
    g_lUdpSocket = lSockfd;

    while (1)
    {
        stTimeValCpy = stTimeVal;
        ReadFdsCpio = ReadFds;
        lResult = select(MaxFdp + 1, &ReadFdsCpio, NULL, NULL, &stTimeValCpy);
        if (lResult < 0)
        {
            perror("ptc_recv_msg_from_pts : fail to select");
            exit(DOS_FAIL);
        }
        else if (0 == lResult)
        {
            continue;
        }

        if (FD_ISSET(lSockfd, &ReadFdsCpio))
        {
            lRecvLen = recvfrom(lSockfd, acRecvBuf, PT_SEND_DATA_SIZE, 0, (struct sockaddr*)&stClientAddr, &ulCliaddrLen);
            if (lRecvLen < 0)
            {
                perror("recvfrom");
                continue;
            }
            pt_logr_debug("ptc recv msg from pts : data size:%d", lRecvLen);
            sem_wait(&g_SemPtcRecv);
            #if PT_MUTEX_DEBUG
            ptc_recv_pthread_mutex_lock(__FILE__, __LINE__);
            #else
            pthread_mutex_lock(&g_mutex_recv);
            #endif
            /* ȡ����Ϣͷ������ */
            pstMsgDes = (PT_MSG_TAG *)acRecvBuf;
            /* �ֽ���ת�� */
            pstMsgDes->ulStreamID = dos_ntohl(pstMsgDes->ulStreamID);
            pstMsgDes->lSeq = dos_ntohl(pstMsgDes->lSeq);
            if (pstMsgDes->enCmdValue == PT_CMD_RESEND)
            {
                /* �ش����� */
                #if PT_MUTEX_DEBUG
                ptc_send_pthread_mutex_lock(__FILE__, __LINE__);
                #else
                pthread_mutex_lock(&g_mutex_send);
                #endif
                pt_logr_info("ptc recv resend req, seq : %d, stream : %d", pstMsgDes->lSeq, pstMsgDes->ulStreamID);

                BOOL bIsResend = DOS_TRUE;
                PT_CMD_EN enCmdValue = PT_CMD_NORMAL;

                g_pstPtcNendSendNode = pt_need_send_node_list_insert(g_pstPtcNendSendNode, g_pstPtcSend->aucID, pstMsgDes, enCmdValue, bIsResend);
                pthread_cond_signal(&g_cond_send);
                #if PT_MUTEX_DEBUG
                ptc_send_pthread_mutex_unlock(__FILE__, __LINE__);
                #else
                pthread_mutex_unlock(&g_mutex_send);
                #endif
                #if PT_MUTEX_DEBUG
                ptc_recv_pthread_mutex_unlock(__FILE__, __LINE__);
                #else
                pthread_mutex_unlock(&g_mutex_recv);
                #endif
                sem_post(&g_SemPtcRecv);
                continue;
            }
            else if (pstMsgDes->enCmdValue == PT_CMD_CONFIRM)
            {
                /* ȷ�Ͻ��� */
                gettimeofday(&now, NULL);
                pt_logr_info("make sure recv seq : %d", pstMsgDes->lSeq);
                ptc_deal_with_confirm_msg(pstMsgDes);
                #if PT_MUTEX_DEBUG
                ptc_recv_pthread_mutex_unlock(__FILE__, __LINE__);
                #else
                pthread_mutex_unlock(&g_mutex_recv);
                #endif
                sem_post(&g_SemPtcRecv);
                continue;
            }
            else if (pstMsgDes->ExitNotifyFlag == DOS_TRUE)
            {
                /* ��Ӧ���������streamID�ڵ� */
                ptc_delete_send_stream_node(pstMsgDes->ulStreamID, pstMsgDes->enDataType, DOS_TRUE);
                ptc_delete_recv_stream_node(pstMsgDes->ulStreamID, pstMsgDes->enDataType, DOS_FALSE);
#if PT_MUTEX_DEBUG
                ptc_recv_pthread_mutex_unlock(__FILE__, __LINE__);
#else
                pthread_mutex_unlock(&g_mutex_recv);
#endif
                sem_post(&g_SemPtcRecv);
                continue;
            }

            if (pstMsgDes->enDataType == PT_DATA_CTRL)
            {
                /* ������Ϣ */
                ptc_ctrl_msg_handle(pstMsgDes, acRecvBuf);
                #if PT_MUTEX_DEBUG
                ptc_recv_pthread_mutex_unlock(__FILE__, __LINE__);
                #else
                pthread_mutex_unlock(&g_mutex_recv);
                #endif
                sem_post(&g_SemPtcRecv);
                continue;
            }

            lResult = ptc_save_into_recv_cache(g_pstPtcRecv, acRecvBuf, lRecvLen, lSockfd);
            if (lResult < 0)
            {
                pt_logr_error("ptc add data into recv cache fail, result : %d", lResult);
                #if PT_MUTEX_DEBUG
                ptc_recv_pthread_mutex_unlock(__FILE__, __LINE__);
                #else
                pthread_mutex_unlock(&g_mutex_recv);
                #endif
                sem_post(&g_SemPtcRecv);
                continue;
            }

            pt_logr_debug("ptc recv from pts length:%d, seq:%d, stream:%d", lRecvLen, pstMsgDes->lSeq, pstMsgDes->ulStreamID);
            if (NULL == pt_need_recv_node_list_search(g_pstPtcNendRecvNode, pstMsgDes->ulStreamID))
            {
                g_pstPtcNendRecvNode = pt_need_recv_node_list_insert(g_pstPtcNendRecvNode, pstMsgDes);
            }
            pthread_cond_signal(&g_cond_recv);

#if PT_MUTEX_DEBUG
            ptc_recv_pthread_mutex_unlock(__FILE__, __LINE__);
#else
            pthread_mutex_unlock(&g_mutex_recv);
#endif
            if (lResult == PT_NEED_CUT_PTHREAD)
            {
                usleep(10); /* �����̣߳�ִ�н��պ��� */
            }
        }
    }
}

#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */
