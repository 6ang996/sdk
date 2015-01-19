/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  �ļ���
 *
 *  ����ʱ��:
 *  ��    ��:
 *  ��    ��:
 *  �޸���ʷ:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>
#include <bs_pub.h>
#include "bs_cdr.h"
#include "bs_stat.h"
#include "bs_def.h"



/* ���ݲ㷢�ͱ�����Ӧ��ҵ��� */
VOID bsd_send_walk_rsp2sl(DLL_NODE_S *pMsgNode, S32 lReqRet)
{
    BS_INTER_MSG_WALK   *pstMsg;

    /* �����·�������ڵ�,����������Ϣ�ڵ� */
    pstMsg = (BS_INTER_MSG_WALK *)pMsgNode->pHandle;
    pstMsg->stMsgTag.dstTid = pstMsg->stMsgTag.srcTid;
    pstMsg->stMsgTag.srcTid = pthread_self();
    pstMsg->stMsgTag.ucMsgType = BS_INTER_MSG_WALK_RSP;
    pstMsg->stMsgTag.lInterErr = lReqRet;

    bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG,
             "Send walk rsp msg to sl, type:%u, len:%u, errcode:%d, tbl:%d",
             pstMsg->stMsgTag.ucMsgType,
             pstMsg->stMsgTag.usMsgLen,
             pstMsg->stMsgTag.lInterErr,
             pstMsg->ulTblType);

    /* ����Ϣ�ڵ�ҵ�D2S������ */
    pthread_mutex_lock(&g_mutexBSD2SMsg);
    DLL_Add(&g_stBSD2SMsgList, pMsgNode);
    pthread_cond_signal(&g_condBSD2SList);
    pthread_mutex_unlock(&g_mutexBSD2SMsg);

}

/* ҵ��㷢�ͱ����������ݲ� */
VOID bss_send_walk_req2dl(U32 ulTblType)
{
    DLL_NODE_S          *pstMsgNode = NULL;
    BS_INTER_MSG_WALK   *pstTblWalkMsg = NULL;

    pstMsgNode = dos_dmem_alloc(sizeof(DLL_NODE_S));
    if (NULL == pstMsgNode)
    {
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
        return;
    }
    DLL_Init_Node(pstMsgNode);

    pstTblWalkMsg = dos_dmem_alloc(sizeof(BS_INTER_MSG_WALK));
    if (NULL == pstTblWalkMsg)
    {
        dos_dmem_free(pstMsgNode);
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
        return;
    }

    pstTblWalkMsg->stMsgTag.srcTid = pthread_self();
    pstTblWalkMsg->stMsgTag.dstTid = 0;    /* ���ն˷ǳ���ȷ,Ŀǰ���⴦��,ͳһ��0 */
    pstTblWalkMsg->stMsgTag.ucMsgType = BS_INTER_MSG_WALK_REQ;
    pstTblWalkMsg->stMsgTag.lInterErr = BS_INTER_ERR_SUCC;
    pstTblWalkMsg->stMsgTag.usMsgLen = sizeof(BS_INTER_MSG_WALK);
    pstTblWalkMsg->stMsgTag.ucReserv = 0;
    pstTblWalkMsg->ulTblType = ulTblType;
    pstTblWalkMsg->FnCallback = NULL;
    pstTblWalkMsg->param = NULL;
    pstMsgNode->pHandle = (VOID *)pstTblWalkMsg;

    bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG,
             "Send walk req msg to dl, type:%u, len:%u, errcode:%d, tbl:%d",
             pstTblWalkMsg->stMsgTag.ucMsgType,
             pstTblWalkMsg->stMsgTag.usMsgLen,
             pstTblWalkMsg->stMsgTag.lInterErr,
             pstTblWalkMsg->ulTblType);


    pthread_mutex_lock(&g_mutexBSS2DMsg);
    DLL_Add(&g_stBSS2DMsgList, pstMsgNode);
    pthread_cond_signal(&g_condBSS2DList);
    pthread_mutex_unlock(&g_mutexBSS2DMsg);

}

/* ҵ��㷢�ͻ��������ݲ� */
VOID bss_send_cdr2dl(DLL_NODE_S *pMsgNode, U8 ucMsgType)
{
    BS_INTER_MSG_CDR    *pstCDRMsg = NULL;

    pstCDRMsg = dos_dmem_alloc(sizeof(BS_INTER_MSG_CDR));
    if (NULL == pstCDRMsg)
    {
        /* ��ԭʼ�����ͷŵ� */
        DOS_ASSERT(0);
        dos_dmem_free(pMsgNode->pHandle);
        pMsgNode->pHandle = NULL;
        dos_dmem_free(pMsgNode);
        bs_trace(BS_TRACE_CDR, LOG_LEVEL_CIRT, "CIRT: alloc memory fail!");
        return;
    }

    pstCDRMsg->stMsgTag.srcTid = pthread_self();
    pstCDRMsg->stMsgTag.dstTid = 0;    /* ���ն˷ǳ���ȷ,Ŀǰ���⴦��,ͳһ��0 */
    pstCDRMsg->stMsgTag.ucMsgType = ucMsgType;
    pstCDRMsg->stMsgTag.lInterErr = BS_INTER_ERR_SUCC;
    pstCDRMsg->stMsgTag.usMsgLen = sizeof(BS_INTER_MSG_CDR);
    pstCDRMsg->stMsgTag.ucReserv = 0;
    pstCDRMsg->pCDR = pMsgNode->pHandle;
    pMsgNode->pHandle = (VOID *)pstCDRMsg;

    bs_trace(BS_TRACE_CDR, LOG_LEVEL_DEBUG,
             "Send cdr to dl, type:%u, len:%u, errcode:%d",
             pstCDRMsg->stMsgTag.ucMsgType,
             pstCDRMsg->stMsgTag.usMsgLen,
             pstCDRMsg->stMsgTag.lInterErr);

    pthread_mutex_lock(&g_mutexBSS2DMsg);
    DLL_Add(&g_stBSS2DMsgList, pMsgNode);
    pthread_cond_signal(&g_condBSS2DList);
    pthread_mutex_unlock(&g_mutexBSS2DMsg);

}

/* ҵ��㷢��ͳ����Ϣ�����ݲ� */
VOID bss_send_stat2dl(DLL_NODE_S *pMsgNode, U8 ucMsgType)
{
    BS_INTER_MSG_STAT   *pstStatMsg = NULL;

    pstStatMsg = dos_dmem_alloc(sizeof(BS_INTER_MSG_STAT));
    if (NULL == pstStatMsg)
    {
        /* ��ԭʼ�����ͷŵ� */
        DOS_ASSERT(0);
        dos_dmem_free(pMsgNode->pHandle);
        pMsgNode->pHandle = NULL;
        dos_dmem_free(pMsgNode);
        bs_trace(BS_TRACE_CDR, LOG_LEVEL_CIRT, "CIRT: alloc memory fail!");
        return;
    }

    pstStatMsg->stMsgTag.srcTid = pthread_self();
    pstStatMsg->stMsgTag.dstTid = 0;    /* ���ն˷ǳ���ȷ,Ŀǰ���⴦��,ͳһ��0 */
    pstStatMsg->stMsgTag.ucMsgType = ucMsgType;
    pstStatMsg->stMsgTag.lInterErr = BS_INTER_ERR_SUCC;
    pstStatMsg->stMsgTag.usMsgLen = sizeof(BS_INTER_MSG_STAT);
    pstStatMsg->stMsgTag.ucReserv = 0;
    pstStatMsg->pStat = pMsgNode->pHandle;
    pMsgNode->pHandle = (VOID *)pstStatMsg;

    bs_trace(BS_TRACE_CDR, LOG_LEVEL_DEBUG,
             "Send stat to dl, type:%u, len:%u, errcode:%d",
             pstStatMsg->stMsgTag.ucMsgType,
             pstStatMsg->stMsgTag.usMsgLen,
             pstStatMsg->stMsgTag.lInterErr);

    pthread_mutex_lock(&g_mutexBSS2DMsg);
    DLL_Add(&g_stBSS2DMsgList, pMsgNode);
    pthread_cond_signal(&g_condBSS2DList);
    pthread_mutex_unlock(&g_mutexBSS2DMsg);

}

/* ҵ��㷢��������Ӧ��Ӧ�ò�,ֻ����BS_MSG_TAG����Ӧ��Ϣ */
VOID bss_send_rsp_msg2app(BS_MSG_TAG *pstMsgTag, U8 ucMsgType)
{
    DLL_NODE_S          *pstMsgNode = NULL;
    BS_MSG_TAG          *pstRspMsg = NULL;

    pstMsgNode = dos_dmem_alloc(sizeof(DLL_NODE_S));
    if (NULL == pstMsgNode)
    {
        bs_trace(BS_TRACE_FS, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
        return;
    }
    DLL_Init_Node(pstMsgNode);

    pstRspMsg = dos_dmem_alloc(sizeof(BS_MSG_TAG));
    if (NULL == pstRspMsg)
    {
        dos_dmem_free(pstMsgNode);
        bs_trace(BS_TRACE_FS, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
        return;
    }

    /* ��Ӧ��Ϣ��������Ϣ������ͬ,��ַ��Ϣ&���к���Ϣ��socket����ʱ����д */
    *pstRspMsg = *pstMsgTag;
    pstRspMsg->ucMsgType = ucMsgType;
    pstRspMsg->usMsgLen = sizeof(BS_MSG_TAG);
    pstRspMsg->ulMsgSeq = 0;
    pstRspMsg->ucErrcode = BS_ERR_SUCC;         /* ����Ӧ,������ͳһΪ�ɹ� */
    pstMsgNode->pHandle = (VOID *)pstRspMsg;

    bs_trace(BS_TRACE_FS, LOG_LEVEL_DEBUG,
             "Send rsp msg to app, type:%u, len:%u, errcode:%d",
             pstRspMsg->ucMsgType,
             pstRspMsg->usMsgLen,
             pstRspMsg->ucErrcode);

    pthread_mutex_lock(&g_mutexBSAppMsgSend);
    DLL_Add(&g_stBSAppMsgSendList, pstMsgNode);
    pthread_cond_signal(&g_condBSAppSendList);
    pthread_mutex_unlock(&g_mutexBSAppMsgSend);

}

/* ҵ��㷢��AAA������Ӧ��Ӧ�ò� */
VOID bss_send_aaa_rsp2app(DLL_NODE_S *pMsgNode)
{
    BS_MSG_AUTH         *pstMsg = NULL;

    /* �����·�������ڵ�,����������Ϣ�ڵ�;��ַ��Ϣ&���к���Ϣ��socket����ʱ����д */
    pstMsg = (BS_MSG_AUTH *)pMsgNode->pHandle;
    pstMsg->stMsgTag.ulMsgSeq = 0;
    pstMsg->stMsgTag.usPort = 0;
    pstMsg->stMsgTag.aulIPAddr[0] = 0;
    pstMsg->stMsgTag.aulIPAddr[1] = 0;
    pstMsg->stMsgTag.aulIPAddr[2] = 0;
    pstMsg->stMsgTag.aulIPAddr[3] = 0;

    bs_trace(BS_TRACE_FS, LOG_LEVEL_DEBUG,
             "Send rsp msg to app, type:%u, len:%u, errcode:%d",
             pstMsg->stMsgTag.ucMsgType,
             pstMsg->stMsgTag.usMsgLen,
             pstMsg->stMsgTag.ucErrcode);

    pthread_mutex_lock(&g_mutexBSAppMsgSend);
    DLL_Add(&g_stBSAppMsgSendList, pMsgNode);
    pthread_cond_signal(&g_condBSAppSendList);
    pthread_mutex_unlock(&g_mutexBSAppMsgSend);

}

#ifdef __cplusplus
}
#endif /* __cplusplus */

