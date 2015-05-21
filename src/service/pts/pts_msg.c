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
sem_t     g_SemPtsRecv;                     /* �����ź��� */
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

extern S32 pts_create_udp_socket(U16 usUdpPort, U32 ulSocketCache);

S8 *pts_get_current_time()
{
    time_t ulCurrTime = 0;

    ulCurrTime = time(NULL);
    return ctime(&ulCurrTime);
}

S32 pts_get_sn_by_id(S8 *szID, S8 *szSN, S32 lLen)
{
    S8 achSql[PTS_SQL_STR_SIZE] = {0};

    dos_memzero(szSN, lLen);
    sprintf(achSql, "select sn from ipcc_alias where id=%s and register = 1;", szID);
    dos_sqlite3_exec_callback(g_pstMySqlite, achSql, pts_get_password_callback, (void *)szSN);
    if (dos_strlen(szSN) > 0)
    {
        return DOS_SUCC;
    }
    else
    {
        return DOS_FAIL;
    }

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

    if (ulStreamID == pstStreamCacheAddr->ulStrreamID)
    {
        return DOS_SUCC;
    }

    return DOS_FAIL;
}

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

U32 pts_recvfrom_ptc_buff_list_insert(list_t *pstHead, U8 *szBUff, U32 ulBuffLen, struct sockaddr_in stClientAddr)
{
    if (DOS_ADDR_INVALID(szBUff) || DOS_ADDR_INVALID(pstHead))
    {
        return DOS_FAIL;
    }

    PTS_REV_MSG_HANDLE_ST *pstNewNode = (PTS_REV_MSG_HANDLE_ST *)dos_dmem_alloc(sizeof(PTS_REV_MSG_HANDLE_ST));
    if (NULL == pstNewNode)
    {
        perror("malloc");
        return DOS_FAIL;
    }

    pstNewNode->paRecvBuff = szBUff;
    pstNewNode->ulRecvLen = ulBuffLen;
    pstNewNode->stClientAddr = stClientAddr;

    dos_list_add_tail(pstHead, &(pstNewNode->stList));

    return DOS_SUCC;
}

/**
 * ������S32 pts_printf_recv_msg(U32 ulIndex, S32 argc, S8 **argv)
 * ���ܣ�
 *      1.��ӡ���ջ�����ƿ����Ϣ
 * ����
 * ����ֵ��
 */
