/**
 * @file : sc_acd.c
 *
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 * 坐席管理
 *
 * @date: 2016年1月14日
 * @arthur: Larry
 *
 * @note 坐席管理在内存中的结构
 * 1. 由g_pstAgentList hash表维护所有的坐席
 * 2. 由g_pstGroupList 维护所有的组，同时在组里面维护每个组成员的hash表
 * 3. 组成员的Hash表中有指向g_pstSiteList中坐席成员的指针
 *  g_pstAgentList
 *    | -- table --- buget1 --- agnet1 --- agnet2 --- agnet3 ...
 *           |   --- buget2 --- agnet4 --- agnet5 --- agnet6 ...
 *
 *  g_pstGroupList
 *    group table --- buget1 --- group1(agnet table1)  --- agent list(DLL)
 *            |                  |
 *            |                  group2(agnet table1)  --- agent list(DLL)
 *            |                  |
 *            |                  ...
 *            |
 *            |   --- buget2 --- group3(agnet table1)  --- agent list(DLL)
 *            |                  |
 *            |                  ...
 *            |
 *            |                  group4(agnet table1)  --- agent list(DLL)
 *            |                  |
 *            |                  ...
 *            ...
 *  其中g_pstGroupList table中所有的agnet使用指向g_pstAgentList中某一个坐席,
 *  同一个坐席可能属于多个坐席组，所以可能g_pstGroupList中有多个坐席指向g_pstAgentList中同一个节点，
 *  所以删除时不能直接delete
 */

#include <dos.h>
#include "sc_def.h"
#include "sc_res.h"
#include "sc_debug.h"
#include "sc_publish.h"
#include "sc_db.h"
#include "bs_pub.h"
#include "sc_pub.h"

extern DB_HANDLE_ST         *g_pstSCDBHandle;

pthread_t          g_pthStatusTask;

/* 坐席组的hash表 */
HASH_TABLE_S      *g_pstAgentList      = NULL;
pthread_mutex_t   g_mutexAgentList     = PTHREAD_MUTEX_INITIALIZER;

HASH_TABLE_S      *g_pstGroupList      = NULL;
pthread_mutex_t   g_mutexGroupList     = PTHREAD_MUTEX_INITIALIZER;

/* 坐席组个数 */
U32               g_ulGroupCount       = 0;

U32 sc_agent_get_channel_id(SC_AGENT_INFO_ST *pstAgentInfo, S8 *pszChannel, U32 ulLen)
{
    if (DOS_ADDR_INVALID(pstAgentInfo))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (DOS_ADDR_INVALID(pszChannel) || 0 == ulLen)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    dos_snprintf(pszChannel, ulLen, "%u_%s"
                , pstAgentInfo->ulAgentID
                , pstAgentInfo->szEmpNo);

    return DOS_SUCC;
}

U32 sc_agent_status_http_get_rsp(U32 ulAgentID)
{
    SC_AGENT_NODE_ST   *pstAgentNode = NULL;
    SC_AGENT_INFO_ST   *pstAgentInfo = NULL;
    S8 szJson[256]        = { 0, };
    S8 szURL[256]         = { 0, };
    S8 szChannel[128]     = { 0, };
    S32 ulPAPIPort = -1;

    /* 找到坐席 */
    pstAgentNode = sc_agent_get_by_id(ulAgentID);
    if (DOS_ADDR_INVALID(pstAgentNode)
        || DOS_ADDR_INVALID(pstAgentNode->pstAgentInfo))
    {
        return DOS_FAIL;
    }

    pstAgentInfo = pstAgentNode->pstAgentInfo;
    if (sc_agent_get_channel_id(pstAgentInfo, szChannel, sizeof(szChannel)) != DOS_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Get channel ID fail for agent: %u", pstAgentInfo->ulAgentID);
        return DOS_FAIL;
    }

    ulPAPIPort = config_hb_get_papi_port();
    if (ulPAPIPort <= 0)
    {
        dos_snprintf(szURL, sizeof(szURL), "http://localhost/pub?id=%s", szChannel);
    }
    else
    {
        dos_snprintf(szURL, sizeof(szURL), "http://localhost:%d/pub?id=%s", ulPAPIPort, szChannel);
    }

    dos_snprintf(szJson, sizeof(szJson)
                    , "{\\\"type\\\":\\\"%u\\\",\\\"body\\\":{\\\"type\\\":\\\"%d\\\",\\\"work_status\\\":\\\"%u\\\",\\\"is_signin\\\":\\\"%u\\\"}}"
                    , ACD_MSG_TYPE_QUERY
                    , ACD_MSG_TYPE_QUERY_STATUS
                    , pstAgentInfo->ucStatus
                    , pstAgentInfo->bConnected ? 1 : 0);


    return sc_pub_send_msg(szURL, szJson, SC_PUB_TYPE_STATUS, NULL);
}

U32 sc_agent_update_status_db(U32 ulSiteID, U32 ulStatus, BOOL bIsConnect)
{
    S8  szQuery[512] = { 0, };
    U32 ulStatusDB = 0;
    SC_DB_MSG_TAG_ST *pstMsg = NULL;

    /* 写数据库的状态
        0  -- 离线
        1  -- 可用
        2  -- 不可用
        3  -- 长签可用
        4  =- 长签不可用
    */
    ulStatusDB = ulStatus > SC_ACD_IDEL ? 2 : ulStatus;
    if (bIsConnect)
    {
        /* 如果是长签, 则把状态修改为长签可用或者长签不可用 */
        ulStatusDB += 2;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Update agent(%d) status(%d)", ulSiteID, ulStatus);

    dos_snprintf(szQuery, sizeof(szQuery), "UPDATE tbl_agent SET status=%d WHERE id = %u;", ulStatusDB, ulSiteID);

    pstMsg = (SC_DB_MSG_TAG_ST *)dos_dmem_alloc(sizeof(SC_DB_MSG_TAG_ST));
    if (DOS_ADDR_INVALID(pstMsg))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }
    pstMsg->ulMsgType = SC_MSG_SAVE_AGENT_STATUS;
    pstMsg->szData = dos_dmem_alloc(dos_strlen(szQuery) + 1);
    if (DOS_ADDR_INVALID(pstMsg->szData))
    {
        DOS_ASSERT(0);
        dos_dmem_free(pstMsg->szData);

        return DOS_FAIL;
    }

    dos_strcpy(pstMsg->szData, szQuery);

    return sc_send_msg2db(pstMsg);
}

U32 sc_agent_signin_proc(SC_AGENT_NODE_ST *pstAgentNode)
{
    SC_AGENT_INFO_ST    *pstAgentInfo   = NULL;
    SC_LEG_CB           *pstLegCB       = NULL;
    SC_SRV_CB           *pstSCB         = NULL;
    U32                 ulRet           = DOS_FAIL;
    SC_MSG_CMD_CALL_ST stCallMsg;

    if (DOS_ADDR_INVALID(pstAgentNode)
        || DOS_ADDR_INVALID(pstAgentNode->pstAgentInfo))
    {
        return DOS_FAIL;
    }

    pstAgentInfo = pstAgentNode->pstAgentInfo;

    /* 判断一下坐席的状态，看看是否已经长签 */
    if (pstAgentInfo->bNeedConnected && pstAgentInfo->bConnected)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_HTTP_API), "Agent %u request signin. But it seems already signin. Exit.", pstAgentInfo->ulAgentID);

        return DOS_FAIL;
    }

    pstLegCB = sc_lcb_get(pstAgentInfo->ulLegNo);
    if (DOS_ADDR_VALID(pstLegCB))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_HTTP_API), "Agent %u request signin. But it seems in a call(LEG: %u). Exit.", pstAgentInfo->ulAgentID, pstAgentInfo->ulLegNo);
        return DOS_FAIL;
    }

    pstSCB = sc_scb_alloc();
    if (DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstLegCB = sc_lcb_alloc();
    if (DOS_ADDR_INVALID(pstLegCB))
    {
        sc_scb_free(pstSCB);
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    pstLegCB->stCall.bValid = DOS_TRUE;
    pstLegCB->ulIndSCBNo = pstSCB->ulSCBNo;

    pstSCB->stSigin.stSCBTag.bValid = DOS_TRUE;
    pstSCB->stSigin.pstAgentNode = pstAgentNode;
    pstSCB->stSigin.ulLegNo = pstLegCB->ulCBNo;
    pstSCB->stSigin.pstAgentNode = pstAgentNode;
    pstSCB->ulAgentID = pstAgentInfo->ulAgentID;
    pstSCB->ulCustomerID = pstAgentInfo->ulCustomerID;

    pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = (SC_SCB_TAG_ST *)&pstSCB->stSigin;

    /* 从主叫号码组中获取主叫号码 */
    ulRet = sc_caller_setting_select_number(pstAgentInfo->ulCustomerID, pstAgentInfo->ulAgentID, SC_SRC_CALLER_TYPE_AGENT, pstLegCB->stCall.stNumInfo.szOriginalCalling, SC_NUM_LENGTH);
    if (ulRet != DOS_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_HTTP_API), "Agent signin customID(%u) get caller number FAIL by agent(%u)", pstAgentInfo->ulCustomerID, pstAgentInfo->ulAgentID);
        sc_scb_free(pstSCB);
        sc_lcb_free(pstLegCB);

        return DOS_FAIL;
    }

    switch (pstAgentInfo->ucBindType)
    {
        case AGENT_BIND_SIP:
            dos_snprintf(pstLegCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstLegCB->stCall.stNumInfo.szOriginalCallee), pstAgentInfo->szUserID);
            pstLegCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND_INTERNAL;
            sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL);

            break;

        case AGENT_BIND_TELE:
            dos_snprintf(pstLegCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstLegCB->stCall.stNumInfo.szOriginalCallee), pstAgentInfo->szTelePhone);
            pstLegCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND;
            sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL);

            break;

        case AGENT_BIND_MOBILE:
            dos_snprintf(pstLegCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstLegCB->stCall.stNumInfo.szOriginalCallee), pstAgentInfo->szMobile);
            pstLegCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND;

            break;

        case AGENT_BIND_TT_NUMBER:
            dos_snprintf(pstLegCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstLegCB->stCall.stNumInfo.szOriginalCallee), pstAgentInfo->szTTNumber);
            pstLegCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND_TT;
            break;

        default:
            break;
    }
    pstLegCB->stCall.stNumInfo.szOriginalCallee[sizeof(pstLegCB->stCall.stNumInfo.szOriginalCallee)-1] = '\0';

    if (pstLegCB->stCall.ucPeerType == SC_LEG_PEER_OUTBOUND)
    {
        /* 需要认证 */
        pstSCB->stSigin.stSCBTag.usStatus = SC_SIGIN_AUTH;
        ulRet = sc_send_usr_auth2bs(pstSCB, pstLegCB);
        if (ulRet != DOS_SUCC)
        {
            sc_scb_free(pstSCB);
            sc_lcb_free(pstLegCB);

            return DOS_FAIL;
        }
    }
    else
    {
        /* 发起呼叫 */
        /* 处理一下号码 */
        dos_snprintf(pstLegCB->stCall.stNumInfo.szRealCallee, sizeof(pstLegCB->stCall.stNumInfo.szCallee), pstLegCB->stCall.stNumInfo.szOriginalCallee);
        dos_snprintf(pstLegCB->stCall.stNumInfo.szRealCalling, sizeof(pstLegCB->stCall.stNumInfo.szCalling), pstLegCB->stCall.stNumInfo.szOriginalCalling);

        dos_snprintf(pstLegCB->stCall.stNumInfo.szCallee, sizeof(pstLegCB->stCall.stNumInfo.szCallee), pstLegCB->stCall.stNumInfo.szOriginalCallee);
        dos_snprintf(pstLegCB->stCall.stNumInfo.szCalling, sizeof(pstLegCB->stCall.stNumInfo.szCalling), pstLegCB->stCall.stNumInfo.szOriginalCalling);

        pstSCB->stSigin.stSCBTag.usStatus = SC_SIGIN_EXEC;

        stCallMsg.stMsgTag.ulMsgType = SC_CMD_CALL;
        stCallMsg.stMsgTag.ulSCBNo = pstSCB->ulSCBNo;
        stCallMsg.stMsgTag.usInterErr = 0;
        stCallMsg.ulSCBNo = pstSCB->ulSCBNo;
        stCallMsg.ulLCBNo = pstLegCB->ulCBNo;

        if (sc_send_cmd_new_call(&stCallMsg.stMsgTag) != DOS_SUCC)
        {
            sc_scb_free(pstSCB);
            sc_lcb_free(pstLegCB);

            return DOS_FAIL;
        }
    }

    return DOS_SUCC;
}

U32 sc_agent_signout_proc(SC_AGENT_INFO_ST *pstAgentInfo)
{
    return DOS_SUCC;
}

U32 sc_agent_notify_get_channel_id(SC_AGENT_INFO_ST *pstAgentInfo, S8 *pszChannel, U32 ulLen)
{
    if (DOS_ADDR_INVALID(pstAgentInfo))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (DOS_ADDR_INVALID(pszChannel) || 0 == ulLen)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    dos_snprintf(pszChannel, ulLen, "%u_%s"
                , pstAgentInfo->ulAgentID
                , pstAgentInfo->szEmpNo);

    return DOS_SUCC;
}

U32 sc_agent_status_notify(SC_AGENT_INFO_ST *pstAgentInfo, U32 ulStatus)
{
    S8 szURL[256]         = { 0, };
    S8 szData[512]        = { 0, };
    S8 szChannel[128]     = { 0, };
    S32 ulPAPIPort        = -1;

    if (DOS_ADDR_INVALID(pstAgentInfo))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if (sc_agent_notify_get_channel_id(pstAgentInfo, szChannel, sizeof(szChannel)) != DOS_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Get channel ID fail for agent: %u", pstAgentInfo->ulAgentID);
        return DOS_FAIL;
    }

    ulPAPIPort = config_hb_get_papi_port();
    if (ulPAPIPort <= 0)
    {
        dos_snprintf(szURL, sizeof(szURL), "http://localhost/pub?id=%s", szChannel);
    }
    else
    {
        dos_snprintf(szURL, sizeof(szURL), "http://localhost:%d/pub?id=%s", ulPAPIPort, szChannel);
    }


    /* 格式中引号前面需要添加"\",提供给push stream做转义用 */
    dos_snprintf(szData, sizeof(szData), "{\\\"type\\\":\\\"1\\\",\\\"body\\\":{\\\"type\\\":\\\"%u\\\",\\\"result\\\":\\\"0\\\"}}", ulStatus);

    return sc_pub_send_msg(szURL, szData, SC_PUB_TYPE_STATUS, NULL);
}


/*
 * 函  数: sc_acd_hash_func4agent
 * 功  能: 坐席的hash函数，通过分机号计算一个hash值
 * 参  数:
 *         S8 *pszExension  : 分机号
 *         U32 *pulHashIndex: 输出参数，计算之后的hash值
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 **/
U32 sc_agent_hash_func4agent(U32 ulAgentID, U32 *pulHashIndex)
{
    if (DOS_ADDR_INVALID(pulHashIndex))
    {
        DOS_ASSERT(0);

        *pulHashIndex = 0;
        return DOS_FAIL;
    }

    *pulHashIndex = ulAgentID % SC_ACD_HASH_SIZE;

    return DOS_SUCC;
}

/*
 * 函  数: sc_acd_hash_func4grp
 * 功  能: 坐席组的hash函数，通过分机号计算一个hash值
 * 参  数:
 *         U32 ulGrpID  : 坐席组ID
 *         U32 *pulHashIndex: 输出参数，计算之后的hash值
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 **/
U32 sc_agent_hash_func4grp(U32 ulGrpID, U32 *pulHashIndex)
{
    if (DOS_ADDR_INVALID(pulHashIndex))
    {
        return DOS_FAIL;
    }

    *pulHashIndex = ulGrpID % SC_ACD_HASH_SIZE;
    return DOS_SUCC;
}

/*
 * 函  数: sc_acd_hash_func4calller_relation
 * 功  能: 主叫号码和坐席对应关系的hash函数，通过主叫号码计算一个hash值
 * 参  数:
 *         U32 ulGrpID  : 坐席组ID
 *         U32 *pulHashIndex: 输出参数，计算之后的hash值
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 **/
U32 sc_agent_hash_func4calller_relation(S8 *szCallerNum, U32 *pulHashIndex)
{
    U64 ulCallerNum = 0;

    if (DOS_ADDR_INVALID(pulHashIndex) || DOS_ADDR_INVALID(szCallerNum))
    {
        return DOS_FAIL;
    }

    dos_sscanf(szCallerNum, "%9lu", &ulCallerNum);

    *pulHashIndex =  ulCallerNum % SC_ACD_CALLER_NUM_RELATION_HASH_SIZE;
    return DOS_SUCC;
}

/*
 * 函  数: sc_acd_grp_hash_find
 * 功  能: 坐席组的hash查找函数
 * 参  数:
 *         VOID *pSymName  : 坐席组ID
 *         HASH_NODE_S *pNode: HASH节点
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 **/
S32 sc_agent_group_hash_find(VOID *pSymName, HASH_NODE_S *pNode)
{
    SC_AGENT_GRP_NODE_ST    *pstGrpHashNode = NULL;
    U32                        ulIndex         = 0;

    if (DOS_ADDR_INVALID(pNode)
        || DOS_ADDR_INVALID(pNode))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstGrpHashNode = (SC_AGENT_GRP_NODE_ST  *)pNode->pHandle;
    ulIndex = (U32)*((U32 *)pSymName);

    if (DOS_ADDR_VALID(pstGrpHashNode)
        && DOS_ADDR_VALID(pSymName)
        && pstGrpHashNode->ulGroupID == ulIndex)
    {
        return DOS_SUCC;
    }

    return DOS_FAIL;
}

/*
 * 函  数: sc_acd_agent_hash_find
 * 功  能: 坐席的hash查找函数
 * 参  数:
 *         VOID *pSymName  : 坐席分机号
 *         HASH_NODE_S *pNode: HASH节点
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 **/
S32 sc_agent_hash_find(VOID *pSymName, HASH_NODE_S *pNode)
{
    SC_AGENT_NODE_ST   *pstAgentQueueNode = NULL;
    U32                          ulAgentID;

    if (DOS_ADDR_INVALID(pSymName)
        || DOS_ADDR_INVALID(pNode))
    {
        return DOS_FAIL;
    }

    pstAgentQueueNode = pNode->pHandle;
    ulAgentID = *(U32 *)pSymName;

    if (DOS_ADDR_INVALID(pstAgentQueueNode)
        || DOS_ADDR_INVALID(pstAgentQueueNode->pstAgentInfo))
    {
        return DOS_FAIL;
    }

    if (ulAgentID != pstAgentQueueNode->pstAgentInfo->ulAgentID)
    {
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/*
 * 函  数: sc_acd_grp_hash_find
 * 功  能: 坐席组的hash查找函数
 * 参  数:
 *         VOID *pSymName  : 坐席组ID
 *         HASH_NODE_S *pNode: HASH节点
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 **/
S32 sc_agent_caller_relation_hash_find(VOID *pSymName, HASH_NODE_S *pNode)
{
    SC_ACD_MEMORY_RELATION_QUEUE_NODE_ST *pstHashNode = NULL;
    S8  *szCallerNum = NULL;

    if (DOS_ADDR_INVALID(pSymName)
        || DOS_ADDR_INVALID(pNode))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstHashNode = (SC_ACD_MEMORY_RELATION_QUEUE_NODE_ST *)pNode->pHandle;
    szCallerNum = (S8 *)pSymName;
    if (DOS_ADDR_VALID(pstHashNode)
        && !dos_strcmp(szCallerNum, pstHashNode->szCallerNum))
    {
        return DOS_SUCC;
    }

    return DOS_FAIL;
}

/*
 * 函  数: S32 sc_acd_site_dll_find(VOID *pSymName, DLL_NODE_S *pNode)
 * 功  能: 坐席的hash查找函数
 * 参  数:
 *         VOID *pSymName  : 坐席分机号
 *         DLL_NODE_S *pNode: 链表节点
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 **/
static S32 sc_agent_dll_find(VOID *pSymName, DLL_NODE_S *pNode)
{
    SC_AGENT_NODE_ST   *pstAgentQueueNode = NULL;
    U32                          ulAgentID;

    if (DOS_ADDR_INVALID(pSymName)
        || DOS_ADDR_INVALID(pNode))
    {
        return DOS_FAIL;
    }

    pstAgentQueueNode = pNode->pHandle;
    ulAgentID = *(U32 *)pSymName;

    if (DOS_ADDR_INVALID(pstAgentQueueNode)
        || DOS_ADDR_INVALID(pstAgentQueueNode->pstAgentInfo))
    {
        return DOS_FAIL;
    }

    if (ulAgentID != pstAgentQueueNode->pstAgentInfo->ulAgentID)
    {
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

U32 sc_agent_query_idel(U32 ulAgentGrpID, BOOL *pblResult)
{
    SC_AGENT_NODE_ST           *pstAgentNode      = NULL;
    SC_AGENT_GRP_NODE_ST       *pstGroupListNode  = NULL;
    HASH_NODE_S                *pstHashNode       = NULL;
    DLL_NODE_S                 *pstDLLNode        = NULL;
    U32                        ulHashVal          = 0;

    if (DOS_ADDR_INVALID(pblResult))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    *pblResult = DOS_FALSE;

    sc_agent_hash_func4grp(ulAgentGrpID, &ulHashVal);
    pthread_mutex_lock(&g_mutexGroupList);
    pstHashNode = hash_find_node(g_pstGroupList, ulHashVal, &ulAgentGrpID, sc_agent_group_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        //sc_logr_error(NULL, SC_ACD, "Cannot fine the group with the ID \"%u\" .", ulAgentGrpID);
        pthread_mutex_unlock(&g_mutexGroupList);

        return DOS_FAIL;
    }

    pstGroupListNode = pstHashNode->pHandle;

    pthread_mutex_lock(&pstGroupListNode->mutexSiteQueue);

    DLL_Scan(&pstGroupListNode->stAgentList, pstDLLNode, DLL_NODE_S*)
    {
        if (DOS_ADDR_INVALID(pstDLLNode)
            || DOS_ADDR_INVALID(pstDLLNode->pHandle))
        {
            //sc_logr_debug(NULL, SC_ACD, "Group List node has no data. Maybe the data has been deleted. Group: %u."
            //                , pstGroupListNode->ulGroupID);
            continue;
        }

        pstAgentNode = pstDLLNode->pHandle;
        if (DOS_ADDR_INVALID(pstAgentNode)
            || DOS_ADDR_INVALID(pstAgentNode->pstAgentInfo))
        {
            //sc_logr_debug(NULL, SC_ACD, "Group List node has no data. Maybe the data has been deleted. Group: %u."
            //                , pstGroupListNode->ulGroupID);
            continue;
        }

        if (pstAgentNode->pstAgentInfo->ulLastIdelTime
            && (time(NULL) - pstAgentNode->pstAgentInfo->ulLastIdelTime < 3))
        {
            //sc_logr_debug(NULL, SC_ACD, "Agent is in protect Agent: %u. Group: %u."
            //                , pstAgentNode->pstAgentInfo->ulSiteID
            //                , pstGroupListNode->ulGroupID);
            continue;
        }

        if (SC_ACD_SITE_IS_USEABLE(pstAgentNode->pstAgentInfo))
        {
            //sc_logr_debug(NULL, SC_ACD, "Found an useable agent. (Agent %u in Group %u)"
            //            , pstAgentNode->pstAgentInfo->ulSiteID
            //            , pstGroupListNode->ulGroupID);

            *pblResult = DOS_TRUE;
            break;
        }
    }

    pthread_mutex_unlock(&pstGroupListNode->mutexSiteQueue);

    pthread_mutex_unlock(&g_mutexGroupList);

    return DOS_SUCC;

}

/*
 * 函  数: U32 sc_acd_update_agent_scbno_by_userid(S8 *szUserID, SC_SCB_ST *pstSCB)
 * 功  能: 根据SIP，查找到绑定的坐席，更新usSCBNo字段. (ucBindType 为 AGENT_BIND_SIP)
 * 参  数:
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 **/
SC_AGENT_NODE_ST *sc_agent_get_by_sip_acc(S8 *szUserID)
{
    U32                         ulHashIndex         = 0;
    HASH_NODE_S                 *pstHashNode        = NULL;
    SC_AGENT_NODE_ST  *pstAgentQueueNode  = NULL;
    SC_AGENT_INFO_ST        *pstAgentData       = NULL;

    if (DOS_ADDR_INVALID(szUserID) || '\0' == szUserID[0])
    {
        return NULL;
    }

    /* 查找SIP绑定的坐席 */
    pthread_mutex_lock(&g_mutexAgentList);

    HASH_Scan_Table(g_pstAgentList, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstAgentList, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            if (DOS_ADDR_INVALID(pstHashNode) || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }
            pstAgentQueueNode = (SC_AGENT_NODE_ST *)pstHashNode->pHandle;
            pstAgentData = pstAgentQueueNode->pstAgentInfo;

            if (DOS_ADDR_INVALID(pstAgentData))
            {
                continue;
            }

            if ('\0' == pstAgentData->szUserID[0])
            {
                continue;
            }

            if (dos_strcmp(pstAgentData->szUserID, szUserID) == 0)
            {
                pthread_mutex_unlock(&g_mutexAgentList);

                return pstAgentQueueNode;
            }
        }
    }

    pthread_mutex_unlock(&g_mutexAgentList);

    return NULL;
}


/*
 * 函  数: U32 sc_acd_group_remove_agent(U32 ulGroupID, S8 *pszUserID)
 * 功  能: 从坐席队列中移除坐席
 * 参  数:
 *       U32 ulGroupID: 坐席组ID
 *       S8 *pszUserID: 坐席唯一标示 sip账户
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 **/
U32 sc_agent_group_remove_agent(U32 ulGroupID, U32 ulAgentID)
{
    SC_AGENT_NODE_ST   *pstAgentQueueNode  = NULL;
    SC_AGENT_GRP_NODE_ST      *pstGroupNode       = NULL;
    HASH_NODE_S                  *pstHashNode        = NULL;
    DLL_NODE_S                   *pstDLLNode         = NULL;
    U32                          ulHashVal           = 0;

    /* 检测所在队列是否存在 */
    sc_agent_hash_func4grp(ulGroupID, &ulHashVal);
    pthread_mutex_lock(&g_mutexGroupList);
    pstHashNode = hash_find_node(g_pstGroupList, ulHashVal , &ulGroupID, sc_agent_group_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_ACD), "Cannot find the group \"%u\" for the site %u.", ulGroupID, ulAgentID);

        pthread_mutex_unlock(&g_mutexGroupList);
        return DOS_FAIL;
    }

    pstGroupNode = pstHashNode->pHandle;

    pstDLLNode = dll_find(&pstGroupNode->stAgentList, (VOID *)&ulAgentID, sc_agent_dll_find);
    if (DOS_ADDR_INVALID(pstDLLNode)
        || DOS_ADDR_INVALID(pstDLLNode->pHandle))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_ACD), "Cannot find the agent %u in the group %u.", ulAgentID, ulGroupID);

        pthread_mutex_unlock(&g_mutexGroupList);
        return DOS_FAIL;
    }

    dll_delete(&pstGroupNode->stAgentList, pstDLLNode);

    pstAgentQueueNode = pstDLLNode->pHandle;

    pstDLLNode->pHandle = NULL;
    dos_dmem_free(pstDLLNode);
    pstDLLNode = NULL;
    pstAgentQueueNode->pstAgentInfo = NULL;
    dos_dmem_free(pstAgentQueueNode);
    pstAgentQueueNode = NULL;

    pstGroupNode->usCount--;

    pthread_mutex_unlock(&g_mutexGroupList);
    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ACD), "Remove agent %u from group %u SUCC.", ulAgentID, ulGroupID);

    return DOS_SUCC;
}

/*
 * 函  数: U32 sc_acd_group_add_agent(U32 ulGroupID, SC_AGENT_INFO_ST *pstAgentInfo)
 * 功  能: 将坐席添加到坐席队列
 * 参  数:
 *       U32 ulGroupID: 坐席组ID
 *       SC_AGENT_INFO_ST *pstAgentInfo : 坐席组信息
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 **/
U32 sc_agent_group_add_agent(U32 ulGroupID, SC_AGENT_INFO_ST *pstAgentInfo)
{
    SC_AGENT_NODE_ST   *pstAgentQueueNode  = NULL;
    SC_AGENT_GRP_NODE_ST      *pstGroupNode       = NULL;
    HASH_NODE_S                  *pstHashNode        = NULL;
    DLL_NODE_S                   *pstDLLNode         = NULL;
    U32                          ulHashVal           = 0;

    if (DOS_ADDR_INVALID(pstAgentInfo))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    /* 检测所在队列是否存在 */
    sc_agent_hash_func4grp(ulGroupID, &ulHashVal);
    pthread_mutex_lock(&g_mutexGroupList);
    pstHashNode = hash_find_node(g_pstGroupList, ulHashVal , &ulGroupID, sc_agent_group_hash_find);
    pthread_mutex_unlock(&g_mutexGroupList);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_ACD), "Cannot find the group \"%u\" for the site %s.", ulGroupID, pstAgentInfo->szUserID);
        return DOS_FAIL;
    }
    pstGroupNode = pstHashNode->pHandle;

    pstDLLNode = (DLL_NODE_S *)dos_dmem_alloc(sizeof(DLL_NODE_S));
    if (DOS_ADDR_INVALID(pstDLLNode))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_ACD), "Add agent to group FAILED, Alloc memory for list Node fail. Agent ID: %u, Group ID:%u"
                , pstAgentInfo->ulAgentID
                , ulGroupID);
        pthread_mutex_unlock(&g_mutexGroupList);
        return DOS_FAIL;
    }

    pstAgentQueueNode = (SC_AGENT_NODE_ST *)dos_dmem_alloc(sizeof(SC_AGENT_NODE_ST));
    if (DOS_ADDR_INVALID(pstAgentQueueNode))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_ACD), "Add agent to group FAILED, Alloc memory fail. Agent ID: %u, Group ID:%u"
                , pstAgentInfo->ulAgentID
                , ulGroupID);

        dos_dmem_free(pstDLLNode);
        pstDLLNode = NULL;
        pthread_mutex_unlock(&g_mutexGroupList);
        return DOS_FAIL;
    }

    DLL_Init_Node(pstDLLNode);
    pstDLLNode->pHandle = pstAgentQueueNode;
    pstAgentQueueNode->pstAgentInfo = pstAgentInfo;

    pthread_mutex_lock(&pstGroupNode->mutexSiteQueue);
    pstAgentQueueNode->ulID = pstGroupNode->usCount;
    pstGroupNode->usCount++;

    DLL_Add(&pstGroupNode->stAgentList, pstDLLNode);
    pthread_mutex_unlock(&pstGroupNode->mutexSiteQueue);

    pthread_mutex_unlock(&g_mutexGroupList);

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ACD), "Add agent to group SUCC. Agent ID: %u, Group ID:%u, Bind Type: %u"
            , pstAgentInfo->ulAgentID
            , pstGroupNode->ulGroupID
            , pstAgentInfo->ucBindType);

    return DOS_SUCC;
}

/*
 * 函  数: sc_acd_add_agent
 * 功  能: 添加坐席
 * 参  数:
 *         SC_AGENT_INFO_ST *pstAgentInfo, 坐席信息描述
 *         U32 ulGrpID 所属组
 * 返回值: SC_AGENT_NODE_ST *返回坐席队列里面的节点。编译调用函数将坐席添加到组。如果失败则返回NULL
 * !!! 该函数只是将坐席加入管理队列，不会将坐席加入坐席对列 !!!
 **/
SC_AGENT_INFO_ST *sc_agent_add(SC_AGENT_INFO_ST *pstAgentInfo)
{
    SC_AGENT_NODE_ST   *pstAgentQueueNode  = NULL;
    SC_AGENT_INFO_ST         *pstAgentData       = NULL;
    HASH_NODE_S                  *pstHashNode        = NULL;
    U32                          ulHashVal           = 0;

    if (DOS_ADDR_INVALID(pstAgentInfo))
    {
        DOS_ASSERT(0);

        return NULL;
    }

    pstAgentQueueNode = (SC_AGENT_NODE_ST *)dos_dmem_alloc(sizeof(SC_AGENT_NODE_ST));
    if (DOS_ADDR_INVALID(pstAgentQueueNode))
    {
        DOS_ASSERT(0);

        return NULL;
    }

    pstAgentData = (SC_AGENT_INFO_ST *)dos_dmem_alloc(sizeof(SC_AGENT_INFO_ST));
    if (DOS_ADDR_INVALID(pstAgentData))
    {
        DOS_ASSERT(0);

        return NULL;
    }

    pstHashNode = (HASH_NODE_S *)dos_dmem_alloc(sizeof(HASH_NODE_S));
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        DOS_ASSERT(0);

        return NULL;
    }

    /* 加入坐席管理hash表 */
    HASH_Init_Node(pstHashNode);
    pstHashNode->pHandle = pstAgentQueueNode;
    sc_agent_hash_func4agent(pstAgentInfo->ulAgentID, &ulHashVal);
    pthread_mutex_lock(&g_mutexAgentList);
    hash_add_node(g_pstAgentList, pstHashNode, ulHashVal, NULL);
    pthread_mutex_unlock(&g_mutexAgentList);

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ACD), "Load Agent. ID: %u, Customer: %u, Group1: %u, Group2: %u"
                    , pstAgentInfo->ulAgentID, pstAgentInfo->ulCustomerID
                    , pstAgentInfo->aulGroupID[0], pstAgentInfo->aulGroupID[1]);

    /* 添加到队列 */
    dos_memcpy(pstAgentData, pstAgentInfo, sizeof(SC_AGENT_INFO_ST));
    pthread_mutex_init(&pstAgentData->mutexLock, NULL);
    pstAgentQueueNode->pstAgentInfo = pstAgentData;

    return pstAgentData;
}


/*
 * 函  数: sc_acd_grp_wolk4delete_site
 * 功  能: 从某一个坐席组里面删除坐席
 * 参  数:
 *         HASH_NODE_S *pNode : 当前节点
 *         VOID *pszExtensition : 被删除坐席的分机号
 * 返回值: VOID
 **/
static VOID sc_agent_group_wolk4delete_agent(HASH_NODE_S *pNode, VOID *pulAgentID)
{
    SC_AGENT_GRP_NODE_ST      *pstGroupListNode  = NULL;
    SC_AGENT_NODE_ST   *pstAgentQueueNode = NULL;
    DLL_NODE_S                   *pstDLLNode        = NULL;

    if (DOS_ADDR_INVALID(pNode) || DOS_ADDR_INVALID(pulAgentID))
    {
        return;
    }

    if (DOS_ADDR_INVALID(pNode)
        || DOS_ADDR_INVALID(pNode->pHandle))
    {
        DOS_ASSERT(0);
        return;
    }

    pstGroupListNode = (SC_AGENT_GRP_NODE_ST *)pNode->pHandle;
    pthread_mutex_lock(&pstGroupListNode->mutexSiteQueue);
    pstDLLNode = dll_find(&pstGroupListNode->stAgentList, (VOID *)pulAgentID, sc_agent_dll_find);
    if (DOS_ADDR_INVALID(pstAgentQueueNode)
        || DOS_ADDR_INVALID(pstDLLNode->pHandle))
    {
        DOS_ASSERT(0);
        pthread_mutex_unlock(&pstGroupListNode->mutexSiteQueue);
        return;
    }

    /* 这个地方先不要free，有可能别的队列也有这个作息 */
    pstAgentQueueNode = pstDLLNode->pHandle;
    pstAgentQueueNode->pstAgentInfo = NULL;

    pthread_mutex_unlock(&pstGroupListNode->mutexSiteQueue);
}

U32 sc_agent_delete(U32  ulAgentID)
{
    SC_AGENT_NODE_ST   *pstAgentQueueNode = NULL;
    HASH_NODE_S                  *pstHashNode       = NULL;
    U32                          ulHashVal          = 0;

    /* 遍历所有组，并删除相关坐席 */
    pthread_mutex_lock(&g_mutexGroupList);
    hash_walk_table(g_pstGroupList, &ulAgentID, sc_agent_group_wolk4delete_agent);
    pthread_mutex_unlock(&g_mutexGroupList);

    /* 做到坐席，然后将其值为删除状态 */
    pthread_mutex_lock(&g_mutexAgentList);
    sc_agent_hash_func4agent(ulAgentID, &ulHashVal);
    pstHashNode = hash_find_node(g_pstAgentList, ulHashVal, &ulAgentID, sc_agent_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_ACD), "Connot find the Site %u while delete", ulAgentID);
        pthread_mutex_unlock(&g_mutexAgentList);
        return DOS_FAIL;
    }

    pstAgentQueueNode = pstHashNode->pHandle;
    if (DOS_ADDR_VALID(pstAgentQueueNode->pstAgentInfo))
    {
        pstAgentQueueNode->pstAgentInfo->bWaitingDelete = DOS_TRUE;
    }
    pthread_mutex_unlock(&g_mutexAgentList);

    return DOS_SUCC;
}

U32 sc_agent_group_add(U32 ulGroupID, U32 ulCustomID, U32 ulPolicy, S8 *pszGroupName)
{
    SC_AGENT_GRP_NODE_ST    *pstGroupListNode = NULL;
    HASH_NODE_S                *pstHashNode      = NULL;
    U32                        ulHashVal         = 0;

    if (DOS_ADDR_INVALID(pszGroupName))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (0 == ulGroupID
        || 0 == ulCustomID
        || ulPolicy >= SC_ACD_POLICY_BUTT)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ACD), "Load Group. ID:%u, Customer:%u, Policy: %u, Name: %s", ulGroupID, ulCustomID, ulPolicy, pszGroupName);

    /* 确定队列 */
    sc_agent_hash_func4grp(ulGroupID, &ulHashVal);
    pthread_mutex_lock(&g_mutexGroupList);
    pstHashNode = hash_find_node(g_pstGroupList, ulHashVal , &ulGroupID, sc_agent_group_hash_find);
    if (DOS_ADDR_VALID(pstHashNode)
        && DOS_ADDR_VALID(pstHashNode->pHandle))
    {
        pstGroupListNode = pstHashNode->pHandle;

        pstGroupListNode->ucACDPolicy = (U8)ulPolicy;
        pstGroupListNode->usCount = 0;
        pstGroupListNode->usLastUsedAgent = 0;
        pstGroupListNode->szLastEmpNo[0] = '\0';

        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_ACD), "Group \"%u\" Already in the list. Update", ulGroupID);
        pthread_mutex_unlock(&g_mutexGroupList);

        return DOS_FAIL;
    }
    pthread_mutex_unlock(&g_mutexGroupList);

    pstGroupListNode = (SC_AGENT_GRP_NODE_ST *)dos_dmem_alloc(sizeof(SC_AGENT_GRP_NODE_ST));
    if (DOS_ADDR_INVALID(pstGroupListNode))
    {
        DOS_ASSERT(0);

        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_ACD), "%s", "Add group fail. Alloc memory fail");
        return DOS_FAIL;
    }
    dos_memzero(pstGroupListNode, sizeof(SC_AGENT_GRP_NODE_ST));

    pstHashNode = (HASH_NODE_S *)dos_dmem_alloc(sizeof(HASH_NODE_S));
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        DOS_ASSERT(0);

        dos_dmem_free(pstGroupListNode);
        pstGroupListNode = NULL;
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_ACD), "%s", "Add group fail. Alloc memory for hash node fail");
        return DOS_FAIL;
    }

    HASH_Init_Node(pstHashNode);
    DLL_Init(&pstGroupListNode->stAgentList);
    pstGroupListNode->pstRelationList = hash_create_table(SC_ACD_CALLER_NUM_RELATION_HASH_SIZE, NULL);
    if (DOS_ADDR_INVALID(pstGroupListNode->pstRelationList))
    {
        DOS_ASSERT(0);
        dos_dmem_free(pstGroupListNode);
        pstGroupListNode = NULL;
        dos_dmem_free(pstHashNode);
        pstHashNode = NULL;

        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_ACD), "%s", "Init Group caller num relation Hash Table Fail.");
        return DOS_FAIL;
    }
    pstGroupListNode->pstRelationList->NodeNum = 0;

    pthread_mutex_init(&pstGroupListNode->mutexSiteQueue, NULL);
    pstGroupListNode->ulCustomID = ulCustomID;
    pstGroupListNode->ulGroupID  = ulGroupID;
    pstGroupListNode->ucACDPolicy = (U8)ulPolicy;
    pstGroupListNode->usCount = 0;
    pstGroupListNode->usLastUsedAgent = 0;
    pstGroupListNode->szLastEmpNo[0] = '\0';
    pstGroupListNode->usID = (U16)g_ulGroupCount;
    pstGroupListNode->ucWaitingDelete = DOS_FALSE;
    if (pszGroupName[0] != '\0')
    {
        dos_strncpy(pstGroupListNode->szGroupName, pszGroupName, sizeof(pstGroupListNode->szGroupName));
        pstGroupListNode->szGroupName[sizeof(pstGroupListNode->szGroupName) - 1] = '\0';
    }

    pthread_mutex_lock(&g_mutexGroupList);
    pstHashNode->pHandle = pstGroupListNode;
    hash_add_node(g_pstGroupList, pstHashNode, ulHashVal, NULL);
    pthread_mutex_unlock(&g_mutexGroupList);
    return DOS_SUCC;
}

U32 sc_agent_group_delete(U32 ulGroupID)
{
    SC_AGENT_GRP_NODE_ST    *pstGroupListNode = NULL;
    HASH_NODE_S                *pstHashNode      = NULL;
    U32                        ulHashVal         = 0;

    if (0 == ulGroupID)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* 确定队列 */
    sc_agent_hash_func4grp(ulGroupID, &ulHashVal);
    pthread_mutex_lock(&g_mutexGroupList);
    pstHashNode = hash_find_node(g_pstGroupList, ulHashVal , &ulGroupID, sc_agent_group_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_ACD), "Connot find the Group \"%u\".", ulGroupID);
        pthread_mutex_unlock(&g_mutexGroupList);

        return DOS_FAIL;
    }

    pstGroupListNode = pstHashNode->pHandle;
    pthread_mutex_lock(&pstGroupListNode->mutexSiteQueue);
    pstGroupListNode->ucWaitingDelete = DOS_TRUE;
    pthread_mutex_unlock(&pstGroupListNode->mutexSiteQueue);
    pthread_mutex_unlock(&g_mutexGroupList);

    return DOS_SUCC;
}


SC_AGENT_NODE_ST * sc_agent_select_by_random(SC_AGENT_GRP_NODE_ST *pstGroupListNode)
{
    U32     ulRandomAgent      = 0;
    SC_AGENT_NODE_ST *pstAgentQueueNode = NULL;
    SC_AGENT_NODE_ST *pstAgentNodeRet   = NULL;
    SC_AGENT_INFO_ST       *pstAgentInfo      = NULL;
    DLL_NODE_S                 *pstDLLNode        = NULL;

    if (DOS_ADDR_INVALID(pstGroupListNode))
    {
        DOS_ASSERT(0);
        return NULL;
    }

    /*
     * 随机一个编号，然后从这个编号开始查找一个可用的坐席。如果到队尾了还没有找到就再从头来
     */

    ulRandomAgent = dos_random(pstGroupListNode->usCount);

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ACD), "Select agent in random. Start find agent %u in group %u, count: %u."
                    , ulRandomAgent
                    , pstGroupListNode->ulGroupID
                    , pstGroupListNode->stAgentList.ulCount);

    DLL_Scan(&pstGroupListNode->stAgentList, pstDLLNode, DLL_NODE_S*)
    {
        if (DOS_ADDR_INVALID(pstDLLNode)
            || DOS_ADDR_INVALID(pstDLLNode->pHandle))
        {
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ACD), "Group List node has no data. Group: %u."
                                        , pstGroupListNode->ulGroupID);
            continue;
        }

        pstAgentQueueNode = pstDLLNode->pHandle;
        if (DOS_ADDR_INVALID(pstAgentQueueNode)
            || DOS_ADDR_INVALID(pstAgentQueueNode->pstAgentInfo))
        {
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ACD), "Group List node has no data. Maybe the data has been deleted. Group: %u."
                            , pstGroupListNode->ulGroupID);
            continue;
        }

        if (pstAgentQueueNode->ulID <= ulRandomAgent)
        {
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ACD), "Found an agent. But the agent's order(%u) is less then last agent order(%u). coutinue.(Agent %d in Group %u)"
                            , pstAgentQueueNode->ulID
                            , ulRandomAgent
                            , pstAgentQueueNode->pstAgentInfo->ulAgentID
                            , pstGroupListNode->ulGroupID);
            continue;
        }

        if (!SC_ACD_SITE_IS_USEABLE(pstAgentQueueNode->pstAgentInfo))
        {
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ACD), "There found an agent. But the agent is not useable. coutinue.(Agent %u in Group %u)"
                            , pstAgentQueueNode->pstAgentInfo->ulAgentID
                            , pstGroupListNode->ulGroupID);

            continue;
        }

        pstAgentNodeRet = pstAgentQueueNode;
        pstAgentInfo = pstAgentQueueNode->pstAgentInfo;
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_ACD), "Found an uaeable agent.(Agent %u in Group %u)"
                        , pstAgentInfo->ulAgentID
                        , pstGroupListNode->ulGroupID);
         break;
    }

    if (DOS_ADDR_INVALID(pstAgentInfo))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ACD), "Select agent in random form header. Start find agent %u in group %u, count: %u."
                        , ulRandomAgent
                        , pstGroupListNode->ulGroupID
                        , pstGroupListNode->stAgentList.ulCount);

        DLL_Scan(&pstGroupListNode->stAgentList, pstDLLNode, DLL_NODE_S*)
        {
            if (DOS_ADDR_INVALID(pstDLLNode)
                || DOS_ADDR_INVALID(pstDLLNode->pHandle))
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ACD), "Group List node has no data. Group: %u."
                                            , pstGroupListNode->ulGroupID);
                continue;
            }

            pstAgentQueueNode = pstDLLNode->pHandle;
            if (DOS_ADDR_INVALID(pstAgentQueueNode)
                || DOS_ADDR_INVALID(pstAgentQueueNode->pstAgentInfo))
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ACD), "Group List node has no data. Maybe the data has been deleted. Group: %u."
                                , pstGroupListNode->ulGroupID);
                continue;
            }

            /* 邋到这里已经查找过所有的的坐席了 */
            if (pstAgentQueueNode->ulID > ulRandomAgent)
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ACD), "The end of the select loop.(Group %u)"
                                , pstGroupListNode->ulGroupID);
                break;
            }

            if (!SC_ACD_SITE_IS_USEABLE(pstAgentQueueNode->pstAgentInfo))
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ACD), "There found an agent. But the agent is not useable. coutinue.(Agent %u in Group %u)"
                                , pstAgentQueueNode->pstAgentInfo->ulAgentID
                                , pstGroupListNode->ulGroupID);
                continue;
            }

            pstAgentNodeRet = pstAgentQueueNode;
            pstAgentInfo = pstAgentQueueNode->pstAgentInfo;
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_ACD), "Found an uaeable agent.(Agent %u in Group %u)"
                            , pstAgentInfo->ulAgentID
                            , pstGroupListNode->ulGroupID);

            break;
        }
    }

    return pstAgentNodeRet;
}

SC_AGENT_NODE_ST * sc_agent_select_by_inorder(SC_AGENT_GRP_NODE_ST *pstGroupListNode)
{
    S8      szLastEmpNo[SC_EMP_NUMBER_LENGTH]     = {0};
    S8      szEligibleEmpNo[SC_EMP_NUMBER_LENGTH] = {0};
    SC_AGENT_NODE_ST *pstAgentQueueNode = NULL;
    SC_AGENT_NODE_ST *pstAgentNodeRet   = NULL;
    SC_AGENT_INFO_ST       *pstAgentInfo      = NULL;
    DLL_NODE_S                 *pstDLLNode        = NULL;

    if (DOS_ADDR_INVALID(pstGroupListNode))
    {
        DOS_ASSERT(0);
        return NULL;
    }

    /*
     * 从上次使用的编号开始查找一个可用的坐席。如果到队尾了还没有找到就再从头来
     */

    dos_strncpy(szLastEmpNo, pstGroupListNode->szLastEmpNo, SC_EMP_NUMBER_LENGTH);
    szLastEmpNo[SC_EMP_NUMBER_LENGTH-1] = '\0';
    szEligibleEmpNo[0] = '\0';

start_find:
    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ACD), "Select agent in order. Start find agent %s in group %u, Count : %u"
                    , szLastEmpNo
                    , pstGroupListNode->ulGroupID
                    , pstGroupListNode->stAgentList.ulCount);

    DLL_Scan(&pstGroupListNode->stAgentList, pstDLLNode, DLL_NODE_S*)
    {
        if (DOS_ADDR_INVALID(pstDLLNode)
            || DOS_ADDR_INVALID(pstDLLNode->pHandle))
        {
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ACD), "Group List node has no data. Group: %u."
                            , pstGroupListNode->ulGroupID);
            continue;
        }

        pstAgentQueueNode = pstDLLNode->pHandle;
        if (DOS_ADDR_INVALID(pstAgentQueueNode)
            || DOS_ADDR_INVALID(pstAgentQueueNode->pstAgentInfo))
        {
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ACD), "Group List node has no data. Maybe the data has been deleted. Group: %u."
                            , pstGroupListNode->ulGroupID);
            continue;
        }

        /* 找到一个比最后一个大且最小工号的坐席 */
        if (dos_strncmp(szLastEmpNo, pstAgentQueueNode->pstAgentInfo->szEmpNo, SC_EMP_NUMBER_LENGTH) >= 0)
        {
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ACD), "Found an agent. But the agent's order(%s) is less then last agent order(%s). coutinue.(Agent %u in Group %u)"
                            , pstAgentQueueNode->pstAgentInfo->szEmpNo
                            , szLastEmpNo
                            , pstAgentQueueNode->pstAgentInfo->ulAgentID
                            , pstGroupListNode->ulGroupID);

            continue;
        }

        if (!SC_ACD_SITE_IS_USEABLE(pstAgentQueueNode->pstAgentInfo))
        {

            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ACD), "There found an agent. But the agent is not useable. coutinue.(Agent %u in Group %u)"
                            , pstAgentQueueNode->pstAgentInfo->ulAgentID
                            , pstGroupListNode->ulGroupID);
            continue;
        }

        if ('\0' != szEligibleEmpNo[0])
        {
            if (dos_strncmp(szEligibleEmpNo, pstAgentQueueNode->pstAgentInfo->szEmpNo, SC_EMP_NUMBER_LENGTH) <= 0)
            {
                continue;
            }
        }
        dos_strncpy(szEligibleEmpNo, pstAgentQueueNode->pstAgentInfo->szEmpNo, SC_EMP_NUMBER_LENGTH);
        szEligibleEmpNo[SC_EMP_NUMBER_LENGTH - 1] = '\0';

        pstAgentNodeRet = pstAgentQueueNode;
        pstAgentInfo = pstAgentQueueNode->pstAgentInfo;
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_ACD), "Found an useable agent.(Agent %u in Group %u)"
                        , pstAgentInfo->ulAgentID
                        , pstGroupListNode->ulGroupID);
    }

    if (DOS_ADDR_INVALID(pstAgentInfo) && szLastEmpNo[0] != '\0')
    {
        /* 没有找到到，找最小的工号编号的坐席来接电话 */
        szLastEmpNo[0] = '\0';

        goto start_find;
    }

    if (DOS_ADDR_VALID(pstAgentInfo))
    {
        dos_strncpy(pstGroupListNode->szLastEmpNo, pstAgentInfo->szEmpNo, SC_EMP_NUMBER_LENGTH);
    }

    return pstAgentNodeRet;
}

SC_AGENT_NODE_ST * sc_agent_select_by_call_count(SC_AGENT_GRP_NODE_ST *pstGroupListNode)
{
    SC_AGENT_NODE_ST *pstAgentQueueNode = NULL;
    SC_AGENT_NODE_ST *pstAgentNode      = NULL;
    DLL_NODE_S                 *pstDLLNode        = NULL;

    if (DOS_ADDR_INVALID(pstGroupListNode))
    {
        DOS_ASSERT(0);
        return NULL;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ACD), "Select agent by the min call count. Start find agent in group %u."
                    , pstGroupListNode->ulGroupID);

    DLL_Scan(&pstGroupListNode->stAgentList, pstDLLNode, DLL_NODE_S*)
    {
        if (DOS_ADDR_INVALID(pstDLLNode)
            || DOS_ADDR_INVALID(pstDLLNode->pHandle))
        {
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ACD), "Group List node has no data. Maybe the data has been deleted. Group: %u."
                            , pstGroupListNode->ulGroupID);
            continue;
        }

        pstAgentQueueNode = pstDLLNode->pHandle;
        if (DOS_ADDR_INVALID(pstAgentQueueNode)
            || DOS_ADDR_INVALID(pstAgentQueueNode->pstAgentInfo))
        {
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ACD), "Group List node has no data. Maybe the data has been deleted. Group: %u."
                            , pstGroupListNode->ulGroupID);
            continue;
        }

        if (!SC_ACD_SITE_IS_USEABLE(pstAgentQueueNode->pstAgentInfo))
        {
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ACD), "There found an agent. But the agent is not useable. coutinue.(Agent %u in Group %u)"
                        , pstAgentQueueNode->pstAgentInfo->ulAgentID
                        , pstGroupListNode->ulGroupID);
            continue;
        }

        if (DOS_ADDR_INVALID(pstAgentNode))
        {
            pstAgentNode = pstAgentQueueNode;
        }

        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_ACD), "Found an uaeable agent. Call Count: %d. (Agent %d in Group %d)"
                        , pstAgentQueueNode->pstAgentInfo->ulCallCnt
                        , pstAgentQueueNode->pstAgentInfo->ulAgentID
                        , pstGroupListNode->ulGroupID);
        if (pstAgentNode->pstAgentInfo->ulCallCnt > pstAgentQueueNode->pstAgentInfo->ulCallCnt)
        {
            pstAgentNode = pstAgentQueueNode;
        }
    }

    if (DOS_ADDR_VALID(pstAgentNode) && DOS_ADDR_VALID(pstAgentNode->pstAgentInfo))
    {
        pstAgentNode->pstAgentInfo->ulCallCnt++;
    }

    return pstAgentNode;
}


SC_AGENT_NODE_ST * sc_agent_select_by_caller(SC_AGENT_GRP_NODE_ST *pstGroupListNode, S8 *szCallerNum)
{
    SC_ACD_MEMORY_RELATION_QUEUE_NODE_ST *pstRelationQueueNode = NULL;
    SC_AGENT_NODE_ST *pstAgentNode      = NULL;
    HASH_NODE_S                *pstHashNode       = NULL;
    HASH_NODE_S                *pstAgentHashNode  = NULL;
    U32                         ulHashVal         = 0;
    U32                         ulAgentHashVal    = 0;
    U32                         ulAgentID          = 0;

    if (DOS_ADDR_INVALID(pstGroupListNode) || DOS_ADDR_INVALID(szCallerNum))
    {
        DOS_ASSERT(0);
        return NULL;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ACD), "Select agent by the calllerNum(%s). Start find agent in group %u. %p"
                    , szCallerNum, pstGroupListNode->ulGroupID, pstGroupListNode->pstRelationList);
    /* 根据主叫号码查找对应的坐席 */
    sc_agent_hash_func4calller_relation(szCallerNum, &ulHashVal);
    pstHashNode = hash_find_node(pstGroupListNode->pstRelationList, ulHashVal, szCallerNum, sc_agent_caller_relation_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ACD), "Not found the agent ulSiteID by CallerNum(%s), Group(%u)"
                        , szCallerNum
                        , pstGroupListNode->ulGroupID);

        goto process_call;
    }

    pstRelationQueueNode = pstHashNode->pHandle;
    ulAgentID = pstRelationQueueNode->ulAgentID;
    /* 根据坐席编号, 在坐席hash中查找到对应的坐席 */
    sc_agent_hash_func4agent(ulAgentID, &ulAgentHashVal);
    pstAgentHashNode = hash_find_node(g_pstAgentList, ulAgentHashVal, (VOID *)&ulAgentID, sc_agent_hash_find);
    if (DOS_ADDR_INVALID(pstAgentHashNode)
        || DOS_ADDR_INVALID(pstAgentHashNode->pHandle))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ACD), "Not found the agent.(Agent %u in Group %u)"
                    , ulAgentID
                    , pstGroupListNode->ulGroupID);

        goto process_call;
    }

    pstAgentNode = pstAgentHashNode->pHandle;
    if (!SC_ACD_SITE_IS_USEABLE(pstAgentNode->pstAgentInfo))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ACD), "There found an agent. But the agent is not useable.(Agent %u in Group %u)"
                , ulAgentID
                , pstGroupListNode->ulGroupID);

        goto process_call;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_ACD), "There found an agent.(Agent %u in Group %u)"
                , ulAgentID
                , pstGroupListNode->ulGroupID);

    return pstAgentNode;

process_call:
    pstAgentNode = sc_agent_select_by_call_count(pstGroupListNode);
    if (DOS_ADDR_VALID(pstAgentNode))
    {
        /* 添加或者更新 主叫号码和坐席对应关系的hash */
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_ACD), "Found an uaeable agent. Call Count: %d. (Agent %d in Group %d)"
                        , pstAgentNode->pstAgentInfo->ulCallCnt
                        , pstAgentNode->pstAgentInfo->ulAgentID
                        , pstGroupListNode->ulGroupID);

        if (DOS_ADDR_VALID(pstHashNode) && DOS_ADDR_VALID(pstHashNode->pHandle))
        {
            /* 更新 */
            pstRelationQueueNode->ulAgentID = pstAgentNode->pstAgentInfo->ulAgentID;

            goto end;
        }

        pstRelationQueueNode = (SC_ACD_MEMORY_RELATION_QUEUE_NODE_ST *)dos_dmem_alloc(sizeof(SC_ACD_MEMORY_RELATION_QUEUE_NODE_ST));
        if (DOS_ADDR_INVALID(pstRelationQueueNode))
        {
            DOS_ASSERT(0);
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_ACD), "%s", "Add CallerNum relationship fail. Alloc memory fail");

            goto end;
        }

        if (DOS_ADDR_INVALID(pstHashNode))
        {
            pstHashNode = (HASH_NODE_S *)dos_dmem_alloc(sizeof(HASH_NODE_S));
            if (DOS_ADDR_INVALID(pstHashNode))
            {
                DOS_ASSERT(0);

                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_ACD), "%s", "Add CallerNum relationship fail. Alloc memory fail");
                dos_dmem_free(pstRelationQueueNode);
                pstRelationQueueNode = NULL;

                goto end;
            }

            HASH_Init_Node(pstHashNode);
            pstHashNode->pHandle = NULL;
            hash_add_node(pstGroupListNode->pstRelationList, pstHashNode, ulHashVal, NULL);
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ACD), "add into hash, ulHashVal : %d, count : %d", ulHashVal, pstGroupListNode->pstRelationList[ulHashVal].NodeNum);
        }

        pstRelationQueueNode->ulAgentID = pstAgentNode->pstAgentInfo->ulAgentID;
        dos_strncpy(pstRelationQueueNode->szCallerNum, szCallerNum, SC_NUM_LENGTH);
        pstRelationQueueNode->szCallerNum[SC_NUM_LENGTH - 1] = '\0';
        pstHashNode->pHandle = pstRelationQueueNode;
    }

end:
    return pstAgentNode;
}


SC_AGENT_NODE_ST *sc_agent_get_by_id(U32 ulAgentID)
{
    HASH_NODE_S      *pstHashNode = NULL;
    SC_AGENT_NODE_ST *pstAgentNode = NULL;
    U32              ulHashIndex = 0;

    sc_agent_hash_func4agent(ulAgentID, &ulHashIndex);
    pstHashNode = hash_find_node(g_pstAgentList, ulHashIndex, &ulAgentID, sc_agent_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_ACD), "Cannot find the agent with this id %u.", ulAgentID);
        return NULL;
    }

    pstAgentNode = pstHashNode->pHandle;
    if (DOS_ADDR_INVALID(pstAgentNode) || DOS_ADDR_INVALID(pstAgentNode->pstAgentInfo))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_ACD), "Cannot find the agent with this id %u..", ulAgentID);
        return NULL;
    }

    if (pstAgentNode->pstAgentInfo->bWaitingDelete)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_ACD), "Agent will be delete later. %u", ulAgentID);
        return NULL;
    }

    return pstAgentNode;
}

SC_AGENT_NODE_ST *sc_agent_select_by_grpid(U32 ulGroupID, S8 *szCallerNum)
{
    SC_AGENT_NODE_ST     *pstAgentNode      = NULL;
    SC_AGENT_GRP_NODE_ST *pstGroupListNode  = NULL;
    HASH_NODE_S          *pstHashNode       = NULL;
    U32                  ulHashVal          = 0;

    if (0 == ulGroupID
        || U32_BUTT == ulGroupID)
    {
        DOS_ASSERT(0);
        return NULL;
    }

    sc_agent_hash_func4grp(ulGroupID, &ulHashVal);
    pthread_mutex_lock(&g_mutexGroupList);
    pstHashNode = hash_find_node(g_pstGroupList, ulHashVal , &ulGroupID, sc_agent_group_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_ACD), "Cannot fine the group with the ID \"%s\".", ulGroupID);
        pthread_mutex_unlock(&g_mutexGroupList);

        return NULL;
    }

    pstGroupListNode = pstHashNode->pHandle;

    pthread_mutex_lock(&pstGroupListNode->mutexSiteQueue);

    switch (pstGroupListNode->ucACDPolicy)
    {
        case SC_ACD_POLICY_IN_ORDER:
            pstAgentNode = sc_agent_select_by_inorder(pstGroupListNode);
            break;

        case SC_ACD_POLICY_MIN_CALL:
            pstAgentNode = sc_agent_select_by_call_count(pstGroupListNode);
            break;

        case SC_ACD_POLICY_RANDOM:
            pstAgentNode = sc_agent_select_by_random(pstGroupListNode);
            break;

        case SC_ACD_POLICY_RECENT:
        case SC_ACD_POLICY_GROUP:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_ACD), "Template not support policy %d", pstGroupListNode->ucACDPolicy);
            break;
        case SC_ACD_POLICY_MEMORY:
            pstAgentNode = sc_agent_select_by_caller(pstGroupListNode, szCallerNum);
            break;
        default:
            break;
    }

    pthread_mutex_unlock(&pstGroupListNode->mutexSiteQueue);

    if (DOS_ADDR_VALID(pstAgentNode)
        && DOS_ADDR_VALID(pstAgentNode->pstAgentInfo))
    {
        pstGroupListNode->usLastUsedAgent = pstAgentNode->ulID;
        dos_strncmp(pstGroupListNode->szLastEmpNo, pstAgentNode->pstAgentInfo->szEmpNo, SC_EMP_NUMBER_LENGTH);
        pstGroupListNode->szLastEmpNo[SC_EMP_NUMBER_LENGTH-1] = '\0';
        pstAgentNode->pstAgentInfo->stStat.ulCallCnt++;
        pstAgentNode->pstAgentInfo->stStat.ulSelectCnt++;

        pthread_mutex_unlock(&g_mutexGroupList);

        return pstAgentNode;
    }
    else
    {
        pthread_mutex_unlock(&g_mutexGroupList);

        return NULL;
    }
}

SC_AGENT_NODE_ST *sc_agent_get_by_userid(S8 *szUserID)
{
    U32               ulHashIndex         = 0;
    HASH_NODE_S       *pstHashNode        = NULL;
    SC_AGENT_NODE_ST  *pstAgentQueueNode  = NULL;
    SC_AGENT_INFO_ST  *pstAgentData       = NULL;

    if (DOS_ADDR_INVALID(szUserID)
        || szUserID[0] == '\0')
    {
        return NULL;
    }

    pthread_mutex_lock(&g_mutexAgentList);

    HASH_Scan_Table(g_pstAgentList, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstAgentList, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            if (DOS_ADDR_INVALID(pstHashNode) || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }
            pstAgentQueueNode = (SC_AGENT_NODE_ST *)pstHashNode->pHandle;
            pstAgentData = pstAgentQueueNode->pstAgentInfo;

            if (DOS_ADDR_INVALID(pstAgentData))
            {
                continue;
            }

            if (pstAgentData->ucBindType != AGENT_BIND_SIP)
            {
                continue;
            }

            if (dos_strcmp(pstAgentData->szUserID, szUserID) == 0)
            {
                pthread_mutex_unlock(&g_mutexAgentList);

                return pstAgentQueueNode;
            }
        }
    }

    pthread_mutex_unlock(&g_mutexAgentList);

    return NULL;
}

/**
 * 通过TT号获取坐席
 *
 * @param szTTNumber TT号
 *
 * @return 坐席指针
 */
SC_AGENT_NODE_ST *sc_agent_get_by_tt_num(S8 *szTTNumber)
{
    U32                         ulHashIndex         = 0;
    HASH_NODE_S                 *pstHashNode        = NULL;
    SC_AGENT_NODE_ST  *pstAgentQueueNode  = NULL;
    SC_AGENT_INFO_ST        *pstAgentData       = NULL;

    if (DOS_ADDR_INVALID(szTTNumber) || szTTNumber[0] == '\0')
    {
        DOS_ASSERT(0);
        return NULL;
    }

    pthread_mutex_lock(&g_mutexAgentList);

    HASH_Scan_Table(g_pstAgentList, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstAgentList, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            if (DOS_ADDR_INVALID(pstHashNode) || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }
            pstAgentQueueNode = (SC_AGENT_NODE_ST *)pstHashNode->pHandle;
            pstAgentData = pstAgentQueueNode->pstAgentInfo;

            if (DOS_ADDR_INVALID(pstAgentData))
            {
                continue;
            }

            if (dos_strcmp(pstAgentData->szTTNumber, szTTNumber) == 0)
            {
                pthread_mutex_unlock(&g_mutexAgentList);

                return pstAgentQueueNode;
            }
        }
    }

    pthread_mutex_unlock(&g_mutexAgentList);

    return NULL;
}

SC_AGENT_NODE_ST *sc_agent_get_by_emp_num(U32 ulCustomID, S8 *pszEmpNum)
{
    U32                 ulHashIndex         = 0;
    HASH_NODE_S         *pstHashNode        = NULL;
    SC_AGENT_NODE_ST    *pstAgentQueueNode  = NULL;
    SC_AGENT_INFO_ST    *pstAgentData       = NULL;

    if (DOS_ADDR_INVALID(pszEmpNum)
        || pszEmpNum[0] == '\0')
    {
        return NULL;
    }

    pthread_mutex_lock(&g_mutexAgentList);

    HASH_Scan_Table(g_pstAgentList, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstAgentList, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            if (DOS_ADDR_INVALID(pstHashNode) || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }
            pstAgentQueueNode = (SC_AGENT_NODE_ST *)pstHashNode->pHandle;
            pstAgentData = pstAgentQueueNode->pstAgentInfo;

            if (DOS_ADDR_INVALID(pstAgentData))
            {
                continue;
            }

            if (ulCustomID == pstAgentData->ulCustomerID
                && dos_strcmp(pstAgentData->szEmpNo, pszEmpNum) == 0)
            {
                pthread_mutex_unlock(&g_mutexAgentList);

                return pstAgentQueueNode;
            }
        }
    }

    pthread_mutex_unlock(&g_mutexAgentList);

    return NULL;
}

U32 sc_agent_get_processing_time(U32 ulAgentID)
{
    SC_AGENT_NODE_ST  *pstAgentQueueNode  = NULL;
    HASH_NODE_S            *pstHashNode   = NULL;
    U32                     ulHashIndex   = 0;
    U32                     ulProcessTime = 0;

    pthread_mutex_lock(&g_mutexAgentList);
    sc_agent_hash_func4agent(ulAgentID, &ulHashIndex);
    pstHashNode = hash_find_node(g_pstAgentList, ulHashIndex, &ulAgentID, sc_agent_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        pthread_mutex_unlock(&g_mutexAgentList);

        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_ACD), "Cannot find the agent with this id %u.", ulAgentID);
        goto finished;
    }
    pthread_mutex_unlock(&g_mutexAgentList);

    if (DOS_ADDR_INVALID(pstHashNode))
    {
        pthread_mutex_unlock(&g_mutexAgentList);

        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_ACD), "Empty hash node for this agent. %u", ulAgentID);
        goto finished;
    }

    pstAgentQueueNode = pstHashNode->pHandle;
    if (DOS_ADDR_INVALID(pstAgentQueueNode)
        || DOS_ADDR_INVALID(pstAgentQueueNode->pstAgentInfo))
    {
        pthread_mutex_unlock(&g_mutexAgentList);

        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_ACD), "The hash node is empty for this agent. %u", ulAgentID);
        goto finished;
    }

    ulProcessTime = pstAgentQueueNode->pstAgentInfo->ucProcesingTime;
    pthread_mutex_unlock(&g_mutexAgentList);

finished:
    return ulProcessTime;
}


U32 sc_agent_group_has_idel_agent(U32 ulAgentGrpID, BOOL *pblResult)
{
    SC_AGENT_NODE_ST *pstAgentNode      = NULL;
    SC_AGENT_GRP_NODE_ST    *pstGroupListNode  = NULL;
    HASH_NODE_S                *pstHashNode       = NULL;
    DLL_NODE_S                 *pstDLLNode        = NULL;
    U32                        ulHashVal          = 0;

    if (DOS_ADDR_INVALID(pblResult))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    *pblResult = DOS_FALSE;


    sc_agent_hash_func4grp(ulAgentGrpID, &ulHashVal);
    pthread_mutex_lock(&g_mutexGroupList);
    pstHashNode = hash_find_node(g_pstGroupList, ulHashVal , &ulAgentGrpID, sc_agent_group_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_ACD), "Cannot fine the group with the ID \"%u\" .", ulAgentGrpID);
        pthread_mutex_unlock(&g_mutexGroupList);

        return DOS_FAIL;
    }

    pstGroupListNode = pstHashNode->pHandle;

    pthread_mutex_lock(&pstGroupListNode->mutexSiteQueue);

    DLL_Scan(&pstGroupListNode->stAgentList, pstDLLNode, DLL_NODE_S*)
    {
        if (DOS_ADDR_INVALID(pstDLLNode)
            || DOS_ADDR_INVALID(pstDLLNode->pHandle))
        {
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ACD), "Group List node has no data. Maybe the data has been deleted. Group: %u."
                            , pstGroupListNode->ulGroupID);
            continue;
        }

        pstAgentNode = pstDLLNode->pHandle;
        if (DOS_ADDR_INVALID(pstAgentNode)
            || DOS_ADDR_INVALID(pstAgentNode->pstAgentInfo))
        {
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ACD), "Group List node has no data. Maybe the data has been deleted. Group: %u."
                            , pstGroupListNode->ulGroupID);
            continue;
        }

        if (pstAgentNode->pstAgentInfo->ulLastIdelTime
            && (time(NULL) - pstAgentNode->pstAgentInfo->ulLastIdelTime < 3))
        {
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ACD), "Agent is in protect Agent: %u. Group: %u."
                            , pstAgentNode->pstAgentInfo->ulAgentID
                            , pstGroupListNode->ulGroupID);
            continue;
        }

        if (SC_ACD_SITE_IS_USEABLE(pstAgentNode->pstAgentInfo))
        {
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ACD), "Found an useable agent. (Agent %u in Group %u)"
                        , pstAgentNode->pstAgentInfo->ulAgentID
                        , pstGroupListNode->ulGroupID);

            *pblResult = DOS_TRUE;
            break;
        }
    }

    pthread_mutex_unlock(&pstGroupListNode->mutexSiteQueue);

    pthread_mutex_unlock(&g_mutexGroupList);

    return DOS_SUCC;

}

U32 sc_agent_group_agent_count(U32 ulGroupID)
{
    U32 ulCnt = 0;

    SC_AGENT_GRP_NODE_ST    *pstGroupListNode  = NULL;
    HASH_NODE_S                *pstHashNode       = NULL;
    U32                        ulHashVal          = 0;


    sc_agent_hash_func4grp(ulGroupID, &ulHashVal);
    pthread_mutex_lock(&g_mutexGroupList);
    pstHashNode = hash_find_node(g_pstGroupList, ulHashVal , &ulGroupID, sc_agent_group_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_ACD), "Cannot fine the group with the ID \"%u\" .", ulGroupID);
        pthread_mutex_unlock(&g_mutexGroupList);

        return 0;
    }

    pstGroupListNode = pstHashNode->pHandle;

    pthread_mutex_lock(&pstGroupListNode->mutexSiteQueue);

    ulCnt = pstGroupListNode->stAgentList.ulCount;

    pthread_mutex_unlock(&pstGroupListNode->mutexSiteQueue);

    pthread_mutex_unlock(&g_mutexGroupList);

    return ulCnt;

}


U32 sc_agent_group_stat_by_id(U32 ulGroupID, U32 *pulTotal, U32 *pulWorking, U32 *pulIdel, U32 *pulBusy)
{
    U32 ulCnt = 0;

    SC_AGENT_NODE_ST *pstAgentNode      = NULL;
    SC_AGENT_GRP_NODE_ST    *pstGroupListNode  = NULL;
    HASH_NODE_S                *pstHashNode       = NULL;
    DLL_NODE_S                 *pstDLLNode        = NULL;
    U32                        ulHashVal          = 0;

    if (DOS_ADDR_VALID(pulTotal))
    {
        *pulTotal = 0;
    }

    if (DOS_ADDR_VALID(pulIdel))
    {
        *pulIdel = 0;
    }

    if (DOS_ADDR_VALID(pulWorking))
    {
        *pulWorking = 0;
    }

    if (DOS_ADDR_VALID(pulBusy))
    {
        *pulBusy = 0;
    }

    sc_agent_hash_func4grp(ulGroupID, &ulHashVal);
    pthread_mutex_lock(&g_mutexGroupList);
    pstHashNode = hash_find_node(g_pstGroupList, ulHashVal , &ulGroupID, sc_agent_group_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_ACD), "Cannot fine the group with the ID \"%u\" .", ulGroupID);
        pthread_mutex_unlock(&g_mutexGroupList);

        return 0;
    }

    pstGroupListNode = pstHashNode->pHandle;

    pthread_mutex_lock(&pstGroupListNode->mutexSiteQueue);

    DLL_Scan(&pstGroupListNode->stAgentList, pstDLLNode, DLL_NODE_S*)
    {
        if (DOS_ADDR_INVALID(pstDLLNode)
            || DOS_ADDR_INVALID(pstDLLNode->pHandle))
        {
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ACD), "Group List node has no data. Maybe the data has been deleted. Group: %u."
                            , pstGroupListNode->ulGroupID);
            continue;
        }

        pstAgentNode = pstDLLNode->pHandle;
        if (DOS_ADDR_INVALID(pstAgentNode)
            || DOS_ADDR_INVALID(pstAgentNode->pstAgentInfo))
        {
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_ACD), "Group List node has no data. Maybe the data has been deleted. Group: %u."
                            , pstGroupListNode->ulGroupID);
            continue;
        }

        if (pstAgentNode->pstAgentInfo->bWaitingDelete)
        {
            continue;
        }

        if (DOS_ADDR_VALID(pulTotal))
        {
            (*pulTotal)++;
        }

        if (SC_ACD_OFFLINE == pstAgentNode->pstAgentInfo->ucStatus)
        {
            continue;
        }

        if (DOS_ADDR_VALID(pulWorking))
        {
            (*pulWorking)++;
        }

        if (SC_ACD_IDEL == pstAgentNode->pstAgentInfo->ucStatus)
        {
            /* 这个地方保护一下 */
            if (pstAgentNode->pstAgentInfo->ulLastIdelTime
                && time(NULL) - pstAgentNode->pstAgentInfo->ulLastIdelTime < 3)
            {
                if (DOS_ADDR_VALID(pulBusy))
                {
                    (*pulBusy)++;
                }
            }
            else
            {
                if (DOS_ADDR_VALID(pulIdel))
                {
                    (*pulIdel)++;
                }
            }
        }
        else
        {
            if (DOS_ADDR_VALID(pulBusy))
            {
                (*pulBusy)++;
            }
        }
    }

    pthread_mutex_unlock(&pstGroupListNode->mutexSiteQueue);

    pthread_mutex_unlock(&g_mutexGroupList);

    return ulCnt;

}


U32 sc_agent_set_busy(SC_AGENT_INFO_ST *pstAgentQueueInfo, U32 ulOperatingType)
{
    U32 ulOldStatus;

    if (DOS_ADDR_INVALID(pstAgentQueueInfo))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulOldStatus = pstAgentQueueInfo->ucStatus;

    switch (pstAgentQueueInfo->ucStatus)
    {
        case SC_ACD_OFFLINE:
        case SC_ACD_IDEL:
        case SC_ACD_AWAY:
            pstAgentQueueInfo->ucStatus = SC_ACD_BUSY;
            break;

        case SC_ACD_BUSY:
            break;
        case SC_ACD_PROC:
            break;
        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_ACD), "Agent %u is in an invalid status.", pstAgentQueueInfo->ulAgentID);
            return DOS_FAIL;
    }

    if (ulOldStatus != pstAgentQueueInfo->ucStatus)
    {
        sc_agent_status_notify(pstAgentQueueInfo, ACD_MSG_SUBTYPE_BUSY);
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Request set agnet status to busy. Agent: %u Old status: %u, Current status: %u"
                    , pstAgentQueueInfo->ulAgentID, ulOldStatus, pstAgentQueueInfo->ucStatus);

    return DOS_SUCC;
}

U32 sc_agent_set_idle(SC_AGENT_INFO_ST *pstAgentQueueInfo, U32 ulOperatingType)
{
    U32 ulOldStatus;

    if (DOS_ADDR_INVALID(pstAgentQueueInfo))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulOldStatus = pstAgentQueueInfo->ucStatus;

    switch (pstAgentQueueInfo->ucStatus)
    {
        case SC_ACD_OFFLINE:
            pstAgentQueueInfo->ucStatus = SC_ACD_IDEL;
            break;
        case SC_ACD_IDEL:
            break;

        case SC_ACD_AWAY:
            pstAgentQueueInfo->ucStatus = SC_ACD_IDEL;
            break;

        case SC_ACD_BUSY:
            /* TODO */
            //if (!sc_ep_chack_has_call4agent(pstAgentQueueInfo->usSCBNo))
            {
                pstAgentQueueInfo->ucStatus = SC_ACD_IDEL;
            }

            break;

        case SC_ACD_PROC:
            break;

        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Agent %u is in an invalid status.", pstAgentQueueInfo->ulAgentID);
            return DOS_FAIL;
            break;
    }

    if (ulOldStatus != pstAgentQueueInfo->ucStatus)
    {
        sc_agent_status_notify(pstAgentQueueInfo, ACD_MSG_SUBTYPE_IDLE);
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Request set agnet status to idle.Agent: %u, Old status: %u, Current status: %u"
                    , pstAgentQueueInfo->ulAgentID, ulOldStatus, pstAgentQueueInfo->ucStatus);

    return DOS_SUCC;
}

U32 sc_agent_set_rest(SC_AGENT_INFO_ST *pstAgentQueueInfo, U32 ulOperatingType)
{
    U32 ulOldStatus;

    if (DOS_ADDR_INVALID(pstAgentQueueInfo))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulOldStatus = pstAgentQueueInfo->ucStatus;

    switch (pstAgentQueueInfo->ucStatus)
    {
        case SC_ACD_OFFLINE:
            pstAgentQueueInfo->ucStatus = SC_ACD_AWAY;
            break;
        case SC_ACD_IDEL:
            pstAgentQueueInfo->ucStatus = SC_ACD_AWAY;
            break;

        case SC_ACD_AWAY:
            break;

        case SC_ACD_BUSY:
            /* TODO 在没有呼叫之前 都应该不让设置 */
            //if (!sc_ep_chack_has_call4agent(pstAgentQueueInfo->usSCBNo))
            {
                pstAgentQueueInfo->ucStatus = SC_ACD_AWAY;
            }
            break;

        case SC_ACD_PROC:
            break;

        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Agent %u is in an invalid status.", pstAgentQueueInfo->ulAgentID);
            return DOS_FAIL;
            break;
    }

    if (ulOldStatus != pstAgentQueueInfo->ucStatus)
    {
        sc_agent_status_notify(pstAgentQueueInfo, ACD_MSG_SUBTYPE_AWAY);
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Request set agnet status to rest. Agent:%u, Old status: %u, Current status: %u"
                    , pstAgentQueueInfo->ulAgentID, ulOldStatus, pstAgentQueueInfo->ucStatus);

    return DOS_SUCC;
}

U32 sc_agent_set_signin(SC_AGENT_NODE_ST *pstAgentNode, U32 ulOperatingType)
{
    U32 ulOldStatus;
    SC_AGENT_INFO_ST   *pstAgentInfo = NULL;     /* 坐席信息 */

    if (DOS_ADDR_INVALID(pstAgentNode)
        || DOS_ADDR_INVALID(pstAgentNode->pstAgentInfo))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstAgentInfo = pstAgentNode->pstAgentInfo;
    ulOldStatus = pstAgentInfo->ucStatus;

    if (pstAgentInfo->ulLastSignInTime)
    {
        sc_agent_stat(SC_AGENT_STAT_SIGNIN, pstAgentInfo, pstAgentInfo->ulAgentID, 0);
    }
    pstAgentInfo->ulLastSignInTime = 0;

    /* 发起长签 */
    if (sc_agent_signin_proc(pstAgentNode) != DOS_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Request set agnet status to signin FAIL. Agent: %u, Old status: %u, Current status: %u"
                        , pstAgentInfo->ulAgentID, ulOldStatus, pstAgentInfo->ucStatus);

        return DOS_FAIL;
    }

    switch (pstAgentInfo->ucStatus)
    {
        case SC_ACD_OFFLINE:
            pstAgentInfo->ucStatus = SC_ACD_IDEL;
            break;
        case SC_ACD_IDEL:
            break;

        case SC_ACD_AWAY:
            pstAgentInfo->ucStatus = SC_ACD_IDEL;
            break;

        case SC_ACD_BUSY:
            /* 这个地方不应该置为IDEL状态，置为IDEL状态，会再次分配呼叫 */
            /*pstAgentQueueInfo->ucStatus = SC_ACD_IDEL;*/
            break;

        case SC_ACD_PROC:
            break;

        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Agent %u is in an invalid status.", pstAgentInfo->ulAgentID);
            return DOS_FAIL;
            break;
    }

    pstAgentInfo->bNeedConnected = DOS_TRUE;

    if (ulOldStatus != pstAgentInfo->ucStatus)
    {
        sc_agent_status_notify(pstAgentInfo, pstAgentInfo->ucStatus);
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Request set agnet status to signin. Agent: %u, Old status: %u, Current status: %u"
                    , pstAgentInfo->ulAgentID, ulOldStatus, pstAgentInfo->ucStatus);

    return DOS_SUCC;
}

U32 sc_agent_set_signout(SC_AGENT_INFO_ST *pstAgentQueueInfo, U32 ulOperatingType)
{
    U32 ulOldStatus;
    SC_SRV_CB *pstSCB       = NULL;
    SC_LEG_CB *pstLegCB     = NULL;

    if (DOS_ADDR_INVALID(pstAgentQueueInfo))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulOldStatus = pstAgentQueueInfo->ucStatus;

    sc_agent_stat(SC_AGENT_STAT_SIGNOUT, pstAgentQueueInfo, pstAgentQueueInfo->ulAgentID, 0);
    switch (pstAgentQueueInfo->ucStatus)
    {
        case SC_ACD_OFFLINE:
            pstAgentQueueInfo->ucStatus = SC_ACD_IDEL;
            break;
        case SC_ACD_IDEL:
            break;

        case SC_ACD_AWAY:
            pstAgentQueueInfo->ucStatus = SC_ACD_IDEL;
            break;

        case SC_ACD_BUSY:
            /* 这个地方不应该置为IDEL状态，置为IDEL状态，会再次分配呼叫 */
            /*pstAgentQueueInfo->ucStatus = SC_ACD_IDEL; */
            break;

        case SC_ACD_PROC:
            break;

        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Agent %u is in an invalid status.", pstAgentQueueInfo->ulAgentID);
            return DOS_FAIL;
            break;
    }

    /* 只要有呼叫都拆 */
    if ((pstAgentQueueInfo->bConnected || pstAgentQueueInfo->bNeedConnected)
        && pstAgentQueueInfo->ulLegNo <= SC_LEG_CB_SIZE)
    {
        /* 拆呼叫 */
        pstLegCB = sc_lcb_get(pstAgentQueueInfo->ulLegNo);
        if (DOS_ADDR_INVALID(pstLegCB))
        {

        }
        else
        {
            pstSCB = sc_scb_get(pstLegCB->ulIndSCBNo);
            if (DOS_ADDR_INVALID(pstSCB))
            {
                sc_lcb_free(pstLegCB);
                pstLegCB = NULL;
            }
            else
            {
                pstSCB->stSigin.stSCBTag.usStatus = SC_SIGIN_RELEASE;
                /* 先执行playback_stop，否者hungup执行不到 */
                sc_req_playback_stop(pstLegCB->ulIndSCBNo, pstAgentQueueInfo->ulLegNo);
                if (sc_req_hungup(pstLegCB->ulIndSCBNo, pstAgentQueueInfo->ulLegNo, CC_ERR_NORMAL_CLEAR) != DOS_SUCC)
                {
                    sc_scb_free(pstSCB);
                    pstSCB = NULL;
                    sc_lcb_free(pstLegCB);
                    pstLegCB = NULL;
                }
            }
        }
    }

    pstAgentQueueInfo->bNeedConnected = DOS_FALSE;

    if (ulOldStatus != pstAgentQueueInfo->ucStatus)
    {
        sc_agent_status_notify(pstAgentQueueInfo, ACD_MSG_SUBTYPE_SIGNOUT);
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Request set agnet status to signout. Agent: %u, Old status: %u, Current status: %u"
                , pstAgentQueueInfo->ulAgentID, ulOldStatus, pstAgentQueueInfo->ucStatus);

    return DOS_SUCC;
}

U32 sc_agent_set_login(SC_AGENT_INFO_ST *pstAgentQueueInfo, U32 ulOperatingType)
{
    U32 ulOldStatus;

    if (DOS_ADDR_INVALID(pstAgentQueueInfo))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulOldStatus = pstAgentQueueInfo->ucStatus;

    sc_agent_stat(SC_AGENT_STAT_ONLINE, pstAgentQueueInfo, pstAgentQueueInfo->ulAgentID, 0);

    switch (pstAgentQueueInfo->ucStatus)
    {
        case SC_ACD_OFFLINE:
            pstAgentQueueInfo->ucStatus = SC_ACD_AWAY;
            break;

        case SC_ACD_IDEL:
            break;

        case SC_ACD_AWAY:
            break;

        case SC_ACD_BUSY:
            break;

        case SC_ACD_PROC:
            break;

        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Agent %u is in an invalid status.", pstAgentQueueInfo->ulAgentID);
            return DOS_FAIL;
            break;
    }

    if (ulOldStatus != pstAgentQueueInfo->ucStatus)
    {
        sc_agent_status_notify(pstAgentQueueInfo, ACD_MSG_SUBTYPE_AWAY);
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Request set agnet status to login. Agent: %u, Old status: %u, Current status: %u"
                    , pstAgentQueueInfo->ulAgentID, ulOldStatus, pstAgentQueueInfo->ucStatus);

    return DOS_SUCC;
}

VOID sc_agent_set_logout(U64 p)
{
    U32                  ulOldStatus;
    SC_AGENT_INFO_ST *pstAgentQueueInfo = (SC_AGENT_INFO_ST *)p;

    if (DOS_ADDR_INVALID(pstAgentQueueInfo))
    {
        DOS_ASSERT(0);
        return;
    }

    /* 只有在长签了，才拆除呼叫 */
    if (pstAgentQueueInfo->bConnected)
    {
        /* 拆除呼叫 */
        pstAgentQueueInfo->bNeedConnected = DOS_FALSE;
    }

    ulOldStatus = pstAgentQueueInfo->ucStatus;

    sc_agent_stat(SC_AGENT_STAT_ONLINE, pstAgentQueueInfo, pstAgentQueueInfo->ulAgentID, 0);

    switch (pstAgentQueueInfo->ucStatus)
    {
        case SC_ACD_OFFLINE:
            pstAgentQueueInfo->ucStatus = SC_ACD_OFFLINE;
            break;

        case SC_ACD_IDEL:
            pstAgentQueueInfo->ucStatus = SC_ACD_OFFLINE;
            break;

        case SC_ACD_AWAY:
            pstAgentQueueInfo->ucStatus = SC_ACD_OFFLINE;
            break;

        case SC_ACD_BUSY:
            pstAgentQueueInfo->ucStatus = SC_ACD_OFFLINE;
            break;

        case SC_ACD_PROC:
            pstAgentQueueInfo->ucStatus = SC_ACD_OFFLINE;
            break;

        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Agent %u is in an invalid status.", pstAgentQueueInfo->ulAgentID);
            break;
    }

    /* 如果 bConnected 为true，应该等待其变为false, 再将坐席的状态改为登出 */
    if (pstAgentQueueInfo->bConnected)
    {
        return;
    }

    if (DOS_ADDR_VALID(pstAgentQueueInfo->htmrLogout))
    {
        dos_tmr_stop(&pstAgentQueueInfo->htmrLogout);
        pstAgentQueueInfo->htmrLogout = NULL;
    }

    if (ulOldStatus != pstAgentQueueInfo->ucStatus)
    {
        sc_agent_status_notify(pstAgentQueueInfo, ACD_MSG_SUBTYPE_LOGINOUT);
    }

    pstAgentQueueInfo->bNeedConnected = DOS_FALSE;
    pstAgentQueueInfo->bConnected = DOS_FALSE;

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Request set agnet status to logout. Agent:%u Old status: %u, Current status: %u"
                , pstAgentQueueInfo->ulAgentID, ulOldStatus, pstAgentQueueInfo->ucStatus);
}

U32 sc_agent_set_force_logout(SC_AGENT_INFO_ST *pstAgentQueueInfo, U32 ulOperatingType)
{
    U32 ulOldStatus;

    if (DOS_ADDR_INVALID(pstAgentQueueInfo))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulOldStatus = pstAgentQueueInfo->ucStatus;

    switch (pstAgentQueueInfo->ucStatus)
    {
        case SC_ACD_OFFLINE:
            pstAgentQueueInfo->ucStatus = SC_ACD_OFFLINE;
            break;

        case SC_ACD_IDEL:
            pstAgentQueueInfo->ucStatus = SC_ACD_OFFLINE;
            break;

        case SC_ACD_AWAY:
            pstAgentQueueInfo->ucStatus = SC_ACD_OFFLINE;
            break;

        case SC_ACD_BUSY:
            pstAgentQueueInfo->ucStatus = SC_ACD_OFFLINE;
            break;

        case SC_ACD_PROC:
            pstAgentQueueInfo->ucStatus = SC_ACD_OFFLINE;
            break;

        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Agent %u is in an invalid status.", pstAgentQueueInfo->ulAgentID);
            return DOS_FAIL;
    }

    /* 只要有呼叫都拆 */
    if (pstAgentQueueInfo->bConnected || pstAgentQueueInfo->ulLegNo != U32_BUTT)
    {
        /*  拆除呼叫 */
    }

    if (ulOldStatus != pstAgentQueueInfo->ucStatus)
    {

        sc_agent_status_notify(pstAgentQueueInfo, ACD_MSG_SUBTYPE_LOGINOUT);
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Request force logout for agnet %u. Old status: %u, Current status: %u"
                , pstAgentQueueInfo->ulAgentID, ulOldStatus, pstAgentQueueInfo->ucStatus);

    return DOS_SUCC;

}


U32 sc_agent_group_http_update_proc(U32 ulAction, U32 ulGrpID)
{
    if (ulAction > SC_ACD_SITE_ACTION_BUTT)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch(ulAction)
    {
        case SC_API_CMD_ACTION_AGENTGREP_ADD:
        case SC_API_CMD_ACTION_AGENTGREP_UPDATE:
            sc_agent_group_init(ulGrpID);
            break;

        case SC_API_CMD_ACTION_AGENTGREP_DELETE:
            sc_agent_group_delete(ulGrpID);
            break;

        default:
            break;
    }
    return DOS_SUCC;
}

/**
 * 函数: U32 sc_acd_http_agent_update_proc(U32 ulAction, U32 ulAgentID, S8 *pszUserID)
 * 功能: 处理HTTP发过来的命令
 * 参数:
 *      U32 ulAction : 命令
 *      U32 ulAgentID : 坐席ID
 *      S8 *pszUserID : 坐席SIP User ID
 * 返回值: 成功返回DOS_SUCC,否则返回DOS_FAIL
 **/
U32 sc_agent_http_update_proc(U32 ulAction, U32 ulAgentID, S8 *pszUserID)
{
    switch (ulAction)
    {
        case SC_ACD_SITE_ACTION_DELETE:
        case SC_ACD_SITE_ACTION_SIGNIN:
        case SC_ACD_SITE_ACTION_SIGNOUT:
        case SC_ACD_SITE_ACTION_ONLINE:
        case SC_ACD_SITE_ACTION_OFFLINE:
        case SC_ACD_SITE_ACTION_EN_QUEUE:
        case SC_ACD_SITE_ACTION_DN_QUEUE:
            break;
        case SC_ACD_SITE_ACTION_ADD:
        case SC_ACD_SITE_ACTION_UPDATE:
            sc_agent_init(ulAgentID);
            break;
        case SC_API_CMD_ACTION_QUERY:
            break;
        default:
            DOS_ASSERT(0);
            return DOS_FAIL;
            break;
    }

    return DOS_SUCC;
}


U32 sc_agent_status_update(U32 ulAction, U32 ulAgentID, U32 ulOperatingType)
{
    SC_AGENT_NODE_ST  *pstAgentQueueNode  = NULL;
    SC_AGENT_INFO_ST   *pstAgentInfo = NULL;     /* 坐席信息 */
    HASH_NODE_S            *pstHashNode = NULL;
    U32                     ulHashIndex = 0;
    U32                     ulResult    = DOS_FAIL;
    U32                     ulOldStatus;


    pthread_mutex_lock(&g_mutexAgentList);
    sc_agent_hash_func4agent(ulAgentID, &ulHashIndex);
    pstHashNode = hash_find_node(g_pstAgentList, ulHashIndex, &ulAgentID, sc_agent_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        pthread_mutex_unlock(&g_mutexAgentList);

        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_ACD), "Cannot find the agent with this id %u.", ulAgentID);
        return DOS_FAIL;
    }
    pthread_mutex_unlock(&g_mutexAgentList);

    if (DOS_ADDR_INVALID(pstHashNode))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_ACD), "Empty hash node for this agent. %u", ulAgentID);
        return DOS_FAIL;
    }

    pstAgentQueueNode = pstHashNode->pHandle;
    if (DOS_ADDR_INVALID(pstAgentQueueNode)
        || DOS_ADDR_INVALID(pstAgentQueueNode->pstAgentInfo))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_ACD), "The hash node is empty for this agent. %u", ulAgentID);
        return DOS_FAIL;
    }

    pstAgentInfo = pstAgentQueueNode->pstAgentInfo;
    ulOldStatus = pstAgentInfo->ucStatus;
    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_ACD), "Agent status changed. Agent: %u, Action: %u", ulAgentID, ulAction);

    switch(ulAction)
    {
        case SC_ACTION_AGENT_BUSY:
            ulResult = sc_agent_set_busy(pstAgentInfo, ulOperatingType);
            break;

        case SC_ACTION_AGENT_IDLE:
            ulResult = sc_agent_set_idle(pstAgentInfo, ulOperatingType);
            break;

        case SC_ACTION_AGENT_REST:
            ulResult = sc_agent_set_rest(pstAgentInfo, ulOperatingType);
            break;

        case SC_ACTION_AGENT_SIGNIN:
            ulResult = sc_agent_set_signin(pstAgentQueueNode, ulOperatingType);
            break;

        case SC_ACTION_AGENT_SIGNOUT:
            ulResult = sc_agent_set_signout(pstAgentInfo, ulOperatingType);
            break;

        case SC_ACTION_AGENT_LOGIN:
            if (pstAgentInfo->htmrLogout)
            {
                dos_tmr_stop(&pstAgentInfo->htmrLogout);
                pstAgentInfo->htmrLogout = NULL;
            }
            ulResult = sc_agent_set_login(pstAgentInfo, ulOperatingType);
            break;

        case SC_ACTION_AGENT_LOGOUT:
            if (pstAgentInfo->htmrLogout)
            {
                dos_tmr_stop(&pstAgentInfo->htmrLogout);
                pstAgentInfo->htmrLogout = NULL;
            }

            ulResult = dos_tmr_start(&pstAgentInfo->htmrLogout, 2000, sc_agent_set_logout, (U64)pstAgentInfo, TIMER_NORMAL_LOOP);
            break;

        case SC_ACTION_AGENT_FORCE_OFFLINE:
            ulResult = sc_agent_set_force_logout(pstAgentInfo, ulOperatingType);
            break;

        case SC_ACTION_AGENT_QUERY:
            ulResult = sc_agent_status_http_get_rsp(ulAgentID);
            break;

        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_ACD), "Invalid action for agent. Action:%u", ulAction);
            ulResult = DOS_FAIL;
    }

    if (ulOldStatus != pstAgentInfo->ucStatus
        && (ulOldStatus <= SC_ACD_IDEL || pstAgentInfo->ucStatus <= SC_ACD_IDEL))
    {
        /* 状态不相同，并且不全部是 忙 的状态，则修改数据库 */
        sc_agent_update_status_db(ulAgentID, pstAgentInfo->ucStatus, pstAgentInfo->bConnected);
    }

    return ulResult;
}


/* 记录坐席统计信息 */
U32 sc_agent_stat_save(SC_AGENT_INFO_ST *pstAgentInfo)
{
    S8  szSQL[512]        = { 0, };
    U32 ulAvgCallDruation = 0;
    U32 i;

    if (DOS_ADDR_INVALID(pstAgentInfo))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (pstAgentInfo->stStat.ulCallCnt)
    {
        ulAvgCallDruation = pstAgentInfo->stStat.ulTotalDuration / pstAgentInfo->stStat.ulCallCnt;
    }
    else
    {
        ulAvgCallDruation = 0;
    }

    for (i=0; i<2; i++)
    {
        if (0 == pstAgentInfo->aulGroupID[i] || U32_BUTT == pstAgentInfo->aulGroupID[i])
        {
            continue;
        }

        dos_snprintf(szSQL, sizeof(szSQL),
                        "INSERT INTO tbl_stat_agents(ctime, type, bid, job_number, group_id, calls, "
                        "calls_connected, total_duration, online_time, avg_call_duration) VALUES("
                        "%u, %u, %u, \"%s\", %u, %u, %u, %u, %u, %u)"
                    , time(NULL), 0, pstAgentInfo->ulAgentID, pstAgentInfo->szEmpNo, pstAgentInfo->aulGroupID[i]
                    , pstAgentInfo->stStat.ulCallCnt, pstAgentInfo->stStat.ulCallConnected
                    , pstAgentInfo->stStat.ulTotalDuration, pstAgentInfo->stStat.ulTimesOnline
                    , ulAvgCallDruation);

        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ACD), "Save agent stat. agent: %u(%s), group: %u, callcnt: %u, call connected: %u, duration: %u, total time: %u"
                    , pstAgentInfo->ulAgentID, pstAgentInfo->szEmpNo, pstAgentInfo->aulGroupID[i]
                    , pstAgentInfo->stStat.ulCallCnt, pstAgentInfo->stStat.ulCallConnected
                    , pstAgentInfo->stStat.ulTotalDuration, pstAgentInfo->stStat.ulTimesOnline);

        if (db_query(g_pstSCDBHandle, szSQL, NULL, NULL, NULL) < 0)
        {
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_ACD), "Save agent stat fail.Agent:%u", pstAgentInfo->ulAgentID);
        }
        else
        {
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ACD), "Save agent stat succ.Agent:%u", pstAgentInfo->ulAgentID);
        }
    }

    dos_memzero(&pstAgentInfo->stStat, sizeof(pstAgentInfo->stStat));

    return DOS_SUCC;
}

U32 sc_agent_stat_audit(U32 ulCycle, VOID *ptr)
{
    HASH_NODE_S                 *pstHashNode       = NULL;
    SC_AGENT_NODE_ST  *pstAgentQueueNode = NULL;
    SC_AGENT_INFO_ST        *pstAgentInfo      = NULL;
    U32             ulHashIndex  = 0;

    HASH_Scan_Table(g_pstAgentList, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstAgentList, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            if (DOS_ADDR_INVALID(pstHashNode))
            {
                DOS_ASSERT(0);
                continue;
            }

            pstAgentQueueNode = pstHashNode->pHandle;
            if (DOS_ADDR_INVALID(pstAgentQueueNode)
                || DOS_ADDR_INVALID(pstAgentQueueNode->pstAgentInfo))
            {
                /* 有可能被删除了，不需要断言 */
                continue;
            }

            pstAgentInfo = pstAgentQueueNode->pstAgentInfo;
            if (pstAgentInfo->bWaitingDelete)
            {
                /* 被删除了*/
                continue;
            }

            /* 如果整理时间超过3分钟，就处理一下 */
            if (pstAgentInfo->ucStatus == SC_ACD_PROC
                && pstAgentInfo->ulLastProcTime != 0
                && time(NULL) - pstAgentInfo->ulLastProcTime > 3 * 60)
            {
                pstAgentInfo->ucStatus = SC_ACD_IDEL;
                sc_agent_status_notify(pstAgentInfo, pstAgentInfo->ucStatus);
            }
        }
    }

    return DOS_SUCC;
}


/*
 * 函  数: sc_acd_agent_stat
 * 功  能: 统计坐席呼叫信息
 * 参  数:
 *       U32 ulType : 统计类型
 *       U32 ulAgentID : 坐席ID
 *       SC_AGENT_INFO_ST *pstAgentInfo: 坐席结构
 * 返回值: 成功返回DOS_SUCC，否则返回DOS_FAIL
 **/
U32 sc_agent_stat(U32 ulType, SC_AGENT_INFO_ST *pstAgentInfo, U32 ulAgentID, U32 ulParam)
{
    SC_AGENT_NODE_ST  *pstAgentQueueNode  = NULL;
    HASH_NODE_S            *pstHashNode = NULL;
    U32                     ulHashIndex = 0;
    U32                     ulCurrentTime = 0;

    if (ulType >= SC_AGENT_STAT_BUTT)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ACD), "Update agent stat. Type: %u, Ageng: %u", ulType, ulAgentID);

    if (DOS_ADDR_INVALID(pstAgentInfo))
    {
        pthread_mutex_lock(&g_mutexAgentList);
        sc_agent_hash_func4agent(ulAgentID, &ulHashIndex);
        pstHashNode = hash_find_node(g_pstAgentList, ulHashIndex, &ulAgentID, sc_agent_hash_find);
        if (DOS_ADDR_INVALID(pstHashNode)
            || DOS_ADDR_INVALID(pstHashNode->pHandle))
        {
            pthread_mutex_unlock(&g_mutexAgentList);

            DOS_ASSERT(0);
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_ACD), "Cannot find the agent with this id %u.", ulAgentID);
            return DOS_FAIL;
        }
        pthread_mutex_unlock(&g_mutexAgentList);

        if (DOS_ADDR_INVALID(pstHashNode))
        {
            DOS_ASSERT(0);
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_ACD), "Empty hash node for this agent. %u", ulAgentID);
            return DOS_FAIL;
        }

        pstAgentQueueNode = pstHashNode->pHandle;
        if (DOS_ADDR_INVALID(pstAgentQueueNode)
            || DOS_ADDR_INVALID(pstAgentQueueNode->pstAgentInfo))
        {
            DOS_ASSERT(0);
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_ACD), "The hash node is empty for this agent. %u", ulAgentID);
            return DOS_FAIL;
        }

        pstAgentInfo = pstAgentQueueNode->pstAgentInfo;
    }

    ulCurrentTime = time(NULL);
    switch (ulType)
    {
        case SC_AGENT_STAT_SELECT:
            pstAgentInfo->stStat.ulSelectCnt++;
            break;

        case SC_AGENT_STAT_CALL_IN:
        case SC_AGENT_STAT_CALL_OUT:
        case SC_AGENT_STAT_CALL:
            pstAgentInfo->stStat.ulCallCnt++;
            pstAgentInfo->stStat.ulSelectCnt++;
            break;

        case SC_AGENT_STAT_CALL_OK:
            pstAgentInfo->stStat.ulCallConnected++;
            pstAgentInfo->ulLastCallTime = ulCurrentTime;
            break;

        case SC_AGENT_STAT_CALL_FINISHED:
            if (pstAgentInfo->ulLastCallTime !=0 && ulCurrentTime >= pstAgentInfo->ulLastCallTime)
            {
                pstAgentInfo->stStat.ulTotalDuration = ulCurrentTime - pstAgentInfo->ulLastCallTime;
            }

            pstAgentInfo->ulLastCallTime = 0;
            break;

        case SC_AGENT_STAT_ONLINE:
            if (pstAgentInfo->ulLastOnlineTime != 0 && pstAgentInfo->ulLastOnlineTime < ulCurrentTime)
            {
                pstAgentInfo->stStat.ulTimesOnline += (ulCurrentTime - pstAgentInfo->ulLastOnlineTime);
            }

            pstAgentInfo->ulLastOnlineTime = ulCurrentTime;
            break;

        case SC_AGENT_STAT_OFFLINE:
            if (pstAgentInfo->ulLastOnlineTime != 0 && ulCurrentTime >= pstAgentInfo->ulLastOnlineTime)
            {
                pstAgentInfo->stStat.ulTimesOnline += (ulCurrentTime - pstAgentInfo->ulLastOnlineTime);
            }
            else
            {
                DOS_ASSERT(0);
            }
            pstAgentInfo->ulLastOnlineTime = 0;
            break;

        case SC_AGENT_STAT_SIGNIN:
            if (pstAgentInfo->ulLastSignInTime != 0 && pstAgentInfo->ulLastSignInTime < ulCurrentTime)
            {
                pstAgentInfo->stStat.ulTimesSignin += (ulCurrentTime - pstAgentInfo->ulLastSignInTime);
            }

            pstAgentInfo->ulLastSignInTime = ulCurrentTime;
            break;

        case SC_AGENT_STAT_SIGNOUT:
            if (ulCurrentTime >= pstAgentInfo->ulLastSignInTime)
            {
                pstAgentInfo->stStat.ulTimesSignin += (ulCurrentTime - pstAgentInfo->ulLastSignInTime);
            }
            else
            {
                DOS_ASSERT(0);
            }
            pstAgentInfo->ulLastSignInTime = 0;
            break;

        default:
            return DOS_SUCC;
    }


    return DOS_SUCC;
}


/* 审计坐席，其实就 写入统计信息 */
U32 sc_agent_audit(U32 ulCycle, VOID *ptr)
{
    HASH_NODE_S                 *pstHashNode       = NULL;
    SC_AGENT_NODE_ST  *pstAgentQueueNode = NULL;
    SC_AGENT_INFO_ST        *pstAgentInfo      = NULL;
    U32             ulHashIndex  = 0;

    HASH_Scan_Table(g_pstAgentList, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstAgentList, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            if (DOS_ADDR_INVALID(pstHashNode))
            {
                DOS_ASSERT(0);
                continue;
            }

            pstAgentQueueNode = pstHashNode->pHandle;
            if (DOS_ADDR_INVALID(pstAgentQueueNode)
                || DOS_ADDR_INVALID(pstAgentQueueNode->pstAgentInfo))
            {
                /* 有可能被删除了，不需要断言 */
                continue;
            }

            pstAgentInfo = pstAgentQueueNode->pstAgentInfo;
            if (pstAgentInfo->bWaitingDelete)
            {
                /* 被删除了*/
                continue;
            }

            sc_agent_stat_save(pstAgentInfo);
        }
    }

    return DOS_SUCC;
}

static VOID sc_agent_hash_wolk4init(HASH_NODE_S *pNode, VOID *pParam)
{
    SC_AGENT_NODE_ST  *pstAgentQueueNode    = NULL;
    U32                         ulIndex               = 0;

    if (DOS_ADDR_INVALID(pNode)
        || DOS_ADDR_INVALID(pNode->pHandle))
    {
        DOS_ASSERT(0);
        return ;
    }

    pstAgentQueueNode = pNode->pHandle;
    if (DOS_ADDR_INVALID(pstAgentQueueNode)
        || DOS_ADDR_INVALID(pstAgentQueueNode->pstAgentInfo))
    {
        return;
    }

    for (ulIndex=0; ulIndex<MAX_GROUP_PER_SITE; ulIndex++)
    {
        if (0 != pstAgentQueueNode->pstAgentInfo->aulGroupID[ulIndex]
            && U32_BUTT != pstAgentQueueNode->pstAgentInfo->aulGroupID[ulIndex])
        {
            sc_agent_group_add_agent(pstAgentQueueNode->pstAgentInfo->aulGroupID[ulIndex], pstAgentQueueNode->pstAgentInfo);
        }
    }
}

static U32 sc_agent_relationship_init()
{
    HASH_NODE_S     *pstHashNode = NULL;
    U32             ulHashIndex  = 0;

    HASH_Scan_Table(g_pstAgentList, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstAgentList, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            sc_agent_hash_wolk4init(pstHashNode, NULL);
        }
    }

    return DOS_SUCC;
}


static S32 sc_agent_init_cb(VOID *PTR, S32 lCount, S8 **pszData, S8 **pszField)
{
    SC_AGENT_NODE_ST  *pstAgentQueueNode  = NULL;
    HASH_NODE_S                 *pstHashNode = NULL;
    S8                          *pszSiteID     = NULL, *pszCustomID = NULL;
    S8                          *pszGroupID0   = NULL, *pszGroupID1 = NULL;
    S8                          *pszExten      = NULL, *pszStatus   = NULL;
    S8                          *pszJobNum     = NULL, *pszUserID   = NULL;
    S8                          *pszRecordFlag = NULL, *pszIsHeader = NULL;
    S8                          *pszTelePhone  = NULL, *pszMobile   = NULL;
    S8                          *pszSelectType = NULL;
    S8                          *pszTTNumber = NULL;
    S8                          *pszSIPID = NULL;
    S8                          *pszFinishTime = NULL;
    SC_AGENT_INFO_ST        *pstSiteInfo = NULL;
    SC_AGENT_INFO_ST        stSiteInfo;
    U32                         ulAgentID   = 0, ulCustomID   = 0, ulGroupID0  = 0;
    U32                         ulGroupID1 = 0, ulRecordFlag = 0, ulIsHeader = 0;
    U32                         ulHashIndex = 0, ulIndex = 0, ulRest = 0, ulSelectType = 0;
    U32                         ulAgentIndex = 0, ulSIPID = 0, ulStatus = 0, ulFinishTime = 0;

    if (DOS_ADDR_INVALID(PTR)
        || DOS_ADDR_INVALID(pszData)
        || DOS_ADDR_INVALID(pszField))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulAgentIndex = *(U32 *)PTR;

    pszSiteID = pszData[0];
    pszStatus= pszData[1];
    pszCustomID = pszData[2];
    pszJobNum = pszData[3];
    pszUserID = pszData[4];
    pszExten = pszData[5];
    pszGroupID0 = pszData[6];
    pszGroupID1 = pszData[7];
    pszRecordFlag = pszData[8];
    pszIsHeader = pszData[9];
    pszSelectType = pszData[10];
    pszTelePhone = pszData[11];
    pszMobile = pszData[12];
    pszTTNumber = pszData[13];
    pszSIPID = pszData[14];
    pszFinishTime = pszData[15];

    if (DOS_ADDR_INVALID(pszSiteID)
        || DOS_ADDR_INVALID(pszStatus)
        || DOS_ADDR_INVALID(pszCustomID)
        || DOS_ADDR_INVALID(pszJobNum)
        || DOS_ADDR_INVALID(pszRecordFlag)
        || DOS_ADDR_INVALID(pszIsHeader)
        || DOS_ADDR_INVALID(pszSelectType)
        || dos_atoul(pszSiteID, &ulAgentID) < 0
        || dos_atoul(pszStatus, &ulStatus) < 0
        || dos_atoul(pszCustomID, &ulCustomID) < 0
        || dos_atoul(pszRecordFlag, &ulRecordFlag) < 0
        || dos_atoul(pszIsHeader, &ulIsHeader) < 0
        || dos_atoul(pszSelectType, &ulSelectType) < 0
        || dos_atoul(pszFinishTime, &ulFinishTime) < 0)
    {
        return 0;
    }

    if (DOS_ADDR_INVALID(pszGroupID0)
        || dos_atoul(pszGroupID0, &ulGroupID0) < 0)
    {
        pszGroupID0 = 0;
    }

    if (DOS_ADDR_INVALID(pszGroupID1)
        || dos_atoul(pszGroupID1, &ulGroupID1) < 0)
    {
        ulGroupID1 = 0;
    }

    if (AGENT_BIND_SIP == ulSelectType)
    {
        if (DOS_ADDR_INVALID(pszSIPID)
            || '\0' == pszSIPID[0]
            || dos_atoul(pszSIPID, &ulSIPID) < 0)
        {
            DOS_ASSERT(0);

            return 0;
        }

        if (DOS_ADDR_INVALID(pszUserID)
            || '\0' == pszUserID[0])
        {
            DOS_ASSERT(0);

            return 0;
        }
    }
    else if (AGENT_BIND_TELE == ulSelectType)
    {
        if (DOS_ADDR_INVALID(pszTelePhone)
            || '\0' == pszTelePhone[0])
        {
            DOS_ASSERT(0);

            return 0;
        }
    }
    else if (AGENT_BIND_MOBILE == ulSelectType)
    {
        if (DOS_ADDR_INVALID(pszMobile)
            || '\0' == pszMobile[0])
        {
            DOS_ASSERT(0);

            return 0;
        }
    }
    else if (AGENT_BIND_TT_NUMBER == ulSelectType)
    {
        if (DOS_ADDR_INVALID(pszTTNumber)
            || '\0' == pszTTNumber[0])
        {
            DOS_ASSERT(0);

            return 0;
        }
    }

    dos_memzero(&stSiteInfo, sizeof(stSiteInfo));
    stSiteInfo.ulAgentID = ulAgentID;
    stSiteInfo.ucStatus = SC_ACD_OFFLINE;
    stSiteInfo.ulCustomerID = ulCustomID;
    stSiteInfo.aulGroupID[0] = ulGroupID0;
    stSiteInfo.aulGroupID[1] = ulGroupID1;
    stSiteInfo.bValid = DOS_TRUE;
    stSiteInfo.bRecord = ulRecordFlag;
    stSiteInfo.ulLegNo = U32_BUTT;
    stSiteInfo.bGroupHeader = ulIsHeader;
    stSiteInfo.ucBindType = (U8)ulSelectType;
    stSiteInfo.ucCallStatus = SC_ACD_CALL_NONE;
    if (stSiteInfo.ucStatus != SC_ACD_OFFLINE)
    {
        stSiteInfo.bLogin = DOS_TRUE;
    }
    else
    {
        stSiteInfo.bLogin = DOS_FALSE;
    }
    stSiteInfo.bWaitingDelete = DOS_FALSE;
    stSiteInfo.bConnected = DOS_FALSE;
    stSiteInfo.ucProcesingTime = 0;
    stSiteInfo.ulSIPUserID = ulSIPID;
    stSiteInfo.ucProcesingTime = (U8)ulFinishTime;
    stSiteInfo.htmrLogout = NULL;
    pthread_mutex_init(&stSiteInfo.mutexLock, NULL);

    if (pszUserID && '\0' != pszUserID[0])
    {
        dos_strncpy(stSiteInfo.szUserID, pszUserID, sizeof(stSiteInfo.szUserID));
        stSiteInfo.szUserID[sizeof(stSiteInfo.szUserID) - 1] = '\0';
    }


    if (pszExten && '\0' != pszExten[0])
    {
        dos_strncpy(stSiteInfo.szExtension, pszExten, sizeof(stSiteInfo.szExtension));
        stSiteInfo.szExtension[sizeof(stSiteInfo.szExtension) - 1] = '\0';
    }

    if ('\0' != pszJobNum[0])
    {
        dos_strncpy(stSiteInfo.szEmpNo, pszJobNum, sizeof(stSiteInfo.szEmpNo));
        stSiteInfo.szEmpNo[sizeof(stSiteInfo.szEmpNo) - 1] = '\0';
    }

    if (pszTelePhone && '\0' != pszTelePhone[0])
    {
        dos_strncpy(stSiteInfo.szTelePhone, pszTelePhone, sizeof(stSiteInfo.szTelePhone));
        stSiteInfo.szTelePhone[sizeof(stSiteInfo.szTelePhone) - 1] = '\0';
    }

    if (pszMobile && '\0' != pszMobile[0])
    {
        dos_strncpy(stSiteInfo.szMobile, pszMobile, sizeof(stSiteInfo.szMobile));
        stSiteInfo.szMobile[sizeof(stSiteInfo.szMobile) - 1] = '\0';
    }

    if (pszTTNumber && '\0' != pszTTNumber[0])
    {
        dos_strncpy(stSiteInfo.szTTNumber, pszTTNumber, sizeof(stSiteInfo.szTTNumber));
        stSiteInfo.szTTNumber[sizeof(stSiteInfo.szTTNumber) - 1] = '\0';
    }

    /* 查看当前要添加的坐席是否已经存在，如果存在，就准备更新就好 */
    sc_agent_hash_func4agent(stSiteInfo.ulAgentID, &ulHashIndex);
    pthread_mutex_lock(&g_mutexAgentList);
    pstHashNode = hash_find_node(g_pstAgentList, ulHashIndex , &stSiteInfo.ulAgentID, sc_agent_hash_find);
    if (DOS_ADDR_VALID(pstHashNode)
        && DOS_ADDR_VALID(pstHashNode->pHandle))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Agent \"%d\" exist. Update", stSiteInfo.ulAgentID);

        pstAgentQueueNode = pstHashNode->pHandle;
        if (pstAgentQueueNode->pstAgentInfo)
        {
            /* 看看坐席有没有去了别的组，如果是，就需要将坐席换到别的组 */
            for (ulIndex = 0; ulIndex < MAX_GROUP_PER_SITE; ulIndex++)
            {
                if (pstAgentQueueNode->pstAgentInfo->aulGroupID[ulIndex] != U32_BUTT
                   && pstAgentQueueNode->pstAgentInfo->aulGroupID[ulIndex] != 0)
                {
                    /* 修改之前组ID合法，修改之后组ID合法，就需要看看前后两个ID是否相同，相同就不做什么了 */
                    if (stSiteInfo.aulGroupID[ulIndex] != U32_BUTT
                       && stSiteInfo.aulGroupID[ulIndex] != 0 )
                    {
                        if (pstAgentQueueNode->pstAgentInfo->aulGroupID[ulIndex] != stSiteInfo.aulGroupID[ulIndex])
                        {
                            /* 从别的组删除 */
                            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ACD), "Agent %u will be removed from Group %u."
                                             , pstAgentQueueNode->pstAgentInfo->ulAgentID
                                             , pstAgentQueueNode->pstAgentInfo->aulGroupID[ulIndex]);

                            ulRest = sc_agent_group_remove_agent(pstAgentQueueNode->pstAgentInfo->aulGroupID[ulIndex]
                                                                , pstAgentQueueNode->pstAgentInfo->ulAgentID);
                            if (DOS_SUCC == ulRest)
                            {
                                /* 添加到新的组 */
                                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ACD), "Agent %u will be added into Group %u."
                                                , stSiteInfo.aulGroupID[ulIndex]
                                                , pstAgentQueueNode->pstAgentInfo->ulAgentID);

                                pstAgentQueueNode->pstAgentInfo->aulGroupID[ulIndex] = stSiteInfo.aulGroupID[ulIndex];

                                sc_agent_group_add_agent(stSiteInfo.aulGroupID[ulIndex], pstAgentQueueNode->pstAgentInfo);
                            }
                        }
                    }
                    /* 修改之前组ID合法，修改之后组ID不合法，就需要吧agent从之前的组里面删除掉 */
                    else
                    {
                        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ACD), "Agent %u will be removed from group %u."
                                        , pstAgentQueueNode->pstAgentInfo->ulAgentID
                                        , pstAgentQueueNode->pstAgentInfo->aulGroupID[ulIndex]);
                        sc_agent_group_remove_agent(pstAgentQueueNode->pstAgentInfo->aulGroupID[ulIndex]
                                                    , pstAgentQueueNode->pstAgentInfo->ulAgentID);
                        pstAgentQueueNode->pstAgentInfo->aulGroupID[ulIndex] = 0;
                    }
                }
                else
                {
                    /* 修改之前组ID不合法，修改之后组ID合法，就需要吧agent添加到所设置的组 */
                    if (stSiteInfo.aulGroupID[ulIndex] != U32_BUTT
                        && stSiteInfo.aulGroupID[ulIndex] != 0)
                    {
                        /* 添加到新的组 */
                        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ACD), "Agent %u will be add into group %u."
                                        , pstAgentQueueNode->pstAgentInfo->ulAgentID
                                        , stSiteInfo.aulGroupID[ulIndex]);

                        pstAgentQueueNode->pstAgentInfo->aulGroupID[ulIndex] = stSiteInfo.aulGroupID[ulIndex];
                        sc_agent_group_add_agent(stSiteInfo.aulGroupID[ulIndex], pstAgentQueueNode->pstAgentInfo);
                    }
                    else
                    {
                        /* 修改之前组ID不合法，修改之后组ID不合法，就啥也不做哩 */
                    }
                }
            }

            pstAgentQueueNode->pstAgentInfo->bRecord = stSiteInfo.bRecord;
            pstAgentQueueNode->pstAgentInfo->bAllowAccompanying = stSiteInfo.bAllowAccompanying;
            pstAgentQueueNode->pstAgentInfo->bGroupHeader = stSiteInfo.bGroupHeader;
            pstAgentQueueNode->pstAgentInfo->ucBindType = stSiteInfo.ucBindType;
            if (ulAgentIndex == SC_INVALID_INDEX)
            {
                /* 更新单个坐席时，状态不需要变化；初始化时，都置为离线 */
                pstAgentQueueNode->pstAgentInfo->ucStatus = stSiteInfo.ucStatus;
            }
            pstAgentQueueNode->pstAgentInfo->ulCustomerID = stSiteInfo.ulCustomerID;
            pstAgentQueueNode->pstAgentInfo->bValid = stSiteInfo.bValid;
            pstAgentQueueNode->pstAgentInfo->ulSIPUserID = stSiteInfo.ulSIPUserID;
            pstAgentQueueNode->pstAgentInfo->ucProcesingTime = stSiteInfo.ucProcesingTime;
            pstAgentQueueNode->pstAgentInfo->ucCallStatus = stSiteInfo.ucCallStatus;

            dos_strncpy(pstAgentQueueNode->pstAgentInfo->szEmpNo, stSiteInfo.szEmpNo,SC_EMP_NUMBER_LENGTH);
            pstAgentQueueNode->pstAgentInfo->szEmpNo[SC_EMP_NUMBER_LENGTH - 1] = '\0';

            if (pszExten &&  '\0' != pszExten[0])
            {
                dos_strncpy(pstAgentQueueNode->pstAgentInfo->szExtension, stSiteInfo.szExtension, SC_NUM_LENGTH);
                pstAgentQueueNode->pstAgentInfo->szExtension[SC_NUM_LENGTH - 1] = '\0';
            }
            else
            {
                pstAgentQueueNode->pstAgentInfo->szExtension[0] = '\0';
            }

            if (pszUserID && '\0' != pszUserID[0])
            {
                dos_strncpy(pstAgentQueueNode->pstAgentInfo->szUserID, stSiteInfo.szUserID,SC_NUM_LENGTH);
                pstAgentQueueNode->pstAgentInfo->szUserID[SC_NUM_LENGTH - 1] = '\0';
            }
            else
            {
                pstAgentQueueNode->pstAgentInfo->szUserID[0] = '\0';
            }

            if (pszTelePhone && '\0' != pszTelePhone[0])
            {
                dos_strncpy(pstAgentQueueNode->pstAgentInfo->szTelePhone, stSiteInfo.szTelePhone,SC_NUM_LENGTH);
                pstAgentQueueNode->pstAgentInfo->szTelePhone[SC_NUM_LENGTH - 1] = '\0';
            }
            else
            {
                pstAgentQueueNode->pstAgentInfo->szTelePhone[0] = '\0';
            }

            if (pszMobile && '\0' != pszMobile[0])
            {
                dos_strncpy(pstAgentQueueNode->pstAgentInfo->szMobile, stSiteInfo.szMobile,SC_NUM_LENGTH);
                pstAgentQueueNode->pstAgentInfo->szMobile[SC_NUM_LENGTH - 1] = '\0';
            }
            else
            {
                pstAgentQueueNode->pstAgentInfo->szMobile[0] = '\0';
            }

            if (pszTTNumber && '\0' != pszTTNumber[0])
            {
                dos_strncpy(pstAgentQueueNode->pstAgentInfo->szTTNumber, stSiteInfo.szTTNumber, SC_NUM_LENGTH);
                pstAgentQueueNode->pstAgentInfo->szTTNumber[SC_NUM_LENGTH - 1] = '\0';
            }
            else
            {
                pstAgentQueueNode->pstAgentInfo->szTTNumber[0] = '\0';
            }
        }

        pthread_mutex_unlock(&g_mutexAgentList);
        return 0;
    }
    pthread_mutex_unlock(&g_mutexAgentList);

    pstSiteInfo = sc_agent_add(&stSiteInfo);
    if (DOS_ADDR_INVALID(pstSiteInfo))
    {
        DOS_ASSERT(0);
        return 0;
    }

    /* 将坐席加入到组 */
    if (ulAgentIndex != SC_INVALID_INDEX)
    {
        for (ulIndex = 0; ulIndex < MAX_GROUP_PER_SITE; ulIndex++)
        {
            if (0 == stSiteInfo.aulGroupID[ulIndex] || U32_BUTT == stSiteInfo.aulGroupID[ulIndex])
            {
                continue;
            }

            if (sc_agent_group_add_agent(stSiteInfo.aulGroupID[ulIndex], pstSiteInfo) != DOS_SUCC)
            {
                DOS_ASSERT(0);
            }
        }
    }
    return 0;
}

U32 sc_agent_init(U32 ulIndex)
{
    S8 szSQL[1024] = { 0, };

    if (SC_INVALID_INDEX == ulIndex)
    {
        dos_snprintf(szSQL, sizeof(szSQL)
                    , "SELECT " \
                      "    a.id, a.status, a.customer_id, a.job_number,b.userid, b.extension, a.group1_id, a.group2_id, " \
                      "    a.voice_record, a.class, a.select_type, a.fixed_telephone, a.mobile_number, " \
                      "    a.tt_number, a.sip_id, a.finish_time " \
                      "FROM " \
                      "    tbl_agent a " \
                      "LEFT JOIN" \
                      "    tbl_sip b "\
                      "ON "\
                      "    b.id = a.sip_id;");
    }
    else
    {
        dos_snprintf(szSQL, sizeof(szSQL)
                    , "SELECT " \
                      "    a.id, a.status, a.customer_id, a.job_number,b.userid, b.extension, a.group1_id, a.group2_id, " \
                      "    a.voice_record, a.class, a.select_type, a.fixed_telephone, a.mobile_number, " \
                      "    a.tt_number, a.sip_id, a.finish_time " \
                      "FROM " \
                      "    tbl_agent a " \
                      "LEFT JOIN" \
                      "    tbl_sip b " \
                      "ON "\
                      "    b.id = a.sip_id " \
                      "WHERE " \
                      "    a.id = %u;",
                      ulIndex);
    }

    if (db_query(g_pstSCDBHandle, szSQL, sc_agent_init_cb, &ulIndex, NULL) < 0)
    {
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

static S32 sc_agent_group_init_cb(VOID *PTR, S32 lCount, S8 **pszData, S8 **pszField)
{
    U32     ulGroupID = 0, ulCustomID = 0, ulPolicy = 0;
    S8      *pszGroupID = NULL, *pszCustomID = NULL, *pszPolicy = NULL, *pszGroupName = NULL;

    if (DOS_ADDR_INVALID(pszField) || DOS_ADDR_INVALID(pszData))
    {
        return 0;
    }

    pszGroupID = pszData[0];
    pszCustomID = pszData[1];
    pszPolicy = pszData[2];
    pszGroupName = pszData[3];

    if (dos_atoul(pszGroupID, &ulGroupID) < 0
        || dos_atoul(pszCustomID, &ulCustomID) < 0
        || dos_atoul(pszPolicy, &ulPolicy) < 0
        || DOS_ADDR_INVALID(pszGroupName))
    {
        return 0;
    }

    return sc_agent_group_add(ulGroupID, ulCustomID, ulPolicy,pszGroupName);
}

U32 sc_agent_group_init(U32 ulIndex)
{
    S8 szSql[1024] = { 0, };

    if (SC_INVALID_INDEX == ulIndex)
    {
        dos_snprintf(szSql, sizeof(szSql), "SELECT id,customer_id,acd_policy,`name` from tbl_group;");
    }
    else
    {
        dos_snprintf(szSql, sizeof(szSql), "SELECT id,customer_id,acd_policy,`name` from tbl_group WHERE id=%d;", ulIndex);
    }

    if (db_query(g_pstSCDBHandle, szSql, sc_agent_group_init_cb, NULL, NULL) < 0)
    {
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

static U32 sc_agent_deinit()
{
    return DOS_SUCC;
}

static U32 sc_agent_group_deinit()
{
    return DOS_SUCC;
}

U32 sc_agent_mngt_init()
{
    g_pstGroupList = hash_create_table(SC_ACD_HASH_SIZE, NULL);
    if (DOS_ADDR_INVALID(g_pstGroupList))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_ACD), "%s", "Init Group Hash Table Fail.");

        return DOS_FAIL;
    }
    g_pstGroupList->NodeNum = 0;

    g_pstAgentList = hash_create_table(SC_ACD_HASH_SIZE, NULL);
    if (DOS_ADDR_INVALID(g_pstAgentList))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_ACD), "%s", "Init Site Hash Table Fail.");

        hash_delete_table(g_pstGroupList, NULL);
        g_pstGroupList = NULL;

        return DOS_FAIL;
    }
    g_pstAgentList->NodeNum = 0;

    if (sc_agent_group_init(SC_INVALID_INDEX) != DOS_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_ACD), "%s", "Init group list fail in ACD.");

        hash_delete_table(g_pstAgentList, NULL);
        g_pstAgentList = NULL;

        hash_delete_table(g_pstGroupList, NULL);
        g_pstGroupList = NULL;

        return DOS_FAIL;
    }
    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Init group list finished. Load %d agent group(s).", g_pstGroupList->NodeNum);


    if (sc_agent_init(SC_INVALID_INDEX) != DOS_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_ACD), "%s", "Init sites list fail in ACD.");

        sc_agent_group_deinit();

        hash_delete_table(g_pstAgentList, NULL);
        g_pstAgentList = NULL;

        hash_delete_table(g_pstGroupList, NULL);
        g_pstGroupList = NULL;

        return DOS_FAIL;
    }
    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Init agent list finished. Load %d agent(s).", g_pstAgentList->NodeNum);

    if (sc_agent_relationship_init() != DOS_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_ACD), "%s", "Init ACD Data FAIL.");

        sc_agent_group_deinit();
        sc_agent_deinit();

        hash_delete_table(g_pstAgentList, NULL);
        g_pstAgentList = NULL;

        hash_delete_table(g_pstGroupList, NULL);
        g_pstGroupList = NULL;

        return DOS_FAIL;
    }

    return DOS_SUCC;
}


