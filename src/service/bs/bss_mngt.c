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

#include <time.h>
#include <dos.h>
#include <json/dos_json.h>
#include <bs_pub.h>
#include "bs_cdr.h"
#include "bs_stat.h"
#include "bs_def.h"
#include "bsd_db.h"

#include <sys/un.h>

extern double ceil(double x);
extern S8 *strptime(const S8 *s, const S8 *format, struct tm *tm);
extern S32 bsd_walk_billing_package_tbl_bak(U32 ulPkgID);


/* ����WEB֪ͨ������ϯ������ */
VOID bss_update_agent(U32 ulOpteration, JSON_OBJ_ST *pstJSONObj)
{
    U32 ulCustomerID = U32_BUTT, ulGroupID1 = U32_BUTT, ulGroupID2 = U32_BUTT, ulAgentID = U32_BUTT;
    U32 ulHashIndex = U32_BUTT;
    const S8  *pszCustomID = NULL, *pszGroupID1 = NULL, *pszGroupID2 = NULL, *pszAgentID = NULL;
    const S8  *pszWhere = NULL;
    HASH_NODE_S *pstHashNode = NULL;
    JSON_OBJ_ST *pstJsonWhere = NULL;
    BS_AGENT_ST *pstAgentInfo = NULL;
    BS_CUSTOMER_ST  *pstCustomer = NULL;

    bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG, "Start update agent. Opteration:%d", ulOpteration);

    switch (ulOpteration)
    {
        case BS_CMD_UPDATE:
            pszGroupID1 = json_get_param(pstJSONObj, "group1_id");
            pszGroupID2 = json_get_param(pstJSONObj, "group2_id");

            if (DOS_ADDR_INVALID(pszGroupID1)
                || DOS_ADDR_INVALID(pszGroupID2)
                || dos_atoul(pszGroupID1, &ulGroupID1) < 0
                || dos_atoul(pszGroupID2, &ulGroupID2) < 0)
            {        DOS_ASSERT(0);

                bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG, "Invalid parameter while process agent update msg. Opteration:%d", ulOpteration);
                goto process_finished;
            }
            /*��ȡwhere����*/
            pszWhere = json_get_param(pstJSONObj, "where");
            if (DOS_ADDR_INVALID(pszWhere))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG, "Get where param FAIL.");
                break;
            }
            /*����json����*/
            pstJsonWhere = json_init((S8 *)pszWhere);
            if (DOS_ADDR_INVALID(pstJsonWhere))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG, "Init json node FAIL.");
                break;
            }

            /*��ȡid��customer id��job number*/
            pszAgentID = json_get_param(pstJsonWhere, "id");
            if (DOS_ADDR_INVALID(pszAgentID))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG, "Get Agent ID FAIL.");
                json_deinit(&pstJsonWhere);
                break;
            }
            pszCustomID = json_get_param(pstJsonWhere,"customer_id");
            if (DOS_ADDR_INVALID(pszCustomID))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG, "Get Customer ID FAIL.");
                json_deinit(&pstJsonWhere);
                break;
            }

            if (dos_atoul(pszCustomID, &ulCustomerID) < 0
                || dos_atoul(pszAgentID, &ulAgentID) < 0)
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG, "bss atoul FAIL.");
                json_deinit(&pstJsonWhere);
                break;
            }

            ulHashIndex = bs_hash_get_index(BS_HASH_TBL_AGENT_SIZE, ulAgentID);
            if (U32_BUTT == ulHashIndex)
            {
                DOS_ASSERT(0);
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG, "Get hash index FAIL while process agent update msg. Opteration:%d", ulOpteration);
                json_deinit(&pstJsonWhere);
                goto process_finished;
            }

            pthread_mutex_lock(&g_mutexAgentTbl);
            pstHashNode = hash_find_node(g_astAgentTbl, ulHashIndex, (VOID *)&ulAgentID, bs_agent_hash_node_match);
            if (DOS_ADDR_INVALID(pstHashNode)
                || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                pthread_mutex_unlock(&g_mutexAgentTbl);
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG, "Cannot find the agent while update. Opteration:%d", ulOpteration);
                json_deinit(&pstJsonWhere);
                break;
            }

            pstAgentInfo = (BS_AGENT_ST *)pstHashNode->pHandle;
            if (DOS_ADDR_INVALID(pstAgentInfo))
            {
                pthread_mutex_unlock(&g_mutexAgentTbl);
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG, "Invalid hash node while update. Opteration:%d", ulOpteration);
                json_deinit(&pstJsonWhere);
                break;
            }

            if (pstAgentInfo->ulCustomerID != ulCustomerID)
            {
                pthread_mutex_unlock(&g_mutexAgentTbl);
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG, "Customer ID is %u, but you find Customer %u.", pstAgentInfo->ulCustomerID, ulCustomerID);
                json_deinit(&pstJsonWhere);
                break;
            }

            pstAgentInfo->ulGroup1 = ulGroupID1;
            pstAgentInfo->ulGroup2 = ulGroupID2;

            pthread_mutex_unlock(&g_mutexAgentTbl);
            json_deinit(&pstJsonWhere);
            break;

        case BS_CMD_DELETE:
            /*��ȡwhere����*/
            pszWhere = json_get_param(pstJSONObj, "where");
            if (DOS_ADDR_INVALID(pszWhere))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG, "Get where param FAIL.");
                break;
            }
            /*����json����*/
            pstJsonWhere = json_init((S8 *)pszWhere);
            if (DOS_ADDR_INVALID(pstJsonWhere))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG, "Init json obj FAIL.");
                break;
            }
            /*��ȡid��customer id*/
            pszAgentID = json_get_param(pstJsonWhere, "id");
            if (DOS_ADDR_INVALID(pszAgentID))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG, "Get Agent ID FAIL.");
                json_deinit(&pstJsonWhere);
                break;
            }
           

            if (dos_atoul(pszAgentID, &ulAgentID) < 0)
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG, "dos_atoul FAIL.");
                json_deinit(&pstJsonWhere);
                break;
            }

            ulHashIndex = bs_hash_get_index(BS_HASH_TBL_AGENT_SIZE, ulAgentID);
            if (U32_BUTT == ulHashIndex)
            {
                DOS_ASSERT(0);
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG, "Get hash index FAIL while process agent update msg. Operation:%d", ulOpteration);
                json_deinit(&pstJsonWhere);
                goto process_finished;
            }

            pthread_mutex_lock(&g_mutexAgentTbl);
            pstHashNode = hash_find_node(g_astAgentTbl, ulHashIndex, (VOID *)&ulAgentID, bs_agent_hash_node_match);
            if (DOS_ADDR_INVALID(pstHashNode)
                || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                pthread_mutex_unlock(&g_mutexAgentTbl);
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG, "Cannot find the agent while update. Operation:%d", ulOpteration);
                json_deinit(&pstJsonWhere);
                break;
            }

            pstAgentInfo = (BS_AGENT_ST *)pstHashNode->pHandle;
            
            pstCustomer = bs_get_customer_st(pstAgentInfo->ulCustomerID);
            if (DOS_ADDR_INVALID(pstCustomer))
            {
                pthread_mutex_unlock(&g_mutexAgentTbl);
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG, "Connot find the customer with the id %u.(%u) Operation:%d"
                            , pstAgentInfo->ulAgentID
                            , pszCustomID
                            , ulOpteration);
                json_deinit(&pstJsonWhere);
                break;
            }

            hash_delete_node(g_astAgentTbl, pstHashNode, ulHashIndex);
            pthread_mutex_lock(&g_mutexCustomerTbl);
            pstCustomer->ulAgentNum--;
            pthread_mutex_unlock(&g_mutexCustomerTbl);
            json_deinit(&pstJsonWhere);
            pthread_mutex_unlock(&g_mutexAgentTbl);
            break;

        case BS_CMD_INSERT:
            pszGroupID1 = json_get_param(pstJSONObj, "group1_id");
            pszGroupID2 = json_get_param(pstJSONObj, "group2_id");

            if (DOS_ADDR_INVALID(pszGroupID1)
                || DOS_ADDR_INVALID(pszGroupID2)
                || dos_atoul(pszGroupID1, &ulGroupID1) < 0
                || dos_atoul(pszGroupID2, &ulGroupID2) < 0)
            {        DOS_ASSERT(0);

                bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG, "Invalid parameter while process agent update msg. Operation:%d", ulOpteration);
                goto process_finished;
            }
            /*��ȡid��customer id*/
            pszAgentID = json_get_param(pstJSONObj, "id");
            pszCustomID = json_get_param(pstJSONObj, "customer_id");
            if (DOS_ADDR_INVALID(pszAgentID)
                || DOS_ADDR_INVALID(pszCustomID)
                || dos_atoul(pszAgentID, &ulAgentID) < 0
                || dos_atoul(pszCustomID, &ulCustomerID) < 0)
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG, "Invalid param or dos_atoul FAIL.");
                break;
            }

            pstCustomer = bs_get_customer_st(ulCustomerID);
            if (DOS_ADDR_INVALID(pstCustomer))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG, "Cannot find the customer with the id %u. Operation:%u"
                            , ulCustomerID
                            , ulOpteration);
                break;
            }

            pstHashNode = dos_dmem_alloc(sizeof(HASH_NODE_S));
            if (DOS_ADDR_INVALID(pstHashNode))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG, "Cannot alloc memory while update agent. Operation:%d", ulOpteration);

                break;
            }
            HASH_Init_Node(pstHashNode);

            pstAgentInfo = dos_dmem_alloc(sizeof(BS_AGENT_ST));
            bs_init_agent_st(pstAgentInfo);

            if (DOS_ADDR_INVALID(pstAgentInfo))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG, "Cannot alloc memory while update agent. Operation:%d", ulOpteration);
                break;
            }

            memset(pstAgentInfo, 0, sizeof(BS_AGENT_ST));
            pstAgentInfo->ulAgentID = ulAgentID;
            pstAgentInfo->ulCustomerID = ulCustomerID;
            pstAgentInfo->ulGroup1 = ulGroupID1;
            pstAgentInfo->ulGroup2 = ulGroupID2;
            pstHashNode->pHandle = pstAgentInfo;

            ulHashIndex = bs_hash_get_index(BS_HASH_TBL_AGENT_SIZE, ulAgentID);
            if (U32_BUTT == ulHashIndex)
            {
                dos_dmem_free(pstAgentInfo);
                pstAgentInfo = NULL;
                pstHashNode->pHandle = NULL;

                dos_dmem_free(pstHashNode);
                pstHashNode = NULL;
            }

            pthread_mutex_lock(&g_mutexAgentTbl);
            hash_add_node(g_astAgentTbl, pstHashNode, ulHashIndex, NULL);
            pthread_mutex_lock(&g_mutexCustomerTbl);
            pstCustomer->ulAgentNum++;
            pthread_mutex_unlock(&g_mutexCustomerTbl);
            pthread_mutex_unlock(&g_mutexAgentTbl);
            break;

        default:
            break;
    }
    bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG, "Update agent. Operation:%d,Group1:%d, Group2:%d", ulOpteration, ulGroupID1, ulGroupID2);
process_finished:
    bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG, "Update agent finished. Operation:%d", ulOpteration);
}

/* �������WEB CMD��ʱ�����Ӧ */
VOID bss_update_customer(U32 ulOpteration, JSON_OBJ_ST *pstJSONObj)
{
    S64 LMoney = 0;
    U32 ulCustomerID = 0;
    time_t ulExpiryTime = 0;
    S8  *pszRet = NULL;
    struct tm stExpiryTm = {0};
    const S8 *pszMoney = NULL, *pszWhere  = NULL, *pszCustomID = NULL, *pszExpireTime = NULL;
    JSON_OBJ_ST  *pstJsonWhere = NULL;
    BS_CUSTOMER_ST  *pstCustomer = NULL;

    bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG, "Start update customer. Operation:%d", ulOpteration);


    /* �������䶯 */
    pszMoney = json_get_param(pstJSONObj, "money");
    if (DOS_ADDR_VALID(pszMoney))
    {
        if (dos_atoll(pszMoney, &LMoney) < 0)
        {
            goto process_finished;
        }

        /*��ȡwhere����*/
        pszWhere = json_get_param(pstJSONObj, "where");
        if (DOS_ADDR_INVALID(pszWhere))
        {
            DOS_ASSERT(0);
            goto process_finished;
        }

        /*��whereת��Ϊjson obj*/
        pstJsonWhere = json_init((S8 *)pszWhere);
        if (DOS_ADDR_INVALID(pstJsonWhere))
        {
            DOS_ASSERT(0);
            goto process_finished;
        }

        /*��ȡcustomer id*/
        pszCustomID = json_get_param(pstJsonWhere, "id");
        if (DOS_ADDR_INVALID(pszCustomID)
            || dos_atoul(pszCustomID, &ulCustomerID) <0)
        {
            DOS_ASSERT(0);
            goto process_finished;
        }

        pstCustomer = bs_get_customer_st(ulCustomerID);
        if (DOS_ADDR_INVALID(pstCustomer))
        {
            goto process_finished;
        }

        pstCustomer->stAccount.LBalanceActive += (LMoney * 10000);
        pstCustomer->stAccount.LBalance += (LMoney * 10000);

        goto process_finished;
    }

    bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG, "Start update customer. Operation:%d", ulOpteration);

    switch (ulOpteration)
    {
        case BS_CMD_UPDATE:
        {
            HASH_NODE_S     *pstHashNode = NULL;
            BS_CUSTOMER_ST  *pstCustomer = NULL;
            JSON_OBJ_ST     *pstSubJsonWhere = NULL;
            U32             ulHashIndex, ulCustomerType, ulCustomerState, ulCustomID;
            U32             ulPackageID, ulBanlanceWarning;
            S32             lMaximumBalance = U32_BUTT;
            const S8        *pszCustomType = NULL, *pszCustomState = NULL, *pszCustomID = NULL, *pszCustomName = NULL, *pszMinBalance = NULL;
            const S8        *pszBillingPkgID = NULL, *pszBalanceWarning = NULL;
            const S8        *pszSubWhere = NULL;

            /*����ǰʱ��ת��Ϊʱ��� */
            pszExpireTime = json_get_param(pstJSONObj, "expiry");
            if (DOS_ADDR_VALID(pszExpireTime))
            {
                pszRet = strptime(pszExpireTime, "%Y-%m-%d", &stExpiryTm);
                if (DOS_ADDR_INVALID(pszRet))
                {
                    bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG, "strptime FAIL.");
                }
                ulExpiryTime = mktime(&stExpiryTm);
            }

            /*��ȡwhere�ֶ�*/
            pszSubWhere = json_get_param(pstJSONObj, "where");
            if (DOS_ADDR_INVALID(pszSubWhere))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: Get where param FAIL.");
                break;
            }

            /*����where json����*/
            pstSubJsonWhere = json_init((S8 *)pszSubWhere);
            if (DOS_ADDR_INVALID(pstSubJsonWhere))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: Make Json Object FAIL.");
                break;
            }

            /*����json�����ȡcustomer id*/
            pszCustomID = json_get_param(pstSubJsonWhere, "id");
            if (DOS_ADDR_INVALID(pszCustomID)
                || dos_atoul(pszCustomID, &ulCustomID) < 0)
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: Invalid msg for delete user.");
                json_deinit(&pstSubJsonWhere);
                break;
            }

            ulHashIndex = bs_hash_get_index(BS_HASH_TBL_CUSTOMER_SIZE, ulCustomID);
            if (U32_BUTT == ulHashIndex)
            {
                DOS_ASSERT(0);
                json_deinit(&pstSubJsonWhere);
                break;
            }

            /* ���hash�����Ѿ����ڣ�˵���д��� */
            pstHashNode = hash_find_node(g_astCustomerTbl
                                    , ulHashIndex
                                    , (VOID *)&ulCustomID
                                    , bs_customer_hash_node_match);
            if (DOS_ADDR_INVALID(pstHashNode))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: Find the customer with the id %d FAIL.", ulCustomID);
                json_deinit(&pstSubJsonWhere);
                break;
            }

            pstCustomer = pstHashNode->pHandle;
            if (DOS_ADDR_INVALID(pstCustomer))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: Find the customer ID %u FAIL.(Invalid Data)", ulCustomID);
                json_deinit(&pstSubJsonWhere);
                break;
            }

            pszCustomName = json_get_param(pstSubJsonWhere, "name");
            pszMinBalance = json_get_param(pstJSONObj, "minimum_balance");

            pszCustomState = json_get_param(pstJSONObj, "status");
            pszCustomType = json_get_param(pstSubJsonWhere, "type");
            pszBillingPkgID = json_get_param(pstJSONObj, "billing_package_id");

            pszBalanceWarning = json_get_param(pstJSONObj, "balance_warning");

            if (DOS_ADDR_INVALID(pszCustomName) || DOS_ADDR_INVALID(pszCustomState)
               || DOS_ADDR_INVALID(pszCustomType) || DOS_ADDR_INVALID(pszBillingPkgID)
               || DOS_ADDR_INVALID(pszBalanceWarning) || DOS_ADDR_INVALID(pszMinBalance))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_NOTIC, "ERR: Parse json param FAIL while adding custom.");

                dos_dmem_free(pstCustomer);
                dos_dmem_free(pstHashNode);
                json_deinit(&pstSubJsonWhere);
                break;
            }

            /* �����Ϸ��� */
            if (dos_atoul(pszCustomID, &pstCustomer->ulCustomerID) < 0
                || dos_atoul(pszCustomType, &ulCustomerType) < 0
                || dos_atoul(pszCustomState, &ulCustomerState) < 0
                || dos_atoul(pszBillingPkgID, &ulPackageID) < 0
                || dos_atoul(pszBalanceWarning, &ulBanlanceWarning) < 0
                || dos_atol(pszMinBalance, &lMaximumBalance) < 0
                || '\0' == pszCustomName[0])
            {   
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_NOTIC, "ERR: Invalid param while adding custom.");

                dos_dmem_free(pstCustomer);
                dos_dmem_free(pstHashNode);
                json_deinit(&pstSubJsonWhere);
                break;
            }

            pstCustomer->stAccount.ulBillingPackageID = ulPackageID;
            pstCustomer->stAccount.lBalanceWarning = ulBanlanceWarning;
            if (DOS_ADDR_VALID(pszExpireTime))
            {
                pstCustomer->stAccount.ulExpiryTime = (U32)ulExpiryTime;
            }  
            pstCustomer->stAccount.lCreditLine  = lMaximumBalance;
            json_deinit(&pstSubJsonWhere);
            break;
        }
        case BS_CMD_DELETE:
        {
            U32  ulCustomID = U32_BUTT, ulHashIndex = U32_BUTT;
            const S8   *pszCustomID = NULL;
            const S8   *pszSubWhere = NULL;
            JSON_OBJ_ST *pstSubJsonWhere = NULL;
            HASH_NODE_S     *pstHashNode = NULL, *pstHashNodeParent = NULL;
            BS_CUSTOMER_ST  *pstCustomer = NULL, *pstCustomParent = NULL;

            /*��ȡwhere*/
            pszSubWhere = json_get_param(pstJSONObj, "where");
            if (DOS_ADDR_INVALID(pszSubWhere))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: Get where param FAIL.");
                break;
            }
            pstSubJsonWhere = json_init((S8 *)pszSubWhere);
            if (DOS_ADDR_INVALID(pstSubJsonWhere))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: json init where FAIL.");
                break;
            }
            pszCustomID = json_get_param(pstSubJsonWhere, "id");
            if (DOS_ADDR_INVALID(pszCustomID)
                || dos_atoul(pszCustomID, &ulCustomID) < 0)
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: Invalid msg for delete user.");
                break;
            }

            ulHashIndex = bs_hash_get_index(BS_HASH_TBL_CUSTOMER_SIZE, ulCustomID);
            if (U32_BUTT == ulHashIndex)
            {
                DOS_ASSERT(0);
                break;
            }

            /* ���hash�����Ѿ������ڣ�˵���д��� */
            pstHashNode = hash_find_node(g_astCustomerTbl
                                    , ulHashIndex
                                    , (VOID *)&ulCustomID
                                    , bs_customer_hash_node_match);
            if (DOS_ADDR_INVALID(pstHashNode))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: Find the customer ID %u FAIL.", ulCustomID);
                break;
            }

            pstCustomer = pstHashNode->pHandle;
            if (DOS_ADDR_INVALID(pstCustomer))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: Find the customer with the id %d FAIL.(Invalid Data)", ulCustomID);
                break;
            }

            pstHashNodeParent = hash_find_node(g_astCustomerTbl
                                    , ulHashIndex
                                    , (VOID *)&pstCustomer->ulParentID
                                    , bs_customer_hash_node_match);
            if (DOS_ADDR_INVALID(pstHashNodeParent))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: Find parent for the customer with the id %d, parent: %d FAIL.", ulCustomID, pstCustomer->ulParentID);
                break;
            }

            pstCustomParent = pstHashNodeParent->pHandle;
            if (DOS_ADDR_INVALID(pstCustomParent))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: Find parent for the customer with the id %d, parent: %d FAIL.(Invalid Data)", ulCustomID, pstCustomer->ulParentID);
                break;
            }

            pthread_mutex_lock(&g_mutexCustomerTbl);
            if (0 != pstCustomer->stChildrenList.ulCount)
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: Delete the customer with the id %d FAIL.(Customer have childen)", pstCustomer->ulParentID);
                pthread_mutex_unlock(&g_mutexCustomerTbl);
                break;
            }

            dll_delete(&pstCustomParent->stChildrenList, (DLL_NODE_S *)pstCustomer);
            hash_delete_node(g_astCustomerTbl, pstHashNode, ulHashIndex);
            dos_dmem_free(pstCustomer);
            pstCustomer = NULL;
            pstHashNode->pHandle = NULL;
            dos_dmem_free(pstHashNode);
            pstHashNode = NULL;

            pthread_mutex_unlock(&g_mutexCustomerTbl);

            break;
        }
        case BS_CMD_INSERT:
        {
            U32             ulHashIndex, ulCustomerType, ulCustomerState, ulBillingPackageID, ulMinBalance, ulBalanceWarning, ulBalance;
            const S8        *pszCustomType, *pszCustomState, *pszCustomID, *pszCustomName, *pszParent;
            const S8        *pszBillingPackageID = NULL, *pszMinBalance = NULL, *pszBalanceWarning = NULL, *pszBalance = NULL;
            HASH_NODE_S     *pstHashNode = NULL;
            BS_CUSTOMER_ST  *pstCustomer = NULL, *pstCustomParent = NULL;

            /*����ǰʱ��ת��Ϊʱ��� */
            pszExpireTime = json_get_param(pstJSONObj, "expiry");
            if (DOS_ADDR_VALID(pszExpireTime))
            {
                pszRet = strptime(pszExpireTime, "%Y-%m-%d", &stExpiryTm);
                if (DOS_ADDR_INVALID(pszRet))
                {
                    bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG, "strptime FAIL.");
                }
                ulExpiryTime = mktime(&stExpiryTm);
            }
            
            pstHashNode = dos_dmem_alloc(sizeof(HASH_NODE_S));
            if (DOS_ADDR_INVALID(pstHashNode))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: alloc memory FAIL!");
                break;
            }
            HASH_Init_Node(pstHashNode);

            pstCustomer = dos_dmem_alloc(sizeof(BS_CUSTOMER_ST));
            if (DOS_ADDR_INVALID(pstCustomer))
            {
                dos_dmem_free(pstHashNode);
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: alloc memory FAIL!");
                break;
            }
            bs_init_customer_st(pstCustomer);

            /* ��json�����л�ȡ���� */
            pszCustomName = json_get_param(pstJSONObj, "name");
            pszCustomID = json_get_param(pstJSONObj, "id");
            pszParent = json_get_param(pstJSONObj, "parent_id");
            pszCustomState = json_get_param(pstJSONObj, "status");
            pszCustomType = json_get_param(pstJSONObj, "type");
            pszBillingPackageID = json_get_param(pstJSONObj, "billing_package_id");
            pszMinBalance = json_get_param(pstJSONObj, "minimum_balance");
            pszBalanceWarning = json_get_param(pstJSONObj, "balance_warning");
            pszBalance = json_get_param(pstJSONObj, "balance");

            if (DOS_ADDR_INVALID(pszCustomName) || DOS_ADDR_INVALID(pszCustomID)
                || DOS_ADDR_INVALID(pszParent) || DOS_ADDR_INVALID(pszCustomState)
                || DOS_ADDR_INVALID(pszCustomType) || DOS_ADDR_INVALID(pszBillingPackageID)
                || DOS_ADDR_INVALID(pszMinBalance) || DOS_ADDR_INVALID(pszBalanceWarning)
                || DOS_ADDR_INVALID(pszBalance))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_NOTIC, "ERR: Parse json param FAIL while adding custom.");

                dos_dmem_free(pstCustomer);
                dos_dmem_free(pstHashNode);

                break;
            }

            /* �����Ϸ��� */
            if (dos_atoul(pszCustomID, &pstCustomer->ulCustomerID) < 0
                || dos_atoul(pszParent, &pstCustomer->ulParentID) < 0
                || dos_atoul(pszCustomType, &ulCustomerType) < 0
                || dos_atoul(pszCustomState, &ulCustomerState) < 0
                || dos_atoul(pszBillingPackageID, &ulBillingPackageID) < 0
                || dos_atoul(pszMinBalance, &ulMinBalance) < 0
                || dos_atoul(pszBalanceWarning, &ulBalanceWarning) < 0
                || dos_atoul(pszBalance, &ulBalance) < 0
                || '\0' == pszCustomName[0])
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_NOTIC, "ERR: Invalid param while adding custom.");

                dos_dmem_free(pstCustomer);
                dos_dmem_free(pstHashNode);

                break;
            }
            pstCustomer->ucCustomerState = (U8)ulCustomerState;
            pstCustomer->ucCustomerType = (U8)ulCustomerType;
            pstCustomer->stAccount.ulBillingPackageID = ulBillingPackageID;
            pstCustomer->stAccount.lCreditLine = (S32)ulMinBalance;
            pstCustomer->stAccount.lBalanceWarning = (S32)ulBalanceWarning;
            pstCustomer->stAccount.LBalance = (S64)ulBalance;
            if (DOS_ADDR_VALID(pszExpireTime))
            {
                pstCustomer->stAccount.ulExpiryTime = (U32)ulExpiryTime;
            }

            dos_snprintf(pstCustomer->szCustomerName, sizeof(pstCustomer->szCustomerName), "%s", pszCustomName);

            pstHashNode->pHandle = (VOID *)pstCustomer;
            ulHashIndex = bs_hash_get_index(BS_HASH_TBL_CUSTOMER_SIZE, pstCustomer->ulCustomerID);
            if (U32_BUTT == ulHashIndex)
            {
                DOS_ASSERT(0);
                
                dos_dmem_free(pstHashNode);
                pstHashNode = NULL;
                
                dos_dmem_free(pstCustomer);
                pstCustomer = NULL;
                
                break;
            }

            /* ���hash�����Ѿ����ڣ�˵���д��� */
            if (hash_find_node(g_astCustomerTbl,
                                  ulHashIndex,
                                  (VOID *)&pstCustomer->ulCustomerID,
                                  bs_customer_hash_node_match))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: customer(%u:%s) is duplicated in DB !",
                pstCustomer->ulCustomerID, pstCustomer->szCustomerName);
                
                dos_dmem_free(pstHashNode);
                pstHashNode = NULL;
                
                dos_dmem_free(pstCustomer);
                pstCustomer = NULL;

                break;
            }

            pstCustomParent = bs_get_customer_st(pstCustomer->ulParentID);
            if (DOS_ADDR_INVALID(pstCustomParent))
            {
                /* û���ҵ����ͻ����Ƿ� */
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_NOTIC
                        , "ERR: Invalid parent customer id while adding custom.(parent:%d,custom:%d)"
                        , pstCustomer->ulParentID
                        , pstCustomer->ulCustomerID);

                dos_dmem_free(pstCustomer);
                pstCustomer = NULL;
                
                dos_dmem_free(pstHashNode);
                pstHashNode = NULL;

                break;
            }

            /* �ϼ��ͻ�������������,ֻ�п����Ǵ����̻򶥼��ͻ� */
            if (BS_CUSTOMER_TYPE_CONSUMER == pstCustomParent->ucCustomerType)
            {
                bs_trace(BS_TRACE_DB, LOG_LEVEL_ERROR, "ERR: The parent of customer(%u:%s) is a comsuer(%u:%s)!",
                         pstCustomer->ulCustomerID, pstCustomer->szCustomerName,
                         pstCustomParent->ulCustomerID, pstCustomParent->szCustomerName);

                dos_dmem_free(pstCustomer);
                pstCustomer = NULL;
                
                dos_dmem_free(pstHashNode);
                pstHashNode = NULL;

                break;
            }

            /* ��HASH���б��� */
            pthread_mutex_lock(&g_mutexCustomerTbl);
            hash_add_node(g_astCustomerTbl, pstHashNode, ulHashIndex, NULL);
            g_astCustomerTbl->NodeNum++;
            pthread_mutex_unlock(&g_mutexCustomerTbl);

            /* ���¿ͻ����ƿ���Ϣ */
            pstCustomer->stAccount.LBalanceActive = pstCustomer->stAccount.LBalance;
            pstCustomer->stNode.pHandle = pstHashNode;

            /* ���¿ͻ��� */
            pstCustomer->pstParent = pstCustomParent;
            bs_customer_add_child(pstCustomParent, pstCustomer);

            break;
        }
        default:
            bs_trace(BS_TRACE_RUN, LOG_LEVEL_NOTIC, "ERR: Unknow command while update customer");
            break;
    }

process_finished:
    if (DOS_ADDR_VALID(pstJsonWhere))
    {
        json_deinit(&pstJsonWhere);
    }
    bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG, "Finish to update customer. Operation:%d", ulOpteration);
}

/* ��Ӫ�̸����ʷѹ��� */
VOID bss_update_billing_package(U32 ulOpteration, JSON_OBJ_ST *pstJSONObj)
{
    BS_BILLING_PACKAGE_ST *pstPkg = NULL;
    struct tm stEffectTime, stExpiryTime;
    time_t ulEffectTime = 0, ulExpiryTime = 0;
    JSON_OBJ_ST  *pstJsonWhere = NULL;
    HASH_NODE_S *pstHashNode = NULL;
    const S8 *pszPkgID = NULL, *pszRuleID = NULL, *pszTable = NULL, *pszServType = NULL, *pszBillingType = NULL, *pszBillingRate = NULL;
    const S8 *pszSrcAttrType1 = NULL, *pszSrcAttrType2 = NULL, *pszSrcAttrValue1 = NULL, *pszSrcAttrValue2 = NULL;
    const S8 *pszFirstBillingUnit = NULL, *pszNextBillingUnit = NULL, *pszFirstBillingCnt = NULL, *pszNextBillingCnt = NULL;
    const S8 *pszDstAttrType1 = NULL, *pszDstAttrType2 = NULL, *pszDstAttrValue1 = NULL, *pszDstAttrValue2 = NULL;
    const S8 *pszEffectTime = NULL, *pszExpiryTime = NULL;
    const S8 *pszWhere = NULL;
    S8       *pszRet = NULL;
    U32 ulPkgID = U32_BUTT, ulRuleID = U32_BUTT, ulServType = U32_BUTT;
    U32 ulSrcAttrType1 = U32_BUTT, ulSrcAttrType2 = U32_BUTT, ulSrcAttrValue1 = U32_BUTT, ulSrcAttrValue2 = U32_BUTT;
    U32 ulDstAttrType1 = U32_BUTT, ulDstAttrType2 = U32_BUTT, ulDstAttrValue1 = U32_BUTT, ulDstAttrValue2 = U32_BUTT;
    U32 ulFirstBillingUnit = U32_BUTT, ulNextBillingUnit = U32_BUTT, ulFirstBillingCnt = U32_BUTT, ulNextBillingCnt= U32_BUTT;
    U32 ulBillingType = U32_BUTT, ulBillingRate = U32_BUTT;
    U32 ulHashIndex = 0;
    U32 ulLoop = 0;
    BOOL bFound = DOS_FALSE, bFoundRule = DOS_FALSE;

    pszTable = json_get_param(pstJSONObj, "table");
    if (DOS_ADDR_INVALID(pszTable))
    {
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Get param value FAIL.");
        return ;
    }
    /*��ʱ���ַ���תΪʱ���*/

    switch(ulOpteration)
    {
        case BS_CMD_UPDATE:
        {
            pszEffectTime = json_get_param(pstJSONObj, "effect_time");
            pszExpiryTime = json_get_param(pstJSONObj, "expire_time");

            if (DOS_ADDR_INVALID(pszEffectTime)
                || DOS_ADDR_INVALID(pszExpiryTime))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Get Effect Time & Expire Time FAIL.");
            }

            pszRet = strptime(pszEffectTime, "%Y-%m-%d", &stEffectTime);
            if (DOS_ADDR_INVALID(pszRet))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Transfer time str to time_t FAIL.");
            }
            pszRet = strptime(pszExpiryTime, "%Y-%m-%d", &stExpiryTime);
            if (DOS_ADDR_INVALID(pszRet))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Transfer time str to time_t FAIL.");
            }

            ulEffectTime = mktime(&stEffectTime);
            ulExpiryTime = mktime(&stExpiryTime);

            pszPkgID = json_get_param(pstJSONObj, "billing_package_id");
            pszServType = json_get_param(pstJSONObj, "serv_type");

            if (DOS_ADDR_INVALID(pszPkgID) 
                || DOS_ADDR_INVALID(pszServType))
            {
                bs_trace(BS_TRACE_RUN,LOG_LEVEL_ERROR , "Get Billing Package ID or Service Type FAIL");
                break;
            }

            if (dos_atoul(pszPkgID, &ulPkgID) < 0
                || dos_atoul(pszServType, &ulServType) < 0)
            {
                bs_trace(BS_TRACE_RUN,LOG_LEVEL_ERROR , "dos_atoul FAIL.");
                break;
            }

            ulHashIndex = bs_hash_get_index(BS_HASH_TBL_BILLING_PACKAGE_SIZE, ulPkgID);
            if (U32_BUTT == ulHashIndex)
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Package ID %u does not exist.", ulPkgID);
                break;
            }
            pstHashNode = hash_find_node(g_astBillingPackageTbl, ulHashIndex, (VOID *)&ulPkgID, bs_billing_package_hash_node_match);
            if (DOS_ADDR_INVALID(pstHashNode)
                || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Package ID %u does not exist.", ulPkgID);
                break;
            }
            pstPkg = (BS_BILLING_PACKAGE_ST *)pstHashNode->pHandle;

            /*��ȡ�Ʒѹ���ID*/
            pszWhere = json_get_param(pstJSONObj, "where");
            if (DOS_ADDR_INVALID(pszWhere))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Get where param FAIL.");
                break;
            }
            pstJsonWhere = json_init((S8 *)pszWhere);
            if (DOS_ADDR_INVALID(pstJsonWhere))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Init json node FAIL.");
                break;
            }
            pszRuleID = json_get_param(pstJsonWhere, "id");
            if (DOS_ADDR_INVALID(pszRuleID))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Get Rule ID FAIL.");
                json_deinit(&pstJsonWhere);
                break;
            }
            if (dos_atoul(pszRuleID, &ulRuleID) < 0)
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "dos_atoul FAIL.");
                json_deinit(&pstJsonWhere);
                break;
            }

            for(ulLoop = 0; ulLoop < BS_MAX_BILLING_RULE_IN_PACKAGE; ++ulLoop)
            {
                /*����ҵ����ʷѹ����ҿ���*/
                if (pstPkg->astRule[ulLoop].ulRuleID == ulRuleID && 1 == pstPkg->astRule[ulLoop].ucValid)
                {
                    pszFirstBillingUnit = json_get_param(pstJSONObj, "first_billing_unit");
                    pszNextBillingUnit = json_get_param(pstJSONObj, "next_billing_unit");
                    pszFirstBillingCnt = json_get_param(pstJSONObj, "first_billing_cnt");
                    pszNextBillingCnt = json_get_param(pstJSONObj, "next_billing_cnt");
                    pszBillingType = json_get_param(pstJSONObj, "billing_type");
                    pszBillingRate = json_get_param(pstJSONObj, "billing_rate");
                    
                    if (DOS_ADDR_INVALID(pszFirstBillingUnit) || DOS_ADDR_INVALID(pszNextBillingUnit)
                        || DOS_ADDR_INVALID(pszFirstBillingCnt) || DOS_ADDR_INVALID(pszNextBillingCnt)
                        || DOS_ADDR_INVALID(pszBillingType) || DOS_ADDR_INVALID(pszBillingRate))
                    {
                        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Invalid Param.");
                        json_deinit(&pstJsonWhere);
                        break;
                    }

                    if (dos_atoul(pszFirstBillingUnit, &ulFirstBillingUnit) < 0
                        || dos_atoul(pszNextBillingUnit, &ulNextBillingUnit) < 0
                        || dos_atoul(pszFirstBillingCnt, &ulFirstBillingCnt) < 0
                        || dos_atoul(pszNextBillingCnt, &ulNextBillingCnt) < 0
                        || dos_atoul(pszBillingType, &ulBillingType) < 0
                        || dos_atoul(pszBillingRate, &ulBillingRate) < 0)
                    {
                        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "dos_atoul FAIL.");
                        json_deinit(&pstJsonWhere);
                        break;
                    }

                    pstPkg->astRule[ulLoop].ulFirstBillingUnit = ulFirstBillingUnit;
                    pstPkg->astRule[ulLoop].ulNextBillingUnit = ulNextBillingUnit;
                    pstPkg->astRule[ulLoop].ucFirstBillingCnt = (U8)ulFirstBillingCnt;
                    pstPkg->astRule[ulLoop].ucNextBillingCnt = (U8)ulNextBillingCnt;
                    pstPkg->astRule[ulLoop].ucBillingType = (U8)ulBillingType;
                    pstPkg->astRule[ulLoop].ulBillingRate = ulBillingRate;
                    pstPkg->astRule[ulLoop].ulEffectTimestamp = ulEffectTime;
                    pstPkg->astRule[ulLoop].ulExpireTimestamp = ulExpiryTime;
                    pstPkg->astRule[ulLoop].ucServType = (U8)ulServType;
                    pstPkg->ucServType = (U8)ulServType;

                    bFound = DOS_TRUE;
                }
            }
            if (DOS_FALSE == bFound)
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Find Package Billing Rule FAIL.");
                json_deinit(&pstJsonWhere);
                break;
            }
            break;
        }
        case BS_CMD_DELETE:
        {
            pszWhere = json_get_param(pstJSONObj, "where");
            if (DOS_ADDR_INVALID(pszWhere))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Get where param FAIL.");
                break;
            }
            pstJsonWhere = json_init((S8 *)pszWhere);
            if (DOS_ADDR_INVALID(pstJsonWhere))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Init json FAIL.");
                break;
            }
            pszRuleID = json_get_param(pstJsonWhere, "id");
            if (DOS_ADDR_INVALID(pszRuleID))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Get Rule ID fail.");
                json_deinit(&pstJsonWhere);
                break;
            }
            if (dos_atoul(pszRuleID, &ulRuleID) < 0)
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "dos_atoul fail.");
                json_deinit(&pstJsonWhere);
                break;
            }

            HASH_Scan_Table(g_astBillingPackageTbl, ulHashIndex)
            {
                HASH_Scan_Bucket(g_astBillingPackageTbl,ulHashIndex, pstHashNode, HASH_NODE_S *)
                {
                    if (DOS_ADDR_INVALID(pstHashNode)
                        || DOS_ADDR_INVALID(pstHashNode->pHandle))
                    {
                        continue;
                    }

                    pstPkg = (BS_BILLING_PACKAGE_ST *)pstHashNode->pHandle;
                    for (ulLoop = 0; ulLoop < BS_MAX_BILLING_RULE_IN_PACKAGE; ++ulLoop)
                    {
                        if (pstPkg->astRule[ulLoop].ulRuleID == ulRuleID)
                        {
                            pstPkg->astRule[ulLoop].ucBillingType = U8_BUTT;
                            pstPkg->astRule[ulLoop].ucDstAttrType1 = U8_BUTT;
                            pstPkg->astRule[ulLoop].ucDstAttrType2 = U8_BUTT;
                            pstPkg->astRule[ulLoop].ucFirstBillingCnt = U8_BUTT;
                            pstPkg->astRule[ulLoop].ucNextBillingCnt = U8_BUTT;
                            pstPkg->astRule[ulLoop].ucPriority = U8_BUTT;
                            pstPkg->astRule[ulLoop].ucServType = BS_SERV_BUTT;
                            pstPkg->astRule[ulLoop].ucSrcAttrType1 = U8_BUTT;
                            pstPkg->astRule[ulLoop].ucSrcAttrType2 = U8_BUTT;
                            pstPkg->astRule[ulLoop].ucValid = 0;
                            pstPkg->astRule[ulLoop].ulBillingRate = U32_BUTT;
                            pstPkg->astRule[ulLoop].ulDstAttrValue1 = U32_BUTT;
                            pstPkg->astRule[ulLoop].ulDstAttrValue2 = U32_BUTT;
                            pstPkg->astRule[ulLoop].ulEffectTimestamp = U32_BUTT;
                            pstPkg->astRule[ulLoop].ulExpireTimestamp = U32_BUTT;
                            pstPkg->astRule[ulLoop].ulFirstBillingUnit = U32_BUTT;
                            pstPkg->astRule[ulLoop].ulNextBillingUnit = U32_BUTT;
                            pstPkg->astRule[ulLoop].ulPackageID = U32_BUTT;
                            pstPkg->astRule[ulLoop].ulRuleID = U32_BUTT;
                            pstPkg->astRule[ulLoop].ulSrcAttrValue1 = U32_BUTT;
                            pstPkg->astRule[ulLoop].ulSrcAttrValue2 = U32_BUTT;
                            pstPkg->astRule[ulLoop].ulRuleID = U32_BUTT;
                        }
                    }
                }
            }
            json_deinit(&pstJsonWhere);
            break;
        }
        case BS_CMD_INSERT:
        {
            pszEffectTime = json_get_param(pstJSONObj, "effect_time");
            pszExpiryTime = json_get_param(pstJSONObj, "expire_time");

            if (DOS_ADDR_INVALID(pszEffectTime)
                || DOS_ADDR_INVALID(pszExpiryTime))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Get Effect Time & Expiry Time FAIL.");
            }

            pszRet = strptime(pszEffectTime, "%Y-%m-%d", &stEffectTime);
            if (DOS_ADDR_INVALID(pszRet))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Transfer time str to time_t FAIL.");
            }
            pszRet = strptime(pszExpiryTime, "%Y-%m-%d", &stExpiryTime);
            if (DOS_ADDR_INVALID(pszRet))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Transfer time str to time_t FAIL.");
            }

            ulEffectTime = mktime(&stEffectTime);
            ulExpiryTime = mktime(&stExpiryTime);
            pszPkgID = json_get_param(pstJSONObj, "billing_package_id");
            pszSrcAttrType1 = json_get_param(pstJSONObj, "src_attr_type1");
            pszSrcAttrType2 = json_get_param(pstJSONObj, "src_attr_type2");
            pszDstAttrType1 = json_get_param(pstJSONObj, "dst_attr_type1");
            pszDstAttrType2 = json_get_param(pstJSONObj, "dst_attr_type2");
            pszSrcAttrValue1 = json_get_param(pstJSONObj, "src_attr_value1");
            pszSrcAttrValue2 = json_get_param(pstJSONObj, "src_attr_value2");
            pszDstAttrValue1 = json_get_param(pstJSONObj, "dst_attr_value1");
            pszDstAttrValue2 = json_get_param(pstJSONObj, "dst_attr_value2");
            pszServType = json_get_param(pstJSONObj, "serv_type");
            pszBillingType = json_get_param(pstJSONObj, "billing_type");
            pszFirstBillingUnit = json_get_param(pstJSONObj, "first_billing_unit");
            pszNextBillingUnit = json_get_param(pstJSONObj, "next_billing_unit");
            pszFirstBillingCnt = json_get_param(pstJSONObj, "first_billing_cnt");
            pszNextBillingCnt = json_get_param(pstJSONObj, "next_billing_cnt");
            pszBillingRate = json_get_param(pstJSONObj, "billing_rate");
            pszRuleID = json_get_param(pstJSONObj, "id");

            if (DOS_ADDR_INVALID(pszPkgID)|| DOS_ADDR_INVALID(pszSrcAttrType1)
                || DOS_ADDR_INVALID(pszSrcAttrType2) || DOS_ADDR_INVALID(pszDstAttrType1)
                || DOS_ADDR_INVALID(pszDstAttrType2) || DOS_ADDR_INVALID(pszSrcAttrValue1)
                || DOS_ADDR_INVALID(pszSrcAttrValue2) || DOS_ADDR_INVALID(pszDstAttrValue1)
                || DOS_ADDR_INVALID(pszDstAttrValue2) || DOS_ADDR_INVALID(pszServType)
                || DOS_ADDR_INVALID(pszBillingType) || DOS_ADDR_INVALID(pszFirstBillingUnit)
                || DOS_ADDR_INVALID(pszNextBillingUnit) || DOS_ADDR_INVALID(pszFirstBillingCnt)
                || DOS_ADDR_INVALID(pszNextBillingCnt) || DOS_ADDR_INVALID(pszBillingRate)
                || DOS_ADDR_INVALID(pszRuleID))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Get param Value FAIL.");
                break;
            }
            if (dos_atoul(pszPkgID, &ulPkgID) < 0
                || dos_atoul(pszSrcAttrType1, &ulSrcAttrType1) < 0
                || dos_atoul(pszSrcAttrType2, &ulSrcAttrType2) < 0
                || dos_atoul(pszDstAttrType1, &ulDstAttrType1) < 0
                || dos_atoul(pszDstAttrType2, &ulDstAttrType2) < 0
                || dos_atoul(pszSrcAttrValue1, &ulSrcAttrValue1) < 0
                || dos_atoul(pszSrcAttrValue2, &ulSrcAttrValue2) < 0
                || dos_atoul(pszDstAttrValue1, &ulDstAttrValue1) < 0
                || dos_atoul(pszDstAttrValue2, &ulDstAttrValue2) < 0
                || dos_atoul(pszServType, &ulServType) < 0
                || dos_atoul(pszBillingType, &ulBillingType) < 0
                || dos_atoul(pszFirstBillingUnit, &ulFirstBillingUnit) < 0
                || dos_atoul(pszNextBillingUnit, &ulNextBillingUnit) < 0
                || dos_atoul(pszFirstBillingCnt, &ulFirstBillingCnt) < 0
                || dos_atoul(pszNextBillingCnt, &ulNextBillingCnt) < 0
                || dos_atoul(pszBillingRate, &ulBillingRate) < 0
                || dos_atoul(pszRuleID, &ulRuleID) < 0)
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "dos_atoul FAIL.");
                break;
            }

            HASH_Scan_Table(g_astBillingPackageTbl,ulHashIndex)
            {
                HASH_Scan_Bucket(g_astBillingPackageTbl, ulHashIndex, pstHashNode, HASH_NODE_S *)
                {
                    if (DOS_ADDR_INVALID(pstHashNode)
                        || DOS_ADDR_INVALID(pstHashNode->pHandle))
                    {
                        continue;
                    }

                    pstPkg = (BS_BILLING_PACKAGE_ST *)pstHashNode->pHandle;
                    if (pstPkg->ulPackageID == ulPkgID)
                    {
                        bFound = DOS_TRUE;
                        break;
                    }
                }
                if (bFound == DOS_TRUE)
                {
                    break;
                }
            }
            /*�����ϣ���д��ڸ��ʷѰ�*/
            if (DOS_TRUE == bFound)
            {
                ulHashIndex = bs_hash_get_index(BS_HASH_TBL_BILLING_PACKAGE_SIZE, ulPkgID);
                pstHashNode = hash_find_node(g_astBillingPackageTbl, ulHashIndex, (VOID *)&ulPkgID, bs_billing_package_hash_node_match);
                pstPkg = (BS_BILLING_PACKAGE_ST *)pstHashNode->pHandle;

                for (ulLoop = 0; ulLoop < BS_MAX_BILLING_RULE_IN_PACKAGE; ++ulLoop)
                {
                    /*������ڸüƷѹ���*/
                    if (pstPkg->astRule[ulLoop].ulRuleID == ulRuleID)
                    {
                        pstPkg->astRule[ulLoop].ucBillingType = (U8)ulBillingType;
                        pstPkg->astRule[ulLoop].ucDstAttrType1 = (U8)ulDstAttrType1;
                        pstPkg->astRule[ulLoop].ucDstAttrType2 = (U8)ulDstAttrType2;
                        pstPkg->astRule[ulLoop].ucFirstBillingCnt = (U8)ulFirstBillingCnt;
                        pstPkg->astRule[ulLoop].ucNextBillingCnt = (U8)ulNextBillingCnt;
                        pstPkg->astRule[ulLoop].ucPriority = 0;
                        pstPkg->astRule[ulLoop].ucServType = (U8)ulServType;
                        pstPkg->astRule[ulLoop].ucSrcAttrType1 = (U8)ulSrcAttrType1;
                        pstPkg->astRule[ulLoop].ucSrcAttrType2 = (U8)ulSrcAttrType2;
                        pstPkg->astRule[ulLoop].ucValid = 1;
                        pstPkg->astRule[ulLoop].ulBillingRate = ulBillingRate;
                        pstPkg->astRule[ulLoop].ulDstAttrValue1 = ulDstAttrValue1;
                        pstPkg->astRule[ulLoop].ulDstAttrValue2 = ulDstAttrValue2;
                        pstPkg->astRule[ulLoop].ulEffectTimestamp = ulEffectTime;
                        pstPkg->astRule[ulLoop].ulExpireTimestamp = ulExpiryTime;
                        pstPkg->astRule[ulLoop].ulFirstBillingUnit = ulFirstBillingUnit;
                        pstPkg->astRule[ulLoop].ulNextBillingUnit = ulNextBillingUnit;
                        pstPkg->astRule[ulLoop].ulPackageID = ulPkgID;
                        pstPkg->astRule[ulLoop].ulSrcAttrValue1 = ulSrcAttrValue1;
                        pstPkg->astRule[ulLoop].ulSrcAttrValue2 = ulSrcAttrValue2;
                        bFoundRule = DOS_TRUE;
                        break;
                    }
                }
                if (DOS_FALSE == bFoundRule)
                {
                    for (ulLoop = 0; ulLoop < BS_MAX_BILLING_RULE_IN_PACKAGE; ++ulLoop)
                    {
                        /*�ҵ���һ�������õĽڵ�ȥ���*/
                        if (0 == pstPkg->astRule[ulLoop].ucValid)
                        {
                            pstPkg->astRule[ulLoop].ucBillingType = (U8)ulBillingType;
                            pstPkg->astRule[ulLoop].ucDstAttrType1 = (U8)ulDstAttrType1;
                            pstPkg->astRule[ulLoop].ucDstAttrType2 = (U8)ulDstAttrType2;
                            pstPkg->astRule[ulLoop].ucFirstBillingCnt = (U8)ulFirstBillingCnt;
                            pstPkg->astRule[ulLoop].ucNextBillingCnt = (U8)ulNextBillingCnt;
                            pstPkg->astRule[ulLoop].ucPriority = 0;
                            pstPkg->astRule[ulLoop].ucServType = (U8)ulServType;
                            pstPkg->astRule[ulLoop].ucSrcAttrType1 = (U8)ulSrcAttrType1;
                            pstPkg->astRule[ulLoop].ucSrcAttrType2 = (U8)ulSrcAttrType2;
                            pstPkg->astRule[ulLoop].ucValid = 1;
                            pstPkg->astRule[ulLoop].ulBillingRate = ulBillingRate;
                            pstPkg->astRule[ulLoop].ulDstAttrValue1 = ulDstAttrValue1;
                            pstPkg->astRule[ulLoop].ulDstAttrValue2 = ulDstAttrValue2;
                            pstPkg->astRule[ulLoop].ulEffectTimestamp = ulEffectTime;
                            pstPkg->astRule[ulLoop].ulExpireTimestamp = ulExpiryTime;
                            pstPkg->astRule[ulLoop].ulFirstBillingUnit = ulFirstBillingUnit;
                            pstPkg->astRule[ulLoop].ulNextBillingUnit = ulNextBillingUnit;
                            pstPkg->astRule[ulLoop].ulPackageID = ulPkgID;
                            pstPkg->astRule[ulLoop].ulSrcAttrValue1 = ulSrcAttrValue1;
                            pstPkg->astRule[ulLoop].ulSrcAttrValue2 = ulSrcAttrValue2;
                            break;
                        }
                     }
                 }
            }
            /*�ʷѰ����ڹ�ϣ����*/
            else
            {
                pstHashNode = dos_dmem_alloc(sizeof(HASH_NODE_S));
                if (DOS_ADDR_INVALID(pstHashNode))
                {
                    bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: alloc memory FAIL!");
                    break;
                }
                HASH_Init_Node(pstHashNode);

                pstPkg = dos_dmem_alloc(sizeof(BS_BILLING_PACKAGE_ST));
                memset(pstPkg, 0, sizeof(BS_BILLING_PACKAGE_ST));
                if (DOS_ADDR_INVALID(pstPkg))
                {
                    dos_dmem_free(pstHashNode);
                    pstHashNode = NULL;
                    bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: alloc memory FAIL!");
                    break;
                }
                bs_init_billing_package_st(pstPkg);

                pstPkg->ulPackageID = ulPkgID;
                pstPkg->ucServType = ulServType;

                pstPkg->astRule[0].ucBillingType = (U8)ulBillingType;
                pstPkg->astRule[0].ucDstAttrType1 = (U8)ulDstAttrType1;
                pstPkg->astRule[0].ucDstAttrType2 = (U8)ulDstAttrType2;
                pstPkg->astRule[0].ucFirstBillingCnt = (U8)ulFirstBillingCnt;
                pstPkg->astRule[0].ucNextBillingCnt = (U8)ulNextBillingCnt;
                pstPkg->astRule[0].ucPriority = 0;
                pstPkg->astRule[0].ucServType = (U8)ulServType;
                pstPkg->astRule[0].ucSrcAttrType1 = (U8)ulSrcAttrType1;
                pstPkg->astRule[0].ucSrcAttrType2 = (U8)ulSrcAttrType2;
                pstPkg->astRule[0].ucValid = 1;
                pstPkg->astRule[0].ulBillingRate = ulBillingRate;
                pstPkg->astRule[0].ulDstAttrValue1 = ulDstAttrValue1;
                pstPkg->astRule[0].ulDstAttrValue2 = ulDstAttrValue2;
                pstPkg->astRule[0].ulEffectTimestamp = ulEffectTime;
                pstPkg->astRule[0].ulExpireTimestamp = ulExpiryTime;
                pstPkg->astRule[0].ulFirstBillingUnit = ulFirstBillingUnit;
                pstPkg->astRule[0].ulNextBillingUnit = ulNextBillingUnit;
                pstPkg->astRule[0].ulPackageID = ulPkgID;
                pstPkg->astRule[0].ulSrcAttrValue1 = ulSrcAttrValue1;
                pstPkg->astRule[0].ulSrcAttrValue2 = ulSrcAttrValue2;

                pstHashNode->pHandle = (VOID *)pstPkg;
                ulHashIndex = bs_hash_get_index(BS_HASH_TBL_BILLING_PACKAGE_SIZE, ulPkgID);
                if (U32_BUTT == ulHashIndex)
                {
                    dos_dmem_free(pstPkg);
                    pstPkg = NULL;
                    pstHashNode->pHandle = NULL;

                    dos_dmem_free(pstHashNode);
                    pstHashNode = NULL;
                    break;
                }

                pthread_mutex_lock(&g_mutexBillingPackageTbl);
                hash_add_node(g_astBillingPackageTbl, pstHashNode, ulHashIndex, NULL);
                g_astBillingPackageTbl->NodeNum++;
                pthread_mutex_unlock(&g_mutexBillingPackageTbl);
            }
            break;
        }
        default:
            break;
    }
}

VOID bss_update_billing_rate(U32 ulOpteration, JSON_OBJ_ST *pstJSONObj)
{
    BS_BILLING_PACKAGE_ST *pstPkg = NULL;
    const S8 *pszBillingRate = NULL, *pszWhere = NULL, *pszCustomerID = NULL, *pszBillingPkgID = NULL, *pszBillingRuleID = NULL;
    U32 ulBillingRate = U32_BUTT, ulCustomerID = U32_BUTT, ulBillingPkgID = U32_BUTT, ulBillingRuleID = U32_BUTT;
    JSON_OBJ_ST *pstJsonWhere = NULL;
    HASH_NODE_S *pstHashNode = NULL;
    U32 ulHashIndex = U32_BUTT, ulLoop = U32_BUTT;
    BOOL bNodeFound = DOS_FALSE;
    
    switch(ulOpteration)
    {
        case BS_CMD_UPDATE:
            {   
                pszBillingRate = json_get_param(pstJSONObj, "billing_rate");
                if (DOS_ADDR_INVALID(pszBillingRate))
                {
                    bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: Get Billing Rate FAIL.");
                    DOS_ASSERT(0);
                    break;
                }

                pszWhere = json_get_param(pstJSONObj, "where");
                if (DOS_ADDR_INVALID(pszBillingRate))
                {
                    bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: Get Where Param FAIL.");
                    DOS_ASSERT(0);
                    break;
                }
                
                pstJsonWhere = json_init((char *)pszWhere);
                if (DOS_ADDR_INVALID(pstJsonWhere))
                {
                    bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: Init JSON obj FAIL.");
                    DOS_ASSERT(0);
                    break;
                }

                pszCustomerID = json_get_param(pstJsonWhere, "customer_id");
                if (DOS_ADDR_INVALID(pszCustomerID))
                {
                    bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: Get Customer ID FAIL.");
                    DOS_ASSERT(0);
                    break;
                }

                pszBillingPkgID = json_get_param(pstJsonWhere, "billing_package_id");
                if (DOS_ADDR_INVALID(pszBillingPkgID))
                {
                    bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: Get Billing Package ID FAIL.");
                    DOS_ASSERT(0);
                    break;
                }

                pszBillingRuleID = json_get_param(pstJsonWhere, "billing_rule_id");
                if (DOS_ADDR_INVALID(pszBillingRuleID))
                {
                    bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: Get Billing Rule ID FAIL.");
                    DOS_ASSERT(0);
                    break;
                }

                if (dos_atoul(pszBillingRate, &ulBillingRate) < 0
                    || dos_atoul(pszCustomerID, &ulCustomerID) < 0
                    || dos_atoul(pszBillingPkgID, &ulBillingPkgID) < 0
                    || dos_atoul(pszBillingRuleID, &ulBillingRuleID) < 0)
                {
                    bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: dos_atoul FAIL.");
                    DOS_ASSERT(0);
                    break;
                }

                ulHashIndex = bs_hash_get_index(BS_HASH_TBL_BILLING_PACKAGE_SIZE, ulBillingPkgID);
                if (U32_BUTT == ulHashIndex)
                {
                    bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: Get Billing Package ID %u FAIL.", ulBillingPkgID);
                    DOS_ASSERT(0);
                    break;
                }
                
                pstHashNode = hash_find_node(g_astBillingPackageTbl, ulHashIndex, (VOID *)&ulBillingPkgID, bs_billing_package_hash_node_match);
                if (DOS_ADDR_INVALID(pstHashNode)
                    || DOS_ADDR_INVALID(pstHashNode->pHandle))
                {
                    bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: Hash Node has no data.");
                    DOS_ASSERT(0);
                    break;
                }

                pstPkg = (BS_BILLING_PACKAGE_ST *)pstHashNode->pHandle;

                for (ulLoop = 0; ulLoop < BS_MAX_BILLING_RULE_IN_PACKAGE; ++ulLoop)
                {
                    if (pstPkg->astRule[ulLoop].ulRuleID == ulBillingRuleID)
                    {
                        pstPkg->astRule[ulLoop].ulBillingRate = ulBillingRate;
                        bNodeFound = DOS_TRUE;
                    }
                }

                if (DOS_TRUE == bNodeFound)
                {
                    bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Billing Rate Updated SUCC.");
                }
                else
                {
                    bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: No Billing Rule ID %u.", ulBillingRuleID);
                }
                break;
            }
        case BS_CMD_DELETE:
            break;
        case BS_CMD_INSERT:
            {
                pszBillingPkgID = json_get_param(pstJSONObj, "billing_package_id");
                if (DOS_ADDR_INVALID(pszBillingPkgID))
                {
                    bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: Get Billing Package ID FAIL.");
                    break;
                }

                if (dos_atoul(pszBillingPkgID, &ulBillingPkgID) < 0)
                {
                    bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: dos_atoul FAIL.");
                    break;
                }

                if (DOS_SUCC != bsd_walk_billing_package_tbl_bak(ulBillingPkgID))
                {
                    bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: Insert New Billing Package FAIL.");
                    break;
                }
            }
            break;
        default:
            break;
    }

    bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Billing Rate updated SUCC.");
}