S32 pts_printf_recv_msg(U32 ulIndex, S32 argc, S8 **argv)
{
    #if 0
    S32 i = 0;
    U32 ulLen = 0;
    S32 lResult = 0;
    S8 szBuff[PT_DATA_BUFF_512] = {0};
    S8 szSN[PT_DATA_BUFF_64] = {0};
    PT_CC_CB_ST *pstPtcSendNode = NULL;
    list_t *pstStreamQueHead = NULL;
    list_t *pstStreamQueNode = NULL;
    PT_STREAM_CB_ST *pstStreamNode = NULL;

    if (argc != 2)
    {
        cli_out_string(ulIndex, "Usage : ptsd recv [ID|SN].\r\n");

        return 0;
    }

    if (pts_is_ptc_sn(argv[1]))
    {
        dos_strncpy(szSN, argv[1], PT_DATA_BUFF_64);
    }
    else if (pts_is_int(argv[1]))
    {
        lResult = pts_get_sn_by_id(argv[1], szSN, PT_DATA_BUFF_64);
        if (lResult != DOS_SUCC)
        {
            cli_out_string(ulIndex, "Get SN fail by ID\r\n");

            return 0;
        }
    }
    else
    {
        cli_out_string(ulIndex, "Usage : ptsd recv [ID|SN].\r\n");

        return 0;
    }

    pstPtcSendNode = pt_ptc_list_search(g_pstPtcListRecv, szSN);
    if (NULL == pstPtcSendNode)
    {
        cli_out_string(ulIndex, "Can not find ptc node\r\n");

        return 0;
    }

    ulLen = snprintf(szBuff, sizeof(szBuff), "\r\n%20s%10s%10s%10s\r\n", "aucID", "type", "ulStreamID");
    cli_out_string(ulIndex, szBuff);

    for (i=0; i<PT_DATA_BUTT; i++)
    {
        pstStreamQueHead = pstPtcSendNode->astDataTypes[i].pstStreamQueHead;
        if (pstStreamQueHead != NULL)
        {
            pstStreamQueNode = pstStreamQueHead;
            while (pstStreamQueNode->next != pstStreamQueHead)
            {
                pstStreamNode = (PT_STREAM_CB_ST *)pstStreamQueNode;
                snprintf(szBuff, sizeof(szBuff), "%.16s%10d%10d%10d\r\n", g_astCmdClient[i].aucID, g_astCmdClient[i].ulStreamID, g_astCmdClient[i].lSocket, g_astCmdClient[i].bIsValid);
                cli_out_string(ulIndex, szBuff);

                pstStreamQueNode = pstStreamQueNode->next;
            }

        }
    }

#endif
    return 0;
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
    printf("send lose data : stream = %d, seq = %d\n", pstMsgDes->ulStreamID, lLoseSeq);
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
    pthread_mutex_t     *pPthreadMutex   = pstLoseMsg->pPthreadMutex;
    S32                 i               = 0;
    U32                 ulCount         = 0;
    U32                 ulArraySub      = 0;

    if (DOS_ADDR_INVALID(pstStreamNode) || DOS_ADDR_INVALID(pPthreadMutex))
    {
        return;
    }

    pthread_mutex_lock(pPthreadMutex);

    if (pstStreamNode->ulCountResend >= 3)
    {
        /* 3���δ�յ������رն�ʱ����sockfd */
        pt_logr_error("stream resend fail, close��stream is %d", pstStreamNode->ulStreamID);
        printf("stream resend fail, close��stream is %d\n", pstStreamNode->ulStreamID);
        dos_tmr_stop(&pstStreamNode->hTmrHandle);
        pstStreamNode->hTmrHandle= NULL;
        pthread_mutex_unlock(pPthreadMutex);

        pts_delete_stream_addr_node(pstLoseMsg->stMsg.ulStreamID);
        pts_delete_recv_stream_node(&pstLoseMsg->stMsg);
        pthread_mutex_lock(&g_mutexPtcSendList);
        pstPtcSendNode = pt_ptc_list_search(&g_stPtcListSend, pstLoseMsg->stMsg.aucID);
        if (NULL != pstPtcSendNode)
        {
            pts_send_exit_notify_to_ptc(&pstLoseMsg->stMsg, pstPtcSendNode);
        }
        pthread_mutex_unlock(&g_mutexPtcSendList);

        pts_delete_send_stream_node(&pstLoseMsg->stMsg);

        return;
    }
    else
    {
        pstStreamNode->ulCountResend++;
        /* �������ҳ���ʧ�İ��������ش���Ϣ */
        for (i=pstStreamNode->lCurrSeq+1; i<pstStreamNode->lMaxSeq; i++)
        {
            ulArraySub = i & (PT_DATA_SEND_CACHE_SIZE - 1);
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
            printf("!!!%p\n", pstStreamNode->hTmrHandle);
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
    if (NULL == pstMsgDes)
    {
        pt_logr_debug("arg error");
        return;
    }

    S32 i = 0;

    BOOL bIsResend = DOS_FALSE;
    PT_CMD_EN enCmdValue = PT_CMD_CONFIRM;
    pstMsgDes->lSeq = lConfirmSeq;

    pt_logr_info("send confirm data, type = %d, stream = %d", pstMsgDes->enDataType,pstMsgDes->ulStreamID);
    pthread_mutex_lock(&g_mutexPtsSendPthread);
    for (i=0; i<PTS_SEND_CONFIRM_MSG_COUNT; i++)
    {
        pt_need_send_node_list_insert(&g_stPtsNendSendNode, pstMsgDes->aucID, pstMsgDes, enCmdValue, bIsResend);
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
    if (NULL == pstMsgDes)
    {
        pt_logr_debug("arg error");
        return;
    }

    BOOL bIsResend = DOS_FALSE;
    PT_CMD_EN enCmdValue = PT_CMD_NORMAL;
    pthread_mutex_lock(&g_mutexPtsSendPthread);
    //if (NULL == pt_need_send_node_list_search(&g_stPtsNendSendNode, pstMsgDes->ulStreamID))
    //{
    pstMsgDes->ulStreamID = PT_CTRL_LOGIN_RSP;
    pt_need_send_node_list_insert(&g_stPtsNendSendNode, pstMsgDes->aucID, pstMsgDes, enCmdValue, bIsResend);
    //}
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
    if (NULL == pstStreamNode)
    {
        /* ����stream node */
        pstStreamNode = pt_stream_node_create(ulStreamID);
        if (NULL == pstStreamNode)
        {
            /* stream node����ʧ�� */
            pt_logr_info("pts_save_into_send_cache : create stream node fail");
            pthread_mutex_unlock(&pstPtcNode->pthreadMutex);

            return PT_SAVE_DATA_FAIL;
        }
        if (szDestIp != NULL)
        {
            pstStreamNode->usServPort = usDestPort;
            inet_pton(AF_INET, szDestIp, (VOID *)(pstStreamNode->aulServIp));
        }
        /* ���� stream list�� */
        pt_stream_queue_insert(pstStreamListHead, &(pstStreamNode->stStreamListNode));
    }


    pstDataQueue = pstStreamNode->unDataQueHead.pstDataTcp;
    if (NULL == pstDataQueue)
    {
        /* ����tcp data queue */
        pstDataQueue = pt_data_tcp_queue_create(PT_DATA_RECV_CACHE_SIZE);
        if (NULL == pstDataQueue)
        {
            /* ����data queueʧ��*/
            pt_logr_info("pts_save_into_send_cache : create data queue fail");
            pthread_mutex_unlock(&pstPtcNode->pthreadMutex);

            return PT_SAVE_DATA_FAIL;
        }

        pstStreamNode->unDataQueHead.pstDataTcp = pstDataQueue;
    }

    /*�����ݲ��뵽data queue��*/
    lResult = pt_send_data_tcp_queue_insert(pstStreamNode, acSendBuf, lDataLen, PT_DATA_SEND_CACHE_SIZE);
    if (lResult < 0)
    {
        pt_logr_info("pts_save_into_send_cache : add data into send cache fail");
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
    if (DOS_ADDR_INVALID(pstStreamNode))
    {
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

            if (szDestIp != NULL)
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
    lResult = pt_send_data_tcp_queue_insert(pstStreamNode, acSendBuf, lDataLen, PT_DATA_SEND_CACHE_SIZE);
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
    if (DOS_ADDR_INVALID(pstMsgDes) || DOS_ADDR_INVALID(acRecvBuf))
    {
        return PT_SAVE_DATA_FAIL;
    }

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
        if(NULL == pstPtcNode)
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
    if (DOS_ADDR_INVALID(pstStreamNode))
    {
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
    lResult = pt_recv_data_tcp_queue_insert(pstStreamNode, pstMsgDes, acRecvBuf, lDataLen, PT_DATA_RECV_CACHE_SIZE);
    if (lResult < 0)
    {
        pt_logr_info("pts_save_into_recv_cache : add data into recv cache fail");
        pthread_mutex_unlock(&pstPtcNode->pthreadMutex);

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
            pt_logr_info("send make sure msg : %d", pstStreamNode->lConfirmSeq + PT_CONFIRM_RECV_MSG_SIZE);
            pts_send_confirm_msg(pstMsgDes, pstStreamNode->lConfirmSeq + PT_CONFIRM_RECV_MSG_SIZE);
            pstStreamNode->lConfirmSeq = pstStreamNode->lConfirmSeq + PT_CONFIRM_RECV_MSG_SIZE;
        }
    }

    if (PT_NEED_CUT_PTHREAD == lResult)
    {
        pthread_mutex_unlock(&pstPtcNode->pthreadMutex);

        return PT_NEED_CUT_PTHREAD;
    }

    /* �ж�Ӧ�÷��͵İ��Ƿ񶪰� */
    if (pstStreamNode->lCurrSeq + 1 == pstMsgDes->lSeq)
    {
        if (NULL != pstStreamNode->hTmrHandle)
        {
            /* ������ڶ�ʱ�����������Ϊ��ʧ����С�� */
            pstStreamNode->ulCountResend = 0;
        }
        pthread_mutex_unlock(&pstPtcNode->pthreadMutex);

        return PT_SAVE_DATA_SUCC;
    }

    if (pstMsgDes->lSeq <= pstStreamNode->lCurrSeq)
    {
        /* �ѷ��͹��İ� */
        pthread_mutex_unlock(&pstPtcNode->pthreadMutex);

        return PT_SAVE_DATA_FAIL;
    }

    ulNextSendArraySub = (pstStreamNode->lCurrSeq + 1) & (PT_DATA_RECV_CACHE_SIZE - 1);
    if (pstDataQueue[ulNextSendArraySub].lSeq == pstStreamNode->lCurrSeq + 1)
    {
        pthread_mutex_unlock(&pstPtcNode->pthreadMutex);
        return PT_SAVE_DATA_SUCC;
    }

    if (pstMsgDes->lSeq == 0 && pstStreamNode->lCurrSeq == -1)
    {
        pthread_mutex_unlock(&pstPtcNode->pthreadMutex);
        return PT_SAVE_DATA_SUCC;
    }

    /* ���������û�ж�ʱ���ģ�������ʱ�� */
    if (DOS_ADDR_INVALID(pstStreamNode->hTmrHandle))
    {
        if (DOS_ADDR_INVALID(pstStreamNode->pstLostParam))
        {
            pstLoseMsg = (PT_LOSE_BAG_MSG_ST *)dos_dmem_alloc(sizeof(PT_LOSE_BAG_MSG_ST));
            if (NULL == pstLoseMsg)
            {
                perror("malloc");
                pthread_mutex_unlock(&pstPtcNode->pthreadMutex);

                return PT_SAVE_DATA_FAIL;
            }
            pstLoseMsg->stMsg = *pstMsgDes;
            pstLoseMsg->pstStreamNode = pstStreamNode;
            pstLoseMsg->pPthreadMutex = &pstPtcNode->pthreadMutex;
            pstStreamNode->pstLostParam = pstLoseMsg;
        }
        pstStreamNode->ulCountResend = 0;
        pthread_mutex_unlock(&pstPtcNode->pthreadMutex);

        pt_logr_info("create timer, %d streamID : %d", __LINE__, pstMsgDes->ulStreamID);
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

    return PT_SAVE_DATA_FAIL;
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
    if (NULL == pstMsgDes)
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
    if (pstPtcRecvNode != NULL)
    {
        dos_tmr_stop(&pstPtcRecvNode->stHBTmrHandle);
        pt_delete_ptc_node(pstPtcRecvNode);
        pthread_mutex_unlock(&g_mutexPtcRecvList);
        pthread_mutex_lock(&g_mutexPtcSendList);
        pstPtcSendNode = pt_ptc_list_search(&g_stPtcListSend, pstMsgDes->aucID);
        if (pstPtcSendNode != NULL)
        {
            pt_delete_ptc_node(pstPtcSendNode);
        }
        pthread_mutex_unlock(&g_mutexPtcSendList);
        /* ֪ͨpts���޸����ݿ� */
        dos_snprintf(szSql, PT_DATA_BUFF_128, "update ipcc_alias set register=0 where sn='%.*s';", PTC_ID_LEN, pstMsgDes->aucID);
        lRet = dos_sqlite3_exec(g_pstMySqlite, szSql);
        if (lRet!= DOS_SUCC)
        {
            pt_logr_info("logout update db fail");
        }
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
VOID pts_handle_login_req(S32 lSockfd, PT_MSG_TAG *pstMsgDes, struct sockaddr_in stClientAddr, S8 *szPtcVersion)
{
    if (NULL == pstMsgDes)
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
        pstPtcNode = pt_ptc_node_create(pstMsgDes->aucID, szPtcVersion, stClientAddr);
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
VOID pts_send_hb_rsp(PT_MSG_TAG *pstMsgDes, struct sockaddr_in stClientAddr)
{
    if (NULL == pstMsgDes)
    {
        return;
    }

    PT_CTRL_DATA_ST stLoginRes;
    S8 szBuff[PT_SEND_DATA_SIZE] = {0};
    PT_MSG_TAG stMsgDes;
    S32 lResult = 0;

    stLoginRes.enCtrlType = PT_CTRL_HB_RSP;
    stMsgDes.enDataType = PT_DATA_CTRL;
    dos_memcpy(stMsgDes.aucID, pstMsgDes->aucID, PTC_ID_LEN);
    stMsgDes.ulStreamID = dos_htonl(PT_CTRL_HB_RSP);
    stMsgDes.ExitNotifyFlag = DOS_FALSE;
    stMsgDes.lSeq = 0;
    stMsgDes.enCmdValue = PT_CMD_NORMAL;
    stMsgDes.bIsEncrypt = DOS_FALSE;
    stMsgDes.bIsCompress = DOS_FALSE;

    dos_memcpy(szBuff, (VOID *)&stMsgDes, sizeof(PT_MSG_TAG));
    dos_memcpy(szBuff+sizeof(PT_MSG_TAG), (VOID *)&stLoginRes, sizeof(PT_CTRL_DATA_ST));

    if (g_ulUdpSocket > 0)
    {
        lResult = sendto(g_ulUdpSocket, szBuff, sizeof(PT_CTRL_DATA_ST) + sizeof(PT_MSG_TAG), 0, (struct sockaddr*)&stClientAddr, sizeof(stClientAddr));
        if (lResult < 0)
        {
            close(g_ulUdpSocket);
            g_ulUdpSocket = -1;
            g_ulUdpSocket = pts_create_udp_socket(g_stPtsMsg.usPtsPort, PTS_SOCKET_CACHE);
        }
        pt_logr_debug("send hb response to ptc : %.16s", pstMsgDes->aucID);
    }
    return;
}

VOID pts_send_exit_notify_to_ptc(PT_MSG_TAG *pstMsgDes, PT_CC_CB_ST *pstPtcSendNode)
{
    if (NULL == pstPtcSendNode)
    {
        return;
    }

    PT_MSG_TAG stMsgDes;
    S8 szBuff[PT_DATA_BUFF_512] = {0};
    S32 lResult = 0;

    stMsgDes.enDataType = pstMsgDes->enDataType;
    dos_memcpy(stMsgDes.aucID, pstPtcSendNode->aucID, PTC_ID_LEN);
    stMsgDes.ulStreamID = dos_htonl(pstMsgDes->ulStreamID);
    stMsgDes.ExitNotifyFlag = DOS_TRUE;
    stMsgDes.lSeq = 0;
    stMsgDes.enCmdValue = PT_CMD_NORMAL;
    stMsgDes.bIsEncrypt = DOS_FALSE;
    stMsgDes.bIsCompress = DOS_FALSE;

    dos_memcpy(szBuff, (VOID *)&stMsgDes, sizeof(PT_MSG_TAG));
    if (g_ulUdpSocket > 0)
    {
        lResult = sendto(g_ulUdpSocket, szBuff, sizeof(PT_MSG_TAG), 0, (struct sockaddr*)&pstPtcSendNode->stDestAddr, sizeof(pstPtcSendNode->stDestAddr));
        if (lResult < 0)
        {
            close(g_ulUdpSocket);
            g_ulUdpSocket = -1;
            g_ulUdpSocket = pts_create_udp_socket(g_stPtsMsg.usPtsPort, PTS_SOCKET_CACHE);
        }
    }
    return;
}

VOID pts_send_login_res2ptc(PT_MSG_TAG *pstMsgDes, struct sockaddr_in stClientAddr, BOOL bLoginRes)
{
    if (NULL == pstMsgDes)
    {
        return;
    }

    PT_CTRL_DATA_ST stLoginRes;
    S8 szBuff[PT_SEND_DATA_SIZE] = {0};
    PT_MSG_TAG stMsgDes;
    S32 lResult = 0;

    stLoginRes.enCtrlType = PT_CTRL_LOGIN_ACK;
    stLoginRes.ucLoginRes = bLoginRes;

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

    if (g_ulUdpSocket > 0)
    {
        lResult = sendto(g_ulUdpSocket, szBuff, sizeof(PT_CTRL_DATA_ST) + sizeof(PT_MSG_TAG), 0, (struct sockaddr*)&stClientAddr, sizeof(stClientAddr));
        if (lResult < 0)
        {
            close(g_ulUdpSocket);
            g_ulUdpSocket = -1;
            g_ulUdpSocket = pts_create_udp_socket(g_stPtsMsg.usPtsPort, PTS_SOCKET_CACHE);
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

S32 pts_create_recv_cache(PT_MSG_TAG *pstMsgDes, S8 *szPtcVersion, struct sockaddr_in stClientAddr)
{
    if (NULL == pstMsgDes)
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
        pstPtcNode = pt_ptc_node_create(pstMsgDes->aucID, szPtcVersion, stClientAddr);
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
        if (pstPtcNode != NULL)
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
            if (pstPtcNode != NULL)
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

    stMsgDes.enDataType = pstNeedRecvNode->enDataType;
    dos_memcpy(stMsgDes.aucID, pstNeedRecvNode->aucID, PTC_ID_LEN);
    stMsgDes.ulStreamID = dos_htonl(pstNeedRecvNode->ulStreamID);
    stMsgDes.ExitNotifyFlag = DOS_TRUE;
    stMsgDes.lSeq = 0;
    stMsgDes.enCmdValue = PT_CMD_NORMAL;
    stMsgDes.bIsEncrypt = DOS_FALSE;
    stMsgDes.bIsCompress = DOS_FALSE;

    if (g_ulUdpSocket > 0)
    {
        lResult = sendto(g_ulUdpSocket, (VOID *)&stMsgDes, sizeof(PT_MSG_TAG), 0,  (struct sockaddr*)&pstPtcNode->stDestAddr, sizeof(pstPtcNode->stDestAddr));
        if (lResult < 0)
        {
            close(g_ulUdpSocket);
            g_ulUdpSocket = -1;
            g_ulUdpSocket = pts_create_udp_socket(g_stPtsMsg.usPtsPort, PTS_SOCKET_CACHE);
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
VOID pts_ctrl_msg_handle(S32 lSockfd, S8 *pData, struct sockaddr_in stClientAddr, S32 lDataLen)
{
    if (NULL == pData)
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
    //S8 szNameDecode[PT_DATA_BUFF_64] = {0};

    pstMsgDes = (PT_MSG_TAG *)pData;
    pstCtrlData = (PT_CTRL_DATA_ST *)(pData + sizeof(PT_MSG_TAG));

    if (pstMsgDes->ExitNotifyFlag == DOS_TRUE)
    {
        pts_delete_send_stream_node(pstMsgDes);
        return;
    }

    dos_memcpy(szID, pstMsgDes->aucID, PTC_ID_LEN);
    szID[PTC_ID_LEN] = '\0';
    switch (pstMsgDes->ulStreamID) //pstCtrlData->enCtrlType
    {
    case PT_CTRL_LOGIN_REQ:
        /* ��½���� */
        if (pts_is_ptc_sn(szID))
        {
            pt_logr_debug("request login ipcc id is %s", szID);
        }
        else
        {
            /* ptc id���� */
            pt_logr_error("request login ipcc id(%s) format error", szID);
            pts_send_login_res2ptc(pstMsgDes, stClientAddr, DOS_FALSE);
            break;
        }
        pts_handle_login_req(lSockfd, pstMsgDes, stClientAddr, pstCtrlData->szLoginVerify);

        break;
    case PT_CTRL_LOGIN_RSP:
        /* ��½��֤, ��ӽ�������ջ��棬����֤�ɹ�������������ʱ�� */
        logr_debug("login rsp : %.16s", pstMsgDes->aucID);
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
            lResult = pts_create_recv_cache(pstMsgDes, szVersion, stClientAddr);
            if (lResult != DOS_SUCC)
            {
                lLoginRes = DOS_FAIL;
            }
        }

        if (DOS_SUCC == lLoginRes)
        {
            pts_send_login_res2ptc(pstMsgDes, stClientAddr, DOS_TRUE);
        }
        else
        {
            pts_send_login_res2ptc(pstMsgDes, stClientAddr, DOS_FALSE);
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
        if(NULL == pstPtcRecvNode)
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
                if (NULL != pstPtcSendNode)
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


        pts_send_hb_rsp(pstMsgDes, stClientAddr);
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
            printf("delete recv stream, %d\n", pstMsgDes->ulStreamID);
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
            printf("delete send stream, %d\n", pstMsgDes->ulStreamID);
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
    if(NULL == pstPtcNode)
    {
        pthread_mutex_unlock(&g_mutexPtcSendList);

        return lResult;
    }

    pstStreamListHead = &pstPtcNode->astDataTypes[pstMsgDes->enDataType].stStreamQueHead;
    if (NULL == pstStreamListHead)
    {
        pthread_mutex_unlock(&g_mutexPtcSendList);

        return lResult;
    }

    pstStreamNode = pt_stream_queue_search(pstStreamListHead, pstMsgDes->ulStreamID);
    if (NULL == pstStreamNode)
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
    if (enDataType < 0 || enDataType >= PT_DATA_BUTT)
    {
        pt_logr_debug("Data Type should in 0-%d: %d", PT_DATA_BUTT - 1, enDataType);
        return PT_SAVE_DATA_FAIL;
    }
    else if (DOS_ADDR_INVALID(pcIpccId) || DOS_ADDR_INVALID(pcData))
    {
        pt_logr_debug("data pointer is NULL");
        return PT_SAVE_DATA_FAIL;
    }

    S32         lResult     = 0;
    BOOL        bIsResend   = DOS_FALSE;
    PT_CMD_EN   enCmdValue  = PT_CMD_NORMAL;
    PT_MSG_TAG  stMsgDes;

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
            if (NULL == pt_need_send_node_list_search(&g_stPtsNendSendNode, ulStreamID))
            {
                pt_need_send_node_list_insert(&g_stPtsNendSendNode, pcIpccId, &stMsgDes, enCmdValue, bIsResend);
            }
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

VOID pts_init()
{
    sem_init(&g_SemPts, 0, 0);
    sem_init(&g_SemPtsRecv, 0, 1);

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
    PT_DATA_TCP_ST   stSendDataNode;
    S8 szBuff[PT_SEND_DATA_SIZE] = {0};
    PT_NEND_SEND_NODE_ST *pstNeedSendNode = NULL;
    PT_DATA_TCP_ST    stRecvDataTcp;
    S32 lResult = 0;
    struct timeval now;
    struct timespec timeout;
    //U32  ulSeqCount = 0;
    //struct timeval stTime;
    pts_init();

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

            pthread_mutex_lock(&g_mutexPtcSendList);
            pstPtcNode = pt_ptc_list_search(&g_stPtcListSend, pstNeedSendNode->aucID);
            if(NULL == pstPtcNode)
            {
                pthread_mutex_unlock(&g_mutexPtcSendList);
                pt_logr_debug("pts_send_msg2ptc : not found ptc");
                dos_dmem_free(pstNeedSendNode);
                pstNeedSendNode = NULL;
                continue;
            }
            else if (pstNeedSendNode->enCmdValue != PT_CMD_NORMAL || pstNeedSendNode->ExitNotifyFlag == DOS_TRUE)
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

                if (g_ulUdpSocket > 0)
                {
                    lResult = sendto(g_ulUdpSocket, (VOID *)&stMsgDes, sizeof(PT_MSG_TAG), 0,  (struct sockaddr*)&pstPtcNode->stDestAddr, sizeof(pstPtcNode->stDestAddr));
                    if (lResult < 0)
                    {
                        close(g_ulUdpSocket);
                        g_ulUdpSocket = -1;
                        g_ulUdpSocket = pts_create_udp_socket(g_stPtsMsg.usPtsPort, PTS_SOCKET_CACHE);
                    }
                }
                pthread_mutex_unlock(&g_mutexPtcSendList);
                dos_dmem_free(pstNeedSendNode);
                pstNeedSendNode = NULL;
                continue;
            }

            pstStreamHead = &pstPtcNode->astDataTypes[pstNeedSendNode->enDataType].stStreamQueHead;
            if (NULL == pstStreamHead)
            {
                pthread_mutex_unlock(&g_mutexPtcSendList);
                pt_logr_debug("pts_send_msg2ptc : stream list is NULL");
                dos_dmem_free(pstNeedSendNode);
                pstNeedSendNode = NULL;
                continue;
            }

            pstStreamNode = pt_stream_queue_search(pstStreamHead, pstNeedSendNode->ulStreamID);
            if(NULL == pstStreamNode)
            {
                pthread_mutex_unlock(&g_mutexPtcSendList);
                pt_logr_debug("pts_send_msg2ptc : cann't found stream node");
                dos_dmem_free(pstNeedSendNode);
                pstNeedSendNode = NULL;
                continue;
            }

            pstSendDataHead = pstStreamNode->unDataQueHead.pstDataTcp;
            if (NULL == pstSendDataHead)
            {
                pthread_mutex_unlock(&g_mutexPtcSendList);
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
                        pt_logr_info("resend data to ptc, stream : %d, seq : %d, size : %d", pstNeedSendNode->ulStreamID, pstNeedSendNode->lSeqResend, stSendDataNode.ulLen);
                        if (g_ulUdpSocket > 0)
                        {
                            usleep(20);
                            lResult = sendto(g_ulUdpSocket, szBuff, stSendDataNode.ulLen + sizeof(PT_MSG_TAG), 0, (struct sockaddr*)&pstPtcNode->stDestAddr, sizeof(pstPtcNode->stDestAddr));
                            if (lResult < 0)
                            {
                                close(g_ulUdpSocket);
                                g_ulUdpSocket = -1;
                                g_ulUdpSocket = pts_create_udp_socket(g_stPtsMsg.usPtsPort, PTS_SOCKET_CACHE);
                            }
                        }
                        ulSendCount--;
                    }

                }
                dos_dmem_free(pstNeedSendNode);
                pstNeedSendNode = NULL;
                pthread_mutex_unlock(&g_mutexPtcSendList);
                continue;
            }

            while(1)
            {
                //gettimeofday(&stTime, NULL);
                //printf("start while time : %d\n", stTime.tv_sec * 1000 * 1000 + stTime.tv_usec);
                /* ����data��ֱ�������� */
                pstStreamNode->lCurrSeq++;
                ulArraySub = (pstStreamNode->lCurrSeq) & (PT_DATA_SEND_CACHE_SIZE - 1);
                stRecvDataTcp = pstSendDataHead[ulArraySub];
                if (stRecvDataTcp.lSeq == pstStreamNode->lCurrSeq)
                {
                    //stSendDataNode = pstSendDataHead[ulArraySub];
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
                    pt_logr_debug("send data to ptc, streamID is %d, seq is %d ", pstNeedSendNode->ulStreamID, pstStreamNode->lCurrSeq);
                    printf("send msg to ptc, stream : %d, seq : %d, len : %d, %s", pstNeedSendNode->ulStreamID, pstStreamNode->lCurrSeq, stRecvDataTcp.ulLen, pts_get_current_time());
                    //gettimeofday(&stTime, NULL);
                    //printf("before send time : %d\n", stTime.tv_sec * 1000 * 1000 + stTime.tv_usec);
                    if (g_ulUdpSocket > 0)
                    {
                        lResult = sendto(g_ulUdpSocket, szBuff, stRecvDataTcp.ulLen + sizeof(PT_MSG_TAG), 0, (struct sockaddr*)&pstPtcNode->stDestAddr, sizeof(pstPtcNode->stDestAddr));
                        if (lResult < 0)
                        {
                            close(g_ulUdpSocket);
                            g_ulUdpSocket = -1;
                            g_ulUdpSocket = pts_create_udp_socket(g_stPtsMsg.usPtsPort, PTS_SOCKET_CACHE);
                        }
                        usleep(20);
                    }
                    //gettimeofday(&stTime, NULL);
                    //printf("behind send time : %d\n", stTime.tv_sec * 1000 * 1000 + stTime.tv_usec);
                }
                else
                {
                    /* ���seqû�з��ͣ���һ */
                    pstStreamNode->lCurrSeq--;

                    //gettimeofday(&stTime, NULL);
                    //printf("end while time : %d\n", stTime.tv_sec * 1000 * 1000 + stTime.tv_usec);
                    break;
                }
            }
            pthread_mutex_unlock(&g_mutexPtcSendList);
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
    S32                 lRecvLen        = 0;
    S32                 lResult         = 0;
    U8                  *pcRecvBuf      = NULL;
    U32                 MaxFdp          = 0;
    struct sockaddr_in  stClientAddr;
    socklen_t           lCliaddrLen     = sizeof(stClientAddr);
    struct timeval      stTimeVal       = {1, 0};
    struct timeval      stTimeValCpy;
    fd_set              ReadFds;
    //PT_MSG_TAG         *pstMsgDes       = NULL;

    while(1)
    {
        if (g_ulUdpSocket <= 0)
        {
            sleep(1);
            continue;
        }

        stTimeValCpy = stTimeVal;
        FD_ZERO(&ReadFds);
        FD_SET(g_ulUdpSocket, &ReadFds);
        MaxFdp = g_ulUdpSocket;

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
        if (FD_ISSET(g_ulUdpSocket, &ReadFds))
        {
            lCliaddrLen = sizeof(stClientAddr);
            pcRecvBuf = (U8 *)dos_dmem_alloc(PT_SEND_DATA_SIZE);
            if (DOS_ADDR_INVALID(pcRecvBuf))
            {
                pt_logr_warning("recv msg from ptc malloc fail!");
                sleep(1);
                continue;
            }

            lRecvLen = recvfrom(g_ulUdpSocket, pcRecvBuf, PT_SEND_DATA_SIZE, 0, (struct sockaddr*)&stClientAddr, &lCliaddrLen);
            if (lRecvLen < 0)
            {
                pt_logr_info("recvfrom fail ,create socket again");
                dos_dmem_free(pcRecvBuf);
                pcRecvBuf = NULL;
                close(g_ulUdpSocket);
                g_ulUdpSocket = -1;
                g_ulUdpSocket = pts_create_udp_socket(g_stPtsMsg.usPtsPort, PTS_SOCKET_CACHE);

                continue;
            }

            //pstMsgDes = pcRecvBuf;
            //if (dos_ntohl(pstMsgDes->ulStreamID) > PT_CTRL_BUTT)
            //{
            //    printf("recv Stream: %u, Seq: %u\r\n", dos_ntohl(pstMsgDes->ulStreamID), dos_ntohl(pstMsgDes->lSeq));
            //}
            pthread_mutex_lock(&g_mutexPtsRecvMsgHandle);
            pts_recvfrom_ptc_buff_list_insert(&g_stMsgRecvFromPtc, pcRecvBuf, lRecvLen, stClientAddr);
            pthread_cond_signal(&g_condPtsRecvMsgHandle);
            pthread_mutex_unlock(&g_mutexPtsRecvMsgHandle);
        }

    } /* end of while(1) */
}

VOID *pts_handle_recvfrom_ptc_msg(VOID *arg)
{
    list_t *pstRecvBuffList = NULL;
    PTS_REV_MSG_HANDLE_ST *pstRecvBuffNode = NULL;
    PT_MSG_TAG         *pstMsgDes      = NULL;
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

        while(1)
        {

            pthread_mutex_lock(&g_mutexPtsRecvMsgHandle);

            if (dos_list_is_empty(&g_stMsgRecvFromPtc))
            {
                pthread_mutex_unlock(&g_mutexPtsRecvMsgHandle);
                break;
            }

            pstRecvBuffList = dos_list_fetch(&g_stMsgRecvFromPtc);
            if (DOS_ADDR_INVALID(pstRecvBuffList))
            {
                pthread_mutex_unlock(&g_mutexPtsRecvMsgHandle);

                DOS_ASSERT(0);
                continue;
            }
            pthread_mutex_unlock(&g_mutexPtsRecvMsgHandle);

            pstRecvBuffNode = dos_list_entry(pstRecvBuffList, PTS_REV_MSG_HANDLE_ST, stList);
            if (DOS_ADDR_INVALID(pstRecvBuffList))
            {
                pthread_mutex_unlock(&g_mutexPtsRecvMsgHandle);

                DOS_ASSERT(0);
                continue;
            }

            sem_wait(&g_SemPtsRecv);

            /* ȡ��ͷ����Ϣ */
            pstMsgDes = (PT_MSG_TAG *)pstRecvBuffNode->paRecvBuff;
            /* �ֽ���ת�� */
            pstMsgDes->ulStreamID = dos_ntohl(pstMsgDes->ulStreamID);
            pstMsgDes->lSeq = dos_ntohl(pstMsgDes->lSeq);

            if (pstMsgDes->enDataType == PT_DATA_CTRL)
            {
                /* ������Ϣ */
                pts_ctrl_msg_handle(g_ulUdpSocket, (S8 *)pstRecvBuffNode->paRecvBuff, pstRecvBuffNode->stClientAddr, pstRecvBuffNode->ulRecvLen);
                sem_post(&g_SemPtsRecv);
            }
            else
            {
                if (pstMsgDes->enCmdValue == PT_CMD_RESEND)
                {
                    /* pts���͵��ش����� */
                    BOOL bIsResend = DOS_TRUE;
                    PT_CMD_EN enCmdValue = PT_CMD_NORMAL;
                    printf("!!!!!!!!!!recv resend msg, stream : %d, seq : %d\n", pstMsgDes->ulStreamID, pstMsgDes->lSeq);
                    pthread_mutex_lock(&g_mutexPtsSendPthread);
                    pt_need_send_node_list_insert(&g_stPtsNendSendNode, pstMsgDes->aucID, pstMsgDes, enCmdValue, bIsResend);
                    pthread_cond_signal(&g_condPtsSend);
                    pthread_mutex_unlock(&g_mutexPtsSendPthread);

                    sem_post(&g_SemPtsRecv);
                }
                else if (pstMsgDes->enCmdValue == PT_CMD_CONFIRM)
                {
                    /* ȷ�Ͻ�����Ϣ */
                    printf("pts recv make sure, seq : %d\n", pstMsgDes->lSeq);
                    pt_logr_debug("pts recv make sure, seq : %d", pstMsgDes->lSeq);
                    lResult = pts_deal_with_confirm_msg(pstMsgDes);
                    if (lResult)
                    {
                        if (PT_DATA_WEB == pstMsgDes->enDataType)
                        {
                            pts_set_cache_full_false(pstMsgDes->ulStreamID);
                        }
                    }

                    sem_post(&g_SemPtsRecv);
                }
                else if (pstMsgDes->ExitNotifyFlag == DOS_TRUE)
                {
                    /* stream �˳� */
                    pt_logr_debug("pts recv exit msg, streamID = %d", pstMsgDes->ulStreamID);
                    printf("pts recv exit msg, streamID = %d\n", pstMsgDes->ulStreamID);
                    pts_delete_stream_addr_node(pstMsgDes->ulStreamID);
                    pts_delete_recv_stream_node(pstMsgDes);
                    pts_delete_send_stream_node(pstMsgDes);

                    if (pstMsgDes->ulStreamID != PT_CTRL_PTC_PACKAGE)
                    {
                        pthread_mutex_lock(&g_mutexPtsRecvPthread);
                        pt_need_recv_node_list_insert(&g_stPtsNendRecvNode, pstMsgDes);
                        pthread_cond_signal(&g_condPtsRecv);
                        pthread_mutex_unlock(&g_mutexPtsRecvPthread);
                    }
                    else
                    {
                        sem_post(&g_SemPtsRecv);
                    }
                }
                else
                {
                    pt_logr_debug("pts recv data from ptc, streamID = %d, seq : %d", pstMsgDes->ulStreamID, pstMsgDes->lSeq);
                    printf("pts recv data from ptc, streamID = %d, seq : %d\n", pstMsgDes->ulStreamID, pstMsgDes->lSeq);
                    lSaveIntoCacheRes = pts_save_into_recv_cache(pstMsgDes, (S8 *)pstRecvBuffNode->paRecvBuff + sizeof(PT_MSG_TAG), pstRecvBuffNode->ulRecvLen-sizeof(PT_MSG_TAG));
                    if (lSaveIntoCacheRes < 0)
                    {
                        sem_post(&g_SemPtsRecv);
                    }
                    else
                    {
                        pthread_mutex_lock(&g_mutexPtsRecvPthread);
                        if (NULL == pt_need_recv_node_list_search(&g_stPtsNendRecvNode, pstMsgDes->ulStreamID))
                        {
                            pt_need_recv_node_list_insert(&g_stPtsNendRecvNode, pstMsgDes);
                        }
                        pthread_cond_signal(&g_condPtsRecv);
                        pthread_mutex_unlock(&g_mutexPtsRecvPthread);
                    }
                }
            }

            dos_dmem_free(pstRecvBuffNode->paRecvBuff);
            pstRecvBuffNode->paRecvBuff = NULL;
            dos_dmem_free(pstRecvBuffNode);
            pstRecvBuffNode = NULL;

            if (lSaveIntoCacheRes == PT_NEED_CUT_PTHREAD)
            {
                /* �����̣߳�ִ�н��պ��� */
                usleep(10);
            }

       } /* end of while(1) */
    }
}


#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */

