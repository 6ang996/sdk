#ifdef  __cplusplus
extern "C" {
#endif

#include <iconv.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <limits.h>
#include <semaphore.h>
#include <sys/time.h>

#include <pt/dos_sqlite3.h>
#include <pt/pts.h>
#include "pts_msg.h"
#include "pts_web.h"

sem_t     g_SemPts;                         /* �����ź��� */
S32       g_lSeqSend           = 0;         /* ���͵İ���� */
S32       g_lResendSeq         = 0;         /* ��Ҫ���½��յİ���� */
list_t    g_stPtcListSend;          /* ���ͻ��� */
list_t    g_stPtcListRecv;          /* ���ջ��� */
list_t    g_stPtsNendRecvNode;      /* pts��Ҫ���յ����� */
list_t    g_stPtsNendSendNode;      /* pts��Ҫ���͵����� */
list_t    g_stMsgRecvFromPtc;
list_t    g_stSendMsgPthreadList;
DLL_S     g_stStreamAddrList;
PTS_SERV_MSG_ST g_stPtsMsg;                 /* ��Ŵ������ļ��ж�ȡ����pts����Ϣ */
pthread_mutex_t g_mutexPtcSendList          = PTHREAD_MUTEX_INITIALIZER;        /* ����ptc�б���� */
pthread_mutex_t g_mutexPtcRecvList          = PTHREAD_MUTEX_INITIALIZER;        /* ����ptc�б���� */
pthread_mutex_t g_mutexPtsSendPthread       = PTHREAD_MUTEX_INITIALIZER;        /* �����߳��� */
pthread_cond_t  g_condPtsSend               = PTHREAD_COND_INITIALIZER;         /* ������������ */
pthread_mutex_t g_mutexPtsRecvPthread       = PTHREAD_MUTEX_INITIALIZER;        /* �����߳��� */
pthread_cond_t  g_condPtsRecv               = PTHREAD_COND_INITIALIZER;         /* ������������ */
pthread_mutex_t g_mutexPtsRecvMsgHandle     = PTHREAD_MUTEX_INITIALIZER;        /* ���������Ϣ�� */
pthread_cond_t  g_condPtsRecvMsgHandle      = PTHREAD_COND_INITIALIZER;         /* ���������Ϣ���� */
pthread_mutex_t g_mutexSendMsgPthreadList   = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t g_mutexStreamAddrList       = PTHREAD_MUTEX_INITIALIZER;
PTS_SERV_SOCKET_ST g_lPtsServSocket[PTS_WEB_SERVER_MAX_SIZE];
S32 g_alUdpSocket[PTS_UDP_LISTEN_PORT_COUNT] = {0};

extern S32 pts_create_udp_socket(U16 usUdpPort, U32 ulSocketCache);
extern U32 dos_get_default_limitation(U32 ulModIndex, U32 * pulLimitation);
VOID pts_delete_send_stream_node(PT_MSG_TAG *pstMsgDes);
VOID pts_delete_recv_stream_node(PT_MSG_TAG *pstMsgDes);

S8 *pts_get_current_time()
{
    time_t ulCurrTime = 0;

    ulCurrTime = time(NULL);
    return ctime(&ulCurrTime);
}

S32 pts_get_sn_by_id(S8 *szID, S8 *szSN, S32 lLen)
{
    S8 achSql[PTS_SQL_STR_SIZE] = {0};
    PTS_SQLITE_GET_VALUE_PARAM_ST stParam;

    dos_memzero(szSN, lLen);
    sprintf(achSql, "select sn from ipcc_alias where id=%s and register = 1;", szID);
    stParam.szValue = szSN;
    stParam.ulLen = lLen;
    dos_sqlite3_exec_callback(g_pstMySqlite, achSql, pts_get_value_callback, (void *)&stParam);
    if (dos_strlen(szSN) > 0)
    {
        return DOS_SUCC;
    }
    else
    {
        return DOS_FAIL;
    }

}

S32 pts_printf_recv_cache_msg(U32 ulIndex, S32 argc, S8 **argv)
{
    S32             i           = 0;
    U32             ulLen       = 0;
    S8              szBuff[PT_DATA_BUFF_512]     = {0};
    list_t          *pstPtcNode     = NULL;
    list_t          *pstPtcHead     = NULL;
    list_t          *pstStreamHead  = NULL;
    list_t          *pstStreamNode  = NULL;
    PT_CC_CB_ST     *pstPtcCB       = NULL;
    PT_STREAM_CB_ST *pstStreamCB    = NULL;

    ulLen = snprintf(szBuff, sizeof(szBuff), "\r\n%-20s%-15s%-10s%-10s%-10s\r\n", "SN", "StreamID", "CurrSeq", "MaxSeq", "ConfirmSeq");
    cli_out_string(ulIndex, szBuff);

    pthread_mutex_lock(&g_mutexPtcRecvList);

    pstPtcHead = &g_stPtcListRecv;
    pstPtcNode = pstPtcHead;

    while (1)
    {
        pstPtcNode = dos_list_work(pstPtcHead, pstPtcNode);
        if (DOS_ADDR_INVALID(pstPtcNode))
        {
            break;
        }

        pstPtcCB = dos_list_entry(pstPtcNode, PT_CC_CB_ST, stCCListNode);
        snprintf(szBuff, sizeof(szBuff), "%.16s----------------------------\r\n", pstPtcCB->aucID);
        cli_out_string(ulIndex, szBuff);
        pthread_mutex_lock(&pstPtcCB->pthreadMutex);
        for (i=0; i<PT_DATA_BUTT; i++)
        {
            pstStreamHead = &pstPtcCB->astDataTypes[i].stStreamQueHead;
            pstStreamNode = pstStreamHead;
            while (1)
            {
                pstStreamNode = dos_list_work(pstStreamHead, pstStreamNode);
                if (DOS_ADDR_INVALID(pstStreamNode))
                {
                    break;
                }

                pstStreamCB = dos_list_entry(pstStreamNode, PT_STREAM_CB_ST, stStreamListNode);

                snprintf(szBuff, sizeof(szBuff), "%.16s%11d%10d%10d%10d\r\n", pstPtcCB->aucID, pstStreamCB->ulStreamID, pstStreamCB->lCurrSeq, pstStreamCB->lMaxSeq, pstStreamCB->lConfirmSeq);
                cli_out_string(ulIndex, szBuff);
            }
        }

        pthread_mutex_unlock(&pstPtcCB->pthreadMutex);

    }

    pthread_mutex_unlock(&g_mutexPtcRecvList);


    return 0;
}

S32 pts_printf_send_cache_msg(U32 ulIndex, S32 argc, S8 **argv)
{
    S32             i           = 0;
    U32             ulLen       = 0;
    S8              szBuff[PT_DATA_BUFF_512]     = {0};
    list_t          *pstPtcNode     = NULL;
    list_t          *pstPtcHead     = NULL;
    list_t          *pstStreamHead  = NULL;
    list_t          *pstStreamNode  = NULL;
    PT_CC_CB_ST     *pstPtcCB       = NULL;
    PT_STREAM_CB_ST *pstStreamCB    = NULL;

    ulLen = snprintf(szBuff, sizeof(szBuff), "\r\n%-20s%-15s%-10s%-10s%-10s\r\n", "SN", "StreamID", "CurrSeq", "MaxSeq", "ConfirmSeq");
    cli_out_string(ulIndex, szBuff);

    pthread_mutex_lock(&g_mutexPtcSendList);

    pstPtcHead = &g_stPtcListSend;
    pstPtcNode = pstPtcHead;

    while (1)
    {
        pstPtcNode = dos_list_work(pstPtcHead, pstPtcNode);
        if (DOS_ADDR_INVALID(pstPtcNode))
        {
            break;
        }

        pstPtcCB = dos_list_entry(pstPtcNode, PT_CC_CB_ST, stCCListNode);
        snprintf(szBuff, sizeof(szBuff), "%.16s----------------------------\r\n", pstPtcCB->aucID);
        cli_out_string(ulIndex, szBuff);
        pthread_mutex_lock(&pstPtcCB->pthreadMutex);
        for (i=0; i<PT_DATA_BUTT; i++)
        {
            pstStreamHead = &pstPtcCB->astDataTypes[i].stStreamQueHead;
            pstStreamNode = pstStreamHead;
            while (1)
            {
                pstStreamNode = dos_list_work(pstStreamHead, pstStreamNode);
                if (DOS_ADDR_INVALID(pstStreamNode))
                {
                    break;
                }

                pstStreamCB = dos_list_entry(pstStreamNode, PT_STREAM_CB_ST, stStreamListNode);

                snprintf(szBuff, sizeof(szBuff), "%.16s%11d%10d%10d%10d\r\n", pstPtcCB->aucID, pstStreamCB->ulStreamID, pstStreamCB->lCurrSeq, pstStreamCB->lMaxSeq, pstStreamCB->lConfirmSeq);
                cli_out_string(ulIndex, szBuff);
            }
        }

        pthread_mutex_unlock(&pstPtcCB->pthreadMutex);

    }

    pthread_mutex_unlock(&g_mutexPtcSendList);

    return 0;
}

S32 pts_trace_debug_ptc(U32 ulIndex, S32 argc, S8 **argv)
{
    list_t          *pstPtcNode     = NULL;
    list_t          *pstPtcHead     = NULL;
    PT_CC_CB_ST     *pstPtcCB       = NULL;
    BOOL            bIsOn           = DOS_FALSE;
    S8              szSN[PT_DATA_BUFF_64] = {0};
    S8              szErrorMsg[PT_DATA_BUFF_128] = {0};
    S32             lResult         = 0;

    if (argc != 3)
    {
        cli_out_string(ulIndex, "Usage : ptsd trace [ID] [on|off].\r\n");

        return 0;
    }

    if (dos_strcmp(argv[2], "off") == 0)
    {
        bIsOn = DOS_FALSE;
    }
    else
    {
        bIsOn = DOS_TRUE;
    }

    if (pts_is_int(argv[1]))
    {
        lResult = pts_get_sn_by_id(argv[1], szSN, PT_DATA_BUFF_64);
        if (lResult != DOS_SUCC)
        {
            snprintf(szErrorMsg, sizeof(szErrorMsg), "  Get sn fail by id(%s)\r\n", argv[1]);
            cli_out_string(ulIndex, szErrorMsg);

            return 0;
        }
    }
    else
    {
        dos_memcpy(szSN, argv[1], PTC_ID_LEN);
    }

    pthread_mutex_lock(&g_mutexPtcRecvList);

    pstPtcHead = &g_stPtcListRecv;
    pstPtcNode = pstPtcHead;

    while (1)
    {
        pstPtcNode = dos_list_work(pstPtcHead, pstPtcNode);
        if (DOS_ADDR_INVALID(pstPtcNode))
        {
            break;
        }

        pstPtcCB = dos_list_entry(pstPtcNode, PT_CC_CB_ST, stCCListNode);
        if (dos_memcmp(pstPtcCB->aucID, szSN, PTC_ID_LEN) == 0)
        {
            pstPtcCB->bIsTrace = bIsOn;
            break;
        }
    }
    pthread_mutex_unlock(&g_mutexPtcRecvList);

    pthread_mutex_lock(&g_mutexPtcSendList);
    pstPtcHead = &g_stPtcListRecv;
    pstPtcNode = pstPtcHead;

    while (1)
    {
        pstPtcNode = dos_list_work(pstPtcHead, pstPtcNode);
        if (DOS_ADDR_INVALID(pstPtcNode))
        {
            break;
        }

        pstPtcCB = dos_list_entry(pstPtcNode, PT_CC_CB_ST, stCCListNode);
        if (dos_memcmp(pstPtcCB->aucID, szSN, PTC_ID_LEN) == 0)
        {
            pstPtcCB->bIsTrace = bIsOn;
            break;
        }
    }
    pthread_mutex_unlock(&g_mutexPtcSendList);

    return 0;
}

/* ����streamID���ҵ�ַ */
S32 pts_find_stream_addr_by_streamID(VOID *pKey, DLL_NODE_S *pstDLLNode)
{
    STREAM_CACHE_ADDR_CB_ST *pstStreamCacheAddr;
    U32 ulStreamID;

    if (DOS_ADDR_INVALID(pKey)
        || DOS_ADDR_INVALID(pstDLLNode)
        || DOS_ADDR_INVALID(pstDLLNode->pHandle))
    {
        return DOS_FAIL;
    }

    ulStreamID = *(U32 *)pKey;
    pstStreamCacheAddr = pstDLLNode->pHandle;

    if (ulStreamID == pstStreamCacheAddr->ulStreamID)
    {
        return DOS_SUCC;
    }

    return DOS_FAIL;
}

#if 0
void pts_delete_stream_addr_node(U32 ulStreamID)
{
    DLL_NODE_S *pstListNode = NULL;

    pthread_mutex_lock(&g_mutexStreamAddrList);
    pstListNode = dll_find(&g_stStreamAddrList, (VOID *)&ulStreamID, pts_find_stream_addr_by_streamID);
    if (DOS_ADDR_VALID(pstListNode))
    {
        pt_stream_addr_delete(&g_stStreamAddrList, pstListNode);
    }
    pthread_mutex_unlock(&g_mutexStreamAddrList);
}
#endif

void pts_free_stream_resource(PT_MSG_TAG *pstMsgDes)
{
    DLL_NODE_S              *pstListNode        = NULL;
    STREAM_CACHE_ADDR_CB_ST *pstStreamCacheAddr = NULL;
    PT_STREAM_CB_ST         *pstStreamRecvNode  = NULL;
    PT_STREAM_CB_ST         *pstStreamSendNode  = NULL;
    PT_CC_CB_ST             *pstPtcRecvNode     = NULL;
    PT_CC_CB_ST             *pstPtcSendNode     = NULL;
    PT_SEND_MSG_PTHREAD     *pstSendPthread     = NULL;

    if (DOS_ADDR_INVALID(pstMsgDes))
    {
        return;
    }
    /* ɾ�� ����ptc��Ϣ��Ӧ��list */
    pts_set_delete_recv_buff_list_flag(pstMsgDes->ulStreamID);
    pt_logr_debug("free stream resource, streamID : %d", pstMsgDes->ulStreamID);
    pthread_mutex_lock(&g_mutexStreamAddrList);
    pstListNode = dll_find(&g_stStreamAddrList, (VOID *)&pstMsgDes->ulStreamID, pts_find_stream_addr_by_streamID);
    if (DOS_ADDR_INVALID(pstListNode))
    {
        pthread_mutex_unlock(&g_mutexStreamAddrList);
        pts_delete_send_stream_node(pstMsgDes);
        pts_delete_recv_stream_node(pstMsgDes);

        return;
    }
    pstStreamCacheAddr = pstListNode->pHandle;
    if (DOS_ADDR_INVALID(pstStreamCacheAddr))
    {
        pt_stream_addr_delete(&g_stStreamAddrList, pstListNode);
        pthread_mutex_unlock(&g_mutexStreamAddrList);
        pts_delete_send_stream_node(pstMsgDes);
        pts_delete_recv_stream_node(pstMsgDes);

        return;
    }

    pstPtcSendNode = pstStreamCacheAddr->pstPtcSendNode;
    pstStreamSendNode = pstStreamCacheAddr->pstStreamSendNode;
    if (DOS_ADDR_INVALID(pstStreamSendNode) || DOS_ADDR_INVALID(pstPtcSendNode))
    {
        pts_delete_send_stream_node(pstMsgDes);
    }
    else
    {
        pthread_mutex_lock(&pstPtcSendNode->pthreadMutex);
        pt_delete_stream_node(&pstStreamSendNode->stStreamListNode, pstMsgDes->enDataType);
        pthread_mutex_unlock(&pstPtcSendNode->pthreadMutex);
    }

    pstPtcRecvNode = pstStreamCacheAddr->pstPtcRecvNode;
    pstStreamRecvNode = pstStreamCacheAddr->pstStreamRecvNode;
    if (DOS_ADDR_INVALID(pstStreamRecvNode) || DOS_ADDR_INVALID(pstPtcRecvNode))
    {
        pts_delete_recv_stream_node(pstMsgDes);
    }
    else
    {
        pthread_mutex_lock(&pstPtcRecvNode->pthreadMutex);
        if (DOS_ADDR_VALID(pstStreamRecvNode->hTmrHandle))
        {
            dos_tmr_stop(&pstStreamRecvNode->hTmrHandle);
            pstStreamRecvNode->hTmrHandle = NULL;
        }
        if (pstMsgDes->enDataType == PT_DATA_WEB)
        {
            if (pstStreamRecvNode->bIsUsing)
            {
                pthread_mutex_lock(&g_mutexSendMsgPthreadList);
                pstSendPthread = pt_send_msg_pthread_search(&g_stSendMsgPthreadList, pstMsgDes->ulStreamID);
                if (DOS_ADDR_VALID(pstSendPthread))
                {
                    if (DOS_ADDR_VALID(pstSendPthread->pstPthreadParam))
                    {
                        pstSendPthread->pstPthreadParam->bIsNeedExit = DOS_TRUE;
                        sem_post(&pstSendPthread->pstPthreadParam->stSemSendMsg);
                    }
                }
                pthread_mutex_unlock(&g_mutexSendMsgPthreadList);
            }
        }

        pt_delete_stream_node(&pstStreamRecvNode->stStreamListNode, pstMsgDes->enDataType);
        pthread_mutex_unlock(&pstPtcRecvNode->pthreadMutex);
    }

    /* ɾ�����滺���ַ�Ľڵ� */
    pt_stream_addr_delete(&g_stStreamAddrList, pstListNode);
    pthread_mutex_unlock(&g_mutexStreamAddrList);
}

PTS_REV_MSG_STREAM_LIST_ST *pts_search_recv_buff_list(list_t *pstHead, U32 ulStreamID)
{
    list_t                      *pstNode            = NULL;
    PTS_REV_MSG_STREAM_LIST_ST  *pstStreamListNode  = NULL;

    if (DOS_ADDR_INVALID(pstHead))
    {
        return NULL;
    }

    if (dos_list_is_empty(pstHead))
    {
        return NULL;
    }

    pstNode = pstHead;

    while (1)
    {
        pstNode = dos_list_work(pstHead, pstNode);
        if (DOS_ADDR_INVALID(pstNode))
        {
            break;
        }

        pstStreamListNode = dos_list_entry(pstNode, PTS_REV_MSG_STREAM_LIST_ST, stSteamList);
        if (pstStreamListNode->ulStreamID == ulStreamID && pstStreamListNode->bIsValid)
        {
            return pstStreamListNode;
        }
    }

    return NULL;
}

PTS_REV_MSG_STREAM_LIST_ST *pts_create_recv_buff_list(list_t *pstHead, U32 ulStreamID)
{
    PTS_REV_MSG_STREAM_LIST_ST  *pstStreamListNode  = NULL;

    if (DOS_ADDR_INVALID(pstHead))
    {
        return NULL;
    }

    pstStreamListNode = (PTS_REV_MSG_STREAM_LIST_ST *)dos_dmem_alloc(sizeof(PTS_REV_MSG_STREAM_LIST_ST));
    if (DOS_ADDR_INVALID(pstStreamListNode))
    {
        /* malloc fail */

        return NULL;
    }

    pthread_mutex_init(&pstStreamListNode->pMutexPthread, NULL);
    pstStreamListNode->ulStreamID = ulStreamID;
    pstStreamListNode->bIsValid = DOS_TRUE;
    pstStreamListNode->ulLastUseTime = time(NULL);
    pstStreamListNode->lHandleMaxSeq = -1;
    dos_list_init(&pstStreamListNode->stBuffList);
    dos_list_node_init(&pstStreamListNode->stSteamList);

    dos_list_add_tail(pstHead, &pstStreamListNode->stSteamList);

    return pstStreamListNode;
}

void pts_delete_recv_buff_list(PTS_REV_MSG_STREAM_LIST_ST *pstStreamListNode)
{
    list_t *pstNode = NULL;
    PTS_REV_MSG_HANDLE_ST *pstRecvBuffNode = NULL;

    if (DOS_ADDR_INVALID(pstStreamListNode))
    {
        return;
    }

    pthread_mutex_lock(&pstStreamListNode->pMutexPthread);

    while (1)
    {
        if (dos_list_is_empty(&pstStreamListNode->stBuffList))
        {
            break;
        }

        pstNode = dos_list_fetch(&pstStreamListNode->stBuffList);
        if (DOS_ADDR_INVALID(pstNode))
        {
            break;
        }

        pstRecvBuffNode = dos_list_entry(pstNode, PTS_REV_MSG_HANDLE_ST, stList);

        if (DOS_ADDR_VALID(pstRecvBuffNode->paRecvBuff))
        {
            dos_dmem_free(pstRecvBuffNode->paRecvBuff);
            pstRecvBuffNode->paRecvBuff = NULL;
        }
        dos_dmem_free(pstRecvBuffNode);
        pstRecvBuffNode = NULL;
    }

    pthread_mutex_unlock(&pstStreamListNode->pMutexPthread);
    pthread_mutex_destroy(&pstStreamListNode->pMutexPthread);
    dos_dmem_free(pstStreamListNode);
    pstStreamListNode = NULL;
}

void pts_set_delete_recv_buff_list_flag(U32 ulStreamID)
{
    PTS_REV_MSG_STREAM_LIST_ST *pstStreamListNode = NULL;

    pthread_mutex_lock(&g_mutexPtsRecvMsgHandle);
    pstStreamListNode = pts_search_recv_buff_list(&g_stMsgRecvFromPtc, ulStreamID);
    if (DOS_ADDR_INVALID(pstStreamListNode))
    {
        pthread_mutex_unlock(&g_mutexPtsRecvMsgHandle);

        return;
    }
    pthread_mutex_unlock(&g_mutexPtsRecvMsgHandle);

    pthread_mutex_lock(&pstStreamListNode->pMutexPthread);
    pstStreamListNode->bIsValid = DOS_FALSE;
    pthread_mutex_unlock(&pstStreamListNode->pMutexPthread);
}


U32 pts_recvfrom_ptc_buff_list_insert(PTS_REV_MSG_STREAM_LIST_ST *pstStreamListNode, U8 *szBUff, U32 ulBuffLen, U32 ulIndex, struct sockaddr_in stClientAddr, S32 lSeq)
{
    PTS_REV_MSG_HANDLE_ST   *pstNewNode     = NULL;

    if (DOS_ADDR_INVALID(szBUff) || DOS_ADDR_INVALID(pstStreamListNode))
    {
        return DOS_FAIL;
    }

    pstNewNode = (PTS_REV_MSG_HANDLE_ST *)dos_dmem_alloc(sizeof(PTS_REV_MSG_HANDLE_ST));
    if (DOS_ADDR_INVALID(pstNewNode))
    {
        perror("malloc");
        return DOS_FAIL;
    }

    pstNewNode->paRecvBuff = szBUff;
    pstNewNode->ulRecvLen = ulBuffLen;
    pstNewNode->ulIndex = ulIndex;
    pstNewNode->lSeq = lSeq;
    pstNewNode->stClientAddr = stClientAddr;
    dos_list_node_init(&(pstNewNode->stList));

    if (dos_list_is_empty(&pstStreamListNode->stBuffList))
    {
        dos_list_add_tail(&pstStreamListNode->stBuffList, &(pstNewNode->stList));

        return DOS_SUCC;
    }

    /* �Ƚϴ������������ź͵�ǰ����ŵĴ�С */
    if (pstStreamListNode->lHandleMaxSeq >= lSeq)
    {
        dos_list_add_head(&pstStreamListNode->stBuffList, &(pstNewNode->stList));
    }
    else
    {
        dos_list_add_tail(&pstStreamListNode->stBuffList, &(pstNewNode->stList));
    }


    return DOS_SUCC;
}

/**
 * ������S32 pts_find_ptc_callback(VOID *para, S32 n_column, S8 **column_value, S8 **column_name)
 * ���ܣ�
 *      1.���ݿ��ѯ�ص�����
 * ����
 * ����ֵ��
 */
S32 pts_find_ptc_callback(VOID *para, S32 n_column, S8 **column_value, S8 **column_name)
{
    S8 *pcDestID = (S8 *)para;
    dos_strncpy(pcDestID, column_value[0], PTC_ID_LEN);
    pcDestID[PTC_ID_LEN] = '\0';

    return 0;
}

/**
 * ������S32 pts_find_ptc_by_dest_addr(S8 *pDestInternetIp, S8 *pDestIntranetIp, S8 *pcDestSN)
 * ���ܣ�
 *      1.���ݹ���˽��ip��ַ�����Ҷ�Ӧ��ptc
 * ����
 *      S8 *pDestInternetIp : ����IP��ַ
 *      S8 *pDestIntranetIp : ˽��ip��ַ
 *      S8 *pcDestSN  : ������Ų��ҵ���ptc��sn
 * ����ֵ��
 */
S32 pts_find_ptc_by_dest_addr(S8 *pDestInternetIp, S8 *pDestIntranetIp, S8 *pcDestSN)
{
    S32 lRet = 0;
    S8 szSql[PT_DATA_BUFF_128] = {0};

    if (NULL == pDestInternetIp || NULL == pDestIntranetIp)
    {
        return DOS_FAIL;
    }

    dos_snprintf(szSql, PT_DATA_BUFF_128, "select sn from ipcc_alias where register = 1 and internetIP='%s' and intranetIP='%s'", pDestInternetIp, pDestIntranetIp);
    lRet = dos_sqlite3_exec_callback(g_pstMySqlite, szSql, pts_find_ptc_callback, (VOID *)pcDestSN);
    if (lRet != DOS_SUCC)
    {
        return DOS_FAIL;
    }

    if (dos_strlen(pcDestSN) > 0)
    {
        return DOS_SUCC;
    }
    else
    {
        dos_snprintf(szSql, PT_DATA_BUFF_128, "select sn from ipcc_alias where register = 1 and internetIP='%s'", pDestInternetIp);
        dos_sqlite3_exec_callback(g_pstMySqlite, szSql, pts_find_ptc_callback, (VOID *)pcDestSN);
        if (dos_strlen(pcDestSN) > 0)
        {
            return DOS_SUCC;
        }
    }
    return DOS_FAIL;
}

/**
 * ������U32 pts_get_ptc_cnt(U32 *pulCnt)
 * ���ܣ�
 *      ��ȡ���ߵ�PTC�ĸ���
 * ����
 *      U32 *pulCnt�� ������������PTC����
 * ����ֵ��
 *      ����ɹ�����DOS_SUCC,ʧ�ܷ���DOS_FAIL
 */
U32 pts_get_ptc_cnt(U32 *pulCnt)
{
    S32 lRet = 0;
    S8 szSql[PT_DATA_BUFF_128] = {0};

    if (DOS_ADDR_INVALID(pulCnt))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    dos_snprintf(szSql, PTS_SQL_STR_SIZE, "select * from ipcc_alias where register=1");
    lRet = dos_sqlite3_record_count(g_pstMySqlite, szSql);
    if (lRet < 0)
    {
        DOS_ASSERT(0);

        pulCnt = 0;
        return DOS_FAIL;
    }

    *pulCnt = (U32)lRet;
    return DOS_SUCC;
}

/**
 * ������VOID pts_data_lose(PT_MSG_TAG *pstMsgDes, S32 lShouldSeq)
 * ���ܣ�
 *      1.���Ͷ�������
 * ����
 *      PT_MSG_TAG *pstMsgDes ��������ʧ����������Ϣ
 * ����ֵ����
 */
VOID pts_data_lose(PT_MSG_TAG *pstMsgDes, S32 lLoseSeq)
{
    if (DOS_ADDR_INVALID(pstMsgDes))
    {
        return;
    }

    BOOL bIsResend = DOS_FALSE;
    PT_CMD_EN enCmdValue = PT_CMD_RESEND;
    pstMsgDes->lSeq = lLoseSeq;

    pt_logr_info("send lose data : stream = %d, seq = %d", pstMsgDes->ulStreamID, lLoseSeq);
    pthread_mutex_lock(&g_mutexPtsSendPthread);
    pt_need_send_node_list_insert(&g_stPtsNendSendNode, pstMsgDes->aucID, pstMsgDes, enCmdValue, bIsResend);
    pthread_cond_signal(&g_condPtsSend);
    pthread_mutex_unlock(&g_mutexPtsSendPthread);
}

/**
 * ������VOID pts_send_lost_data_req(U64 ulLoseMsg)
 * ���ܣ�
 *      1.�ش���ʧ�İ�
 * ����
 *      U64 ulLoseMsg ��
 * ����ֵ����
 */
VOID pts_send_lost_data_req(U64 ulLoseMsg)
{
    if (0 == ulLoseMsg)
    {
        return;
    }

    PT_LOSE_BAG_MSG_ST  *pstLoseMsg     = (PT_LOSE_BAG_MSG_ST *)ulLoseMsg;
    PT_STREAM_CB_ST     *pstStreamNode  = pstLoseMsg->pstStreamNode;
    PT_CC_CB_ST         *pstPtcSendNode = NULL;
    pthread_mutex_t     *pPthreadMutex  = pstLoseMsg->pPthreadMutex;
    S32                 i               = 0;
    U32                 ulCount         = 0;
    U32                 ulArraySub      = 0;
    S32                 lCurrSeq        = 0;
    S32                 lMaxSeq         = 0;
    PT_MSG_TAG          stUdpMsg;

    if (DOS_ADDR_INVALID(pstStreamNode) || DOS_ADDR_INVALID(pPthreadMutex))
    {
        return;
    }

    pthread_mutex_lock(pPthreadMutex);

    pt_logr_info("resend stream : %d, count : %d", pstLoseMsg->stMsg.ulStreamID, pstStreamNode->ulCountResend);
    if (pstStreamNode->ulCountResend >= 3)
    {
        /* 3���δ�յ������رն�ʱ����sockfd */
        pt_logr_error("stream resend fail, close��stream is %d", pstStreamNode->ulStreamID);
        dos_tmr_stop(&pstStreamNode->hTmrHandle);
        pstStreamNode->hTmrHandle= NULL;
        pthread_mutex_unlock(pPthreadMutex);

        stUdpMsg = pstLoseMsg->stMsg;
        pthread_mutex_lock(&g_mutexPtcSendList);
        pstPtcSendNode = pt_ptc_list_search(&g_stPtcListSend, stUdpMsg.aucID);
        if (DOS_ADDR_VALID(pstPtcSendNode))
        {
            pts_send_exit_notify_to_ptc(&pstLoseMsg->stMsg, pstPtcSendNode);
        }
        pthread_mutex_unlock(&g_mutexPtcSendList);

        pts_free_stream_resource(&stUdpMsg);

        return;
    }
    else
    {
        pstStreamNode->ulCountResend++;
        /* �������ҳ���ʧ�İ��������ش���Ϣ */
        lCurrSeq = pstStreamNode->lCurrSeq;
        lMaxSeq = pstStreamNode->lMaxSeq;

        for (i=lCurrSeq+1; i<lMaxSeq; i++)
        {
            ulArraySub = i & (PT_DATA_RECV_CACHE_SIZE - 1);
            if (0 == i)
            {
                /* �ж�0�Ű��Ƿ�ʧ */
                if (pstStreamNode->unDataQueHead.pstDataTcp[ulArraySub].ulLen == 0)
                {
                    ulCount++;
                    pts_data_lose(&pstLoseMsg->stMsg, i);
                }
            }
            else if (pstStreamNode->unDataQueHead.pstDataTcp[ulArraySub].lSeq != i)
            {
                ulCount++;
                /* ���Ͷ����ط����� */
                pts_data_lose(&pstLoseMsg->stMsg, i);
            }
        }
    }
    pt_logr_debug("send lose data count : %d", ulCount);

    if (0 == ulCount)
    {
        if (DOS_ADDR_VALID(pstStreamNode->hTmrHandle))
        {
            dos_tmr_stop(&pstStreamNode->hTmrHandle);
            pstStreamNode->hTmrHandle = NULL;
        }
    }

    pthread_mutex_unlock(pPthreadMutex);
}

/**
 * ������VOID pts_send_confirm_msg(PT_MSG_TAG *pstMsgDes, U32 lConfirmSeq)
 * ���ܣ�
 *      1.����ȷ�Ͻ�����Ϣ
 * ����
 *
 * ����ֵ����
 */
VOID pts_send_confirm_msg(PT_MSG_TAG *pstMsgDes, U32 lConfirmSeq)
{
    S32 i = 0;
    BOOL bIsResend = DOS_FALSE;
    PT_CMD_EN enCmdValue = PT_CMD_CONFIRM;

    if (DOS_ADDR_INVALID(pstMsgDes))
    {
        return;
    }

    pstMsgDes->lSeq = lConfirmSeq;

    pt_logr_debug("send confirm data, type = %d, stream = %d", pstMsgDes->enDataType, pstMsgDes->ulStreamID);
    pthread_mutex_lock(&g_mutexPtsSendPthread);
    for (i=0; i<PTS_SEND_CONFIRM_MSG_COUNT; i++)
    {
        pt_need_send_node_list_insert(&g_stPtsNendSendNode, pstMsgDes->aucID, pstMsgDes, enCmdValue, bIsResend);
        usleep(10);
    }
    pthread_cond_signal(&g_condPtsSend);
    pthread_mutex_unlock(&g_mutexPtsSendPthread);

}

/**
 * ������VOID pts_send_login_verify(PT_MSG_TAG *pstMsgDes)
 * ���ܣ�
 *      1.���͵�½��֤��Ϣ
 * ����
 * ����ֵ����
 */
VOID pts_send_login_verify(PT_MSG_TAG *pstMsgDes)
{
    PT_MSG_TAG stMsgDes;

    stMsgDes.enDataType = PT_DATA_CTRL;
    stMsgDes.ulStreamID = PT_CTRL_LOGIN_RSP;
    stMsgDes.lSeq = 0;

    BOOL bIsResend = DOS_FALSE;
    PT_CMD_EN enCmdValue = PT_CMD_NORMAL;
    pthread_mutex_lock(&g_mutexPtsSendPthread);
    pt_need_send_node_list_insert(&g_stPtsNendSendNode, pstMsgDes->aucID, &stMsgDes, enCmdValue, bIsResend);
    pthread_cond_signal(&g_condPtsSend);
    pthread_mutex_unlock(&g_mutexPtsSendPthread);

}

/**
 * ������VOID pts_create_rand_str(S8 *szVerifyStr)
 * ���ܣ�
 *      1.��������ַ���
 * ����
 * ����ֵ����
 */
VOID pts_create_rand_str(S8 *szVerifyStr)
{
    U8 i = 0;
    if (NULL == szVerifyStr)
    {
        return;
    }

    srand((int)time(0));
    for(i=0; i<PT_LOGIN_VERIFY_SIZE-1; i++)
    {
        szVerifyStr[i] = 1 + (rand()&CHAR_MAX);
    }
    szVerifyStr[PT_LOGIN_VERIFY_SIZE-1] = '\0';
}

/**
 * ������S32 pts_key_convert(S8 *szKey, S8 *szDest, S8 *szVersion)
 * ���ܣ�
 *      1.��½��֤
 * ����
 *      S8 *szKey     ��ԭʼֵ
 *      S8 *szDest    ��ת�����ֵ
 *      S8 *szVersion ��ptc�汾��
 * ����ֵ����
 */
S32 pts_key_convert(S8 *szKey, S8 *szDest, S8 *szPtcVersion)
{
    if (NULL == szKey || NULL == szDest)
    {
        return DOS_FAIL;
    }

    S32 i = 0;

    //if (dos_strncmp(szPtcVersion, "1.1", dos_strlen("1.1")) == 0)
    //{
    /* TODO 1.1�汾��֤���� */
    for (i=0; i<PT_LOGIN_VERIFY_SIZE-1; i++)
    {
        szDest[i] = szKey[i]&0xA9;
    }
    szDest[PT_LOGIN_VERIFY_SIZE] = '\0';
    //}

    return DOS_SUCC;
}

/**
 * ������S32 pts_get_key(PT_MSG_TAG *pstMsgDes, S8 *szKey, U32 ulLoginVerSeq)
 * ���ܣ�
 *      1.
 * ����
 * ����ֵ����
 */
S32 pts_get_key(PT_MSG_TAG *pstMsgDes, S8 *szKey, U32 ulLoginVerSeq)
{
    if (NULL == pstMsgDes || NULL == szKey)
    {
        pt_logr_debug("pts_get_key : arg error");
        return DOS_FAIL;
    }

    list_t  *pstStreamHead          = NULL;
    PT_STREAM_CB_ST *pstStreamNode  = NULL;
    PT_CC_CB_ST     *pstCCNode      = NULL;
    PT_CTRL_DATA_ST *pstCtrlData    = NULL;
    PT_DATA_QUE_HEAD_UN  punDataList;
    U32 ulArrarySub = 0;

    pthread_mutex_lock(&g_mutexPtcSendList);
    pstCCNode = pt_ptc_list_search(&g_stPtcListSend, pstMsgDes->aucID);
    if(NULL == pstCCNode)
    {
        pt_logr_debug("pts_get_key : not found ptc");
        pthread_mutex_unlock(&g_mutexPtcSendList);

        return DOS_FAIL;
    }

    pstStreamHead = &pstCCNode->astDataTypes[pstMsgDes->enDataType].stStreamQueHead;
    pstStreamNode = pt_stream_queue_search(pstStreamHead, PT_CTRL_LOGIN_RSP);
    if (NULL == pstStreamNode)
    {
        pt_logr_debug("pts_get_key : not found stream node");
        pthread_mutex_unlock(&g_mutexPtcSendList);

        return DOS_FAIL;
    }

    punDataList = pstStreamNode->unDataQueHead;
    if (NULL == punDataList.pstDataTcp)
    {
        pt_logr_debug("pts_get_key : data queue is NULL");
    }
    else
    {
        ulArrarySub = (ulLoginVerSeq) & (PT_DATA_SEND_CACHE_SIZE - 1);
        if (punDataList.pstDataTcp[ulArrarySub].lSeq == ulLoginVerSeq)
        {
            pstCtrlData = (PT_CTRL_DATA_ST *)punDataList.pstDataTcp[ulArrarySub].szBuff;
            dos_memcpy(szKey, pstCtrlData->szLoginVerify, PT_LOGIN_VERIFY_SIZE-1);
            szKey[PT_LOGIN_VERIFY_SIZE-1] = '\0';
            pthread_mutex_unlock(&g_mutexPtcSendList);

            return DOS_SUCC;
        }
    }

    pthread_mutex_unlock(&g_mutexPtcSendList);

    return DOS_FAIL;
}

/**
 * ������S32 pts_save_into_send_cache(PT_CC_CB_ST *pstPtcNode, PT_MSG_TAG *pstMsgDes, S8 *acSendBuf, S32 lDataLen)
 *      1.��ӵ����ͻ���
 * ����
 *      PT_CC_CB_ST *pstPtcNode : ptc�����ַ
 *      PT_MSG_TAG *pstMsgDes   ��ͷ����Ϣ
 *      S8 *acSendBuf           ��������
 *      S32 lDataLen            �������ݳ���
 * ����ֵ��DOS_SUCC ֪ͨproxy������Ϣ
 *         DOS_FAIL ʧ�ܣ����߶���
 */

S32 pts_save_ctrl_msg_into_send_cache(U8 *pcIpccId, U32 ulStreamID, PT_DATA_TYPE_EN enDataType, S8 *acSendBuf, S32 lDataLen, S8 *szDestIp, U16 usDestPort)
{
    if (DOS_ADDR_INVALID(pcIpccId) || DOS_ADDR_INVALID(acSendBuf))
    {
        return PT_SAVE_DATA_FAIL;
    }

    S32                 lResult             = 0;
    list_t              *pstStreamListHead  = NULL;
    PT_STREAM_CB_ST     *pstStreamNode      = NULL;
    PT_DATA_TCP_ST      *pstDataQueue       = NULL;
    PT_CC_CB_ST         *pstPtcNode         = NULL;

    pthread_mutex_lock(&g_mutexPtcSendList);
    pstPtcNode = pt_ptc_list_search(&g_stPtcListSend, pcIpccId);
    if(DOS_ADDR_INVALID(pstPtcNode))
    {
        pt_logr_debug("pts_send_msg : not found PTC");
        pthread_mutex_unlock(&g_mutexPtcSendList);

        return PT_SAVE_DATA_FAIL;
    }
    pthread_mutex_lock(&pstPtcNode->pthreadMutex);
    pthread_mutex_unlock(&g_mutexPtcSendList);

    pstStreamListHead = &pstPtcNode->astDataTypes[enDataType].stStreamQueHead;
    pstStreamNode = pt_stream_queue_search(pstStreamListHead, ulStreamID);
    if (DOS_ADDR_INVALID(pstStreamNode))
    {
        /* ����stream node */
        pstStreamNode = pt_stream_node_create(ulStreamID);
        if (DOS_ADDR_INVALID(pstStreamNode))
        {
            /* stream node����ʧ�� */
            pt_logr_info("pts_save_into_send_cache : create stream node fail");
            pthread_mutex_unlock(&pstPtcNode->pthreadMutex);

            return PT_SAVE_DATA_FAIL;
        }
        if (DOS_ADDR_VALID(szDestIp))
        {
            pstStreamNode->usServPort = usDestPort;
            inet_pton(AF_INET, szDestIp, (VOID *)(pstStreamNode->aulServIp));
        }
        /* ���� stream list�� */
        pt_stream_queue_insert(pstStreamListHead, &(pstStreamNode->stStreamListNode));
    }


    pstDataQueue = pstStreamNode->unDataQueHead.pstDataTcp;
    if (DOS_ADDR_INVALID(pstDataQueue))
    {
        /* ����tcp data queue */
        pstDataQueue = pt_data_tcp_queue_create(PT_DATA_RECV_CACHE_SIZE);
        if (DOS_ADDR_INVALID(pstDataQueue))
        {
            /* ����data queueʧ��*/
            pt_logr_info("pts_save_into_send_cache : create data queue fail");
            pthread_mutex_unlock(&pstPtcNode->pthreadMutex);

            return PT_SAVE_DATA_FAIL;
        }

        pstStreamNode->unDataQueHead.pstDataTcp = pstDataQueue;
    }

    /*�����ݲ��뵽data queue��*/
    lResult = pt_send_data_tcp_queue_insert(pstStreamNode, acSendBuf, lDataLen, PT_DATA_SEND_CACHE_SIZE, pstPtcNode->bIsTrace);
    if (lResult < 0)
    {
        pt_logr_info("pts_save_into_send_cache : add data into send cache fail");
        pts_trace(pstPtcNode->bIsTrace, LOG_LEVEL_DEBUG, "pts_save_into_send_cache : add data into send cache fail");
        pthread_mutex_unlock(&pstPtcNode->pthreadMutex);

        return PT_SAVE_DATA_FAIL;
    }
    pthread_mutex_unlock(&pstPtcNode->pthreadMutex);

    return lResult;
}

S32 pts_save_msg_into_send_cache(U8 *pcIpccId, U32 ulStreamID, PT_DATA_TYPE_EN enDataType, S8 *acSendBuf, S32 lDataLen, S8 *szDestIp, U16 usDestPort)
{
    STREAM_CACHE_ADDR_CB_ST *pstStreamCacheAddr = NULL;
    PT_CC_CB_ST             *pstPtcNode         = NULL;
    DLL_NODE_S              *pstListNode        = NULL;
    list_t                  *pstStreamListHead  = NULL;
    PT_STREAM_CB_ST         *pstStreamNode      = NULL;
    PT_DATA_TCP_ST          *pstDataQueue       = NULL;
    S32                     lResult             = 0;

    pthread_mutex_lock(&g_mutexStreamAddrList);

    pstListNode = dll_find(&g_stStreamAddrList, (VOID *)&ulStreamID, pts_find_stream_addr_by_streamID);
    if (DOS_ADDR_INVALID(pstListNode))
    {
        /* ���� STREAM_CACHE_ADDR_CB_ST */
        pstListNode = (HASH_NODE_S *)dos_dmem_alloc(sizeof(HASH_NODE_S));
        if (DOS_ADDR_INVALID(pstListNode))
        {
            DOS_ASSERT(0);
            pt_logr_info("malloc fail");
            pthread_mutex_unlock(&g_mutexStreamAddrList);

            return PT_SAVE_DATA_FAIL;
        }
        DLL_Init_Node(pstListNode);

        pstStreamCacheAddr = pt_stream_addr_create(ulStreamID);
        if (DOS_ADDR_INVALID(pstStreamCacheAddr))
        {
            dos_dmem_free(pstListNode);
            pstListNode = NULL;

            DOS_ASSERT(0);
            pt_logr_info("malloc fail");
            pthread_mutex_unlock(&g_mutexStreamAddrList);

            return PT_SAVE_DATA_FAIL;
        }
        pstListNode->pHandle = pstStreamCacheAddr;

        DLL_Add(&g_stStreamAddrList, pstListNode);
    }
    else
    {
        pstStreamCacheAddr = pstListNode->pHandle;
    }

    pstPtcNode = pstStreamCacheAddr->pstPtcSendNode;
    if (DOS_ADDR_INVALID(pstPtcNode))
    {
        /* ����ptc sn����ptc */
        pthread_mutex_lock(&g_mutexPtcSendList);

        pstPtcNode = pt_ptc_list_search(&g_stPtcListSend, pcIpccId);
        if(DOS_ADDR_INVALID(pstPtcNode))
        {
            pt_logr_debug("pts_send_msg : not found PTC");
            pthread_mutex_unlock(&g_mutexPtcSendList);
            pt_stream_addr_delete(&g_stStreamAddrList, pstListNode);
            pthread_mutex_unlock(&g_mutexStreamAddrList);

            return PT_SAVE_DATA_FAIL;
        }
        pstStreamCacheAddr->pstPtcSendNode = pstPtcNode;

        pthread_mutex_unlock(&g_mutexPtcSendList);
    }

    pthread_mutex_lock(&pstPtcNode->pthreadMutex);

    pstStreamNode = pstStreamCacheAddr->pstStreamSendNode;
    if (DOS_ADDR_INVALID(pstStreamNode) || pstStreamNode->ulStreamID != ulStreamID)
    {
        pstStreamCacheAddr->pstStreamRecvNode = NULL;
        /* ����stream */
        pstStreamListHead = &pstPtcNode->astDataTypes[enDataType].stStreamQueHead;
        pstStreamNode = pt_stream_queue_search(pstStreamListHead, ulStreamID);
        if (DOS_ADDR_INVALID(pstStreamNode))
        {
            /* ����stream node */
            pstStreamNode = pt_stream_node_create(ulStreamID);
            if (DOS_ADDR_INVALID(pstStreamNode))
            {
                /* stream node����ʧ�� */
                pt_logr_info("pts_save_into_send_cache : create stream node fail");
                pthread_mutex_unlock(&pstPtcNode->pthreadMutex);
                pthread_mutex_unlock(&g_mutexStreamAddrList);

                return PT_SAVE_DATA_FAIL;
            }

            if (DOS_ADDR_VALID(szDestIp))
            {
                pstStreamNode->usServPort = usDestPort;
                inet_pton(AF_INET, szDestIp, (VOID *)(pstStreamNode->aulServIp));
            }
            /* ���� stream list�� */
            pt_stream_queue_insert(pstStreamListHead, &(pstStreamNode->stStreamListNode));

            pstStreamCacheAddr->pstStreamSendNode= pstStreamNode;
        }
    }

    pthread_mutex_unlock(&g_mutexStreamAddrList);

    pstDataQueue = pstStreamNode->unDataQueHead.pstDataTcp;
    if (DOS_ADDR_INVALID(pstDataQueue))
    {
        /* ����tcp data queue */
        pstDataQueue = pt_data_tcp_queue_create(PT_DATA_RECV_CACHE_SIZE);
        if (DOS_ADDR_INVALID(pstDataQueue))
        {
            /* ����data queueʧ��*/
            pt_logr_info("pts_save_into_send_cache : create data queue fail");
            pthread_mutex_unlock(&pstPtcNode->pthreadMutex);

            return PT_SAVE_DATA_FAIL;
        }

        pstStreamNode->unDataQueHead.pstDataTcp = pstDataQueue;
    }

    /*�����ݲ��뵽data queue��*/
    lResult = pt_send_data_tcp_queue_insert(pstStreamNode, acSendBuf, lDataLen, PT_DATA_SEND_CACHE_SIZE, pstPtcNode->bIsTrace);
    if (lResult < 0)
    {
        pt_logr_info("pts_save_into_send_cache : add data into send cache fail");
        pthread_mutex_unlock(&pstPtcNode->pthreadMutex);

        return PT_SAVE_DATA_FAIL;
    }

    pthread_mutex_unlock(&pstPtcNode->pthreadMutex);

    return lResult;
}


/**
 * ������S32 pts_save_into_recv_cache(PT_CC_CB_ST *pstPtcNode, PT_MSG_TAG *pstMsgDes, S8 *acRecvBuf, S32 lDataLen)
 *      1.��ӵ����ͻ���
 * ����
 *      PT_CC_CB_ST *pstPtcNode : ptc�����ַ
 *      PT_MSG_TAG *pstMsgDes   ��ͷ����Ϣ
 *      S8 *acRecvBuf           ��������
 *      S32 lDataLen            �������ݳ���
 * ����ֵ��DOS_SUCC ֪ͨproxy������Ϣ
 *         DOS_FAIL ʧ�ܣ����߶���
 */
S32 pts_save_into_recv_cache(PT_MSG_TAG *pstMsgDes, S8 *acRecvBuf, S32 lDataLen)
{
    S32                     i                   = 0;
    S32                     lResult             = 0;
    U32                     ulNextSendArraySub  = 0;
    U32                     ulArraySub          = 0;
    list_t                  *pstStreamListHead  = NULL;
    PT_LOSE_BAG_MSG_ST      *pstLoseMsg         = NULL;
    STREAM_CACHE_ADDR_CB_ST *pstStreamCacheAddr = NULL;
    PT_CC_CB_ST             *pstPtcNode         = NULL;
    DLL_NODE_S              *pstListNode        = NULL;
    PT_STREAM_CB_ST         *pstStreamNode      = NULL;
    PT_DATA_TCP_ST          *pstDataQueue       = NULL;
    PT_SAVE_RESULT_EN       enReturn            = PT_SAVE_DATA_FAIL;

    if (DOS_ADDR_INVALID(pstMsgDes) || DOS_ADDR_INVALID(acRecvBuf))
    {
        return PT_SAVE_DATA_FAIL;
    }

    /* �ж�stream�Ƿ��ڷ��Ͷ����д��ڣ��������ڣ�˵�����stream�Ѿ����� */
    pthread_mutex_lock(&g_mutexStreamAddrList);
    pstListNode = dll_find(&g_stStreamAddrList, (VOID *)&pstMsgDes->ulStreamID, pts_find_stream_addr_by_streamID);
    if (DOS_ADDR_INVALID(pstListNode))
    {
        pthread_mutex_unlock(&g_mutexStreamAddrList);

        return PT_SAVE_DATA_FAIL;
    }

    pstStreamCacheAddr = pstListNode->pHandle;
    if (DOS_ADDR_INVALID(pstStreamCacheAddr))
    {
        pthread_mutex_unlock(&g_mutexStreamAddrList);

        return PT_SAVE_DATA_FAIL;
    }

    pstPtcNode = pstStreamCacheAddr->pstPtcRecvNode;
    if (DOS_ADDR_INVALID(pstPtcNode))
    {
        pthread_mutex_lock(&g_mutexPtcRecvList);
        pstPtcNode = pt_ptc_list_search(&g_stPtcListRecv, pstMsgDes->aucID);
        if(DOS_ADDR_INVALID(pstPtcNode))
        {
            pt_logr_info("pts_recv_msg_from_ptc : not found ipcc");
            pthread_mutex_unlock(&g_mutexPtcRecvList);
            pthread_mutex_unlock(&g_mutexStreamAddrList);
            return PT_SAVE_DATA_FAIL;
        }
        pstStreamCacheAddr->pstPtcRecvNode = pstPtcNode;

        pthread_mutex_unlock(&g_mutexPtcRecvList);
    }

    pthread_mutex_lock(&pstPtcNode->pthreadMutex);
    pstStreamNode = pstStreamCacheAddr->pstStreamRecvNode;
    if (DOS_ADDR_INVALID(pstStreamNode) || pstStreamNode->ulStreamID != pstMsgDes->ulStreamID)
    {
        pstStreamCacheAddr->pstStreamRecvNode = NULL;
        pstStreamListHead = &pstPtcNode->astDataTypes[pstMsgDes->enDataType].stStreamQueHead;
        pstStreamNode = pt_stream_queue_search(pstStreamListHead, pstMsgDes->ulStreamID);
        if (DOS_ADDR_INVALID(pstStreamNode))
        {
            /* ����stream node */
            pstStreamNode = pt_stream_node_create(pstMsgDes->ulStreamID);
            if (DOS_ADDR_INVALID(pstStreamNode))
            {
                /* stream node����ʧ�� */
                pt_logr_info("pts_save_into_send_cache : create stream node fail");
                pthread_mutex_unlock(&pstPtcNode->pthreadMutex);
                pthread_mutex_unlock(&g_mutexStreamAddrList);

                return PT_SAVE_DATA_FAIL;
            }
            /* ���� stream list�� */
            pt_stream_queue_insert(pstStreamListHead, &(pstStreamNode->stStreamListNode));

            pstStreamCacheAddr->pstStreamRecvNode = pstStreamNode;
        }
    }

    pthread_mutex_unlock(&g_mutexStreamAddrList);

    pstDataQueue = pstStreamNode->unDataQueHead.pstDataTcp;
    if (DOS_ADDR_INVALID(pstDataQueue))
    {
        /* ����tcp data queue */
        pstDataQueue = pt_data_tcp_queue_create(PT_DATA_RECV_CACHE_SIZE);
        if (NULL == pstDataQueue)
        {
            /* data queueʧ�� */
            pt_logr_info("pts_save_into_recv_cache : create tcp data queue fail");
            pthread_mutex_unlock(&pstPtcNode->pthreadMutex);

            return PT_SAVE_DATA_FAIL;
        }

        pstStreamNode->unDataQueHead.pstDataTcp = pstDataQueue;
    }

    /* ͳ�ư������� */
    if (pstMsgDes->lSeq > pstStreamNode->lMaxSeq)
    {
        pstPtcNode->ulUdpRecvDataCount++;
        pstPtcNode->ulUdpLostDataCount += (pstMsgDes->lSeq - pstStreamNode->lMaxSeq - 1);
    }

    /* �����ݲ��뵽data queue�� */
    pts_trace(pstPtcNode->bIsTrace, LOG_LEVEL_DEBUG, "deal with, stream : %d, seq : %d, len : %d", pstMsgDes->ulStreamID, pstMsgDes->lSeq, lDataLen);
    lResult = pt_recv_data_tcp_queue_insert(pstStreamNode, pstMsgDes, acRecvBuf, lDataLen, PT_DATA_RECV_CACHE_SIZE);
    if (lResult < 0)
    {
        pts_trace(pstPtcNode->bIsTrace, LOG_LEVEL_DEBUG, "pts_save_into_recv_cache : add data into recv cache fail");
        pthread_mutex_unlock(&pstPtcNode->pthreadMutex);

        return PT_SAVE_DATA_FAIL;
    }
    else if (PT_NEED_CUT_PTHREAD == lResult)
    {
        /* û�д滺�棬����Ҫ�ж��� */
        pts_trace(pstPtcNode->bIsTrace, LOG_LEVEL_DEBUG, "pts_save_into_recv_cache : add data into recv cache full");
        pthread_mutex_unlock(&pstPtcNode->pthreadMutex);

        return PT_NEED_CUT_PTHREAD;
    }

    pts_trace(pstPtcNode->bIsTrace, LOG_LEVEL_DEBUG, "pts_save_into_recv_cache : add data into recv cache succ");

    /* ÿ64��������Ƿ�������, ���ȫ�����գ�����ȷ���źŸ�ptc */
    if (pstStreamNode->lMaxSeq - pstStreamNode->lConfirmSeq >= PT_CONFIRM_RECV_MSG_SIZE)
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
            pts_trace(pstPtcNode->bIsTrace, LOG_LEVEL_DEBUG, "send make sure msg : %d", pstStreamNode->lConfirmSeq + PT_CONFIRM_RECV_MSG_SIZE);
            pt_logr_info("send make sure msg : %d", pstStreamNode->lConfirmSeq + PT_CONFIRM_RECV_MSG_SIZE);
            pts_send_confirm_msg(pstMsgDes, pstStreamNode->lConfirmSeq + PT_CONFIRM_RECV_MSG_SIZE);
            pstStreamNode->lConfirmSeq = pstStreamNode->lConfirmSeq + PT_CONFIRM_RECV_MSG_SIZE;
        }
    }

    if (pstMsgDes->lSeq < pstStreamNode->lMaxSeq)
    {
        pstStreamNode->ulCountResend = 0;
        pthread_mutex_unlock(&pstPtcNode->pthreadMutex);

        return PT_SAVE_DATA_SUCC;
    }

    /* lSeq �϶������һ���� */
    if (pstMsgDes->lSeq > 0)
    {
        /* �ж�ǰһ�����Ƿ����, �����ڣ�˵��û�ж��� */
        ulNextSendArraySub = (pstMsgDes->lSeq - 1) & (PT_DATA_RECV_CACHE_SIZE - 1);
        if (pstDataQueue[ulNextSendArraySub].lSeq == pstMsgDes->lSeq - 1)
        {
            pthread_mutex_unlock(&pstPtcNode->pthreadMutex);

            return PT_SAVE_DATA_SUCC;
        }
    }
    else
    {
        /* pstMsgDes->lSeq Ϊ0 */
        pthread_mutex_unlock(&pstPtcNode->pthreadMutex);

        return PT_SAVE_DATA_SUCC;
    }

    /* �ж�Ӧ�÷��͵İ��Ƿ���� */
    ulNextSendArraySub = (pstStreamNode->lCurrSeq + 1) & (PT_DATA_RECV_CACHE_SIZE - 1);
    if (pstDataQueue[ulNextSendArraySub].lSeq == pstStreamNode->lCurrSeq + 1)
    {
        enReturn = PT_SAVE_DATA_SUCC;
    }

    /* ���������û�ж�ʱ���ģ�������ʱ�� */
    if (DOS_ADDR_INVALID(pstStreamNode->hTmrHandle))
    {
        if (DOS_ADDR_INVALID(pstStreamNode->pstLostParam))
        {
            pstLoseMsg = (PT_LOSE_BAG_MSG_ST *)dos_dmem_alloc(sizeof(PT_LOSE_BAG_MSG_ST));
            if (DOS_ADDR_INVALID(pstLoseMsg))
            {
                pthread_mutex_unlock(&pstPtcNode->pthreadMutex);
                pt_logr_info("malloc lose fail");

                return PT_SAVE_DATA_FAIL;
            }
            pstLoseMsg->stMsg = *pstMsgDes;
            pstLoseMsg->pstStreamNode = pstStreamNode;
            pstLoseMsg->pPthreadMutex = &pstPtcNode->pthreadMutex;
            pstStreamNode->pstLostParam = pstLoseMsg;
        }
        pstStreamNode->ulCountResend = 0;
        pthread_mutex_unlock(&pstPtcNode->pthreadMutex);

        pt_logr_info("create lost data timer, %d streamID : %d", __LINE__, pstMsgDes->ulStreamID);
        pts_send_lost_data_req((U64)pstStreamNode->pstLostParam);
        lResult = dos_tmr_start(&pstStreamNode->hTmrHandle, PT_SEND_LOSE_DATA_TIMER, pts_send_lost_data_req, (U64)pstStreamNode->pstLostParam, TIMER_NORMAL_LOOP);
        if (PT_SAVE_DATA_FAIL == lResult)
        {
            pt_logr_debug("pts_save_into_recv_cache : start timer fail");

            return PT_SAVE_DATA_FAIL;
        }
    }
    else
    {
        pthread_mutex_unlock(&pstPtcNode->pthreadMutex);
    }

    return enReturn;
}

/**
 * ������VOID pts_handle_logout_req(PT_MSG_TAG *pstMsgDes)
 * ���ܣ�
 *      1.�����˳���½����
 * ����
 *
 * ����ֵ��
 */
VOID pts_handle_logout_req(PT_MSG_TAG *pstMsgDes)
{
    if (DOS_ADDR_INVALID(pstMsgDes))
    {
        return;
    }

    S32 lRet = 0;
    PT_CC_CB_ST *pstPtcRecvNode = NULL;
    PT_CC_CB_ST *pstPtcSendNode = NULL;
    S8 szSql[PT_DATA_BUFF_128] = {0};

    /* �˳���¼��ɾ��ptc node��֪ͨ�����޸�ptc״̬ */
    pthread_mutex_lock(&g_mutexPtcRecvList);
    pstPtcRecvNode = pt_ptc_list_search(&g_stPtcListRecv, pstMsgDes->aucID);
    if (DOS_ADDR_VALID(pstPtcRecvNode))
    {
         /* �޸����ݿ� */
        dos_snprintf(szSql, PT_DATA_BUFF_128, "update ipcc_alias set register=0 where sn='%.*s';", PTC_ID_LEN, pstMsgDes->aucID);
        lRet = dos_sqlite3_exec(g_pstMySqlite, szSql);
        if (lRet!= DOS_SUCC)
        {
            pt_logr_info("logout update db fail");
        }

        dos_tmr_stop(&pstPtcRecvNode->stHBTmrHandle);
        pt_delete_ptc_node(pstPtcRecvNode);
        pthread_mutex_unlock(&g_mutexPtcRecvList);
        pthread_mutex_lock(&g_mutexPtcSendList);
        pstPtcSendNode = pt_ptc_list_search(&g_stPtcListSend, pstMsgDes->aucID);
        if (DOS_ADDR_VALID(pstPtcSendNode))
        {
            pt_delete_ptc_node(pstPtcSendNode);
        }
        pthread_mutex_unlock(&g_mutexPtcSendList);
    }
    else
    {
        pthread_mutex_unlock(&g_mutexPtcRecvList);
    }

    return;
}

/**
 * ������VOID pts_handle_login_req(S32 lSockfd, PT_MSG_TAG *pstMsgDes, struct sockaddr_in stClientAddr, S8 *szPtcVersion)
 * ���ܣ�
 *      1.�����½����
 * ����
 *
 * ����ֵ��
 */
VOID pts_handle_login_req(PT_MSG_TAG *pstMsgDes, struct sockaddr_in stClientAddr, S8 *szPtcVersion, U32 ulIndex)
{
    if (DOS_ADDR_INVALID(pstMsgDes) || ulIndex >= PTS_UDP_LISTEN_PORT_COUNT)
    {
        return;
    }

    S8 szKey[PT_LOGIN_VERIFY_SIZE] = {0};
    S8 szBuff[sizeof(PT_CTRL_DATA_ST)] = {0};
    PT_CTRL_DATA_ST stCtrlData;
    PT_CC_CB_ST *pstPtcNode = NULL;
    S32 lResult = 0;

    /* ��������ַ��� */
    pts_create_rand_str(szKey);
    dos_memcpy(stCtrlData.szLoginVerify, szKey, PT_LOGIN_VERIFY_SIZE);
    stCtrlData.enCtrlType = PT_CTRL_LOGIN_RSP;

    /* ��ӵ�½��֤������ַ��������Ͷ��� */
    pthread_mutex_lock(&g_mutexPtcSendList);
    pstPtcNode = pt_ptc_list_search(&g_stPtcListSend, pstMsgDes->aucID);
    if(DOS_ADDR_INVALID(pstPtcNode))
    {
        pstPtcNode = pt_ptc_node_create(pstMsgDes->aucID, szPtcVersion, stClientAddr, ulIndex);
        if (NULL == pstPtcNode)
        {
            pt_logr_debug("create ptc node fail");
            return;
        }
        pt_ptc_list_insert(&g_stPtcListSend, pstPtcNode);
    }
    else
    {
        /* ���ptc��Դ */
        pt_delete_ptc_resource(pstPtcNode);
        pstPtcNode->ulIndex = ulIndex;
        pstPtcNode->stDestAddr = stClientAddr;
    }
    pthread_mutex_unlock(&g_mutexPtcSendList);

    dos_memcpy(szBuff, (VOID *)&stCtrlData, sizeof(PT_CTRL_DATA_ST));

    lResult = pts_save_ctrl_msg_into_send_cache(pstMsgDes->aucID, PT_CTRL_LOGIN_RSP, pstMsgDes->enDataType, szBuff, sizeof(PT_CTRL_DATA_ST), NULL, 0);
    if (lResult != PT_SAVE_DATA_FAIL)
    {
        pts_send_login_verify(pstMsgDes);
    }

    return;
}

/**
 * ������VOID pts_send_hb_rsp(PT_MSG_TAG *pstMsgDes)
 * ���ܣ�
 *      1.����������Ӧ
 * ����
 *
 * ����ֵ��
 */
VOID pts_send_hb_rsp(PT_MSG_TAG *pstMsgDes, struct sockaddr_in stClientAddr, U32 ulIndex)
{
    if (DOS_ADDR_INVALID(pstMsgDes))
    {
        return;
    }

    S8 szBuff[PT_SEND_DATA_SIZE] = {0};
    PT_MSG_TAG stMsgDes;
    S32 lResult = 0;

    stMsgDes.enDataType = PT_DATA_CTRL;
    dos_memcpy(stMsgDes.aucID, pstMsgDes->aucID, PTC_ID_LEN);
    stMsgDes.ulStreamID = dos_htonl(PT_CTRL_HB_RSP);
    stMsgDes.ExitNotifyFlag = DOS_FALSE;
    stMsgDes.lSeq = 0;
    stMsgDes.enCmdValue = PT_CMD_NORMAL;
    stMsgDes.bIsEncrypt = DOS_FALSE;
    stMsgDes.bIsCompress = DOS_FALSE;

    dos_memcpy(szBuff, (VOID *)&stMsgDes, sizeof(PT_MSG_TAG));

    if (g_alUdpSocket[ulIndex] > 0)
    {
        lResult = sendto(g_alUdpSocket[ulIndex], szBuff, sizeof(PT_MSG_TAG), 0, (struct sockaddr*)&stClientAddr, sizeof(stClientAddr));
        if (lResult < 0)
        {
            close(g_alUdpSocket[ulIndex]);
            g_alUdpSocket[ulIndex] = -1;
            g_alUdpSocket[ulIndex] = pts_create_udp_socket(g_stPtsMsg.usPtsPort[ulIndex], PTS_SOCKET_CACHE);
        }
        pt_logr_debug("send hb response to ptc : %.16s", pstMsgDes->aucID);
    }
    return;
}

VOID pts_send_exit_notify_to_ptc(PT_MSG_TAG *pstMsgDes, PT_CC_CB_ST *pstPtcSendNode)
{
    if (DOS_ADDR_INVALID(pstPtcSendNode))
    {
        return;
    }

    PT_MSG_TAG stMsgDes;
    S8 szBuff[PT_DATA_BUFF_512] = {0};
    S32 lResult = 0;
    U32 ulIndex = pstPtcSendNode->ulIndex;
    if (ulIndex >= PTS_UDP_LISTEN_PORT_COUNT)
    {
        return;
    }

    stMsgDes.enDataType = pstMsgDes->enDataType;
    dos_memcpy(stMsgDes.aucID, pstPtcSendNode->aucID, PTC_ID_LEN);
    stMsgDes.ulStreamID = dos_htonl(pstMsgDes->ulStreamID);
    stMsgDes.ExitNotifyFlag = DOS_TRUE;
    stMsgDes.lSeq = 0;
    stMsgDes.enCmdValue = PT_CMD_NORMAL;
    stMsgDes.bIsEncrypt = DOS_FALSE;
    stMsgDes.bIsCompress = DOS_FALSE;

    dos_memcpy(szBuff, (VOID *)&stMsgDes, sizeof(PT_MSG_TAG));
    if (g_alUdpSocket[ulIndex] > 0)
    {
        lResult = sendto(g_alUdpSocket[ulIndex], szBuff, sizeof(PT_MSG_TAG), 0, (struct sockaddr*)&pstPtcSendNode->stDestAddr, sizeof(pstPtcSendNode->stDestAddr));
        if (lResult < 0)
        {
            close(g_alUdpSocket[ulIndex]);
            g_alUdpSocket[ulIndex] = -1;
            g_alUdpSocket[ulIndex] = pts_create_udp_socket(g_stPtsMsg.usPtsPort[ulIndex], PTS_SOCKET_CACHE);
        }
    }
    return;
}

VOID pts_send_login_res2ptc(PT_MSG_TAG *pstMsgDes, struct sockaddr_in stClientAddr, U32 ulIndex, PT_PTC_LOGIN_RESULT enLoginResult)
{
    PT_CTRL_DATA_ST stLoginRes;
    S8 szBuff[PT_SEND_DATA_SIZE] = {0};
    PT_MSG_TAG stMsgDes;
    S32 lResult = 0;

    if (DOS_ADDR_INVALID(pstMsgDes) || ulIndex >= PTS_UDP_LISTEN_PORT_COUNT)
    {
        return;
    }

    stLoginRes.enCtrlType = PT_CTRL_LOGIN_ACK;
    stLoginRes.ucLoginRes = enLoginResult;

    stMsgDes.enDataType = PT_DATA_CTRL;
    dos_memcpy(stMsgDes.aucID, pstMsgDes->aucID, PTC_ID_LEN);
    stMsgDes.ulStreamID = dos_htonl(PT_CTRL_LOGIN_ACK);
    stMsgDes.ExitNotifyFlag = DOS_FALSE;
    stMsgDes.lSeq = 0;
    stMsgDes.enCmdValue = PT_CMD_NORMAL;
    stMsgDes.bIsEncrypt = DOS_FALSE;
    stMsgDes.bIsCompress = DOS_FALSE;

    dos_memcpy(szBuff, (VOID *)&stMsgDes, sizeof(PT_MSG_TAG));
    dos_memcpy(szBuff+sizeof(PT_MSG_TAG), (VOID *)&stLoginRes, sizeof(PT_CTRL_DATA_ST));

    if (g_alUdpSocket[ulIndex] > 0)
    {
        lResult = sendto(g_alUdpSocket[ulIndex], szBuff, sizeof(PT_CTRL_DATA_ST) + sizeof(PT_MSG_TAG), 0, (struct sockaddr*)&stClientAddr, sizeof(stClientAddr));
        if (lResult < 0)
        {
            close(g_alUdpSocket[ulIndex]);
            g_alUdpSocket[ulIndex] = -1;
            g_alUdpSocket[ulIndex] = pts_create_udp_socket(g_stPtsMsg.usPtsPort[ulIndex], PTS_SOCKET_CACHE);
        }
    }

    pt_logr_debug("send login response to ptc : %.16s", pstMsgDes->aucID);

    return;
}

/**
 * ������VOID pts_hd_timeout_callback(U64 param)
 * ���ܣ�
 *      1.������Ӧ��ʱ����
 * ����
 *
 * ����ֵ��
 */
VOID pts_hd_timeout_callback(U64 param)
{
    S32 lRet = 0;
    PT_CC_CB_ST *pstPtcRecvNode = (PT_CC_CB_ST *)param;
    PT_CC_CB_ST *pstPtcSendNode = NULL;
    PT_MSG_TAG  stMsgDes;
    S8 szSql[PT_DATA_BUFF_128] = {0};
    U8 aucID[PTC_ID_LEN] = {0};
    pstPtcRecvNode->usHBOutTimeCount++;
    pt_logr_debug("%.16s hd rsp timeout : %d", pstPtcRecvNode->aucID, pstPtcRecvNode->usHBOutTimeCount);
    if (pstPtcRecvNode->usHBOutTimeCount > PTS_HB_TIMEOUT_COUNT_MAX)
    {
        /* ��������޷��յ�������ptc���� */
        pt_logr_info("ptc lost connect : %.*s", PTC_ID_LEN, pstPtcRecvNode->aucID);
        dos_tmr_stop(&pstPtcRecvNode->stHBTmrHandle);
        pstPtcRecvNode->stHBTmrHandle = NULL;
        pstPtcRecvNode->usHBOutTimeCount = 0;

        stMsgDes.ulStreamID = PT_CTRL_LOGOUT;
        stMsgDes.enDataType = PT_DATA_CTRL;
        dos_memcpy(stMsgDes.aucID, pstPtcRecvNode->aucID, PTC_ID_LEN);
        /* �ͷ�ptc���ջ��� */

        pthread_mutex_lock(&g_mutexPtcRecvList);
        dos_memcpy(aucID, pstPtcRecvNode->aucID, PTC_ID_LEN);
        pt_delete_ptc_node(pstPtcRecvNode);
        pthread_mutex_unlock(&g_mutexPtcRecvList);

        /* �ͷ�ptc���ͻ��� */
        pthread_mutex_lock(&g_mutexPtcSendList);

        pstPtcSendNode = pt_ptc_list_search(&g_stPtcListSend, aucID);
        if (pstPtcSendNode != NULL)
        {
            pt_delete_ptc_node(pstPtcSendNode);
        }

        pthread_mutex_unlock(&g_mutexPtcSendList);

        /* �޸����ݿ� */
        dos_snprintf(szSql, PT_DATA_BUFF_128, "update ipcc_alias set register=0 where sn='%.*s';", PTC_ID_LEN, aucID);
        lRet = dos_sqlite3_exec(g_pstMySqlite, szSql);
        if (lRet != DOS_SUCC)
        {
            pt_logr_info("ptc lost connect, update db fail");
        }
    }
}

S32 pts_create_recv_cache(PT_MSG_TAG *pstMsgDes, S8 *szPtcVersion, struct sockaddr_in stClientAddr, U32 ulIndex)
{
    if (DOS_ADDR_INVALID(pstMsgDes) || ulIndex >= PTS_UDP_LISTEN_PORT_COUNT)
    {
        return DOS_FAIL;
    }

    PT_CC_CB_ST *pstPtcNode = NULL;
    PT_MSG_TAG stMsgDes;
    S32 lResult = 0;

    pthread_mutex_lock(&g_mutexPtcRecvList);
    pstPtcNode = pt_ptc_list_search(&g_stPtcListRecv, pstMsgDes->aucID);
    if(NULL == pstPtcNode)
    {
        pstPtcNode = pt_ptc_node_create(pstMsgDes->aucID, szPtcVersion, stClientAddr, ulIndex);
        if (NULL == pstPtcNode)
        {
            pt_logr_info("pts_login_verify : create ptc node fail");
            return DOS_FAIL;
        }
        pt_ptc_list_insert(&g_stPtcListRecv, pstPtcNode);

    }
    else
    {
        pstPtcNode->usHBOutTimeCount = 0;
        pstPtcNode->stDestAddr = stClientAddr;
    }


    stMsgDes.ExitNotifyFlag = pstMsgDes->ExitNotifyFlag;
    stMsgDes.ulStreamID = PT_CTRL_LOGIN_ACK;
    stMsgDes.enDataType = pstMsgDes->enDataType;

    /* ��½�ɹ��������������յĶ�ʱ�� */
    if (pstPtcNode->stHBTmrHandle != NULL)
    {
        dos_tmr_stop(&pstPtcNode->stHBTmrHandle);
        pstPtcNode->stHBTmrHandle= NULL;
    }

    lResult = dos_tmr_start(&pstPtcNode->stHBTmrHandle, PTS_WAIT_HB_TIME, pts_hd_timeout_callback, (U64)pstPtcNode, TIMER_NORMAL_LOOP);
    if (lResult < 0)
    {
        pt_logr_info("pts start hb timer : start timer fail");
        return DOS_FAIL;
    }
    pthread_mutex_unlock(&g_mutexPtcRecvList);

    return lResult;
}

/**
 * ������S32 pts_login_verify(S32 lSockfd, PT_MSG_TAG *pstMsgDes, S8 *pData, struct sockaddr_in stClientAddr, S8 *szPtcVersion)
 * ���ܣ�
 *      1.��½��֤
 * ����
 *
 * ����ֵ��
 */
S32 pts_login_verify(PT_MSG_TAG *pstMsgDes, S8 *pData, struct sockaddr_in stClientAddr, S8 *szPtcVersion)
{
    if (NULL == pstMsgDes || NULL == pData || NULL == szPtcVersion)
    {
        return DOS_FAIL;
    }

    S32 lResult = 0;
    S32 lRet = DOS_FAIL;
    S8 szKey[PT_LOGIN_VERIFY_SIZE] = {0};
    S8 szDestKey[PT_LOGIN_VERIFY_SIZE] = {0};
    PT_CTRL_DATA_ST *pstCtrlData = NULL;
    PT_CC_CB_ST *pstPtcNode = NULL;

    pstCtrlData = (PT_CTRL_DATA_ST *)(pData+sizeof(PT_MSG_TAG));
    pstCtrlData->ulLginVerSeq = dos_ntohl(pstCtrlData->ulLginVerSeq);
    /* ��ȡ���͵�����ַ��� */
    lResult = pts_get_key(pstMsgDes, szKey, pstCtrlData->ulLginVerSeq);
    if (lResult < 0)
    {
        /* TODO ��֤ʧ�ܣ�������ͻ��� */
        pt_logr_debug("pts_login_verify : not find key");
        pthread_mutex_lock(&g_mutexPtcSendList);
        pstPtcNode = pt_ptc_list_search(&g_stPtcListSend, pstMsgDes->aucID);
        if (DOS_ADDR_VALID(pstPtcNode))
        {
            pt_delete_ptc_node(pstPtcNode);
        }
        pthread_mutex_unlock(&g_mutexPtcSendList);
        lRet = DOS_FAIL;
    }
    else
    {
        lResult = pts_key_convert(szKey, szDestKey, szPtcVersion);
        if (lResult < 0)
        {
            pt_logr_debug("key convert error");
            pthread_mutex_lock(&g_mutexPtcSendList);
            pstPtcNode = pt_ptc_list_search(&g_stPtcListSend, pstMsgDes->aucID);
            if (DOS_ADDR_VALID(pstPtcNode))
            {
                pt_delete_ptc_node(pstPtcNode);
            }
            pthread_mutex_unlock(&g_mutexPtcSendList);
            lRet = DOS_FAIL;
        }
        else
        {
            if (dos_memcmp(szDestKey, pstCtrlData->szLoginVerify, PT_LOGIN_VERIFY_SIZE))
            {
                pt_logr_info("login : verify fail");
                /* ��֤ʧ�� */
                pthread_mutex_lock(&g_mutexPtcSendList);
                pstPtcNode = pt_ptc_list_search(&g_stPtcListSend, pstMsgDes->aucID);
                if (pstPtcNode != NULL)
                {
                    pt_delete_ptc_node(pstPtcNode);
                }
                pthread_mutex_unlock(&g_mutexPtcSendList);
                lRet = DOS_FAIL;
            }
            else
            {
                pt_logr_info("login : verify succ");
                lRet = DOS_SUCC;
            }
        }
    }

    return lRet;
}

/**
 * ������VOID pts_send_exit_notify2ptc(PT_CC_CB_ST *pstPtcNode, PT_NEND_RECV_NODE_ST *pstNeedRecvNode)
 * ���ܣ�
 *      1.�����˳�֪ͨ��ptc
 * ����
 *
 * ����ֵ��
 */
VOID pts_send_exit_notify2ptc(PT_CC_CB_ST *pstPtcNode, PT_NEND_RECV_NODE_ST *pstNeedRecvNode)
{
    if (DOS_ADDR_INVALID(pstPtcNode) || DOS_ADDR_INVALID(pstNeedRecvNode))
    {
        return;
    }

    PT_MSG_TAG stMsgDes;
    S32 lResult = 0;
    U32 ulIndex = pstPtcNode->ulIndex;

    if (ulIndex >= PTS_UDP_LISTEN_PORT_COUNT)
    {
        return;
    }

    stMsgDes.enDataType = pstNeedRecvNode->enDataType;
    dos_memcpy(stMsgDes.aucID, pstNeedRecvNode->aucID, PTC_ID_LEN);
    stMsgDes.ulStreamID = dos_htonl(pstNeedRecvNode->ulStreamID);
    stMsgDes.ExitNotifyFlag = DOS_TRUE;
    stMsgDes.lSeq = 0;
    stMsgDes.enCmdValue = PT_CMD_NORMAL;
    stMsgDes.bIsEncrypt = DOS_FALSE;
    stMsgDes.bIsCompress = DOS_FALSE;

    if (g_alUdpSocket[ulIndex] > 0)
    {
        pt_logr_info("send exit to ptc, streamID : %d", pstNeedRecvNode->ulStreamID);
        lResult = sendto(g_alUdpSocket[ulIndex], (VOID *)&stMsgDes, sizeof(PT_MSG_TAG), 0,  (struct sockaddr*)&pstPtcNode->stDestAddr, sizeof(pstPtcNode->stDestAddr));
        if (lResult < 0)
        {
            close(g_alUdpSocket[ulIndex]);
            g_alUdpSocket[ulIndex] = -1;
            g_alUdpSocket[ulIndex] = pts_create_udp_socket(g_stPtsMsg.usPtsPort[ulIndex], PTS_SOCKET_CACHE);
        }
    }
}

/**
 * ������S32 pts_get_curr_position_callback(VOID *para, S32 n_column, S8 **column_value, S8 **column_name)
 * ���ܣ�
 *      1.ping ��ô������ʱ���λ��
 * ����
 *
 * ����ֵ��
 */
S32 pts_get_curr_position_callback(VOID *para, S32 n_column, S8 **column_value, S8 **column_name)
{
    *(S32 *)para = atoi(column_value[0]);

    return 0;
}

S32 code_convert(S8 *from_charset, S8 *to_charset, S8 *inbuf, size_t inlen, S8 *outbuf, size_t outlen)
{
    iconv_t cd;
    S8 **pin = &inbuf;
    S8 **pout = &outbuf;

    cd = iconv_open(to_charset, from_charset);
    if (0 == cd)
    {
        return DOS_FAIL;
    }
    dos_memzero(outbuf, outlen);
    if (-1 == iconv(cd, pin, &inlen, pout, &outlen))
    {
        return DOS_FAIL;
    }

    iconv_close(cd);

    return DOS_SUCC;
}

S32 g2u(S8 *inbuf, size_t inlen, char *outbuf, size_t outlen)
{
    return code_convert("gb2312", "utf-8", inbuf, inlen, outbuf, outlen);
}

/**
 * ������VOID pts_ctrl_msg_handle(S32 lSockfd, U32 ulStreamID, S8 *pData, struct sockaddr_in stClientAddr)
 * ���ܣ�
 *      1.������Ϣ����
 * ����
 *
 * ����ֵ��
 */
VOID pts_ctrl_msg_handle(S8 *pData, struct sockaddr_in stClientAddr, S32 lDataLen, U32 ulIndex)
{
    if (DOS_ADDR_INVALID(pData))
    {
        return;
    }

    PT_CTRL_DATA_ST *pstCtrlData = NULL;
    HEART_BEAT_RTT_TSG *pstHbRTT = NULL;
    S32 lResult = 0;
    S32 lLoginRes = 0;
    PT_MSG_TAG * pstMsgDes = NULL;
    PT_CC_CB_ST *pstPtcRecvNode = NULL;
    PT_CC_CB_ST *pstPtcSendNode = NULL;
    S8 szSql[PTS_SQL_STR_SIZE] = {0};
    S8 szID[PTC_ID_LEN+1] = {0};
    S8 szPtcIntranetIP[IPV6_SIZE] = {0};   /* ����IP */
    S8 szPtcInternetIP[IPV6_SIZE] = {0};   /* ����IP */
    U16 usPtcIntranetPort = 0;
    U16 usPtcInternetPort = 0;
    S8 szPtcType[PT_DATA_BUFF_10] = {0};
    PT_PING_PACKET_ST *pstPingPacket = NULL;
    struct timeval stEndTime;
    U32 ulPingTime;
    S32 lCurPosition = -1;
    U32 ulDataField = 0;
    double dHBTimeInterval = 0.0;
    S8 szVersion[PT_IP_ADDR_SIZE] = {0};
    S8 szPtcName[PT_DATA_BUFF_64] = {0};
    U32 ulPTCCnt = 0, ulPTCLimit = 0;
    //S8 szNameDecode[PT_DATA_BUFF_64] = {0};

    pstMsgDes = (PT_MSG_TAG *)pData;
    pstCtrlData = (PT_CTRL_DATA_ST *)(pData + sizeof(PT_MSG_TAG));

    if (pstMsgDes->ExitNotifyFlag == DOS_TRUE)
    {
        pt_logr_debug("free ctrl msg, stream : %d", pstMsgDes->ulStreamID);
        pts_set_delete_recv_buff_list_flag(pstMsgDes->ulStreamID);
        pts_delete_send_stream_node(pstMsgDes);
        return;
    }

    dos_memcpy(szID, pstMsgDes->aucID, PTC_ID_LEN);
    szID[PTC_ID_LEN] = '\0';
    switch (pstMsgDes->ulStreamID)
    {
    case PT_CTRL_LOGIN_REQ:
        /* ��½���� */
        /* ��ȡ���Ƹ����������ȡʧ�ܾ�ʹ��Ĭ��ֵ */
        if (licc_get_limitation(PTS_SUBMOD_PTCS, &ulPTCLimit) != DOS_SUCC
            && dos_get_default_limitation(PTS_SUBMOD_PTCS, &ulPTCLimit) != DOS_SUCC)
        {
            ulPTCLimit = 0;
        }

        /* �����ȡ����ʧ�� ��ֱ�Ӿܾ� */
        if (pts_get_ptc_cnt(&ulPTCCnt) != DOS_SUCC
            || ulPTCCnt >= ulPTCLimit)
        {
            /* ptc id���� */
            pt_logr_notic("PTC %s request login. But the license do not allow. Current PTC count %u, Limitation: %u"
                            , szID, ulPTCCnt, ulPTCLimit);
            pts_send_login_res2ptc(pstMsgDes, stClientAddr, ulIndex, PT_PTC_LOGIN_FAIL_LICENSE);
            break;
        }

        pt_logr_notic("PTC %s request login. Current PTC count %u, Limitation: %u"
                            , szID, ulPTCCnt, ulPTCLimit);

        if (pts_is_ptc_sn(szID))
        {
            pt_logr_info("request login ipcc id is %s", szID);
        }
        else
        {
            /* ptc id���� */
            pt_logr_notic("request login ipcc SN(%s) format error", szID);
            pts_send_login_res2ptc(pstMsgDes, stClientAddr, ulIndex, PT_PTC_LOGIN_FAIL_SN);

            break;
        }
        pts_handle_login_req(pstMsgDes, stClientAddr, pstCtrlData->szLoginVerify, ulIndex);

        break;
    case PT_CTRL_LOGIN_RSP:
        /* ��½��֤, ��ӽ�������ջ��棬����֤�ɹ�������������ʱ�� */
        pt_logr_info("login rsp : %.16s", pstMsgDes->aucID);
        /* �ж� version */
        if (pstCtrlData->szVersion[1] != '.')
        {
            inet_ntop(AF_INET, (void *)(pstCtrlData->szVersion), szVersion, PT_IP_ADDR_SIZE);
        }
        else
        {
            dos_strcpy(szVersion, (S8 *)pstCtrlData->szVersion);
        }

        lLoginRes = pts_login_verify(pstMsgDes, pData, stClientAddr, szVersion);
        if (DOS_SUCC == lLoginRes)
        {
            inet_ntop(AF_INET, (void *)(pstMsgDes->aulServIp), szPtcIntranetIP, IPV6_SIZE);
            usPtcIntranetPort = dos_ntohs(pstMsgDes->usServPort);
            inet_ntop(AF_INET, &stClientAddr.sin_addr, szPtcInternetIP, IPV6_SIZE);
            usPtcInternetPort = dos_ntohs(stClientAddr.sin_port);

            switch (pstCtrlData->enPtcType)
            {
                case PT_PTC_TYPE_EMBEDDED:
                    strcpy(szPtcType, "Embedded");
                    dos_strncpy(szPtcName, pstCtrlData->szPtcName, PT_DATA_BUFF_64);
                    break;
                case PT_PTC_TYPE_WINDOWS:
                    dos_strcpy(szPtcType, "Windows");
                    lResult = g2u(pstCtrlData->szPtcName, dos_strlen(pstCtrlData->szPtcName), szPtcName, PT_DATA_BUFF_64);
                    if (lResult != DOS_SUCC)
                    {
                        pt_logr_info("ptc name iconv fail");
                        dos_strncpy(szPtcName, pstCtrlData->szPtcName, PT_DATA_BUFF_64);
                    }
                    break;
                case PT_PTC_TYPE_LINUX:
                    strcpy(szPtcType, "Linux");
                    dos_strncpy(szPtcName, pstCtrlData->szPtcName, PT_DATA_BUFF_64);
                    break;
                default:
                    break;
            }


            dos_snprintf(szSql, PTS_SQL_STR_SIZE, "select * from ipcc_alias where sn='%.*s'", PTC_ID_LEN, pstMsgDes->aucID);
            lResult = dos_sqlite3_record_count(g_pstMySqlite, szSql);
            if (lResult < 0)
            {
                DOS_ASSERT(0);
                lLoginRes = DOS_FAIL;
            }
            else if (DOS_TRUE == lResult)  /* �ж��Ƿ���� */
            {
                /* ���ڣ�����IPCC��ע��״̬ */
                pt_logr_debug("pts_send_msg2client : db existed");
                dos_snprintf(szSql, PTS_SQL_STR_SIZE, "update ipcc_alias set register=1, name='%s', version='%s', lastLoginTime=datetime('now','localtime'), intranetIP='%s'\
, intranetPort=%d, internetIP='%s', internetPort=%d, ptcType='%s', achPtsMajorDomain='%s', achPtsMinorDomain='%s'\
, usPtsMajorPort=%d, usPtsMinorPort=%d, szPtsHistoryIp1='%s', szPtsHistoryIp2='%s', szPtsHistoryIp3='%s'\
, usPtsHistoryPort1=%d, usPtsHistoryPort2=%d, usPtsHistoryPort3=%d, szMac='%s' where sn='%.*s';"
                             , szPtcName, szVersion, szPtcIntranetIP, usPtcIntranetPort, szPtcInternetIP, usPtcInternetPort
                             , szPtcType, pstCtrlData->achPtsMajorDomain, pstCtrlData->achPtsMinorDomain, dos_ntohs(pstCtrlData->usPtsMajorPort)
                             , dos_ntohs(pstCtrlData->usPtsMinorPort), pstCtrlData->szPtsHistoryIp1, pstCtrlData->szPtsHistoryIp2, pstCtrlData->szPtsHistoryIp3
                             , dos_ntohs(pstCtrlData->usPtsHistoryPort1), dos_ntohs(pstCtrlData->usPtsHistoryPort2), dos_ntohs(pstCtrlData->usPtsHistoryPort3)
                             , pstCtrlData->szMac, PTC_ID_LEN, pstMsgDes->aucID);

                lResult = dos_sqlite3_exec(g_pstMySqlite, szSql);
                if (lResult != DOS_SUCC)
                {
                    DOS_ASSERT(0);
                    pt_logr_info("update db state fail : %.*s", PTC_ID_LEN, pstMsgDes->aucID);
                    lLoginRes = DOS_FAIL;
                }
            }
            else
            {
                /* �����ڣ����IPCC��DB */
                pt_logr_debug("pts_send_msg2client : db insert");
                dos_snprintf(szSql, PTS_SQL_STR_SIZE, "INSERT INTO ipcc_alias (\"id\", \"sn\", \"name\", \"remark\", \"version\", \"register\", \"domain\", \"intranetIP\", \"internetIP\", \"intranetPort\"\
, \"internetPort\", \"ptcType\", \"achPtsMajorDomain\", \"achPtsMinorDomain\", \"usPtsMajorPort\", \"usPtsMinorPort\", \"szPtsHistoryIp1\", \"szPtsHistoryIp2\", \"szPtsHistoryIp3\"\
, \"usPtsHistoryPort1\", \"usPtsHistoryPort2\", \"usPtsHistoryPort3\", \"szMac\") VALUES (NULL, '%s', '%s', NULL, '%s', %d, NULL, '%s', '%s', %d, %d, '%s', '%s', '%s', %d, %d, '%s', '%s', '%s', %d, %d, %d, '%s');"
                             , szID, szPtcName, szVersion, DOS_TRUE, szPtcIntranetIP, szPtcInternetIP, usPtcIntranetPort, usPtcInternetPort, szPtcType
                             , pstCtrlData->achPtsMajorDomain, pstCtrlData->achPtsMinorDomain, dos_ntohs(pstCtrlData->usPtsMajorPort), dos_ntohs(pstCtrlData->usPtsMinorPort)
                             , pstCtrlData->szPtsHistoryIp1, pstCtrlData->szPtsHistoryIp2, pstCtrlData->szPtsHistoryIp3, dos_ntohs(pstCtrlData->usPtsHistoryPort1)
                             , dos_ntohs(pstCtrlData->usPtsHistoryPort2), dos_ntohs(pstCtrlData->usPtsHistoryPort3), pstCtrlData->szMac);

                lResult = dos_sqlite3_exec(g_pstMySqlite, szSql);
                if (lResult != DOS_SUCC)
                {
                    DOS_ASSERT(0);
                    pt_logr_info("insert db state fail : %.*s", PTC_ID_LEN, pstMsgDes->aucID);
                    lLoginRes = DOS_FAIL;
                }
            }
        }
        else
        {
            /* ��֤ʧ�� */
        }

        /* �����¼�ɹ����������ջ��� */
        if (DOS_SUCC == lLoginRes)
        {
            lResult = pts_create_recv_cache(pstMsgDes, szVersion, stClientAddr, ulIndex);
            if (lResult != DOS_SUCC)
            {
                lLoginRes = DOS_FAIL;
            }
        }

        if (DOS_SUCC == lLoginRes)
        {
            pts_send_login_res2ptc(pstMsgDes, stClientAddr, ulIndex, PT_PTC_LOGIN_SUCC);
        }
        else
        {
            pts_send_login_res2ptc(pstMsgDes, stClientAddr, ulIndex, PT_PTC_LOGIN_FAIL_VERIFY);
        }

        break;
    case PT_CTRL_LOGOUT:
        /* �˳���½ */
        pts_handle_logout_req(pstMsgDes);

        break;
    case PT_CTRL_HB_REQ:
        /* ����, �޸�ptc�е�usHBOutTimeCount */
        pt_logr_debug("recv hb from ptc : %.16s", pstMsgDes->aucID);
        if (lDataLen - sizeof(PT_MSG_TAG) > sizeof(HEART_BEAT_RTT_TSG))
        {
            dHBTimeInterval = (double)pstCtrlData->lHBTimeInterval/1000;
        }
        else
        {
            pstHbRTT =  (HEART_BEAT_RTT_TSG *)(pData + sizeof(PT_MSG_TAG));
            dHBTimeInterval = (double)pstHbRTT->lHBTimeInterval/1000;
        }
        /* ����������Ӧ֮���ʱ���͹���IP�����µ����ݿ� */
        inet_ntop(AF_INET, &stClientAddr.sin_addr, szPtcInternetIP, IPV6_SIZE);
        usPtcInternetPort = dos_ntohs(stClientAddr.sin_port);

        dos_snprintf(szSql, PTS_SQL_STR_SIZE, "update ipcc_alias set heartbeatTime=%.2f, internetIP='%s', internetPort=%d where sn='%.*s';", dHBTimeInterval, szPtcInternetIP, usPtcInternetPort, PTC_ID_LEN, pstMsgDes->aucID);
        lResult = dos_sqlite3_exec(g_pstMySqlite, szSql);
        if (lResult != DOS_SUCC)
        {
            pt_logr_info("hb time, update db fail");
        }
        pthread_mutex_lock(&g_mutexPtcRecvList);
        pstPtcRecvNode = pt_ptc_list_search(&g_stPtcListRecv, pstMsgDes->aucID);
        if(DOS_ADDR_INVALID(pstPtcRecvNode))
        {
            pt_logr_info("pts_ctrl_msg_handle : can not found ptc id = %.16s", pstMsgDes->aucID);
            pthread_mutex_unlock(&g_mutexPtcRecvList);
            break;
        }
        else
        {
            pstPtcRecvNode->usHBOutTimeCount = 0;
            if (pstPtcRecvNode->stDestAddr.sin_addr.s_addr != stClientAddr.sin_addr.s_addr || pstPtcRecvNode->stDestAddr.sin_port != stClientAddr.sin_port)
            {
                pstPtcRecvNode->stDestAddr = stClientAddr;
                pthread_mutex_unlock(&g_mutexPtcRecvList);
                pthread_mutex_lock(&g_mutexPtcSendList);
                pstPtcSendNode = pt_ptc_list_search(&g_stPtcListSend, pstMsgDes->aucID);
                if (DOS_ADDR_VALID(pstPtcSendNode))
                {
                    pstPtcSendNode->stDestAddr = stClientAddr;
                }
                pthread_mutex_unlock(&g_mutexPtcSendList);
            }
            else
            {
                pthread_mutex_unlock(&g_mutexPtcRecvList);
            }
        }


        pts_send_hb_rsp(pstMsgDes, stClientAddr, ulIndex);
        break;
    case PT_CTRL_PTS_MAJOR_DOMAIN:
        if (pstCtrlData->achPtsMajorDomain[0] != '\0')
        {
            dos_snprintf(szSql, PTS_SQL_STR_SIZE, "update ipcc_alias set achPtsMajorDomain='%s' where sn='%.*s';", pstCtrlData->achPtsMajorDomain, PTC_ID_LEN, pstMsgDes->aucID);
            lResult = dos_sqlite3_exec(g_pstMySqlite, szSql);
            if (lResult != DOS_SUCC)
            {
                DOS_ASSERT(0);
                pt_logr_info("change major domain, update db fail");
            }
        }
        if (pstCtrlData->usPtsMajorPort != 0)
        {
            dos_snprintf(szSql, PTS_SQL_STR_SIZE, "update ipcc_alias set usPtsMajorPort=%d where sn='%.*s';", dos_ntohs(pstCtrlData->usPtsMajorPort), PTC_ID_LEN, pstMsgDes->aucID);
            lResult = dos_sqlite3_exec(g_pstMySqlite, szSql);
            if (lResult != DOS_SUCC)
            {
                DOS_ASSERT(0);
                pt_logr_info("change major port, update db fail");
            }
        }
        break;
    case PT_CTRL_PTS_MINOR_DOMAIN:
        if (pstCtrlData->achPtsMinorDomain[0] != '\0')
        {
            dos_snprintf(szSql, PTS_SQL_STR_SIZE, "update ipcc_alias set achPtsMinorDomain='%s' where sn='%.*s';", pstCtrlData->achPtsMinorDomain, PTC_ID_LEN, pstMsgDes->aucID);
            lResult = dos_sqlite3_exec(g_pstMySqlite, szSql);
            if (lResult != DOS_SUCC)
            {
                DOS_ASSERT(0);
                pt_logr_info("change minor domain, update db fail");
            }
        }
        if (pstCtrlData->usPtsMinorPort != 0)
        {
            dos_snprintf(szSql, PTS_SQL_STR_SIZE, "update ipcc_alias set usPtsMinorPort=%d where sn='%.*s';", dos_ntohs(pstCtrlData->usPtsMinorPort), PTC_ID_LEN, pstMsgDes->aucID);
            lResult = dos_sqlite3_exec(g_pstMySqlite, szSql);
            if (lResult != DOS_SUCC)
            {
                DOS_ASSERT(0);
                pt_logr_info("change minor port, update db fail");
            }
        }
        break;
    case PT_CTRL_PING_ACK:
        pstPingPacket = (PT_PING_PACKET_ST *)(pData + sizeof(PT_MSG_TAG));
        gettimeofday(&stEndTime, NULL);
        /* ����ʱ��� */
        ulPingTime = (stEndTime.tv_sec - pstPingPacket->stStartTime.tv_sec) * 1000 * 1000 + (stEndTime.tv_usec - pstPingPacket->stStartTime.tv_usec);
        /* ��������ʱʱ�� */
        if (ulPingTime < (PTS_PING_TIMEOUT)*1000)
        {
            dos_tmr_stop(&pstPingPacket->hTmrHandle);
            pstPingPacket->hTmrHandle = NULL;
            dos_snprintf(szSql, PTS_SQL_STR_SIZE, "select curr_position from ping_result where sn='%.*s' limit 1 ", PTC_ID_LEN, pstMsgDes->aucID);
            lResult = dos_sqlite3_exec_callback(g_pstMySqlite, szSql, pts_get_curr_position_callback, (VOID *)&lCurPosition);
            if (lResult != DOS_SUCC)
            {
                DOS_ASSERT(0);
                return;
            }

            if (lCurPosition < 0)
            {
                dos_snprintf(szSql, PTS_SQL_STR_SIZE, "INSERT INTO ping_result (\"id\", \"sn\", \"curr_position\", \"timer0\") VALUES (NULL, '%.*s', 0, %d)", PTC_ID_LEN, pstMsgDes->aucID, ulPingTime);
            }
            else
            {
                ulDataField = lCurPosition + 1;
                if (ulDataField > 7)
                {
                    ulDataField = 0;
                }

                dos_snprintf(szSql, PTS_SQL_STR_SIZE, "update ping_result set curr_position=%d, timer%d=%d where sn='%.*s'", ulDataField, ulDataField, ulPingTime, PTC_ID_LEN, pstMsgDes->aucID);
            }

            lResult = dos_sqlite3_exec(g_pstMySqlite, szSql);
            if (lResult != DOS_SUCC)
            {
                pt_logr_info("update hb time fail : %.*s", PTC_ID_LEN, pstMsgDes->aucID);
            }
        }
        break;
    default:
        break;
    }

}

/**
 * ������VOID pts_delete_stream_node(PT_MSG_TAG *pstMsgDes, PT_CC_CB_ST *pstPtcRecvNode)
 * ���ܣ�
 *      1.ɾ��stream �ڵ�
 * ����
 *
 * ����ֵ��
 */

VOID pts_delete_recv_stream_node(PT_MSG_TAG *pstMsgDes)
{
    if (DOS_ADDR_INVALID(pstMsgDes))
    {
        return;
    }

    list_t              *pstStreamListHead  = NULL;
    PT_STREAM_CB_ST     *pstStreamNode      = NULL;
    PT_CC_CB_ST         *pstPtcRecvNode     = NULL;
    PT_SEND_MSG_PTHREAD *pstSendPthread     = NULL;

    pthread_mutex_lock(&g_mutexPtcRecvList);
    pstPtcRecvNode = pt_ptc_list_search(&g_stPtcListRecv, pstMsgDes->aucID);
    if (DOS_ADDR_INVALID(pstPtcRecvNode))
    {
        pthread_mutex_unlock(&g_mutexPtcRecvList);

        return;
    }

    pthread_mutex_lock(&pstPtcRecvNode->pthreadMutex);
    pthread_mutex_unlock(&g_mutexPtcRecvList);

    pstStreamListHead = &pstPtcRecvNode->astDataTypes[pstMsgDes->enDataType].stStreamQueHead;
    if (DOS_ADDR_VALID(pstStreamListHead))
    {
        pstStreamNode = pt_stream_queue_search(pstStreamListHead, pstMsgDes->ulStreamID);
        if (DOS_ADDR_VALID(pstStreamNode))
        {
            if (DOS_ADDR_VALID(pstStreamNode->hTmrHandle))
            {
                dos_tmr_stop(&pstStreamNode->hTmrHandle);
                pstStreamNode->hTmrHandle = NULL;
            }
            if (pstMsgDes->enDataType == PT_DATA_WEB)
            {
                if (pstStreamNode->bIsUsing)
                {
                    pthread_mutex_lock(&g_mutexSendMsgPthreadList);
                    pstSendPthread = pt_send_msg_pthread_search(&g_stSendMsgPthreadList, pstMsgDes->ulStreamID);
                    if (DOS_ADDR_VALID(pstSendPthread))
                    {
                        if (DOS_ADDR_VALID(pstSendPthread->pstPthreadParam))
                        {
                            pstSendPthread->pstPthreadParam->bIsNeedExit = DOS_TRUE;
                            sem_post(&pstSendPthread->pstPthreadParam->stSemSendMsg);
                        }
                    }
                    pthread_mutex_unlock(&g_mutexSendMsgPthreadList);
                }
            }

            pt_delete_stream_node(&pstStreamNode->stStreamListNode, pstMsgDes->enDataType);
        }
    }

    pthread_mutex_unlock(&pstPtcRecvNode->pthreadMutex);

}

VOID pts_delete_send_stream_node(PT_MSG_TAG *pstMsgDes)
{
    if (DOS_ADDR_INVALID(pstMsgDes))
    {
        return;
    }

    list_t          *pstStreamListHead  = NULL;
    PT_STREAM_CB_ST *pstStreamNode      = NULL;
    PT_CC_CB_ST     *pstPtcSendNode     = NULL;

    pthread_mutex_lock(&g_mutexPtcSendList);
    pstPtcSendNode = pt_ptc_list_search(&g_stPtcListSend, pstMsgDes->aucID);
    if (DOS_ADDR_INVALID(pstPtcSendNode))
    {
         pthread_mutex_unlock(&g_mutexPtcSendList);

         return;
    }
    pthread_mutex_lock(&pstPtcSendNode->pthreadMutex);
    pthread_mutex_unlock(&g_mutexPtcSendList);

    pstStreamListHead = &pstPtcSendNode->astDataTypes[pstMsgDes->enDataType].stStreamQueHead;
    if (DOS_ADDR_VALID(pstStreamListHead))
    {
        pstStreamNode = pt_stream_queue_search(pstStreamListHead, pstMsgDes->ulStreamID);
        if (DOS_ADDR_VALID(pstStreamNode))
        {
            pt_delete_stream_node(&pstStreamNode->stStreamListNode, pstMsgDes->enDataType);
        }
    }

    pthread_mutex_unlock(&pstPtcSendNode->pthreadMutex);
}

/**
 * ������S32 pts_deal_with_confirm_msg(PT_CC_CB_ST *pstPtcNode, PT_MSG_TAG *pstMsgDes)
 * ���ܣ�
 *      1.������Ϣȷ����Ϣ
 * ����
 *
 * ����ֵ����
 */
S32 pts_deal_with_confirm_msg(PT_MSG_TAG *pstMsgDes)
{
    PT_CC_CB_ST        *pstPtcNode         = NULL;
    list_t             *pstStreamListHead  = NULL;
    PT_STREAM_CB_ST    *pstStreamNode      = NULL;
    S32 lResult = DOS_FALSE;

    if (NULL == pstMsgDes)
    {
        return lResult;
    }


    pthread_mutex_lock(&g_mutexPtcSendList);

    pstPtcNode = pt_ptc_list_search(&g_stPtcListSend, pstMsgDes->aucID);
    if(DOS_ADDR_INVALID(pstPtcNode))
    {
        pthread_mutex_unlock(&g_mutexPtcSendList);

        return lResult;
    }

    pstStreamListHead = &pstPtcNode->astDataTypes[pstMsgDes->enDataType].stStreamQueHead;
    if (DOS_ADDR_INVALID(pstStreamListHead))
    {
        pthread_mutex_unlock(&g_mutexPtcSendList);

        return lResult;
    }

    pstStreamNode = pt_stream_queue_search(pstStreamListHead, pstMsgDes->ulStreamID);
    if (DOS_ADDR_INVALID(pstStreamNode))
    {
        pthread_mutex_unlock(&g_mutexPtcSendList);

        return lResult;
    }

    if (pstStreamNode->lConfirmSeq < pstMsgDes->lSeq)
    {
        pstStreamNode->lConfirmSeq = pstMsgDes->lSeq;
        lResult = DOS_TRUE;
    }

    pthread_mutex_unlock(&g_mutexPtcSendList);

    return lResult;
}

/**
 * ������void pts_save_msg_into_cache(S8 *pcIpccId, PT_DATA_TYPE_EN enDataType, U32 ulStreamID, S8 *pcData, S32 lDataLen, U8 ExitNotifyFlag)
 * ���ܣ�
 *      1.������ݵ����ͻ�����
 *      2.�����ӳɹ�����֪ͨ�����̣߳���������
 * ����
 *      S8 *pcIpccId                  : IPCC ID
 *      PT_DATA_TYPE_EN enDataType    ��data������
 *      U32 ulStreamID                ��stream ID
 *      S8 *pcData                    ��������
 *      S32 lDataLen                  �������ݳ���
 *      U8 ExitNotifyFlag             ��֪ͨ�Է���Ӧ�Ƿ����
 * ����ֵ����
 */
S32 pts_save_msg_into_cache(U8 *pcIpccId, PT_DATA_TYPE_EN enDataType, U32 ulStreamID, S8 *pcData, S32 lDataLen, S8 *szDestIp, U16 usDestPort)
{
    S32         lResult     = 0;
    BOOL        bIsResend   = DOS_FALSE;
    PT_CMD_EN   enCmdValue  = PT_CMD_NORMAL;
    PT_MSG_TAG  stMsgDes;

    if (enDataType < 0 || enDataType >= PT_DATA_BUTT || DOS_ADDR_INVALID(pcIpccId) || DOS_ADDR_INVALID(pcData))
    {
        return PT_SAVE_DATA_FAIL;
    }

    if (ulStreamID < PT_CTRL_BUTT)
    {
        lResult = pts_save_ctrl_msg_into_send_cache(pcIpccId, ulStreamID, enDataType, pcData, lDataLen, szDestIp, usDestPort);
        if (lResult < 0)
        {
            /* ��ӷ�����Ϣʧ�� */
            pt_logr_info("save msg into send cache fail!");

            return PT_SAVE_DATA_FAIL;
        }
        else
        {
            stMsgDes.ExitNotifyFlag = DOS_FALSE;
            stMsgDes.ulStreamID = ulStreamID;
            stMsgDes.enDataType = enDataType;
            pthread_mutex_lock(&g_mutexPtsSendPthread);
            pt_need_send_node_list_insert(&g_stPtsNendSendNode, pcIpccId, &stMsgDes, enCmdValue, bIsResend);
            pthread_cond_signal(&g_condPtsSend);
            pthread_mutex_unlock(&g_mutexPtsSendPthread);
        }
    }
    else
    {
        lResult = pts_save_msg_into_send_cache(pcIpccId, ulStreamID, enDataType, pcData, lDataLen, szDestIp, usDestPort);
        if (lResult < 0)
        {
            /* ��ӷ�����Ϣʧ�� */
            pt_logr_info("save msg into send cache fail!");

            return PT_SAVE_DATA_FAIL;
        }
        else
        {
            stMsgDes.ExitNotifyFlag = DOS_FALSE;
            stMsgDes.ulStreamID = ulStreamID;
            stMsgDes.enDataType = enDataType;
            pthread_mutex_lock(&g_mutexPtsSendPthread);
            if (NULL == pt_need_send_node_list_search(&g_stPtsNendSendNode, ulStreamID))
            {
                pt_need_send_node_list_insert(&g_stPtsNendSendNode, pcIpccId, &stMsgDes, enCmdValue, bIsResend);
            }
            pthread_cond_signal(&g_condPtsSend);
            pthread_mutex_unlock(&g_mutexPtsSendPthread);
        }
    }

    return lResult;
}

VOID pts_global_variable_init()
{
    sem_init(&g_SemPts, 0, 0);

    pthread_mutex_lock(&g_mutexPtcSendList);
    dos_list_init(&g_stPtcListSend);
    pthread_mutex_unlock(&g_mutexPtcSendList);

    pthread_mutex_lock(&g_mutexPtcRecvList);
    dos_list_init(&g_stPtcListRecv);
    pthread_mutex_unlock(&g_mutexPtcRecvList);

    pthread_mutex_lock(&g_mutexPtsRecvMsgHandle);
    dos_list_init(&g_stMsgRecvFromPtc);
    pthread_mutex_unlock(&g_mutexPtsRecvMsgHandle);

    pthread_mutex_lock(&g_mutexPtsRecvPthread);
    dos_list_init(&g_stPtsNendRecvNode);
    pthread_mutex_unlock(&g_mutexPtsRecvPthread);

    pthread_mutex_lock(&g_mutexPtsSendPthread);
    dos_list_init(&g_stPtsNendSendNode);
    pthread_mutex_unlock(&g_mutexPtsSendPthread);

    pthread_mutex_lock(&g_mutexStreamAddrList);
    DLL_Init(&g_stStreamAddrList);
    pthread_mutex_unlock(&g_mutexStreamAddrList);

    pthread_mutex_lock(&g_mutexSendMsgPthreadList);
    dos_list_init(&g_stSendMsgPthreadList);
    pthread_mutex_unlock(&g_mutexSendMsgPthreadList);
}

/**
 * ������void *pts_send_msg2ipcc(VOID *arg)
 * ���ܣ�
 *      1.���������߳�
 * ����
 *      VOID *arg :ͨ��ͨ�ŵ�sockfd
 * ����ֵ��void *
 */
VOID *pts_send_msg2ptc(VOID *arg)
{
    PT_CC_CB_ST      *pstPtcNode     = NULL;
    U32              ulArraySub      = 0;
    U32              ulSendCount     = 0;
    PT_MSG_TAG       stMsgDes;
    list_t           *pstStreamHead   = NULL;
    list_t           *pstNendSendList = NULL;
    PT_STREAM_CB_ST  *pstStreamNode   = NULL;
    PT_DATA_TCP_ST   *pstSendDataHead = NULL;
    DLL_NODE_S       *pstListNode     = NULL;
    STREAM_CACHE_ADDR_CB_ST *pstStreamCacheAddr = NULL;
    PT_DATA_TCP_ST   stSendDataNode;
    S8 szBuff[PT_SEND_DATA_SIZE] = {0};
    PT_NEND_SEND_NODE_ST *pstNeedSendNode = NULL;
    PT_DATA_TCP_ST    stRecvDataTcp;
    S32 lResult = 0;
    U32 ulIndex = 0;
    struct timeval now;
    struct timespec timeout;

    pts_global_variable_init();

    while(1)
    {
        gettimeofday(&now, NULL);
        timeout.tv_sec = now.tv_sec + 1;
        timeout.tv_nsec = now.tv_usec * 1000;
        pthread_mutex_lock(&g_mutexPtsSendPthread);
        pthread_cond_timedwait(&g_condPtsSend, &g_mutexPtsSendPthread, &timeout);
        pthread_mutex_unlock(&g_mutexPtsSendPthread);

        while(1)
        {
            pthread_mutex_lock(&g_mutexPtsSendPthread);
            if (dos_list_is_empty(&g_stPtsNendSendNode))
            {
                pthread_mutex_unlock(&g_mutexPtsSendPthread);
                break;
            }

            pstNendSendList = dos_list_fetch(&g_stPtsNendSendNode);
            if (DOS_ADDR_INVALID(pstNendSendList))
            {
                pthread_mutex_unlock(&g_mutexPtsSendPthread);
                DOS_ASSERT(0);
                continue;
            }
            pthread_mutex_unlock(&g_mutexPtsSendPthread);

            pstNeedSendNode = dos_list_entry(pstNendSendList, PT_NEND_SEND_NODE_ST, stListNode);

            dos_memzero(&stMsgDes, sizeof(PT_MSG_TAG));

            if (PT_DATA_CTRL == pstNeedSendNode->enDataType)
            {
                pthread_mutex_lock(&g_mutexPtcSendList);
                pstPtcNode = pt_ptc_list_search(&g_stPtcListSend, pstNeedSendNode->aucID);
                if(DOS_ADDR_INVALID(pstPtcNode))
                {
                    pthread_mutex_unlock(&g_mutexPtcSendList);
                    pt_logr_debug("pts_send_msg2ptc : not found ptc");
                    dos_dmem_free(pstNeedSendNode);
                    pstNeedSendNode = NULL;
                    continue;
                }
                pthread_mutex_lock(&pstPtcNode->pthreadMutex);
                pthread_mutex_unlock(&g_mutexPtcSendList);
            }
            else
            {
                pthread_mutex_lock(&g_mutexStreamAddrList);
                pstListNode = dll_find(&g_stStreamAddrList, (VOID *)&pstNeedSendNode->ulStreamID, pts_find_stream_addr_by_streamID);
                if (DOS_ADDR_INVALID(pstListNode))
                {
                    pthread_mutex_unlock(&g_mutexStreamAddrList);
                    dos_dmem_free(pstNeedSendNode);
                    pstNeedSendNode = NULL;
                    continue;
                }

                pstStreamCacheAddr = pstListNode->pHandle;
                if (DOS_ADDR_INVALID(pstStreamCacheAddr))
                {
                    pthread_mutex_unlock(&g_mutexStreamAddrList);
                    dos_dmem_free(pstNeedSendNode);
                    pstNeedSendNode = NULL;
                    continue;
                }

                pstPtcNode = pstStreamCacheAddr->pstPtcSendNode;
                if (DOS_ADDR_INVALID(pstPtcNode))
                {
                    pthread_mutex_unlock(&g_mutexStreamAddrList);
                    dos_dmem_free(pstNeedSendNode);
                    pstNeedSendNode = NULL;
                    continue;
                }

                pstStreamNode = pstStreamCacheAddr->pstStreamSendNode;
                pthread_mutex_lock(&pstPtcNode->pthreadMutex);
                pthread_mutex_unlock(&g_mutexStreamAddrList);
            }

            ulIndex = pstPtcNode->ulIndex;
            if (ulIndex >= PTS_UDP_LISTEN_PORT_COUNT)
            {
                pthread_mutex_unlock(&pstPtcNode->pthreadMutex);
                pt_logr_debug("pts_send_msg2ptc : not found ptc");
                dos_dmem_free(pstNeedSendNode);
                pstNeedSendNode = NULL;
                continue;
            }

            if (pstNeedSendNode->enCmdValue != PT_CMD_NORMAL || pstNeedSendNode->ExitNotifyFlag == DOS_TRUE)
            {
                /* ���Ͷ������ط����� / ��Ϣȷ�Ͻ��� / ���ͽ�����Ϣ */
                stMsgDes.enDataType = pstNeedSendNode->enDataType;
                dos_memcpy(stMsgDes.aucID, pstNeedSendNode->aucID, PTC_ID_LEN);
                stMsgDes.ulStreamID = dos_htonl(pstNeedSendNode->ulStreamID);
                stMsgDes.ExitNotifyFlag = pstNeedSendNode->ExitNotifyFlag;
                stMsgDes.lSeq = dos_htonl(pstNeedSendNode->lSeqResend);
                stMsgDes.enCmdValue = pstNeedSendNode->enCmdValue;
                stMsgDes.bIsEncrypt = DOS_FALSE;
                stMsgDes.bIsCompress = DOS_FALSE;

                if (g_alUdpSocket[ulIndex] > 0)
                {
                    lResult = sendto(g_alUdpSocket[ulIndex], (VOID *)&stMsgDes, sizeof(PT_MSG_TAG), 0,  (struct sockaddr*)&pstPtcNode->stDestAddr, sizeof(pstPtcNode->stDestAddr));
                    if (lResult < 0)
                    {
                        close(g_alUdpSocket[ulIndex]);
                        g_alUdpSocket[ulIndex] = -1;
                        g_alUdpSocket[ulIndex] = pts_create_udp_socket(g_stPtsMsg.usPtsPort[ulIndex], PTS_SOCKET_CACHE);
                    }
                }
                pthread_mutex_unlock(&pstPtcNode->pthreadMutex);
                dos_dmem_free(pstNeedSendNode);
                pstNeedSendNode = NULL;
                continue;
            }

            if (PT_DATA_CTRL == pstNeedSendNode->enDataType)
            {
                pstStreamHead = &pstPtcNode->astDataTypes[pstNeedSendNode->enDataType].stStreamQueHead;
                if (DOS_ADDR_INVALID(pstStreamHead))
                {
                    pthread_mutex_unlock(&pstPtcNode->pthreadMutex);
                    pt_logr_debug("pts_send_msg2ptc : stream list is NULL");
                    dos_dmem_free(pstNeedSendNode);
                    pstNeedSendNode = NULL;
                    continue;
                }

                pstStreamNode = pt_stream_queue_search(pstStreamHead, pstNeedSendNode->ulStreamID);
            }

            if(DOS_ADDR_INVALID(pstStreamNode) || pstStreamNode->ulStreamID != pstNeedSendNode->ulStreamID)
            {
                pthread_mutex_unlock(&pstPtcNode->pthreadMutex);
                pt_logr_debug("pts_send_msg2ptc : cann't found stream node");
                dos_dmem_free(pstNeedSendNode);
                pstNeedSendNode = NULL;
                continue;
            }

            pstSendDataHead = pstStreamNode->unDataQueHead.pstDataTcp;
            if (DOS_ADDR_INVALID(pstSendDataHead))
            {
                pthread_mutex_unlock(&pstPtcNode->pthreadMutex);
                pt_logr_debug("send data to ptc : data queue is NULL");
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
                    pt_logr_debug("send data to pts : data package is not exit");
                }
                else
                {
                    stSendDataNode = pstSendDataHead[ulArraySub];
                    stMsgDes.enDataType = pstNeedSendNode->enDataType;
                    dos_memcpy(stMsgDes.aucID, pstNeedSendNode->aucID, PTC_ID_LEN);
                    stMsgDes.ulStreamID = dos_htonl(pstNeedSendNode->ulStreamID);
                    stMsgDes.ExitNotifyFlag = stSendDataNode.ExitNotifyFlag;
                    stMsgDes.lSeq = dos_htonl(pstNeedSendNode->lSeqResend);
                    stMsgDes.enCmdValue = pstNeedSendNode->enCmdValue;
                    stMsgDes.bIsEncrypt = DOS_FALSE;
                    stMsgDes.bIsCompress = DOS_FALSE;
                    dos_memcpy(stMsgDes.aulServIp, pstStreamNode->aulServIp, IPV6_SIZE);
                    stMsgDes.usServPort = dos_htons(pstStreamNode->usServPort);

                    dos_memcpy(szBuff, (VOID *)&stMsgDes, sizeof(PT_MSG_TAG));
                    dos_memcpy(szBuff+sizeof(PT_MSG_TAG), stSendDataNode.szBuff, stSendDataNode.ulLen);

                    ulSendCount = PT_RESEND_RSP_COUNT;    /*�ش��ģ���������*/
                    while (ulSendCount)
                    {
                        pts_trace(pstPtcNode->bIsTrace, LOG_LEVEL_DEBUG, "resend data to ptc, stream : %d, seq : %d, size : %d", pstNeedSendNode->ulStreamID, pstNeedSendNode->lSeqResend, stSendDataNode.ulLen);
                        if (g_alUdpSocket[ulIndex] > 0)
                        {
                            usleep(20);
                            lResult = sendto(g_alUdpSocket[ulIndex], szBuff, stSendDataNode.ulLen + sizeof(PT_MSG_TAG), 0, (struct sockaddr*)&pstPtcNode->stDestAddr, sizeof(pstPtcNode->stDestAddr));
                            if (lResult < 0)
                            {
                                close(g_alUdpSocket[ulIndex]);
                                g_alUdpSocket[ulIndex] = -1;
                                g_alUdpSocket[ulIndex] = pts_create_udp_socket(g_stPtsMsg.usPtsPort[ulIndex], PTS_SOCKET_CACHE);
                            }
                        }
                        ulSendCount--;
                    }

                }
                dos_dmem_free(pstNeedSendNode);
                pstNeedSendNode = NULL;
                pthread_mutex_unlock(&pstPtcNode->pthreadMutex);
                continue;
            }

            while(1)
            {
                /* ����data��ֱ�������� */
                pstStreamNode->lCurrSeq++;
                ulArraySub = (pstStreamNode->lCurrSeq) & (PT_DATA_SEND_CACHE_SIZE - 1);
                stRecvDataTcp = pstSendDataHead[ulArraySub];
                if (stRecvDataTcp.lSeq == pstStreamNode->lCurrSeq)
                {
                    stMsgDes.enDataType = pstNeedSendNode->enDataType;
                    dos_memcpy(stMsgDes.aucID, pstNeedSendNode->aucID, PTC_ID_LEN);
                    stMsgDes.ulStreamID = dos_htonl(pstNeedSendNode->ulStreamID);
                    stMsgDes.ExitNotifyFlag = stRecvDataTcp.ExitNotifyFlag;
                    stMsgDes.lSeq = dos_htonl(pstStreamNode->lCurrSeq);
                    stMsgDes.enCmdValue = pstNeedSendNode->enCmdValue;
                    stMsgDes.bIsEncrypt = DOS_FALSE;
                    stMsgDes.bIsCompress = DOS_FALSE;
                    dos_memcpy(stMsgDes.aulServIp, pstStreamNode->aulServIp, IPV6_SIZE);
                    stMsgDes.usServPort = dos_htons(pstStreamNode->usServPort);

                    dos_memcpy(szBuff, (VOID *)&stMsgDes, sizeof(PT_MSG_TAG));
                    dos_memcpy(szBuff+sizeof(PT_MSG_TAG), stRecvDataTcp.szBuff, stRecvDataTcp.ulLen);

                    if (g_alUdpSocket[ulIndex] > 0)
                    {
                        pts_trace(pstPtcNode->bIsTrace, LOG_LEVEL_DEBUG, "send data to ptc, streamID is %d, seq is %d ", pstNeedSendNode->ulStreamID, pstStreamNode->lCurrSeq);
                        lResult = sendto(g_alUdpSocket[ulIndex], szBuff, stRecvDataTcp.ulLen + sizeof(PT_MSG_TAG), 0, (struct sockaddr*)&pstPtcNode->stDestAddr, sizeof(pstPtcNode->stDestAddr));
                        if (lResult < 0)
                        {
                            close(g_alUdpSocket[ulIndex]);
                            g_alUdpSocket[ulIndex] = -1;
                            g_alUdpSocket[ulIndex] = pts_create_udp_socket(g_stPtsMsg.usPtsPort[ulIndex], PTS_SOCKET_CACHE);
                        }
                        usleep(20);
                    }
                }
                else
                {
                    /* ���seqû�з��ͣ���һ */
                    pstStreamNode->lCurrSeq--;

                    break;
                }
            }
            pthread_mutex_unlock(&pstPtcNode->pthreadMutex);
            dos_dmem_free(pstNeedSendNode);
            pstNeedSendNode = NULL;
        } /* end of while(1) */

    } /* end of while(1) */
}

/**
 * ������void *pts_recv_msg_from_ipcc(VOID *arg)
 * ���ܣ�
 *      1.���������߳�
 * ����
 *      VOID *arg :ͨ��ͨ�ŵ�sockfd
 * ����ֵ��
 */
VOID *pts_recv_msg_from_ptc(VOID *arg)
{
    S32                 i               = 0;
    S32                 lRecvLen        = 0;
    S32                 lResult         = 0;
    U8                  *pcRecvBuf      = NULL;
    U32                 MaxFdp          = 0;
    struct sockaddr_in  stClientAddr;
    socklen_t           lCliaddrLen     = sizeof(stClientAddr);
    struct timeval      stTimeVal       = {1, 0};
    struct timeval      stTimeValCpy;
    fd_set              ReadFds;
    PTS_REV_MSG_STREAM_LIST_ST *pstStreamListNode = NULL;
    PT_MSG_TAG                 *pstMsgDes         = NULL;

    while(1)
    {
        if (g_alUdpSocket[0] <= 0 && g_alUdpSocket[1] <= 0)
        {
            sleep(1);
            continue;
        }

        stTimeValCpy = stTimeVal;
        FD_ZERO(&ReadFds);
        if (g_alUdpSocket[0] > 0)
        {
            FD_SET(g_alUdpSocket[0], &ReadFds);
        }

        if (g_alUdpSocket[1] > 0)
        {
            FD_SET(g_alUdpSocket[1], &ReadFds);
        }

        MaxFdp = g_alUdpSocket[0] > g_alUdpSocket[1] ? g_alUdpSocket[0] : g_alUdpSocket[1];

        lResult = select(MaxFdp + 1, &ReadFds, NULL, NULL, &stTimeValCpy);
        if (lResult < 0)
        {
            perror("pts_recv_msg_from_ptc select");
            DOS_ASSERT(0);
            sleep(1);
            continue;
        }
        else if (0 == lResult)
        {
            continue;
        }

        for (i=0; i<PTS_UDP_LISTEN_PORT_COUNT; i++)
        {
            if (g_alUdpSocket[i] <= 0)
            {
                continue;
            }

            if (FD_ISSET(g_alUdpSocket[i], &ReadFds))
            {
                lCliaddrLen = sizeof(stClientAddr);
                pcRecvBuf = (U8 *)dos_dmem_alloc(PT_SEND_DATA_SIZE);
                if (DOS_ADDR_INVALID(pcRecvBuf))
                {
                    pt_logr_warning("recv msg from ptc malloc fail!");
                    sleep(1);

                    break;
                }

                lRecvLen = recvfrom(g_alUdpSocket[i], pcRecvBuf, PT_SEND_DATA_SIZE, 0, (struct sockaddr*)&stClientAddr, &lCliaddrLen);
                if (lRecvLen < 0)
                {
                    pt_logr_info("recvfrom fail ,create socket again");
                    dos_dmem_free(pcRecvBuf);
                    pcRecvBuf = NULL;
                    close(g_alUdpSocket[i]);
                    g_alUdpSocket[i] = -1;
                    g_alUdpSocket[i] = pts_create_udp_socket(g_stPtsMsg.usPtsPort[i], PTS_SOCKET_CACHE);

                    break;
                }

                pstMsgDes = (PT_MSG_TAG *)pcRecvBuf;
                /* �ֽ���ת�� */
                pstMsgDes->ulStreamID = dos_ntohl(pstMsgDes->ulStreamID);
                pstMsgDes->lSeq = dos_ntohl(pstMsgDes->lSeq);
                if (pstMsgDes->ulStreamID > PT_CTRL_BUTT)
                {
                     pt_logr_debug("recv from ptc, stream : %d, seq : %d", pstMsgDes->ulStreamID, pstMsgDes->lSeq);
                }
                pthread_mutex_lock(&g_mutexPtsRecvMsgHandle);

                pstStreamListNode = pts_search_recv_buff_list(&g_stMsgRecvFromPtc, pstMsgDes->ulStreamID);
                if (DOS_ADDR_INVALID(pstStreamListNode))
                {
                    /* �½� */
                    pstStreamListNode = pts_create_recv_buff_list(&g_stMsgRecvFromPtc, pstMsgDes->ulStreamID);
                    if (DOS_ADDR_INVALID(pstStreamListNode))
                    {
                        dos_dmem_free(pcRecvBuf);
                        pcRecvBuf = NULL;
                        pthread_mutex_unlock(&g_mutexPtsRecvMsgHandle);
                        continue;
                    }
                }
                else
                {
                    pstStreamListNode->ulLastUseTime = time(NULL);
                }

                pthread_mutex_unlock(&g_mutexPtsRecvMsgHandle);
                pthread_mutex_lock(&pstStreamListNode->pMutexPthread);
                pts_recvfrom_ptc_buff_list_insert(pstStreamListNode, pcRecvBuf, lRecvLen, i, stClientAddr, pstMsgDes->lSeq);
                pthread_mutex_unlock(&pstStreamListNode->pMutexPthread);

                pthread_mutex_lock(&g_mutexPtsRecvMsgHandle);
                pthread_cond_signal(&g_condPtsRecvMsgHandle);
                pthread_mutex_unlock(&g_mutexPtsRecvMsgHandle);
            }
        }

    } /* end of while(1) */
}

VOID *pts_handle_recvfrom_ptc_msg(VOID *arg)
{
    list_t *pstRecvBuffList = NULL;
    list_t *pstRecvBuffNode = NULL;
    PTS_REV_MSG_HANDLE_ST   *pstRecvBuff        = NULL;
    PT_MSG_TAG              *pstMsgDes          = NULL;
    PTS_REV_MSG_STREAM_LIST_ST *pstStreamListNode = NULL;
    S32  lSaveIntoCacheRes = 0;
    S32 lResult = 0;
    struct timeval now;
    struct timespec timeout;

    while (1)
    {
        gettimeofday(&now, NULL);
        timeout.tv_sec = now.tv_sec + 1;
        timeout.tv_nsec = now.tv_usec * 1000;

        pthread_mutex_lock(&g_mutexPtsRecvMsgHandle);
        pthread_cond_timedwait(&g_condPtsRecvMsgHandle, &g_mutexPtsRecvMsgHandle, &timeout);
        pthread_mutex_unlock(&g_mutexPtsRecvMsgHandle);

        pstRecvBuffList = &g_stMsgRecvFromPtc;

        while(1)
        {
            if (dos_list_is_empty(&g_stMsgRecvFromPtc))
            {
                pthread_mutex_unlock(&g_mutexPtsRecvMsgHandle);
                break;
            }

            pthread_mutex_lock(&g_mutexPtsRecvMsgHandle);
            pstRecvBuffList = dos_list_work(&g_stMsgRecvFromPtc, pstRecvBuffList);
            if (DOS_ADDR_INVALID(pstRecvBuffList))
            {
                pthread_mutex_unlock(&g_mutexPtsRecvMsgHandle);
                break;
            }

            pstStreamListNode = dos_list_entry(pstRecvBuffList, PTS_REV_MSG_STREAM_LIST_ST, stSteamList);
            if (time(NULL) - pstStreamListNode->ulLastUseTime > PTS_RECVFROM_PTC_MSG_TIMEOUT)
            {
                /* ��ʱ���������ڵ� */
                pstStreamListNode->bIsValid = DOS_FALSE;
            }

            pthread_mutex_unlock(&g_mutexPtsRecvMsgHandle);

            while (1)
            {
                pthread_mutex_lock(&pstStreamListNode->pMutexPthread);

                if (!pstStreamListNode->bIsValid)
                {
                    /* ɾ�� */
                    pthread_mutex_unlock(&pstStreamListNode->pMutexPthread);
                    pthread_mutex_lock(&g_mutexPtsRecvMsgHandle);
                    pstRecvBuffList = pstStreamListNode->stSteamList.prev;
                    dos_list_del(&pstStreamListNode->stSteamList);
                    pthread_mutex_unlock(&g_mutexPtsRecvMsgHandle);

                    pts_delete_recv_buff_list(pstStreamListNode);

                    break;
                }

                if (dos_list_is_empty(&pstStreamListNode->stBuffList))
                {
                    pthread_mutex_unlock(&pstStreamListNode->pMutexPthread);
                    break;
                }

                pstRecvBuffNode = dos_list_fetch(&pstStreamListNode->stBuffList);
                if (DOS_ADDR_INVALID(pstRecvBuffNode))
                {
                    pthread_mutex_unlock(&pstStreamListNode->pMutexPthread);
                    break;
                }
                pstRecvBuff = dos_list_entry(pstRecvBuffNode, PTS_REV_MSG_HANDLE_ST, stList);
                pstStreamListNode->lHandleMaxSeq = pstStreamListNode->lHandleMaxSeq > pstRecvBuff->lSeq ? pstStreamListNode->lHandleMaxSeq : pstRecvBuff->lSeq;
                pthread_mutex_unlock(&pstStreamListNode->pMutexPthread);

                /* ȡ��ͷ����Ϣ */
                pstMsgDes = (PT_MSG_TAG *)pstRecvBuff->paRecvBuff;
                if (pstMsgDes->enDataType == PT_DATA_CTRL && pstMsgDes->ulStreamID != PT_CTRL_PTC_PACKAGE)
                {
                    /* ������Ϣ */
                    pts_ctrl_msg_handle((S8 *)pstRecvBuff->paRecvBuff, pstRecvBuff->stClientAddr, pstRecvBuff->ulRecvLen, pstRecvBuff->ulIndex);
                }
                else
                {
                    if (pstMsgDes->enCmdValue == PT_CMD_RESEND)
                    {
                        /* pts���͵��ش����� */
                        BOOL bIsResend = DOS_TRUE;
                        PT_CMD_EN enCmdValue = PT_CMD_NORMAL;
                        pt_logr_debug("recv resend msg, stream : %d, seq : %d", pstMsgDes->ulStreamID, pstMsgDes->lSeq);
                        pthread_mutex_lock(&g_mutexPtsSendPthread);
                        pt_need_send_node_list_insert(&g_stPtsNendSendNode, pstMsgDes->aucID, pstMsgDes, enCmdValue, bIsResend);
                        pthread_cond_signal(&g_condPtsSend);
                        pthread_mutex_unlock(&g_mutexPtsSendPthread);

                    }
                    else if (pstMsgDes->enCmdValue == PT_CMD_CONFIRM)
                    {
                        /* ȷ�Ͻ�����Ϣ */
                        pt_logr_debug("pts recv make sure, seq : %d", pstMsgDes->lSeq);
                        lResult = pts_deal_with_confirm_msg(pstMsgDes);
                        if (lResult)
                        {
                            if (PT_DATA_WEB == pstMsgDes->enDataType)
                            {
                                pts_set_cache_full_false(pstMsgDes->ulStreamID);
                            }
                        }

                    }
                    else if (pstMsgDes->ExitNotifyFlag == DOS_TRUE)
                    {
                        /* stream �˳� */
                        pt_logr_debug("pts recv exit msg, streamID = %d", pstMsgDes->ulStreamID);
                        pts_free_stream_resource(pstMsgDes);
                        pthread_mutex_lock(&g_mutexPtsRecvPthread);
                        pt_need_recv_node_list_insert(&g_stPtsNendRecvNode, pstMsgDes);
                        pthread_cond_signal(&g_condPtsRecv);
                        pthread_mutex_unlock(&g_mutexPtsRecvPthread);
                    }
                    else
                    {
                        pt_logr_debug("deal with data from ptc, streamID = %d, seq : %d, len : %d", pstMsgDes->ulStreamID, pstMsgDes->lSeq, pstRecvBuff->ulRecvLen-sizeof(PT_MSG_TAG));
                        lSaveIntoCacheRes = pts_save_into_recv_cache(pstMsgDes, (S8 *)pstRecvBuff->paRecvBuff + sizeof(PT_MSG_TAG), pstRecvBuff->ulRecvLen-sizeof(PT_MSG_TAG));
                        if (PT_SAVE_DATA_SUCC == lSaveIntoCacheRes)
                        {
                            pthread_mutex_lock(&g_mutexPtsRecvPthread);
                            if (NULL == pt_need_recv_node_list_search(&g_stPtsNendRecvNode, pstMsgDes->ulStreamID))
                            {
                                pt_need_recv_node_list_insert(&g_stPtsNendRecvNode, pstMsgDes);
                            }
                            pthread_cond_signal(&g_condPtsRecv);
                            pthread_mutex_unlock(&g_mutexPtsRecvPthread);
                        }
                        else if (PT_NEED_CUT_PTHREAD == lSaveIntoCacheRes)
                        {
                            /* ���� */
                            pthread_mutex_lock(&pstStreamListNode->pMutexPthread);
                            dos_list_add_head(&pstStreamListNode->stBuffList, &pstRecvBuff->stList);
                            pthread_mutex_unlock(&pstStreamListNode->pMutexPthread);
                            break;
                        }
                        else
                        {
                            /* ʧ�ܻ������Ѿ����ڵİ� */
                            pt_logr_info("save into recv cache fail");
                        }
                    }
                }

                dos_dmem_free(pstRecvBuff->paRecvBuff);
                pstRecvBuff->paRecvBuff = NULL;
                dos_dmem_free(pstRecvBuff);
                pstRecvBuff = NULL;
            }
       } /* end of while(1) */
    }
}

#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */

