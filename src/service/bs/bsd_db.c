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
#include <json/dos_json.h>
#include "bs_cdr.h"
#include "bs_stat.h"
#include "bs_def.h"
#include "bsd_db.h"


/* ���ݿ��� */
DB_HANDLE_ST    *g_pstDBHandle = NULL;

static S32 bsd_record_cnt_cb(VOID* pParam, S32 lCnt, S8 **aszData, S8 **aszFields)
{
    U32 ulCnt = 0;
    S32 *pulCnt = NULL;

    if (DOS_ADDR_INVALID(pParam)|| DOS_ADDR_INVALID(aszData) || DOS_ADDR_INVALID(*aszData))
    {
        DOS_ASSERT(0);
        return -1;
    }

    if (dos_atoul(aszData[0], &ulCnt) < 0)
    {
        DOS_ASSERT(0);
        return -1;
    }

    pulCnt = (S32 *)pParam;
    *pulCnt = ulCnt;

    return 0;
}

static S32 bsd_walk_customer_tbl_cb(VOID* pParam, S32 lCnt, S8 **aszData, S8 **aszFields)
{
    U32             ulCnt = 0, ulHashIndex, ulCustomerType, ulCustomerState;
    U32             ulBallingPageage, ulBanlance;
    HASH_NODE_S     *pstHashNode = NULL;
    BS_CUSTOMER_ST  *pstCustomer = NULL;

    pstHashNode = dos_dmem_alloc(sizeof(HASH_NODE_S));
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
        return BS_INTER_ERR_MEM_ALLOC_FAIL;
    }
    HASH_Init_Node(pstHashNode);

    pstCustomer = dos_dmem_alloc(sizeof(BS_CUSTOMER_ST));
    if (DOS_ADDR_INVALID(pstCustomer))
    {
        dos_dmem_free(pstHashNode);
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
        return BS_INTER_ERR_MEM_ALLOC_FAIL;
    }
    bs_init_customer_st(pstCustomer);

    //TODO:�������ݿ���Ϣ���ṹ����(�����˻�����)
    if (dos_atoul(aszData[0], &pstCustomer->ulCustomerID) < 0
        || dos_atoul(aszData[2], &pstCustomer->ulParentID) < 0
        || dos_atoul(aszData[3], &ulCustomerType)
        || dos_atoul(aszData[4], &ulCustomerState) < 0
        || dos_atoul(aszData[5], &ulBallingPageage) < 0
        || dos_atoul(aszData[6], &ulBanlance) < 0)
    {
        DOS_ASSERT(0);
        dos_dmem_free(pstHashNode);
        dos_dmem_free(pstCustomer);
        pstHashNode = NULL;
        pstCustomer = NULL;
        return -1;
    }

    pstCustomer->ucCustomerType = (U8)ulCustomerType;
    pstCustomer->ucCustomerState = (U8)ulCustomerState;
    pstCustomer->stAccount.ulAccountID = pstCustomer->ulCustomerID;
    pstCustomer->stAccount.ulBillingPackageID = ulBallingPageage;
    pstCustomer->stAccount.LBalanceActive = ulBanlance;
    dos_strncpy(pstCustomer->szCustomerName, aszData[1], sizeof(pstCustomer->szCustomerName));
    pstCustomer->szCustomerName[sizeof(pstCustomer->szCustomerName) - 1] = '\0';

    pstHashNode->pHandle = (VOID *)pstCustomer;
    ulHashIndex = bs_hash_get_index(BS_HASH_TBL_CUSTOMER_SIZE, pstCustomer->ulCustomerID);
    if (U32_BUTT == ulHashIndex)
    {
        DOS_ASSERT(0);
        return -1;
    }


    pthread_mutex_lock(&g_mutexCustomerTbl);
    /* ��ŵ���ϣ��֮ǰ�Ȳ���,ȷ���Ƿ����ظ� */
    if(NULL == hash_find_node(g_astCustomerTbl,
                              ulHashIndex,
                              (VOID *)&pstCustomer->ulCustomerID,
                              bs_customer_hash_node_match))
    {
        ulCnt++;
        hash_add_node(g_astCustomerTbl, pstHashNode, ulHashIndex, NULL);
        g_astCustomerTbl->NodeNum++;
    }
    else
    {
        bs_trace(BS_TRACE_DB, LOG_LEVEL_ERROR, "ERR: customer(%u:%s) is duplicated in DB !",
                 pstCustomer->ulCustomerID, pstCustomer->szCustomerName);
        dos_dmem_free(pstHashNode);
        dos_dmem_free(pstCustomer);
    }
    pthread_mutex_unlock(&g_mutexCustomerTbl);

    return 0;
}

/* �����ͻ����ݱ� */
S32 bsd_walk_customer_tbl(BS_INTER_MSG_WALK *pstMsg)
{
    U32             ulCnt = 0, ulHashIndex;
    S8              szQuery[512] = { 0 };
    HASH_NODE_S     *pstHashNode = NULL;
    BS_CUSTOMER_ST  *pstCustomer = NULL;

    dos_snprintf(szQuery, sizeof(szQuery), "SELECT `id`,`name`,`parent_id`,`type`,`status`, `billing_package_id`, `balance` from tbl_customer;");
    if (db_query(g_pstDBHandle, szQuery, bsd_walk_customer_tbl_cb, NULL, NULL) != DB_ERR_SUCC)
    {
        bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Read customers from DB FAIL!");
        return BS_INTER_ERR_FAIL;
    }

    HASH_Scan_Table(g_astCustomerTbl, ulHashIndex)
    {
        HASH_Scan_Bucket(g_astCustomerTbl, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            if (DOS_ADDR_INVALID(pstHashNode) || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }

            pstCustomer = pstHashNode->pHandle;
            dos_snprintf(szQuery, sizeof(szQuery), "SELECT count(*) FROM tbl_agent WHERE tbl_agent.customer_id=%d;", pstCustomer->ulCustomerID);
            if (db_query(g_pstDBHandle, szQuery, bsd_record_cnt_cb, (VOID *)&pstCustomer->ulAgentNum, NULL) != DB_ERR_SUCC)
            {
                bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Read Agent list fail for custom %d!", pstCustomer->ulCustomerID);
                return BS_INTER_ERR_FAIL;
            }

            dos_snprintf(szQuery, sizeof(szQuery), "SELECT count(*) FROM tbl_sipassign WHERE tbl_sipassign.customer_id=%d;", pstCustomer->ulCustomerID);
            if (db_query(g_pstDBHandle, szQuery, bsd_record_cnt_cb, (VOID *)&pstCustomer->ulUserLineNum, NULL) != DB_ERR_SUCC)
            {
                bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Read Agent user line fail for custom %d!", pstCustomer->ulCustomerID);
                return BS_INTER_ERR_FAIL;
            }

            dos_snprintf(szQuery, sizeof(szQuery), "SELECT count(*) FROM tbl_caller WHERE tbl_caller.customer_id=%d;", pstCustomer->ulCustomerID);
            if (db_query(g_pstDBHandle, szQuery, bsd_record_cnt_cb, (VOID *)&pstCustomer->ulNumberNum, NULL) != DB_ERR_SUCC)
            {
                bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Read Agent number list fail for custom %d!", pstCustomer->ulCustomerID);
                return BS_INTER_ERR_FAIL;
            }
        }
    }

    bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Read customers from DB SUCC!", ulCnt);
    return BS_INTER_ERR_SUCC;

}


static S32 bsd_walk_agent_tbl_cb(VOID* pParam, S32 lCnt, S8 **aszData, S8 **aszFields)
{
    U32                     ulCnt = 0, ulHashIndex;
    HASH_NODE_S             *pstHashNode = NULL;
    HASH_NODE_S             *pstMatchNode = NULL;
    BS_AGENT_ST             *pstAgent = NULL;

    /* ֵ��ѯ��4�м�¼ */
    if (lCnt != 4)
    {
        DOS_ASSERT(0);

        return -1;
    }

    if (DOS_ADDR_INVALID(aszData) || DOS_ADDR_INVALID(*aszData))
    {
        DOS_ASSERT(0);

        return -1;
    }

    pstHashNode = dos_dmem_alloc(sizeof(HASH_NODE_S));
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
        return BS_INTER_ERR_MEM_ALLOC_FAIL;
    }
    HASH_Init_Node(pstHashNode);

    pstAgent = dos_dmem_alloc(sizeof(BS_AGENT_ST));
    if (DOS_ADDR_INVALID(pstAgent))
    {
        dos_dmem_free(pstHashNode);
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
        return BS_INTER_ERR_MEM_ALLOC_FAIL;
    }
    bs_init_agent_st(pstAgent);

    if (dos_atoul(aszData[0], &pstAgent->ulAgentID) < 0
        || dos_atoul(aszData[1], &pstAgent->ulCustomerID) < 0
        || dos_atoul(aszData[2], &pstAgent->ulGroup1) < 0
        || dos_atoul(aszData[3], &pstAgent->ulGroup2) < 0)
    {
        DOS_ASSERT(0);

        dos_dmem_free(pstHashNode);
        dos_dmem_free(pstAgent);
        pstHashNode = NULL;
        pstAgent = NULL;

        return -1;
    }

    pstHashNode->pHandle = (VOID *)pstAgent;
    ulHashIndex = bs_hash_get_index(BS_HASH_TBL_AGENT_SIZE, pstAgent->ulAgentID);
    if (U32_BUTT == ulHashIndex)
    {
        DOS_ASSERT(0);
        return -1;
    }

    pthread_mutex_lock(&g_mutexAgentTbl);
    /* ��ŵ���ϣ��֮ǰ�Ȳ���,ȷ���Ƿ����ظ� */
    pstMatchNode = hash_find_node(g_astAgentTbl,
                                  ulHashIndex,
                                  (VOID *)&pstAgent->ulAgentID,
                                  bs_agent_hash_node_match);
    if(DOS_ADDR_INVALID(pstMatchNode))
    {
        ulCnt++;
        hash_add_node(g_astAgentTbl, pstHashNode, ulHashIndex, NULL);
        g_astAgentTbl->NodeNum++;
    }
    else
    {
        bs_trace(BS_TRACE_DB, LOG_LEVEL_ERROR,
                 "ERR: agent(%u) is duplicated in DB !",
                 pstAgent->ulAgentID);
        dos_dmem_free(pstHashNode);
        dos_dmem_free(pstAgent);
    }
    pthread_mutex_unlock(&g_mutexAgentTbl);

    return 0;
}


/* ������ϯ�� */
S32 bsd_walk_agent_tbl(BS_INTER_MSG_WALK *pstMsg)
{
    S8 szQuery[1024] = { 0, };

    dos_snprintf(szQuery, sizeof(szQuery), "SELECT id, customer_id, group1_id, group2_id FROM tbl_agent;");

    if (db_query(g_pstDBHandle, szQuery, bsd_walk_agent_tbl_cb, NULL, NULL) != DB_ERR_SUCC)
    {
        bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Read agents info from DB fail!");

        return BS_INTER_ERR_FAIL;
    }
    bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Read agents info from DB succ!");

    return BS_INTER_ERR_SUCC;
}


static S32 bsd_walk_billing_package_tbl_cb(VOID* pParam, S32 lCnt, S8 **aszData, S8 **aszFields)
{
    U32                     ulCnt = 0, ulHashIndex;
    HASH_NODE_S             *pstHashNode = NULL;
    HASH_NODE_S             *pstMatchNode = NULL;
    BS_BILLING_PACKAGE_ST   *pstBillingPackage = NULL;
    BS_BILLING_PACKAGE_ST   *pstMatchPackage = NULL;
    U32                     ulSrcAttrType1, ulSrcAttrType2, ulDstAttrType1, ulDstAttrType2;
    U32                     ulServType, ulFirstBillingCnt, ulNextBillingCnt, ulServType1;
    U32                     ulBillingType;

    pstHashNode = dos_dmem_alloc(sizeof(HASH_NODE_S));
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
        return BS_INTER_ERR_MEM_ALLOC_FAIL;
    }
    HASH_Init_Node(pstHashNode);

    pstBillingPackage = dos_dmem_alloc(sizeof(BS_BILLING_PACKAGE_ST));
    if (DOS_ADDR_INVALID(pstBillingPackage))
    {
        dos_dmem_free(pstHashNode);
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
        return BS_INTER_ERR_MEM_ALLOC_FAIL;
    }
    bs_init_billing_package_st(pstBillingPackage);

    /* �ʷѹ����ȴ浽����0���±��� */
    if (dos_atoul(aszData[0], &pstBillingPackage->ulPackageID) < 0
        || dos_atoul(aszData[1], &ulServType) < 0
        || dos_atoul(aszData[2], &pstBillingPackage->astRule[0].ulPackageID) < 0
        || dos_atoul(aszData[3], &pstBillingPackage->astRule[0].ulRuleID) < 0
        || dos_atoul(aszData[4], &ulSrcAttrType1) < 0
        || dos_atoul(aszData[5], &ulSrcAttrType2) < 0
        || dos_atoul(aszData[6], &ulDstAttrType1) < 0
        || dos_atoul(aszData[7], &ulDstAttrType2) < 0
        || dos_atoul(aszData[8], &pstBillingPackage->astRule[0].ulSrcAttrValue1) < 0
        || dos_atoul(aszData[9], &pstBillingPackage->astRule[0].ulSrcAttrValue2) < 0
        || dos_atoul(aszData[10], &pstBillingPackage->astRule[0].ulDstAttrValue1) < 0
        || dos_atoul(aszData[11], &pstBillingPackage->astRule[0].ulDstAttrValue2) < 0
        || dos_atoul(aszData[12], &pstBillingPackage->astRule[0].ulFirstBillingUnit) < 0
        || dos_atoul(aszData[13], &pstBillingPackage->astRule[0].ulNextBillingUnit) < 0
        || dos_atoul(aszData[14], &ulFirstBillingCnt) < 0
        || dos_atoul(aszData[15], &ulNextBillingCnt) < 0
        || dos_atoul(aszData[16], &ulServType1) < 0
        || dos_atoul(aszData[17], &ulBillingType) < 0
        || dos_atoul(aszData[18], &pstBillingPackage->astRule[0].ulBillingRate) < 0
        || dos_atoul(aszData[19], &pstBillingPackage->astRule[0].ulEffectTimestamp) < 0
        || dos_atoul(aszData[20], &pstBillingPackage->astRule[0].ulExpireTimestamp) < 0
        || dos_atoul(aszData[21], &pstBillingPackage->astRule[0].ulPackageID) < 0)
    {
        /* ���ô���hash��ڵ�,�ͷ��ڴ� */
        dos_dmem_free(pstHashNode);
        dos_dmem_free(pstBillingPackage);
        pstHashNode = NULL;
        pstBillingPackage = NULL;

        return -1;
    }

    pstBillingPackage->ucServType = (U8)ulServType;
    pstBillingPackage->astRule[0].ucSrcAttrType1 = (U8)ulSrcAttrType1;
    pstBillingPackage->astRule[0].ucSrcAttrType2 = (U8)ulSrcAttrType2;
    pstBillingPackage->astRule[0].ucDstAttrType1 = (U8)ulDstAttrType1;
    pstBillingPackage->astRule[0].ucDstAttrType2 = (U8)ulDstAttrType2;
    pstBillingPackage->astRule[0].ucFirstBillingCnt = (U8)ulFirstBillingCnt;
    pstBillingPackage->astRule[0].ucNextBillingCnt = (U8)ulNextBillingCnt;
    pstBillingPackage->astRule[0].ucServType = (U8)ulServType1;
    pstBillingPackage->astRule[0].ucBillingType = (U8)ulBillingType;

    pstHashNode->pHandle = (VOID *)pstBillingPackage;
    ulHashIndex = bs_hash_get_index(BS_HASH_TBL_BILLING_PACKAGE_SIZE, pstBillingPackage->ulPackageID);
    if (U32_BUTT == ulHashIndex)
    {
        DOS_ASSERT(0);
        return -1;
    }

    pthread_mutex_lock(&g_mutexBillingPackageTbl);
    /* ��ŵ���ϣ��֮ǰ�Ȳ���,ȷ���Ƿ����ظ� */
    pstMatchNode = hash_find_node(g_astBillingPackageTbl,
                                  ulHashIndex,
                                  (VOID *)&pstBillingPackage,
                                  bs_billing_package_hash_node_match);
    if(DOS_ADDR_INVALID(pstMatchNode))
    {
        ulCnt++;
        hash_add_node(g_astBillingPackageTbl, pstHashNode, ulHashIndex, NULL);
        g_astBillingPackageTbl->NodeNum++;
    }
    else
    {
        /* ÿ��ҵ���ж����Ʒѹ���,�����ͻ,ֱ�Ӹ��� */

        U8  ucPriority;

        ucPriority = pstBillingPackage->astRule[0].ucPriority;
        if (ucPriority >= BS_MAX_BILLING_RULE_IN_PACKAGE)
        {
            bs_trace(BS_TRACE_DB, LOG_LEVEL_ERROR,
                     "ERR: priority(%u) of billing rule(package:%u, service:%u) is wrong in DB !",
                     ucPriority, pstBillingPackage->ulPackageID, pstBillingPackage->ucServType);
        }
        else
        {
            pstMatchPackage = (BS_BILLING_PACKAGE_ST *)pstMatchNode->pHandle;
            pstMatchPackage->astRule[ucPriority] = pstBillingPackage->astRule[0];
        }

        /* ���ô���hash��ڵ�,�ͷ��ڴ� */
        dos_dmem_free(pstHashNode);
        dos_dmem_free(pstBillingPackage);
    }
    pthread_mutex_unlock(&g_mutexBillingPackageTbl);

    return 0;
}


/* �����ʷ����ݱ� */
S32 bsd_walk_billing_package_tbl(BS_INTER_MSG_WALK *pstMsg)
{
    S8 szQuery[1024] = {0, };

    dos_snprintf(szQuery, sizeof(szQuery), "SELECT * FROM tbl_billing_rule t1 LEFT JOIN tbl_billing_package t2 ON t1.billing_package_id = t2.id;");

    if (db_query(g_pstDBHandle, szQuery, bsd_walk_billing_package_tbl_cb, NULL, NULL) != DB_ERR_SUCC)
    {
        bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Read billing package from DB FAIL!");
        return BS_INTER_ERR_FAIL;
    }

    bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Read billing package from DB OK!");
    return BS_INTER_ERR_SUCC;

}
#if 0
static S32 bsd_walk_settle_tbl_cb(BS_INTER_MSG_WALK *pstMsg)
{
    U32                     ulCnt = 0, ulHashIndex;
    HASH_NODE_S             *pstHashNode = NULL;
    BS_SETTLE_ST            *pstSettle = NULL;

    pstHashNode = dos_dmem_alloc(sizeof(HASH_NODE_S));
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
        return BS_INTER_ERR_MEM_ALLOC_FAIL;
    }
    HASH_Init_Node(pstHashNode);

    pstSettle = dos_dmem_alloc(sizeof(BS_SETTLE_ST));
    if (DOS_ADDR_INVALID(pstSettle))
    {
        dos_dmem_free(pstHashNode);
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
        return BS_INTER_ERR_MEM_ALLOC_FAIL;
    }
    bs_init_settle_st(pstSettle);

    //TODO : Copy���ݵ����ƿ�

    pstHashNode->pHandle = (VOID *)pstSettle;
    ulHashIndex = bs_hash_get_index(BS_HASH_TBL_SETTLE_SIZE, pstSettle->usTrunkID);

    pthread_mutex_lock(&g_mutexSettleTbl);
    /* ��ŵ���ϣ��֮ǰ�Ȳ���,ȷ���Ƿ����ظ� */
    if(NULL == hash_find_node(g_astSettleTbl,
                              ulHashIndex,
                              (VOID *)pstSettle,
                              bs_settle_hash_node_match))
    {
        ulCnt++;
        hash_add_node(g_astSettleTbl, pstHashNode, ulHashIndex, NULL);
        g_astSettleTbl->NodeNum++;
    }
    else
    {
        bs_trace(BS_TRACE_DB, LOG_LEVEL_ERROR, "ERR: settle info(trunk:%u, sp:%u) is duplicated in DB !",
                 pstSettle->usTrunkID, pstSettle->ulSPID);
        dos_dmem_free(pstHashNode);
        dos_dmem_free(pstSettle);
    }
    pthread_mutex_unlock(&g_mutexSettleTbl);

    return 0;
}
#endif
/* �����������ݱ� */
S32 bsd_walk_settle_tbl(BS_INTER_MSG_WALK *pstMsg)
{
    //TODO : �����ѯ���ݿ�����ѯ�ʷѱ�
    //TODO:���ζ�ȡÿһ������,�����Ϣ��ֵ���ṹ����

    bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Read settle info from DB !");
    return BS_INTER_ERR_SUCC;
}

static S32 bsd_walk_web_cmd_cb(VOID* pParam, S32 lCnt, S8 **aszData, S8 **aszFields)
{
    BS_WEB_CMD_INFO_ST      *pszTblRow   = NULL;
    DLL_NODE_S              *pstListNode = NULL;
    JSON_OBJ_ST             *pstJSONObj  = NULL;
    U32                     ulTimestamp = 0;

    /* ��ѯ�����ֶ� */
    if (lCnt != 3)
    {
        DOS_ASSERT(0);

        return -1;
    }

    if (DOS_ADDR_INVALID(aszData) || DOS_ADDR_INVALID(*aszData))
    {
        DOS_ASSERT(0);

        return -1;
    }

    /* ��һ����ʱ�������2����json���� */
    if ( DOS_ADDR_INVALID(aszData[1])
        || DOS_ADDR_INVALID(aszData[2]))
    {
        DOS_ASSERT(0);

        return -1;
    }

    pstListNode = dos_dmem_alloc(sizeof(HASH_NODE_S));
    if (DOS_ADDR_INVALID(pstListNode))
    {
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
        goto error_proc;
    }
    DLL_Init_Node(pstListNode);

    pszTblRow = (BS_WEB_CMD_INFO_ST *)dos_dmem_alloc(sizeof(BS_WEB_CMD_INFO_ST));
    if (DOS_ADDR_INVALID(pszTblRow))
    {
        DOS_ASSERT(0);
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
        goto error_proc;
    }

    pstJSONObj = json_init(aszData[2]);
    if (DOS_ADDR_INVALID(pstJSONObj))
    {
        DOS_ASSERT(0);
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Warning: Parse the json fail!");
        goto error_proc;
    }

    if (dos_atoul(aszData[1], &ulTimestamp) < 0)
    {
        DOS_ASSERT(0);
        goto error_proc;
    }

    pszTblRow->pstData = pstJSONObj;
    pstListNode->pHandle = pszTblRow;

    pthread_mutex_lock(&g_mutexWebCMDTbl);
    DLL_Add(&g_stWebCMDTbl, pstListNode);
    pthread_mutex_unlock(&g_mutexWebCMDTbl);

    return 0;

error_proc:
    if (pstListNode)
    {
        dos_dmem_free(pstListNode);
    }

    if (pszTblRow)
    {
        dos_dmem_free(pszTblRow);
    }

    if (pstJSONObj)
    {
        json_deinit(&pstJSONObj);
    }

    return -1;
}

/* ����WEB�������ʱ�� */
S32 bsd_walk_web_cmd_tbl(BS_INTER_MSG_WALK *pstMsg)
{
    S8 szQuery[256] = {0, };

    if (!g_pstDBHandle || g_pstDBHandle->ulDBStatus != DB_STATE_CONNECTED)
    {
        DOS_ASSERT(0);
        bs_trace(BS_TRACE_DB, LOG_LEVEL_EMERG, "DB Has been down or not init.");
        return -1;
    }

    dos_snprintf(szQuery, sizeof(szQuery), "SELECT id,ctime,json_fields FROM tmp_tbl_modify;");

    if (db_query(g_pstDBHandle, szQuery, bsd_walk_web_cmd_cb, NULL, NULL) != DB_ERR_SUCC)
    {
        bs_trace(BS_TRACE_DB, LOG_LEVEL_NOTIC, "DB query failed.(%s)", szQuery);
        return -1;
    }

    dos_snprintf(szQuery, sizeof(szQuery), "delete from tmp_tbl_modify;");
    db_transaction_begin(g_pstDBHandle);
    if (db_query(g_pstDBHandle, szQuery, NULL, NULL, NULL) != DB_ERR_SUCC)
    {
        bs_trace(BS_TRACE_DB, LOG_LEVEL_NOTIC, "DB query failed. (%s)", szQuery);
        db_transaction_rollback(g_pstDBHandle);
        return -1;
    }
    db_transaction_commit(g_pstDBHandle);

    return 0;
}


/* �洢ԭʼ���� */
VOID bsd_save_original_cdr(BS_INTER_MSG_CDR *pstMsg)
{
    U32             i;
    BS_MSG_CDR      *pstCDR;
    S8              szQuery[1024] = { 0, };
    S8              szTime[128];

    pstCDR = (BS_MSG_CDR *)pstMsg->pCDR;
    if (DOS_ADDR_INVALID(pstCDR))
    {
        DOS_ASSERT(0);
        return;
    }

    if (pstCDR->ucLegNum > BS_MAX_SESSION_LEG_IN_BILL)
    {
        DOS_ASSERT(0);
        bs_trace(BS_TRACE_CDR, LOG_LEVEL_ERROR, "ERR: leg num(%u) in CDR is too many!", pstCDR->ucLegNum);
        return;
    }

    /* ���ζ�ȡÿ��LEG����Ϣ,ת��Ϊ�����洢�����ݿ��� */
    for(i = 0; i < pstCDR->ucLegNum; i++)
    {

        //TODO:�洢���������ݿ���
        dos_snprintf(szQuery, sizeof(szQuery), "INSERT IGNORE INTO "
                    	"tbl_cdr (id, customer_id, account_id, user_id, task_id, type1, type2, type3"
                    	", record_file, caller, callee, CID, agent_num, start_time, ring_time"
                    	", answer_time, ivr_end_time, dtmf_time, hold_cnt, hold_times, peer_trunk_id"
                    	", terminate_cause, release_part, payload_type, package_loss_rate, cdr_mark"
                    	", sessionID, bridge_time, bye_time, peer_ip1, peer_ip2, tbl_cdr.peer_ip3, peer_ip4)"
                    "VALUES(NULL, %u, %u, %u, %u, %u, %u, %u, \"%s\", \"%s\", \"%s\", \"%s\""
                        ", \"%s\", FROM_UNIXTIME(%u), FROM_UNIXTIME(%u), FROM_UNIXTIME(%u), FROM_UNIXTIME(%u), FROM_UNIXTIME(%u), %u, %u, %u, %u, %u, %u, %u, %u, \"%s\""
                        ", FROM_UNIXTIME(%u), FROM_UNIXTIME(%u), %u, %u, %u, %u);"
                    , pstCDR->astSessionLeg[i].ulCustomerID
                    , pstCDR->astSessionLeg[i].ulAccountID
                    , pstCDR->astSessionLeg[i].ulUserID
                    , pstCDR->astSessionLeg[i].ulTaskID
                    , pstCDR->astSessionLeg[i].aucServType[0]
                    , pstCDR->astSessionLeg[i].aucServType[1]
                    , pstCDR->astSessionLeg[i].aucServType[2]
                    , pstCDR->astSessionLeg[i].szRecordFile
                    , pstCDR->astSessionLeg[i].szCaller
                    , pstCDR->astSessionLeg[i].szCallee
                    , pstCDR->astSessionLeg[i].szCID
                    , pstCDR->astSessionLeg[i].szAgentNum
                    , pstCDR->astSessionLeg[i].ulStartTimeStamp
                    , pstCDR->astSessionLeg[i].ulRingTimeStamp
                    , pstCDR->astSessionLeg[i].ulAnswerTimeStamp
                    , pstCDR->astSessionLeg[i].ulIVRFinishTimeStamp
                    , pstCDR->astSessionLeg[i].ulDTMFTimeStamp
                    , pstCDR->astSessionLeg[i].ulHoldCnt
                    , pstCDR->astSessionLeg[i].ulHoldTimeLen
                    , pstCDR->astSessionLeg[i].usPeerTrunkID
                    , pstCDR->astSessionLeg[i].usTerminateCause
                    , pstCDR->astSessionLeg[i].ucReleasePart
                    , pstCDR->astSessionLeg[i].ucPayloadType
                    , pstCDR->astSessionLeg[i].ucPacketLossRate
                    , pstCDR->astSessionLeg[i].ulCDRMark
                    , pstCDR->astSessionLeg[i].szSessionID
                    , pstCDR->astSessionLeg[i].ulBridgeTimeStamp
                    , pstCDR->astSessionLeg[i].ulByeTimeStamp
                    , pstCDR->astSessionLeg[i].aulPeerIP[0]
                    , pstCDR->astSessionLeg[i].aulPeerIP[1]
                    , pstCDR->astSessionLeg[i].aulPeerIP[2]
                    , pstCDR->astSessionLeg[i].aulPeerIP[3]);

        if (db_query(g_pstDBHandle, szQuery, NULL, NULL, NULL) < 0)
        {
            bs_trace(BS_TRACE_DB, LOG_LEVEL_ERROR, "Save CDR in DB! (%s)", szQuery);
            continue;
        }

        bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Save CDR in DB !");
    }

}

/* �洢�������� */
VOID bsd_save_voice_cdr(BS_INTER_MSG_CDR *pstMsg)
{
    BS_CDR_VOICE_ST *pstCDR;
    S8              szQuery[1024] = { 0, };

    pstCDR = (BS_CDR_VOICE_ST *)pstMsg->pCDR;
    if (DOS_ADDR_INVALID(pstCDR))
    {
        DOS_ASSERT(0);
        return;
    }

    dos_snprintf(szQuery, sizeof(szQuery), "INSERT INTO tbl_cdr_voice("
                	"`id`,`customer_id`,`account_id`,`user_id`,`task_id`,`billing_rule_id`,`type`,`fee_l1`,"
                	"`fee_l2`,`fee_l3`,`fee_l4`,`fee_l5`,`record_file`,`caller`,`callee`,`CID`,`agent_num`,"
                	"`pdd_len`,`ring_times`,`answer_times`,`ivr_end_times`,`dtmf_times`,`wait_agent_times`,"
                	"`time_len`,`hold_cnt`,`hold_times`,`peer_trunk_id`,`terminate_cause`,`release_part`,"
                	"`payload_type`,`package_loss_rate`,`record_flag`,`agent_level`,`cdr_mark`,`cdr_type`,"
                	"`peer_ip1`,`peer_ip2`,`peer_ip3`,`peer_ip4`)"
                "VALUES(NULL, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, \"%s\", \"%s\", \"%s\""
                	", \"%s\", \"%s\", %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u"
                	", %u, %u, %u, %u, %u, %u, %u, %u);"
                	, pstCDR->ulCustomerID, pstCDR->ulAccountID, pstCDR->ulUserID
                	, pstCDR->ulTaskID, pstCDR->ulRuleID, pstCDR->ucServType
                	, pstCDR->aulFee[0], pstCDR->aulFee[1], pstCDR->aulFee[2]
                	, pstCDR->aulFee[3], pstCDR->aulFee[4], pstCDR->szRecordFile
                	, pstCDR->szCaller, pstCDR->szCallee, pstCDR->szCID, pstCDR->szAgentNum
                	, pstCDR->ulPDDLen, pstCDR->ulRingTime, pstCDR->ulAnswerTimeStamp
                	, pstCDR->ulIVRFinishTime, pstCDR->ulDTMFTime, pstCDR->ulWaitAgentTime
                	, pstCDR->ulTimeLen, pstCDR->ulHoldCnt, pstCDR->ulHoldTimeLen
                	, pstCDR->usPeerTrunkID, pstCDR->usTerminateCause, pstCDR->ucReleasePart
                	, pstCDR->ucPayloadType, pstCDR->ucPacketLossRate, pstCDR->ucRecordFlag
                	, pstCDR->ucAgentLevel, pstCDR->stCDRTag.ulCDRMark, pstCDR->stCDRTag.ucCDRType
                	, pstCDR->aulPeerIP[0], pstCDR->aulPeerIP[1], pstCDR->aulPeerIP[2], pstCDR->aulPeerIP[3]);

    if (db_query(g_pstDBHandle, szQuery, NULL, NULL, NULL) < 0)
    {
        bs_trace(BS_TRACE_DB, LOG_LEVEL_ERROR, "Save CDR in DB! (%s)", szQuery);
    }
    else
    {
        bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Save CDR in DB !");
    }
}

/* �洢�������� */
VOID bsd_save_recording_cdr(BS_INTER_MSG_CDR *pstMsg)
{
    BS_CDR_RECORDING_ST *pstCDR;
    S8                  szQuery[1024] = { 0, };

    pstCDR = (BS_CDR_RECORDING_ST *)pstMsg->pCDR;
    if (DOS_ADDR_INVALID(pstCDR))
    {
        DOS_ASSERT(0);
        return;
    }

    dos_snprintf(szQuery, sizeof(szQuery), "INSERT INTO `tbl_cdr_record` ("
                      "`id`,`customer_id`,`account_id`,`user_id`,`task_id`,`billing_rule_id`,"
                      "`fee_l1`,`fee_l2`,`fee_l3`,`fee_l4`,`fee_l5`,"
                      "`record_file`,`caller`,`callee`,`CID`,`agent_num`,"
                      "`start_time`,`time_len`,`agent_level`,`cdr_mark`,`cdr_type`)"
                    "VALUES(NULL, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, \"%s\""
                    		, "\%s\", \"%s\", \"%s\", \"%s\", %u, %u, %u, %u, %u);"
                	, pstCDR->ulCustomerID, pstCDR->ulAccountID, pstCDR->ulUserID
                	, pstCDR->ulTaskID, pstCDR->ulRuleID, pstCDR->aulFee[0]
                	, pstCDR->aulFee[1], pstCDR->aulFee[2], pstCDR->aulFee[3]
                	, pstCDR->aulFee[4], pstCDR->szRecordFile, pstCDR->szCaller
                	, pstCDR->szCallee, pstCDR->szCID, pstCDR->szAgentNum
                	, pstCDR->ulRecordTimeStamp, pstCDR->ulTimeLen, pstCDR->ucAgentLevel
                	, pstCDR->stCDRTag.ulCDRMark, pstCDR->stCDRTag.ucCDRType);

    if (db_query(g_pstDBHandle, szQuery, NULL, NULL, NULL) < 0)
    {
        bs_trace(BS_TRACE_DB, LOG_LEVEL_ERROR, "Save CDR in DB! (%s)", szQuery);
    }
    else
    {
        bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Save CDR in DB !");
    }
}

/* �洢��Ϣ���� */
VOID bsd_save_message_cdr(BS_INTER_MSG_CDR *pstMsg)
{
    BS_CDR_MS_ST    *pstCDR;
    S8              szQuery[1024] = { 0, };

    pstCDR = (BS_CDR_MS_ST *)pstMsg->pCDR;
    if (DOS_ADDR_INVALID(pstCDR))
    {
        DOS_ASSERT(0);
        return;
    }

    dos_snprintf(szQuery, sizeof(szQuery), "INSERT INTO `tbl_cdr_ms` ("
                      "`id`,`customer_id`,`account_id`,`user_id`,`sms_id`,`billing_rule_id`,"
                      "`type`,`fee_l1`,`fee_l2`,`fee_l3`,`fee_l4`,`fee_l5`,`caller`,`callee`,"
                      "`agent_num`,`deal_time`,`arrived_time`,`msg_len`,`peer_trunk_id`,"
                      "`terminate_cause`,`agent_level`,`cdr_mark`,`cdr_type`,`peer_ip1`,"
                      "`peer_ip2`,`peer_ip3`,`peer_ip4`)"
                    "VALUES (%u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u"
                    	", \"%s\", \"%s\", \"%s\", %u, %u, %u, %u, %u, %u"
                    	", %u, %u, %u, %u, %u, %u);"
                	, pstCDR->ulCustomerID, pstCDR->ulAccountID, pstCDR->ulUserID, 0
                	, pstCDR->ulRuleID, pstCDR->ucServType, pstCDR->aulFee[0]
                	, pstCDR->aulFee[1], pstCDR->aulFee[2], pstCDR->aulFee[3]
                	, pstCDR->aulFee[4], pstCDR->szCaller, pstCDR->szCallee
                	, pstCDR->szAgentNum, pstCDR->ulTimeStamp, pstCDR->ulArrivedTimeStamp
                	, pstCDR->ulLen, pstCDR->usPeerTrunkID, pstCDR->usTerminateCause, pstCDR->ucAgentLevel
                	, pstCDR->stCDRTag.ulCDRMark, pstCDR->stCDRTag.ucCDRType, pstCDR->aulPeerIP[0]
                	, pstCDR->aulPeerIP[1], pstCDR->aulPeerIP[2], pstCDR->aulPeerIP[3]);

    if (db_query(g_pstDBHandle, szQuery, NULL, NULL, NULL) < 0)
    {
        bs_trace(BS_TRACE_DB, LOG_LEVEL_ERROR, "Save CDR in DB! (%s)", szQuery);
    }
    else
    {
        bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Save CDR in DB !");
    }


}

/* �洢���㻰�� */
VOID bsd_save_settle_cdr(BS_INTER_MSG_CDR *pstMsg)
{
    BS_CDR_SETTLE_ST    *pstCDR;
    S8                  szQuery[1024] = { 0, };

    pstCDR = (BS_CDR_SETTLE_ST *)pstMsg->pCDR;
    if (DOS_ADDR_INVALID(pstCDR))
    {
        DOS_ASSERT(0);
        return;
    }
    dos_snprintf(szQuery, sizeof(szQuery), "INSERT INTO `tbl_cdr_settle` ("
                      "`id`, `sp_customer_ id`,`billing_rule_id`,`ctime`,"
                      "`type`,`fee`,`caller`,`callee`,`deal_times`,`peer_trunk_id`,"
                      "`terminate_cause`,`cdr_mark`,`cdr_type`,`peer_ip1`,"
                      "`peer_ip2`,`peer_ip3`,`peer_ip4`)"
                    "VALUES(NULL, %u, %u, %u, %u, %u, \"%s\", \"%s\", %u"
                    	", %u, %u, %u, %u, %u, %u, %u, %u);"
                	, pstCDR->ulSPID, pstCDR->ulRuleID, pstCDR->ulTimeStamp
                	, pstCDR->ucServType, pstCDR->ulFee, pstCDR->szCaller
                	, pstCDR->szCallee, pstCDR->ulTimeStamp, pstCDR->usPeerTrunkID
                	, pstCDR->usTerminateCause, pstCDR->stCDRTag.ulCDRMark, pstCDR->stCDRTag.ucCDRType
                	, pstCDR->aulPeerIP[0], pstCDR->aulPeerIP[1], pstCDR->aulPeerIP[2], pstCDR->aulPeerIP[3]);

    if (db_query(g_pstDBHandle, szQuery, NULL, NULL, NULL) < 0)
    {
        bs_trace(BS_TRACE_DB, LOG_LEVEL_ERROR, "Save CDR in DB! (%s)", szQuery);
    }
    else
    {
        bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Save CDR in DB !");
    }

}

/* �洢��𻰵� */
VOID bsd_save_rent_cdr(BS_INTER_MSG_CDR *pstMsg)
{
    BS_CDR_RENT_ST    *pstCDR;
    S8                szQuery[1024] = { 0, };

    pstCDR = (BS_CDR_RENT_ST *)pstMsg->pCDR;
    if (DOS_ADDR_INVALID(pstCDR))
    {
        DOS_ASSERT(0);
        return;
    }

    dos_snprintf(szQuery, sizeof(szQuery), "INSERT INTO `tbl_cdr_rent` ("
                      "`id`,`customer_id`,`account_id`,`billing_rule_id`,`ctime`,"
                      "`type`,`fee_l1`,`fee_l2`,`fee_l3`,`fee_l4`,`fee_l5`,"
                      "`agent_level`,`cdr_mark`,`cdr_type`)"
                    "VALUES(NULL, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u);"
                	, pstCDR->ulCustomerID, pstCDR->ulAccountID, pstCDR->ulRuleID, pstCDR->ulTimeStamp
                	, pstCDR->ucAttrType, pstCDR->aulFee[0], pstCDR->aulFee[1], pstCDR->aulFee[2]
                	, pstCDR->aulFee[3], pstCDR->aulFee[4], pstCDR->ucAgentLevel, pstCDR->stCDRTag.ulCDRMark
                	, pstCDR->stCDRTag.ucCDRType);

    if (db_query(g_pstDBHandle, szQuery, NULL, NULL, NULL) < 0)
    {
        bs_trace(BS_TRACE_DB, LOG_LEVEL_ERROR, "Save CDR in DB! (%s)", szQuery);
    }
    else
    {
        bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Save CDR in DB !");
    }
}

/* �洢�˻��໰�� */
VOID bsd_save_account_cdr(BS_INTER_MSG_CDR *pstMsg)
{
    BS_CDR_ACCOUNT_ST   *pstCDR;
    S8                  szQuery[1024] = { 0, };

    pstCDR = (BS_CDR_ACCOUNT_ST *)pstMsg->pCDR;
    if (DOS_ADDR_INVALID(pstCDR))
    {
        DOS_ASSERT(0);
        return;
    }

    dos_snprintf(szQuery, sizeof(szQuery), "INSERT IGNORE INTO `tbl_cdr_account` ("
                      "`id`,`customer_id`,`account_id`,`ctime`,"
                      "`type`,`money`,`balance`,`peer_account_id`,"
                      "`operator_id`,`note`)"
                    "VALUES(NULL, %u, %u, %u, %u, %u, %u, %u, %u, \"%s\");"
                	, pstCDR->ulCustomerID, pstCDR->ulAccountID, pstCDR->ulTimeStamp
                	, pstCDR->ucOperateType, pstCDR->lMoney, pstCDR->LBalance
                	, pstCDR->ulPeeAccount, pstCDR->ulOperatorID, pstCDR->szRemark);

    if (db_query(g_pstDBHandle, szQuery, NULL, NULL, NULL) < 0)
    {
        bs_trace(BS_TRACE_DB, LOG_LEVEL_ERROR, "Save CDR in DB! (%s)", szQuery);
    }
    else
    {
        bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Save CDR in DB !");
    }


}

/* �洢���ֺ���ͳ�� */
VOID bsd_save_outband_stat(BS_INTER_MSG_STAT *pstMsg)
{
    BS_STAT_OUTBAND_ST  *pstStat;

    pstStat = (BS_STAT_OUTBAND_ST *)pstMsg->pStat;
    if (DOS_ADDR_INVALID(pstStat))
    {
        DOS_ASSERT(0);
        return;
    }

    //TODO:�洢ͳ����Ϣ�����ݿ���

    bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Save outband stat in DB !");

}

/* �洢���ֺ���ͳ�� */
VOID bsd_save_inband_stat(BS_INTER_MSG_STAT *pstMsg)
{
    BS_STAT_INBAND_ST   *pstStat;

    pstStat = (BS_STAT_INBAND_ST *)pstMsg->pStat;
    if (DOS_ADDR_INVALID(pstStat))
    {
        DOS_ASSERT(0);
        return;
    }

    //TODO:�洢ͳ����Ϣ�����ݿ���

    bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Save inband stat in DB !");

}

/* �洢���ֺ���ͳ�� */
VOID bsd_save_outdialing_stat(BS_INTER_MSG_STAT *pstMsg)
{
    BS_STAT_OUTDIALING_ST   *pstStat;

    pstStat = (BS_STAT_OUTDIALING_ST *)pstMsg->pStat;
    if (DOS_ADDR_INVALID(pstStat))
    {
        DOS_ASSERT(0);
        return;
    }

    //TODO:�洢ͳ����Ϣ�����ݿ���

    bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Save out dialing stat in DB !");

}

/* �洢���ֺ���ͳ�� */
VOID bsd_save_message_stat(BS_INTER_MSG_STAT *pstMsg)
{
    BS_STAT_MESSAGE_ST  *pstStat;

    pstStat = (BS_STAT_MESSAGE_ST *)pstMsg->pStat;
    if (DOS_ADDR_INVALID(pstStat))
    {
        DOS_ASSERT(0);
        return;
    }

    //TODO:�洢ͳ����Ϣ�����ݿ���

    bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Save message stat in DB !");

}

/* �洢���ֺ���ͳ�� */
VOID bsd_save_account_stat(BS_INTER_MSG_STAT *pstMsg)
{
    BS_STAT_ACCOUNT_ST  *pstStat;

    pstStat = (BS_STAT_ACCOUNT_ST *)pstMsg->pStat;
    if (DOS_ADDR_INVALID(pstStat))
    {
        DOS_ASSERT(0);
        return;
    }

    //TODO:�洢ͳ����Ϣ�����ݿ���

    bs_trace(BS_TRACE_DB, LOG_LEVEL_DEBUG, "Save account stat in DB !");

}


S32 bs_init_db()
{
    g_pstDBHandle = db_create(DB_TYPE_MYSQL);
    if (!g_pstDBHandle)
    {
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: create db handle fail!");
        return -1;
    }

    if (config_get_db_host(g_pstDBHandle->szHost, DB_MAX_STR_LEN) < 0)
    {
        DOS_ASSERT(0);
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: Get DB host fail !");
        goto errno_proc;
    }

    if (config_get_db_user(g_pstDBHandle->szUsername, DB_MAX_STR_LEN) < 0)
    {
        DOS_ASSERT(0);
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: Get DB username fail !");
        goto errno_proc;
    }

    if (config_get_db_password(g_pstDBHandle->szPassword, DB_MAX_STR_LEN) < 0)
    {
        DOS_ASSERT(0);
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: Get DB username password fail !");
        goto errno_proc;
    }

    g_pstDBHandle->usPort = config_get_db_port();
    if (0 == g_pstDBHandle->usPort || g_pstDBHandle->usPort)
    {
        g_pstDBHandle->usPort = 3306;
    }

    if (config_get_db_dbname(g_pstDBHandle->szDBName, DB_MAX_STR_LEN) < 0)
    {
        DOS_ASSERT(0);
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: Get DB Name fail !");
        goto errno_proc;
    }

    if (db_open(g_pstDBHandle) < 0)
    {
        DOS_ASSERT(0);
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: Open DB fail !");
        goto errno_proc;
    }

    return 0;

errno_proc:
    db_destroy(&g_pstDBHandle);
    return -1;
}


#ifdef __cplusplus
}
#endif /* __cplusplus */