VOID bss_update_call_task(U32 ulOpteration, JSON_OBJ_ST *pstJSONObj)
{
    BS_TASK_ST *pstTask = NULL;
    JSON_OBJ_ST *pstJsonWhere = NULL;
    HASH_NODE_S *pstHashNode = NULL;
    const S8   *pszTaskID = NULL, *pszWhere = NULL;
    U32 ulHashIndex = 0;
    U32 ulTaskID = U32_BUTT;

    switch (ulOpteration)
    {
        case BS_CMD_UPDATE:
        {
            pszWhere = json_get_param(pstJSONObj, "where");
            if (DOS_ADDR_INVALID(pszWhere))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Get where param fail.");
                break;
            }
            pstJsonWhere = json_init((S8 *)pszWhere);
            if (DOS_ADDR_INVALID(pstJsonWhere))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Init json fail.");
                break;
            }
            pszTaskID = json_get_param(pstJSONObj, "id");
            if (DOS_ADDR_INVALID(pszTaskID))
            {
                json_deinit(&pstJsonWhere);
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Get Task ID fail.");
                break;
            }
            if (dos_atoul(pszTaskID, &ulTaskID) < 0)
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "dos_atoul fail.");
                json_deinit(&pstJsonWhere);
                break;
            }

            ulHashIndex = bs_hash_get_index(BS_HASH_TBL_TASK_SIZE, ulTaskID);
            if (U32_BUTT == ulHashIndex)
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Task ID %u does not exist.", ulTaskID);
                json_deinit(&pstJsonWhere);
                break;
            }
            pstHashNode = hash_find_node(g_astTaskTbl, ulHashIndex, (VOID *)&ulTaskID, bs_task_hash_node_match);
            if (DOS_ADDR_INVALID(pstHashNode)
                || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Hash Node does not exist.");
                json_deinit(&pstJsonWhere);
                break;
            }
            pstTask = (BS_TASK_ST *)pstHashNode->pHandle;
            /*������ص����ݣ���ʱ����*/
            bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Tasks Update SUCC.");
            json_deinit(&pstJsonWhere);
            break;
        }
        case BS_CMD_DELETE:
        {
            pszWhere = json_get_param(pstJSONObj, "where");
            if (DOS_ADDR_INVALID(pszWhere))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Get where param fail.");
                break;
            }
            pstJsonWhere = json_init((S8 *)pszWhere);
            if (DOS_ADDR_INVALID(pstJsonWhere))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Init json fail.");
                break;
            }
            pszTaskID = json_get_param(pstJSONObj, "id");
            if (DOS_ADDR_INVALID(pszTaskID))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Get Task ID fail.");
                json_deinit(&pstJsonWhere);
                break;
            }
            if (dos_atoul(pszTaskID, &ulTaskID) < 0)
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "dos_atoul fail.");
                json_deinit(&pstJsonWhere);
                break;
            }

            ulHashIndex = bs_hash_get_index(BS_HASH_TBL_TASK_SIZE, ulTaskID);
            if (U32_BUTT == ulHashIndex)
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Task ID %u does not exist.", ulTaskID);
                json_deinit(&pstJsonWhere);
                break;
            }

            pstHashNode = hash_find_node(g_astTaskTbl, ulHashIndex, (VOID *)&ulTaskID, bs_task_hash_node_match);
            if (DOS_ADDR_INVALID(pstHashNode)
                || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Task ID %u does not exist.", ulTaskID);
                json_deinit(&pstJsonWhere);
                break;
            }
            pstTask = (BS_TASK_ST *)pstHashNode->pHandle;
            if (DOS_ADDR_INVALID(pstTask))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Task not found.");
                json_deinit(&pstJsonWhere);
                break;
            }
            /*������ȣ�˵��û���ҵ�*/
            if (pstTask->ulTaskID != ulTaskID)
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Task not found.");
                json_deinit(&pstJsonWhere);
                break;
            }
            hash_delete_node(g_astTaskTbl, pstHashNode, ulHashIndex);
            bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Delete Task ID %u SUCC.", ulTaskID);
            json_deinit(&pstJsonWhere);
            break;
        }
        case BS_CMD_INSERT:
        {
            pszTaskID = json_get_param(pstJSONObj, "id");
            if (DOS_ADDR_INVALID(pszTaskID))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Get id param fail.");
                break;
            }
            if (dos_atoul(pszTaskID, &ulTaskID) < 0)
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "dos_atoul fail.");
                break;
            }

            ulHashIndex = bs_hash_get_index(BS_HASH_TBL_TASK_SIZE, ulTaskID);
            if (U32_BUTT == ulHashIndex)
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Task ID %u does not exist.", ulTaskID);
                break;
            }

            pstHashNode = hash_find_node(g_astTaskTbl, ulHashIndex, (VOID *)&ulTaskID, bs_task_hash_node_match);
            if (DOS_ADDR_INVALID(pstHashNode))
            {
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Task ID %u does not exist, it will be added.", ulTaskID);
                pstHashNode = (HASH_NODE_S *)dos_dmem_alloc(sizeof(HASH_NODE_S));
                if (DOS_ADDR_INVALID(pstHashNode))
                {
                    bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Alloc mem fail.");
                    break;
                }
                pstTask = (BS_TASK_ST *)dos_dmem_alloc(sizeof(BS_TASK_ST));
                if (DOS_ADDR_INVALID(pstTask))
                {
                    bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Alloc mem fail.");
                    dos_dmem_free(pstHashNode);
                    pstHashNode = NULL;
                    break;
                }

                pstHashNode->pHandle = (VOID *)pstTask;
                HASH_Init_Node(pstHashNode);

                hash_add_node(g_astTaskTbl, pstHashNode, ulHashIndex, NULL);
                bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "Add Task SUCC.");
                break;
            }
            break;
        }
        default:
            break;
    }
}


VOID bss_data_update()
{
    const S8                *pszTblName  = NULL;
    const S8                *pszOpter    = NULL;
    BS_WEB_CMD_INFO_ST      *pszTblRow   = NULL;
    DLL_NODE_S              *pstListNode = NULL;
    JSON_OBJ_ST             *pstJsonNode = NULL;
    U32                     ulOpteration = 0;

    while (1)
    {
        pthread_mutex_lock(&g_mutexWebCMDTbl);
        pstListNode = dll_fetch(&g_stWebCMDTbl);
        pthread_mutex_unlock(&g_mutexWebCMDTbl);

        if (DOS_ADDR_INVALID(pstListNode))
        {
            break;
        }

        pszTblRow = pstListNode->pHandle;
        if (DOS_ADDR_INVALID(pszTblRow))
        {
            DOS_ASSERT(0);

            dos_dmem_free(pstListNode);
            pstListNode = NULL;
            continue;
        }

        pstJsonNode = pszTblRow->pstData;
        if (DOS_ADDR_INVALID(pstJsonNode))
        {
            dos_dmem_free(pszTblRow);
            pszTblRow = NULL;

            dos_dmem_free(pstListNode);
            pstListNode = NULL;
            continue;
        }


        pszTblName = json_get_param(pstJsonNode,"table");
        pszOpter = json_get_param(pstJsonNode,"dboperate");
        if (DOS_ADDR_INVALID(pszTblName) || DOS_ADDR_INVALID(pszOpter))
        {
            json_deinit(&pstJsonNode);
            pszTblRow->pstData = NULL;

            dos_dmem_free(pszTblRow);
            pszTblRow = NULL;
            pstListNode->pHandle = NULL;

            dos_dmem_free(pstListNode);
            pstListNode = NULL;

            continue;
        }

        if (dos_strcmp(pszOpter, "update") == 0)
        {
            ulOpteration = BS_CMD_UPDATE;
        }
        else if (dos_strcmp(pszOpter, "insert") == 0)
        {
            ulOpteration = BS_CMD_INSERT;
        }
        else if (dos_strcmp(pszOpter, "delete") == 0)
        {
            ulOpteration = BS_CMD_DELETE;
        }
        else
        {
            bs_trace(BS_TRACE_RUN, LOG_LEVEL_NOTIC, "Notice: It's a unknown opterator while deal with the WEB CMD.");

            json_deinit(&pszTblRow->pstData);
            pszTblRow->pstData = NULL;

            dos_dmem_free(pszTblRow);
            pszTblRow = NULL;
            pstListNode->pHandle = NULL;

            dos_dmem_free(pstListNode);
            pstListNode = NULL;

            continue;
        }

        if (dos_strcmp(pszTblName, "tbl_customer") == 0)
        {
            bss_update_customer(ulOpteration, pstJsonNode);
        }
        else if (dos_strcmp(pszTblName, "tbl_agent") == 0)
        {
            bss_update_agent(ulOpteration, pstJsonNode);
        }
        else if (dos_strcmp(pszTblName, "tbl_billing_package") == 0)
        {
            bss_update_billing_package(ulOpteration, pstJsonNode);
        }
        else if (dos_strcmp(pszTblName, "tbl_billing_rate") == 0)
        {
            bss_update_billing_rate(ulOpteration, pstJsonNode);
        }
        else if (dos_strcmp(pszTblName, "tbl_calltask") == 0)
        {
            bss_update_call_task(ulOpteration, pstJsonNode);
        }
        else
        {
            bs_trace(BS_TRACE_RUN, LOG_LEVEL_NOTIC, "Notice: It's a unknown table while deal with the WEB CMD.");
        }

        json_deinit(&pszTblRow->pstData);
        pszTblRow->pstData = NULL;

        dos_dmem_free(pszTblRow);
        pszTblRow = NULL;
        pstListNode->pHandle = NULL;

        dos_dmem_free(pstListNode);
        pstListNode = NULL;
    }
}

/* ����������� */
S32 bss_walk_tbl_rsp(DLL_NODE_S *pMsgNode)
{
    BS_INTER_MSG_WALK *pstMsg = pMsgNode->pHandle;

    S32     lRet = BS_INTER_ERR_FAIL;

    switch (pstMsg->ulTblType)
    {
        case BS_TBL_TYPE_CUSTOMER:
            bs_build_customer_tree();
            break;

        case BS_TBL_TYPE_AGENT:
            bs_stat_agent_num();
            /* ����,������Ϊ����������� */
            g_stBssCB.bIsMaintain = DOS_FALSE;
            break;

        case BS_TBL_TYPE_BILLING_PACKAGE:
        case BS_TBL_TYPE_SETTLE:
            /* ���账�� */
            break;
        case BS_TBL_TYPE_TMP_CMD:
            bss_data_update();
            break;
        default:
            bs_trace(BS_TRACE_RUN, LOG_LEVEL_NOTIC, "Notice: It's a unknown table");
            break;
    }

    /* ������Ϣ��������ȫ����,�ͷ���Ϣ�ڵ� */
    dos_dmem_free(pMsgNode->pHandle);
    pMsgNode->pHandle = NULL;
    dos_dmem_free(pMsgNode);

    return lRet;

}


/* ���ݲ���Ϣ������ */
VOID bss_dl_msg_proc(DLL_NODE_S *pMsgNode)
{
    BS_INTER_MSG_TAG    *pstMsgTag;

    if (DOS_ADDR_INVALID(pMsgNode))
    {
        DOS_ASSERT(0);
        return;
    }

    pstMsgTag = (BS_INTER_MSG_TAG *)pMsgNode->pHandle;
    if (DOS_ADDR_INVALID(pMsgNode))
    {
        /* ������Ϻ�,�ͷű��ڵ� */
        dos_dmem_free(pMsgNode);
        DOS_ASSERT(0);
        return;
    }

    bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG, "Recv msg from dl, type:%u, len:%u, errcode:%d",
             pstMsgTag->ucMsgType, pstMsgTag->usMsgLen, pstMsgTag->lInterErr);

    switch (pstMsgTag->ucMsgType)
    {
        case BS_INTER_MSG_WALK_RSP:
            bss_walk_tbl_rsp(pMsgNode);
            break;

        default:
            bs_trace(BS_TRACE_RUN, LOG_LEVEL_NOTIC, "Notice: Unknown msg type:%u", pstMsgTag->ucMsgType);
            /* δ֪��Ϣ,��������,�ͷ��ڴ� */
            dos_dmem_free(pMsgNode->pHandle);
            pMsgNode->pHandle = NULL;
            dos_dmem_free(pMsgNode);
            break;
    }

}

/* ������APP����Ϣ��ӵ���Ӧ���� */
VOID bss_add_aaa_list(VOID *pMsg)
{
    DLL_NODE_S          *pstMsgNode = NULL;
    BS_MSG_AUTH         *pstMsg = NULL;

    pstMsgNode = dos_dmem_alloc(sizeof(DLL_NODE_S));
    if (NULL == pstMsgNode)
    {
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
        return;
    }
    DLL_Init_Node(pstMsgNode);

    pstMsg = dos_dmem_alloc(sizeof(BS_MSG_AUTH));
    if (NULL == pstMsg)
    {
        dos_dmem_free(pstMsgNode);
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
        return;
    }

    /* ��Ϣ��� */
    *pstMsg = *(BS_MSG_AUTH *)pMsg;
    pstMsgNode->pHandle = (VOID *)pstMsg;
    pthread_mutex_lock(&g_mutexBSAAAMsg);
    DLL_Add(&g_stBSAAAMsgList, pstMsgNode);
    pthread_cond_signal(&g_condBSAAAList);
    pthread_mutex_unlock(&g_mutexBSAAAMsg);

}

/* ������APP����Ϣ��ӵ���Ӧ���� */
VOID bss_add_cdr_list(VOID *pMsg)
{
    DLL_NODE_S          *pstMsgNode = NULL;
    BS_MSG_CDR          *pstMsg = NULL;

    pstMsgNode = dos_dmem_alloc(sizeof(DLL_NODE_S));
    if (NULL == pstMsgNode)
    {
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
        return;
    }
    DLL_Init_Node(pstMsgNode);

    pstMsg = dos_dmem_alloc(sizeof(BS_MSG_CDR));
    if (NULL == pstMsg)
    {
        dos_dmem_free(pstMsgNode);
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
        return;
    }

    /* ��Ϣ��� */
    *pstMsg = *(BS_MSG_CDR *)pMsg;

    printf ("%s:%d, %d, %d\r\n", __FUNCTION__, __LINE__, pstMsg->ucLegNum, ((BS_MSG_CDR *)pMsg)->ucLegNum);

    pstMsgNode->pHandle = (VOID *)pstMsg;
    pthread_mutex_lock(&g_mutexBSCDR);
    DLL_Add(&g_stBSCDRList, pstMsgNode);
    pthread_cond_signal(&g_condBSCDRList);
    pthread_mutex_unlock(&g_mutexBSCDR);

}

/* Ӧ�ò���Ϣ������ */
VOID bss_app_msg_proc(VOID *pMsg)
{
    S8              szIPStr[128];
    BS_MSG_TAG      *pstMsgTag;

    if (DOS_ADDR_INVALID(pMsg))
    {
        DOS_ASSERT(0);
        return;
    }

    pstMsgTag = (BS_MSG_TAG *)pMsg;
    dos_ipaddrtostr(pstMsgTag->aulIPAddr[0], szIPStr, sizeof(szIPStr));
    bs_trace(BS_TRACE_FS, LOG_LEVEL_DEBUG,
             "Recv msg from app, addr:%s:%u, type:%u, len:%u, CR:%u, seq:%u, errcode:%u",
             szIPStr, dos_ntohs(pstMsgTag->usPort), pstMsgTag->ucMsgType, pstMsgTag->usMsgLen,
             pstMsgTag->ulCRNo, pstMsgTag->ulMsgSeq, pstMsgTag->ucErrcode);

    switch (pstMsgTag->ucMsgType)
    {
        case BS_MSG_HELLO_REQ:
            bss_send_rsp_msg2app(pstMsgTag, BS_MSG_HELLO_RSP);
            break;

        case BS_MSG_START_UP_NOTIFY:
            //TODO:�Ժ��ٿ��Ǵ���(���罫��FS�ϵ��м仰�����)
            break;

        case BS_MSG_BALANCE_QUERY_REQ:
        case BS_MSG_USER_AUTH_REQ:
        case BS_MSG_ACCOUNT_AUTH_REQ:
            /* �͵�aaa��Ϣ���н��д��� */
            bss_add_aaa_list(pMsg);
            break;

        case BS_MSG_BILLING_START_REQ:
            bss_send_rsp_msg2app(pstMsgTag, BS_MSG_BILLING_START_RSP);
            break;

        case BS_MSG_BILLING_UPDATE_REQ:
            bss_send_rsp_msg2app(pstMsgTag, BS_MSG_BILLING_UPDATE_RSP);
            break;

        case BS_MSG_BILLING_STOP_REQ:
            bss_add_cdr_list(pMsg);
            bss_send_rsp_msg2app(pstMsgTag, BS_MSG_BILLING_STOP_RSP);
            break;

        case BS_MSG_BILLING_RELEASE_ACK:
            //TODO
            break;

        default:
            bs_trace(BS_TRACE_RUN, LOG_LEVEL_NOTIC, "Notice: Unknown msg, type:%u", pstMsgTag->ucMsgType);
            break;
    }

}


/* �ڲ���Ϣ�����̴߳��� */
VOID *bss_recv_bsd_msg(VOID *arg)
{
    DLL_NODE_S *pNode;
    struct timespec stTimeout;

    while (1)
    {
        pNode = NULL;

        /* ��ȡ��Ϣ���е�һ������ */
        pthread_mutex_lock(&g_mutexBSD2SMsg);
        stTimeout.tv_sec = time(0) + 1;
        stTimeout.tv_nsec = 0;
        pthread_cond_timedwait(&g_condBSD2SList, &g_mutexBSD2SMsg, &stTimeout);
        while (1)
        {
            if (DLL_Count(&g_stBSD2SMsgList) <= 0)
            {
                break;
            }

            pNode = dll_fetch(&g_stBSD2SMsgList);
            if (NULL == pNode)
            {
                continue;
            }

            /* ������Ϣ���� */
            bss_dl_msg_proc(pNode);
        }

        pthread_mutex_unlock(&g_mutexBSD2SMsg);
    }
    return NULL;
}

/* ҵ��㷢����Ϣ��Ӧ�ò� */
VOID *bss_send_msg2app(VOID *arg)
{
    S32             lRet;
    DLL_NODE_S      *pNode;
    BSS_APP_CONN    *pstAppConn;
    BS_MSG_TAG      *pstMsgTag;
    struct timespec stTimeout;

    while (1)
    {
        pNode = NULL;

        /* ��ȡ��Ϣ���е�һ������ */
        pthread_mutex_lock(&g_mutexBSAppMsgSend);
        stTimeout.tv_sec = time(0) + 1;
        stTimeout.tv_nsec = 0;
        pthread_cond_timedwait(&g_condBSAppSendList, &g_mutexBSAppMsgSend, &stTimeout);

        while (1)
        {
            if (DLL_Count(&g_stBSAppMsgSendList) <= 0)
            {
                break;
            }

            pNode = dll_fetch(&g_stBSAppMsgSendList);
            if (NULL == pNode)
            {
                /* ������û����Ϣ */
                continue;
            }

            pstMsgTag = (BS_MSG_TAG *)pNode->pHandle;
            if (DOS_ADDR_INVALID(pstMsgTag))
            {
                DOS_ASSERT(0);
                dos_dmem_free(pNode);
                continue;
            }

            /* ��ȡsocket������Ϣ */
            pstAppConn = bs_get_app_conn(pstMsgTag);
            if (DOS_ADDR_VALID(pstAppConn))
            {
                /* ������Ϣ���к� */
                /* @TODO ֻ�����������к�ά�� */
#if 0
                pstAppConn->ulMsgSeq++;
                pstMsgTag->ulMsgSeq = pstAppConn->ulMsgSeq;
#endif
                /* ��BS��ַ��Ϣ���뵽��Ϣ */
                pstMsgTag->aulIPAddr[0] = 0;        /* IP��ַ��ʱͳһ��0,�Ժ��ж�̨BSʱ�ٸĽ� */
                pstMsgTag->aulIPAddr[1] = 0;
                pstMsgTag->aulIPAddr[2] = 0;
                pstMsgTag->aulIPAddr[3] = 0;
                pstMsgTag->usPort = g_stBssCB.usUDPListenPort;
                /* ϵͳ����ά��״̬,ͳһ������ */
                if (DOS_TRUE == g_stBssCB.bIsMaintain)
                {
                    pstMsgTag->ucErrcode = BS_ERR_MAINTAIN;
                }

                if (pstAppConn->bIsTCP)
                {
                    lRet = send(pstAppConn->lSockfd, pstMsgTag, pstMsgTag->usMsgLen, 0);
                }
                else
                {
                    lRet = sendto(pstAppConn->lSockfd, pstMsgTag, pstMsgTag->usMsgLen,
                                  0, (struct sockaddr *)&pstAppConn->stAddr, pstAppConn->lAddrLen);
                }

                if (lRet != (S32)pstMsgTag->usMsgLen)
                {
                    bs_trace(BS_TRACE_FS, LOG_LEVEL_NOTIC,
                             "Notice: send msg to app fail! result:%d, type:%u, len:%u, errno:%d",
                             lRet, pstMsgTag->ucMsgType, pstMsgTag->usMsgLen, errno);
                }
                else
                {
                    bs_trace(BS_TRACE_FS, LOG_LEVEL_DEBUG,
                             "Send msg to app succ! type:%u, seq:%u, len:%u, errcode:%u, addr:%X, port: %d",
                             pstMsgTag->ucMsgType, pstMsgTag->ulMsgSeq,
                             pstMsgTag->usMsgLen, pstMsgTag->ucErrcode,
                             pstAppConn->stAddr.sin_addr.s_addr, dos_htons(pstAppConn->stAddr.sin_port));
                }
            }
            else
            {
                bs_trace(BS_TRACE_FS, LOG_LEVEL_NOTIC,
                         "Notice: send msg fail! can't find conn info! addr:0x%X:%u",
                         pstMsgTag->aulIPAddr[0], pstMsgTag->usPort);
            }

            /* ������Ϻ�,�ͷű��ڵ� */
            dos_dmem_free(pNode->pHandle);
            dos_dmem_free(pNode);
        }
        pthread_mutex_unlock(&g_mutexBSAppMsgSend);
    }
    return NULL;
}

/* ҵ��㴦������web ����Ϣ */
VOID *bss_recv_msg_from_web(VOID *arg)
{
    S32                 lSocket, lAcceptedSocket, lRet, lAddrLen;
    S8                  strBuff[BS_MAX_MSG_LEN];
    BS_MSG_TAG          *pstMsgTag;
    struct sockaddr_un  stAddr, stAddrIn;
    struct timeval stTimeout={2, 0};
    #define PERM    "0666"
    mode_t lMode = strtol(PERM, 0, 8);

    #define SOCK_PATH   "/var/www/html/temp/bsconn.sock"


    /* ��ʼ��socket(UNIX STREAM��ʽ) */
    lSocket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (lSocket < 0)
    {
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: create socket fail!");
        return NULL;
    }

    /* ��ʼ�����󶨷�������ַ */
    dos_memzero(&stAddrIn, sizeof(stAddrIn));
    dos_memzero(&stAddr, sizeof(stAddr));
    lAddrLen = sizeof(struct sockaddr_un);
    stAddr.sun_family = AF_UNIX;
    strncpy(stAddr.sun_path, SOCK_PATH, sizeof(stAddr.sun_path) - 1);
    unlink(SOCK_PATH);
    lRet = bind(lSocket, (struct sockaddr *)&stAddr, lAddrLen);
    if (lRet < 0)
    {
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: bind socket fail!");
        close(lSocket);
        return NULL;
    }

    /* Ŀǰ����������ȷ��,��ʱ��SOMAXCONNΪ׼ */
    lRet = listen(lSocket, SOMAXCONN);
    if (lRet < 0)
    {
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: listen socket fail!");
        close(lSocket);
        return NULL;
    }

    /* �޸Ĳ���Ȩ�� */
    lRet = chmod(SOCK_PATH, lMode);
    if (lRet < 0)
    {
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: chmod fail!");
        close(lSocket);
        return NULL;
    }

    while (1)
    {
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG, "Watting for a connection from the web...");

        lAddrLen = sizeof(struct sockaddr_un);
        lAcceptedSocket = accept(lSocket, (struct sockaddr *)&stAddrIn, (socklen_t *)&lAddrLen);
        if (lAcceptedSocket < 0)
        {
            bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: accept socket fail!");
            break;
        }

        bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG, "Accept a web connection.");

        /* ���÷��ͽ��ճ�ʱ,��ֹ��ʱ������������Ӧ����ʱ */
        setsockopt(lAcceptedSocket, SOL_SOCKET, SO_SNDTIMEO, (char *)&stTimeout, sizeof(stTimeout));
        setsockopt(lAcceptedSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&stTimeout, sizeof(stTimeout));

        lRet = recv(lAcceptedSocket, strBuff, sizeof(strBuff), 0);
        if (0 == lRet || EINTR == errno || EAGAIN == errno)
        {
            /* ��������ʧ��,�˳����� */
            bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG, "Did not receive any data, close the connection.");
            close(lAcceptedSocket);
            continue;
        }
        else if (lRet < 0)
        {
            DOS_ASSERT(0);
            bs_trace(BS_TRACE_RUN, LOG_LEVEL_NOTIC, "Notice: recv data form socket fail! errno:%d", errno);
            close(lAcceptedSocket);
            break;
        }
        else if (lRet < sizeof(BS_MSG_TAG))
        {
            /* �յ��ı��ĳ����쳣,������Ϣ */
            bs_trace(BS_TRACE_RUN, LOG_LEVEL_INFO, "Info: recv wrong data form socket! len:%d", lRet);
            close(lAcceptedSocket);
            continue;
        }

        pstMsgTag = (BS_MSG_TAG *)strBuff;

        pthread_mutex_lock(&g_mutexTableUpdate);
        g_bTableUpdate = DOS_TRUE;
        pthread_mutex_unlock(&g_mutexTableUpdate);

        bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG, "Table requested operation: Success.");

        close(lAcceptedSocket);
    }

    close(lSocket);

    return NULL;
}

/* bss ����WEB�ͻ��˵�֪ͨ��Ϣ */
VOID *bss_web_msg_proc(VOID *arg)
{
    U32 ulTableUpdate;

    while (1)
    {
        pthread_mutex_lock(&g_mutexTableUpdate);
        ulTableUpdate = g_bTableUpdate;
        g_bTableUpdate= DOS_FALSE;
        pthread_mutex_unlock(&g_mutexTableUpdate);

        bs_trace(BS_TRACE_RUN, LOG_LEVEL_NOTIC, "Check update cmd. %X", ulTableUpdate);
        if (ulTableUpdate)
        {
            bs_trace(BS_TRACE_RUN, LOG_LEVEL_NOTIC, "Send table walk request to dl.");
            bss_send_walk_req2dl(BS_TBL_TYPE_TMP_CMD);
        }

        dos_task_delay(20 * 1000);
    }
    return NULL;
}


/* ҵ��㴦������Ӧ�ò����Ϣ */
VOID *bss_recv_msg_from_app(VOID *arg)
{
    S32                 lSocket, lMaxFd, lRet, lAddrLen;
    S8                  strBuff[BS_MAX_MSG_LEN];
    fd_set              stFDSet;
    BS_MSG_TAG          *pstMsgTag;
    struct sockaddr_in  stAddr, stAddrIn;


    /* ��ʼ��socket(��ʱֻ����UDP��ʽ) */
    lSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (lSocket < 0)
    {
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: create socket fail!");
        return NULL;
    }

    /* ��ʼ�����󶨷�������ַ */
    dos_memzero(&stAddrIn, sizeof(stAddrIn));
    dos_memzero(&stAddr, sizeof(stAddr));
    lAddrLen = sizeof(struct sockaddr_in);
    stAddr.sin_family = AF_INET;
    stAddr.sin_port = g_stBssCB.usUDPListenPort;
    stAddr.sin_addr.s_addr = dos_htonl(INADDR_ANY);
    lRet = bind(lSocket, (struct sockaddr *)&stAddr, lAddrLen);
    if (lRet < 0)
    {
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: bind socket fail!");
        close(lSocket);
        lSocket = -1;
        return NULL;
    }

    while (1)
    {
        /* ׼��select */
        FD_ZERO(&stFDSet);
        FD_SET(lSocket, &stFDSet);
        lMaxFd = lSocket + 1;

        lRet = select(lMaxFd, &stFDSet, NULL, NULL, NULL);
        if (0 == lRet || EINTR == errno)
        {
            /* socket�ޱ仯��ϵͳ�ж� */
            continue;
        }
        else if (lRet < 0)
        {
            DOS_ASSERT(0);
            bs_trace(BS_TRACE_RUN, LOG_LEVEL_NOTIC, "Notice: select fd fail! errno:%d", errno);
            break;
        }

        if (FD_ISSET(lSocket, &stFDSet) <= 0)
        {
            continue;
        }

        lRet = recvfrom(lSocket, strBuff, sizeof(strBuff), 0,  (struct sockaddr *)&stAddrIn , (socklen_t *)&lAddrLen);
        if (0 == lRet || EINTR == errno || EAGAIN == errno)
        {
            /* ���ӹر�/ϵͳ�жϻ�����һ��(ͨ���Ƿ���������µ�������������) */
            continue;
        }
        else if (lRet < 0)
        {
            DOS_ASSERT(0);
            bs_trace(BS_TRACE_RUN, LOG_LEVEL_NOTIC, "Notice: recv data form socket fail! errno:%d", errno);
            break;
        }
        else if (lRet < sizeof(BS_MSG_TAG))
        {
            /* �յ���UDP���ĳ����쳣,������Ϣ */
            bs_trace(BS_TRACE_RUN, LOG_LEVEL_INFO, "Info: recv wrong data form socket! len:%d", lRet);
            continue;
        }

#if (DOS_TRUE == BS_ONLY_LISTEN_LOCAL)
        if (stAddrIn.sin_addr.s_addr != BS_LOCAL_IP_ADDR)
        {
            bs_trace(BS_TRACE_RUN, LOG_LEVEL_DEBUG, "Only accept local(127.0.0.1) connect!");
            continue;
        }
#endif

        /* ��socket��ַ��Ϣ���µ���Ϣ�� */
        pstMsgTag = (BS_MSG_TAG *)strBuff;
        if (pstMsgTag->usPort != stAddrIn.sin_port
            || pstMsgTag->aulIPAddr[0] != stAddrIn.sin_addr.s_addr)
        {
            bs_trace(BS_TRACE_FS, LOG_LEVEL_DEBUG,
                     "Refresh app addr. old:0x%X:%u, new:0x%X:%u",
                     dos_ntohl(pstMsgTag->aulIPAddr[0]), dos_ntohs(pstMsgTag->usPort),
                     dos_ntohl(stAddrIn.sin_addr.s_addr), dos_ntohs(stAddrIn.sin_port));
            pstMsgTag->usPort = stAddrIn.sin_port;
            pstMsgTag->aulIPAddr[0] = stAddrIn.sin_addr.s_addr;
            pstMsgTag->aulIPAddr[1] = 0;
            pstMsgTag->aulIPAddr[2] = 0;
            pstMsgTag->aulIPAddr[3] = 0;

        }

        bs_save_app_conn(lSocket, &stAddrIn, lAddrLen, DOS_FALSE);

        bss_app_msg_proc((VOID *)strBuff);

    }

    return NULL;
}

/* ��ѯ��� */
VOID bss_query_balance(DLL_NODE_S *pMsgNode)
{
    U8              ucErrCode = BS_ERR_SUCC;
    BS_CUSTOMER_ST  *pstCustomer = NULL;
    BS_MSG_AUTH     *pstMsg = NULL;

    /* ǰ���Ѿ��жϹ���ַ�Ϸ���,�˴�ֱ��ʹ�ü��� */
    pstMsg = pMsgNode->pHandle;
    bs_trace(BS_TRACE_FS, LOG_LEVEL_DEBUG,
             "Query balance, customer:%u, account:%u, userid:%u, agentid:%u",
             pstMsg->ulCustomerID, pstMsg->ulAccountID,
             pstMsg->ulUserID, pstMsg->ulAgentID);

    if (pstMsg->ulCustomerID != 0)
    {
        /* Ŀǰϵͳ���,FS��BS�������ݿ�,һ�����пͻ���Ϣ�͹���;Ŀǰһ���ͻ�һ���˻� */
        DOS_ASSERT(0);
        bs_trace(BS_TRACE_FS, LOG_LEVEL_ERROR, "Err: no customer info in balance query msg!");
        ucErrCode = BS_ERR_PARAM_ERR;
        goto query_fail;
    }

    pstCustomer = bs_get_customer_st(pstMsg->ulCustomerID);
    if (NULL == pstCustomer)
    {
        bs_trace(BS_TRACE_FS, LOG_LEVEL_DEBUG, "Can't find customer!");
        ucErrCode = BS_ERR_NOT_EXIST;
        goto query_fail;
    }

    /* ����ԭ��Ϣ�洢��Ϣ */
    pstMsg->ulAccountID = pstCustomer->stAccount.ulAccountID;
    pstMsg->lBalance = (U32)pstCustomer->stAccount.LBalanceActive;
    pstMsg->ucBalanceWarning = (pstMsg->lBalance < pstCustomer->stAccount.lBalanceWarning)?DOS_TRUE:DOS_FALSE;
    ucErrCode = BS_ERR_SUCC;

    bs_trace(BS_TRACE_FS, LOG_LEVEL_DEBUG,
             "Query succ, customer(%u:%s) balanse:%d, warning:%d!",
             pstMsg->ulCustomerID, pstCustomer->szCustomerName,
             pstMsg->lBalance, pstMsg->ucBalanceWarning);

query_fail:
    pstMsg->stMsgTag.ucMsgType = BS_MSG_BALANCE_QUERY_RSP;
    pstMsg->stMsgTag.ucErrcode = ucErrCode;
    bss_send_aaa_rsp2app(pMsgNode);
    return;
}

/*
�û���֤:���ݿͻ�ID��ҵ�����Ͳ��Ҷ�Ӧ���˻�,�ж��Ƿ���֤ͨ��
˵��:
1.Ŀǰһ���ͻ�ֻ��һ���˻�;
2.Ŀǰδ���˻���ͨ��ҵ��������,Ĭ���˻���ͨ����ҵ��;
3.�ж��˻�ʵ������Ƿ����Կ�չҵ��,���������ҵ��ʹ��ʱ��/����;
*/
VOID bss_user_auth(DLL_NODE_S *pMsgNode)
{
    U8              ucErrCode = BS_ERR_SUCC;
    S8              szServType[128];
    U32             i, ulServNum, ulStrLen, ulMinSession;
    U32             aulMaxSession[BS_MAX_SERVICE_TYPE_IN_SESSION];
    BS_CUSTOMER_ST  *pstCustomer = NULL;
    BS_MSG_AUTH     *pstMsg = NULL;
    BS_BILLING_PACKAGE_ST *pstPackage = NULL;

    /* ǰ���Ѿ��жϹ���ַ�Ϸ���,�˴�ֱ��ʹ�ü��� */
    pstMsg = pMsgNode->pHandle;
    szServType[0] = '\0';
    ulServNum = 0;
    ulStrLen = 0;
    for (i = 0; i < BS_MAX_SERVICE_TYPE_IN_SESSION; i++)
    {
        if (0 == pstMsg->aucServType[i]
            || U8_BUTT == pstMsg->aucServType[i]
            || BS_SERV_BUTT == pstMsg->aucServType[i])
        {
            break;
        }

        ulServNum++;
        ulStrLen = dos_strlen(szServType);
        dos_snprintf(szServType + ulStrLen,
                     sizeof(szServType) - ulStrLen,
                     "serv%u:%u, ", i, pstMsg->aucServType[i]);
        szServType[sizeof(szServType) - 1] = '\0';

    }

    /* ������Ϣ,�ַ������ͱ����Դ����� */
    pstMsg->szPwd[sizeof(pstMsg->szPwd)-1] = '\0';
    pstMsg->szSessionID[sizeof(pstMsg->szSessionID)-1] = '\0';
    pstMsg->szCaller[sizeof(pstMsg->szCaller)-1] = '\0';
    pstMsg->szCallee[sizeof(pstMsg->szCallee)-1] = '\0';
    pstMsg->szCID[sizeof(pstMsg->szCID)-1] = '\0';
    pstMsg->szAgentNum[sizeof(pstMsg->szAgentNum)-1] = '\0';

    bs_trace(BS_TRACE_FS, LOG_LEVEL_DEBUG,
             "User auth, customer:%u, account:%u, userid:%u, agentid:%u, "
             "%s, session id:%s, session num:%d, agent:%s, caller:%s, callee:%s, timestamp: %u",
             pstMsg->ulCustomerID, pstMsg->ulAccountID,
             pstMsg->ulUserID, pstMsg->ulAgentID,
             szServType, pstMsg->szSessionID,
             pstMsg->ulSessionNum, pstMsg->szAgentNum,
             pstMsg->szCaller,pstMsg->szCallee, pstMsg->ulTimeStamp);

    if (0 == ulServNum)
    {
        DOS_ASSERT(0);
        bs_trace(BS_TRACE_FS, LOG_LEVEL_ERROR, "%s", "Err: no server type in auth msg!");
        ucErrCode = BS_ERR_PARAM_ERR;
        goto auth_fail;
    }

    if (0 == pstMsg->ulCustomerID)
    {
        /* Ŀǰϵͳ���,FS��BS�������ݿ�,һ�����пͻ���Ϣ�͹���;Ŀǰһ���ͻ�һ���˻� */
        DOS_ASSERT(0);
        bs_trace(BS_TRACE_FS, LOG_LEVEL_ERROR, "%s", "Err: no customer info in auth msg!");
        ucErrCode = BS_ERR_PARAM_ERR;
        goto auth_fail;
    }

    pstCustomer = bs_get_customer_st(pstMsg->ulCustomerID);
    if (NULL == pstCustomer)
    {
        bs_trace(BS_TRACE_FS, LOG_LEVEL_DEBUG, "%s", "Can't find customer!");
        ucErrCode = BS_ERR_NOT_EXIST;
        goto auth_fail;
    }

    if (pstCustomer->ucCustomerType != BS_CUSTOMER_TYPE_CONSUMER)
    {
        /* ֻ�������߿ͻ��Ż������֤ */
        bs_trace(BS_TRACE_FS, LOG_LEVEL_DEBUG,
                 "It's only consumer can user auth! customer:%u, type:%u",
                 pstCustomer->ulCustomerID,
                 pstCustomer->ucCustomerType);
        ucErrCode = BS_ERR_NOT_MATCH;
        goto auth_fail;
    }

    /* ����ԭ��Ϣ�洢��Ϣ */
    pstMsg->ulAccountID = pstCustomer->stAccount.ulAccountID;
    pstMsg->lBalance = (U32)pstCustomer->stAccount.LBalanceActive;
    pstMsg->ucBalanceWarning = (pstMsg->lBalance < pstCustomer->stAccount.lBalanceWarning)?DOS_TRUE:DOS_FALSE;

    if (pstCustomer->stAccount.LBalanceActive < (S64)pstCustomer->stAccount.lCreditLine)
    {
        /* ʵ������������ֵ,��֤ʧ�� */
        bs_trace(BS_TRACE_FS, LOG_LEVEL_DEBUG,
                 "Balance is less than credit line! balance:%d, line:%d",
                 pstCustomer->stAccount.LBalanceActive,
                 pstCustomer->stAccount.lCreditLine);
        ucErrCode = BS_ERR_LACK_FEE;
        goto auth_fail;
    }

    /* ��ȡ�Ʒѹ���,��ҵ�����Ԥ����,��ȡmax session��Ϣ */
    for (i = 0; i < ulServNum; i++)
    {
        aulMaxSession[i] = 0;
        ulMinSession = U32_BUTT;
        pstPackage = NULL;

        bs_trace(BS_TRACE_FS, LOG_LEVEL_DEBUG
                , "Get billing package %d for service %d"
                , pstCustomer->stAccount.ulBillingPackageID
                , pstMsg->aucServType[i]);

        pstPackage = bs_get_billing_package(pstCustomer->stAccount.ulBillingPackageID, pstMsg->aucServType[i]);
        if (NULL == pstPackage)
        {
            /* ���Ҳ����Ʒѹ���,����ʧ�� */
            ucErrCode = BS_ERR_RESTRICT;
            goto auth_fail;
        }

        aulMaxSession[i] = bs_pre_billing(pstCustomer, pstMsg, pstPackage);
        if (U32_BUTT == aulMaxSession[i])
        {
            ucErrCode = BS_ERR_RESTRICT;
            goto auth_fail;
        }

        if (aulMaxSession[i] < ulMinSession)
        {
            ulMinSession = aulMaxSession[i];
        }
    }

    if (ulMinSession != U32_BUTT)
    {
        /* �ֱ�һ��,�����ж��ҵ���session,���sessionֵ���ڵ���ҵ����Сֵ����ҵ������ */
        pstMsg->ulMaxSession = ulMinSession/ulServNum;
    }
    else
    {
        pstMsg->ulMaxSession = 0;
    }

    ucErrCode = BS_ERR_SUCC;
    bs_trace(BS_TRACE_FS, LOG_LEVEL_DEBUG,
             "Auth succ, customer(%u:%s) balanse:%d, warning:%d, max session:%u!",
             pstMsg->ulCustomerID, pstCustomer->szCustomerName,
             pstMsg->lBalance, pstMsg->ucBalanceWarning, pstMsg->ulMaxSession);

auth_fail:
    pstMsg->stMsgTag.ucMsgType = BS_MSG_USER_AUTH_RSP;
    pstMsg->stMsgTag.ucErrcode = ucErrCode;
    bss_send_aaa_rsp2app(pMsgNode);
    return;
}

/* �˻���֤ */
VOID bss_account_auth(DLL_NODE_S *pMsgNode)
{

    //TODO:��ʱδ��չ�˻���֤��ҵ��(����Ԥ���ѿ���)
}

/* ����¼������ */
VOID bss_generate_record_cdr(BS_BILL_SESSION_LEG *pstSessionLeg)
{
    DLL_NODE_S          *pstMsgNode = NULL;
    BS_CDR_RECORDING_ST *pstCDR = NULL;

    pstMsgNode = dos_dmem_alloc(sizeof(DLL_NODE_S));
    if (NULL == pstMsgNode)
    {
        bs_trace(BS_TRACE_CDR, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
        return;
    }
    DLL_Init_Node(pstMsgNode);

    pstCDR = dos_dmem_alloc(sizeof(BS_CDR_RECORDING_ST));
    if (NULL == pstCDR)
    {
        dos_dmem_free(pstMsgNode);
        bs_trace(BS_TRACE_CDR, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
        return;
    }

    /* ���ɻ�����Ϣ */
    dos_memzero(pstCDR, sizeof(BS_CDR_RECORDING_ST));
    pstCDR->stCDRTag.ulCDRMark = pstSessionLeg->ulCDRMark;
    pstCDR->stCDRTag.ucCDRType = BS_CDR_RECORDING;

    pstCDR->ulUserID = pstSessionLeg->ulUserID;
    pstCDR->ulAgentID = pstSessionLeg->ulAgentID;
    pstCDR->ulCustomerID = pstSessionLeg->ulCustomerID;
    pstCDR->ulAccountID = pstSessionLeg->ulAccountID;
    pstCDR->ulTaskID = pstSessionLeg->ulTaskID;

    dos_strncpy(pstCDR->szCaller, pstSessionLeg->szCaller, sizeof(pstCDR->szCaller));
    dos_strncpy(pstCDR->szCallee, pstSessionLeg->szCallee, sizeof(pstCDR->szCallee));
    dos_strncpy(pstCDR->szCID, pstSessionLeg->szCID, sizeof(pstCDR->szCID));
    dos_strncpy(pstCDR->szAgentNum, pstSessionLeg->szAgentNum, sizeof(pstCDR->szAgentNum));
    dos_strncpy(pstCDR->szRecordFile, pstSessionLeg->szRecordFile, sizeof(pstCDR->szRecordFile));

    pstCDR->ulRecordTimeStamp = pstSessionLeg->ulAnswerTimeStamp;
    pstCDR->ulTimeLen = pstSessionLeg->ulByeTimeStamp - pstSessionLeg->ulAnswerTimeStamp;
    if (pstCDR->ulRecordTimeStamp != 0 && 0 == pstCDR->ulTimeLen)
    {
        /* ʱ��̫��,�������� */
        pstCDR->ulTimeLen = 1;
    }

    pstMsgNode->pHandle = (VOID *)pstCDR;

    bs_trace(BS_TRACE_CDR, LOG_LEVEL_DEBUG,
             "Generate recording cdr, "
             "mark:%u, type:%u, customer:%u, account:%u, "
             "userid:%u, agentid:%u, taskid:%u, record:%s, "
             "caller:%s, callee:%s, cid:%s, agent:%s, "
             "record time:%u, time len:%u",
             pstCDR->stCDRTag.ulCDRMark, pstCDR->stCDRTag.ucCDRType,
             pstCDR->ulCustomerID, pstCDR->ulAccountID,
             pstCDR->ulUserID, pstCDR->ulAgentID,
             pstCDR->ulTaskID, pstCDR->szRecordFile,
             pstCDR->szCaller, pstCDR->szCallee,
             pstCDR->szCID, pstCDR->szAgentNum,
             pstCDR->ulRecordTimeStamp, pstCDR->ulTimeLen);

    /* ��Ϣ��� */
    pthread_mutex_lock(&g_mutexBSBilling);
    DLL_Add(&g_stBSBillingList, pstMsgNode);
    pthread_cond_signal(&g_condBSBillingList);
    pthread_mutex_unlock(&g_mutexBSBilling);

}

/* ���ɽ��㻰�� */
VOID bss_generate_settle_cdr(BS_BILL_SESSION_LEG *pstSessionLeg)
{
    U8                  ucServType;
    U32                 i;
    DLL_NODE_S          *pstMsgNode = NULL;
    BS_CDR_SETTLE_ST    *pstCDR = NULL;

    /* Ŀǰֻ������������/��Ϣ���������ɽ��㻰��;������ݲ����� */
    ucServType = BS_SERV_BUTT;
    for (i = 0; i < BS_MAX_SERVICE_TYPE_IN_SESSION; i++)
    {
        if (BS_SERV_OUTBAND_CALL == pstSessionLeg->aucServType[i]
            || BS_SERV_AUTO_DIALING == pstSessionLeg->aucServType[i]
            || BS_SERV_PREVIEW_DIALING == pstSessionLeg->aucServType[i]
            || BS_SERV_PREDICTIVE_DIALING == pstSessionLeg->aucServType[i]
            || BS_SERV_VERIFY == pstSessionLeg->aucServType[i]
            || BS_SERV_SMS_SEND == pstSessionLeg->aucServType[i]
            || BS_SERV_MMS_SEND == pstSessionLeg->aucServType[i])
        {
            ucServType = pstSessionLeg->aucServType[i];
            break;
        }
    }

    if (BS_SERV_BUTT == ucServType)
    {
        return;
    }

    pstMsgNode = dos_dmem_alloc(sizeof(DLL_NODE_S));
    if (NULL == pstMsgNode)
    {
        bs_trace(BS_TRACE_CDR, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
        return;
    }
    DLL_Init_Node(pstMsgNode);

    pstCDR = dos_dmem_alloc(sizeof(BS_CDR_SETTLE_ST));
    if (NULL == pstCDR)
    {
        dos_dmem_free(pstMsgNode);
        bs_trace(BS_TRACE_CDR, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
        return;
    }

    /* ���ɻ�����Ϣ */
    dos_memzero(pstCDR, sizeof(BS_CDR_SETTLE_ST));
    pstCDR->stCDRTag.ulCDRMark = pstSessionLeg->ulCDRMark;
    pstCDR->stCDRTag.ucCDRType = BS_CDR_SETTLE;
    dos_strncpy(pstCDR->szCaller, pstSessionLeg->szCaller, sizeof(pstCDR->szCaller));
    dos_strncpy(pstCDR->szCallee, pstSessionLeg->szCallee, sizeof(pstCDR->szCallee));
    pstCDR->ulTimeStamp = pstSessionLeg->ulAnswerTimeStamp;
    pstCDR->ulLen = pstSessionLeg->ulByeTimeStamp - pstSessionLeg->ulAnswerTimeStamp;
    pstCDR->aulPeerIP[0] = pstSessionLeg->aulPeerIP[0];
    pstCDR->aulPeerIP[1] = pstSessionLeg->aulPeerIP[1];
    pstCDR->aulPeerIP[2] = pstSessionLeg->aulPeerIP[2];
    pstCDR->aulPeerIP[3] = pstSessionLeg->aulPeerIP[3];
    pstCDR->usPeerTrunkID = pstSessionLeg->usPeerTrunkID;
    pstCDR->usTerminateCause = pstSessionLeg->usTerminateCause;
    pstCDR->ucServType = ucServType;

    pstMsgNode->pHandle = (VOID *)pstCDR;

    if (pstCDR->ulTimeStamp != 0 && 0 == pstCDR->ulLen)
    {
        /* ʱ��̫��,�������� */
        pstCDR->ulLen = 1;
    }

    bs_trace(BS_TRACE_CDR, LOG_LEVEL_DEBUG,
             "Generate settle cdr, "
             "mark:%u, type:%u, caller:%s, callee:%s, "
             "trunk id:%u, trunk ip:0x%X, servtype:%u, cause:%u, "
             "time:%u, len:%u",
             pstCDR->stCDRTag.ulCDRMark, pstCDR->stCDRTag.ucCDRType,
             pstCDR->szCaller, pstCDR->szCallee,
             pstCDR->usPeerTrunkID, pstCDR->aulPeerIP[0],
             pstCDR->ucServType, pstCDR->usTerminateCause,
             pstCDR->ulTimeStamp, pstCDR->ulLen);

    /* ��Ϣ��� */
    pthread_mutex_lock(&g_mutexBSBilling);
    DLL_Add(&g_stBSBillingList, pstMsgNode);
    pthread_cond_signal(&g_condBSBillingList);
    pthread_mutex_unlock(&g_mutexBSBilling);

}

/* �����������ͻ��� */
VOID bss_generate_voice_cdr(BS_BILL_SESSION_LEG *pstSessionLeg, U8 ucServType)
{
    DLL_NODE_S          *pstMsgNode = NULL;
    BS_CDR_VOICE_ST     *pstCDR = NULL;

    pstMsgNode = dos_dmem_alloc(sizeof(DLL_NODE_S));
    if (NULL == pstMsgNode)
    {
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
        return;
    }
    DLL_Init_Node(pstMsgNode);

    pstCDR = dos_dmem_alloc(sizeof(BS_CDR_VOICE_ST));
    if (NULL == pstCDR)
    {
        dos_dmem_free(pstMsgNode);
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
        return;
    }

    /* ���ɻ�����Ϣ */
    dos_memzero(pstCDR, sizeof(BS_CDR_VOICE_ST));
    pstCDR->stCDRTag.ulCDRMark = pstSessionLeg->ulCDRMark;
    pstCDR->stCDRTag.ucCDRType = BS_CDR_VOICE;
    pstCDR->ulUserID = pstSessionLeg->ulUserID;
    pstCDR->ulAgentID = pstSessionLeg->ulAgentID;
    pstCDR->ulCustomerID = pstSessionLeg->ulCustomerID;
    pstCDR->ulAccountID = pstSessionLeg->ulAccountID;
    pstCDR->ulTaskID = pstSessionLeg->ulTaskID;
    dos_strncpy(pstCDR->szCaller, pstSessionLeg->szCaller, sizeof(pstCDR->szCaller));
    pstCDR->szCaller[sizeof(pstCDR->szCaller) - 1] = '\0';
    dos_strncpy(pstCDR->szCallee, pstSessionLeg->szCallee, sizeof(pstCDR->szCallee));
    pstCDR->szCallee[sizeof(pstCDR->szCallee) - 1] = '\0';
    dos_strncpy(pstCDR->szCID, pstSessionLeg->szCID, sizeof(pstCDR->szCID));
    pstCDR->szCID[sizeof(pstCDR->szCID) - 1] = '\0';
    dos_strncpy(pstCDR->szAgentNum, pstSessionLeg->szAgentNum, sizeof(pstCDR->szAgentNum));
    pstCDR->szAgentNum[sizeof(pstCDR->szAgentNum) - 1] = '\0';
    dos_strncpy(pstCDR->szRecordFile, pstSessionLeg->szRecordFile, sizeof(pstCDR->szRecordFile));
    pstCDR->szRecordFile[sizeof(pstCDR->szRecordFile) - 1] = '\0';
    pstCDR->ulPDDLen = pstSessionLeg->ulRingTimeStamp - pstSessionLeg->ulStartTimeStamp;
    pstCDR->ulRingTime = pstSessionLeg->ulRingTimeStamp;
    pstCDR->ulAnswerTimeStamp = pstSessionLeg->ulAnswerTimeStamp;
    pstCDR->ulDTMFTime = pstSessionLeg->ulDTMFTimeStamp;
    pstCDR->ulIVRFinishTime = pstSessionLeg->ulIVRFinishTimeStamp;
    pstCDR->ulWaitAgentTime = pstSessionLeg->ulBridgeTimeStamp - pstSessionLeg->ulAnswerTimeStamp;
    pstCDR->ulTimeLen = pstSessionLeg->ulByeTimeStamp - pstSessionLeg->ulAnswerTimeStamp;
    pstCDR->ulHoldCnt = pstSessionLeg->ulHoldCnt;
    pstCDR->ulHoldTimeLen = pstSessionLeg->ulHoldTimeLen;
    pstCDR->aulPeerIP[0] = pstSessionLeg->aulPeerIP[0];
    pstCDR->aulPeerIP[1] = pstSessionLeg->aulPeerIP[1];
    pstCDR->aulPeerIP[2] = pstSessionLeg->aulPeerIP[2];
    pstCDR->aulPeerIP[3] = pstSessionLeg->aulPeerIP[3];
    pstCDR->usPeerTrunkID = pstSessionLeg->usPeerTrunkID;
    pstCDR->usTerminateCause = pstSessionLeg->usTerminateCause;
    pstCDR->ucServType = ucServType;
    pstCDR->ucRecordFlag = ('\0'== pstSessionLeg->szRecordFile[0])?DOS_FALSE:DOS_TRUE;
    pstCDR->ucReleasePart = pstSessionLeg->ucReleasePart;
    pstCDR->ucPayloadType = pstSessionLeg->ucPayloadType;
    pstCDR->ucPacketLossRate = pstSessionLeg->ucPacketLossRate;

    pstMsgNode->pHandle = (VOID *)pstCDR;

    if (pstCDR->ulAnswerTimeStamp != 0 && 0 == pstCDR->ulTimeLen)
    {
        /* ʱ��̫��,�������� */
        pstCDR->ulTimeLen = 1;
    }


    bs_trace(BS_TRACE_CDR, LOG_LEVEL_DEBUG,
             "Generate voice cdr, "
             "mark:%u, type:%u, customer:%u, account:%u, "
             "userid:%u, agentid:%u, taskid:%u, record:%s, "
             "caller:%s, callee:%s, cid:%s, agent:%s, "
             "start time:%u, time len:%u, servtype:%u, cause:%u",
             pstCDR->stCDRTag.ulCDRMark, pstCDR->stCDRTag.ucCDRType,
             pstCDR->ulCustomerID, pstCDR->ulAccountID,
             pstCDR->ulUserID, pstCDR->ulAgentID,
             pstCDR->ulTaskID, pstCDR->szRecordFile,
             pstCDR->szCaller, pstCDR->szCallee,
             pstCDR->szCID, pstCDR->szAgentNum,
             pstCDR->ulAnswerTimeStamp, pstCDR->ulTimeLen,
             pstCDR->ucServType, pstCDR->usTerminateCause);

    /* ��Ϣ��� */
    pthread_mutex_lock(&g_mutexBSBilling);
    DLL_Add(&g_stBSBillingList, pstMsgNode);
    pthread_cond_signal(&g_condBSBillingList);
    pthread_mutex_unlock(&g_mutexBSBilling);
    return;

}

/* ������Ϣ���� */
VOID bss_generate_message_cdr(BS_BILL_SESSION_LEG *pstSessionLeg, U8 ucServType)
{
    DLL_NODE_S          *pstMsgNode = NULL;
    BS_CDR_MS_ST        *pstCDR = NULL;

    pstMsgNode = dos_dmem_alloc(sizeof(DLL_NODE_S));
    if (NULL == pstMsgNode)
    {
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
        return;
    }
    DLL_Init_Node(pstMsgNode);

    pstCDR = dos_dmem_alloc(sizeof(BS_CDR_MS_ST));
    if (NULL == pstCDR)
    {
        dos_dmem_free(pstMsgNode);
        bs_trace(BS_TRACE_RUN, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
        return;
    }

    /* ���ɻ�����Ϣ */
    dos_memzero(pstCDR, sizeof(BS_CDR_VOICE_ST));
    pstCDR->stCDRTag.ulCDRMark = pstSessionLeg->ulCDRMark;
    pstCDR->stCDRTag.ucCDRType = BS_CDR_VOICE;
    pstCDR->ulUserID = pstSessionLeg->ulUserID;
    pstCDR->ulAgentID = pstSessionLeg->ulAgentID;
    pstCDR->ulCustomerID = pstSessionLeg->ulCustomerID;
    pstCDR->ulAccountID = pstSessionLeg->ulAccountID;
    pstCDR->ulTaskID = pstSessionLeg->ulTaskID;
    dos_strncpy(pstCDR->szCaller, pstSessionLeg->szCaller, sizeof(pstCDR->szCaller));
    dos_strncpy(pstCDR->szCallee, pstSessionLeg->szCallee, sizeof(pstCDR->szCallee));
    dos_strncpy(pstCDR->szAgentNum, pstSessionLeg->szAgentNum, sizeof(pstCDR->szAgentNum));
    pstCDR->ulTimeStamp = pstSessionLeg->ulStartTimeStamp;
    pstCDR->ulArrivedTimeStamp = pstSessionLeg->ulAnswerTimeStamp;
    pstCDR->ulLen = pstSessionLeg->ulByeTimeStamp - pstSessionLeg->ulAnswerTimeStamp;
    pstCDR->aulPeerIP[0] = pstSessionLeg->aulPeerIP[0];
    pstCDR->aulPeerIP[1] = pstSessionLeg->aulPeerIP[1];
    pstCDR->aulPeerIP[2] = pstSessionLeg->aulPeerIP[2];
    pstCDR->aulPeerIP[3] = pstSessionLeg->aulPeerIP[3];
    pstCDR->usPeerTrunkID = pstSessionLeg->usPeerTrunkID;
    pstCDR->usTerminateCause = pstSessionLeg->usTerminateCause;
    pstCDR->ucServType = ucServType;

    pstMsgNode->pHandle = (VOID *)pstCDR;

    bs_trace(BS_TRACE_CDR, LOG_LEVEL_DEBUG,
             "Generate message cdr, "
             "mark:%u, type:%u, customer:%u, account:%u, "
             "userid:%u, agentid:%u, taskid:%u, len:%u, "
             "caller:%s, callee:%s, agent:%s, peerip:0x%X, "
             "time:%u, arrived:%u, servtype:%u, cause:%u",
             pstCDR->stCDRTag.ulCDRMark, pstCDR->stCDRTag.ucCDRType,
             pstCDR->ulCustomerID, pstCDR->ulAccountID,
             pstCDR->ulUserID, pstCDR->ulAgentID,
             pstCDR->ulTaskID, pstCDR->ulLen,
             pstCDR->szCaller, pstCDR->szCallee,
             pstCDR->szAgentNum, pstCDR->aulPeerIP[0],
             pstCDR->ulTimeStamp, pstCDR->ulArrivedTimeStamp,
             pstCDR->ucServType, pstCDR->usTerminateCause);

    /* ��Ϣ��� */
    pthread_mutex_lock(&g_mutexBSBilling);
    DLL_Add(&g_stBSBillingList, pstMsgNode);
    pthread_cond_signal(&g_condBSBillingList);
    pthread_mutex_unlock(&g_mutexBSBilling);
    return;

}

/* ���ɺ���ͳ����Ϣ */
VOID bss_generate_outband_stat(BS_STAT_TAG *pstStatTag, BS_STAT_OUTBAND *pstOutbandStat)
{
    DLL_NODE_S          *pstMsgNode = NULL;
    BS_STAT_OUTBAND_ST  *pstStat = NULL;

    pstMsgNode = dos_dmem_alloc(sizeof(DLL_NODE_S));
    if (NULL == pstMsgNode)
    {
        bs_trace(BS_TRACE_STAT, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
        return;
    }
    DLL_Init_Node(pstMsgNode);

    pstStat = dos_dmem_alloc(sizeof(BS_STAT_OUTBAND_ST));
    if (NULL == pstStat)
    {
        dos_dmem_free(pstMsgNode);
        bs_trace(BS_TRACE_STAT, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
        return;
    }

    /* ����ͳ����Ϣ */
    dos_memzero(pstStat, sizeof(BS_STAT_OUTBAND_ST));
    pstStat->stStatTag = *pstStatTag;
    pstStat->stOutBand = *pstOutbandStat;
    pstMsgNode->pHandle = (VOID *)pstStat;

    bs_trace(BS_TRACE_STAT, LOG_LEVEL_DEBUG,
             "Generate outband stat, "
             "object type:%u, object id:%u, time stamp:%u, call cnt:%u, "
             "ring cnt:%u, busy cnt:%u, not exist cnt:%u, no answer cnt:%u, "
             "reject cnt:%u, early release cnt:%u, answer cnt:%u, total pdd:%u, "
             "total answer time:%u",
             pstStat->stStatTag.ucObjectType, pstStat->stStatTag.ulObjectID,
             pstStat->stOutBand.ulTimeStamp, pstStat->stOutBand.ulCallCnt,
             pstStat->stOutBand.ulRingCnt, pstStat->stOutBand.ulBusyCnt,
             pstStat->stOutBand.ulNotExistCnt, pstStat->stOutBand.ulNoAnswerCnt,
             pstStat->stOutBand.ulRejectCnt, pstStat->stOutBand.ulEarlyReleaseCnt,
             pstStat->stOutBand.ulAnswerCnt, pstStat->stOutBand.ulPDD,
             pstStat->stOutBand.ulAnswerTime);

    bss_send_stat2dl(pstMsgNode, BS_INTER_MSG_OUTBAND_STAT);

}

/* ���ɺ���ͳ����Ϣ */
VOID bss_generate_inband_stat(BS_STAT_TAG *pstStatTag, BS_STAT_INBAND *pstInbandStat)
{
    DLL_NODE_S          *pstMsgNode = NULL;
    BS_STAT_INBAND_ST   *pstStat = NULL;

    pstMsgNode = dos_dmem_alloc(sizeof(DLL_NODE_S));
    if (NULL == pstMsgNode)
    {
        bs_trace(BS_TRACE_STAT, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
        return;
    }
    DLL_Init_Node(pstMsgNode);

    pstStat = dos_dmem_alloc(sizeof(BS_STAT_INBAND_ST));
    if (NULL == pstStat)
    {
        dos_dmem_free(pstMsgNode);
        bs_trace(BS_TRACE_STAT, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
        return;
    }

    /* ����ͳ����Ϣ */
    dos_memzero(pstStat, sizeof(BS_STAT_INBAND_ST));
    pstStat->stStatTag = *pstStatTag;
    pstStat->stInBand = *pstInbandStat;
    pstMsgNode->pHandle = (VOID *)pstStat;

    bs_trace(BS_TRACE_STAT, LOG_LEVEL_DEBUG,
             "Generate outband stat, "
             "object type:%u, object id:%u, time stamp:%u, call cnt:%u, "
             "ring cnt:%u, busy cnt:%u, no answer cnt:%u, early release cnt:%u, "
             "answer cnt:%u, conn agent cnt:%u, agent answer cnt:%u, hold cnt:%u, "
             "total answer time:%u, total wait angent time:%u, "
             "total agent answer time:%u, total hold time:%u",
             pstStat->stStatTag.ucObjectType, pstStat->stStatTag.ulObjectID,
             pstStat->stInBand.ulTimeStamp, pstStat->stInBand.ulCallCnt,
             pstStat->stInBand.ulRingCnt, pstStat->stInBand.ulBusyCnt,
             pstStat->stInBand.ulNoAnswerCnt, pstStat->stInBand.ulEarlyReleaseCnt,
             pstStat->stInBand.ulAnswerCnt, pstStat->stInBand.ulConnAgentCnt,
             pstStat->stInBand.ulAgentAnswerCnt, pstStat->stInBand.ulHoldCnt,
             pstStat->stInBand.ulAnswerTime, pstStat->stInBand.ulWaitAgentTime,
             pstStat->stInBand.ulAgentAnswerTime, pstStat->stInBand.ulHoldTime);

    bss_send_stat2dl(pstMsgNode, BS_INTER_MSG_INBAND_STAT);

}

/* �������ͳ����Ϣ */
VOID bss_generate_outdialing_stat(BS_STAT_TAG *pstStatTag, BS_STAT_OUTDIALING *pstOutDialingStat)
{
    DLL_NODE_S              *pstMsgNode = NULL;
    BS_STAT_OUTDIALING_ST   *pstStat = NULL;

    pstMsgNode = dos_dmem_alloc(sizeof(DLL_NODE_S));
    if (NULL == pstMsgNode)
    {
        bs_trace(BS_TRACE_STAT, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
        return;
    }
    DLL_Init_Node(pstMsgNode);

    pstStat = dos_dmem_alloc(sizeof(BS_STAT_OUTDIALING_ST));
    if (NULL == pstStat)
    {
        dos_dmem_free(pstMsgNode);
        bs_trace(BS_TRACE_STAT, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
        return;
    }

    /* ����ͳ����Ϣ */
    dos_memzero(pstStat, sizeof(BS_STAT_OUTDIALING_ST));
    pstStat->stStatTag = *pstStatTag;
    pstStat->stOutDialing = *pstOutDialingStat;
    pstMsgNode->pHandle = (VOID *)pstStat;

    bs_trace(BS_TRACE_STAT, LOG_LEVEL_DEBUG,
             "Generate outband stat, "
             "object type:%u, object id:%u, time stamp:%u, call cnt:%u, "
             "ring cnt:%u, busy cnt:%u, not exist cnt:%u, no answer cnt:%u, "
             "reject cnt:%u, early release cnt:%u, answer cnt:%u, conn agent cnt:%u, "
             "agent answer cnt:%u, total pdd:%u, total answer time:%u, total wait angent time:%u, "
             "total agent answer time:%u",
             pstStat->stStatTag.ucObjectType, pstStat->stStatTag.ulObjectID,
             pstStat->stOutDialing.ulTimeStamp, pstStat->stOutDialing.ulCallCnt,
             pstStat->stOutDialing.ulRingCnt, pstStat->stOutDialing.ulBusyCnt,
             pstStat->stOutDialing.ulNotExistCnt, pstStat->stOutDialing.ulNoAnswerCnt,
             pstStat->stOutDialing.ulRejectCnt, pstStat->stOutDialing.ulEarlyReleaseCnt,
             pstStat->stOutDialing.ulAnswerCnt, pstStat->stOutDialing.ulConnAgentCnt,
             pstStat->stOutDialing.ulAgentAnswerCnt, pstStat->stOutDialing.ulPDD,
             pstStat->stOutDialing.ulAnswerTime, pstStat->stOutDialing.ulWaitAgentTime,
             pstStat->stOutDialing.ulAgentAnswerTime);

    bss_send_stat2dl(pstMsgNode, BS_INTER_MSG_OUTDIALING_STAT);

}

/* ������Ϣͳ����Ϣ */
VOID bss_generate_message_stat(BS_STAT_TAG *pstStatTag, BS_STAT_MESSAGE *pstMessageStat)
{
    DLL_NODE_S          *pstMsgNode = NULL;
    BS_STAT_MESSAGE_ST  *pstStat = NULL;

    pstMsgNode = dos_dmem_alloc(sizeof(DLL_NODE_S));
    if (NULL == pstMsgNode)
    {
        bs_trace(BS_TRACE_STAT, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
        return;
    }
    DLL_Init_Node(pstMsgNode);

    pstStat = dos_dmem_alloc(sizeof(BS_STAT_MESSAGE_ST));
    if (NULL == pstStat)
    {
        dos_dmem_free(pstMsgNode);
        bs_trace(BS_TRACE_STAT, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
        return;
    }

    /* ����ͳ����Ϣ */
    dos_memzero(pstStat, sizeof(BS_STAT_MESSAGE_ST));
    pstStat->stStatTag = *pstStatTag;
    pstStat->stMS = *pstMessageStat;
    pstMsgNode->pHandle = (VOID *)pstStat;

    bs_trace(BS_TRACE_STAT, LOG_LEVEL_DEBUG,
             "Generate outband stat, "
             "object type:%u, object id:%u, time stamp:%u, send cnt:%u, "
             "recv cnt:%u, succ cnt:%u, fail cnt:%u, send len:%u, recv len:%u, ",
             pstStat->stStatTag.ucObjectType, pstStat->stStatTag.ulObjectID,
             pstStat->stMS.ulTimeStamp, pstStat->stMS.ulSendCnt,
             pstStat->stMS.ulRecvCnt, pstStat->stMS.ulSendSucc,
             pstStat->stMS.ulSendFail, pstStat->stMS.ulSendLen,
             pstStat->stMS.ulRecvLen);

    bss_send_stat2dl(pstMsgNode, BS_INTER_MSG_MESSAGE_STAT);

}

/* �����˻�ͳ����Ϣ */
VOID bss_generate_account_stat(BS_STAT_ACCOUNT_ST *pstAccountStat)
{
    DLL_NODE_S          *pstMsgNode = NULL;
    BS_STAT_ACCOUNT_ST  *pstStat = NULL;

    pstMsgNode = dos_dmem_alloc(sizeof(DLL_NODE_S));
    if (NULL == pstMsgNode)
    {
        bs_trace(BS_TRACE_STAT, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
        return;
    }
    DLL_Init_Node(pstMsgNode);

    pstStat = dos_dmem_alloc(sizeof(BS_STAT_ACCOUNT_ST));
    if (NULL == pstStat)
    {
        dos_dmem_free(pstMsgNode);
        bs_trace(BS_TRACE_STAT, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
        return;
    }

    /* ����ͳ����Ϣ */
    dos_memzero(pstStat, sizeof(BS_STAT_ACCOUNT_ST));
    *pstStat = *pstAccountStat;
    pstMsgNode->pHandle = (VOID *)pstStat;

    bs_trace(BS_TRACE_STAT, LOG_LEVEL_DEBUG,
             "Generate account stat, "
             "object type:%u, object id:%u, time stamp:%u, profit:%d, "
             "outband fee:%d, inband fee:%d, auto dialing fee:%d, preview dialing fee:%d, "
             "predictive fee:%d, record fee:%d, sms fee:%d, mms fee:%d, rent fee:%d",
             pstStat->stStatTag.ucObjectType, pstStat->stStatTag.ulObjectID,
             pstStat->ulTimeStamp, pstStat->lProfit,
             pstStat->lOutBandCallFee, pstStat->lInBandCallFee,
             pstStat->lAutoDialingFee, pstStat->lPreviewDailingFee,
             pstStat->lPredictiveDailingFee, pstStat->lRecordFee,
             pstStat->lSmsFee, pstStat->lMmsFee,
             pstStat->lRentFee);

    bss_send_stat2dl(pstMsgNode, BS_INTER_MSG_ACCOUNT_STAT);

}

/* �����ֽ� */
VOID bss_cdr_factoring(BS_BILL_SESSION_LEG *pstSessionLeg)
{
    U8              i, ucServType;

    for (i = 0; i < BS_MAX_SERVICE_TYPE_IN_SESSION; i++)
    {
        ucServType = pstSessionLeg->aucServType[i];

        switch (ucServType)
        {
            case BS_SERV_OUTBAND_CALL:
            case BS_SERV_INBAND_CALL:
            case BS_SERV_INTER_CALL:
            case BS_SERV_AUTO_DIALING:
            case BS_SERV_PREVIEW_DIALING:
            case BS_SERV_PREDICTIVE_DIALING:
            case BS_SERV_CALL_FORWARD:
            case BS_SERV_CALL_TRANSFER:
            case BS_SERV_PICK_UP:
            case BS_SERV_VERIFY:
                bss_generate_voice_cdr(pstSessionLeg, ucServType);
                break;

            case BS_SERV_RECORDING:
                /* ���ڴ˴����� */
                break;

            case BS_SERV_CONFERENCE:
                /* �ݲ�����,�Ժ�֧�� */
                break;

            case BS_SERV_VOICE_MAIL:
                /* �ݲ�����,�Ժ�֧�� */
                break;

            case BS_SERV_SMS_SEND:
            case BS_SERV_SMS_RECV:
            case BS_SERV_MMS_SEND:
            case BS_SERV_MMS_RECV:
                bss_generate_message_cdr(pstSessionLeg, ucServType);
                break;

            default:

                break;

        }
    }
}

/* ��ʼ�Ʒ� */
VOID bss_billing_start(DLL_NODE_S *pMsgNode)
{
    S32             i, j;
    U32             ulServNum, ulStrLen;
    S8              szServType[128] = {'\0',};
    BS_MSG_CDR      *pstMsg;
    BS_BILL_SESSION_LEG *pstSessionLeg;

    /* ��ҵ��ֽ⻰��,���ڵ��Ʒѻ���������,���üƷ�׼�� */

    //TODO:���ڼ��ʵ�ֵĿ���,�ݲ�����ʼ�Ʒ���Ϣ,ͳһ�ŵ��Ʒѽ�����Ϣ�н��д���

    /* ǰ���Ѿ��жϹ���ַ�Ϸ���,�˴�ֱ��ʹ�ü��� */
    pstMsg = pMsgNode->pHandle;

    for (i = 0; i < (S32)pstMsg->ucLegNum && i < BS_MAX_SESSION_LEG_IN_BILL; i++)
    {
        pstSessionLeg = &pstMsg->astSessionLeg[i];
        for (j = 0, ulServNum = 0; j < BS_MAX_SERVICE_TYPE_IN_SESSION; j++)
        {
            if (0 == pstSessionLeg->aucServType[j]
                || U8_BUTT == pstSessionLeg->aucServType[j]
                || BS_SERV_BUTT == pstSessionLeg->aucServType[j])
            {
                break;
            }

            ulServNum++;
            ulStrLen = dos_strlen(szServType);
            dos_snprintf(szServType + ulStrLen,
                         sizeof(szServType) - ulStrLen,
                         "serv%u:%u, ", j, pstSessionLeg->aucServType[j]);
            szServType[sizeof(szServType) - 1] = '\0';

        }

        /* ������Ϣ,�ַ������ͱ����Դ����� */
        pstSessionLeg->szRecordFile[sizeof(pstSessionLeg->szRecordFile)-1] = '\0';
        pstSessionLeg->szSessionID[sizeof(pstSessionLeg->szSessionID)-1] = '\0';
        pstSessionLeg->szCaller[sizeof(pstSessionLeg->szCaller)-1] = '\0';
        pstSessionLeg->szCallee[sizeof(pstSessionLeg->szCallee)-1] = '\0';
        pstSessionLeg->szCID[sizeof(pstSessionLeg->szCID)-1] = '\0';
        pstSessionLeg->szAgentNum[sizeof(pstSessionLeg->szAgentNum)-1] = '\0';

        bs_trace(BS_TRACE_FS, LOG_LEVEL_DEBUG,
                 "Billing start, leg:%d, customer:%u, account:%u, userid:%u, "
                 "agentid:%u, taskid:%u, %s, record:%s, "
                 "caller:%s, callee:%s, cid:%s, agent:%s, session:%s, "
                 "start time:%u, ring time:%u, answer time:%u, bye time:%u, "
                 "trunk id:%u, loss rate:%u, release part:%u, terminate cause:%u",
                 i, pstSessionLeg->ulCustomerID, pstSessionLeg->ulAccountID,
                 pstSessionLeg->ulUserID, pstSessionLeg->ulAgentID,
                 pstSessionLeg->ulTaskID, szServType,
                 pstSessionLeg->szRecordFile, pstSessionLeg->szCaller,
                 pstSessionLeg->szCallee, pstSessionLeg->szCID,
                 pstSessionLeg->szAgentNum, pstSessionLeg->szSessionID,
                 pstSessionLeg->ulStartTimeStamp, pstSessionLeg->ulRingTimeStamp,
                 pstSessionLeg->ulAnswerTimeStamp, pstSessionLeg->ulByeTimeStamp,
                 pstSessionLeg->usPeerTrunkID, pstSessionLeg->ucPacketLossRate,
                 pstSessionLeg->ucReleasePart, pstSessionLeg->usTerminateCause);

    }
}

/* �ƷѸ��� */
VOID bss_billing_update(DLL_NODE_S *pMsgNode)
{
    S32             i, j;
    U32             ulServNum, ulStrLen;
    S8              szServType[128] = {'\0',};
    BS_MSG_CDR      *pstMsg;
    BS_BILL_SESSION_LEG *pstSessionLeg;

    /* ʵʱ�����˻����,����ʱ������,���������̷������;
       �ر��,����û�мƷѽ�����Ϣ�����,������̴߳��� */

    //TODO:һ��ͨ�������мƷ���Ϣ���ܷ����ı�,�ֽ׶λ��ڼ�ʵ�ֵĿ���,�ݲ�����ƷѸ�����Ϣ,ͳһ�ŵ��Ʒѽ�����Ϣ�н��д���


    /* ǰ���Ѿ��жϹ���ַ�Ϸ���,�˴�ֱ��ʹ�ü��� */
    pstMsg = pMsgNode->pHandle;
    for (i = 0; i < (S32)pstMsg->ucLegNum && i < BS_MAX_SESSION_LEG_IN_BILL; i++)
    {
        pstSessionLeg = &pstMsg->astSessionLeg[i];
        for (j = 0, ulServNum = 0; j < BS_MAX_SERVICE_TYPE_IN_SESSION; j++)
        {
            if (0 == pstSessionLeg->aucServType[j]
                || U8_BUTT == pstSessionLeg->aucServType[j]
                || BS_SERV_BUTT == pstSessionLeg->aucServType[j])
            {
                break;
            }

            ulServNum++;
            ulStrLen = dos_strlen(szServType);
            dos_snprintf(szServType + ulStrLen,
                         sizeof(szServType) - ulStrLen,
                         "serv%u:%u", j, pstSessionLeg->aucServType[j]);
            szServType[sizeof(szServType) - 1] = '\0';

        }

        /* ������Ϣ,�ַ������ͱ����Դ����� */
        pstSessionLeg->szRecordFile[sizeof(pstSessionLeg->szRecordFile)-1] = '\0';
        pstSessionLeg->szSessionID[sizeof(pstSessionLeg->szSessionID)-1] = '\0';
        pstSessionLeg->szCaller[sizeof(pstSessionLeg->szCaller)-1] = '\0';
        pstSessionLeg->szCallee[sizeof(pstSessionLeg->szCallee)-1] = '\0';
        pstSessionLeg->szCID[sizeof(pstSessionLeg->szCID)-1] = '\0';
        pstSessionLeg->szAgentNum[sizeof(pstSessionLeg->szAgentNum)-1] = '\0';

        bs_trace(BS_TRACE_FS, LOG_LEVEL_DEBUG,
                 "Billing update, leg:%d, customer:%u, account:%u, userid:%u, "
                 "agentid:%u, taskid:%u, %s, record:%s, "
                 "caller:%s, callee:%s, cid:%s, agent:%s, session:%s, "
                 "start time:%u, ring time:%u, answer time:%u, bye time:%u, "
                 "trunk id:%u, loss rate:%u, release part:%u, terminate cause:%u",
                 i, pstSessionLeg->ulCustomerID, pstSessionLeg->ulAccountID,
                 pstSessionLeg->ulUserID, pstSessionLeg->ulAgentID,
                 pstSessionLeg->ulTaskID, szServType,
                 pstSessionLeg->szRecordFile, pstSessionLeg->szCaller,
                 pstSessionLeg->szCallee, pstSessionLeg->szCID,
                 pstSessionLeg->szAgentNum, pstSessionLeg->szSessionID,
                 pstSessionLeg->ulStartTimeStamp, pstSessionLeg->ulRingTimeStamp,
                 pstSessionLeg->ulAnswerTimeStamp, pstSessionLeg->ulByeTimeStamp,
                 pstSessionLeg->usPeerTrunkID, pstSessionLeg->ucPacketLossRate,
                 pstSessionLeg->ucReleasePart, pstSessionLeg->usTerminateCause);

    }

}

/* �Ʒѽ���:��ҵ��ֽ⻰��,���мƷ�,������;ͬʱ����ԭʼ�����Ĵ洢 */
VOID bss_billing_stop(DLL_NODE_S *pMsgNode)
{
    S32             i, j;
    U32             ulStrLen;
    S8              szServType[128] = {'\0',};
    U8              ucRecordServIndex = U8_BUTT;
    BS_MSG_CDR      *pstMsg;
    BS_BILL_SESSION_LEG *pstSessionLeg;

    /* ǰ���Ѿ��жϹ���ַ�Ϸ���,�˴�ֱ��ʹ�ü��� */
    pstMsg = pMsgNode->pHandle;

    for (i = 0; i < (S32)pstMsg->ucLegNum && i < BS_MAX_SESSION_LEG_IN_BILL; i++)
    {
        pstSessionLeg = &pstMsg->astSessionLeg[i];
        ucRecordServIndex = U8_BUTT;
        for (j = 0; j < BS_MAX_SERVICE_TYPE_IN_SESSION; j++)
        {
            if (0 == pstSessionLeg->aucServType[j]
                || U8_BUTT == pstSessionLeg->aucServType[j]
                || BS_SERV_BUTT < pstSessionLeg->aucServType[j])
            {
                break;
            }

            if (BS_SERV_RECORDING == pstSessionLeg->aucServType[j]
                && ucRecordServIndex == U8_BUTT)
            {
                ucRecordServIndex = j;
            }

            ulStrLen = dos_strlen(szServType);
            dos_snprintf(szServType + ulStrLen,
                         sizeof(szServType) - ulStrLen,
                         "serv%u:%u, ", j, pstSessionLeg->aucServType[j]);
            szServType[sizeof(szServType) - 1] = '\0';

        }

        /* ������Ϣ,�ַ������ͱ����Դ����� */
        pstSessionLeg->szRecordFile[sizeof(pstSessionLeg->szRecordFile)-1] = '\0';
        pstSessionLeg->szSessionID[sizeof(pstSessionLeg->szSessionID)-1] = '\0';
        pstSessionLeg->szCaller[sizeof(pstSessionLeg->szCaller)-1] = '\0';
        pstSessionLeg->szCallee[sizeof(pstSessionLeg->szCallee)-1] = '\0';
        pstSessionLeg->szCID[sizeof(pstSessionLeg->szCID)-1] = '\0';
        pstSessionLeg->szAgentNum[sizeof(pstSessionLeg->szAgentNum)-1] = '\0';

        /* ����CDR��� */
        pstSessionLeg->ulCDRMark = g_stBssCB.ulCDRMark;

        bs_trace(BS_TRACE_FS, LOG_LEVEL_DEBUG,
                 "Billing start, leg:%d, mark:%u, customer:%u, account:%u, "
                 "userid:%u, agentid:%u, taskid:%u, %s, record:%s, "
                 "caller:%s, callee:%s, cid:%s, agent:%s, session:%s, "
                 "start time:%u, ring time:%u, answer time:%u, bye time:%u, "
                 "trunk id:%u, loss rate:%u, release part:%u, terminate cause:%u",
                 i, pstSessionLeg->ulCDRMark,
                 pstSessionLeg->ulCustomerID, pstSessionLeg->ulAccountID,
                 pstSessionLeg->ulUserID, pstSessionLeg->ulAgentID,
                 pstSessionLeg->ulTaskID, szServType,
                 pstSessionLeg->szRecordFile, pstSessionLeg->szCaller,
                 pstSessionLeg->szCallee, pstSessionLeg->szCID,
                 pstSessionLeg->szAgentNum, pstSessionLeg->szSessionID,
                 pstSessionLeg->ulStartTimeStamp, pstSessionLeg->ulRingTimeStamp,
                 pstSessionLeg->ulAnswerTimeStamp, pstSessionLeg->ulByeTimeStamp,
                 pstSessionLeg->usPeerTrunkID, pstSessionLeg->ucPacketLossRate,
                 pstSessionLeg->ucReleasePart, pstSessionLeg->usTerminateCause);

        /* ¼��ҵ����һ������ҵ��,����ϯϢϢ���, ����ֱ�Ӹ���LEG����¼������ */
        if (ucRecordServIndex != U8_BUTT)
        {
            bss_generate_record_cdr(pstSessionLeg);
        }

        /* ���ɽ��㻰�� */
        if (pstSessionLeg->usPeerTrunkID != U16_BUTT
            && pstSessionLeg->usPeerTrunkID != 0)
        {
            bss_generate_settle_cdr(pstSessionLeg);
        }

    }

    g_stBssCB.ulCDRMark++;

    /* ������LEG�ȶԻ������зֽ� */
    bss_cdr_factoring(&pstMsg->astSessionLeg[0]);
    if (dos_strcmp(pstMsg->astSessionLeg[0].szCallee, pstMsg->astSessionLeg[1].szCallee) != 0)
    {
        /* ���в���ͬ,��������ҵ��,�������һ��LEG���л����ֽ� */
        bss_cdr_factoring(&pstMsg->astSessionLeg[1]);
    }

    /* �ٱ���ԭʼ���� */
    bss_send_cdr2dl(pMsgNode, BS_INTER_MSG_ORIGINAL_CDR);

}

/* ������������ */
VOID bss_voice_cdr_proc(DLL_NODE_S *pMsgNode)
{
    U8                      i;
    S8                      szFee[128] = {'\0',};
    U32                     ulPackageID, ulFee, ulStrLen;
    S32                     lRebate;
    BS_BILLING_MATCH_ST     stBillingMatch;
    BS_CDR_VOICE_ST         *pstCDR = NULL;
    BS_CUSTOMER_ST          *pstCustomer = NULL;
    BS_CUSTOMER_ST          *pstParent = NULL;
    BS_BILLING_RULE_ST      *pstRule = NULL;
    BS_BILLING_PACKAGE_ST   *pstPackage = NULL;

    /* ǰ���Ѿ��жϹ�ָ��Ϸ��� */
    pstCDR = (BS_CDR_VOICE_ST *)pMsgNode->pHandle;

    bs_trace(BS_TRACE_BILLING, LOG_LEVEL_DEBUG,
             "Porc voice cdr, "
             "mark:%u, type:%u, customer:%u, account:%u, "
             "userid:%u, agentid:%u, taskid:%u, record:%s, "
             "caller:%s, callee:%s, cid:%s, agent:%s, "
             "start time:%u, time len:%u, servtype:%u, cause:%u",
             pstCDR->stCDRTag.ulCDRMark, pstCDR->stCDRTag.ucCDRType,
             pstCDR->ulCustomerID, pstCDR->ulAccountID,
             pstCDR->ulUserID, pstCDR->ulAgentID,
             pstCDR->ulTaskID, pstCDR->szRecordFile,
             pstCDR->szCaller, pstCDR->szCallee,
             pstCDR->szCID, pstCDR->szAgentNum,
             pstCDR->ulAnswerTimeStamp, pstCDR->ulTimeLen,
             pstCDR->ucServType, pstCDR->usTerminateCause);

    if (0 == pstCDR->ulTimeLen)
    {
        /* �Ժ�����,��������,ֻ���¼���� */
        goto save_cdr;
    }

    pstCustomer = bs_get_customer_st(pstCDR->ulCustomerID);
    if (NULL == pstCustomer)
    {
        /* �Ҳ�����Ӧ�Ŀͻ���Ϣ */
        DOS_ASSERT(0);
        bs_trace(BS_TRACE_BILLING, LOG_LEVEL_DEBUG, "Can't find customer!");
        goto save_cdr;
    }

    dos_memzero(&stBillingMatch, sizeof(stBillingMatch));
    stBillingMatch.ulCustomerID = pstCDR->ulCustomerID;
    stBillingMatch.ulComsumerID = pstCDR->ulCustomerID;
    stBillingMatch.ulAgentID = pstCDR->ulAgentID;
    stBillingMatch.ulUserID = pstCDR->ulUserID;
    stBillingMatch.ulTimeStamp = pstCDR->ulAnswerTimeStamp;
    dos_strncpy(stBillingMatch.szCaller, pstCDR->szCaller, sizeof(stBillingMatch.szCaller));
    dos_strncpy(stBillingMatch.szCallee, pstCDR->szCallee, sizeof(stBillingMatch.szCallee));

    /* ���ݿͻ���,������� */
    for (i = 0, pstParent = pstCustomer; i < BS_MAX_AGENT_LEVEL; i++)
    {
        if (NULL == pstParent)
        {
            /* û�и��ͻ�,Ӧ�����ڶ���;����Ӧ����������� */
            DOS_ASSERT(0);
            goto save_cdr;
        }

        if (BS_CUSTOMER_TYPE_TOP == pstParent->ucCustomerType)
        {
            /* �����ͻ�,�����ٲ� */
            break;
        }

        ulPackageID = pstParent->stAccount.ulBillingPackageID;
        pstPackage = bs_get_billing_package(ulPackageID, pstCDR->ucServType);
        if (NULL == pstPackage)
        {
            /* ���Ҳ����Ʒѹ��� */
            bs_trace(BS_TRACE_BILLING, LOG_LEVEL_WARNING,
                     "Warning: can not find billing package! "
                     "Customer:%u, package:%u, servtype:%u, CDR mark:%u",
                     pstCustomer->ulCustomerID, pstCustomer->stAccount.ulBillingPackageID,
                     pstCDR->ucServType, pstCDR->stCDRTag.ulCDRMark);
            goto save_cdr;
        }

        pstRule = bs_match_billing_rule(pstPackage, &stBillingMatch);
        if (NULL == pstRule)
        {
            /* ƥ��ʧ�� */
            bs_trace(BS_TRACE_BILLING, LOG_LEVEL_WARNING,
                     "Warning: match billing rule fail! "
                     "package:%u, service:%u, customer:%u, agent:%u, "
                     "userid:%u, timestamp:%u, caller:%s, callee:%s, CDR mark:%u",
                     pstPackage->ulPackageID, pstPackage->ucServType,
                     stBillingMatch.ulCustomerID, stBillingMatch.ulAgentID,
                     stBillingMatch.ulUserID, stBillingMatch.ulTimeStamp,
                     stBillingMatch.szCaller, stBillingMatch.szCallee,
                     pstCDR->stCDRTag.ulCDRMark);

            goto save_cdr;
        }

        if (0 == i)
        {
            /* ��¼�����߻����۷Ѷ�Ӧ�ļƷѹ��� */
            pstCDR->ulRuleID = pstRule->ulRuleID;
        }

        if (0 == pstRule->ulBillingRate)
        {
            /* ����Ϊ0,���ü����� */
            goto save_cdr;
        }

        switch (pstRule->ucBillingType)
        {
            case BS_BILLING_BY_TIMELEN:
                ulFee = pstRule->ulBillingRate * pstRule->ucFirstBillingCnt;
                if (pstCDR->ulTimeLen > pstRule->ulFirstBillingUnit)
                {
                    /* �����׸��Ʒѵ�λ */

                    U32     ulCnt;
                    ulCnt = ceil((double)(pstCDR->ulTimeLen - pstRule->ulFirstBillingUnit)/(double)pstRule->ulNextBillingUnit);
                    ulFee += pstRule->ulBillingRate * pstRule->ucNextBillingCnt * ulCnt;
                }

                /* ��¼���� */
                pstCDR->aulFee[i] = ulFee;
                break;

            case BS_BILLING_BY_COUNT:
                /* ���μƷ���Ϊ��,ֱ��ʹ�õ�λ���ʼ��� */
                pstCDR->aulFee[i] = pstRule->ulBillingRate;
                break;

            case BS_BILLING_BY_TRAFFIC:
                /* ��δʵ�� */
                break;

            default:
                DOS_ASSERT(0);
                bs_trace(BS_TRACE_BILLING, LOG_LEVEL_WARNING,
                         "Warning: not supported billing type(%u)! ",
                         pstRule->ucBillingType);
                goto save_cdr;
        }

        ulStrLen = dos_strlen(szFee);
        dos_snprintf(szFee + ulStrLen,
                     sizeof(szFee) - ulStrLen,
                     "fee%u:%u, ", i, pstCDR->aulFee[i]);
        szFee[sizeof(szFee) - 1] = '\0';

        pstParent = pstParent->pstParent;
    }

    if (i >= BS_MAX_AGENT_LEVEL)
    {
        /* �ͻ��㼶Խ��,�쳣 */
        DOS_ASSERT(0);
        bs_trace(BS_TRACE_BILLING, LOG_LEVEL_ERROR,
                 "Err: agent level exceed! customer:%u",
                 stBillingMatch.ulCustomerID);

        goto save_cdr;
    }

    /* ��¼�ͻ��㼶 */
    pstCDR->ucAgentLevel = i;

    /* �۷�&���� */
    pthread_mutex_lock(&pstCustomer->stAccount.mutexAccount);
    pstCustomer->stAccount.LBalanceActive -= pstCDR->aulFee[0];
    /* ����ͳ����Ϣ */
    bs_account_stat_refresh(&pstCustomer->stAccount,
                            pstCDR->aulFee[0],
                            0,
                            pstCDR->ucServType);
    pthread_mutex_unlock(&pstCustomer->stAccount.mutexAccount);

    pstParent = pstCustomer->pstParent;
    for (i = 0; i < pstCDR->ucAgentLevel; i++)
    {
        lRebate = (S32)pstCDR->aulFee[i] - (S32)pstCDR->aulFee[i+1];
        pthread_mutex_lock(&pstParent->stAccount.mutexAccount);
        if (pstParent->ucCustomerType != BS_CUSTOMER_TYPE_TOP)
        {
            /* ֻ�д����̲��з��� */
            pstParent->stAccount.lRebate += lRebate;
            /* ����ͳ����Ϣ */
            bs_account_stat_refresh(&pstParent->stAccount,
                                    pstCDR->aulFee[i+1],
                                    lRebate,
                                    pstCDR->ucServType);
        }
        pthread_mutex_unlock(&pstParent->stAccount.mutexAccount);

        /* ���пͻ��ķ���������ڶ����ͻ� */
        pthread_mutex_lock(&g_stBssCB.pstTopCustomer->stAccount.mutexAccount);
        g_stBssCB.pstTopCustomer->stAccount.lRebate += lRebate;
        pthread_mutex_unlock(&g_stBssCB.pstTopCustomer->stAccount.mutexAccount);

        pstParent = pstParent->pstParent;
    }

    bs_trace(BS_TRACE_BILLING, LOG_LEVEL_DEBUG,
             "Porc voice cdr succ, "
             "mark:%u, customer:%u, account:%u, servtype:%u, "
             "caller:%s, callee:%s, start time:%u, time len:%u, "
             "%s",
             pstCDR->stCDRTag.ulCDRMark, pstCDR->ulCustomerID,
             pstCDR->ulAccountID, pstCDR->ucServType,
             pstCDR->szCaller, pstCDR->szCallee,
             pstCDR->ulAnswerTimeStamp, pstCDR->ulTimeLen,
             szFee);


    /* ����ͳ�� */
    bs_stat_voice(pstCDR);

save_cdr:
    bss_send_cdr2dl(pMsgNode, BS_INTER_MSG_VOICE_CDR);

}

/* ¼���������� */
VOID bss_recording_cdr_proc(DLL_NODE_S *pMsgNode)
{
    U8                      i;
    S8                      szFee[128] = {'\0',};
    U32                     ulPackageID, ulFee, ulStrLen;
    S32                     lRebate;
    BS_BILLING_MATCH_ST     stBillingMatch;
    BS_CDR_RECORDING_ST     *pstCDR = NULL;
    BS_CUSTOMER_ST          *pstCustomer = NULL;
    BS_CUSTOMER_ST          *pstParent = NULL;
    BS_BILLING_RULE_ST      *pstRule = NULL;
    BS_BILLING_PACKAGE_ST   *pstPackage = NULL;

    /* ǰ���Ѿ��жϹ�ָ��Ϸ��� */
    pstCDR = (BS_CDR_RECORDING_ST *)pMsgNode->pHandle;

    bs_trace(BS_TRACE_BILLING, LOG_LEVEL_DEBUG,
             "Porc recording cdr, "
             "mark:%u, type:%u, customer:%u, account:%u, "
             "userid:%u, agentid:%u, taskid:%u, record:%s, "
             "caller:%s, callee:%s, cid:%s, agent:%s, "
             "start time:%u, time len:%u",
             pstCDR->stCDRTag.ulCDRMark, pstCDR->stCDRTag.ucCDRType,
             pstCDR->ulCustomerID, pstCDR->ulAccountID,
             pstCDR->ulUserID, pstCDR->ulAgentID,
             pstCDR->ulTaskID, pstCDR->szRecordFile,
             pstCDR->szCaller, pstCDR->szCallee,
             pstCDR->szCID, pstCDR->szAgentNum,
             pstCDR->ulRecordTimeStamp, pstCDR->ulTimeLen);

    if (0 == pstCDR->ulTimeLen)
    {
        /* �Ժ�����,��������,ֻ���¼���� */
        goto save_cdr;
    }

    pstCustomer = bs_get_customer_st(pstCDR->ulCustomerID);
    if (NULL == pstCustomer)
    {
        /* �Ҳ�����Ӧ�Ŀͻ���Ϣ */
        DOS_ASSERT(0);
        bs_trace(BS_TRACE_BILLING, LOG_LEVEL_DEBUG, "Can't find customer!");
        goto save_cdr;
    }

    dos_memzero(&stBillingMatch, sizeof(stBillingMatch));
    stBillingMatch.ulCustomerID = pstCDR->ulCustomerID;
    stBillingMatch.ulComsumerID = pstCDR->ulCustomerID;
    stBillingMatch.ulAgentID = pstCDR->ulAgentID;
    stBillingMatch.ulUserID = pstCDR->ulUserID;
    stBillingMatch.ulTimeStamp = pstCDR->ulRecordTimeStamp;
    dos_strncpy(stBillingMatch.szCaller, pstCDR->szCaller, sizeof(stBillingMatch.szCaller));
    dos_strncpy(stBillingMatch.szCallee, pstCDR->szCallee, sizeof(stBillingMatch.szCallee));

    /* ���ݿͻ���,������� */
    for (i = 0, pstParent = pstCustomer; i < BS_MAX_AGENT_LEVEL; i++)
    {
        if (NULL == pstParent)
        {
            /* û�и��ͻ�,Ӧ�����ڶ���;����Ӧ����������� */
            DOS_ASSERT(0);
            goto save_cdr;
        }

        if (BS_CUSTOMER_TYPE_TOP == pstParent->ucCustomerType)
        {
            /* �����ͻ�,�����ٲ� */
            break;
        }

        ulPackageID = pstParent->stAccount.ulBillingPackageID;
        pstPackage = bs_get_billing_package(ulPackageID, BS_SERV_RECORDING);
        if (NULL == pstPackage)
        {
            /* ���Ҳ����Ʒѹ��� */
            bs_trace(BS_TRACE_BILLING, LOG_LEVEL_WARNING,
                     "Warning: can not find billing package! "
                     "Customer:%u, package:%u, CDR mark:%u",
                     pstCustomer->ulCustomerID, pstCustomer->stAccount.ulBillingPackageID,
                     pstCDR->stCDRTag.ulCDRMark);
            goto save_cdr;
        }

        pstRule = bs_match_billing_rule(pstPackage, &stBillingMatch);
        if (NULL == pstRule)
        {
            /* ƥ��ʧ�� */
            bs_trace(BS_TRACE_BILLING, LOG_LEVEL_WARNING,
                     "Warning: match billing rule fail! "
                     "package:%u, service:%u, customer:%u, agent:%u, "
                     "userid:%u, timestamp:%u, caller:%s, callee:%s, CDR mark:%u",
                     pstPackage->ulPackageID, pstPackage->ucServType,
                     stBillingMatch.ulCustomerID, stBillingMatch.ulAgentID,
                     stBillingMatch.ulUserID, stBillingMatch.ulTimeStamp,
                     stBillingMatch.szCaller, stBillingMatch.szCallee,
                     pstCDR->stCDRTag.ulCDRMark);

            goto save_cdr;
        }

        if (0 == i)
        {
            /* ��¼�����߻����۷Ѷ�Ӧ�ļƷѹ��� */
            pstCDR->ulRuleID = pstRule->ulRuleID;
        }

        if (0 == pstRule->ulBillingRate)
        {
            /* ����Ϊ0,���ü����� */
            goto save_cdr;
        }

        switch (pstRule->ucBillingType)
        {
            case BS_BILLING_BY_TIMELEN:
                ulFee = pstRule->ulBillingRate * pstRule->ucFirstBillingCnt;
                if (pstCDR->ulTimeLen > pstRule->ulFirstBillingUnit)
                {
                    /* �����׸��Ʒѵ�λ */

                    U32     ulCnt;

                    ulCnt = ceil((double)(pstCDR->ulTimeLen - pstRule->ulFirstBillingUnit)/(double)pstRule->ulNextBillingUnit);
                    ulFee += pstRule->ulBillingRate * pstRule->ucNextBillingCnt * ulCnt;
                }

                /* ��¼���� */
                pstCDR->aulFee[i] = ulFee;
                break;

            case BS_BILLING_BY_COUNT:
                /* ���μƷ���Ϊ��,ֱ��ʹ�õ�λ���ʼ��� */
                pstCDR->aulFee[i] = pstRule->ulBillingRate;
                break;

            case BS_BILLING_BY_TRAFFIC:
                /* ��δʵ�� */
                break;

            default:
                DOS_ASSERT(0);
                bs_trace(BS_TRACE_BILLING, LOG_LEVEL_WARNING,
                         "Warning: not supported billing type(%u)! ",
                         pstRule->ucBillingType);
                goto save_cdr;
        }

        ulStrLen = dos_strlen(szFee);
        dos_snprintf(szFee + ulStrLen,
                     sizeof(szFee) - ulStrLen,
                     "fee%u:%u, ", i, pstCDR->aulFee[i]);
        szFee[sizeof(szFee) - 1] = '\0';

        pstParent = pstParent->pstParent;
    }

    if (i >= BS_MAX_AGENT_LEVEL)
    {
        /* �ͻ��㼶Խ��,�쳣 */
        DOS_ASSERT(0);
        bs_trace(BS_TRACE_BILLING, LOG_LEVEL_ERROR,
                 "Err: agent level exceed! customer:%u",
                 stBillingMatch.ulCustomerID);

        goto save_cdr;
    }

    /* ��¼�ͻ��㼶 */
    pstCDR->ucAgentLevel = i;

    /* �۷�&���� */
    pthread_mutex_lock(&pstCustomer->stAccount.mutexAccount);
    pstCustomer->stAccount.LBalanceActive -= pstCDR->aulFee[0];
    /* ����ͳ����Ϣ */
    bs_account_stat_refresh(&pstCustomer->stAccount,
                            pstCDR->aulFee[0],
                            0,
                            BS_SERV_RECORDING);
    pthread_mutex_unlock(&pstCustomer->stAccount.mutexAccount);
    pstParent = pstCustomer->pstParent;
    for (i = 0; i < pstCDR->ucAgentLevel; i++)
    {
        lRebate = (S32)pstCDR->aulFee[i] - (S32)pstCDR->aulFee[i+1];
        pthread_mutex_lock(&pstParent->stAccount.mutexAccount);
        if (pstParent->ucCustomerType != BS_CUSTOMER_TYPE_TOP)
        {
            /* ֻ�д����̲��з��� */
            pstParent->stAccount.lRebate += lRebate;
            /* ����ͳ����Ϣ */
            bs_account_stat_refresh(&pstParent->stAccount,
                                    pstCDR->aulFee[i+1],
                                    lRebate,
                                    BS_SERV_RECORDING);
        }
        pthread_mutex_unlock(&pstParent->stAccount.mutexAccount);

        /* ���пͻ��ķ���������ڶ����ͻ� */
        pthread_mutex_lock(&g_stBssCB.pstTopCustomer->stAccount.mutexAccount);
        g_stBssCB.pstTopCustomer->stAccount.lRebate += lRebate;
        pthread_mutex_unlock(&g_stBssCB.pstTopCustomer->stAccount.mutexAccount);

        pstParent = pstParent->pstParent;
    }

    bs_trace(BS_TRACE_BILLING, LOG_LEVEL_DEBUG,
             "Porc recording cdr succ, "
             "mark:%u, customer:%u, account:%u, caller:%s, "
             "callee:%s, start time:%u, time len:%u, %s",
             pstCDR->stCDRTag.ulCDRMark, pstCDR->ulCustomerID,
             pstCDR->ulAccountID, pstCDR->szCaller,
             pstCDR->szCallee, pstCDR->ulRecordTimeStamp,
             pstCDR->ulTimeLen, szFee);

save_cdr:
    bss_send_cdr2dl(pMsgNode, BS_INTER_MSG_RECORDING_CDR);

}

/* ��Ϣ(SMS/MMS)�������� */
VOID bss_message_cdr_proc(DLL_NODE_S *pMsgNode)
{
    U8                      i;
    S8                      szFee[128] = {'\0',};
    U32                     ulPackageID, ulStrLen;
    S32                     lRebate;
    BS_BILLING_MATCH_ST     stBillingMatch;
    BS_CDR_MS_ST            *pstCDR = NULL;
    BS_CUSTOMER_ST          *pstCustomer = NULL;
    BS_CUSTOMER_ST          *pstParent = NULL;
    BS_BILLING_RULE_ST      *pstRule = NULL;
    BS_BILLING_PACKAGE_ST   *pstPackage = NULL;

    /* ǰ���Ѿ��жϹ�ָ��Ϸ��� */
    pstCDR = (BS_CDR_MS_ST *)pMsgNode->pHandle;

    bs_trace(BS_TRACE_BILLING, LOG_LEVEL_DEBUG,
             "Porc message cdr, "
             "mark:%u, type:%u, customer:%u, account:%u, "
             "userid:%u, agentid:%u, taskid:%u, caller:%s, "
             "callee:%s, agent:%s, time:%u, len:%u, "
             "servtype:%u, cause:%u",
             pstCDR->stCDRTag.ulCDRMark, pstCDR->stCDRTag.ucCDRType,
             pstCDR->ulCustomerID, pstCDR->ulAccountID,
             pstCDR->ulUserID, pstCDR->ulAgentID,
             pstCDR->ulTaskID, pstCDR->szCaller,
             pstCDR->szCallee, pstCDR->szAgentNum,
             pstCDR->ulTimeStamp, pstCDR->ulLen,
             pstCDR->ucServType, pstCDR->usTerminateCause);

    if (0 == pstCDR->ulArrivedTimeStamp)
    {
        /* �Ժ�����,��������,ֻ���¼���� */
        goto save_cdr;
    }

    pstCustomer = bs_get_customer_st(pstCDR->ulCustomerID);
    if (NULL == pstCustomer)
    {
        /* �Ҳ�����Ӧ�Ŀͻ���Ϣ */
        DOS_ASSERT(0);
        bs_trace(BS_TRACE_BILLING, LOG_LEVEL_DEBUG, "Can't find customer!");
        goto save_cdr;
    }

    dos_memzero(&stBillingMatch, sizeof(stBillingMatch));
    stBillingMatch.ulCustomerID = pstCDR->ulCustomerID;
    stBillingMatch.ulComsumerID = pstCDR->ulCustomerID;
    stBillingMatch.ulAgentID = pstCDR->ulAgentID;
    stBillingMatch.ulUserID = pstCDR->ulUserID;
    stBillingMatch.ulTimeStamp = pstCDR->ulTimeStamp;
    dos_strncpy(stBillingMatch.szCaller, pstCDR->szCaller, sizeof(stBillingMatch.szCaller));
    dos_strncpy(stBillingMatch.szCallee, pstCDR->szCallee, sizeof(stBillingMatch.szCallee));

    /* ���ݿͻ���,������� */
    for (i = 0, pstParent = pstCustomer; i < BS_MAX_AGENT_LEVEL; i++)
    {
        if (NULL == pstParent)
        {
            /* û�и��ͻ�,Ӧ�����ڶ���;����Ӧ����������� */
            DOS_ASSERT(0);
            goto save_cdr;
        }

        if (BS_CUSTOMER_TYPE_TOP == pstParent->ucCustomerType)
        {
            /* �����ͻ�,�����ٲ� */
            break;
        }

        ulPackageID = pstParent->stAccount.ulBillingPackageID;
        pstPackage = bs_get_billing_package(ulPackageID, pstCDR->ucServType);
        if (NULL == pstPackage)
        {
            /* ���Ҳ����Ʒѹ��� */
            bs_trace(BS_TRACE_BILLING, LOG_LEVEL_WARNING,
                     "Warning: can not find billing package! "
                     "Customer:%u, package:%u, servtype:%u, CDR mark:%u",
                     pstCustomer->ulCustomerID, pstCustomer->stAccount.ulBillingPackageID,
                     pstCDR->ucServType, pstCDR->stCDRTag.ulCDRMark);
            goto save_cdr;
        }

        pstRule = bs_match_billing_rule(pstPackage, &stBillingMatch);
        if (NULL == pstRule)
        {
            /* ƥ��ʧ�� */
            bs_trace(BS_TRACE_BILLING, LOG_LEVEL_WARNING,
                     "Warning: match billing rule fail! "
                     "package:%u, service:%u, customer:%u, agent:%u, "
                     "userid:%u, timestamp:%u, caller:%s, callee:%s, CDR mark:%u",
                     pstPackage->ulPackageID, pstPackage->ucServType,
                     stBillingMatch.ulCustomerID, stBillingMatch.ulAgentID,
                     stBillingMatch.ulUserID, stBillingMatch.ulTimeStamp,
                     stBillingMatch.szCaller, stBillingMatch.szCallee,
                     pstCDR->stCDRTag.ulCDRMark);

            goto save_cdr;
        }

        if (0 == i)
        {
            /* ��¼�����߻����۷Ѷ�Ӧ�ļƷѹ��� */
            pstCDR->ulRuleID = pstRule->ulRuleID;
        }

        if (0 == pstRule->ulBillingRate)
        {
            /* ����Ϊ0,���ü����� */
            goto save_cdr;
        }

        switch (pstRule->ucBillingType)
        {
            case BS_BILLING_BY_COUNT:
                /* ���μƷ���Ϊ��,ֱ��ʹ�õ�λ���ʼ��� */
                pstCDR->aulFee[i] = pstRule->ulBillingRate;
                break;

            case BS_BILLING_BY_TRAFFIC:
                /* ��δʵ�� */
                break;

            default:
                DOS_ASSERT(0);
                bs_trace(BS_TRACE_BILLING, LOG_LEVEL_WARNING,
                         "Warning: not supported billing type(%u)! ",
                         pstRule->ucBillingType);
                goto save_cdr;
        }

        ulStrLen = dos_strlen(szFee);
        dos_snprintf(szFee + ulStrLen,
                     sizeof(szFee) - ulStrLen,
                     "fee%u:%u, ", i, pstCDR->aulFee[i]);
        szFee[sizeof(szFee) - 1] = '\0';

        pstParent = pstParent->pstParent;
    }

    if (i >= BS_MAX_AGENT_LEVEL)
    {
        /* �ͻ��㼶Խ��,�쳣 */
        DOS_ASSERT(0);
        bs_trace(BS_TRACE_BILLING, LOG_LEVEL_ERROR,
                 "Err: agent level exceed! customer:%u",
                 stBillingMatch.ulCustomerID);

        goto save_cdr;
    }

    /* ��¼�ͻ��㼶 */
    pstCDR->ucAgentLevel = i;

    /* �۷�&���� */
    pthread_mutex_lock(&pstCustomer->stAccount.mutexAccount);
    pstCustomer->stAccount.LBalanceActive -= pstCDR->aulFee[0];
    /* ����ͳ����Ϣ */
    bs_account_stat_refresh(&pstCustomer->stAccount,
                            pstCDR->aulFee[0],
                            0,
                            pstCDR->ucServType);
    pthread_mutex_unlock(&pstCustomer->stAccount.mutexAccount);
    pstParent = pstCustomer->pstParent;
    for (i = 0; i < pstCDR->ucAgentLevel; i++)
    {
        lRebate = (S32)pstCDR->aulFee[i] - (S32)pstCDR->aulFee[i+1];
        pthread_mutex_lock(&pstParent->stAccount.mutexAccount);
        if (pstParent->ucCustomerType != BS_CUSTOMER_TYPE_TOP)
        {
            /* ֻ�д����̲��з��� */
            pstParent->stAccount.lRebate += lRebate;
            /* ����ͳ����Ϣ */
            bs_account_stat_refresh(&pstParent->stAccount,
                                    pstCDR->aulFee[i+1],
                                    lRebate,
                                    pstCDR->ucServType);
        }
        pthread_mutex_unlock(&pstParent->stAccount.mutexAccount);

        /* ���пͻ��ķ���������ڶ����ͻ� */
        pthread_mutex_lock(&g_stBssCB.pstTopCustomer->stAccount.mutexAccount);
        g_stBssCB.pstTopCustomer->stAccount.lRebate += lRebate;
        pthread_mutex_unlock(&g_stBssCB.pstTopCustomer->stAccount.mutexAccount);

        pstParent = pstParent->pstParent;
    }

    bs_trace(BS_TRACE_BILLING, LOG_LEVEL_DEBUG,
             "Porc message cdr succ, "
             "mark:%u, customer:%u, account:%u, servtype:%u, "
             "caller:%s, callee:%s, time:%u, len:%u, "
             "%s",
             pstCDR->stCDRTag.ulCDRMark, pstCDR->ulCustomerID,
             pstCDR->ulAccountID, pstCDR->ucServType,
             pstCDR->szCaller, pstCDR->szCallee,
             pstCDR->ulTimeStamp, pstCDR->ulLen,
             szFee);

    /* ����ͳ�� */
    bs_stat_message(pstCDR);

save_cdr:
    bss_send_cdr2dl(pMsgNode, BS_INTER_MSG_MESSAGE_CDR);

}

/* ���㻰������ */
VOID bss_settle_cdr_proc(DLL_NODE_S *pMsgNode)
{
    U32                     ulPackageID, ulFee;
    BS_BILLING_MATCH_ST     stBillingMatch;
    BS_CDR_SETTLE_ST        *pstCDR = NULL;
    BS_BILLING_RULE_ST      *pstRule = NULL;
    BS_BILLING_PACKAGE_ST   *pstPackage = NULL;

    /* ǰ���Ѿ��жϹ�ָ��Ϸ��� */
    pstCDR = (BS_CDR_SETTLE_ST *)pMsgNode->pHandle;

    bs_trace(BS_TRACE_BILLING, LOG_LEVEL_DEBUG,
             "Porc settle cdr, "
             "mark:%u, type:%u, caller:%s, callee:%s, "
             "start time:%u, time len:%u, servtype:%u, cause:%u, "
             "trunk:%u, peer ip:0x%X",
             pstCDR->stCDRTag.ulCDRMark, pstCDR->stCDRTag.ucCDRType,
             pstCDR->szCaller, pstCDR->szCallee,
             pstCDR->ulTimeStamp, pstCDR->ulLen,
             pstCDR->ucServType, pstCDR->usTerminateCause,
             pstCDR->usPeerTrunkID, pstCDR->aulPeerIP[0]);

    if (0 == pstCDR->ulLen)
    {
        /* �Ժ�����,��������,Ҳ�����¼ */
        return;
    }

    dos_memzero(&stBillingMatch, sizeof(stBillingMatch));
    stBillingMatch.ulCustomerID = pstCDR->ulSPID;
    stBillingMatch.ulComsumerID = pstCDR->ulSPID;
    stBillingMatch.ulTimeStamp = pstCDR->ulTimeStamp;
    dos_strncpy(stBillingMatch.szCaller, pstCDR->szCaller, sizeof(stBillingMatch.szCaller));
    dos_strncpy(stBillingMatch.szCallee, pstCDR->szCallee, sizeof(stBillingMatch.szCallee));

    ulPackageID = bs_get_settle_packageid(pstCDR->usPeerTrunkID);
    pstPackage = bs_get_billing_package(ulPackageID, BS_SERV_SETTLE);
    if (NULL == pstPackage)
    {
        /* ���Ҳ����Ʒѹ���,����ʧ�� */
        bs_trace(BS_TRACE_BILLING, LOG_LEVEL_NOTIC,
                 "Notice: can not find billing package! "
                 "package:%u, servtype:%u, CDR mark:%u",
                 ulPackageID, BS_SERV_SETTLE, pstCDR->stCDRTag.ulCDRMark);
        goto save_cdr;
    }

    pstRule = bs_match_billing_rule(pstPackage, &stBillingMatch);
    if (NULL == pstRule)
    {
        /* ƥ��ʧ�� */
        bs_trace(BS_TRACE_BILLING, LOG_LEVEL_NOTIC,
                 "Notice: match billing rule fail! "
                 "package:%u, service:%u, CDR mark:%u, "
                 "timestamp:%u, caller:%s, callee:%s, ",
                 pstPackage->ulPackageID, pstPackage->ucServType,
                 stBillingMatch.ulTimeStamp, pstCDR->stCDRTag.ulCDRMark,
                 stBillingMatch.szCaller, stBillingMatch.szCallee);

        goto save_cdr;
    }

    /* ��¼�����۷Ѷ�Ӧ�ļƷѹ��� */
    pstCDR->ulRuleID = pstRule->ulRuleID;

    if (0 == pstRule->ulBillingRate)
    {
        /* ����Ϊ0,���ü����� */
        goto save_cdr;
    }

    switch (pstRule->ucBillingType)
    {
        case BS_BILLING_BY_TIMELEN:
            ulFee = pstRule->ulBillingRate * pstRule->ucFirstBillingCnt;
            if (pstCDR->ulLen > pstRule->ulFirstBillingUnit)
            {
                /* �����׸��Ʒѵ�λ */

                U32     ulCnt;

                ulCnt = ceil((double)(pstCDR->ulLen - pstRule->ulFirstBillingUnit)/(double)pstRule->ulNextBillingUnit);
                ulFee += pstRule->ulBillingRate * pstRule->ucNextBillingCnt * ulCnt;
            }

            /* ��¼���� */
            pstCDR->ulFee = ulFee;
            break;

        case BS_BILLING_BY_COUNT:
            /* ���μƷ���Ϊ��,ֱ��ʹ�õ�λ���ʼ��� */
            pstCDR->ulFee = pstRule->ulBillingRate;
            break;

        case BS_BILLING_BY_TRAFFIC:
            /* ��δʵ�� */
            break;

        default:
            DOS_ASSERT(0);
            bs_trace(BS_TRACE_BILLING, LOG_LEVEL_WARNING,
                     "Warning: not supported billing type(%u)! ",
                     pstRule->ucBillingType);
            goto save_cdr;
    }


    /* ���㻰������۷�,�����ݿ����ü�¼���� */

    bs_trace(BS_TRACE_BILLING, LOG_LEVEL_DEBUG,
             "Porc settle cdr succ, "
             "mark:%u, trunk:%u, peer ip:0x%X, caller:%s, "
             "callee:%s, start time:%u, time len:%u, fee:%u",
             pstCDR->stCDRTag.ulCDRMark, pstCDR->usPeerTrunkID,
             pstCDR->aulPeerIP[0], pstCDR->szCaller,
             pstCDR->szCallee, pstCDR->ulTimeStamp,
             pstCDR->ulLen, pstCDR->ulFee);

save_cdr:
    bss_send_cdr2dl(pMsgNode, BS_INTER_MSG_SETTLE_CDR);

}

/* ѭ�����ڼƷ� */
VOID bss_cycle_billing(VOID)
{
    time_t                  stTime;
    struct tm               *t = NULL;

    S32                     lRebate;
    U32                     ulCnt, ulNum, ulTimeStamp;
    U32                     i, j, ulHashIndex;
    U32                     aulFee[BS_MAX_AGENT_LEVEL];
    BOOL                    bBillingOk = DOS_FALSE;
    HASH_NODE_S             *pstHashNode = NULL;
    BS_CUSTOMER_ST          *pstCustomer = NULL;
    BS_CUSTOMER_ST          *pstCurrent = NULL;
    BS_CUSTOMER_ST          *pstParent = NULL;
    BS_BILLING_PACKAGE_ST   *pstPackage = NULL;
    BS_BILLING_PACKAGE_ST   *pstParentPackage = NULL;
    BS_BILLING_RULE_ST      *pstRule = NULL;
    DLL_NODE_S              *pstMsgNode = NULL;
    BS_CDR_RENT_ST          *pstCDR = NULL;

    bs_trace(BS_TRACE_BILLING, LOG_LEVEL_DEBUG, "Start cycle billing!");

    stTime = time(NULL);
    t = localtime(&stTime);

    ulTimeStamp = (U32)stTime;
    ulCnt = 0;
    pthread_mutex_lock(&g_mutexCustomerTbl);
    HASH_Scan_Table(g_astCustomerTbl, ulHashIndex)
    {
        HASH_Scan_Bucket(g_astCustomerTbl, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            ulCnt++;
            if (ulCnt > g_astCustomerTbl->NodeNum)
            {
                DOS_ASSERT(0);
                bs_trace(BS_TRACE_BILLING, LOG_LEVEL_ERROR,
                         "Err: customer number is exceed in hash table(%u)!",
                         g_astCustomerTbl->NodeNum);
                pthread_mutex_unlock(&g_mutexCustomerTbl);
                return;
            }

            pstCustomer = (BS_CUSTOMER_ST *)pstHashNode->pHandle;
            if (DOS_ADDR_INVALID(pstCustomer))
            {
                /* �ڵ�δ����ͻ���Ϣ */
                DOS_ASSERT(0);
                continue;
            }

            if (pstCustomer->ucCustomerType != BS_CUSTOMER_TYPE_CONSUMER
                || pstCustomer->ucCustomerState != BS_CUSTOMER_STATE_ACTIVE)
            {
                /* �������������ն������߲Ż���мƷѴ��� */
                continue;
            }

            pstPackage = bs_get_billing_package(pstCustomer->stAccount.ulBillingPackageID, BS_SERV_RENT);
            if (NULL == pstPackage)
            {
                /* �������ҵ�� */
                continue;
            }

            /* �����ȼ��Ӹߵ�������ƥ��,ƥ��ɹ����˳�ƥ�� */
            for (i = 0; i < BS_MAX_BILLING_RULE_IN_PACKAGE; i++)
            {
                pstRule = &pstPackage->astRule[i];

                if (BS_BILLING_BY_CYCLE == pstRule->ucBillingType)
                {
                    /* �����ڼƷѷ�ʽ */
                    DOS_ASSERT(0);
                    continue;
                }

                if(!bs_billing_rule_is_properly(pstRule))
                {
                    /* �Ʒѹ����׵� */
                    continue;
                }

                if ((pstRule->ulExpireTimestamp != 0
                     && ulTimeStamp >= pstRule->ulExpireTimestamp)
                    || ulTimeStamp < pstRule->ulEffectTimestamp)
                {
                    /* �Ʒѹ�����δ��Ч;0��ʾ��Զ��Ч */
                    continue;
                }

                switch (pstRule->ucSrcAttrType2)
                {
                    case BS_BILLING_CYCLE_DAY:
                        /* ����ÿ��ִ��һ��,����ִ�м��� */
                        break;

                    case BS_BILLING_CYCLE_WEEK:
                        /* һ��һ��,ͳһ��ÿ����ִ�� */
                        if (t->tm_wday != 0)
                        {
                            continue;
                        }
                        break;

                    case BS_BILLING_CYCLE_MONTH:
                        /* һ��һ��,ͳһ��ÿ��1��ִ�� */
                        if (t->tm_mday != 1)
                        {
                            continue;
                        }
                        break;

                    case BS_BILLING_CYCLE_YEAR:
                        /* һ��һ��,ͳһԪ��ִ�� */
                        if (t->tm_yday != 0)
                        {
                            continue;
                        }
                        break;

                    default:
                        bs_trace(BS_TRACE_BILLING, LOG_LEVEL_ERROR,
                                 "Err: cycle type is wrong in cycle billing rule! "
                                 "Customer:%u, package:%u, type:%u",
                                 pstCustomer->ulCustomerID,
                                 pstCustomer->stAccount.ulBillingPackageID,
                                 pstRule->ucSrcAttrType2);

                        break;
                }

                /* ֻҪ�Ʒѹ�����Ч,����Ҫ���д��� */
                ulNum = 0;
                if (pstRule->ulSrcAttrValue1 != 0)
                {
                    /* ����ֵ��Ϊ0,ֱ�ӿ۷Ѽ��� */
                    ulNum = pstRule->ulSrcAttrValue1;
                }
                else
                {
                    switch (pstRule->ucSrcAttrType1)
                    {
                        case BS_BILLING_ATTR_CONCURRENT:
                        case BS_BILLING_ATTR_SET:
                            /* ���������ײ��ඨ�ڿ۷�,�޷�ͳ�� */
                            DOS_ASSERT(0);
                            bs_trace(BS_TRACE_BILLING, LOG_LEVEL_ERROR,
                                     "Err: attr value is wrong in cycle billing rule! "
                                     "Customer:%u, package:%u, attr:%u, value:%u",
                                     pstCustomer->ulCustomerID,
                                     pstCustomer->stAccount.ulBillingPackageID,
                                     pstRule->ucSrcAttrType1,
                                     pstRule->ulSrcAttrValue1);
                            /* ע��˴���break */
                            continue;

                        case BS_BILLING_ATTR_RESOURCE_AGENT:
                            ulNum = pstCustomer->ulAgentNum;
                            break;

                        case BS_BILLING_ATTR_RESOURCE_NUMBER:
                            ulNum = pstCustomer->ulNumberNum;
                            break;

                        case BS_BILLING_ATTR_RESOURCE_LINE:
                            ulNum = pstCustomer->ulUserLineNum;
                            break;

                        default:
                            DOS_ASSERT(0);
                            bs_trace(BS_TRACE_BILLING, LOG_LEVEL_ERROR,
                                     "Err: attr type is wrong in cycle billing rule! "
                                     "Customer:%u, package:%u, attr:%u",
                                     pstCustomer->ulCustomerID,
                                     pstCustomer->stAccount.ulBillingPackageID,
                                     pstRule->ucSrcAttrType1);

                            /* ע��˴���break */
                            continue;
                    }
                }

                /* ������㼶�����̵ķ��� */
                pstCurrent = pstCustomer;
                pstParent = pstCurrent->pstParent;
                aulFee[0] = ulNum * pstRule->ulBillingRate;
                for (j = 1; j < BS_MAX_AGENT_LEVEL; j++)
                {
                    if (DOS_ADDR_INVALID(pstParent))
                    {
                        /* û�и��ͻ�,Ӧ�����ڶ���;����Ӧ����������� */
                        DOS_ASSERT(0);
                        break;
                    }

                    if (BS_CUSTOMER_TYPE_TOP == pstParent->ucCustomerType)
                    {
                        /* �����ͻ�,�����ٲ� */
                        bBillingOk = DOS_TRUE;
                        break;
                    }

                    pstPackage = bs_get_billing_package(pstCurrent->stAccount.ulBillingPackageID, BS_SERV_RENT);
                    if (NULL == pstPackage)
                    {
                        /* ���Ҳ����ͻ����ʷ���Ϣ,�쳣 */
                        DOS_ASSERT(0);
                        break;
                    }

                    pstParentPackage = bs_get_billing_package(pstParent->stAccount.ulBillingPackageID, BS_SERV_RENT);
                    if (NULL == pstParentPackage)
                    {
                        /* ���Ҳ�����һ���ͻ����ʷ���Ϣ,�쳣 */
                        DOS_ASSERT(0);
                        break;
                    }

                    if (pstParentPackage->astRule[i].ulBillingRate > pstPackage->astRule[i].ulBillingRate)
                    {
                        /* �ϼ��ͻ����ʴ����¼�,�쳣 */
                        DOS_ASSERT(0);
                        bs_trace(BS_TRACE_BILLING, LOG_LEVEL_ERROR,
                                 "Err: billing rate is lower than superiors! "
                                 "customer:%u, rate:%u vs customer:%u, rate:%u",
                                 pstCurrent->ulCustomerID,
                                 pstPackage->astRule[i].ulBillingRate,
                                 pstParent->ulCustomerID,
                                 pstParentPackage->astRule[i].ulBillingRate);
                        break;
                    }

                    aulFee[j] = ulNum * pstParentPackage->astRule[i].ulBillingRate;

                    pstCurrent = pstParent;
                    pstParent = pstCurrent->pstParent;
                }

                if (!bBillingOk)
                {
                    continue;
                }

                /* ���ɻ��� */
                pstMsgNode = dos_dmem_alloc(sizeof(DLL_NODE_S));
                if (NULL == pstMsgNode)
                {
                    DOS_ASSERT(0);
                    bs_trace(BS_TRACE_CDR, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
                    continue;
                }
                DLL_Init_Node(pstMsgNode);

                pstCDR = dos_dmem_alloc(sizeof(BS_CDR_RENT_ST));
                if (NULL == pstCDR)
                {
                    DOS_ASSERT(0);
                    dos_dmem_free(pstMsgNode);
                    bs_trace(BS_TRACE_CDR, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
                    continue;
                }

                dos_memzero(pstCDR, sizeof(BS_CDR_RENT_ST));
                pstCDR->stCDRTag.ulCDRMark = 0;
                pstCDR->stCDRTag.ucCDRType = BS_CDR_RENT;

                pstCDR->ulCustomerID = pstCustomer->ulCustomerID;
                pstCDR->ulAccountID = pstCustomer->stAccount.ulAccountID;
                pstCDR->ulRuleID = pstRule->ulRuleID;
                pstCDR->ulTimeStamp = ulTimeStamp;
                pstCDR->ucAttrType = pstRule->ucSrcAttrType1;
                pstCDR->ucAgentLevel = j;
                dos_memcpy(pstCDR->aulFee, aulFee, sizeof(pstCDR->aulFee));

                pstMsgNode->pHandle = (VOID *)pstCDR;

                bs_trace(BS_TRACE_CDR, LOG_LEVEL_DEBUG,
                         "Generate rent cdr, "
                         "mark:%u, type:%u, customer:%u, account:%u, "
                         "time:%u, attrtype:%u",
                         pstCDR->stCDRTag.ulCDRMark, pstCDR->stCDRTag.ucCDRType,
                         pstCDR->ulCustomerID, pstCDR->ulAccountID,
                         pstCDR->ulTimeStamp, pstCDR->ucAttrType);

                /* �۷�&���� */
                pthread_mutex_lock(&pstCustomer->stAccount.mutexAccount);
                pstCustomer->stAccount.LBalanceActive -= pstCDR->aulFee[0];
                /* ����ͳ����Ϣ */
                bs_account_stat_refresh(&pstCustomer->stAccount,
                                        pstCDR->aulFee[0],
                                        0,
                                        BS_SERV_RENT);
                pthread_mutex_unlock(&pstCustomer->stAccount.mutexAccount);
                pstParent = pstCustomer->pstParent;
                for (j = 0; j < pstCDR->ucAgentLevel; j++)
                {
                    lRebate = (S32)pstCDR->aulFee[j] - (S32)pstCDR->aulFee[j+1];
                    pthread_mutex_lock(&pstParent->stAccount.mutexAccount);
                    if (pstParent->ucCustomerType != BS_CUSTOMER_TYPE_TOP)
                    {
                        /* ֻ�д����̲��з��� */
                        pstParent->stAccount.lRebate += lRebate;
                        /* ����ͳ����Ϣ */
                        bs_account_stat_refresh(&pstParent->stAccount,
                                                pstCDR->aulFee[j+1],
                                                lRebate,
                                                BS_SERV_RENT);
                    }
                    pthread_mutex_unlock(&pstParent->stAccount.mutexAccount);

                    /* ���пͻ��ķ���������ڶ����ͻ� */
                    pthread_mutex_lock(&g_stBssCB.pstTopCustomer->stAccount.mutexAccount);
                    g_stBssCB.pstTopCustomer->stAccount.lRebate += lRebate;
                    pthread_mutex_unlock(&g_stBssCB.pstTopCustomer->stAccount.mutexAccount);

                    pstParent = pstParent->pstParent;
                }

                bss_send_cdr2dl(pstMsgNode, BS_INTER_MSG_RENT_CDR);
            }
        }
    }
    pthread_mutex_unlock(&g_mutexCustomerTbl);

    bs_trace(BS_TRACE_BILLING, LOG_LEVEL_DEBUG, "Total proc %d cycle billing!", ulCnt);

}

/* �������ͳ����Ϣ */
VOID bss_clear_task_stat(VOID)
{
    U32     ulHashIndex;

    pthread_mutex_lock(&g_mutexTaskTbl);
    HASH_Scan_Table(g_astTaskTbl, ulHashIndex)
    {
        HASH_Bucket_FreeAll(g_astTaskTbl, ulHashIndex, bs_free_node);
    }
    pthread_mutex_unlock(&g_mutexTaskTbl);

    bs_trace(BS_TRACE_STAT, LOG_LEVEL_DEBUG, "Clear all the task statistic!");

}

/* �ͻ�ͳ�ƴ��� */
VOID bss_customer_stat_proc(VOID)
{
    U8                      ucPos;
    U32                     ulCnt;
    U32                     ulHashIndex;
    HASH_NODE_S             *pstHashNode = NULL;
    BS_CUSTOMER_ST          *pstCustomer = NULL;

    ulCnt = 0;

    /* ȷ��ͳ�Ƶ������±� */
    if (g_stBssCB.ulHour&0x1)
    {
        ucPos = 1;
    }
    else
    {
        ucPos = 0;
    }

    pthread_mutex_lock(&g_mutexCustomerTbl);
    HASH_Scan_Table(g_astCustomerTbl, ulHashIndex)
    {
        HASH_Scan_Bucket(g_astCustomerTbl, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            ulCnt++;
            if (ulCnt > g_astCustomerTbl->NodeNum)
            {
                DOS_ASSERT(0);
                bs_trace(BS_TRACE_STAT, LOG_LEVEL_ERROR,
                         "Err: customer number is exceed in hash table(%u)!",
                         g_astCustomerTbl->NodeNum);
                pthread_mutex_unlock(&g_mutexCustomerTbl);
                return;
            }

#if 0       /* �ͻ�����ܱ������̸߳ı�;���˴���ʱ�����п��ܵ��������ӻ�,�˴������������Ż� */
            /* ͳ��ʵʱ��Ҫ�󲢲�̫��,���ܳ�ʱ��������ϣ�� */
            if (0x4F == ulCnt & 0x4F)
            {
                /* ÿ64���ͻ��ͷ�һ�� */
                pthread_mutex_unlock(&g_mutexCustomerTbl);
                dos_task_delay(1);
                pthread_mutex_lock(&g_mutexCustomerTbl);
            }
#endif
            pstCustomer = (BS_CUSTOMER_ST *)pstHashNode->pHandle;
            if (DOS_ADDR_INVALID(pstCustomer))
            {
                /* �ڵ�δ����ͻ���Ϣ */
                DOS_ASSERT(0);
                continue;
            }

            pstCustomer->stStat.stStatTag.ucObjectType = BS_OBJECT_CUSTOMER;
            pstCustomer->stStat.stStatTag.ulObjectID = pstCustomer->ulCustomerID;

            if (pstCustomer->stStat.astOutBand[ucPos].ulTimeStamp != 0)
            {
                /* ͳ��ֵ��Ч,��Ҫ�洢�����ݿ� */
                bss_generate_outband_stat(&pstCustomer->stStat.stStatTag, &pstCustomer->stStat.astOutBand[ucPos]);

                /* ���ͳ��ֵ */
                dos_memzero(&pstCustomer->stStat.astOutBand[ucPos], sizeof(BS_STAT_OUTBAND));
            }

            if (pstCustomer->stStat.astInBand[ucPos].ulTimeStamp != 0)
            {
                /* ͳ��ֵ��Ч,��Ҫ�洢�����ݿ� */
                bss_generate_inband_stat(&pstCustomer->stStat.stStatTag, &pstCustomer->stStat.astInBand[ucPos]);

                /* ���ͳ��ֵ */
                dos_memzero(&pstCustomer->stStat.astInBand[ucPos], sizeof(BS_STAT_INBAND));
            }

            if (pstCustomer->stStat.astOutDialing[ucPos].ulTimeStamp != 0)
            {
                /* ͳ��ֵ��Ч,��Ҫ�洢�����ݿ� */
                bss_generate_outdialing_stat(&pstCustomer->stStat.stStatTag, &pstCustomer->stStat.astOutDialing[ucPos]);

                /* ���ͳ��ֵ */
                dos_memzero(&pstCustomer->stStat.astOutDialing[ucPos], sizeof(BS_STAT_OUTDIALING));
            }

            if (pstCustomer->stStat.astMS[ucPos].ulTimeStamp != 0)
            {
                /* ͳ��ֵ��Ч,��Ҫ�洢�����ݿ� */
                bss_generate_message_stat(&pstCustomer->stStat.stStatTag, &pstCustomer->stStat.astMS[ucPos]);

                /* ���ͳ��ֵ */
                dos_memzero(&pstCustomer->stStat.astMS[ucPos], sizeof(BS_STAT_MESSAGE));
            }
        }
    }
    pthread_mutex_unlock(&g_mutexCustomerTbl);

    bs_trace(BS_TRACE_STAT, LOG_LEVEL_DEBUG, "Total proc %u customers's stat!", ulCnt);
}


/* ��ϯͳ�ƴ��� */
VOID bss_agent_stat_proc(VOID)
{
    U8                      ucPos;
    U32                     ulCnt;
    U32                     ulHashIndex;
    HASH_NODE_S             *pstHashNode = NULL;
    BS_AGENT_ST             *pstAgent = NULL;

    ulCnt = 0;

    /* ȷ��ͳ�Ƶ������±� */
    if (g_stBssCB.ulHour&0x1)
    {
        ucPos = 1;
    }
    else
    {
        ucPos = 0;
    }

    pthread_mutex_lock(&g_mutexAgentTbl);
    HASH_Scan_Table(g_astAgentTbl, ulHashIndex)
    {
        HASH_Scan_Bucket(g_astAgentTbl, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            ulCnt++;
            if (ulCnt > g_astAgentTbl->NodeNum)
            {
                DOS_ASSERT(0);
                bs_trace(BS_TRACE_STAT, LOG_LEVEL_ERROR,
                         "Err: agent number is exceed in hash table(%u)!",
                         g_astAgentTbl->NodeNum);
                pthread_mutex_unlock(&g_mutexAgentTbl);
                return;
            }

            pstAgent = (BS_AGENT_ST *)pstHashNode->pHandle;
            if (DOS_ADDR_INVALID(pstAgent))
            {
                /* �ڵ�δ����ͻ���Ϣ */
                DOS_ASSERT(0);
                continue;
            }

            pstAgent->stStat.stStatTag.ucObjectType = BS_OBJECT_AGENT;
            pstAgent->stStat.stStatTag.ulObjectID = pstAgent->ulAgentID;

            if (pstAgent->stStat.astOutBand[ucPos].ulTimeStamp != 0)
            {
                /* ͳ��ֵ��Ч,��Ҫ�洢�����ݿ� */
                bss_generate_outband_stat(&pstAgent->stStat.stStatTag, &pstAgent->stStat.astOutBand[ucPos]);

                /* ���ͳ��ֵ */
                dos_memzero(&pstAgent->stStat.astOutBand[ucPos], sizeof(BS_STAT_OUTBAND));
            }

            if (pstAgent->stStat.astInBand[ucPos].ulTimeStamp != 0)
            {
                /* ͳ��ֵ��Ч,��Ҫ�洢�����ݿ� */
                bss_generate_inband_stat(&pstAgent->stStat.stStatTag, &pstAgent->stStat.astInBand[ucPos]);

                /* ���ͳ��ֵ */
                dos_memzero(&pstAgent->stStat.astInBand[ucPos], sizeof(BS_STAT_INBAND));
            }

            if (pstAgent->stStat.astOutDialing[ucPos].ulTimeStamp != 0)
            {
                /* ͳ��ֵ��Ч,��Ҫ�洢�����ݿ� */
                bss_generate_outdialing_stat(&pstAgent->stStat.stStatTag, &pstAgent->stStat.astOutDialing[ucPos]);

                /* ���ͳ��ֵ */
                dos_memzero(&pstAgent->stStat.astOutDialing[ucPos], sizeof(BS_STAT_OUTDIALING));
            }

            if (pstAgent->stStat.astMS[ucPos].ulTimeStamp != 0)
            {
                /* ͳ��ֵ��Ч,��Ҫ�洢�����ݿ� */
                bss_generate_message_stat(&pstAgent->stStat.stStatTag, &pstAgent->stStat.astMS[ucPos]);

                /* ���ͳ��ֵ */
                dos_memzero(&pstAgent->stStat.astMS[ucPos], sizeof(BS_STAT_MESSAGE));
            }
        }
    }
    pthread_mutex_unlock(&g_mutexAgentTbl);

    bs_trace(BS_TRACE_STAT, LOG_LEVEL_DEBUG, "Total proc %u agents's stat!", ulCnt);
}

/* �м�ͳ�ƴ��� */
VOID bss_trunk_stat_proc(VOID)
{
    U8                      ucPos;
    U32                     ulCnt;
    U32                     ulHashIndex;
    HASH_NODE_S             *pstHashNode = NULL;
    BS_SETTLE_ST            *pstSettle = NULL;

    ulCnt = 0;

    /* ȷ��ͳ�Ƶ������±� */
    if (g_stBssCB.ulHour&0x1)
    {
        ucPos = 1;
    }
    else
    {
        ucPos = 0;
    }

    pthread_mutex_lock(&g_mutexSettleTbl);
    HASH_Scan_Table(g_astSettleTbl, ulHashIndex)
    {
        HASH_Scan_Bucket(g_astSettleTbl, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            ulCnt++;
            if (ulCnt > g_astSettleTbl->NodeNum)
            {
                DOS_ASSERT(0);
                bs_trace(BS_TRACE_STAT, LOG_LEVEL_ERROR,
                         "Err: trunk number is exceed in hash table(%u)!",
                         g_astSettleTbl->NodeNum);
                pthread_mutex_unlock(&g_mutexSettleTbl);
                return;
            }

            pstSettle = (BS_SETTLE_ST *)pstHashNode->pHandle;
            if (DOS_ADDR_INVALID(pstSettle))
            {
                /* �ڵ�δ����ͻ���Ϣ */
                DOS_ASSERT(0);
                continue;
            }

            pstSettle->stStat.stStatTag.ucObjectType = BS_OBJECT_TRUNK;
            pstSettle->stStat.stStatTag.ulObjectID = (U32)pstSettle->usTrunkID;

            if (pstSettle->stStat.astOutBand[ucPos].ulTimeStamp != 0)
            {
                /* ͳ��ֵ��Ч,��Ҫ�洢�����ݿ� */
                bss_generate_outband_stat(&pstSettle->stStat.stStatTag, &pstSettle->stStat.astOutBand[ucPos]);

                /* ���ͳ��ֵ */
                dos_memzero(&pstSettle->stStat.astOutBand[ucPos], sizeof(BS_STAT_OUTBAND));
            }

            if (pstSettle->stStat.astInBand[ucPos].ulTimeStamp != 0)
            {
                /* ͳ��ֵ��Ч,��Ҫ�洢�����ݿ� */
                bss_generate_inband_stat(&pstSettle->stStat.stStatTag, &pstSettle->stStat.astInBand[ucPos]);

                /* ���ͳ��ֵ */
                dos_memzero(&pstSettle->stStat.astInBand[ucPos], sizeof(BS_STAT_INBAND));
            }

            if (pstSettle->stStat.astOutDialing[ucPos].ulTimeStamp != 0)
            {
                /* ͳ��ֵ��Ч,��Ҫ�洢�����ݿ� */
                bss_generate_outdialing_stat(&pstSettle->stStat.stStatTag, &pstSettle->stStat.astOutDialing[ucPos]);

                /* ���ͳ��ֵ */
                dos_memzero(&pstSettle->stStat.astOutDialing[ucPos], sizeof(BS_STAT_OUTDIALING));
            }

            if (pstSettle->stStat.astMS[ucPos].ulTimeStamp != 0)
            {
                /* ͳ��ֵ��Ч,��Ҫ�洢�����ݿ� */
                bss_generate_message_stat(&pstSettle->stStat.stStatTag, &pstSettle->stStat.astMS[ucPos]);

                /* ���ͳ��ֵ */
                dos_memzero(&pstSettle->stStat.astMS[ucPos], sizeof(BS_STAT_MESSAGE));
            }
        }
    }
    pthread_mutex_unlock(&g_mutexSettleTbl);

    bs_trace(BS_TRACE_STAT, LOG_LEVEL_DEBUG, "Total proc %u trunks's stat!", ulCnt);
}

/* ����ͳ�ƴ��� */
VOID bss_task_stat_proc(VOID)
{
    U8                      ucPos;
    U32                     ulCnt;
    U32                     ulHashIndex;
    HASH_NODE_S             *pstHashNode = NULL;
    BS_TASK_ST              *pstTask = NULL;

    ulCnt = 0;

    /* ȷ��ͳ�Ƶ������±� */
    if (g_stBssCB.ulHour&0x1)
    {
        ucPos = 1;
    }
    else
    {
        ucPos = 0;
    }

    pthread_mutex_lock(&g_mutexTaskTbl);
    HASH_Scan_Table(g_astTaskTbl, ulHashIndex)
    {
        HASH_Scan_Bucket(g_astTaskTbl, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            ulCnt++;
            if (ulCnt > g_astTaskTbl->NodeNum)
            {
                DOS_ASSERT(0);
                bs_trace(BS_TRACE_STAT, LOG_LEVEL_ERROR,
                         "Err: task number is exceed in hash table(%u)!",
                         g_astTaskTbl->NodeNum);
                pthread_mutex_unlock(&g_mutexTaskTbl);
                return;
            }

            pstTask = (BS_TASK_ST *)pstHashNode->pHandle;
            if (DOS_ADDR_INVALID(pstTask))
            {
                /* �ڵ�δ����ͻ���Ϣ */
                DOS_ASSERT(0);
                continue;
            }

            pstTask->stStat.stStatTag.ucObjectType = BS_OBJECT_TASK;
            pstTask->stStat.stStatTag.ulObjectID = pstTask->ulTaskID;

            if (pstTask->stStat.astOutBand[ucPos].ulTimeStamp != 0)
            {
                /* ͳ��ֵ��Ч,��Ҫ�洢�����ݿ� */
                bss_generate_outband_stat(&pstTask->stStat.stStatTag, &pstTask->stStat.astOutBand[ucPos]);

                /* ���ͳ��ֵ */
                dos_memzero(&pstTask->stStat.astOutBand[ucPos], sizeof(BS_STAT_OUTBAND));
            }

            if (pstTask->stStat.astInBand[ucPos].ulTimeStamp != 0)
            {
                /* ͳ��ֵ��Ч,��Ҫ�洢�����ݿ� */
                bss_generate_inband_stat(&pstTask->stStat.stStatTag, &pstTask->stStat.astInBand[ucPos]);

                /* ���ͳ��ֵ */
                dos_memzero(&pstTask->stStat.astInBand[ucPos], sizeof(BS_STAT_INBAND));
            }

            if (pstTask->stStat.astOutDialing[ucPos].ulTimeStamp != 0)
            {
                /* ͳ��ֵ��Ч,��Ҫ�洢�����ݿ� */
                bss_generate_outdialing_stat(&pstTask->stStat.stStatTag, &pstTask->stStat.astOutDialing[ucPos]);

                /* ���ͳ��ֵ */
                dos_memzero(&pstTask->stStat.astOutDialing[ucPos], sizeof(BS_STAT_OUTDIALING));
            }

            if (pstTask->stStat.astMS[ucPos].ulTimeStamp != 0)
            {
                /* ͳ��ֵ��Ч,��Ҫ�洢�����ݿ� */
                bss_generate_message_stat(&pstTask->stStat.stStatTag, &pstTask->stStat.astMS[ucPos]);

                /* ���ͳ��ֵ */
                dos_memzero(&pstTask->stStat.astMS[ucPos], sizeof(BS_STAT_MESSAGE));
            }
        }
    }
    pthread_mutex_unlock(&g_mutexTaskTbl);

    bs_trace(BS_TRACE_STAT, LOG_LEVEL_DEBUG, "Total proc %u tasks's stat!", ulCnt);
}

/* ����ͳ�ƴ��� */
VOID bss_account_stat_proc(VOID)
{
    U8                      ucPos;
    U32                     ulCnt;
    U32                     ulHashIndex;
    HASH_NODE_S             *pstHashNode = NULL;
    BS_CUSTOMER_ST          *pstCustomer = NULL;
    BS_ACCOUNT_ST           *pstAccount = NULL;

    ulCnt = 0;

    /* ȷ��ͳ�Ƶ������±� */
    if (g_stBssCB.ulHour&0x1)
    {
        ucPos = 1;
    }
    else
    {
        ucPos = 0;
    }

    pthread_mutex_lock(&g_mutexCustomerTbl);
    HASH_Scan_Table(g_astCustomerTbl, ulHashIndex)
    {
        HASH_Scan_Bucket(g_astCustomerTbl, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            ulCnt++;
            if (ulCnt > g_astCustomerTbl->NodeNum)
            {
                DOS_ASSERT(0);
                bs_trace(BS_TRACE_STAT, LOG_LEVEL_ERROR,
                         "Err: customer number is exceed in hash table(%u)!",
                         g_astCustomerTbl->NodeNum);
                pthread_mutex_unlock(&g_mutexCustomerTbl);
                return;
            }

#if 0       /* �ͻ�����ܱ������̸߳ı�;���˴���ʱ�����п��ܵ��������ӻ�,�˴������������Ż� */
            /* ͳ��ʵʱ��Ҫ�󲢲�̫��,���ܳ�ʱ��������ϣ�� */
            if (0x4F == ulCnt & 0x4F)
            {
                /* ÿ64���ͻ��ͷ�һ�� */
                pthread_mutex_unlock(&g_mutexCustomerTbl);
                dos_task_delay(1);
                pthread_mutex_lock(&g_mutexCustomerTbl);
            }
#endif
            pstCustomer = (BS_CUSTOMER_ST *)pstHashNode->pHandle;
            if (DOS_ADDR_INVALID(pstCustomer))
            {
                /* �ڵ�δ����ͻ���Ϣ */
                DOS_ASSERT(0);
                continue;
            }

            pstAccount = &pstCustomer->stAccount;
            pthread_mutex_lock(&pstAccount->mutexAccount);
            pstAccount->stStat.stStatTag.ucObjectType = BS_OBJECT_ACCOUNT;
            pstAccount->stStat.stStatTag.ulObjectID = pstAccount->ulAccountID;

            if (pstAccount->stStat.ulTimeStamp != 0)
            {
                /* ͳ��ֵ��Ч,��Ҫ�洢�����ݿ� */
                bss_generate_account_stat(&pstAccount->stStat);

                /* ���ͳ��ֵ */
                dos_memzero(&pstAccount->stStat, sizeof(pstAccount->stStat));
            }
            pthread_mutex_unlock(&pstAccount->mutexAccount);
        }
    }
    pthread_mutex_unlock(&g_mutexCustomerTbl);

    bs_trace(BS_TRACE_STAT, LOG_LEVEL_DEBUG, "Total proc %u customers's stat!", ulCnt);
}

/* ѭ�����ڴ���,ÿ������1�� */
VOID bss_day_cycle_proc(U64 uLParam)
{
    dos_tmr_start(&g_stBssCB.hDayCycleTmr,
                  (24 * 3600 * 1000),
                  bss_day_cycle_proc,
                  0,
                  TIMER_NORMAL_NO_LOOP);

    g_stBssCB.bIsDayCycle = DOS_TRUE;
}

/* ѭ�����ڴ���,ÿ������1�� */
VOID bss_hour_cycle_proc(U64 uLParam)
{
    dos_tmr_start(&g_stBssCB.hHourCycleTmr,
                  (3600 * 1000),
                  bss_hour_cycle_proc,
                  0,
                  TIMER_NORMAL_NO_LOOP);

    g_stBssCB.bIsHourCycle = DOS_TRUE;

}

/* ��֤��Ϣ���� */
VOID *bss_aaa(VOID *arg)
{
    DLL_NODE_S      *pMsgNode;
    BS_MSG_AUTH     *pstMsg;
    struct timespec stTimeout;

    while (1)
    {
        pMsgNode = NULL;

        /* ��ȡ��Ϣ���е�һ������ */
        pthread_mutex_lock(&g_mutexBSAAAMsg);
        stTimeout.tv_sec = time(0) + 1;
        stTimeout.tv_nsec = 0;
        pthread_cond_timedwait(&g_condBSAAAList, &g_mutexBSAAAMsg, &stTimeout);
        while (1)
        {
            if (DLL_Count(&g_stBSAAAMsgList) <= 0)
            {
                break;
            }

            pMsgNode = dll_fetch(&g_stBSAAAMsgList);
            if (NULL == pMsgNode)
            {
                /* ������û����Ϣ */
                continue;
            }

            pstMsg = (BS_MSG_AUTH *)pMsgNode->pHandle;
            if (DOS_ADDR_INVALID(pstMsg))
            {
                DOS_ASSERT(0);
                dos_dmem_free(pMsgNode);
                continue;
            }

            switch (pstMsg->stMsgTag.ucMsgType)
            {
                case BS_MSG_BALANCE_QUERY_REQ:
                    bss_query_balance(pMsgNode);
                    break;

                case BS_MSG_USER_AUTH_REQ:
                    bss_user_auth(pMsgNode);
                    break;

                case BS_MSG_ACCOUNT_AUTH_REQ:
                    bss_account_auth(pMsgNode);
                    break;

                default:
                    DOS_ASSERT(0);
                    bs_trace(BS_TRACE_FS, LOG_LEVEL_ERROR, "Err: unexpected msg, type:%u",
                             pstMsg->stMsgTag.ucMsgType);
                    /* δ֪��Ϣ,��������,�ͷ��ڴ� */
                    dos_dmem_free(pMsgNode->pHandle);
                    pMsgNode->pHandle = NULL;
                    dos_dmem_free(pMsgNode);
                    break;
            }
        }
        pthread_mutex_unlock(&g_mutexBSAAAMsg);
    }

    return NULL;

}

/* ԭʼ�������� */
VOID *bss_cdr(VOID *arg)
{
    DLL_NODE_S      *pMsgNode;
    BS_MSG_CDR      *pstMsg;
    struct timespec stTimeout;

    while (1)
    {
        pMsgNode = NULL;

        /* ��ȡ��Ϣ���е�һ������ */
        pthread_mutex_lock(&g_mutexBSCDR);
        stTimeout.tv_sec = time(0) + 1;
        stTimeout.tv_nsec = 0;
        pthread_cond_timedwait(&g_condBSCDRList, &g_mutexBSCDR, &stTimeout);

        while (1)
        {
            if (DLL_Count(&g_stBSCDRList) <= 0)
            {
                break;
            }

            pMsgNode = dll_fetch(&g_stBSCDRList);
            if (NULL == pMsgNode)
            {
                /* ������û����Ϣ */
                continue;
            }

            pstMsg = (BS_MSG_CDR *)pMsgNode->pHandle;
            if (DOS_ADDR_INVALID(pstMsg))
            {
                DOS_ASSERT(0);
                dos_dmem_free(pMsgNode);
                continue;
            }

            switch (pstMsg->stMsgTag.ucMsgType)
            {
                case BS_MSG_BILLING_START_REQ:
                    bss_billing_start(pMsgNode);
                    break;

                case BS_MSG_BILLING_UPDATE_REQ:
                    bss_billing_update(pMsgNode);
                    break;

                case BS_MSG_BILLING_STOP_REQ:
                    bss_billing_stop(pMsgNode);
                    break;

                default:
                    DOS_ASSERT(0);
                    bs_trace(BS_TRACE_FS, LOG_LEVEL_ERROR, "Err: unexpected msg, type:%u",
                             pstMsg->stMsgTag.ucMsgType);
                    /* δ֪��Ϣ,��������,�ͷ��ڴ� */
                    dos_dmem_free(pMsgNode->pHandle);
                    pMsgNode->pHandle = NULL;
                    dos_dmem_free(pMsgNode);
                    break;
            }
        }

        pthread_mutex_unlock(&g_mutexBSCDR);
    }

    return NULL;

}

/* �ƷѴ��� */
VOID *bss_billing(VOID *arg)
{
    DLL_NODE_S      *pMsgNode;
    BS_CDR_TAG      *pstMsgTag;
    struct timespec stTimeout;

    while (1)
    {
        pMsgNode = NULL;

        /* ��ȡ��Ϣ���е�һ������ */
        pthread_mutex_lock(&g_mutexBSBilling);
        stTimeout.tv_sec = time(0) + 1;
        stTimeout.tv_nsec = 0;
        pthread_cond_timedwait(&g_condBSBillingList, &g_mutexBSBilling, &stTimeout);

        while (1)
        {
            if (DLL_Count(&g_stBSBillingList) <= 0)
            {
                break;
            }

            pMsgNode = dll_fetch(&g_stBSBillingList);
            if (NULL == pMsgNode)
            {
                /* ������û����Ϣ */
                continue;
            }

            pstMsgTag = (BS_CDR_TAG *)pMsgNode->pHandle;
            if (DOS_ADDR_INVALID(pstMsgTag))
            {
                DOS_ASSERT(0);
                dos_dmem_free(pMsgNode);
                continue;
            }

            switch (pstMsgTag->ucCDRType)
            {
                case BS_CDR_VOICE:
                    bss_voice_cdr_proc(pMsgNode);

                    break;

                case BS_CDR_RECORDING:
                    bss_recording_cdr_proc(pMsgNode);
                    break;

                case BS_CDR_MESSAGE:
                    bss_message_cdr_proc(pMsgNode);
                    break;

                case BS_CDR_SETTLE:
                    bss_settle_cdr_proc(pMsgNode);
                    break;

                default:
                    DOS_ASSERT(0);
                    bs_trace(BS_TRACE_BILLING, LOG_LEVEL_ERROR, "Err: unexpected cdr, type:%u",
                             pstMsgTag->ucCDRType);
                    /* δ֪��Ϣ,��������,�ͷ��ڴ� */
                    dos_dmem_free(pMsgNode->pHandle);
                    pMsgNode->pHandle = NULL;
                    dos_dmem_free(pMsgNode);
                    break;
            }
        }

        pthread_mutex_unlock(&g_mutexBSBilling);
    }

    return NULL;

}

/* ������ */
VOID *bss_accounting(VOID *arg)
{
    U8                      ucOperateType;
    S32                     lMoney;
    U32                     ulOperateDir;
    U32                     ulTimeStamp, ulPeeAccount;
    U32                     ulCnt, ulHashIndex;
    S8                      szTimeStamp[32];
    HASH_NODE_S             *pstHashNode = NULL;
    BS_CUSTOMER_ST          *pstCustomer = NULL;
    BS_ACCOUNT_ST           *pstAccount = NULL;
    BS_CDR_ACCOUNT_ST       *pstCDR = NULL;
    DLL_NODE_S              *pstMsgNode = NULL;

    while (1)
    {
        if (g_stBssCB.bIsBillDay)
        {
            /* ��Ҫ�����˵� */
            bs_trace(BS_TRACE_ACCOUNT, LOG_LEVEL_DEBUG, "Start generating monthly bill!");

            //TODO:���˵�Ҳ���Կ�����WEB��̨�������
        }

        /* ÿ��һ��ʱ���һ���� */
        dos_task_delay(BS_ACCOUNTING_INTERVAL * 1000);

        ulCnt = 0;
        ulTimeStamp = (U32)time(NULL);
        ulPeeAccount = 0;

        pthread_mutex_lock(&g_mutexCustomerTbl);
        HASH_Scan_Table(g_astCustomerTbl, ulHashIndex)
        {
            HASH_Scan_Bucket(g_astCustomerTbl, ulHashIndex, pstHashNode, HASH_NODE_S *)
            {
                ulCnt++;
                if (ulCnt > g_astCustomerTbl->NodeNum)
                {
                    DOS_ASSERT(0);
                    bs_trace(BS_TRACE_BILLING, LOG_LEVEL_ERROR,
                             "Err: customer number is exceed in hash table(%u)!",
                             g_astCustomerTbl->NodeNum);
                    pthread_mutex_unlock(&g_mutexCustomerTbl);
                    goto loop_finish;
                }

#if 0       /* �ͻ�����ܱ������̸߳ı�;���˴���ʱ�����п��ܵ��������ӻ�,�˴������������Ż� */
                /* ����ʵʱ��Ҫ�󲢲�̫��,���ܳ�ʱ��������ϣ�� */
                if (0x4F == ulCnt & 0x4F)
                {
                    /* ÿ64���ͻ������ͷ�һ�� */
                    pthread_mutex_unlock(&g_mutexCustomerTbl);
                    dos_task_delay(1);
                    pthread_mutex_lock(&g_mutexCustomerTbl);
                }
#endif
                pstCustomer = (BS_CUSTOMER_ST *)pstHashNode->pHandle;
                if (DOS_ADDR_INVALID(pstCustomer))
                {
                    /* �ڵ�δ����ͻ���Ϣ */
                    DOS_ASSERT(0);
                    continue;
                }

                pstAccount = &pstCustomer->stAccount;
                pthread_mutex_lock(&pstAccount->mutexAccount);

                lMoney = (U32)(pstAccount->LBalance - pstAccount->LBalanceActive);
                if (lMoney != 0)
                {
                    /* ���ɿ۷������񻰵� */
                    pstMsgNode = dos_dmem_alloc(sizeof(DLL_NODE_S));
                    if (NULL == pstMsgNode)
                    {
                        DOS_ASSERT(0);
                        bs_trace(BS_TRACE_ACCOUNT, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
                        pthread_mutex_unlock(&pstAccount->mutexAccount);
                        continue;
                    }
                    DLL_Init_Node(pstMsgNode);

                    pstCDR = dos_dmem_alloc(sizeof(BS_CDR_ACCOUNT_ST));
                    if (NULL == pstCDR)
                    {
                        DOS_ASSERT(0);
                        dos_dmem_free(pstMsgNode);
                        bs_trace(BS_TRACE_ACCOUNT, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
                        pthread_mutex_unlock(&pstAccount->mutexAccount);
                        continue;
                    }

                    dos_memzero(pstCDR, sizeof(BS_CDR_ACCOUNT_ST));

                    dos_snprintf(szTimeStamp, sizeof(szTimeStamp), "Last:%u",pstAccount->ulAccountingTime);
                    szTimeStamp[sizeof(szTimeStamp) - 1] = '\0';

                    pstCDR->stCDRTag.ulCDRMark = 0;
                    pstCDR->stCDRTag.ucCDRType = BS_CDR_ACCOUNT;
                    pstCDR->ulCustomerID = pstCustomer->ulCustomerID;
                    pstCDR->ulAccountID = pstAccount->ulAccountID;
                    pstCDR->ucOperateType = BS_ACCOUNT_DEDUCTION;
                    pstCDR->lMoney = lMoney;
                    pstCDR->LBalance = pstAccount->LBalanceActive;
                    pstCDR->ulPeeAccount = ulPeeAccount;
                    pstCDR->ulTimeStamp = ulTimeStamp;
                    pstCDR->ulOperatorID = BS_SYS_OPERATOR_ID;
                    pstCDR->ulOperateDir = BS_ACCOUNT_PAY;
                    dos_strncpy(pstCDR->szRemark, szTimeStamp, sizeof(pstCDR->szRemark));

                    pstMsgNode->pHandle = (VOID *)pstCDR;

                    bs_trace(BS_TRACE_CDR, LOG_LEVEL_DEBUG,
                             "Generate accounting cdr, "
                             "mark:%u, type:%u, customer:%u, account:%u, "
                             "operate type:%u, money:%d, balance:%ld, peer account:%u, "
                             "time:%u, operator:%u, remark:%s",
                             pstCDR->stCDRTag.ulCDRMark, pstCDR->stCDRTag.ucCDRType,
                             pstCDR->ulCustomerID, pstCDR->ulAccountID,
                             pstCDR->ucOperateType, pstCDR->lMoney,
                             pstCDR->LBalance, pstCDR->ulPeeAccount,
                             pstCDR->ulTimeStamp, pstCDR->ulOperatorID,
                             pstCDR->szRemark);

                    /* �����˻���Ϣ */
                    bs_trace(BS_TRACE_ACCOUNT, LOG_LEVEL_DEBUG,
                             "Refresh account, old: customer:%u, account:%u, "
                             "balance:%ld, active balance:%ld, rebate:%u, accounting time:%u",
                             pstAccount->ulCustomerID, pstAccount->ulAccountID,
                             pstAccount->LBalance, pstAccount->LBalanceActive,
                             pstAccount->lRebate, pstAccount->ulAccountingTime);
                    pstAccount->LBalance = pstAccount->LBalanceActive;
                    pstAccount->ulAccountingTime = ulTimeStamp;
                    bs_trace(BS_TRACE_ACCOUNT, LOG_LEVEL_DEBUG,
                             "Refresh account, new: customer:%u, account:%u, "
                             "balance:%ld, active balance:%ld, rebate:%u, accounting time:%u",
                             pstAccount->ulCustomerID, pstAccount->ulAccountID,
                             pstAccount->LBalance, pstAccount->LBalanceActive,
                             pstAccount->lRebate, pstAccount->ulAccountingTime);

                    /* ���ͻ��������ݲ�,�����ݲ�洢�����ݿ��� */
                    bss_send_cdr2dl(pstMsgNode, BS_INTER_MSG_ACCOUNT_CDR);

                }

                if (pstAccount->lRebate != 0
                    && BS_CUSTOMER_TYPE_TOP != pstCustomer->ucCustomerType)
                {
                    /* ���ɷ��������񻰵� */
                    pstMsgNode = dos_dmem_alloc(sizeof(DLL_NODE_S));
                    if (NULL == pstMsgNode)
                    {
                        DOS_ASSERT(0);
                        bs_trace(BS_TRACE_ACCOUNT, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
                        pthread_mutex_unlock(&pstAccount->mutexAccount);
                        continue;
                    }
                    DLL_Init_Node(pstMsgNode);

                    pstCDR = dos_dmem_alloc(sizeof(BS_CDR_ACCOUNT_ST));
                    if (NULL == pstCDR)
                    {
                        DOS_ASSERT(0);
                        dos_dmem_free(pstMsgNode);
                        bs_trace(BS_TRACE_ACCOUNT, LOG_LEVEL_ERROR, "ERR: alloc memory fail!");
                        pthread_mutex_unlock(&pstAccount->mutexAccount);
                        continue;
                    }

                    dos_memzero(pstCDR, sizeof(BS_CDR_ACCOUNT_ST));

                    dos_snprintf(szTimeStamp, sizeof(szTimeStamp), "Last:%u",pstAccount->ulAccountingTime);
                    szTimeStamp[sizeof(szTimeStamp) - 1] = '\0';

                    if (BS_CUSTOMER_TYPE_TOP == pstCustomer->ucCustomerType)
                    {
                        /* �����ͻ��Ƿ����֧���� */
                        ulOperateDir = BS_ACCOUNT_PAY;
                        ucOperateType = BS_ACCOUNT_REBATE_PAY;
                        ulPeeAccount = 0;
                    }
                    else
                    {
                        /* �������Ƿ���Ļ�÷� */
                        ucOperateType = BS_ACCOUNT_REBATE_GET;
                        ulOperateDir = BS_ACCOUNT_GET;
                        ulPeeAccount = g_stBssCB.pstTopCustomer->stAccount.ulAccountID;
                    }

                    pstCDR->stCDRTag.ulCDRMark = 0;
                    pstCDR->stCDRTag.ucCDRType = BS_CDR_ACCOUNT;
                    pstCDR->ulCustomerID = pstCustomer->ulCustomerID;
                    pstCDR->ulAccountID = pstAccount->ulAccountID;
                    pstCDR->ucOperateType = ucOperateType;
                    pstCDR->lMoney = pstAccount->lRebate;
                    pstCDR->LBalance = pstAccount->LBalanceActive;
                    pstCDR->ulPeeAccount = 0;
                    pstCDR->ulTimeStamp = ulTimeStamp;
                    pstCDR->ulOperatorID = BS_SYS_OPERATOR_ID;
                    pstCDR->ulOperateDir = ulOperateDir;
                    dos_strncpy(pstCDR->szRemark, szTimeStamp, sizeof(pstCDR->szRemark));

                    pstMsgNode->pHandle = (VOID *)pstCDR;

                    bs_trace(BS_TRACE_CDR, LOG_LEVEL_DEBUG,
                             "Generate accounting cdr, "
                             "mark:%u, type:%u, customer:%u, account:%u, "
                             "operate type:%u, money:%d, balance:%d, peer account:%u, "
                             "time:%u, operator:%u, remark:%s",
                             pstCDR->stCDRTag.ulCDRMark, pstCDR->stCDRTag.ucCDRType,
                             pstCDR->ulCustomerID, pstCDR->ulAccountID,
                             pstCDR->ucOperateType, pstCDR->lMoney,
                             pstCDR->LBalance, pstCDR->ulPeeAccount,
                             pstCDR->ulTimeStamp, pstCDR->ulOperatorID,
                             pstCDR->szRemark);

                    /* �����˻���Ϣ */
                    bs_trace(BS_TRACE_ACCOUNT, LOG_LEVEL_DEBUG,
                             "Refresh account, old: customer:%u, account:%u, "
                             "balance:%d, active balance:%d, rebate:%u, accounting time:%u",
                             pstAccount->ulCustomerID, pstAccount->ulAccountID,
                             pstAccount->LBalance, pstAccount->LBalanceActive,
                             pstAccount->lRebate, pstAccount->ulAccountingTime);
                    pstAccount->LBalanceActive += pstAccount->lRebate;
                    pstAccount->LBalance += pstAccount->lRebate;
                    pstAccount->lRebate = 0;
                    pstAccount->ulAccountingTime = ulTimeStamp;
                    bs_trace(BS_TRACE_ACCOUNT, LOG_LEVEL_DEBUG,
                             "Refresh account, new: customer:%u, account:%u, "
                             "balance:%d, active balance:%d, rebate:%u, accounting time:%u",
                             pstAccount->ulCustomerID, pstAccount->ulAccountID,
                             pstAccount->LBalance, pstAccount->LBalanceActive,
                             pstAccount->lRebate, pstAccount->ulAccountingTime);


                    /* ���ͻ��������ݲ�,�����ݲ�洢�����ݿ��� */
                    bss_send_cdr2dl(pstMsgNode, BS_INTER_MSG_ACCOUNT_CDR);

                }

                pthread_mutex_unlock(&pstAccount->mutexAccount);
            }
        }
        pthread_mutex_unlock(&g_mutexCustomerTbl);

loop_finish:
        g_stBssCB.bIsBillDay = DOS_FALSE;

        bs_trace(BS_TRACE_BILLING, LOG_LEVEL_DEBUG, "Total proc %u cycle accounting!", ulCnt);
    }

    return NULL;

}

/*
ͳ�ƴ���
˵��:
1.BSϵͳ��Ҫ�Կͻ���Ӫ����Ӫ���������Ϣ����ͳ��,�����뵽�ڴ����ݿ���,
  ����ͻ�ͨ��ǰ̨��ȡ��̬ҵ������;�ڴ����ݿ���ֻ���浱���ͳ������;
2.ͨ��ҵ����ص�ʵʱͳ��,�ڲ�������ʱ����;
3.�˻���ͳ������,�ڳ���ʱ����ͳ��;
4.ͳ�����ڳ���ȷ��Ϊ1Сʱ,��ÿСʱ��ͳ�����ݴ洢�����ݿ���;
5.��Ϊ����������ܲ���ʱ,��ϵͳ����,����ͳ��������ͳ�����ݲ�׼ȷ,
  ���,����ʵʱͳ������,�޷���֤�ڴ����ݿ���ͳ�����ݵ�׼ȷ��;
6.WEB��̨��ÿ���ҵ��ͷ���,���ݻ�������ͳ��ÿСʱ����(׼ȷ����)�����浽�����,
  �����ڿͻ�WEBǰ̨չʾ;
*/
VOID *bss_stat(VOID *arg)
{
    time_t          stTime;
    struct tm       *t = NULL;

    while (1)
    {
        /* ע��:�˴����ڼƷ�������˴���,��������Ƚ϶���,���ŵ��Ʒ��߳�Ҳ������:
           ��Ϊ�Ʒ��߳��������߳��л���������������,�п��ܴ��ڳ�ʱ������״̬;
           Ҳ����������һ���̴߳���,���ڼ򵥿���,���ڴ˴����� */
        if (g_stBssCB.bIsDayCycle)
        {
            stTime = time(NULL);
            t = localtime(&stTime);

            if (1 == t->tm_mday)
            {
                /* ÿ��1�����˵��� */
                g_stBssCB.bIsBillDay = DOS_TRUE;
            }

            /* ѭ���ƷѴ��� */
            bss_cycle_billing();

            /* �������ͳ����Ϣ */
            bss_clear_task_stat();

            g_stBssCB.bIsDayCycle = DOS_FALSE;
        }

        if (!g_stBssCB.bIsHourCycle)
        {
            /* ͳ���������ȼ�����,�ͷ�ϵͳ��Դ */
            dos_task_delay(1000);
            continue;
        }
        g_stBssCB.bIsHourCycle = DOS_FALSE;

        /* ÿСʱ�洢һ��ҵ��ͳ������ */
        bss_customer_stat_proc();
        bss_agent_stat_proc();
        bss_trunk_stat_proc();
        bss_task_stat_proc();

        /* ÿ��洢1���˻�����ͳ������ */
        if (23 == g_stBssCB.ulHour)
        {
            bss_account_stat_proc();
        }

        /* �������ͳ�ƻ��� */
        g_stBssCB.ulStatHourBase += 3600;
        g_stBssCB.ulHour++;
        if (24 == g_stBssCB.ulHour)
        {
            g_stBssCB.ulHour = 0;
            g_stBssCB.ulStatDayBase += (3600 * 24);
        }
    }

    return NULL;

}

/* ��� */
VOID *bss_audit(VOID *arg)
{
    while (1)
    {
        //TODO
        dos_task_delay(BS_AUDIT_INTERVAL * 1000);
    }

    return NULL;
}


#ifdef __cplusplus
}
#endif /* __cplusplus */


