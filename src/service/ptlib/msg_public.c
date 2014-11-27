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
#include <ptc_pts/msg_public.h>

PT_PROT_TYPE_EN  g_aenDataProtType[PT_DATA_BUTT] = {PT_TCP, PT_TCP, PT_TCP};

PT_DATA_TCP_ST *pt_data_tcp_queue_create(U32 ulCacheSize)
{
    PT_DATA_TCP_ST *pstHead = NULL;

    pstHead = (PT_DATA_TCP_ST *)malloc(sizeof(PT_DATA_TCP_ST) * ulCacheSize);
    if (NULL == pstHead)
    {
        perror("malloc");
        return NULL;
    }
    else
    {
        return pstHead;
    }
}

BOOL pt_send_data_tcp_queue_add(PT_STREAM_CB_ST* pstStreamNode, U8 ExitNotifyFlag, S8 *pcData, S32 lDataLen, S32 lSeq, U32 ulCacheSize)
{
    if (NULL == pcData || NULL == pstStreamNode || NULL == pstStreamNode->unDataQueHead.pstDataTcp)
    {
    alert("pointer arg is NULL\n");
    return DOS_FAIL;
    }

    U32 ulArraySub = lSeq & (ulCacheSize - 1);  /*������*/
    PT_DATA_TCP_ST* pstDataQueHead = pstStreamNode->unDataQueHead.pstDataTcp;

    if (pstDataQueHead[ulArraySub].bIsSaveMsg != DOS_TRUE)
    {
        memcpy(pstDataQueHead[ulArraySub].szBuff, pcData, sizeof(pstDataQueHead[ulArraySub].szBuff));
    }
    else
    {
        if (pstDataQueHead[ulArraySub].lSeq == lSeq)
        {
            /*�Ѵ���*/
            return DOS_SUCC;
        }
        else
        {
            /*���ڱ�����ݰ�*/
            pstStreamNode->ulMinSeq = pstDataQueHead->lSeq + 1;    /*���»����е���С���*/
            memset(pstDataQueHead[ulArraySub].szBuff, 0, PT_RECV_DATA_SIZE);
            memcpy(pstDataQueHead[ulArraySub].szBuff, pcData, lDataLen);
        }
    }

    pstStreamNode->ulMaxSeq = pstStreamNode->ulMaxSeq > lSeq ? pstStreamNode->ulMaxSeq : lSeq;
    pstDataQueHead[ulArraySub].lSeq = lSeq;
    pstDataQueHead[ulArraySub].ulLen = lDataLen;
    pstDataQueHead[ulArraySub].ExitNotifyFlag = ExitNotifyFlag;
    pstDataQueHead[ulArraySub].bIsSaveMsg = DOS_TRUE;

    return DOS_SUCC;
}

PT_STREAM_CB_ST *pt_stream_node_create(U32 ulStreamID, PT_DATA_QUE_HEAD_UN unDataQueHead, S32 lSeq)
{
    if (NULL == unDataQueHead.pstDataTcp)
    {
        alert("arg pointer is NULL");
        return NULL;
    }

    PT_STREAM_CB_ST *stStreamNode = NULL;
    list_t *pstHead = NULL;

    stStreamNode = (PT_STREAM_CB_ST *)malloc(sizeof(PT_STREAM_CB_ST));
    if (NULL == stStreamNode)
    {
        perror("malloc");
        return NULL;
    }

    stStreamNode->unDataQueHead = unDataQueHead;
    stStreamNode->ulStreamID = ulStreamID;
    stStreamNode->ulMaxSeq = lSeq;
    stStreamNode->ulMinSeq = lSeq;
    stStreamNode->ulCurrSeq = lSeq - 1;
    stStreamNode->lSockFd = 0;
    stStreamNode->hTmrHandle = NULL;
    stStreamNode->ulCountResend = 0;

    pstHead = &(stStreamNode->stStreamListNode);
    list_init(pstHead);

    return stStreamNode;
}

/**
 * ������list_t *pt_stream_queue_insert(list_t *pstHead, list_t  *pstStreamNode)
 * ���ܣ�
 *      1.�����в����µĽڵ�
 * ����
 *      list_t *pstHead         ��streamͷ�ڵ�
 *      list_t  *pstStreamNode  ����Ҫ����Ľڵ�
 * ����ֵ������ͷ�ڵ�
 */
list_t *pt_stream_queue_insert(list_t *pstHead, list_t  *pstStreamNode)
{
    if (NULL == pstStreamNode)
    {
        alert("arg error\n");
        return NULL;
    }

    if (NULL == pstHead)
    {
        pstHead = pstStreamNode;
        list_init(pstHead);
    }
    else
    {
        list_add_tail(pstHead, pstStreamNode);
    }

    return pstHead;
}


/**
 * ������PT_STREAM_CB_ST *pt_stream_queue_serach(list_t *pstStreamListHead, U32 ulStreamID)
 * ���ܣ�
 *      1.��stream�����У�����stream IDΪulStreamID�Ľڵ�
 * ����
 *      list_t *pstStreamListHead    ��streamͷ���
 *      U32 ulStreamID                      ����Ҫ���ҵ�stream ID
 * ����ֵ���ҵ�������stream node��ַ��δ�ҵ�����NULL
 */
PT_STREAM_CB_ST *pt_stream_queue_serach(list_t *pstStreamListHead, U32 ulStreamID)
{
    if (NULL == pstStreamListHead)
    {
        alert("empty list!\n");
        return NULL;
    }

    list_t   *pstNode = NULL;
    PT_STREAM_CB_ST *pstData = NULL;

    pstNode = pstStreamListHead;
    pstData = list_entry(pstNode, PT_STREAM_CB_ST, stStreamListNode);

    while (pstData->ulStreamID != ulStreamID && pstNode->next != pstStreamListHead)
    {
        pstNode = pstNode->next;
        pstData = list_entry(pstNode, PT_STREAM_CB_ST, stStreamListNode);
    }

    if (pstData->ulStreamID == ulStreamID)
    {
        return pstData;
    }
    else
    {
        alert("not found!");
        return NULL;
    }
}

PT_STREAM_CB_ST *pt_stream_queue_serach_by_sockfd(PT_CC_CB_ST *pstCCCB, S32 lSockFd)
{
    if (NULL == pstCCCB)
    {
        alert("empty list!");
        return NULL;
    }

    list_t  *pstStreamListHead = NULL;
    list_t  *pstNode = NULL;
    PT_STREAM_CB_ST   *pstData = NULL;
    S32 i = 0;

    for (i=0; i<PT_DATA_BUTT; i++)
    {
        pstStreamListHead = pstCCCB->astDataTypes[i].pstStreamQueHead;

        if (NULL == pstStreamListHead)
        {
            continue;
        }

        pstNode = pstStreamListHead;
        pstData = list_entry(pstNode, PT_STREAM_CB_ST, stStreamListNode);

        while (pstData->lSockFd != lSockFd && pstNode->next != pstStreamListHead)
        {
            pstNode = pstNode->next;
            pstData = list_entry(pstNode, PT_STREAM_CB_ST, stStreamListNode);
        }

        if (pstData->lSockFd == lSockFd)
        {
            return pstData;
        }
        else
        {
            continue;
        }
    }

    alert("empty list!");
    return NULL;
}


/**
 * ������list_t *pt_create_stream_and_data_que(U32 ulStreamID, S8 *pcData, S32 lDataLen, U8 ExitNotifyFlag, S32 lSeq, U32 ulCacheSize)
 * ���ܣ�
 *      1.��������ΪulCacheSize�����ݻ���
 *      2.����stream�ڵ㣬�������ַ��ӵ��ڵ���
 *      3.��ӱ��ΪlSeq�İ�������
 * ����
 *      U32 ulStreamID    ��stream ID
 *      S8  *pcData       ��������
 *      S32 lDataLen      �������ݳ���
 *      U8  ExitNotifyFlag��֪ͨ�Է���Ӧ�Ƿ����
 *      S32 lSeq          ����Ҫ�����İ��ı��
 *      U32 ulCacheSize   �����ݻ���Ĵ�С
 * ����ֵ���ɹ�����stream node��ַ��ʧ�ܷ���NULL
 */
list_t *pt_create_stream_and_data_que(U32 ulStreamID, S8 *pcData, S32 lDataLen, U8 ExitNotifyFlag, S32 lSeq, U32 ulCacheSize)
{
    if (NULL == pcData)
    {
        alert("arg error\n");
        return NULL;
    }

    PT_DATA_TCP_ST      *pstTcpDataQueHead  = NULL;
    PT_STREAM_CB_ST     *pstStreamNode      = NULL;
    PT_DATA_QUE_HEAD_UN  unDataQueHead;

    unDataQueHead.pstDataTcp = NULL;
    pstTcpDataQueHead = pt_data_tcp_queue_create(ulCacheSize);
    if (NULL == pstTcpDataQueHead)
    {
        alert("create data queue fail\n");
        return NULL;
    }

    unDataQueHead.pstDataTcp = pstTcpDataQueHead;
    pstStreamNode = pt_stream_node_create(ulStreamID, unDataQueHead, lSeq);
    if (NULL == pstStreamNode)
    {
        alert("create stream queue fail\n");
        free(pstTcpDataQueHead);
        return NULL;
    }

    if (pt_send_data_tcp_queue_add(pstStreamNode, ExitNotifyFlag, pcData, lDataLen, lSeq, ulCacheSize))
    {
        alert("add data queue fail\n");
        free(pstTcpDataQueHead);
        free(pstStreamNode);
        return NULL;
    }

    return &(pstStreamNode->stStreamListNode);

}

/*�ͷ�һ��stream*/
list_t *pt_delete_stream_resource(list_t *pstStreamListHead, list_t *pstStreamListNode, PT_DATA_TYPE_EN enDataType)
{
    if (NULL == pstStreamListHead || NULL == pstStreamListNode || enDataType >= PT_DATA_BUTT)
    {
        alert("arg error\n");
        return pstStreamListHead;
    }

    PT_STREAM_CB_ST *pstStreamNode =  list_entry(pstStreamListNode, PT_STREAM_CB_ST, stStreamListNode);

    if (pstStreamListHead == pstStreamListNode)
    {
        /*�ͷŵ�һ���ڵ㣬ͷ��㷢���ı�*/
        if (pstStreamListHead == pstStreamListHead->next)
        {
            pstStreamListHead = NULL;
        }
        else
        {
            pstStreamListHead = pstStreamListNode->next;
        }
    }

    list_del(pstStreamListNode);
    if (g_aenDataProtType[enDataType] == PT_UDP)
    {
        /*��Ҫ�ͷŴ洢data�Ŀռ�*/
    }
    else
    {
        list_del(pstStreamListNode);
        free(pstStreamNode);
    }

    return pstStreamListHead;
}

/*�ͷ�һ��IPCC*/
list_t *pt_delete_CC_resource(list_t *stCCCBRecv, PT_CC_CB_ST *pstCCNode)
{
    if (NULL == stCCCBRecv || NULL == pstCCNode)
    {
        alert("arg error\n");
        return stCCCBRecv;
    }

    S32 i = 0;
    list_t *pstStreamListHead = NULL;

    /*�ͷŵ�cc�����е�stream*/
    for (i=0; i<PT_DATA_BUTT; i++)
    {
        pstStreamListHead = pstCCNode->astDataTypes[i].pstStreamQueHead;
        if (pstStreamListHead != NULL)
        {
            while (pstStreamListHead->next != pstStreamListHead)
            {
                pstStreamListHead = pt_delete_stream_resource(pstStreamListHead, pstStreamListHead, i);
            }

            pt_delete_stream_resource(pstStreamListHead, pstStreamListHead, i);
        }
    }

    /*�ͷŵ�IPCC*/
	if (stCCCBRecv == &pstCCNode->stCCListNode)
	{
		/*ɾ��ͷ*/
		if (stCCCBRecv->next == stCCCBRecv)
		{
			stCCCBRecv = NULL;
		}
		else
		{
			stCCCBRecv = stCCCBRecv->next;
		}
	}
    list_del(&pstCCNode->stCCListNode);
	free(pstCCNode);

	return stCCCBRecv;
}

#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */
