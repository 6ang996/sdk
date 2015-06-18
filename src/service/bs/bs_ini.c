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
#include "bs_stat.h"
#include "bs_cdr.h"
#include "bsd_db.h"
#include "bs_def.h"

/* Web����BS�������Ѹ���,����ˢ������ */
pthread_mutex_t g_mutexTableUpdate = PTHREAD_MUTEX_INITIALIZER;
BOOL                 g_bTableUpdate = FALSE;

pthread_mutex_t g_mutexCustomerTbl = PTHREAD_MUTEX_INITIALIZER;
HASH_TABLE_S    *g_astCustomerTbl = NULL;
pthread_mutex_t g_mutexBillingPackageTbl = PTHREAD_MUTEX_INITIALIZER;
HASH_TABLE_S    *g_astBillingPackageTbl = NULL;
pthread_mutex_t g_mutexSettleTbl = PTHREAD_MUTEX_INITIALIZER;
HASH_TABLE_S    *g_astSettleTbl = NULL;
pthread_mutex_t g_mutexAgentTbl = PTHREAD_MUTEX_INITIALIZER;
HASH_TABLE_S    *g_astAgentTbl = NULL;
pthread_mutex_t g_mutexTaskTbl = PTHREAD_MUTEX_INITIALIZER;
HASH_TABLE_S    *g_astTaskTbl = NULL;

pthread_cond_t  g_condBSS2DList  = PTHREAD_COND_INITIALIZER;
pthread_mutex_t g_mutexBSS2DMsg = PTHREAD_MUTEX_INITIALIZER;
DLL_S           g_stBSS2DMsgList;                           /* ҵ��㵽���ݲ���Ϣ���� */
pthread_cond_t  g_condBSD2SList  = PTHREAD_COND_INITIALIZER;
pthread_mutex_t g_mutexBSD2SMsg = PTHREAD_MUTEX_INITIALIZER;
DLL_S           g_stBSD2SMsgList;                           /* ���ݲ㵽ҵ�����Ϣ���� */
pthread_cond_t  g_condBSAppSendList  = PTHREAD_COND_INITIALIZER;
pthread_mutex_t g_mutexBSAppMsgSend = PTHREAD_MUTEX_INITIALIZER;
DLL_S           g_stBSAppMsgSendList;                       /* Ӧ�ò���Ϣ���Ͷ��� */
pthread_cond_t  g_condBSAAAList  = PTHREAD_COND_INITIALIZER;
pthread_mutex_t g_mutexBSAAAMsg = PTHREAD_MUTEX_INITIALIZER;
DLL_S           g_stBSAAAMsgList;                           /* AAA��Ϣ���� */
pthread_cond_t  g_condBSCDRList  = PTHREAD_COND_INITIALIZER;
pthread_mutex_t g_mutexBSCDR = PTHREAD_MUTEX_INITIALIZER;
DLL_S           g_stBSCDRList;                              /* ԭʼCDR������� */
pthread_cond_t  g_condBSBillingList  = PTHREAD_COND_INITIALIZER;
pthread_mutex_t g_mutexBSBilling = PTHREAD_MUTEX_INITIALIZER;
DLL_S           g_stBSBillingList;                          /* �Ʒѻ���������� */

pthread_mutex_t g_mutexWebCMDTbl = PTHREAD_MUTEX_INITIALIZER;
DLL_S           g_stWebCMDTbl;                              /* WEB����������б� */
U32             g_ulLastCMDTimestamp = 0;

BSS_CB  g_stBssCB;

VOID bs_init_msg_list(VOID)
{
    pthread_mutex_lock(&g_mutexBSS2DMsg);
    DLL_Init(&g_stBSS2DMsgList);
    pthread_mutex_unlock(&g_mutexBSS2DMsg);

    pthread_mutex_lock(&g_mutexBSD2SMsg);
    DLL_Init(&g_stBSD2SMsgList);
    pthread_mutex_unlock(&g_mutexBSD2SMsg);

    pthread_mutex_lock(&g_mutexBSAppMsgSend);
    DLL_Init(&g_stBSAppMsgSendList);
    pthread_mutex_unlock(&g_mutexBSAppMsgSend);

    pthread_mutex_lock(&g_mutexBSAAAMsg);
    DLL_Init(&g_stBSAAAMsgList);
    pthread_mutex_unlock(&g_mutexBSAAAMsg);

    pthread_mutex_lock(&g_mutexBSCDR);
    DLL_Init(&g_stBSCDRList);
    pthread_mutex_unlock(&g_mutexBSCDR);

    pthread_mutex_lock(&g_mutexBSBilling);
    DLL_Init(&g_stBSBillingList);
    pthread_mutex_unlock(&g_mutexBSBilling);

}

VOID bs_build_customer_tree(VOID)
{
    U32             ulCnt = 0, ulHashIndex;
    HASH_NODE_S     *pstHashNode = NULL;
    BS_CUSTOMER_ST  *pstCustomer = NULL;
    BS_CUSTOMER_ST  *pstParentCustomer = NULL;

    bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG, "Now start build customer tree in memory!");

    pthread_mutex_lock(&g_mutexCustomerTbl);
    HASH_Scan_Table(g_astCustomerTbl, ulHashIndex)
    {
        HASH_Scan_Bucket(g_astCustomerTbl, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            ulCnt++;
            if (ulCnt > g_astCustomerTbl->NodeNum)
            {
                DOS_ASSERT(0);
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR,
                         "Err: customer number is exceed in hash table(%u)!",
                         g_astCustomerTbl->NodeNum);
                pthread_mutex_unlock(&g_mutexCustomerTbl);
                return;
            }

            pstCustomer = (BS_CUSTOMER_ST *)pstHashNode->pHandle;
            if (DOS_ADDR_INVALID(pstCustomer))
            {
                /* �ڵ�δ����ͻ���Ϣ,�쳣,������Ӧ���ͷŸýڵ�,�����ǵ�����ڵ�������Թ̶�,�ڴ治����й¶����,����֮ */
                DOS_ASSERT(0);
                ulCnt--;
                continue;
            }

            /* ��ȡ���ڵ� */
            pthread_mutex_unlock(&g_mutexCustomerTbl);
            pstParentCustomer = bs_get_customer_st(pstCustomer->ulParentID);
            pthread_mutex_lock(&g_mutexCustomerTbl);
            if (DOS_ADDR_INVALID(pstParentCustomer))
            {
                /* �޸��ڵ�,������ֻ��top���Ͳ������ */
                if (pstCustomer->ucCustomerType != BS_CUSTOMER_TYPE_TOP)
                {
                    bs_trace(BS_TRACE_DB, LOG_LEVEL_ERROR, "Err: customer(%u:%s) has no parent!",
                             pstCustomer->ulCustomerID, pstCustomer->szCustomerName);

                    ulCnt--;
                }
                else
                {
                    /* ����top�ͻ���Ϣ��ҵ�����ƿ� */
                    g_stBssCB.pstTopCustomer = pstCustomer;
                }

                continue;
            }

            /* �ϼ��ͻ�������������,ֻ�п����Ǵ����̻򶥼��ͻ� */
            if (BS_CUSTOMER_TYPE_CONSUMER == pstParentCustomer->ucCustomerType)
            {
                bs_trace(BS_TRACE_DB, LOG_LEVEL_ERROR, "Err: the parent of customer(%u:%s) is a comsuer(%u:%s)!",
                         pstCustomer->ulCustomerID, pstCustomer->szCustomerName,
                         pstParentCustomer->ulCustomerID, pstParentCustomer->szCustomerName);

                ulCnt--;
                continue;
            }

            /* ���¿ͻ����ƿ���Ϣ */
            pstCustomer->stAccount.LBalanceActive = pstCustomer->stAccount.LBalance;
            pstCustomer->stNode.pHandle = pstHashNode;

            /* ���¿ͻ��� */
            pstCustomer->pstParent = pstParentCustomer;
            bs_customer_add_child(pstParentCustomer, pstCustomer);
        }
    }
    pthread_mutex_unlock(&g_mutexCustomerTbl);

    bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG, "Total %d customers in tree!", ulCnt);

}

/* ��ʼ���ͻ���Ϣ�ṹ�� */
VOID bs_init_customer_st(BS_CUSTOMER_ST *pstCustomer)
{
    dos_memzero((VOID *)pstCustomer, sizeof(BS_CUSTOMER_ST));

    pstCustomer->ulCustomerID = U32_BUTT;
    pstCustomer->ulParentID = U32_BUTT;
    pstCustomer->ucCustomerType = BS_CUSTOMER_TYPE_BUTT;
    pstCustomer->ucCustomerState = BS_CUSTOMER_STATE_BUTT;
    pthread_mutex_init(&pstCustomer->stAccount.mutexAccount, NULL);
    pstCustomer->stAccount.ulAccountID = U32_BUTT;
    pstCustomer->stAccount.ulCustomerID = U32_BUTT;
    pstCustomer->stAccount.ulBillingPackageID = U32_BUTT;
    pstCustomer->pstParent = NULL;

    DLL_Init(&pstCustomer->stChildrenList);
    DLL_Init_Node(&pstCustomer->stNode);

}

/* ��ʼ���ͻ���Ϣ��ϣ�� */
S32 bs_init_customer_tbl(VOID)
{
    pthread_mutex_lock(&g_mutexCustomerTbl);
    g_astCustomerTbl = hash_create_table(BS_HASH_TBL_CUSTOMER_SIZE, NULL);
    if (!g_astCustomerTbl)
    {
        pthread_mutex_unlock(&g_mutexCustomerTbl);
        DOS_ASSERT(0);
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Creat customer hash table fail!");
        return DOS_FAIL;
    }
    g_astCustomerTbl->NodeNum = 0;
    pthread_mutex_unlock(&g_mutexCustomerTbl);

    /* �����ݲ㷢�ͱ����û���������Ϣ,��ȡ�ͻ����ݵ��ڴ� */
    bss_send_walk_req2dl(BS_TBL_TYPE_CUSTOMER, NULL, NULL);

    return DOS_SUCC;
}

/* ��ʼ����ϯ��Ϣ�ṹ�� */
VOID bs_init_agent_st(BS_AGENT_ST *pstAgent)
{
    dos_memzero((VOID *)pstAgent, sizeof(BS_AGENT_ST));

    pstAgent->ulCustomerID = U32_BUTT;
    pstAgent->ulAgentID = U32_BUTT;
    pstAgent->ulGroup1 = U32_BUTT;
    pstAgent->ulGroup2= U32_BUTT;

}

/* ��ʼ����ϯ��Ϣ��ϣ�� */
S32 bs_init_agent_tbl(VOID)
{
    pthread_mutex_lock(&g_mutexAgentTbl);
    g_astAgentTbl = hash_create_table(BS_HASH_TBL_AGENT_SIZE, NULL);
    if (!g_astAgentTbl)
    {
        pthread_mutex_unlock(&g_mutexAgentTbl);
        DOS_ASSERT(0);
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Creat agent hash table fail!");
        return DOS_FAIL;
    }
    g_astAgentTbl->NodeNum = 0;
    pthread_mutex_unlock(&g_mutexAgentTbl);

    /* �����ݲ㷢�ͱ����û���������Ϣ,��ȡ�ͻ����ݵ��ڴ� */
    bss_send_walk_req2dl(BS_TBL_TYPE_AGENT, NULL, NULL);

    return DOS_SUCC;
}

S32 bs_init_web_cmd_list(VOID)
{
    DLL_Init(&g_stWebCMDTbl);

    bss_send_walk_req2dl(BS_TBL_TYPE_TMP_CMD_DEL, NULL, NULL);

    return 0;
}

/* ��ʼ���ʷ���Ϣ�ṹ�� */
VOID bs_init_billing_package_st(BS_BILLING_PACKAGE_ST *pstBillingPackage)
{
    dos_memzero((VOID *)pstBillingPackage, sizeof(BS_BILLING_PACKAGE_ST));

    pstBillingPackage->ulPackageID = U32_BUTT;
    pstBillingPackage->ucServType = BS_SERV_BUTT;

}

/* ��ʼ���ʷѰ���ϣ�� */
S32 bs_init_billing_package_tbl(VOID)
{
    pthread_mutex_lock(&g_mutexBillingPackageTbl);
    g_astBillingPackageTbl = hash_create_table(BS_HASH_TBL_BILLING_PACKAGE_SIZE, NULL);
    if (!g_astBillingPackageTbl)
    {
        pthread_mutex_unlock(&g_mutexBillingPackageTbl);
        DOS_ASSERT(0);
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Creat billing package hash table fail!");
        return DOS_FAIL;
    }
    g_astBillingPackageTbl->NodeNum = 0;
    pthread_mutex_unlock(&g_mutexBillingPackageTbl);

    /* �����ݲ㷢�ͱ����ʷѱ�������Ϣ,��ȡ�ʷ����ݵ��ڴ� */
    bss_send_walk_req2dl(BS_TBL_TYPE_BILLING_PACKAGE, NULL, NULL);

    return DOS_SUCC;
}


/* ��ʼ��������Ϣ�ṹ�� */
VOID bs_init_settle_st(BS_SETTLE_ST *pstSettle)
{
    dos_memzero((VOID *)pstSettle, sizeof(BS_SETTLE_ST));

    pstSettle->usTrunkID = U16_BUTT;
    pstSettle->ulSPID = U32_BUTT;
    pstSettle->ulPackageID = U32_BUTT;
}

/* ��ʼ�������ϣ�� */
S32 bs_init_settle_tbl(VOID)
{
    pthread_mutex_lock(&g_mutexSettleTbl);
    g_astSettleTbl = hash_create_table(BS_HASH_TBL_SETTLE_SIZE, NULL);
    if (!g_astSettleTbl)
    {
        pthread_mutex_unlock(&g_mutexSettleTbl);
        DOS_ASSERT(0);
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Creat billing package hash table fail!");
        return DOS_FAIL;
    }
    g_astSettleTbl->NodeNum = 0;
    pthread_mutex_unlock(&g_mutexSettleTbl);

    /* �����ݲ㷢�ͱ��������������Ϣ,��ȡ�������ݵ��ڴ� */
    bss_send_walk_req2dl(BS_TBL_TYPE_SETTLE, NULL, NULL);

    return DOS_SUCC;
}

/* ��ʼ����ϯ��Ϣ��ϣ�� */
S32 bs_init_task_tbl(VOID)
{
    pthread_mutex_lock(&g_mutexTaskTbl);
    g_astTaskTbl = hash_create_table(BS_HASH_TBL_AGENT_SIZE, NULL);
    if (!g_astTaskTbl)
    {
        pthread_mutex_unlock(&g_mutexTaskTbl);
        DOS_ASSERT(0);
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Creat task hash table fail!");
        return DOS_FAIL;
    }
    g_astTaskTbl->NodeNum = 0;
    pthread_mutex_unlock(&g_mutexTaskTbl);
    return DOS_SUCC;
}


/* ģ���������� */
S32 bs_start()
{
    time_t          stTime;
    struct tm       *t = NULL;
    U32             ulTimeStamp, ulTimeout;
    S32             lRet = 0;
    pthread_t       tid;

    bs_init_msg_list();
    dos_memzero(&g_stBssCB, sizeof(g_stBssCB));
    g_stBssCB.bIsMaintain = DOS_TRUE;
    g_stBssCB.bIsBillDay = DOS_FALSE;
    g_stBssCB.bIsDayCycle = DOS_FALSE;
    g_stBssCB.bIsHourCycle = DOS_FALSE;
    g_stBssCB.ulMaxMsNum = BS_MAX_MESSAGE_NUM;
    g_stBssCB.ulMaxVocTime = BS_MAX_VOICE_SESSION_TIME;
    g_stBssCB.usUDPListenPort = dos_htons(BS_UDP_LINSTEN_PORT);
    g_stBssCB.usTCPListenPort = dos_htons(BS_TCP_LINSTEN_PORT);
    g_stBssCB.ulCommProto = BSCOMM_PROTO_UNIX;
    srandom((U32)time(NULL));
    g_stBssCB.ulCDRMark = (U32)random();
    g_stBssCB.pstTopCustomer = NULL;
    g_stBssCB.hDayCycleTmr = NULL;
    g_stBssCB.hHourCycleTmr= NULL;
    g_stBssCB.ulTraceFlag = U32_BUTT;

    lRet = bs_init_db();
    if (lRet < 0)
    {
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: create pthread(operate_data) error!");
        return DOS_FAIL;
    }

    lRet = pthread_create(&tid, NULL, bsd_recv_bss_msg, NULL);
    if (lRet < 0)
    {
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: create pthread(operate_data) error!");
        return DOS_FAIL;
    }

    lRet = pthread_create(&tid, NULL, bss_recv_bsd_msg, NULL);
    if (lRet < 0)
    {
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: create pthread(recv_inter_msg) error!");
        return DOS_FAIL;
    }

    lRet = bs_init_billing_package_tbl();
    if (lRet < 0)
    {
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: init billing package fail!");
        return DOS_FAIL;
    }

    lRet = bs_init_settle_tbl();
    if (lRet < 0)
    {
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: init task hash tbl fail!");
        return DOS_FAIL;
    }

    lRet = bs_init_task_tbl();
    if (lRet < 0)
    {
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: init settle hash tbl fail!");
        return DOS_FAIL;
    }

    lRet = bs_init_customer_tbl();
    if (lRet < 0)
    {
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: init customer hash table fail!");
        return DOS_FAIL;
    }

    lRet = bs_init_agent_tbl();
    if (lRet < 0)
    {
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: init customer hash table fail!");
        return DOS_FAIL;
    }

    lRet = bs_init_web_cmd_list();
    if (lRet < 0)
    {
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: init web cmd list fail!");
        return DOS_FAIL;
    }

    lRet = pthread_create(&tid, NULL, bss_send_msg2app, NULL);
    if (lRet < 0)
    {
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: create pthread(send_msg2app) error!");
        return DOS_FAIL;
    }

    lRet = pthread_create(&tid, NULL, bss_recv_msg_from_app, NULL);
    if (lRet < 0)
    {
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: create pthread(recv_msg_from_app) error!");
        return DOS_FAIL;
    }

    lRet = pthread_create(&tid, NULL, bss_recv_msg_from_web, NULL);
    if (lRet < 0)
    {
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: create pthread(recv_msg_from_web) error!");
        return DOS_FAIL;
    }

    lRet = pthread_create(&tid, NULL, bss_web_msg_proc, NULL);
    if (lRet < 0)
    {
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: create pthread(web_msg_proc) error!");
        return DOS_FAIL;
    }

    lRet = pthread_create(&tid, NULL, bss_aaa, NULL);
    if (lRet < 0)
    {
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: create pthread(aaa) error!");
        return DOS_FAIL;
    }

    lRet = pthread_create(&tid, NULL, bss_cdr, NULL);
    if (lRet < 0)
    {
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: create pthread(original_cdr) error!");
        return DOS_FAIL;
    }

    lRet = pthread_create(&tid, NULL, bss_billing, NULL);
    if (lRet < 0)
    {
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: create pthread(billing) error!");
        return DOS_FAIL;
    }

    lRet = pthread_create(&tid, NULL, bss_accounting, NULL);
    if (lRet < 0)
    {
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: create pthread(accounting) error!");
        return DOS_FAIL;
    }

    lRet = pthread_create(&tid, NULL, bss_stat, NULL);
    if (lRet < 0)
    {
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: create pthread(stat) error!");
        return DOS_FAIL;
    }

    lRet = pthread_create(&tid, NULL, bss_audit, NULL);
    if (lRet < 0)
    {
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: create pthread(audit) error!");
        return DOS_FAIL;
    }

    lRet = pthread_create(&tid, NULL, bsd_backup, NULL);
    if (lRet < 0)
    {
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: create pthread(backup) error!");
        return DOS_FAIL;
    }

    stTime = time(NULL);
    t = localtime(&stTime);
    ulTimeStamp = (U32)stTime;

    g_stBssCB.ulHour = (U32)t->tm_hour;
    /* ͳ�ƻ�������Ϊ����0�� */
    g_stBssCB.ulStatDayBase = (U32)(ulTimeStamp - (3600 * t->tm_hour) - (t->tm_min * 60) - t->tm_sec);

    /* ѭ�����ڼƷ�,������ÿ���賿1������ */
    if (NULL == g_stBssCB.hDayCycleTmr)
    {
        if (t->tm_hour > 1)
        {
            /* �ӳٵ���һ�յ�1�� */
            ulTimeout = (U32)((24 * 3600) - (t->tm_hour - 1) * 3600 - (t->tm_min * 60) - t->tm_sec);
        }
        else
        {
            ulTimeout = (U32)((1 * 3600) - (t->tm_hour * 3600) - (t->tm_min * 60) - t->tm_sec);
        }

        if (ulTimeout != 0)
        {
            dos_tmr_start(&g_stBssCB.hDayCycleTmr,
                          ulTimeout * 1000,
                          bss_day_cycle_proc,
                          0,
                          TIMER_NORMAL_NO_LOOP);
        }
        else
        {
            bss_day_cycle_proc(0);
        }
    }

    /* ѭ������ͳ��,ÿ��1��Сʱ����1�� */
    if (NULL == g_stBssCB.hHourCycleTmr)
    {
        /* ͳ�ƻ�������Ϊ���� */
        g_stBssCB.ulStatHourBase = (U32)(ulTimeStamp - (t->tm_min * 60) - t->tm_sec);

        /* ÿ����㽫��һ��Сʱ��ͳ�����ݴ洢�����ݿ�,������ʱ��,�ӳٵ���һ����� */
        ulTimeout = (U32)(3600 - (t->tm_min * 60)  - t->tm_sec + 1800);

        dos_tmr_start(&g_stBssCB.hHourCycleTmr,
                      ulTimeout * 1000,
                      bss_hour_cycle_proc,
                      0,
                      TIMER_NORMAL_NO_LOOP);
    }

    return DOS_SUCC;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

