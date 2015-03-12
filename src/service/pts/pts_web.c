#ifdef  __cplusplus
extern "C"{
#endif

#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/select.h>
#include <limits.h>
#include <netdb.h>
#include <ctype.h>
#include <sys/time.h>
#include <semaphore.h>
#include <netinet/tcp.h>

#include <dos.h>
#include <pt/dos_sqlite3.h>
#include <pt/pts.h>
#include "pts_msg.h"
#include "pts_web.h"


list_t  *m_stClinetCBList = NULL; /* �ͻ����������� */
pthread_mutex_t g_web_client_mutex  = PTHREAD_MUTEX_INITIALIZER;   /* ���� m_stClinetCBList ���ź��� */
S32 m_lPtcSeq = 0;
fd_set g_ReadFds;                 /* �ͻ����׽��ּ��� */
S32 g_lMaxSocket = 0;
S32 g_lWebServSocket = 0;
const S8 *szPtsFifoName = "/tmp/pts_fifo";
S32 g_lPtsPipeWRFd = -1;

VOID pts_web_sem_wait(S8 *szFileName, U32 ulLine)
{
    printf("%s %d %.*s : sem wait\n", szFileName, ulLine, PTS_TIME_SIZE, pts_get_current_time());
    pthread_mutex_lock(&g_web_client_mutex);
    printf("%s %d %.*s : sem wait in\n", szFileName, ulLine, PTS_TIME_SIZE, pts_get_current_time());
}

VOID pts_web_sem_post(S8 *szFileName, U32 ulLine)
{
    printf("%s %d %.*s : sem post\n", szFileName, ulLine, PTS_TIME_SIZE, pts_get_current_time());
    pthread_mutex_unlock(&g_web_client_mutex);
}

VOID pts_web_free_stream(PT_NEND_RECV_NODE_ST *pstNeedRecvNode, PT_STREAM_CB_ST *pstStreamNode, PT_CC_CB_ST *pstCCNode)
{
    PT_MSG_TAG stMsgDes;

    if (NULL == pstNeedRecvNode || NULL == pstStreamNode || NULL == pstCCNode)
    {
        return;
    }

    dos_memcpy(stMsgDes.aucID, pstNeedRecvNode->aucID, PTC_ID_LEN);
    stMsgDes.enDataType = pstNeedRecvNode->enDataType;
    stMsgDes.ulStreamID = pstNeedRecvNode->ulStreamID;
    /* ��ptc���ͽ���֪ͨ */
    pts_send_exit_notify2ptc(pstCCNode, pstNeedRecvNode);
    /* �ͷ�stream�ڵ� */
    pts_delete_recv_stream_node(&stMsgDes, pstCCNode, DOS_FALSE);
    pts_delete_send_stream_node(&stMsgDes, NULL, DOS_TRUE);
}


VOID pts_web_close_socket(S32 lSocket)
{
    FD_CLR(lSocket, &g_ReadFds);
    #if PT_WEB_MUTEX_DEBUG
    pts_web_sem_wait(__FILE__, __LINE__);
    #else
    pthread_mutex_lock(&g_web_client_mutex);
    #endif
    m_stClinetCBList = pts_clinetCB_delete_by_sockfd(m_stClinetCBList, lSocket);
    #if PT_WEB_MUTEX_DEBUG
    pts_web_sem_post(__FILE__, __LINE__);
    #else
    pthread_mutex_unlock(&g_web_client_mutex);
    #endif
    close(lSocket);
}

VOID pts_web_close_socket_without_sem(S32 lSocket)
{
    FD_CLR(lSocket, &g_ReadFds);
    m_stClinetCBList = pts_clinetCB_delete_by_sockfd(m_stClinetCBList, lSocket);
    close(lSocket);
}

VOID pts_web_free_resource(S32 lSocket)
{
    list_t           *pstStreamList         = NULL;
    PT_CC_CB_ST      *pstCCNode             = NULL;
    PT_STREAM_CB_ST  *pstStreamNode         = NULL;
    PT_NEND_RECV_NODE_ST stNeedRecvNode;
    PT_MSG_TAG stMsgDes;
    PTS_CLIENT_CB_ST stClientMsg;
    PTS_CLIENT_CB_ST *pstClient = NULL;

    #if PT_WEB_MUTEX_DEBUG
    pts_web_sem_wait(__FILE__, __LINE__);
    #else
    pthread_mutex_lock(&g_web_client_mutex);
    #endif
    pstClient = pts_clinetCB_search_by_sockfd(m_stClinetCBList, lSocket);
    if (pstClient == NULL)
    {
        #if PT_WEB_MUTEX_DEBUG
        pts_web_sem_post(__FILE__, __LINE__);
        #else
        pthread_mutex_unlock(&g_web_client_mutex);
        #endif
        return;
    }
    stClientMsg = *pstClient;
    m_stClinetCBList = pts_clinetCB_delete_node(m_stClinetCBList, pstClient);
    //m_stClinetCBList = pts_clinetCB_delete_by_sockfd(m_stClinetCBList, lSocket);
    #if PT_WEB_MUTEX_DEBUG
    pts_web_sem_post(__FILE__, __LINE__);
    #else
    pthread_mutex_unlock(&g_web_client_mutex);
    #endif

    #if PT_MUTEX_DEBUG
    pts_send_pthread_mutex_lock(__FILE__, __LINE__);
    #else
    pthread_mutex_lock(&g_pts_mutex_send);
    #endif
    pstCCNode = pt_ptc_list_search(g_pstPtcListSend, stClientMsg.aucID);
    if(NULL == pstCCNode)
    {
        #if PT_MUTEX_DEBUG
        pts_send_pthread_mutex_unlock(__FILE__, __LINE__);
        #else
        pthread_mutex_unlock(&g_pts_mutex_send);
        #endif
        return;
    }

    pstStreamList = pstCCNode->astDataTypes[PT_DATA_WEB].pstStreamQueHead;
    if (NULL == pstStreamList)
    {
        #if PT_MUTEX_DEBUG
        pts_send_pthread_mutex_unlock(__FILE__, __LINE__);
        #else
        pthread_mutex_unlock(&g_pts_mutex_send);
        #endif
        return;
    }

    pstStreamNode = pt_stream_queue_search(pstStreamList, stClientMsg.ulStreamID);
    if (NULL == pstStreamNode)
    {
        #if PT_MUTEX_DEBUG
        pts_send_pthread_mutex_unlock(__FILE__, __LINE__);
        #else
        pthread_mutex_unlock(&g_pts_mutex_send);
        #endif
        return;
    }

    if (NULL == pstStreamNode->unDataQueHead.pstDataTcp)
    {
        #if PT_MUTEX_DEBUG
        pts_send_pthread_mutex_unlock(__FILE__, __LINE__);
        #else
        pthread_mutex_unlock(&g_pts_mutex_send);
        #endif
        return;
    }

    dos_memcpy(stNeedRecvNode.aucID, stClientMsg.aucID, PTC_ID_LEN);
    stNeedRecvNode.enDataType = PT_DATA_WEB;
    stNeedRecvNode.ulStreamID = stClientMsg.ulStreamID;
    dos_memcpy(stMsgDes.aucID, stClientMsg.aucID, PTC_ID_LEN);
    stMsgDes.enDataType = PT_DATA_WEB;
    stMsgDes.ulStreamID = stClientMsg.ulStreamID;

    /* �ͷ��շ����� */
    pts_delete_send_stream_node(&stMsgDes, pstCCNode, DOS_FALSE);
    #if PT_MUTEX_DEBUG
    pts_send_pthread_mutex_unlock(__FILE__, __LINE__);
    #else
    pthread_mutex_unlock(&g_pts_mutex_send);
    #endif

    pts_delete_recv_stream_node(&stMsgDes, NULL, DOS_TRUE);
    pts_send_exit_notify2ptc(pstCCNode, &stNeedRecvNode);
}

/**
 * ������list_t *pts_clinetCB_insert(list_t *psthead, S32 lSockfd, struct sockaddr_in stClientAddr)
 * ���ܣ������µ��������������
 * ����
 * ����ֵ:
 */
list_t *pts_clinetCB_insert(list_t *psthead, S32 lSockfd, struct sockaddr_in stClientAddr)
{
    PTS_CLIENT_CB_ST *stNewNode = NULL;

    stNewNode = (PTS_CLIENT_CB_ST *)dos_dmem_alloc(sizeof(PTS_CLIENT_CB_ST));
    if (NULL == stNewNode)
    {
        perror("dos_dmem_malloc");
        return NULL;
    }

    stNewNode->lSocket = lSockfd;
    stNewNode->ulStreamID = pts_create_stream_id();
    stNewNode->stClientAddr = stClientAddr;
    stNewNode->eSaveHeadFlag = DOS_FALSE;
    pt_logr_debug("sockfd : %d, stream : %d", lSockfd, stNewNode->ulStreamID);
    if (NULL == psthead)
    {
        psthead = &(stNewNode->stList);
        dos_list_init(psthead);
    }
    else
    {
        dos_list_add_tail(psthead, &(stNewNode->stList));
    }

    return psthead;
}

/**
 * ������PTS_CLIENT_CB_ST* pts_clinetCB_search_by_sockfd(list_t *pstHead, S32 lSockfd)
 * ���ܣ��������������еĽڵ㣬ͨ��lSockfd
 * ����
 * ����ֵ:
 */
PTS_CLIENT_CB_ST* pts_clinetCB_search_by_sockfd(list_t *pstHead, S32 lSockfd)
{
    list_t    *pstNode = NULL;
    PTS_CLIENT_CB_ST *pstData = NULL;

    if (NULL == pstHead)
    {
        pt_logr_debug("empty list!");
        return NULL;
    }

    pstNode = pstHead;
    pstData = dos_list_entry(pstNode, PTS_CLIENT_CB_ST, stList);
    while (pstData->lSocket != lSockfd && pstNode->next != pstHead)
    {
        pstNode = pstNode->next;
        pstData = (PTS_CLIENT_CB_ST *)dos_list_entry(pstNode, PTS_CLIENT_CB_ST, stList);
    }
    if (pstData->lSocket == lSockfd)
    {
        return pstData;
    }
    else
    {
        pt_logr_debug("not found!");
        return NULL;
    }
}

/**
 * ������PTS_CLIENT_CB_ST* pts_clinetCB_search_by_stream(list_t* pstHead, U32 ulStreamID)
 * ���ܣ��������������еĽڵ㣬ͨ��ulStreamID
 * ����
 * ����ֵ:
 */
PTS_CLIENT_CB_ST* pts_clinetCB_search_by_stream(list_t* pstHead, U32 ulStreamID)
{
    list_t *pstNode = NULL;
    PTS_CLIENT_CB_ST *pstData = NULL;

    if (NULL == pstHead)
    {
        pt_logr_debug("PTS_CLIENT_CB_ST search : empty list");
        return NULL;
    }

    pstNode = pstHead;
    pstData = dos_list_entry(pstNode, PTS_CLIENT_CB_ST, stList);
    while (pstData->ulStreamID!=ulStreamID && pstNode->next!=pstHead)
    {
        pstNode = pstNode->next;
        pstData = dos_list_entry(pstNode, PTS_CLIENT_CB_ST, stList);
    }
    if (pstData->ulStreamID == ulStreamID)
    {
        return pstData;
    }
    else
    {
        pt_logr_debug("PTS_CLIENT_CB_ST search by stream : not found");
        return NULL;
    }
}

list_t *pts_clinetCB_delete_node(list_t* pstHead, PTS_CLIENT_CB_ST* pstNode)
{
    if (NULL == pstHead || NULL == pstNode)
    {
        return pstHead;
    }

    if (pstHead == &pstNode->stList)
    {
        if (pstHead->next == pstHead)
        {
            pstHead = NULL;
        }
        else
        {
            pstHead = pstHead->next;
        }
    }

    dos_list_del(&pstNode->stList);
    pstNode->lSocket = -1;
    dos_dmem_free(pstNode);

    return pstHead;
}

/**
 * ������list_t *pts_clinetCB_delete_by_sockfd(list_t* pstHead, S32 lSockfd)
 * ���ܣ�ɾ�����������еĽڵ㣬ͨ��lSockfd
 * ����
 * ����ֵ:
 */
list_t *pts_clinetCB_delete_by_sockfd(list_t* pstHead, S32 lSockfd)
{
    list_t *pstNode = NULL;
    PTS_CLIENT_CB_ST *pstData = NULL;

    if (NULL == pstHead)
    {
        pt_logr_debug("PTS_CLIENT_CB_ST delete : empty list");
        return pstHead;
    }

    pstNode = pstHead;
    pstData = dos_list_entry(pstNode, PTS_CLIENT_CB_ST, stList);

    while (pstData->lSocket != lSockfd && pstNode->next != pstHead)
    {
        pstNode = pstNode->next;
        pstData = dos_list_entry(pstNode, PTS_CLIENT_CB_ST, stList);
    }
    if (pstData->lSocket == lSockfd)
    {
        if (pstNode == pstHead)
        {
            if (pstNode->next == pstHead)
            {
                pstHead = NULL;
            }
            else
            {
                pstHead = pstNode->next;
            }
        }
        dos_list_del(pstNode);
        pstData->lSocket = -1;
        dos_dmem_free(pstData);
        pstData = NULL;
    }
    else
    {
        pt_logr_debug("PTS_CLIENT_CB_ST delete : not found!");
    }

    return pstHead;
}

/**
 * ������list_t *pts_clinetCB_delete_by_stream(list_t* pstHead, U32 ulStreamID)
 * ���ܣ�ɾ�����������еĽڵ㣬ͨ��ulStreamID
 * ����
 * ����ֵ:
 */
list_t *pts_clinetCB_delete_by_stream(list_t* pstHead, U32 ulStreamID)
{
    list_t    *pstNode = NULL;
    PTS_CLIENT_CB_ST *pstData = NULL;

    if (NULL == pstHead)
    {
        pt_logr_debug("empty list!");
        return pstHead;
    }

    pstNode = pstHead;
    pstData = dos_list_entry(pstNode, PTS_CLIENT_CB_ST, stList);

    while (pstData->ulStreamID != ulStreamID && pstNode->next != pstHead)
    {
        pstNode = pstNode->next;
        pstData = dos_list_entry(pstNode, PTS_CLIENT_CB_ST, stList);
    }
    if (pstData->ulStreamID == ulStreamID)
    {
        if (pstNode == pstHead)
        {
            if (pstNode->next == pstHead)
            {
                pstHead = NULL;
            }
            else
            {
                pstHead = pstNode->next;
            }
        }
        dos_list_del(pstNode);
        close(pstData->lSocket);
        FD_CLR(pstData->lSocket, &g_ReadFds);
        dos_dmem_free(pstData);
        pstData = NULL;
    }
    else
    {
        pt_logr_debug("not found!");
    }

    return pstHead;
}

/**
 * ������BOOL pts_is_http_head(S8 *pcRequest)
 * ���ܣ��ж��Ƿ���http����ͷ
 * ����
 * ����ֵ:
 */
BOOL pts_is_http_head(S8 *pcRequest)
{
    if (NULL == pcRequest)
    {
        return DOS_FALSE;
    }

    if (NULL != dos_strstr(pcRequest,"HTTP/1.1"))
    {
        return DOS_TRUE;
    }
    return DOS_FALSE;
}

/**
 * ������BOOL pts_is_http_end(S8 *pcRequest)
 * ���ܣ��ж�http����ͷ�Ƿ����
 * ����
 * ����ֵ:
 */
BOOL pts_is_http_end(S8 *pcRequest)
{
    if (NULL == pcRequest)
    {
        return DOS_FALSE;
    }

    if (NULL != dos_strstr(pcRequest,"\r\n\r\n"))
    {
        return DOS_TRUE;
    }
    return DOS_FALSE;
}

/**
 * ������BOOL pts_request_ptc_proxy(S8 *pcRequest, U32 ulConnfd, U32 ulStreamID, U8* pcIpccId, S32 lReqLen, S8 *szUrl)
 * ���ܣ���������� ptc proxy
 * ����
 * ����ֵ:
 */
BOOL pts_request_ptc_proxy(S8 *pcRequest, U32 ulConnfd, U32 ulStreamID, U8* pcIpccId, S32 lReqLen, S8 *szUrl)
{
    BOOL bIsGetID = DOS_FALSE;
    U16 usDestPort = 0;
    U32 ulSendBufSize = 0;
    S8 *pStr1 = NULL;
    S8 *pcCookie = NULL;
    S8 szDestIP[PT_IP_ADDR_SIZE] = {0};
    U8 aucDestID[PTC_ID_LEN+1] = {0};
    S8 szDestPortStr[PT_DATA_BUFF_16] = {0};
    S8 szCookieId[PT_DATA_BUFF_16] = {0};
    S8 szServPort[PT_DATA_BUFF_16] = {0};
    /* ��ȡ����Ķ˿� */
    pStr1 = dos_strstr(pcRequest, "Host:");
    if (NULL == pStr1)
    {
        pts_web_close_socket(ulConnfd);
        return bIsGetID;
    }
    sscanf(pStr1, "%*[^:]:%*[^:]:%[0-9]", szServPort);

    snprintf(szCookieId, PT_DATA_BUFF_16, "ptsId_%s", szServPort);

    pcCookie = dos_strstr(pcRequest, szCookieId);
    if (pcCookie != NULL)
    {
        sscanf(pcCookie, "%*[^=]=%[^!]!%[^!]!%[0-9]", aucDestID, szDestIP, szDestPortStr);
        usDestPort = atoi(szDestPortStr);
        pt_logr_debug("aucDestID = %s, szDestIP = %s, szDestPortStr = %s", aucDestID, szDestIP, szDestPortStr);
        pStr1 = pcRequest;
        ulSendBufSize = lReqLen;
        pt_logr_debug("pts send web server filename : %s, sockfd : %d, stream : %d", szUrl, ulConnfd, ulStreamID);

        while (1)
        {
            if (ulSendBufSize <= PT_RECV_DATA_SIZE)
            {
                pts_save_msg_into_cache(aucDestID, PT_DATA_WEB, ulStreamID, pStr1, ulSendBufSize, szDestIP, usDestPort);
                break;
            }
            else
            {
                pts_save_msg_into_cache(aucDestID, PT_DATA_WEB, ulStreamID, pStr1, PT_RECV_DATA_SIZE, szDestIP, usDestPort);
                ulSendBufSize -= PT_RECV_DATA_SIZE;
                pStr1 += PT_RECV_DATA_SIZE;
            }
        }
        dos_memcpy(pcIpccId, aucDestID, PTC_ID_LEN);
        bIsGetID = DOS_TRUE;
    }
    else
    {
        pt_logr_info("not get cookie");
        pts_web_close_socket(ulConnfd);
    }

    return bIsGetID;
}

/**
 * ������BOOL pts_deal_with_http_head(S8 *pcRequest, U32 ulConnfd, U32 ulStreamID, U8* pcIpccId, S32 lReqLen)
 * ���ܣ�
 *      1.����web���������
 * ����
 *      S8 *pcRequest :  ������
 *      U32 ulConnfd:    �׽���
 *      U32 ulStreamID:  stream ID
 *      U8* pcIpccId:    ��ȡ�豸��ID
 *      S32 lReqLen:     ���ĵĴ�С
 * ����ֵ��BOOL:   �Ƿ��ñ��ĵĴ�С
 */
BOOL pts_deal_with_http_head(S8 *pcRequest, U32 ulConnfd, U32 ulStreamID, U8* pcIpccId, S32 lReqLen)
{
    if (NULL == pcRequest || NULL == pcIpccId)
    {
        pt_logr_debug("pts_deal_with_http_head : param null");
        return DOS_FALSE;
    }

    S8 szUrl[PT_DATA_BUFF_128] = {0};
    BOOL bIsGetID = DOS_FALSE;

    pt_logr_info("send req socket : %d, ulStreamID : %d", ulConnfd, ulStreamID);
    bIsGetID = pts_request_ptc_proxy(pcRequest, ulConnfd, ulStreamID, pcIpccId, lReqLen, szUrl);

    return bIsGetID;
}

/**
 * ������VOID pts_deal_with_web_request(S8 *pcRequest, U32 ulConnfd, S32 lReqLen)
 * ���ܣ��������������
 * ����
 * ����ֵ:
 */
VOID pts_deal_with_web_request(S8 *pcRequest, U32 ulConnfd, S32 lReqLen)
{
    PTS_CLIENT_CB_ST* pstHttpHead   = NULL;
    S8 *pcHeadBuf = NULL;                //PT_SEND_DATA_SIZE
    BOOL bIsGetID = DOS_FALSE;
    U8 *pcIpccId = (U8 *)dos_dmem_alloc(PTC_ID_LEN);
    if (NULL == pcIpccId)
    {
        perror("dos_dmem_malloc");
        /* TODO */
        return;
    }

    dos_memzero(pcIpccId, PTC_ID_LEN);
    pstHttpHead = pts_clinetCB_search_by_sockfd(m_stClinetCBList, ulConnfd);
    if (NULL == pstHttpHead)
    {
        return;
    }

    if (!pstHttpHead->eSaveHeadFlag)
    {
        if (pts_is_http_head(pcRequest))
        {
            if (pts_is_http_end(pcRequest))
            {
                bIsGetID = pts_deal_with_http_head(pcRequest, ulConnfd, pstHttpHead->ulStreamID, pcIpccId, lReqLen);
                if (bIsGetID)
                {
                    dos_memcpy(pstHttpHead->aucID, pcIpccId, PTC_ID_LEN);
                }
            }
            else
            {
                pstHttpHead->pcRequestHead = (S8 *)dos_dmem_alloc(lReqLen);
                if (NULL == pstHttpHead->pcRequestHead)
                {
                    perror("dos_dmem_malloc");
                    return;
                }
                else
                {
                    dos_memcpy(pstHttpHead->pcRequestHead, pcRequest, lReqLen);
                    pstHttpHead->lReqLen = lReqLen;
                    pstHttpHead->eSaveHeadFlag = DOS_TRUE;
                }
            }
        }
        else
        {
            pts_save_msg_into_cache(pstHttpHead->aucID, PT_DATA_WEB, pstHttpHead->ulStreamID, pcRequest, lReqLen, NULL, 0);
        }
    }
    else
    {
        pcHeadBuf = (S8 *)dos_dmem_alloc(pstHttpHead->lReqLen + lReqLen);
        if (NULL == pcHeadBuf)
        {
            perror("malloc");
            return;
        }

        dos_memcpy(pcHeadBuf, pstHttpHead->pcRequestHead, pstHttpHead->lReqLen);
        dos_memcpy(pcHeadBuf+pstHttpHead->lReqLen, pcRequest, lReqLen);

        if (pts_is_http_end(pcHeadBuf))
        {

            bIsGetID = pts_deal_with_http_head(pcHeadBuf, ulConnfd, pstHttpHead->ulStreamID, pcIpccId, pstHttpHead->lReqLen+lReqLen);
            if (bIsGetID)
            {
                dos_memcpy(pstHttpHead->aucID, pcIpccId, PTC_ID_LEN);
            }
            dos_dmem_free(pstHttpHead->pcRequestHead);
            pstHttpHead->pcRequestHead = NULL;
            pstHttpHead->eSaveHeadFlag = DOS_FALSE;
            dos_dmem_free(pcHeadBuf);
            pcHeadBuf = NULL;
        }
        else
        {
            dos_dmem_free(pstHttpHead->pcRequestHead);
            pstHttpHead->pcRequestHead = pcHeadBuf;
            pstHttpHead->lReqLen += lReqLen;
        }

    }
    dos_dmem_free(pcIpccId);
    pcIpccId = NULL;

    return;
}


VOID pts_web_timeout_callback(U64 arg)
{
    U32 ulArraySub = (U32)arg;

    g_lPtsServSocket[ulArraySub].hTmrHandle = NULL;
    FD_CLR(g_lPtsServSocket[ulArraySub].lSocket, &g_ReadFds);
    close(g_lPtsServSocket[ulArraySub].lSocket);
    g_lPtsServSocket[ulArraySub].lSocket = -1;
    pt_logr_debug("timer timeout port : %d\n", g_lPtsServSocket[ulArraySub].usServPort);
}

VOID pts_create_web_serv_socket(U32 ulArraySeq)
{
    U16 usPort = g_lWebServSocket;
    S32 lSocket = 0;
    U32 lResult = 0;

    while(1)
    {
        lSocket = pts_create_tcp_socket(usPort);
        if (lSocket <= 0)
        {
            usPort++;
            if (usPort > g_stPtsMsg.usWebServPort + 1000)
            {
                usPort = g_stPtsMsg.usWebServPort;
            }
            continue;
        }
        break;
    }

    FD_SET(lSocket, &g_ReadFds);
    lResult = write(g_lPtsPipeWRFd, "s", 1);
    g_lPtsServSocket[ulArraySeq].lSocket = lSocket;
    g_lPtsServSocket[ulArraySeq].usServPort = usPort;
    g_lMaxSocket = g_lMaxSocket > lSocket ? g_lMaxSocket : lSocket;
    /* ������ʱ�� */
    lResult = dos_tmr_start(&g_lPtsServSocket[ulArraySeq].hTmrHandle, PTS_WEB_TIMEOUT, pts_web_timeout_callback, ulArraySeq, TIMER_NORMAL_NO_LOOP);
    if (lResult < 0)
    {
        pt_logr_info("ptc_send_hb_req : start timer fail");
    }

    if (usPort == g_stPtsMsg.usWebServPort + 1000)
    {
        g_lWebServSocket = g_stPtsMsg.usWebServPort;
    }
    else
    {
        g_lWebServSocket = usPort + 1;
    }
    return;
}


S32 pts_is_serv_socket(S32 lSocket)
{
    S32 i = 0;
    S32 lResult = 0;

    for (i=0; i<PTS_WEB_SERVER_MAX_SIZE; i++)
    {
        if (g_lPtsServSocket[i].lSocket == lSocket)
        {
            if (g_lPtsServSocket[i].hTmrHandle != NULL)
            {
                dos_tmr_stop(&g_lPtsServSocket[i].hTmrHandle);
                g_lPtsServSocket[i].hTmrHandle = NULL;
            }
            lResult = dos_tmr_start(&g_lPtsServSocket[i].hTmrHandle, PTS_WEB_TIMEOUT, pts_web_timeout_callback, i, TIMER_NORMAL_NO_LOOP);
            if (lResult < 0)
            {
                pt_logr_info("ptc_send_hb_req : start timer fail");
            }
            return DOS_TRUE;
        }
    }

    return DOS_FALSE;
}

/**
 * ������VOID *pts_recv_msg_from_web(VOID *arg)
 * ���ܣ��߳� ������������
 * ����
 * ����ֵ:
 */
VOID *pts_recv_msg_from_web(VOID *arg)
{
    //S32 lSocket = 0;
    S32 lConnFd = 0;
    S32 lMaxFdp = 0;
    S32 i       = 0;
    S32 lRecvLen  = 0;
    S32 lResult = 0;
    fd_set ReadFdsCpy;
    S8  szRecvBuf[PT_RECV_DATA_SIZE] = {0};
    struct sockaddr_in stClientAddr;
    socklen_t ulCliaddrLen = sizeof(struct sockaddr_in);
    struct timeval stTimeval = {1, 0};
    struct timeval stTvCopy;
    S32 lPipeFd = -1;
    g_lWebServSocket = g_stPtsMsg.usWebServPort;

    /* �����ܵ� */
    if(access(szPtsFifoName, F_OK) == -1)
    {
        /* �ܵ��ļ�������,���������ܵ� */
        lResult = mkfifo(szPtsFifoName, 0777);
        if(lResult != 0)
        {
            logr_error("Could not create fifo %s\n", szPtsFifoName);
            perror("mkfifo");
            exit(EXIT_FAILURE);
        }
    }

    lPipeFd = open(szPtsFifoName, O_RDONLY | O_NONBLOCK);
    g_lPtsPipeWRFd = open(szPtsFifoName, O_WRONLY);
    FD_ZERO(&g_ReadFds);
    FD_SET(lPipeFd, &g_ReadFds);
    g_lMaxSocket = lPipeFd;

    while (1)
    {
        dos_memcpy(&stTvCopy, &stTimeval, sizeof(struct timeval));
        ReadFdsCpy = g_ReadFds;
        lMaxFdp = g_lMaxSocket;
        lResult = select(lMaxFdp + 1, &ReadFdsCpy, NULL, NULL, &stTvCopy);
        if (lResult < 0)
        {
            perror("pts recv msg from proxy  : fail to select");
            exit(DOS_FAIL);
        }
        else if (0 == lResult)
        {
            continue;
        }

        for (i=0; i<=lMaxFdp; i++)
        {
            if (FD_ISSET(i, &ReadFdsCpy))
            {
                if (i == lPipeFd)
                {
                    lRecvLen = read(lPipeFd, szRecvBuf, PT_RECV_DATA_SIZE);
                    continue;
                }

                if (pts_is_serv_socket(i))
                {
                    pt_logr_debug("%s", "new connect from client to  web service of pts");
                    if ((lConnFd = accept(i, (struct sockaddr*)&stClientAddr, &ulCliaddrLen)) < 0)
                    {
                        perror("pts recv msg from proxy accept");
                        exit(DOS_FAIL);
                    }

                    FD_SET(lConnFd, &g_ReadFds);
                    g_lMaxSocket = g_lMaxSocket > lConnFd ? g_lMaxSocket : lConnFd;
#if PT_WEB_MUTEX_DEBUG
                    pts_web_sem_wait(__FILE__, __LINE__);
#else
                    pthread_mutex_lock(&g_web_client_mutex);
#endif
                    m_stClinetCBList = pts_clinetCB_insert(m_stClinetCBList, lConnFd, stClientAddr);
#if PT_WEB_MUTEX_DEBUG
                    pts_web_sem_post(__FILE__, __LINE__);
#else
                    pthread_mutex_unlock(&g_web_client_mutex);
#endif
                    continue;
                }
                else
                {
                    dos_memzero(szRecvBuf, PT_RECV_DATA_SIZE);
                    lRecvLen = recv(i, szRecvBuf, PT_RECV_DATA_SIZE, 0);
                    if (lRecvLen == 0)
                    {
                        /*if (EAGAIN == errno)
                        {
                            pt_logr_debug("%s", "recv signal eagain\n");
                            continue;
                        }*/
                        FD_CLR(i, &g_ReadFds);
                        pts_web_free_resource(i);
                        close(i);
                    }
                    else if (lRecvLen < 0)
                    {
                        FD_CLR(i, &g_ReadFds);
                        pts_web_free_resource(i);
                        close(i);
                    }
                    else
                    {
                        pts_deal_with_web_request(szRecvBuf, i, lRecvLen);
                    }
                }
            }

        }/*end of for*/

    }/*end of while(1)*/
}


/**
 * ������VOID pts_send_msg2web(PT_NEND_RECV_NODE_ST *pstNeedRecvNode)
 * ���ܣ�������Ϣ��web�����
 * ����
 * ����ֵ:
 */
VOID pts_send_msg2web(PT_NEND_RECV_NODE_ST *pstNeedRecvNode)
{
    S32              lResult                    = 0;
    U32              ulArraySub                 = 0;
    S8               *pcSendMsg                 = NULL;
    //S8             *pcStrchr                  = NULL;
    list_t           *pstStreamList             = NULL;
    PT_CC_CB_ST      *pstCCNode                 = NULL;
    PT_STREAM_CB_ST  *pstStreamNode             = NULL;
    PTS_CLIENT_CB_ST *pstClinetCB               = NULL;
    PT_DATA_TCP_ST   *pstDataTcp                = NULL;
    //S8             cookie[PT_DATA_BUFF_128]   = {0};
    socklen_t optlen = sizeof(S32);
    S32 tcpinfo;

    #if PT_WEB_MUTEX_DEBUG
    pts_web_sem_wait(__FILE__, __LINE__);
    #else
    pthread_mutex_lock(&g_web_client_mutex);
    #endif
    pstClinetCB = pts_clinetCB_search_by_stream(m_stClinetCBList, pstNeedRecvNode->ulStreamID);
    if(NULL == pstClinetCB)
    {
        /* û���ҵ�stream��Ӧ���׽��� */
        pt_logr_debug("pts send msg to proxy : error not found stockfd");
        #if PT_WEB_MUTEX_DEBUG
        pts_web_sem_post(__FILE__, __LINE__);
        #else
        pthread_mutex_unlock(&g_web_client_mutex);
        #endif
        return;
    }

    if (pstNeedRecvNode->ExitNotifyFlag)
    {
        /* �ر�socket */
        pts_web_close_socket_without_sem(pstClinetCB->lSocket);
        #if PT_WEB_MUTEX_DEBUG
        pts_web_sem_post(__FILE__, __LINE__);
        #else
        pthread_mutex_unlock(&g_web_client_mutex);
        #endif
        return;
    }

    pstCCNode = pt_ptc_list_search(g_pstPtcListRecv, pstNeedRecvNode->aucID);
    if(NULL == pstCCNode)
    {
        pt_logr_debug("pts send msg to proxy : not found ptc id is %s", pstNeedRecvNode->aucID);
        #if PT_WEB_MUTEX_DEBUG
        pts_web_sem_post(__FILE__, __LINE__);
        #else
        pthread_mutex_unlock(&g_web_client_mutex);
        #endif
        return;
    }

    pstStreamList = pstCCNode->astDataTypes[pstNeedRecvNode->enDataType].pstStreamQueHead;
    if (NULL == pstStreamList)
    {
        pt_logr_debug("pts send msg to proxy : not found stream list of type is %d", pstNeedRecvNode->enDataType);
        #if PT_WEB_MUTEX_DEBUG
        pts_web_sem_post(__FILE__, __LINE__);
        #else
        pthread_mutex_unlock(&g_web_client_mutex);
        #endif
        return;
    }

    pstStreamNode = pt_stream_queue_search(pstStreamList, pstNeedRecvNode->ulStreamID);
    if (NULL == pstStreamNode)
    {
        pt_logr_debug("pts send msg to proxy : not found stream node of id is %d", pstNeedRecvNode->ulStreamID);
        #if PT_WEB_MUTEX_DEBUG
        pts_web_sem_post(__FILE__, __LINE__);
        #else
        pthread_mutex_unlock(&g_web_client_mutex);
        #endif
        return;
    }

    if (NULL == pstStreamNode->unDataQueHead.pstDataTcp)
    {
        pt_logr_debug("pts send msg to proxy : not found data queue");
        #if PT_WEB_MUTEX_DEBUG
        pts_web_sem_post(__FILE__, __LINE__);
        #else
        pthread_mutex_unlock(&g_web_client_mutex);
        #endif
        return;
    }

    while(1)
    {
        /* ���Ͱ���ֱ�������� */
        pstStreamNode->lCurrSeq++;
        ulArraySub = (pstStreamNode->lCurrSeq) & (PT_DATA_RECV_CACHE_SIZE - 1);
        pstDataTcp = pstStreamNode->unDataQueHead.pstDataTcp;
        if (pstDataTcp[ulArraySub].lSeq == pstStreamNode->lCurrSeq)
        {
            if (pstDataTcp[ulArraySub].ulLen == 0)
            {
                pts_web_close_socket_without_sem(pstClinetCB->lSocket);
                write(g_lPtsPipeWRFd, "s", 1);
                #if PT_WEB_MUTEX_DEBUG
                pts_web_sem_post(__FILE__, __LINE__);
                #else
                pthread_mutex_unlock(&g_web_client_mutex);
                #endif
                pts_web_free_stream(pstNeedRecvNode, pstStreamNode, pstCCNode);
                return;
            }
            /* ��http��Ӧͷ��ptc ID����Ϊcookie */
            pcSendMsg = pstDataTcp[ulArraySub].szBuff;
            {
                if (pstClinetCB->lSocket < 0)
                {
                    #if PT_WEB_MUTEX_DEBUG
                    pts_web_sem_post(__FILE__, __LINE__);
                    #else
                    pthread_mutex_unlock(&g_web_client_mutex);
                    #endif
                    return;
                }
                if (getsockopt(pstClinetCB->lSocket, IPPROTO_TCP, TCP_INFO, &tcpinfo, &optlen) < 0)
                {
                    pt_logr_info("get info fail");
                    exit(1);
                }

                if (TCP_CLOSE == tcpinfo || TCP_CLOSE_WAIT == tcpinfo || TCP_CLOSING == tcpinfo)
                {
                    pts_web_close_socket_without_sem(pstClinetCB->lSocket);
                    #if PT_WEB_MUTEX_DEBUG
                    pts_web_sem_post(__FILE__, __LINE__);
                    #else
                    pthread_mutex_unlock(&g_web_client_mutex);
                    #endif
                    pts_web_free_stream(pstNeedRecvNode, pstStreamNode, pstCCNode);
                    return;
                }

                lResult = send(pstClinetCB->lSocket, pcSendMsg, pstDataTcp[ulArraySub].ulLen, 0);
                if (lResult <= 0)
                {
                    pt_logr_info("send result : %d, socket : %d", lResult, pstClinetCB->lSocket);
                    pts_web_close_socket_without_sem(pstClinetCB->lSocket);
                    #if PT_WEB_MUTEX_DEBUG
                    pts_web_sem_post(__FILE__, __LINE__);
                    #else
                    pthread_mutex_unlock(&g_web_client_mutex);
                    #endif
                    pts_web_free_stream(pstNeedRecvNode, pstStreamNode, pstCCNode);
                    return;
                }
            }
           //pt_logr_notic("pts send msg to web len : %d, seq : %d, stream : %d", pstDataTcp[ulArraySub].ulLen, pstStreamNode->lCurrSeq, pstClinetCB->ulStreamID);
        }
        else
        {
            pstStreamNode->lCurrSeq--;
            break;
        }
    } /* end of while(1) */
    #if PT_WEB_MUTEX_DEBUG
    pts_web_sem_post(__FILE__, __LINE__);
    #else
    pthread_mutex_unlock(&g_web_client_mutex);
    #endif
    return;
}

#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */
