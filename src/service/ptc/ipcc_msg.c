#ifdef  __cplusplus
extern "C"{
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include "ipcc_msg.h"

U32  g_ulStreamIDSend = 0;                                      /*����Stream ID*/
U32  g_ulStreamIDRecv = 0;                                      /*����Stream ID*/
BOOL g_bIsResend      = DOS_FALSE;                              /*�ط���flag*/
BOOL g_bIsResendReq   = DOS_FALSE;                              /*�ط�����flag*/
S32  g_lSeqSend       = 0;                                      /*�ط������*/
pthread_mutex_t g_mutex_send = PTHREAD_MUTEX_INITIALIZER;       /*�����߳���*/
pthread_cond_t  g_cond_send  = PTHREAD_COND_INITIALIZER;        /*������������*/
pthread_mutex_t g_mutex_recv = PTHREAD_MUTEX_INITIALIZER;       /*�����߳���*/
pthread_cond_t  g_cond_recv  = PTHREAD_COND_INITIALIZER;        /*������������*/

PT_CC_CB_ST *g_stCCCBSend = NULL;                               /**/
PT_CC_CB_ST *g_stCCCBRecv = NULL;                               /**/
PTC_SERV_MSG_ST  g_stServMsg;
PT_DATA_TYPE_EN g_enDataTypeSend;                               /*����msg����*/
PT_DATA_TYPE_EN g_enDataTypeRecv;                               /*����msg����*/

void ptc_data_lose(PT_MSG_ST *pstMsgDes, S32 lShouldSeq)
{
    if (NULL == pstMsgDes)
    {
        alert("arg is NULL");
        return;
    }
    pthread_mutex_lock(&g_mutex_send);
    g_bIsResend = DOS_FALSE;
    g_bIsResendReq = DOS_TRUE;
    g_enDataTypeSend = pstMsgDes->enDataType;
    g_ulStreamIDSend = pstMsgDes->ulStreamID;
    g_lSeqSend = lShouldSeq;

    pthread_cond_signal(&g_cond_send);
    pthread_mutex_unlock(&g_mutex_send);

    return;
}

void ptc_send_lose_data_msg_callback(U64 ulLoseMsg)
{
    if (0 == ulLoseMsg)
    {
        return;
    }
    PT_LOSE_BAG_MSG_ST *pstLoseMsg = (PT_LOSE_BAG_MSG_ST *)ulLoseMsg;
    PT_MSG_ST *pstMsg = pstLoseMsg->stMsg;
    PT_STREAM_CB_ST *pstStreamNode = pstLoseMsg->stStreamNode;

    S32 i = 0;
    U32 ulCount = 0;
    U32 ulArraySub = 0;

    if (pstStreamNode->ulCountResend == 3)
    {
        /*�ر�sockfd*/
    }
    else
    {
        pstStreamNode->ulCountResend++;
        for (i=pstStreamNode->ulCurrSeq+1; i<pstStreamNode->ulMaxSeq; i++)
        {
            ulArraySub = i & (PT_DATA_SEND_CACHE_SIZE - 1);
            if (pstStreamNode->unDataQueHead.pstDataTcp[ulArraySub].bIsSaveMsg != DOS_TRUE ||
                (pstStreamNode->unDataQueHead.pstDataTcp[ulArraySub].bIsSaveMsg == DOS_TRUE &&
                pstStreamNode->unDataQueHead.pstDataTcp[ulArraySub].lSeq <= pstStreamNode->ulCurrSeq))
            {
                ulCount++;
                pstStreamNode->ulCountResend++;
                /*���Ͷ����ط�����*/
                ptc_data_lose(pstMsg, ulArraySub);
            }
        }
    }

    if (0 == ulCount)
    {
        /*û����Ҫ���͵Ķ����ط����󣬹رն�ʱ��*/
        dos_tmr_stop(&pstStreamNode->hTmrHandle);
        pstStreamNode->ulCountResend = 0;
    }
}

S32 ptc_key_convert(S8 *szKey, S8 *szDest)
{
    if (NULL == szKey || NULL == szDest)
    {

        return DOS_FAIL;
    }

    S32 i = 0;

    for (i=0; i<PTC_LOGIN_VERIFY_SIZE-1; i++)
    {
        szDest[i] = szKey[i]&0xA9;
    }
    szDest[PTC_LOGIN_VERIFY_SIZE] = '\0';

    return DOS_SUCC;
}

/*----------------------- public  function ------------------------*/


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
void ptc_ipcc_send_msg(PT_DATA_TYPE_EN enDataType, U32 ulStreamID, S8 *pcData, S32 lDataLen, U8 ExitNotifyFlag)
{
    list_t           *pstStreamListHead  = NULL;
    list_t           *pstStreamListNode  = NULL;
    PT_STREAM_CB_ST  *pstStreamNode      = NULL;
    S32              lDataSeq            = 0;

    if (enDataType < 0 || enDataType > 2)
    {
        alert("Data Type should in 0-2: %d", enDataType);
        return;
    }
    else if (NULL == pcData)
    {
        alert("data pointer is NULL");
        return;
    }

    pthread_mutex_lock(&g_mutex_send);

    if (NULL == g_stCCCBSend)
    {
        /*??*/
    }
    pstStreamListHead = g_stCCCBSend->astDataTypes[enDataType].pstStreamQueHead;
    if (NULL == pstStreamListHead)
    {
        /*stream�������ڣ���Ҫ�½�*/
        alert("pstStreamListHead is NULL");
        /*��չ:����enDataType���жϴ���UDP�����ݻ��棬����TCP�ģ�����ֻ����TCP*/
        pstStreamListHead = pt_create_stream_and_data_que(ulStreamID, pcData, lDataLen, ExitNotifyFlag, 0, PT_DATA_SEND_CACHE_SIZE);
        if (NULL == pstStreamListHead)
        {
            /*����ʧ��*/
            goto ptc_send_lock;
        }
        else
        {
            g_stCCCBSend->astDataTypes[enDataType].pstStreamQueHead = pstStreamListHead;
        }
    }
    else
    {
        pstStreamNode = pt_stream_queue_serach(pstStreamListHead, ulStreamID);
        if (NULL == pstStreamNode)
        {
            /*δ�ҵ�stram node����Ҫ�½�*/
            pstStreamListNode = pt_create_stream_and_data_que(ulStreamID, pcData, lDataLen, ExitNotifyFlag, 0, PT_DATA_SEND_CACHE_SIZE);
            if (NULL == pstStreamListNode)
            {
                /*����ʧ��*/
                goto ptc_send_lock;
            }
            else
            {
                pstStreamListHead = pt_stream_queue_insert(pstStreamListHead, pstStreamListNode);
                if (NULL == pstStreamListHead)
                {
                    alert("insert stream queue fail");
                    pstStreamNode = list_entry(pstStreamListHead, PT_STREAM_CB_ST, stStreamListNode);
                    free(pstStreamNode->unDataQueHead.pstDataTcp);
                    free(pstStreamNode);
                    goto ptc_send_lock;
                }
            }
        }
        else
        {
            pstStreamNode->ulMaxSeq = pstStreamNode->ulMaxSeq + 1;
            lDataSeq = pstStreamNode->ulMaxSeq;

            if (pt_send_data_tcp_queue_add(pstStreamNode, ExitNotifyFlag, pcData, lDataLen, lDataSeq, PT_DATA_SEND_CACHE_SIZE))
            {
                alert("add data queue fail");
                /*��ô����???*/
                goto ptc_send_lock;
            }
        }
    }

    g_enDataTypeSend = enDataType;
    g_ulStreamIDSend = ulStreamID;
    g_bIsResend = DOS_FALSE;
    g_bIsResendReq = DOS_FALSE;
    pthread_cond_signal(&g_cond_send);
ptc_send_lock: pthread_mutex_unlock(&g_mutex_send);
}


/**
 * ������void *ptc_send_msg2amc(void *arg)
 * ���ܣ�
 *      1.���������߳�
 * ����
 *      void *arg :ͨ��ͨ�ŵ�sockfd
 * ����ֵ��void *
 */
void *ptc_send_msg2amc(void *arg)
{
    S32              lSockfd         = *(S32 *)arg;
    S32              lSeq            = 0;
    U32              ulArraySub      = 0;
    U32              ulSendCount     = 0;
    list_t           *pstStreamHead  = NULL;
    PT_STREAM_CB_ST  *pstStreamNode  = NULL;
    PT_DATA_TCP_ST   stSendDataNode;
    PT_MSG_ST        stMsgDes;
    S8 acBuff[PT_SEND_DATA_SIZE]     = {0};

    while (1)
    {
        pthread_mutex_lock(&g_mutex_send);
        pthread_cond_wait(&g_cond_send, &g_mutex_send);

        memset(&stMsgDes, 0, sizeof(PT_MSG_ST));
        if (g_bIsResendReq)
        {
            /*���Ͷ������ط�����*/
            stMsgDes.enDataType = g_enDataTypeSend;
            memcpy(stMsgDes.aucID, g_stCCCBSend->aucID, IPCC_ID_LEN);
            stMsgDes.ulStreamID = g_ulStreamIDSend;
            stMsgDes.ExitNotifyFlag = DOS_FALSE;
            stMsgDes.lSeq = g_lSeqSend;
            stMsgDes.bIsResendReq = DOS_TRUE;
            stMsgDes.bIsEncrypt = DOS_FALSE;
            stMsgDes.bIsCompress = DOS_FALSE;

            alert("lose data bag request");
            sendto(lSockfd, (void *)&stMsgDes, sizeof(PT_MSG_ST), 0,  (struct sockaddr*)&g_stCCCBSend->stDestAddr, sizeof(g_stCCCBSend->stDestAddr));
        }
        else
        {
            /*�������ݰ�*/
            if (NULL == g_stCCCBSend)
            {
                /*??*/
            }
            pstStreamHead = g_stCCCBSend->astDataTypes[g_enDataTypeSend].pstStreamQueHead;
            if (NULL == pstStreamHead)
            {
                alert("not found data");
            }
            else
            {
                pstStreamNode = pt_stream_queue_serach(pstStreamHead, g_ulStreamIDSend);
                if(NULL == pstStreamNode)
                {
                    alert("not found data");
                }
                else
                {
                    if (g_bIsResend)
                    {
                        /*Ҫ���ش��İ�*/
                        lSeq = g_lSeqSend;
                    }
                    else
                    {
                        lSeq = pstStreamNode->ulCurrSeq + 1;
                        pstStreamNode->ulCurrSeq = lSeq;
                    }

                    if (NULL == pstStreamNode->unDataQueHead.pstDataTcp)
                    {
                        alert("not found data");
                    }
                    else if (lSeq < pstStreamNode->ulMinSeq || lSeq > pstStreamNode->ulMaxSeq)
                    {
                        alert("data bag not exit");
                    }
                    else
                    {
                        /*���࣬�ҳ�lSeq�������±�*/
                        ulArraySub = lSeq&(PT_DATA_SEND_CACHE_SIZE - 1);
                        stSendDataNode = pstStreamNode->unDataQueHead.pstDataTcp[ulArraySub];

                        stMsgDes.enDataType = g_enDataTypeSend;
                        memcpy(stMsgDes.aucID, g_stCCCBSend->aucID, IPCC_ID_LEN);
                        stMsgDes.ulStreamID = g_ulStreamIDSend;
                        stMsgDes.ExitNotifyFlag = stSendDataNode.ExitNotifyFlag;
                        stMsgDes.lSeq = lSeq;
                        stMsgDes.bIsResendReq = DOS_FALSE;
                        stMsgDes.bIsEncrypt = DOS_FALSE;
                        stMsgDes.bIsCompress = DOS_FALSE;
                        stMsgDes.aulServIp[0] = g_stServMsg.achProxyIP[0];
                        stMsgDes.usServPort = g_stServMsg.usProxyPort;

                        alert("send data size:%d", stSendDataNode.ulLen);
                        memcpy(acBuff, (void *)&stMsgDes, sizeof(PT_MSG_ST));
                        memcpy(acBuff+sizeof(PT_MSG_ST), stSendDataNode.szBuff, stSendDataNode.ulLen);
                        if (g_bIsResend)
                        {
                            ulSendCount = PT_RESEND_COUNT;    /*�ش��ģ���������*/
                        }
                        else
                        {
                            ulSendCount = 1;
                        }
                        while (ulSendCount)
                        {
                            sendto(lSockfd, acBuff, stSendDataNode.ulLen + sizeof(PT_MSG_ST), 0, (struct sockaddr*)&g_stCCCBSend->stDestAddr, sizeof(g_stCCCBSend->stDestAddr));
                            ulSendCount--;
                        }

                    }
                }
            }
        }
        pthread_mutex_unlock(&g_mutex_send);
    }

    pthread_mutex_destroy(&g_mutex_send);
    pthread_cond_destroy(&g_cond_send);
}

/**
 * ������void *ptc_recv_msg_from_amc(void *arg)
 * ���ܣ�
 *      1.���������߳�
 * ����
 *      void *arg :ͨ��ͨ�ŵ�sockfd
 * ����ֵ��void *
 */
void *ptc_recv_msg_from_amc(void *arg)
{
    S32                lSockfd                         = *(S32*)arg;
    S8                 acRecvBuf[PT_SEND_DATA_SIZE]    = {0};
    S32                lDataLen                        = 0;
    S32                lResult                         = 0;
    PT_MSG_ST          *pstMsgDes                      = NULL;
    list_t             *pstStreamListHead              = NULL;
    list_t             *pstStreamListNode              = NULL;
    PT_STREAM_CB_ST    *pstStreamNode                  = NULL;
    PT_LOSE_BAG_MSG_ST *stLoseMsg                      = NULL;
    S32                lShouldSeq                      = 0;
    struct sockaddr_in stClientAddr;
    socklen_t          ulCliaddrLen                     = sizeof(stClientAddr);
    S8                 szDestKey[PTC_LOGIN_VERIFY_SIZE] = {0};
	S8 				   acMsgHead[PTC_DATA_BUFF_512]     = {0};           


    while (1)
    {
        lDataLen = recvfrom(lSockfd, acRecvBuf, PT_SEND_DATA_SIZE, 0, (struct sockaddr*)&stClientAddr, &ulCliaddrLen);
        if (lDataLen < 0)
        {
            perror("recvfrom");
            continue;
        }
        /*�ж��Ƿ�����AMC*/
    //    else if (stClientAddr != g_stCCCBSend->stDestAddr)
    //    {
    //        alert("%s %d  recv other dest msg\n", __FILE__, __LINE__);
    //        continue;
    //    }

        alert("recv size:%d\n", lDataLen);
        pthread_mutex_lock(&g_mutex_recv);

        /*ȡ����������*/
        memcpy(acMsgHead, acRecvBuf, sizeof(PT_MSG_ST));
        pstMsgDes = (PT_MSG_ST *)acMsgHead;

        g_enDataTypeRecv = pstMsgDes->enDataType;
        g_ulStreamIDRecv = pstMsgDes->ulStreamID;

        if (pstMsgDes->enDataType == PT_DATA_CTRL)
        {
            /*������Ϣ*/
            switch (pstMsgDes->ulStreamID)
            {
                case 1:
                    /*��½��֤*/
                    lResult = ptc_key_convert(acRecvBuf + sizeof(PT_MSG_ST), szDestKey);
                    if (lResult < 0)
                    {
                         break;
                    }
                    /*����key��pts*/

                    ptc_ipcc_send_msg(PT_DATA_CTRL, 2, szDestKey, PTC_LOGIN_VERIFY_SIZE, DOS_FALSE);
                    break;
                case 2:
                    /*��½���*/
                    printf("��½���:%s\n", acRecvBuf + sizeof(PT_MSG_ST));
                    break;
                default:
                    break;
            }
        }
        else if (pstMsgDes->bIsResendReq)
        {
            /*�ش�����*/
            pthread_mutex_lock(&g_mutex_send);
            g_bIsResend = DOS_TRUE;
            g_bIsResendReq = DOS_FALSE;
            g_enDataTypeSend = pstMsgDes->enDataType;
            g_ulStreamIDSend = pstMsgDes->ulStreamID;
            g_lSeqSend = pstMsgDes->lSeq;
            pthread_cond_signal(&g_cond_send);
            pthread_mutex_unlock(&g_mutex_send);

            goto ptc_recv_lock;
        }
        else
        {
            if (NULL == g_stCCCBRecv)
            {
                /*??*/
            }
            pstStreamListHead = g_stCCCBRecv->astDataTypes[pstMsgDes->enDataType].pstStreamQueHead;
            if (NULL == pstStreamListHead)
            {
                pstStreamListHead = pt_create_stream_and_data_que(pstMsgDes->ulStreamID, acRecvBuf, lDataLen, pstMsgDes->ExitNotifyFlag, 0, PT_DATA_RECV_CACHE_SIZE);
                if (NULL == pstStreamListHead)
                {
                    /*����ʧ��*/
                    goto ptc_recv_lock;
                }
                else
                {
                    g_stCCCBSend->astDataTypes[pstMsgDes->enDataType].pstStreamQueHead = pstStreamListHead;
                }

                if (pstMsgDes->lSeq != 0)
                {
                    /*�½�һ��stramʱ����һ��data�ı��Ӧ��Ϊ0�����Ͷ�����Ϣ,������ʱ��*/
                    pstStreamNode = list_entry(pstStreamListHead, PT_STREAM_CB_ST, stStreamListNode);
                    stLoseMsg = (PT_LOSE_BAG_MSG_ST *)malloc(sizeof(PT_LOSE_BAG_MSG_ST));
                    if (NULL == stLoseMsg)
                    {
                        perror("malloc");
                        return NULL;
                    }

                    stLoseMsg->stMsg = pstMsgDes;
                    stLoseMsg->stStreamNode = pstStreamNode;

                    ptc_send_lose_data_msg_callback((U64)stLoseMsg);

                    lResult = dos_tmr_start(&pstStreamNode->hTmrHandle, PT_SEND_LOSE_DATA_TIMER, ptc_send_lose_data_msg_callback, (U64)stLoseMsg, TIMER_NORMAL_LOOP);
                    if (DOS_FAIL == lResult)
                    {
                        alert("start timer fail");
                        /*�ͷ���Դ!!!!!*/
                        return NULL;
                    }
                    goto ptc_recv_lock;
                }

            }
            else
            {
                pstStreamNode = pt_stream_queue_serach(pstStreamListHead, pstMsgDes->ulStreamID);
                if (NULL == pstStreamNode)
                {
                    pstStreamListNode = pt_create_stream_and_data_que(pstMsgDes->ulStreamID, acRecvBuf, lDataLen, pstMsgDes->ExitNotifyFlag, 0, PT_DATA_RECV_CACHE_SIZE);
                    if (NULL == pstStreamListNode)
                    {
                        /*����ʧ��*/
                        goto ptc_recv_lock;
                    }
                    else
                    {
                        pstStreamListHead = pt_stream_queue_insert(pstStreamListHead, pstStreamListNode);
                        if (NULL == pstStreamListHead)
                        {
                            alert("insert stream queue fail");
                            /*��Դ�ͷ� !!!!!!*/
                            goto ptc_recv_lock;
                        }
                    }

                    if (pstMsgDes->lSeq != 0)
                    {
                        /*����*/
                        pstStreamNode = list_entry(pstStreamListHead, PT_STREAM_CB_ST, stStreamListNode);
                        stLoseMsg = (PT_LOSE_BAG_MSG_ST *)malloc(sizeof(PT_LOSE_BAG_MSG_ST));
                        if (NULL == stLoseMsg)
                        {
                            perror("malloc");
                            return NULL;
                        }

                        stLoseMsg->stMsg = pstMsgDes;
                        stLoseMsg->stStreamNode = pstStreamNode;
                        ptc_send_lose_data_msg_callback((U64)stLoseMsg);

                        lResult = dos_tmr_start(&pstStreamNode->hTmrHandle, PT_SEND_LOSE_DATA_TIMER, ptc_send_lose_data_msg_callback, (U64)stLoseMsg, TIMER_NORMAL_LOOP);
                        if (DOS_FAIL == lResult)
                        {
                            alert("start timer fail\n");
                            /*�ͷ���Դ!!!!!*/
                            return NULL;
                        }
                        goto ptc_recv_lock;
                    }
                }
                else
                {
                    pstStreamNode->ulMaxSeq = pstStreamNode->ulMaxSeq + 1;
                    lShouldSeq = pstStreamNode->ulMaxSeq;

                    if (((pstMsgDes->lSeq - pstStreamNode->ulCurrSeq) & (PT_DATA_RECV_CACHE_SIZE - 1)) == 0)
                    {
                        /*�����һȦ,����*/
                        /*�ر�sockfd*/
                        goto ptc_recv_lock;
                    }

                    if (pt_send_data_tcp_queue_add(pstStreamNode, pstMsgDes->ExitNotifyFlag, acRecvBuf, lDataLen, lShouldSeq, PT_DATA_RECV_CACHE_SIZE))
                    {
                        alert("add data queue fail");
                        /*��Դ�ͷ� !!!!!!*/
                        goto ptc_recv_lock;
                    }

                    if (pstMsgDes->lSeq != lShouldSeq)
                    {
                        /*������ !!!!*/
                        if (NULL == pstStreamNode->hTmrHandle)
                        {
                            stLoseMsg = (PT_LOSE_BAG_MSG_ST *)malloc(sizeof(PT_LOSE_BAG_MSG_ST));
                            if (NULL == stLoseMsg)
                            {
                                perror("malloc");
                                return NULL;
                            }
                            pstStreamNode->ulCountResend = 0;

                            stLoseMsg->stMsg = pstMsgDes;
                            stLoseMsg->stStreamNode = pstStreamNode;

                            /*û�ж�ʱ���ģ�������ʱ��*/
                            lResult = dos_tmr_start(&pstStreamNode->hTmrHandle, PT_SEND_LOSE_DATA_TIMER, ptc_send_lose_data_msg_callback, (U64)stLoseMsg, TIMER_NORMAL_LOOP);
                            if (DOS_FAIL == lResult)
                            {
                                alert("start timer fail");
                                /*�ͷ���Դ!!!!!*/
                                return NULL;
                            }
                        }
                        goto ptc_recv_lock;
                    }
                    else
                    {
                        /*��С�İ���ʧ�İ��ѽ��յ������ø���*/
                        pstStreamNode->ulCountResend = 0;
                    }
                }

            }
        }
        pthread_cond_signal(&g_cond_recv);
ptc_recv_lock:  pthread_mutex_unlock(&g_mutex_recv);
    }
}

#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */