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


/* �ͷ������� */
VOID bs_free_node(VOID *pNode)
{
    DLL_NODE_S *pstNode;

    if(DOS_ADDR_INVALID(pNode))
    {
        DOS_ASSERT(0);
        return;
    }

    pstNode = (DLL_NODE_S *)pNode;
    if(DOS_ADDR_INVALID(pstNode->pHandle))
    {
        DOS_ASSERT(0);
        dos_dmem_free(pNode);
        return;
    }

    dos_dmem_free(pstNode->pHandle);
    dos_dmem_free(pNode);

}

/* �жϿͻ���ϣ��ڵ��Ƿ�ƥ�� */
S32 bs_customer_hash_node_match(VOID *pKey, HASH_NODE_S *pNode)
{
    BS_CUSTOMER_ST  *pstCustomer = NULL;

    if (DOS_ADDR_INVALID(pKey) || DOS_ADDR_INVALID(pNode))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstCustomer = (BS_CUSTOMER_ST *)pNode->pHandle;
    if (DOS_ADDR_INVALID(pstCustomer))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (pstCustomer->ulCustomerID == *(U32 *)pKey)
    {
        return DOS_SUCC;
    }

    return DOS_FAIL;
}

/* �жϿͻ���ϣ��ڵ��Ƿ�ƥ�� */
S32 bs_agent_hash_node_match(VOID *pKey, HASH_NODE_S *pNode)
{
    BS_AGENT_ST     *pstAgent = NULL;

    if (DOS_ADDR_INVALID(pKey) || DOS_ADDR_INVALID(pNode))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstAgent = (BS_AGENT_ST *)pNode->pHandle;
    if (DOS_ADDR_INVALID(pstAgent))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (pstAgent->ulAgentID == *(U32 *)pKey)
    {
        return DOS_SUCC;
    }

    return DOS_FAIL;
}


/* �ж��ʷѹ�ϣ��ڵ��Ƿ�ƥ�� */
S32 bs_billing_package_hash_node_match(VOID *pKey, HASH_NODE_S *pNode)
{
    BS_BILLING_PACKAGE_ST   *pstMatch = NULL;
    BS_BILLING_PACKAGE_ST   *pstBillingPackage = NULL;

    if (DOS_ADDR_INVALID(pKey) || DOS_ADDR_INVALID(pNode))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstBillingPackage = (BS_BILLING_PACKAGE_ST *)pNode->pHandle;
    if (DOS_ADDR_INVALID(pstBillingPackage))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstMatch = (BS_BILLING_PACKAGE_ST *)pKey;
    if (pstBillingPackage->ulPackageID == pstMatch->ulPackageID
        && pstBillingPackage->ucServType == pstMatch->ucServType)
    {
        return DOS_SUCC;
    }

    return DOS_FAIL;
}

/* �жϽ����ϣ��ڵ��Ƿ�ƥ�� */
S32 bs_settle_hash_node_match(VOID *pKey, HASH_NODE_S *pNode)
{
    BS_SETTLE_ST    *pstMatch = NULL;
    BS_SETTLE_ST    *pstSettle = NULL;

    if (DOS_ADDR_INVALID(pKey) || DOS_ADDR_INVALID(pNode))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstSettle = (BS_SETTLE_ST *)pNode->pHandle;
    if (DOS_ADDR_INVALID(pstSettle))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstMatch = (BS_SETTLE_ST *)pKey;
    if (pstSettle->usTrunkID == pstMatch->usTrunkID)
    {
        return DOS_SUCC;
    }

    return DOS_FAIL;
}

/* �ж������ϣ��ڵ��Ƿ�ƥ�� */
S32 bs_task_hash_node_match(VOID *pKey, HASH_NODE_S *pNode)
{
    BS_TASK_ST     *pstTask = NULL;

    if (DOS_ADDR_INVALID(pKey) || DOS_ADDR_INVALID(pNode))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstTask = (BS_TASK_ST *)pNode->pHandle;
    if (DOS_ADDR_INVALID(pstTask))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (pstTask->ulTaskID == *(U32 *)pKey)
    {
        return DOS_SUCC;
    }

    return DOS_FAIL;
}

/*
˵��:����ID�͹�ϣ���С��ȡ��ϣ����,U32_BUTTΪ��Чֵ
��ʾ:Ϊ���Ч��,Ҫ���ϣ���СΪ2��������
*/
U32 bs_hash_get_index(U32 ulHashTblSize, U32 ulID)
{
    if (U32_BUTT == ulID)
    {
        return U32_BUTT;
    }

    return ulID & (ulHashTblSize - 1);
}

/* ��ȡ�ͻ���Ϣ�ڹ�ϣ���еĽڵ� */
HASH_NODE_S *bs_get_customer_node(U32 ulCustomerID)
{
    U32             ulHashIndex;
    HASH_NODE_S     *pstHashNode = NULL;

    ulHashIndex = bs_hash_get_index(BS_HASH_TBL_CUSTOMER_SIZE, ulCustomerID);
    if (U32_BUTT == ulHashIndex)
    {
        DOS_ASSERT(0);
        return NULL;
    }

    pthread_mutex_lock(&g_mutexCustomerTbl);
    pstHashNode = hash_find_node(g_astCustomerTbl, ulHashIndex,
                                  (VOID *)&ulCustomerID,
                                  bs_customer_hash_node_match);
    pthread_mutex_unlock(&g_mutexCustomerTbl);

    return pstHashNode;

}

/* ��ȡ�ͻ���Ϣ�ṹ�� */
BS_CUSTOMER_ST *bs_get_customer_st(U32 ulCustomerID)
{
    HASH_NODE_S     *pstHashNode = NULL;

    pstHashNode = bs_get_customer_node(ulCustomerID);
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        return NULL;
    }

    return (BS_CUSTOMER_ST *)pstHashNode->pHandle;

}

/* ��ȡ��ϯ��Ϣ�ṹ�� */
BS_AGENT_ST *bs_get_agent_st(U32 ulAgentID)
{
    U32             ulHashIndex;
    HASH_NODE_S     *pstHashNode = NULL;

    ulHashIndex = bs_hash_get_index(BS_HASH_TBL_AGENT_SIZE, ulAgentID);
    if (U32_BUTT == ulHashIndex)
    {
        DOS_ASSERT(0);
        return NULL;
    }

    pthread_mutex_lock(&g_mutexAgentTbl);
    pstHashNode = hash_find_node(g_astAgentTbl, ulHashIndex,
                                  (VOID *)&ulAgentID,
                                  bs_agent_hash_node_match);
    pthread_mutex_unlock(&g_mutexAgentTbl);
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        return NULL;
    }

    return (BS_AGENT_ST *)pstHashNode->pHandle;

}

/* ��ȡ������Ϣ�ṹ�� */
BS_SETTLE_ST *bs_get_settle_st(U16 usTrunkID)
{
    U32             ulHashIndex;
    HASH_NODE_S     *pstHashNode = NULL;
    BS_SETTLE_ST    stSettle;

    ulHashIndex = bs_hash_get_index(BS_HASH_TBL_SETTLE_SIZE, usTrunkID);
    if (U32_BUTT == ulHashIndex)
    {
        DOS_ASSERT(0);
        return NULL;
    }

    stSettle.usTrunkID = usTrunkID;
    pthread_mutex_lock(&g_mutexSettleTbl);
    pstHashNode = hash_find_node(g_astSettleTbl, ulHashIndex,
                                  (VOID *)&stSettle,
                                  bs_settle_hash_node_match);
    pthread_mutex_unlock(&g_mutexSettleTbl);
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        return NULL;
    }

    return (BS_SETTLE_ST *)pstHashNode->pHandle;

}

/* ��ȡ������Ϣ�ṹ�� */
BS_TASK_ST *bs_get_task_st(U32 ulTaskID)
{
    U32             ulHashIndex;
    HASH_NODE_S     *pstHashNode = NULL;

    ulHashIndex = bs_hash_get_index(BS_HASH_TBL_TASK_SIZE, ulTaskID);
    if (U32_BUTT == ulHashIndex)
    {
        DOS_ASSERT(0);
        return NULL;
    }

    pthread_mutex_lock(&g_mutexTaskTbl);
    pstHashNode = hash_find_node(g_astTaskTbl, ulHashIndex,
                                  (VOID *)&ulTaskID,
                                  bs_task_hash_node_match);
    pthread_mutex_unlock(&g_mutexTaskTbl);
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        return NULL;
    }

    return (BS_TASK_ST *)pstHashNode->pHandle;

}


/* �ͻ�������ӽڵ㺯�� */
VOID bs_customer_add_child(BS_CUSTOMER_ST *pstCustomer, BS_CUSTOMER_ST *pstChildCustomer)
{
    DLL_NODE_S      *pstNode = NULL;

    if (DOS_ADDR_INVALID(pstCustomer) || DOS_ADDR_INVALID(pstChildCustomer))
    {
        DOS_ASSERT(0);
        return;
    }

    /* �����Ƿ��Ѿ���ӹ����ӽڵ�,ע��match������Ӧ��ʹ��HASH_NODE_S,����DLL_NODE_S�ṹ��ͬ,���ܵ��� */
    pstNode = dll_find(&pstCustomer->stChildrenList, (VOID *)&pstChildCustomer->ulCustomerID, bs_customer_hash_node_match);
    if (DOS_ADDR_VALID(pstNode))
    {
        /* �ڵ��Ѿ����� */
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_WARNING, "Warning: the child(%u:%s) of customer(%u:%s) is already exist when build tree!",
                 pstChildCustomer->ulCustomerID, pstChildCustomer->szCustomerName,
                 pstCustomer->ulCustomerID, pstCustomer->szCustomerName);
        return;
    }

    DLL_Add(&pstCustomer->stChildrenList, &pstChildCustomer->stNode);
}

/* ��ȡӦ�ò�socket�����Ϣ */
BSS_APP_CONN *bs_get_app_conn(BS_MSG_TAG *pstMsgTag)
{
    S32     i;

    if (DOS_ADDR_INVALID(pstMsgTag))
    {
        DOS_ASSERT(0);
        return NULL;
    }

    for (i = 0; i < BS_MAX_APP_CONN_NUM; i++)
    {
        if (g_stBssCB.astAPPConn[i].bIsConn
            && pstMsgTag->aulIPAddr[0] == g_stBssCB.astAPPConn[i].aulIPAddr[0]
            && pstMsgTag->aulIPAddr[1] == g_stBssCB.astAPPConn[i].aulIPAddr[1]
            && pstMsgTag->aulIPAddr[2] == g_stBssCB.astAPPConn[i].aulIPAddr[2]
            && pstMsgTag->aulIPAddr[3] == g_stBssCB.astAPPConn[i].aulIPAddr[3])
        {
            return &g_stBssCB.astAPPConn[i];
        }
    }

    return NULL;
}

/* ����Ӧ�ò�socket�����Ϣ,Ŀǰֻʵ��IPv4 */
BSS_APP_CONN *bs_save_app_conn(S32 lSockfd, U8 *pstAddrIn, U32 ulAddrinHeader, S32 lAddrLen)
{
    S32                 i, lIdlePos = -1;
    BSS_APP_CONN        *pstAppConn = NULL;
    struct sockaddr_in  *pstAddr;
    struct sockaddr_un  *pstUnAddr;

    if (DOS_ADDR_INVALID(pstAddrIn))
    {
        DOS_ASSERT(0);
        return NULL;
    }

    /* ���ҵ�ַ�Ƿ��Ѿ������,������������� */
    for (i = 0; i < BS_MAX_APP_CONN_NUM; i++)
    {
        if (g_stBssCB.astAPPConn[i].bIsValid)
        {
            if (0 == dos_memcmp(pstAddrIn, &g_stBssCB.astAPPConn[i].stAddr, sizeof(struct sockaddr_in)))
            {
                pstAppConn = &g_stBssCB.astAPPConn[i];

                /* �����unix socket����Ҫ����һ�±�ʾ�� */
                if (BSCOMM_PROTO_UNIX == g_stBssCB.ulCommProto
                    && ulAddrinHeader != g_stBssCB.astAPPConn[i].aulIPAddr[0])
                {
                    pstAppConn->aulIPAddr[0] = ulAddrinHeader;
                }

                break;
            }
        }
        else if (-1 == lIdlePos)
        {
            lIdlePos = i;
        }
    }

    if (NULL == pstAppConn && lIdlePos != -1)
    {
        /* δ�����,���ڿ���λ�ñ�����Ϣ */
        pstAppConn = &g_stBssCB.astAPPConn[lIdlePos];
        pstAppConn->bIsValid = DOS_TRUE;
        pstAppConn->bIsConn = DOS_TRUE;
        pstAppConn->ucCommType = (U8)g_stBssCB.ulCommProto;
        pstAppConn->ulReserv = 0;
        pstAppConn->ulMsgSeq = 0;
        pstAppConn->lSockfd = lSockfd;
        pstAppConn->lAddrLen = lAddrLen;

        if (g_stBssCB.ulCommProto != BSCOMM_PROTO_UNIX)
        {
            pstAddr = (struct sockaddr_in *)pstAddrIn;

            pstAppConn->aulIPAddr[0] = pstAddr->sin_addr.s_addr;
            pstAppConn->aulIPAddr[1] = 0;
            pstAppConn->aulIPAddr[2] = 0;
            pstAppConn->aulIPAddr[3] = 0;
            pstAppConn->stAddr.stInAddr.sin_family = AF_INET;
            pstAppConn->stAddr.stInAddr.sin_port = pstAddr->sin_port;
            pstAppConn->stAddr.stInAddr.sin_addr.s_addr = pstAddr->sin_addr.s_addr;
        }
        else
        {
            pstAppConn->aulIPAddr[0] = ulAddrinHeader;
            pstAppConn->aulIPAddr[1] = 0;
            pstAppConn->aulIPAddr[2] = 0;
            pstAppConn->aulIPAddr[3] = 0;

            pstUnAddr = (struct sockaddr_un *)pstAddrIn;
            pstAppConn->stAddr.stUnAddr.sun_family = AF_UNIX;
            dos_strncpy(pstAppConn->stAddr.stUnAddr.sun_path, pstUnAddr->sun_path, lAddrLen);
            pstAppConn->stAddr.stUnAddr.sun_path[lAddrLen] = '\0';

            bs_trace(BS_TRACE_FS, LOG_LEVEL_DEBUG, "New unix socket %s, len: %d", pstAppConn->stAddr.stUnAddr.sun_path, lAddrLen);
        }
    }

    return pstAppConn;
}

/* ͳ����ϯ���� */
VOID bs_stat_agent_num(VOID)
{
    U32             ulCnt = 0, ulHashIndex;
    HASH_NODE_S     *pstHashNode = NULL;
    BS_AGENT_ST     *pstAgent = NULL;
    BS_CUSTOMER_ST  *pstCustomer = NULL;

    bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG, "Stat agent number!");

    pthread_mutex_lock(&g_mutexAgentTbl);
    HASH_Scan_Table(g_astAgentTbl, ulHashIndex)
    {
        HASH_Scan_Bucket(g_astAgentTbl, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            ulCnt++;
            if (ulCnt > g_astAgentTbl->NodeNum)
            {
                DOS_ASSERT(0);
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR,
                         "Err: agent number is exceed in hash table(%u)!",
                         g_astAgentTbl->NodeNum);
                pthread_mutex_unlock(&g_mutexAgentTbl);
                return;
            }

            pstAgent = (BS_AGENT_ST *)pstHashNode->pHandle;
            if (DOS_ADDR_INVALID(pstAgent))
            {
                /* �ڵ�δ����ͻ���Ϣ,�쳣,������Ӧ���ͷŸýڵ�,�����ǵ�����ڵ�������Թ̶�,�ڴ治����й¶����,����֮ */
                DOS_ASSERT(0);
                ulCnt--;
                continue;
            }

            /* ��ȡ�ͻ���Ϣ�ṹ�� */
            pstCustomer = bs_get_customer_st(pstAgent->ulCustomerID);
            if (NULL == pstCustomer)
            {
                /* �Ҳ��� */
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR,
                         "Err: unknown agent! agent:%u, customer:%u",
                         pstAgent->ulAgentID, pstAgent->ulCustomerID);
                continue;
            }

            /* ���¿ͻ����ƿ���Ϣ */
            pstCustomer->ulAgentNum++;
        }
    }
    pthread_mutex_unlock(&g_mutexAgentTbl);

    bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG, "Total %d agent!", ulCnt);

}

/* �����м̱�Ų����ʷѰ�ID */
U32 bs_get_settle_packageid(U16 usTrunkID)
{
    U32             ulHashIndex;
    HASH_NODE_S     *pstHashNode = NULL;
    BS_SETTLE_ST    *pstSettle = NULL;
    BS_SETTLE_ST    stSettle;

    stSettle.usTrunkID = usTrunkID;
    ulHashIndex = bs_hash_get_index(BS_HASH_TBL_SETTLE_SIZE, usTrunkID);
    if (U32_BUTT == ulHashIndex)
    {
        DOS_ASSERT(0);
        return U32_BUTT;
    }

    pthread_mutex_lock(&g_mutexSettleTbl);
    pstHashNode = hash_find_node(g_astSettleTbl,
                                 ulHashIndex,
                                 (VOID *)&stSettle,
                                 bs_settle_hash_node_match);
    pthread_mutex_unlock(&g_mutexSettleTbl);

    pstSettle = (BS_SETTLE_ST *)pstHashNode->pHandle;
    if (DOS_ADDR_INVALID(pstSettle))
    {
        DOS_ASSERT(0);
        return U32_BUTT;
    }

    return pstSettle->ulPackageID;
}

/* �ж��Ƿ�������Ϣ��ҵ�� */
BOOL bs_is_message_service(U8 ucServType)
{
    if (BS_SERV_MMS_RECV == ucServType
        || BS_SERV_MMS_SEND == ucServType
        || BS_SERV_SMS_RECV == ucServType
        || BS_SERV_SMS_SEND == ucServType)
    {
        return DOS_TRUE;
    }

    return DOS_FALSE;
}

/* �����ض�ҵ�����͵ļƷѰ� */
BS_BILLING_PACKAGE_ST *bs_get_billing_package(U32 ulPackageID, U8 ucServType)
{
    U32                     ulHashIndex;
    BS_BILLING_PACKAGE_ST   stMatch;
    HASH_NODE_S             *pstHashNode = NULL;

    stMatch.ulPackageID = ulPackageID;
    stMatch.ucServType = ucServType;

    bs_trace(BS_TRACE_RUN, LOG_LEVEL_INFO, "Get billing package %d for service %d", ulPackageID, ucServType);

    ulHashIndex = bs_hash_get_index(BS_HASH_TBL_BILLING_PACKAGE_SIZE, ulPackageID);
    if (U32_BUTT == ulHashIndex)
    {
        DOS_ASSERT(0);
        return NULL;
    }

    pthread_mutex_lock(&g_mutexBillingPackageTbl);
    pstHashNode = hash_find_node(g_astBillingPackageTbl, ulHashIndex,
                                  (VOID *)&stMatch,
                                  bs_billing_package_hash_node_match);
    pthread_mutex_unlock(&g_mutexBillingPackageTbl);
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        return NULL;
    }

    return (BS_BILLING_PACKAGE_ST *)pstHashNode->pHandle;
}

/* �жϼƷѹ����Ƿ�ǡ�� */
BOOL bs_billing_rule_is_properly(BS_BILLING_RULE_ST  *pstRule)
{
    BOOL    bIsProperly = DOS_FALSE;
    U32     ulMatch;

    /* ����ǰָ��Ϸ�����ȷ��,�����ж� */

    if (0 == pstRule->ulPackageID
        || U32_BUTT == pstRule->ulPackageID
        || 0 == pstRule->ulRuleID
        || U32_BUTT == pstRule->ulRuleID)
    {
        /* �Ʒѹ���δ���� */
        return bIsProperly;
    }

    if (0 == pstRule->ucServType)
    {
        /* ��������δ���� */
        return bIsProperly;
    }

    /* Ϊ��߱Ƚ�Ч��,ʹ��λ�����ļ��㷽ʽ;Ǳ��Ҫ��:����ֵ���ó���31 */
    switch (pstRule->ucBillingType)
    {
        case BS_BILLING_BY_TIMELEN:
            ulMatch = (1<<BS_BILLING_ATTR_REGION)|(1<<BS_BILLING_ATTR_TEL_OPERATOR)
                      |(1<<BS_BILLING_ATTR_NUMBER_TYPE)|(1<<BS_BILLING_ATTR_TIME)
                      |(1<<BS_BILLING_ATTR_ACCUMULATE_TIMELEN);
            if (((0 == pstRule->ucSrcAttrType1) || ((1<<pstRule->ucSrcAttrType1)&ulMatch))
                && ((0 == pstRule->ucSrcAttrType2) || ((1<<pstRule->ucSrcAttrType2)&ulMatch))
                && ((0 == pstRule->ucDstAttrType1) || ((1<<pstRule->ucDstAttrType1)&ulMatch))
                && ((0 == pstRule->ucDstAttrType2) || ((1<<pstRule->ucDstAttrType2)&ulMatch)))
            {
                bIsProperly = DOS_TRUE;
            }
            break;

        case BS_BILLING_BY_COUNT:
            ulMatch = (1<<BS_BILLING_ATTR_REGION)|(1<<BS_BILLING_ATTR_TEL_OPERATOR)
                      |(1<<BS_BILLING_ATTR_NUMBER_TYPE)|(1<<BS_BILLING_ATTR_TIME)
                      |(1<<BS_BILLING_ATTR_ACCUMULATE_COUNT);
            if (((0 == pstRule->ucSrcAttrType1) || ((1<<pstRule->ucSrcAttrType1)&ulMatch))
                && ((0 == pstRule->ucSrcAttrType2) || ((1<<pstRule->ucSrcAttrType2)&ulMatch))
                && ((0 == pstRule->ucDstAttrType1) || ((1<<pstRule->ucDstAttrType1)&ulMatch))
                && ((0 == pstRule->ucDstAttrType2) || ((1<<pstRule->ucDstAttrType2)&ulMatch)))
            {
                bIsProperly = DOS_TRUE;
            }
            break;

        case BS_BILLING_BY_TRAFFIC:
            /* �ݲ�֧�� */
            break;

        case BS_BILLING_BY_CYCLE:
            ulMatch = (1<<BS_BILLING_ATTR_CONCURRENT)
                      |(1<<BS_BILLING_ATTR_SET)
                      |(1<<BS_BILLING_ATTR_RESOURCE_AGENT)
                      |(1<<BS_BILLING_ATTR_RESOURCE_NUMBER)
                      |(1<<BS_BILLING_ATTR_RESOURCE_LINE);

            /* pstRule->ucSrcAttrType1 Ϊ�ƷѶ���(��ϯ����·����)�� pstRule->ucSrcAttrType2Ϊ����(�գ��ܣ���)*/
            /* ���pstRule->ucSrcAttrType2���ܲ�������ıȽ� */
            if (BS_SERV_RENT == pstRule->ucServType
                && ((0 == pstRule->ucSrcAttrType1) || ((1<<pstRule->ucSrcAttrType1)&ulMatch))
                && ((0 == pstRule->ucDstAttrType1) || ((1<<pstRule->ucDstAttrType1)&ulMatch))
                && ((0 == pstRule->ucDstAttrType2) || ((1<<pstRule->ucDstAttrType2)&ulMatch))
                && pstRule->ulBillingRate != 0)
            {
                bIsProperly = DOS_TRUE;
            }
            break;

        default:
            break;
    }

    return bIsProperly;

}

/* ��ȡ�Ʒ����Բ������� */
VOID *bs_get_attr_input(BS_BILLING_MATCH_ST *pstBillingMatch, U8 ucAttrType, BOOL bIsSrc)
{
    VOID    *pAddr = NULL;

    if (DOS_ADDR_INVALID(pstBillingMatch))
    {
        DOS_ASSERT(0);
        return pAddr;
    }

    if (U8_BUTT == ucAttrType && 0 == ucAttrType)
    {
        return NULL;
    }

    switch (ucAttrType)
    {
        case BS_BILLING_ATTR_REGION:
        case BS_BILLING_ATTR_TEL_OPERATOR:
        case BS_BILLING_ATTR_NUMBER_TYPE:
            if (bIsSrc)
            {
                pAddr = (VOID *)pstBillingMatch->szCaller;
            }
            else
            {
                pAddr = (VOID *)pstBillingMatch->szCallee;
            }
            break;

        case BS_BILLING_ATTR_TIME:
            pAddr = (VOID *)&pstBillingMatch->ulTimeStamp;
            break;

        case BS_BILLING_ATTR_ACCUMULATE_TIMELEN:
        case BS_BILLING_ATTR_ACCUMULATE_COUNT:
        case BS_BILLING_ATTR_ACCUMULATE_TRAFFIC:
            /* �ۼ��� */
            pAddr = (VOID *)pstBillingMatch;
            break;

        case BS_BILLING_ATTR_CONCURRENT:
        case BS_BILLING_ATTR_SET:
        case BS_BILLING_ATTR_RESOURCE_AGENT:
        case BS_BILLING_ATTR_RESOURCE_NUMBER:
        case BS_BILLING_ATTR_RESOURCE_LINE:
            /* ��Դ���Ƿ��䵽�ͻ��� */
            pAddr = (VOID *)&pstBillingMatch->ulComsumerID;
            break;

        default:
            break;

    }

    return pAddr;

}

/* ��ȡ��������� */
U32 bs_get_number_region(S8 *szNum, U32 ulStrLen)
{
    if (DOS_ADDR_INVALID(szNum))
    {
        DOS_ASSERT(0);
        return U32_BUTT;
    }

    //TODO��ѯ���ݿ�,��ȡ�����������Ϣ;�ر��,�����������Ϣ���һ���Զ����ڴ���ʹ��

    /*
    ����ֵ32bit����:���Ҵ���(32-17bit),ʡ���(16-11bit),����(10-1bit)
    #define BS_MASK_REGION_COUNTRY          0xFFFF0000
    #define BS_MASK_REGION_PROVINCE         0xFC00
    #define BS_MASK_REGION_CITY             0x3FF
    */

    return U32_BUTT;
}

/* ��ȡ��������������Ӫ�� */
U32 bs_get_number_teloperator(S8 *szNum, U32 ulStrLen)
{
    if (DOS_ADDR_INVALID(szNum))
    {
        DOS_ASSERT(0);
        return BS_OPERATOR_UNKNOWN;
    }

    //TODO��ѯ���ݿ�,��ȡ����������Ӫ����Ϣ


    return BS_OPERATOR_UNKNOWN;
}

/* ��ȡ�������� */
U32 bs_get_number_type(S8 *szNum, U32 ulStrLen)
{
    if (DOS_ADDR_INVALID(szNum))
    {
        DOS_ASSERT(0);
        return BS_NUMBER_TYPE_BUTT;
    }

    //TODO��ѯ���ݿ�,��ȡ����������Ӫ����Ϣ


    return BS_NUMBER_TYPE_BUTT;
}

/* �ж�ʱ���Ƿ����ض���ʱ���� */
BOOL bs_time_is_in_period(U32 *pulTimeStamp, U32 ulPeriodID)
{
    if (DOS_ADDR_INVALID(pulTimeStamp))
    {
        DOS_ASSERT(0);
        return DOS_FALSE;
    }

    //TODO����ʱ��ID��ѯ���ݿ�,��ȡʱ�ζ���

    /* ע��:ʱ�ζ��岻һ����������,����ÿ�̶ܹ�ʱ��,ÿ��̶�ʱ��ε� */

    return BS_NUMBER_TYPE_BUTT;
}

/* ��ȡ�ۼƹ��� */
BS_ACCUMULATE_RULE *bs_get_accumulate_rule(U32 ulRuleID)
{
    //TODO����ID��ѯ���ݿ�,��ȡ�ۼƹ�����(���ȡ���ڴ�)


    return NULL;
}

/* �ж�ͨ����ֹԭ���Ƿ�Ϊ����æ */
BOOL bs_cause_is_busy(U16 usCause)
{
    //TODO
    return DOS_FALSE;
}

/* �ж�ͨ����ֹԭ���Ƿ�Ϊ���в����� */
BOOL bs_cause_is_not_exist(U16 usCause)
{
    //TODO
    return DOS_FALSE;
}

/* �ж�ͨ����ֹԭ���Ƿ�Ϊ������Ӧ�� */
BOOL bs_cause_is_no_answer(U16 usCause)
{
    //TODO
    return DOS_FALSE;
}

/* �ж�ͨ����ֹԭ���Ƿ�Ϊ���оܾ� */
BOOL bs_cause_is_reject(U16 usCause)
{
    //TODO
    return DOS_FALSE;
}

/* ����ͳ��ˢ�� */
VOID bs_outband_stat_refresh(BS_STAT_OUTBAND *pstDst, BS_STAT_OUTBAND *pstSrc)
{
    pstDst->ulTimeStamp = pstSrc->ulTimeStamp;
    pstDst->ulCallCnt += pstSrc->ulCallCnt;
    pstDst->ulRingCnt += pstSrc->ulRingCnt;
    pstDst->ulBusyCnt += pstSrc->ulBusyCnt;
    pstDst->ulNotExistCnt += pstSrc->ulNotExistCnt;
    pstDst->ulNoAnswerCnt += pstSrc->ulNoAnswerCnt;
    pstDst->ulRejectCnt += pstSrc->ulRejectCnt;
    pstDst->ulEarlyReleaseCnt += pstSrc->ulEarlyReleaseCnt;
    pstDst->ulAnswerCnt += pstSrc->ulAnswerCnt;
    pstDst->ulPDD += pstSrc->ulPDD;
    pstDst->ulAnswerTime += pstSrc->ulAnswerTime;

}

/* ���ͳ��ˢ�� */
VOID bs_inband_stat_refresh(BS_STAT_INBAND *pstDst, BS_STAT_INBAND *pstSrc)
{
    pstDst->ulTimeStamp = pstSrc->ulTimeStamp;
    pstDst->ulCallCnt += pstSrc->ulCallCnt;
    pstDst->ulRingCnt += pstSrc->ulRingCnt;
    pstDst->ulBusyCnt += pstSrc->ulBusyCnt;
    pstDst->ulNoAnswerCnt += pstSrc->ulNoAnswerCnt;
    pstDst->ulEarlyReleaseCnt += pstSrc->ulEarlyReleaseCnt;
    pstDst->ulAnswerCnt += pstSrc->ulAnswerCnt;
    pstDst->ulConnAgentCnt += pstSrc->ulConnAgentCnt;
    pstDst->ulAgentAnswerCnt += pstSrc->ulAgentAnswerCnt;
    pstDst->ulHoldCnt += pstSrc->ulHoldCnt;
    pstDst->ulAnswerTime += pstSrc->ulAnswerTime;
    pstDst->ulWaitAgentTime += pstSrc->ulWaitAgentTime;
    pstDst->ulAgentAnswerTime += pstSrc->ulAgentAnswerTime;
    pstDst->ulHoldTime += pstSrc->ulHoldTime;

}

/* ���ͳ��ˢ�� */
VOID bs_outdialing_stat_refresh(BS_STAT_OUTDIALING *pstDst, BS_STAT_OUTDIALING *pstSrc)
{
    pstDst->ulTimeStamp = pstSrc->ulTimeStamp;
    pstDst->ulCallCnt += pstSrc->ulCallCnt;
    pstDst->ulRingCnt += pstSrc->ulRingCnt;
    pstDst->ulBusyCnt += pstSrc->ulBusyCnt;
    pstDst->ulNotExistCnt += pstSrc->ulNotExistCnt;
    pstDst->ulNoAnswerCnt += pstSrc->ulNoAnswerCnt;
    pstDst->ulRejectCnt += pstSrc->ulRejectCnt;
    pstDst->ulEarlyReleaseCnt += pstSrc->ulEarlyReleaseCnt;
    pstDst->ulAnswerCnt += pstSrc->ulAnswerCnt;
    pstDst->ulPDD += pstSrc->ulPDD;
    pstDst->ulAnswerTime += pstSrc->ulAnswerTime;
    pstDst->ulWaitAgentTime += pstSrc->ulWaitAgentTime;
    pstDst->ulAgentAnswerTime += pstSrc->ulAgentAnswerTime;

}

/* ����ͳ��ˢ�� */
VOID bs_message_stat_refresh(BS_STAT_MESSAGE *pstDst, BS_STAT_MESSAGE *pstSrc)
{
    pstDst->ulTimeStamp = pstSrc->ulTimeStamp;
    pstDst->ulSendCnt += pstSrc->ulSendCnt;
    pstDst->ulRecvCnt += pstSrc->ulRecvCnt;
    pstDst->ulSendSucc += pstSrc->ulSendSucc;
    pstDst->ulSendFail += pstSrc->ulSendFail;
    pstDst->ulSendLen += pstSrc->ulSendLen;
    pstDst->ulRecvLen += pstSrc->ulRecvLen;

}

/* �˻�ͳ��ˢ�� */
VOID bs_account_stat_refresh(BS_ACCOUNT_ST *pstAccount,
                                  S32 lFee,
                                  S32 lPorfit,
                                  U8 ucServType)
{
    BS_STAT_ACCOUNT_ST  *pstStat;

    if (DOS_ADDR_INVALID(pstAccount))
    {
        DOS_ASSERT(0);
        return;
    }

    pstStat = &pstAccount->stStat;
    pstStat->ulTimeStamp = g_stBssCB.ulStatDayBase;
    pstStat->lProfit += lPorfit;
    switch (ucServType)
    {
        case BS_SERV_OUTBAND_CALL:
            pstStat->lOutBandCallFee += lFee;
            break;

        case BS_SERV_INBAND_CALL:
            pstStat->lInBandCallFee += lFee;
            break;

        case BS_SERV_AUTO_DIALING:
            pstStat->lAutoDialingFee += lFee;
            break;

        case BS_SERV_PREVIEW_DIALING:
            pstStat->lPreviewDailingFee += lFee;
            break;

        case BS_SERV_PREDICTIVE_DIALING:
            pstStat->lPredictiveDailingFee += lFee;
            break;

        case BS_SERV_RECORDING:
            pstStat->lRecordFee += lFee;
            break;

        case BS_SERV_CONFERENCE:
            pstStat->lConferenceFee += lFee;
            break;

        case BS_SERV_SMS_SEND:
        case BS_SERV_SMS_RECV:
            pstStat->lSmsFee += lFee;
            break;

        case BS_SERV_MMS_SEND:
        case BS_SERV_MMS_RECV:
            pstStat->lMmsFee += lFee;
            break;

        case BS_SERV_RENT:
            pstStat->lRentFee += lFee;
            break;

        default:
            break;
    }

    bs_trace(BS_TRACE_STAT, LOG_LEVEL_DEBUG,
             "Refresh account stat, "
             "account id:%u, time stamp:%u, profit:%d, "
             "outband fee:%d, inband fee:%d, auto dialing fee:%d, preview dialing fee:%d, "
             "predictive fee:%d, record fee:%d, sms fee:%d, mms fee:%d, rent fee:%d",
             pstAccount->ulAccountID,
             pstStat->ulTimeStamp, pstStat->lProfit,
             pstStat->lOutBandCallFee, pstStat->lInBandCallFee,
             pstStat->lAutoDialingFee, pstStat->lPreviewDailingFee,
             pstStat->lPredictiveDailingFee, pstStat->lRecordFee,
             pstStat->lSmsFee, pstStat->lMmsFee,
             pstStat->lRentFee);

}

/* ���ֺ���ͳ�� */
VOID bs_stat_outband(BS_CDR_VOICE_ST *pstCDR)
{
    U8                  ucPos;
    BS_CUSTOMER_ST      *pstCustomer = NULL;
    BS_AGENT_ST         *pstAgent = NULL;
    BS_TASK_ST          *pstTask = NULL;
    BS_SETTLE_ST        *pstSettle = NULL;
    BS_STAT_OUTBAND     *pstStat = NULL;
    BS_STAT_OUTBAND     stStat;

    /* ȷ��ͳ�Ƶ������±� */
    if (g_stBssCB.ulHour&0x1)
    {
        ucPos = 1;
    }
    else
    {
        ucPos = 0;
    }

    dos_memzero(&stStat, sizeof(stStat));
    stStat.ulTimeStamp = g_stBssCB.ulStatHourBase;
    stStat.ulCallCnt = 1;
    if (pstCDR->ulPDDLen != 0)
    {
        stStat.ulRingCnt = 1;
        stStat.ulPDD = pstCDR->ulPDDLen;
    }

    if (bs_cause_is_busy(pstCDR->usTerminateCause))
    {
        stStat.ulBusyCnt = 1;
    }
    else if (bs_cause_is_not_exist(pstCDR->usTerminateCause))
    {
        stStat.ulNotExistCnt = 1;
    }
    else if (bs_cause_is_no_answer(pstCDR->usTerminateCause))
    {
        stStat.ulNoAnswerCnt = 1;
    }
    else if (bs_cause_is_reject(pstCDR->usTerminateCause))
    {
        stStat.ulRejectCnt = 1;
    }

    if (pstCDR->ulTimeLen > 0)
    {
        stStat.ulAnswerCnt = 1;
        stStat.ulAnswerTime = pstCDR->ulTimeLen;
    }
    else if (BS_SESS_RELEASE_BY_CALLER == pstCDR->ucReleasePart)
    {
        stStat.ulEarlyReleaseCnt = 1;
    }

    if (pstCDR->ulCustomerID != 0 && pstCDR->ulCustomerID != U32_BUTT)
    {
        pstCustomer = bs_get_customer_st(pstCDR->ulCustomerID);
        if (pstCustomer != NULL)
        {
            pstStat = &pstCustomer->stStat.astOutBand[ucPos];
            bs_outband_stat_refresh(pstStat, &stStat);

            bs_trace(BS_TRACE_STAT, LOG_LEVEL_DEBUG,
                     "Refresh customer stat, "
                     "serv type:%u, customer id:%u, time stamp:%u, call cnt:%u, "
                     "ring cnt:%u, busy cnt:%u, not exist cnt:%u, no answer cnt:%u, "
                     "reject cnt:%u, early release cnt:%u, answer cnt:%u, total pdd:%u, "
                     "total answer time:%u",
                     pstCDR->ucServType, pstCDR->ulCustomerID,
                     pstStat->ulTimeStamp, pstStat->ulCallCnt,
                     pstStat->ulRingCnt, pstStat->ulBusyCnt,
                     pstStat->ulNotExistCnt, pstStat->ulNoAnswerCnt,
                     pstStat->ulRejectCnt, pstStat->ulEarlyReleaseCnt,
                     pstStat->ulAnswerCnt, pstStat->ulPDD,
                     pstStat->ulAnswerTime);
        }
    }

    if (pstCDR->ulAgentID != 0 && pstCDR->ulAgentID != U32_BUTT)
    {
        pstAgent = bs_get_agent_st(pstCDR->ulAgentID);
        if (pstAgent != NULL)
        {
            pstStat = &pstAgent->stStat.astOutBand[ucPos];
            bs_outband_stat_refresh(pstStat, &stStat);

            bs_trace(BS_TRACE_STAT, LOG_LEVEL_DEBUG,
                     "Refresh agent stat, "
                     "serv type:%u, agent id:%u, time stamp:%u, call cnt:%u, "
                     "ring cnt:%u, busy cnt:%u, not exist cnt:%u, no answer cnt:%u, "
                     "reject cnt:%u, early release cnt:%u, answer cnt:%u, total pdd:%u, "
                     "total answer time:%u",
                     pstCDR->ucServType, pstCDR->ulAgentID,
                     pstStat->ulTimeStamp, pstStat->ulCallCnt,
                     pstStat->ulRingCnt, pstStat->ulBusyCnt,
                     pstStat->ulNotExistCnt, pstStat->ulNoAnswerCnt,
                     pstStat->ulRejectCnt, pstStat->ulEarlyReleaseCnt,
                     pstStat->ulAnswerCnt, pstStat->ulPDD,
                     pstStat->ulAnswerTime);
        }
    }

    if (pstCDR->ulTaskID != 0 && pstCDR->ulTaskID != U32_BUTT)
    {
        pstTask = bs_get_task_st(pstCDR->ulTaskID);
        if (pstTask != NULL)
        {
            pstStat = &pstTask->stStat.astOutBand[ucPos];
            bs_outband_stat_refresh(pstStat, &stStat);

            bs_trace(BS_TRACE_STAT, LOG_LEVEL_DEBUG,
                     "Refresh task stat, "
                     "serv type:%u, task id:%u, time stamp:%u, call cnt:%u, "
                     "ring cnt:%u, busy cnt:%u, not exist cnt:%u, no answer cnt:%u, "
                     "reject cnt:%u, early release cnt:%u, answer cnt:%u, total pdd:%u, "
                     "total answer time:%u",
                     pstCDR->ucServType, pstCDR->ulTaskID,
                     pstStat->ulTimeStamp, pstStat->ulCallCnt,
                     pstStat->ulRingCnt, pstStat->ulBusyCnt,
                     pstStat->ulNotExistCnt, pstStat->ulNoAnswerCnt,
                     pstStat->ulRejectCnt, pstStat->ulEarlyReleaseCnt,
                     pstStat->ulAnswerCnt, pstStat->ulPDD,
                     pstStat->ulAnswerTime);
        }
    }

    if (pstCDR->usPeerTrunkID != 0 && pstCDR->usPeerTrunkID != U16_BUTT)
    {
        pstSettle = bs_get_settle_st(pstCDR->usPeerTrunkID);
        if (pstSettle != NULL)
        {
            pstStat = &pstSettle->stStat.astOutBand[ucPos];
            bs_outband_stat_refresh(pstStat, &stStat);

            bs_trace(BS_TRACE_STAT, LOG_LEVEL_DEBUG,
                     "Refresh trunk stat, "
                     "serv type:%u, trunk id:%u, time stamp:%u, call cnt:%u, "
                     "ring cnt:%u, busy cnt:%u, not exist cnt:%u, no answer cnt:%u, "
                     "reject cnt:%u, early release cnt:%u, answer cnt:%u, total pdd:%u, "
                     "total answer time:%u",
                     pstCDR->ucServType, pstCDR->usPeerTrunkID,
                     pstStat->ulTimeStamp, pstStat->ulCallCnt,
                     pstStat->ulRingCnt, pstStat->ulBusyCnt,
                     pstStat->ulNotExistCnt, pstStat->ulNoAnswerCnt,
                     pstStat->ulRejectCnt, pstStat->ulEarlyReleaseCnt,
                     pstStat->ulAnswerCnt, pstStat->ulPDD,
                     pstStat->ulAnswerTime);
        }
    }

}

/* ��ֺ���ͳ�� */
VOID bs_stat_inband(BS_CDR_VOICE_ST *pstCDR)
{
    U8                  ucPos;
    BS_CUSTOMER_ST      *pstCustomer = NULL;
    BS_AGENT_ST         *pstAgent = NULL;
    BS_TASK_ST          *pstTask = NULL;
    BS_SETTLE_ST        *pstSettle = NULL;
    BS_STAT_INBAND      *pstStat = NULL;
    BS_STAT_INBAND      stStat;

    /* ȷ��ͳ�Ƶ������±� */
    if (g_stBssCB.ulHour&0x1)
    {
        ucPos = 1;
    }
    else
    {
        ucPos = 0;
    }

    dos_memzero(&stStat, sizeof(stStat));
    stStat.ulTimeStamp = g_stBssCB.ulStatHourBase;
    stStat.ulCallCnt = 1;
    if (pstCDR->ulPDDLen != 0)
    {
        stStat.ulRingCnt = 1;
    }

    if (bs_cause_is_busy(pstCDR->usTerminateCause))
    {
        stStat.ulBusyCnt = 1;
    }
    else if (bs_cause_is_no_answer(pstCDR->usTerminateCause))
    {
        stStat.ulNoAnswerCnt = 1;
    }

    if (pstCDR->ulTimeLen > 0)
    {
        stStat.ulAnswerCnt = 1;
        stStat.ulAgentAnswerCnt = 1;
        stStat.ulAnswerTime = pstCDR->ulTimeLen;
        stStat.ulAgentAnswerTime = pstCDR->ulTimeLen;
    }
    else if (BS_SESS_RELEASE_BY_CALLER == pstCDR->ucReleasePart)
    {
        stStat.ulEarlyReleaseCnt = 1;
    }

    if (pstCDR->ulAgentID != 0 && pstCDR->ulAgentID != U32_BUTT)
    {
        stStat.ulConnAgentCnt = 1;
    }

    stStat.ulHoldCnt = pstCDR->ulHoldCnt;
    stStat.ulWaitAgentTime = pstCDR->ulWaitAgentTime;
    stStat.ulHoldTime = pstCDR->ulHoldTimeLen;

    if (pstCDR->ulCustomerID != 0 && pstCDR->ulCustomerID != U32_BUTT)
    {
        pstCustomer = bs_get_customer_st(pstCDR->ulCustomerID);
        if (pstCustomer != NULL)
        {
            pstStat = &pstCustomer->stStat.astInBand[ucPos];
            bs_inband_stat_refresh(pstStat, &stStat);

            bs_trace(BS_TRACE_STAT, LOG_LEVEL_DEBUG,
                     "Refresh customer stat, "
                     "serv type:%u, customer id:%u, time stamp:%u, call cnt:%u, "
                     "ring cnt:%u, busy cnt:%u, no answer cnt:%u, early release cnt:%u, "
                     "answer cnt:%u, conn agent cnt:%u, agent answer cnt:%u, hold cnt:%u, "
                     "total answer time:%u, total wait angent time:%u, "
                     "total agent answer time:%u, total hold time:%u",
                     pstCDR->ucServType, pstCDR->ulCustomerID,
                     pstStat->ulTimeStamp, pstStat->ulCallCnt,
                     pstStat->ulRingCnt, pstStat->ulBusyCnt,
                     pstStat->ulNoAnswerCnt, pstStat->ulEarlyReleaseCnt,
                     pstStat->ulAnswerCnt, pstStat->ulConnAgentCnt,
                     pstStat->ulAgentAnswerCnt, pstStat->ulHoldCnt,
                     pstStat->ulAnswerTime, pstStat->ulWaitAgentTime,
                     pstStat->ulAgentAnswerTime, pstStat->ulHoldTime);
        }
    }

    if (pstCDR->ulAgentID != 0 && pstCDR->ulAgentID != U32_BUTT)
    {
        pstAgent = bs_get_agent_st(pstCDR->ulAgentID);
        if (pstAgent != NULL)
        {
            pstStat = &pstAgent->stStat.astInBand[ucPos];
            bs_inband_stat_refresh(pstStat, &stStat);

            bs_trace(BS_TRACE_STAT, LOG_LEVEL_DEBUG,
                     "Refresh agent stat, "
                     "serv type:%u, agent id:%u, time stamp:%u, call cnt:%u, "
                     "ring cnt:%u, busy cnt:%u, no answer cnt:%u, early release cnt:%u, "
                     "answer cnt:%u, conn agent cnt:%u, agent answer cnt:%u, hold cnt:%u, "
                     "total answer time:%u, total wait angent time:%u, "
                     "total agent answer time:%u, total hold time:%u",
                     pstCDR->ucServType, pstCDR->ulAgentID,
                     pstStat->ulTimeStamp, pstStat->ulCallCnt,
                     pstStat->ulRingCnt, pstStat->ulBusyCnt,
                     pstStat->ulNoAnswerCnt, pstStat->ulEarlyReleaseCnt,
                     pstStat->ulAnswerCnt, pstStat->ulConnAgentCnt,
                     pstStat->ulAgentAnswerCnt, pstStat->ulHoldCnt,
                     pstStat->ulAnswerTime, pstStat->ulWaitAgentTime,
                     pstStat->ulAgentAnswerTime, pstStat->ulHoldTime);
        }
    }

    if (pstCDR->ulTaskID != 0 && pstCDR->ulTaskID != U32_BUTT)
    {
        pstTask = bs_get_task_st(pstCDR->ulTaskID);
        if (pstTask != NULL)
        {
            pstStat = &pstTask->stStat.astInBand[ucPos];
            bs_inband_stat_refresh(pstStat, &stStat);

            bs_trace(BS_TRACE_STAT, LOG_LEVEL_DEBUG,
                     "Refresh task stat, "
                     "serv type:%u, task id:%u, time stamp:%u, call cnt:%u, "
                     "ring cnt:%u, busy cnt:%u, no answer cnt:%u, early release cnt:%u, "
                     "answer cnt:%u, conn agent cnt:%u, agent answer cnt:%u, hold cnt:%u, "
                     "total answer time:%u, total wait angent time:%u, "
                     "total agent answer time:%u, total hold time:%u",
                     pstCDR->ucServType, pstCDR->ulTaskID,
                     pstStat->ulTimeStamp, pstStat->ulCallCnt,
                     pstStat->ulRingCnt, pstStat->ulBusyCnt,
                     pstStat->ulNoAnswerCnt, pstStat->ulEarlyReleaseCnt,
                     pstStat->ulAnswerCnt, pstStat->ulConnAgentCnt,
                     pstStat->ulAgentAnswerCnt, pstStat->ulHoldCnt,
                     pstStat->ulAnswerTime, pstStat->ulWaitAgentTime,
                     pstStat->ulAgentAnswerTime, pstStat->ulHoldTime);
        }
    }

    if (pstCDR->usPeerTrunkID != 0 && pstCDR->usPeerTrunkID != U16_BUTT)
    {
        pstSettle = bs_get_settle_st(pstCDR->usPeerTrunkID);
        if (pstSettle != NULL)
        {
            pstStat = &pstSettle->stStat.astInBand[ucPos];
            bs_inband_stat_refresh(pstStat, &stStat);

            bs_trace(BS_TRACE_STAT, LOG_LEVEL_DEBUG,
                     "Refresh trunk stat, "
                     "serv type:%u, trunk id:%u, time stamp:%u, call cnt:%u, "
                     "ring cnt:%u, busy cnt:%u, no answer cnt:%u, early release cnt:%u, "
                     "answer cnt:%u, conn agent cnt:%u, agent answer cnt:%u, hold cnt:%u, "
                     "total answer time:%u, total wait angent time:%u, "
                     "total agent answer time:%u, total hold time:%u",
                     pstCDR->ucServType, pstCDR->usPeerTrunkID,
                     pstStat->ulTimeStamp, pstStat->ulCallCnt,
                     pstStat->ulRingCnt, pstStat->ulBusyCnt,
                     pstStat->ulNoAnswerCnt, pstStat->ulEarlyReleaseCnt,
                     pstStat->ulAnswerCnt, pstStat->ulConnAgentCnt,
                     pstStat->ulAgentAnswerCnt, pstStat->ulHoldCnt,
                     pstStat->ulAnswerTime, pstStat->ulWaitAgentTime,
                     pstStat->ulAgentAnswerTime, pstStat->ulHoldTime);
        }
    }

}

/* ���ͳ�� */
VOID bs_stat_outdialing(BS_CDR_VOICE_ST *pstCDR)
{
    U8                  ucPos;
    BS_CUSTOMER_ST      *pstCustomer = NULL;
    BS_AGENT_ST         *pstAgent = NULL;
    BS_TASK_ST          *pstTask = NULL;
    BS_SETTLE_ST        *pstSettle = NULL;
    BS_STAT_OUTDIALING  *pstStat = NULL;
    BS_STAT_OUTDIALING  stStat;

    /* ȷ��ͳ�Ƶ������±� */
    if (g_stBssCB.ulHour&0x1)
    {
        ucPos = 1;
    }
    else
    {
        ucPos = 0;
    }

    dos_memzero(&stStat, sizeof(stStat));
    stStat.ulTimeStamp = g_stBssCB.ulStatHourBase;
    stStat.ulCallCnt = 1;
    if (pstCDR->ulPDDLen != 0)
    {
        stStat.ulRingCnt = 1;
        stStat.ulPDD = pstCDR->ulPDDLen;
    }

    if (bs_cause_is_busy(pstCDR->usTerminateCause))
    {
        stStat.ulBusyCnt = 1;
    }
    else if (bs_cause_is_not_exist(pstCDR->usTerminateCause))
    {
        stStat.ulNotExistCnt = 1;
    }
    else if (bs_cause_is_no_answer(pstCDR->usTerminateCause))
    {
        stStat.ulNoAnswerCnt = 1;
    }
    else if (bs_cause_is_reject(pstCDR->usTerminateCause))
    {
        stStat.ulRejectCnt = 1;
    }

    if (pstCDR->ulTimeLen > 0)
    {
        stStat.ulAnswerCnt = 1;
        stStat.ulAnswerTime = pstCDR->ulTimeLen;
        stStat.ulAgentAnswerTime = pstCDR->ulTimeLen;
    }
    else if (BS_SESS_RELEASE_BY_CALLER == pstCDR->ucReleasePart)
    {
        stStat.ulEarlyReleaseCnt = 1;
    }

    stStat.ulWaitAgentTime = pstCDR->ulWaitAgentTime;

    if (pstCDR->ulCustomerID != 0 && pstCDR->ulCustomerID != U32_BUTT)
    {
        pstCustomer = bs_get_customer_st(pstCDR->ulCustomerID);
        if (pstCustomer != NULL)
        {
            pstStat = &pstCustomer->stStat.astOutDialing[ucPos];
            bs_outdialing_stat_refresh(pstStat, &stStat);

            bs_trace(BS_TRACE_STAT, LOG_LEVEL_DEBUG,
                     "Refresh customer stat, "
                     "serv type:%u, customer id:%u, time stamp:%u, call cnt:%u, "
                     "ring cnt:%u, busy cnt:%u, not exist cnt:%u, no answer cnt:%u, "
                     "reject cnt:%u, early release cnt:%u, answer cnt:%u, conn agent cnt:%u, "
                     "agent answer cnt:%u, total pdd:%u, total answer time:%u, total wait angent time:%u, "
                     "total agent answer time:%u",
                     pstCDR->ucServType, pstCDR->ulCustomerID,
                     pstStat->ulTimeStamp, pstStat->ulCallCnt,
                     pstStat->ulRingCnt, pstStat->ulBusyCnt,
                     pstStat->ulNotExistCnt, pstStat->ulNoAnswerCnt,
                     pstStat->ulRejectCnt, pstStat->ulEarlyReleaseCnt,
                     pstStat->ulAnswerCnt, pstStat->ulConnAgentCnt,
                     pstStat->ulAgentAnswerCnt, pstStat->ulPDD,
                     pstStat->ulAnswerTime, pstStat->ulWaitAgentTime,
                     pstStat->ulAgentAnswerTime);
        }
    }

    if (pstCDR->ulAgentID != 0 && pstCDR->ulAgentID != U32_BUTT)
    {
        pstAgent = bs_get_agent_st(pstCDR->ulAgentID);
        if (pstAgent != NULL)
        {
            pstStat = &pstAgent->stStat.astOutDialing[ucPos];
            bs_outdialing_stat_refresh(pstStat, &stStat);

            bs_trace(BS_TRACE_STAT, LOG_LEVEL_DEBUG,
                     "Refresh agent stat, "
                     "serv type:%u, agent id:%u, time stamp:%u, call cnt:%u, "
                     "ring cnt:%u, busy cnt:%u, not exist cnt:%u, no answer cnt:%u, "
                     "reject cnt:%u, early release cnt:%u, answer cnt:%u, conn agent cnt:%u, "
                     "agent answer cnt:%u, total pdd:%u, total answer time:%u, total wait angent time:%u, "
                     "total agent answer time:%u",
                     pstCDR->ucServType, pstCDR->ulAgentID,
                     pstStat->ulTimeStamp, pstStat->ulCallCnt,
                     pstStat->ulRingCnt, pstStat->ulBusyCnt,
                     pstStat->ulNotExistCnt, pstStat->ulNoAnswerCnt,
                     pstStat->ulRejectCnt, pstStat->ulEarlyReleaseCnt,
                     pstStat->ulAnswerCnt, pstStat->ulConnAgentCnt,
                     pstStat->ulAgentAnswerCnt, pstStat->ulPDD,
                     pstStat->ulAnswerTime, pstStat->ulWaitAgentTime,
                     pstStat->ulAgentAnswerTime);
        }
    }

    if (pstCDR->ulTaskID != 0 && pstCDR->ulTaskID != U32_BUTT)
    {
        pstTask = bs_get_task_st(pstCDR->ulTaskID);
        if (pstTask != NULL)
        {
            pstStat = &pstTask->stStat.astOutDialing[ucPos];
            bs_outdialing_stat_refresh(pstStat, &stStat);

            bs_trace(BS_TRACE_STAT, LOG_LEVEL_DEBUG,
                     "Refresh task stat, "
                     "serv type:%u, task id:%u, time stamp:%u, call cnt:%u, "
                     "ring cnt:%u, busy cnt:%u, not exist cnt:%u, no answer cnt:%u, "
                     "reject cnt:%u, early release cnt:%u, answer cnt:%u, conn agent cnt:%u, "
                     "agent answer cnt:%u, total pdd:%u, total answer time:%u, total wait angent time:%u, "
                     "total agent answer time:%u",
                     pstCDR->ucServType, pstCDR->ulTaskID,
                     pstStat->ulTimeStamp, pstStat->ulCallCnt,
                     pstStat->ulRingCnt, pstStat->ulBusyCnt,
                     pstStat->ulNotExistCnt, pstStat->ulNoAnswerCnt,
                     pstStat->ulRejectCnt, pstStat->ulEarlyReleaseCnt,
                     pstStat->ulAnswerCnt, pstStat->ulConnAgentCnt,
                     pstStat->ulAgentAnswerCnt, pstStat->ulPDD,
                     pstStat->ulAnswerTime, pstStat->ulWaitAgentTime,
                     pstStat->ulAgentAnswerTime);
        }
    }

    if (pstCDR->usPeerTrunkID != 0 && pstCDR->usPeerTrunkID != U16_BUTT)
    {
        pstSettle = bs_get_settle_st(pstCDR->usPeerTrunkID);
        if (pstSettle != NULL)
        {
            pstStat = &pstSettle->stStat.astOutDialing[ucPos];
            bs_outdialing_stat_refresh(pstStat, &stStat);

            bs_trace(BS_TRACE_STAT, LOG_LEVEL_DEBUG,
                     "Refresh trunk stat, "
                     "serv type:%u, trunk id:%u, time stamp:%u, call cnt:%u, "
                     "ring cnt:%u, busy cnt:%u, not exist cnt:%u, no answer cnt:%u, "
                     "reject cnt:%u, early release cnt:%u, answer cnt:%u, conn agent cnt:%u, "
                     "agent answer cnt:%u, total pdd:%u, total answer time:%u, total wait angent time:%u, "
                     "total agent answer time:%u",
                     pstCDR->ucServType, pstCDR->usPeerTrunkID,
                     pstStat->ulTimeStamp, pstStat->ulCallCnt,
                     pstStat->ulRingCnt, pstStat->ulBusyCnt,
                     pstStat->ulNotExistCnt, pstStat->ulNoAnswerCnt,
                     pstStat->ulRejectCnt, pstStat->ulEarlyReleaseCnt,
                     pstStat->ulAnswerCnt, pstStat->ulConnAgentCnt,
                     pstStat->ulAgentAnswerCnt, pstStat->ulPDD,
                     pstStat->ulAnswerTime, pstStat->ulWaitAgentTime,
                     pstStat->ulAgentAnswerTime);
        }
    }

}

/* ����ҵ��ͳ�� */
VOID bs_stat_voice(BS_CDR_VOICE_ST *pstCDR)
{
    switch (pstCDR->ucServType)
    {
        case BS_SERV_OUTBAND_CALL:
            bs_stat_outband(pstCDR);
            break;

        case BS_SERV_INBAND_CALL:
            bs_stat_inband(pstCDR);
            break;

        case BS_SERV_AUTO_DIALING:
            bs_stat_outdialing(pstCDR);
            break;

        case BS_SERV_PREVIEW_DIALING:
            bs_stat_outdialing(pstCDR);
            break;

        case BS_SERV_PREDICTIVE_DIALING:
            bs_stat_outdialing(pstCDR);
            break;

        default:
            break;
    }
}

/* ��Ϣҵ��ͳ�� */
VOID bs_stat_message(BS_CDR_MS_ST *pstCDR)
{
    U8                  ucPos;
    BS_CUSTOMER_ST      *pstCustomer = NULL;
    BS_AGENT_ST         *pstAgent = NULL;
    BS_TASK_ST          *pstTask = NULL;
    BS_SETTLE_ST        *pstSettle = NULL;
    BS_STAT_MESSAGE     *pstStat = NULL;
    BS_STAT_MESSAGE     stStat;

    /* ȷ��ͳ�Ƶ������±� */
    if (g_stBssCB.ulHour&0x1)
    {
        ucPos = 1;
    }
    else
    {
        ucPos = 0;
    }

    dos_memzero(&stStat, sizeof(stStat));
    stStat.ulTimeStamp = g_stBssCB.ulStatHourBase;
    switch (pstCDR->ucServType)
    {
        case BS_SERV_SMS_SEND:
        case BS_SERV_MMS_SEND:
            stStat.ulSendCnt = 1;
            if (pstCDR->ulArrivedTimeStamp != 0)
            {
                stStat.ulSendSucc = 1;
            }
            else
            {
                stStat.ulSendFail= 1;
            }
            stStat.ulSendLen = pstCDR->ulLen;
            break;

        case BS_SERV_SMS_RECV:
        case BS_SERV_MMS_RECV:
            stStat.ulRecvCnt = 1;
            stStat.ulRecvLen = pstCDR->ulLen;
            break;

        default:
            break;
    }

    if (pstCDR->ulCustomerID != 0 && pstCDR->ulCustomerID != U32_BUTT)
    {
        pstCustomer = bs_get_customer_st(pstCDR->ulCustomerID);
        if (pstCustomer != NULL)
        {
            pstStat = &pstCustomer->stStat.astMS[ucPos];
            bs_message_stat_refresh(pstStat, &stStat);

            bs_trace(BS_TRACE_STAT, LOG_LEVEL_DEBUG,
                     "Refresh customer stat, "
                     "serv type:%u, customer id:%u, time stamp:%u, send cnt:%u, "
                     "recv cnt:%u, succ cnt:%u, fail cnt:%u, send len:%u, recv len:%u, ",
                     pstCDR->ucServType, pstCDR->ulCustomerID,
                     pstStat->ulTimeStamp, pstStat->ulSendCnt,
                     pstStat->ulRecvCnt, pstStat->ulSendSucc,
                     pstStat->ulSendFail, pstStat->ulSendLen,
                     pstStat->ulRecvLen);
        }
    }

    if (pstCDR->ulAgentID != 0 && pstCDR->ulAgentID != U32_BUTT)
    {
        pstAgent = bs_get_agent_st(pstCDR->ulAgentID);
        if (pstAgent != NULL)
        {
            pstStat = &pstAgent->stStat.astMS[ucPos];
            bs_message_stat_refresh(pstStat, &stStat);

            bs_trace(BS_TRACE_STAT, LOG_LEVEL_DEBUG,
                     "Refresh agent stat, "
                     "serv type:%u, agent id:%u, time stamp:%u, send cnt:%u, "
                     "recv cnt:%u, succ cnt:%u, fail cnt:%u, send len:%u, recv len:%u, ",
                     pstCDR->ucServType, pstCDR->ulAgentID,
                     pstStat->ulTimeStamp, pstStat->ulSendCnt,
                     pstStat->ulRecvCnt, pstStat->ulSendSucc,
                     pstStat->ulSendFail, pstStat->ulSendLen,
                     pstStat->ulRecvLen);
        }
    }

    if (pstCDR->ulTaskID != 0 && pstCDR->ulTaskID != U32_BUTT)
    {
        pstTask = bs_get_task_st(pstCDR->ulTaskID);
        if (pstTask != NULL)
        {
            pstStat = &pstTask->stStat.astMS[ucPos];
            bs_message_stat_refresh(pstStat, &stStat);

            bs_trace(BS_TRACE_STAT, LOG_LEVEL_DEBUG,
                     "Refresh task stat, "
                     "serv type:%u, task id:%u, time stamp:%u, send cnt:%u, "
                     "recv cnt:%u, succ cnt:%u, fail cnt:%u, send len:%u, recv len:%u, ",
                     pstCDR->ucServType, pstCDR->ulTaskID,
                     pstStat->ulTimeStamp, pstStat->ulSendCnt,
                     pstStat->ulRecvCnt, pstStat->ulSendSucc,
                     pstStat->ulSendFail, pstStat->ulSendLen,
                     pstStat->ulRecvLen);
        }
    }

    if (pstCDR->usPeerTrunkID != 0 && pstCDR->usPeerTrunkID != U16_BUTT)
    {
        pstSettle = bs_get_settle_st(pstCDR->usPeerTrunkID);
        if (pstSettle != NULL)
        {
            pstStat = &pstSettle->stStat.astMS[ucPos];
            bs_message_stat_refresh(pstStat, &stStat);

            bs_trace(BS_TRACE_STAT, LOG_LEVEL_DEBUG,
                     "Refresh trunk stat, "
                     "serv type:%u, trunk id:%u, time stamp:%u, send cnt:%u, "
                     "recv cnt:%u, succ cnt:%u, fail cnt:%u, send len:%u, recv len:%u, ",
                     pstCDR->ucServType, pstCDR->usPeerTrunkID,
                     pstStat->ulTimeStamp, pstStat->ulSendCnt,
                     pstStat->ulRecvCnt, pstStat->ulSendSucc,
                     pstStat->ulSendFail, pstStat->ulSendLen,
                     pstStat->ulRecvLen);
        }
    }

}


/* �����ۼƹ����ȡͳ��ֵ */
U32 bs_get_timelen_stat(BS_BILLING_MATCH_ST *pstBillingMatch)
{
    if (DOS_ADDR_INVALID(pstBillingMatch))
    {
        DOS_ASSERT(0);
        return 0;
    }

    //TODO��ѯ���ݿ�,��ȡͳ��ֵ


    /* ���ע��:�����ݲ���ɲ���,��Ҫ�������ݿ����Ч��,������˵����̳߳�ʱ������ */


    return 0;
}

/* �����ۼƹ����ȡͳ��ֵ */
U32 bs_get_count_stat(BS_BILLING_MATCH_ST *pstBillingMatch)
{
    if (DOS_ADDR_INVALID(pstBillingMatch))
    {
        DOS_ASSERT(0);
        return 0;
    }

    //TODO��ѯ���ݿ�,��ȡͳ��ֵ

    /* ���ע��:�����ݲ���ɲ���,��Ҫ�������ݿ����Ч��,������˵����̳߳�ʱ������ */

    return 0;
}

/* �����ۼƹ����ȡͳ��ֵ */
U32 bs_get_traffic_stat(BS_BILLING_MATCH_ST *pstBillingMatch)
{
    if (DOS_ADDR_INVALID(pstBillingMatch))
    {
        DOS_ASSERT(0);
        return 0;
    }

    //TODO��ѯ���ݿ�,��ȡͳ��ֵ

    /* ���ע��:�����ݲ���ɲ���,��Ҫ�������ݿ����Ч��,������˵����̳߳�ʱ������ */


    return 0;
}

/*
�Ʒ�����ֵƥ��
˵��:
1.�Ȼ�ȡ����ֵ,������ٽ���ƥ��;
2.���ݲ�ͬ���������,pInputָ��ͬ�Ĳ�����ַ;
3.pulValueָ�������Ӧ������ֵ��ַ,Ϊ���ⷴ����ѯ,���Դ�����Чֵ;
*/
S32 bs_match_billing_attr(U8 ucAttrType, U32 ulAttrValue, VOID *pInput, U32 *pulValue)
{
    S8                      *szNum;
    U32                     ulValue;
    S32                     lMatchResult = DOS_FAIL;
    U32                     ulStat;
    BS_ACCUMULATE_RULE      *pstAccuRule = NULL;
    BS_BILLING_MATCH_ST     *pstInput = NULL;

    if (DOS_ADDR_INVALID(pulValue))
    {
        DOS_ASSERT(0);
        return lMatchResult;
    }

    if (U8_BUTT == ucAttrType || 0 == ucAttrType)
    {
        /* ���޶����� */
        return DOS_SUCC;
    }

    if (*pulValue != U32_BUTT)
    {
        /* ������ѯ��,���治�ٽ��в�ѯ */
        ulValue = *pulValue;
    }
    else
    {
        ulValue = U32_BUTT;
        *pulValue = U32_BUTT;
    }

    switch (ucAttrType)
    {
        case BS_BILLING_ATTR_REGION:
            /*
            1.����������,32bit����:3���ֶ�,���Ҵ���(32-17bit),ʡ���(16-11bit),����(10-1bit)
            2.����������,ƥ�����:
              a.0-ͨ������;
              b.�ֶ�ֵ:0-ͨ���ֶδ�������е���;
              c.��ȫƥ��:�����ֶξ���ͬ;
            */
            if (0 == ulAttrValue)
            {
                /* ͨ������,�����ѯ */
                lMatchResult = DOS_SUCC;
            }
            else
            {
                szNum = (S8 *)pInput;
                if (DOS_ADDR_VALID(szNum)
                    && U32_BUTT == ulValue)
                {
                    ulValue = bs_get_number_region(szNum, BS_MAX_CALL_NUM_LEN);
                    *pulValue = ulValue;
                }

                if (U32_BUTT == ulValue)
                {
                    /* ��ѯʧ�� */
                    lMatchResult = DOS_FAIL;
                }
                else if (ulAttrValue == (ulAttrValue&ulValue))
                {
                    lMatchResult = DOS_SUCC;
                }
            }
            break;

        case BS_BILLING_ATTR_TEL_OPERATOR:
            /*
            3.��Ӫ������,ƥ�����:
              a.0-ͨ������;
              b.Ҫôͨ��,Ҫôƥ�䵥����Ӫ��;
            */
            if (0 == ulAttrValue)
            {
                /* ͨ������,�����ѯ */
                lMatchResult = DOS_SUCC;
            }
            else
            {
                szNum = (S8 *)pInput;
                if (DOS_ADDR_VALID(szNum)
                    && U32_BUTT == ulValue)
                {
                    ulValue = bs_get_number_teloperator(szNum, BS_MAX_CALL_NUM_LEN);
                    *pulValue = ulValue;
                }

                if (U32_BUTT == ulValue)
                {
                    /* ��ѯʧ�� */
                    lMatchResult = DOS_FAIL;
                }
                else if (ulAttrValue == ulValue)
                {
                    lMatchResult = DOS_SUCC;
                }
            }
            break;

        case BS_BILLING_ATTR_NUMBER_TYPE:
            /*
            4.������������,ƥ�����:
              a.0-ͨ������;
              b.Ҫôͨ��,Ҫôƥ�䵥����������;
            */
            if (0 == ulAttrValue)
            {
                /* ͨ������,�����ѯ */
                lMatchResult = DOS_SUCC;
            }
            else
            {
                szNum = (S8 *)pInput;
                if (DOS_ADDR_VALID(szNum)
                    && U32_BUTT == ulValue)
                {
                    ulValue = bs_get_number_type(szNum, BS_MAX_CALL_NUM_LEN);
                    *pulValue = ulValue;
                }

                if (U32_BUTT == ulValue)
                {
                    /* ��ѯʧ�� */
                    lMatchResult = DOS_FAIL;
                }
                else if (ulAttrValue == ulValue)
                {
                    lMatchResult = DOS_SUCC;
                }
            }
            break;

        case BS_BILLING_ATTR_TIME:
            /*
            5.ʱ������,ƥ�����:
              a.0-���޶�ʱ��;
              b.��������ֵ,��ѯ���ݿ�,�ж�ʱ���Ƿ������ݿ��ʱ�䶨����;
            */
            if (0 == ulAttrValue)
            {
                /* ���޶�ʱ��,�����ѯ */
                lMatchResult = DOS_SUCC;
            }
            else
            {
                if (ulAttrValue == ulValue)
                {
                    /* ����ƥ��� */
                    lMatchResult = DOS_SUCC;
                }
                else if (bs_time_is_in_period((U32 *)pInput, ulAttrValue))
                {
                    lMatchResult = DOS_SUCC;
                    *pulValue = ulAttrValue;
                }
            }
            break;

        case BS_BILLING_ATTR_ACCUMULATE_TIMELEN:
        case BS_BILLING_ATTR_ACCUMULATE_COUNT:
        case BS_BILLING_ATTR_ACCUMULATE_TRAFFIC:
            /*
            6.�ۼ�������,ƥ�����:
              a.��������ֵ��ѯ���ݿ�,��ȡ�ۼƹ���;
              b.�ۼƹ�������ۼƶ���/�ۼ�����/ҵ��/���ֵ;
              c.ͳ������;
              d.�ۼƶ���/�ۼ�����/ҵ����ͬ�������,ͳ������С�����ֵ��ƥ��ɹ�;
              e.���ֵΪ0,��ʾ���޶����ֵ;
            */

            if (ulValue != U32_BUTT && ulAttrValue == ulValue)
            {
                /* ����ƥ��� */
                lMatchResult = DOS_SUCC;
                break;
            }

            pstInput = (BS_BILLING_MATCH_ST *)pInput;
            pstAccuRule = bs_get_accumulate_rule(ulAttrValue);
            if (DOS_ADDR_INVALID(pstAccuRule) || DOS_ADDR_INVALID(pstInput))
            {
                lMatchResult = DOS_FAIL;
                break;
            }

            if (0 == pstAccuRule->ulMaxValue)
            {
                /* ���޶�,�����ѯͳ�� */
                lMatchResult = DOS_SUCC;
            }
            else
            {
                /* ʹ�����ݿ��е����ý���ͳ�� */
                pstInput->stAccuRule = *pstAccuRule;
                if (BS_BILLING_ATTR_ACCUMULATE_TIMELEN == ucAttrType)
                {
                    ulStat = bs_get_timelen_stat(pstInput);
                }
                else if (BS_BILLING_ATTR_ACCUMULATE_COUNT == ucAttrType)
                {
                    ulStat = bs_get_count_stat(pstInput);
                }
                else if (BS_BILLING_ATTR_ACCUMULATE_TRAFFIC == ucAttrType)
                {
                    ulStat = bs_get_traffic_stat(pstInput);
                }

                if (ulStat < pstAccuRule->ulMaxValue)
                {
                    lMatchResult = DOS_SUCC;
                    *pulValue = ulAttrValue;
                }
            }

            break;

        case BS_BILLING_ATTR_CONCURRENT:
        case BS_BILLING_ATTR_SET:
        case BS_BILLING_ATTR_RESOURCE_AGENT:
        case BS_BILLING_ATTR_RESOURCE_NUMBER:
        case BS_BILLING_ATTR_RESOURCE_LINE:
            /*
            6.��Դ������,ֻ���������ҵ����ʹ��,��ϵͳ���м��㴦��,����ƥ��.
              a.����ֵΪ��Դ����,����Ϊ��Դ����;�����ֶο��Բ���;;
              b.����ֵΪ0��ʾ��ϵͳͳ������;
            */
            lMatchResult = DOS_SUCC;
            break;

        default:
            break;

    }

    return lMatchResult;

}

/* ƥ��Ʒѹ��� */
BS_BILLING_RULE_ST *bs_match_billing_rule(BS_BILLING_PACKAGE_ST *pstPackage,
                                              BS_BILLING_MATCH_ST *pstBillingMatch)
{
    S32                 i, lMatch;
    U32                 aulSrcValue[BS_BILLING_ATTR_LAST] = {U32_BUTT};
    U32                 aulDstValue[BS_BILLING_ATTR_LAST] = {U32_BUTT};
    VOID                *pInput;
    BS_BILLING_RULE_ST  *pstRule    = NULL;
    BS_BILLING_RULE_ST  *pstRuleRet = NULL;

    /* �����ȼ��Ӹߵ�������ƥ��,ƥ��ɹ����˳�ƥ�� */
    for (i = 0; i < BS_MAX_BILLING_RULE_IN_PACKAGE; i++)
    {
        pstRule = &pstPackage->astRule[i];

        if (BS_BILLING_BY_CYCLE == pstRule->ucBillingType)
        {
            /* �˴����������ڼƷѷ�ʽ */
            continue;
        }

        if(!bs_billing_rule_is_properly(pstRule))
        {
            /* �Ʒѹ����׵� */
            continue;
        }

        if ((pstRule->ulExpireTimestamp != 0
             && pstBillingMatch->ulTimeStamp >= pstRule->ulExpireTimestamp)
            || pstBillingMatch->ulTimeStamp < pstRule->ulEffectTimestamp)
        {
            /* �Ʒѹ�����δ��Ч;0��ʾ��Զ��Ч */
            continue;
        }

        /* �����ж�ÿ���Ʒ������Ƿ�ƥ�� */

        /* ��ȡ����ֵ���� */
        pInput = bs_get_attr_input(pstBillingMatch, pstRule->ucSrcAttrType1, DOS_TRUE);
        /* �Ʒ�����ƥ��(����ֵһ�������ȡ����¼����,�����ظ�����) */
        lMatch = bs_match_billing_attr(pstRule->ucSrcAttrType1, pstRule->ulSrcAttrValue1,
                                       pInput, &aulSrcValue[pstRule->ucSrcAttrType1]);
        if (lMatch != DOS_SUCC)
        {
            continue;
        }

        /* ��ȡ����ֵ���� */
        pInput = bs_get_attr_input(pstBillingMatch, pstRule->ucSrcAttrType2, DOS_TRUE);
        /* �Ʒ�����ƥ��(����ֵһ�������ȡ����¼����,�����ظ�����) */
        lMatch = bs_match_billing_attr(pstRule->ucSrcAttrType2, pstRule->ulSrcAttrValue2,
                                       pInput, &aulSrcValue[pstRule->ucSrcAttrType2]);
        if (lMatch != DOS_SUCC)
        {
            continue;
        }

        /* ��ȡ����ֵ���� */
        pInput = bs_get_attr_input(pstBillingMatch, pstRule->ucDstAttrType1, DOS_TRUE);
        /* �Ʒ�����ƥ��(����ֵһ�������ȡ����¼����,�����ظ�����) */
        lMatch = bs_match_billing_attr(pstRule->ucDstAttrType1, pstRule->ulDstAttrValue1,
                                       pInput, &aulDstValue[pstRule->ucDstAttrType1]);
        if (lMatch != DOS_SUCC)
        {
            continue;
        }

        /* ��ȡ����ֵ���� */
        pInput = bs_get_attr_input(pstBillingMatch, pstRule->ucDstAttrType2, DOS_TRUE);
        /* �Ʒ�����ƥ��(����ֵһ�������ȡ����¼����,�����ظ�����) */
        lMatch = bs_match_billing_attr(pstRule->ucDstAttrType2, pstRule->ulDstAttrValue2,
                                       pInput, &aulDstValue[pstRule->ucDstAttrType2]);
        if (lMatch != DOS_SUCC)
        {
            continue;
        }

        /* ����ƥ��ɹ� */
        if (DOS_ADDR_INVALID(pstRuleRet))
        {
            pstRuleRet = pstRule;
        }
        else if (pstRuleRet->ucPriority > pstRule->ucPriority)
        {
            /* ѡ�����ȼ��ߵ� */
            pstRuleRet = pstRule;
        }
    }

    if (DOS_ADDR_INVALID(pstRuleRet))
    {
        bs_trace(BS_TRACE_BILLING, LOG_LEVEL_DEBUG,
                 "Match billing rule fail! package:%u, service:%u, customer:%u, agent:%u"
                 "userid:%u, timestamp:%u, caller:%s, callee:%s",
                 pstPackage->ulPackageID, pstPackage->ucServType,
                 pstBillingMatch->ulCustomerID, pstBillingMatch->ulAgentID,
                 pstBillingMatch->ulUserID, pstBillingMatch->ulTimeStamp,
                 pstBillingMatch->szCaller, pstBillingMatch->szCallee);

    }
    else
    {
        bs_trace(BS_TRACE_BILLING, LOG_LEVEL_DEBUG,
                 "Match billing rule succ! "
                 "package:%u, rule:%u, service:%u, priority:%u, type:%u"
                 "src attr1:%u-%u, attr2:%u-%u, dst attr1:%u-%u, attr1:%u-%u, "
                 "first unit:%u, cnt:%u, next unit:%u, cnt:%u, rate:%u",
                 pstRuleRet->ulPackageID, pstRuleRet->ulRuleID,
                 pstRuleRet->ucServType, pstRuleRet->ucPriority,
                 pstRuleRet->ucBillingType, pstRuleRet->ucSrcAttrType1,
                 pstRuleRet->ulSrcAttrValue1, pstRuleRet->ucSrcAttrType2,
                 pstRuleRet->ulSrcAttrValue2, pstRuleRet->ucDstAttrType1,
                 pstRuleRet->ulDstAttrValue1, pstRuleRet->ucDstAttrType2,
                 pstRuleRet->ulDstAttrValue2, pstRuleRet->ulFirstBillingUnit,
                 pstRuleRet->ucFirstBillingCnt, pstRuleRet->ulNextBillingUnit,
                 pstRuleRet->ucNextBillingCnt, pstRuleRet->ulBillingRate);
    }

    return pstRuleRet;
}

/* Ԥ���۴���,һ��������֤���� */
U32 bs_pre_billing(BS_CUSTOMER_ST *pstCustomer, BS_MSG_AUTH *pstMsg, BS_BILLING_PACKAGE_ST *pstPackage)
{
    U32                 ulMaxSession;
    BS_BILLING_RULE_ST  *pstRule;
    BS_BILLING_MATCH_ST stBillingMatch;

    if (DOS_ADDR_INVALID(pstCustomer) || DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstPackage))
    {
        DOS_ASSERT(0);
        return U32_BUTT;
    }

    ulMaxSession = U32_BUTT;
    dos_memzero(&stBillingMatch, sizeof(stBillingMatch));
    stBillingMatch.ulCustomerID = pstMsg->ulCustomerID;
    stBillingMatch.ulComsumerID = pstMsg->ulCustomerID;
    stBillingMatch.ulAgentID = pstMsg->ulAgentID;
    stBillingMatch.ulUserID = pstMsg->ulUserID;
    stBillingMatch.ulTimeStamp = pstMsg->ulTimeStamp;
    dos_strncpy(stBillingMatch.szCaller, pstMsg->szCaller, sizeof(stBillingMatch.szCaller));
    dos_strncpy(stBillingMatch.szCallee, pstMsg->szCallee, sizeof(stBillingMatch.szCallee));

    pstRule = bs_match_billing_rule(pstPackage, &stBillingMatch);
    if (NULL == pstRule)
    {
        /* ƥ��ʧ�� */
        bs_trace(BS_TRACE_BILLING, LOG_LEVEL_DEBUG,
                 "Prebilling fail! package:%u, service:%u",
                 pstPackage->ulPackageID, pstPackage->ucServType);
        return ulMaxSession;
    }

    /* ��ʼ����,���ڶ��μƷѷ��ʽ���Ԥ������ */
    if (pstRule->ulBillingRate != 0 && pstRule->ucNextBillingCnt != 0)
    {
        ulMaxSession = (U32)((S64)pstRule->ulNextBillingUnit
                     * (pstCustomer->stAccount.LBalanceActive + pstCustomer->stAccount.lCreditLine)
                     / ((S64)pstRule->ulBillingRate * (S64)pstRule->ucNextBillingCnt));
    }

    if (bs_is_message_service(pstRule->ucServType)
        && ulMaxSession > g_stBssCB.ulMaxMsNum)
    {
        ulMaxSession = g_stBssCB.ulMaxMsNum;
    }
    else if (BS_BILLING_BY_TIMELEN == pstRule->ucBillingType
             && ulMaxSession > g_stBssCB.ulMaxVocTime)
    {
        ulMaxSession = g_stBssCB.ulMaxVocTime;
    }

    bs_trace(BS_TRACE_BILLING, LOG_LEVEL_DEBUG,
             "Prebilling succ! package:%u, service:%u, customer:%u, agent:%u"
             "userid:%u, timestamp:%u, caller:%s, callee:%s, max session:%u",
             pstPackage->ulPackageID, pstPackage->ucServType,
             stBillingMatch.ulCustomerID, stBillingMatch.ulAgentID,
             stBillingMatch.ulUserID, stBillingMatch.ulTimeStamp,
             stBillingMatch.szCaller, stBillingMatch.szCallee,
             ulMaxSession);

    return ulMaxSession;
}



#ifdef __cplusplus
}
#endif /* __cplusplus */


