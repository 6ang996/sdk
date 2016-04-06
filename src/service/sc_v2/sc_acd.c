/**
 * @file : sc_acd.c
 *
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 * ��ϯ����
 *
 * @date: 2016��1��14��
 * @arthur: Larry
 *
 * @note ��ϯ�������ڴ��еĽṹ
 * 1. ��g_pstAgentList hash��ά�����е���ϯ
 * 2. ��g_pstGroupList ά�����е��飬ͬʱ��������ά��ÿ�����Ա��hash��
 * 3. ���Ա��Hash������ָ��g_pstSiteList����ϯ��Ա��ָ��
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
 *  ����g_pstGroupList table�����е�agnetʹ��ָ��g_pstAgentList��ĳһ����ϯ,
 *  ͬһ����ϯ�������ڶ����ϯ�飬���Կ���g_pstGroupList���ж����ϯָ��g_pstAgentList��ͬһ���ڵ㣬
 *  ����ɾ��ʱ����ֱ��delete
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

/* ��ϯ���hash�� */
HASH_TABLE_S      *g_pstAgentList      = NULL;
pthread_mutex_t   g_mutexAgentList     = PTHREAD_MUTEX_INITIALIZER;

HASH_TABLE_S      *g_pstGroupList      = NULL;
pthread_mutex_t   g_mutexGroupList     = PTHREAD_MUTEX_INITIALIZER;

/* ��ϯ����� */
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

    /* �ҵ���ϯ */
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
                    , "{\\\"type\\\":\\\"%u\\\",\\\"body\\\":{\\\"work_status\\\":\\\"%d\\\",\\\"server_status\\\":\\\"%u\\\",\\\"is_signin\\\":\\\"%u\\\"}}"
                    , ACD_MSG_TYPE_QUERY
                    , pstAgentInfo->ucWorkStatus
                    , pstAgentInfo->ucServStatus
                    , pstAgentInfo->bConnected ? 1 : 0);


    return sc_pub_send_msg(szURL, szJson, SC_PUB_TYPE_STATUS, NULL);
}

U32 sc_agent_update_status_db(SC_AGENT_INFO_ST *pstAgentInfo)
{
    SC_DB_MSG_AGENT_STATUS_ST   *pstAgentStatus = NULL;

    if (DOS_ADDR_INVALID(pstAgentInfo))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Update agent(%u), w-status(%u), s-status(%u)"
        , pstAgentInfo->ulAgentID, pstAgentInfo->ucWorkStatus, pstAgentInfo->ucServStatus);

    pstAgentStatus = (SC_DB_MSG_AGENT_STATUS_ST *)dos_dmem_alloc(sizeof(SC_DB_MSG_AGENT_STATUS_ST));
    if (DOS_ADDR_INVALID(pstAgentStatus))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    dos_memzero(pstAgentStatus, sizeof(SC_DB_MSG_AGENT_STATUS_ST));
    pstAgentStatus->stMsgTag.ulMsgType = SC_MSG_SAVE_AGENT_STATUS;
    pstAgentStatus->stMsgTag.szData = NULL;
    pstAgentStatus->ulAgentID = pstAgentInfo->ulAgentID;
    pstAgentStatus->ulServStatus = pstAgentInfo->ucServStatus;
    pstAgentStatus->ulWorkStatus = pstAgentInfo->ucWorkStatus;
    pstAgentStatus->bIsSigin = pstAgentInfo->bNeedConnected;
    pstAgentStatus->bIsInterception = pstAgentInfo->bIsInterception;
    pstAgentStatus->bIsWhisper = pstAgentInfo->bIsWhisper;
    dos_strncpy(pstAgentStatus->szEmpNo, pstAgentInfo->szEmpNo, SC_NUM_LENGTH-1);
    pstAgentStatus->szEmpNo[SC_NUM_LENGTH-1] = '\0';

    return sc_send_msg2db((SC_DB_MSG_TAG_ST *)pstAgentStatus);
}

U32 sc_agent_signin_proc(SC_AGENT_NODE_ST *pstAgentNode)
{
    SC_AGENT_INFO_ST    *pstAgentInfo   = NULL;
    SC_LEG_CB           *pstLegCB       = NULL;
    SC_SRV_CB           *pstSCB         = NULL;
    U32                 ulRet           = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstAgentNode)
        || DOS_ADDR_INVALID(pstAgentNode->pstAgentInfo))
    {
        return DOS_FAIL;
    }

    pstAgentInfo = pstAgentNode->pstAgentInfo;

    /* �ж�һ����ϯ��״̬�������Ƿ��Ѿ���ǩ */
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
    pstSCB->ulAgentID = pstAgentInfo->ulAgentID;
    pstSCB->ulCustomerID = pstAgentInfo->ulCustomerID;

    pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = (SC_SCB_TAG_ST *)&pstSCB->stSigin;

    /* �����к������л�ȡ���к��� */
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
            sc_scb_set_service(pstSCB, BS_SERV_INTER_CALL);
            break;

        case AGENT_BIND_TELE:
            dos_snprintf(pstLegCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstLegCB->stCall.stNumInfo.szOriginalCallee), pstAgentInfo->szTelePhone);
            pstLegCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND;
            sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL);

            break;

        case AGENT_BIND_MOBILE:
            dos_snprintf(pstLegCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstLegCB->stCall.stNumInfo.szOriginalCallee), pstAgentInfo->szMobile);
            pstLegCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND;
            sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL);
            break;

        case AGENT_BIND_TT_NUMBER:
            dos_snprintf(pstLegCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstLegCB->stCall.stNumInfo.szOriginalCallee), pstAgentInfo->szTTNumber);
            pstLegCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND_TT;
            sc_scb_set_service(pstSCB, BS_SERV_INTER_CALL);
            break;

        default:
            break;
    }
    pstLegCB->stCall.stNumInfo.szOriginalCallee[sizeof(pstLegCB->stCall.stNumInfo.szOriginalCallee)-1] = '\0';

    if (pstLegCB->stCall.ucPeerType == SC_LEG_PEER_OUTBOUND)
    {
        /* ��Ҫ��֤ */
        pstSCB->stSigin.stSCBTag.usStatus = SC_SIGIN_AUTH;
        ulRet = sc_send_usr_auth2bs(pstSCB, pstLegCB);
        if (ulRet != DOS_SUCC)
        {
            sc_scb_free(pstSCB);
            sc_lcb_free(pstLegCB);

            return DOS_FAIL;
        }
    }
    else if (pstLegCB->stCall.ucPeerType == SC_LEG_PEER_OUTBOUND_TT)
    {
        ulRet = sc_make_call2eix(pstSCB, pstLegCB);
        pstSCB->stSigin.stSCBTag.usStatus = SC_SIGIN_EXEC;
    }
    else
    {
        ulRet = sc_make_call2sip(pstSCB, pstLegCB);
        pstSCB->stSigin.stSCBTag.usStatus = SC_SIGIN_EXEC;
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

U32 sc_agent_status_notify(SC_AGENT_INFO_ST *pstAgentInfo)
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


    /* ��ʽ������ǰ����Ҫ���"\",�ṩ��push stream��ת���� */
    dos_snprintf(szData, sizeof(szData), "{\\\"type\\\":\\\"%u\\\",\\\"body\\\":{\\\"work_status\\\":\\\"%u\\\",\\\"server_status\\\":\\\"%u\\\",\\\"is_signin\\\":\\\"%u\\\",\\\"result\\\":\\\"0\\\"}}"
                    , ACD_MSG_TYPE_STATUS
                    , pstAgentInfo->ucWorkStatus
                    , pstAgentInfo->ucServStatus
                    , pstAgentInfo->bNeedConnected);

    return sc_pub_send_msg(szURL, szData, SC_PUB_TYPE_STATUS, NULL);
}

/**
    ���е���
*/
U32 sc_agent_call_notify(SC_AGENT_INFO_ST *pstAgentInfo, S8 *szCaller)
{
    S8 szURL[256]      = { 0, };
    S8 szData[512]     = { 0, };
    S8 szChannel[128]  = { 0, };
    S32 ulPAPIPort     = -1;

    if (DOS_ADDR_INVALID(pstAgentInfo)
        || DOS_ADDR_INVALID(szCaller))
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


    /* ��ʽ������ǰ����Ҫ���"\",�ṩ��push stream��ת���� */
    dos_snprintf(szData, sizeof(szData), "{\\\"type\\\":\\\"%u\\\",\\\"body\\\":{\\\"number\\\":\\\"%s\\\"}}"
                    , ACD_MSG_TYPE_CALL_NOTIFY, szCaller);

    return sc_pub_send_msg(szURL, szData, SC_PUB_TYPE_STATUS, NULL);
}


U32 sc_agent_marker_update_req(U32 ulCustomID, U32 ulAgentID, S32 lKey, S8 *szCallerNum)
{
    S8 szURL[256]      = { 0, };
    S8 szData[512]     = { 0, };
    S32 ulPAPIPort     = -1;

    ulPAPIPort = config_hb_get_papi_port();
    if (ulPAPIPort <= 0)
    {
        dos_snprintf(szURL, sizeof(szURL), "http://localhost/index.php/papi");
    }
    else
    {
        dos_snprintf(szURL, sizeof(szURL), "http://localhost:%d/index.php/papi", ulPAPIPort);
    }

    /* ��ʽ������ǰ����Ҫ���"\",�ṩ��push stream��ת���� */
    dos_snprintf(szData, sizeof(szData), "data={\"type\":\"%u\", \"data\":{\"customer_id\":\"%u\", \"agent_id\":\"%u\", \"marker\":\"%d\", \"number\":\"%s\"}}"
                    , SC_PUB_TYPE_MARKER
                    , ulCustomID, ulAgentID
                    , lKey
                    , szCallerNum);

    return sc_pub_send_msg(szURL, szData, SC_PUB_TYPE_MARKER, NULL);
}

/*
 * ��  ��: sc_acd_hash_func4agent
 * ��  ��: ��ϯ��hash������ͨ���ֻ��ż���һ��hashֵ
 * ��  ��:
 *         S8 *pszExension  : �ֻ���
 *         U32 *pulHashIndex: �������������֮���hashֵ
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
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
 * ��  ��: sc_acd_hash_func4grp
 * ��  ��: ��ϯ���hash������ͨ���ֻ��ż���һ��hashֵ
 * ��  ��:
 *         U32 ulGrpID  : ��ϯ��ID
 *         U32 *pulHashIndex: �������������֮���hashֵ
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
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
 * ��  ��: sc_acd_hash_func4calller_relation
 * ��  ��: ���к������ϯ��Ӧ��ϵ��hash������ͨ�����к������һ��hashֵ
 * ��  ��:
 *         U32 ulGrpID  : ��ϯ��ID
 *         U32 *pulHashIndex: �������������֮���hashֵ
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
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
 * ��  ��: sc_acd_grp_hash_find
 * ��  ��: ��ϯ���hash���Һ���
 * ��  ��:
 *         VOID *pSymName  : ��ϯ��ID
 *         HASH_NODE_S *pNode: HASH�ڵ�
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
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
 * ��  ��: sc_acd_agent_hash_find
 * ��  ��: ��ϯ��hash���Һ���
 * ��  ��:
 *         VOID *pSymName  : ��ϯ�ֻ���
 *         HASH_NODE_S *pNode: HASH�ڵ�
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
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
 * ��  ��: sc_acd_grp_hash_find
 * ��  ��: ��ϯ���hash���Һ���
 * ��  ��:
 *         VOID *pSymName  : ��ϯ��ID
 *         HASH_NODE_S *pNode: HASH�ڵ�
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
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
 * ��  ��: S32 sc_acd_site_dll_find(VOID *pSymName, DLL_NODE_S *pNode)
 * ��  ��: ��ϯ��hash���Һ���
 * ��  ��:
 *         VOID *pSymName  : ��ϯ�ֻ���
 *         DLL_NODE_S *pNode: ����ڵ�
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
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
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_ACD), "Group List node has no data. Maybe the data has been deleted. Group: %u."
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

/*
 * ��  ��: U32 sc_acd_update_agent_scbno_by_userid(S8 *szUserID, SC_SCB_ST *pstSCB)
 * ��  ��: ����SIP�����ҵ��󶨵���ϯ������usSCBNo�ֶ�. (ucBindType Ϊ AGENT_BIND_SIP)
 * ��  ��:
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
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

    /* ����SIP�󶨵���ϯ */
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
 * ��  ��: U32 sc_acd_group_remove_agent(U32 ulGroupID, S8 *pszUserID)
 * ��  ��: ����ϯ�������Ƴ���ϯ
 * ��  ��:
 *       U32 ulGroupID: ��ϯ��ID
 *       S8 *pszUserID: ��ϯΨһ��ʾ sip�˻�
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 **/
U32 sc_agent_group_remove_agent(U32 ulGroupID, U32 ulAgentID)
{
    SC_AGENT_NODE_ST   *pstAgentQueueNode  = NULL;
    SC_AGENT_GRP_NODE_ST      *pstGroupNode       = NULL;
    HASH_NODE_S                  *pstHashNode        = NULL;
    DLL_NODE_S                   *pstDLLNode         = NULL;
    U32                          ulHashVal           = 0;

    /* ������ڶ����Ƿ���� */
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
 * ��  ��: U32 sc_acd_group_add_agent(U32 ulGroupID, SC_AGENT_INFO_ST *pstAgentInfo)
 * ��  ��: ����ϯ��ӵ���ϯ����
 * ��  ��:
 *       U32 ulGroupID: ��ϯ��ID
 *       SC_AGENT_INFO_ST *pstAgentInfo : ��ϯ����Ϣ
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
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

    /* ������ڶ����Ƿ���� */
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
 * ��  ��: sc_acd_add_agent
 * ��  ��: �����ϯ
 * ��  ��:
 *         SC_AGENT_INFO_ST *pstAgentInfo, ��ϯ��Ϣ����
 *         U32 ulGrpID ������
 * ����ֵ: SC_AGENT_NODE_ST *������ϯ��������Ľڵ㡣������ú�������ϯ��ӵ��顣���ʧ���򷵻�NULL
 * !!! �ú���ֻ�ǽ���ϯ���������У����Ὣ��ϯ������ϯ���� !!!
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

    /* ������ϯ����hash�� */
    HASH_Init_Node(pstHashNode);
    pstHashNode->pHandle = pstAgentQueueNode;
    sc_agent_hash_func4agent(pstAgentInfo->ulAgentID, &ulHashVal);
    pthread_mutex_lock(&g_mutexAgentList);
    hash_add_node(g_pstAgentList, pstHashNode, ulHashVal, NULL);
    pthread_mutex_unlock(&g_mutexAgentList);

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ACD), "Load Agent. ID: %u, Customer: %u, Group1: %u, Group2: %u"
                    , pstAgentInfo->ulAgentID, pstAgentInfo->ulCustomerID
                    , pstAgentInfo->aulGroupID[0], pstAgentInfo->aulGroupID[1]);

    /* ��ӵ����� */
    dos_memcpy(pstAgentData, pstAgentInfo, sizeof(SC_AGENT_INFO_ST));
    pthread_mutex_init(&pstAgentData->mutexLock, NULL);
    pstAgentQueueNode->pstAgentInfo = pstAgentData;

    return pstAgentData;
}


/*
 * ��  ��: sc_acd_grp_wolk4delete_site
 * ��  ��: ��ĳһ����ϯ������ɾ����ϯ
 * ��  ��:
 *         HASH_NODE_S *pNode : ��ǰ�ڵ�
 *         VOID *pszExtensition : ��ɾ����ϯ�ķֻ���
 * ����ֵ: VOID
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

    /* ����ط��Ȳ�Ҫfree���п��ܱ�Ķ���Ҳ�������Ϣ */
    pstAgentQueueNode = pstDLLNode->pHandle;
    pstAgentQueueNode->pstAgentInfo = NULL;

    pthread_mutex_unlock(&pstGroupListNode->mutexSiteQueue);
}

U32 sc_agent_delete(U32  ulAgentID)
{
    SC_AGENT_NODE_ST   *pstAgentQueueNode = NULL;
    HASH_NODE_S                  *pstHashNode       = NULL;
    U32                          ulHashVal          = 0;

    /* ���������飬��ɾ�������ϯ */
    pthread_mutex_lock(&g_mutexGroupList);
    hash_walk_table(g_pstGroupList, &ulAgentID, sc_agent_group_wolk4delete_agent);
    pthread_mutex_unlock(&g_mutexGroupList);

    /* ������ϯ��Ȼ����ֵΪɾ��״̬ */
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

    /* ȷ������ */
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

    /* ȷ������ */
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
     * ���һ����ţ�Ȼ��������ſ�ʼ����һ�����õ���ϯ���������β�˻�û���ҵ����ٴ�ͷ��
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
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_ACD), "Found an useable agent.(Agent %u in Group %u)"
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

            /* �嵽�����Ѿ����ҹ����еĵ���ϯ�� */
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
     * ���ϴ�ʹ�õı�ſ�ʼ����һ�����õ���ϯ���������β�˻�û���ҵ����ٴ�ͷ��
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

        /* �ҵ�һ�������һ��������С���ŵ���ϯ */
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
        /* û���ҵ���������С�Ĺ��ű�ŵ���ϯ���ӵ绰 */
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
    /* �������к�����Ҷ�Ӧ����ϯ */
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
    /* ������ϯ���, ����ϯhash�в��ҵ���Ӧ����ϯ */
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
        /* ��ӻ��߸��� ���к������ϯ��Ӧ��ϵ��hash */
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_ACD), "Found an uaeable agent. Call Count: %d. (Agent %d in Group %d)"
                        , pstAgentNode->pstAgentInfo->ulCallCnt
                        , pstAgentNode->pstAgentInfo->ulAgentID
                        , pstGroupListNode->ulGroupID);

        if (DOS_ADDR_VALID(pstHashNode) && DOS_ADDR_VALID(pstHashNode->pHandle))
        {
            /* ���� */
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
 * ͨ��TT�Ż�ȡ��ϯ
 *
 * @param szTTNumber TT��
 *
 * @return ��ϯָ��
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

    SC_AGENT_NODE_ST            *pstAgentNode      = NULL;
    SC_AGENT_GRP_NODE_ST        *pstGroupListNode  = NULL;
    HASH_NODE_S                 *pstHashNode       = NULL;
    DLL_NODE_S                  *pstDLLNode        = NULL;
    U32                         ulHashVal          = 0;

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

        if (SC_ACD_WORK_IDEL != pstAgentNode->pstAgentInfo->ucWorkStatus)
        {
            continue;
        }

        if (DOS_ADDR_VALID(pulWorking))
        {
            (*pulWorking)++;
        }

        if (SC_ACD_SERV_IDEL == pstAgentNode->pstAgentInfo->ucServStatus)
        {
            /* ����ط�����һ�� */
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

U32 sc_agent_set_ringing(SC_AGENT_INFO_ST *pstAgentQueueInfo)
{
    BOOL                bIsPub       = DOS_FALSE;

    if (DOS_ADDR_INVALID(pstAgentQueueInfo))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (pstAgentQueueInfo->ucServStatus)
    {
        case SC_ACD_SERV_IDEL:
        case SC_ACD_SERV_CALL_OUT:
        case SC_ACD_SERV_CALL_IN:
        case SC_ACD_SERV_RINGBACK:
        case SC_ACD_SERV_PROC:
            pstAgentQueueInfo->ucServStatus = SC_ACD_SERV_RINGING;
            bIsPub = DOS_TRUE;
            break;
        case SC_ACD_SERV_RINGING:
            break;
        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Agent %u is in an invalid status.", pstAgentQueueInfo->ulAgentID);
            return DOS_FAIL;
            break;
    }

    if (bIsPub)
    {
        sc_agent_status_notify(pstAgentQueueInfo);
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Request set agnet status to ringing. Agent: %u"
                    , pstAgentQueueInfo->ulAgentID);

    return DOS_SUCC;
}

U32 sc_agent_set_ringback(SC_AGENT_INFO_ST *pstAgentQueueInfo)
{
    BOOL                bIsPub       = DOS_FALSE;

    if (DOS_ADDR_INVALID(pstAgentQueueInfo))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (pstAgentQueueInfo->ucServStatus)
    {
        case SC_ACD_SERV_IDEL:
        case SC_ACD_SERV_CALL_OUT:
        case SC_ACD_SERV_CALL_IN:
        case SC_ACD_SERV_RINGING:
        case SC_ACD_SERV_PROC:
            pstAgentQueueInfo->ucServStatus = SC_ACD_SERV_RINGBACK;
            bIsPub = DOS_TRUE;
            break;
        case SC_ACD_SERV_RINGBACK:
            break;
        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Agent %u is in an invalid status.", pstAgentQueueInfo->ulAgentID);
            return DOS_FAIL;
            break;
    }

    if (bIsPub)
    {
        sc_agent_status_notify(pstAgentQueueInfo);
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Request set agnet status to ring back. Agent: %u"
                    , pstAgentQueueInfo->ulAgentID);

    return DOS_SUCC;
}

U32 sc_agent_set_call_out(SC_AGENT_INFO_ST *pstAgentQueueInfo)
{
    BOOL                bIsPub       = DOS_FALSE;

    if (DOS_ADDR_INVALID(pstAgentQueueInfo))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (pstAgentQueueInfo->ucServStatus)
    {
        case SC_ACD_SERV_IDEL:
        case SC_ACD_SERV_RINGBACK:
        case SC_ACD_SERV_CALL_IN:
        case SC_ACD_SERV_RINGING:
        case SC_ACD_SERV_PROC:
            pstAgentQueueInfo->ucServStatus = SC_ACD_SERV_CALL_OUT;
            bIsPub = DOS_TRUE;
            break;
        case SC_ACD_SERV_CALL_OUT:
            break;
        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Agent %u is in an invalid status.", pstAgentQueueInfo->ulAgentID);
            return DOS_FAIL;
            break;
    }

    if (bIsPub)
    {
        sc_agent_status_notify(pstAgentQueueInfo);
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Request set agnet status to call out.Agent: %u"
                    , pstAgentQueueInfo->ulAgentID);

    return DOS_SUCC;
}

U32 sc_agent_set_call_in(SC_AGENT_INFO_ST *pstAgentQueueInfo)
{
    BOOL                bIsPub       = DOS_FALSE;

    if (DOS_ADDR_INVALID(pstAgentQueueInfo))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (pstAgentQueueInfo->ucServStatus)
    {
        case SC_ACD_SERV_IDEL:
        case SC_ACD_SERV_RINGING:
        case SC_ACD_SERV_CALL_OUT:
        case SC_ACD_SERV_RINGBACK:
        case SC_ACD_SERV_PROC:
            pstAgentQueueInfo->ucServStatus = SC_ACD_SERV_CALL_IN;
            bIsPub = DOS_TRUE;
            break;
        case SC_ACD_SERV_CALL_IN:
            break;
        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Agent %u is in an invalid status.", pstAgentQueueInfo->ulAgentID);
            return DOS_FAIL;
            break;
    }

    if (bIsPub)
    {
        sc_agent_status_notify(pstAgentQueueInfo);
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Request set agnet status to call in.Agent: %u"
                    , pstAgentQueueInfo->ulAgentID);

    return DOS_SUCC;
}

U32 sc_agent_set_idle(SC_AGENT_INFO_ST *pstAgentQueueInfo)
{
    BOOL                bIsPub       = DOS_FALSE;

    if (DOS_ADDR_INVALID(pstAgentQueueInfo))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (pstAgentQueueInfo->ucServStatus)
    {
        case SC_ACD_SERV_IDEL:
            break;
        case SC_ACD_SERV_CALL_OUT:
        case SC_ACD_SERV_CALL_IN:
        case SC_ACD_SERV_RINGING:
        case SC_ACD_SERV_RINGBACK:
        case SC_ACD_SERV_PROC:
            pstAgentQueueInfo->ucServStatus = SC_ACD_SERV_IDEL;
            if (!pstAgentQueueInfo->bNeedConnected)
            {
                pstAgentQueueInfo->ulLegNo = U32_BUTT;
            }
            bIsPub = DOS_TRUE;
            break;
        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Agent %u is in an invalid status.", pstAgentQueueInfo->ulAgentID);
            return DOS_FAIL;
            break;
    }

    if (bIsPub)
    {
        sc_agent_status_notify(pstAgentQueueInfo);
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Request set agnet status to idle. Agent: %u"
                    , pstAgentQueueInfo->ulAgentID);

    return DOS_SUCC;
}

U32 sc_agent_set_proc(SC_AGENT_INFO_ST *pstAgentQueueInfo)
{
    BOOL                bIsPub       = DOS_FALSE;

    if (DOS_ADDR_INVALID(pstAgentQueueInfo))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (pstAgentQueueInfo->ucServStatus)
    {
        case SC_ACD_SERV_CALL_OUT:
        case SC_ACD_SERV_CALL_IN:
            pstAgentQueueInfo->ucServStatus = SC_ACD_SERV_PROC;
            bIsPub = DOS_TRUE;
            break;
        case SC_ACD_SERV_IDEL:
        case SC_ACD_SERV_RINGING:
        case SC_ACD_SERV_RINGBACK:
        case SC_ACD_SERV_PROC:
            break;
        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Agent %u is in an invalid status.", pstAgentQueueInfo->ulAgentID);
            return DOS_FAIL;
            break;
    }

    if (bIsPub)
    {
        sc_agent_status_notify(pstAgentQueueInfo);
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Request set agnet status to proc.Agent: %u"
                    , pstAgentQueueInfo->ulAgentID);

    return DOS_SUCC;
}

U32 sc_agent_serv_status_update(SC_AGENT_INFO_ST *pstAgentQueueInfo, U32 ulServStatus, U32 ulServType)
{
    U32 ulRet = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstAgentQueueInfo)
        || ulServStatus >= SC_ACD_SERV_BUTT)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (ulServStatus)
    {
        case SC_ACD_SERV_IDEL:
            ulRet = sc_agent_set_idle(pstAgentQueueInfo);
            sc_agent_stat(SC_AGENT_STAT_CALL_FINISHED, pstAgentQueueInfo, 0, 0);
            break;

        case SC_ACD_SERV_CALL_OUT:
            ulRet = sc_agent_set_call_out(pstAgentQueueInfo);
            sc_agent_stat(SC_AGENT_STAT_CALL_OK, pstAgentQueueInfo, 0, 0);
            break;

        case SC_ACD_SERV_CALL_IN:
            ulRet = sc_agent_set_call_in(pstAgentQueueInfo);
            sc_agent_stat(SC_AGENT_STAT_CALL_OK, pstAgentQueueInfo, 0, 0);
            break;

        case SC_ACD_SERV_RINGING:
            ulRet = sc_agent_set_ringing(pstAgentQueueInfo);

            if (ulServType == SC_SRV_PREVIEW_CALL
                || ulServType == SC_SRV_AUTO_PREVIEW)
            {
                break;
            }

            sc_agent_stat(SC_AGENT_STAT_CALL_IN, pstAgentQueueInfo, 0, 0);
            break;

        case SC_ACD_SERV_RINGBACK:
            ulRet = sc_agent_set_ringback(pstAgentQueueInfo);
            sc_agent_stat(SC_AGENT_STAT_CALL_OUT, pstAgentQueueInfo, 0, 0);
            break;

        case SC_ACD_SERV_PROC:
            ulRet = sc_agent_set_proc(pstAgentQueueInfo);
            break;

        default:
            break;
    }

    if (DOS_SUCC == ulRet)
    {
        /* ���浽���ݿ� */
        sc_agent_update_status_db(pstAgentQueueInfo);
    }

    return ulRet;
}

U32 sc_agent_set_signin(SC_AGENT_NODE_ST *pstAgentNode, U32 ulOperatingType)
{
    BOOL                bIsPub       = DOS_FALSE;
    SC_AGENT_INFO_ST   *pstAgentInfo = NULL;     /* ��ϯ��Ϣ */

    if (DOS_ADDR_INVALID(pstAgentNode)
        || DOS_ADDR_INVALID(pstAgentNode->pstAgentInfo))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstAgentInfo = pstAgentNode->pstAgentInfo;

    /* ����ǩ */
    if (sc_agent_signin_proc(pstAgentNode) != DOS_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Request set agnet status to signin FAIL. Agent: %u"
                        , pstAgentInfo->ulAgentID);

        return DOS_FAIL;
    }

    switch (pstAgentInfo->ucWorkStatus)
    {
        case SC_ACD_WORK_OFFLINE:
        case SC_ACD_WORK_BUSY:
        case SC_ACD_WORK_AWAY:
        case SC_ACD_WORK_IDEL:
            pstAgentInfo->ucWorkStatus = SC_ACD_WORK_IDEL;
            sc_agent_stat(SC_AGENT_STAT_ONLINE, pstAgentNode->pstAgentInfo, pstAgentNode->pstAgentInfo->ulAgentID, 0);
            bIsPub = DOS_TRUE;
            break;
        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Agent %u is in an invalid status.", pstAgentInfo->ulAgentID);
            return DOS_FAIL;
            break;
    }

    pstAgentInfo->bNeedConnected = DOS_TRUE;

    if (bIsPub)
    {
        sc_agent_status_notify(pstAgentInfo);
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Request set agnet status to signin. Agent: %u"
                    , pstAgentInfo->ulAgentID);

    return DOS_SUCC;
}

U32 sc_agent_access_set_sigin(SC_AGENT_NODE_ST *pstAgent, SC_SRV_CB *pstSCB, SC_LEG_CB *pstLegCB)
{
    U32 ulRet = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstAgent)
        || DOS_ADDR_INVALID(pstAgent->pstAgentInfo)
        || DOS_ADDR_INVALID(pstSCB)
        || DOS_ADDR_INVALID(pstLegCB))
    {
        return DOS_FAIL;
    }

    /* ͨ�� ������ ��ǩʱ */
    if (pstLegCB->stCall.ucPeerType == SC_LEG_PEER_INBOUND_INTERNAL)
    {
        pstLegCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND_INTERNAL;
    }
    else if (pstLegCB->stCall.ucPeerType == SC_LEG_PEER_INBOUND_TT)
    {
        pstLegCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND_TT;
    }
    else
    {
        return DOS_FAIL;
    }

    /* �޸������к��� */
    dos_snprintf(pstLegCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstLegCB->stCall.stNumInfo.szOriginalCallee), pstLegCB->stCall.stNumInfo.szOriginalCalling);
    /* �����к������л�ȡ���к��� */
    ulRet = sc_caller_setting_select_number(pstAgent->pstAgentInfo->ulCustomerID, pstAgent->pstAgentInfo->ulAgentID, SC_SRC_CALLER_TYPE_AGENT, pstLegCB->stCall.stNumInfo.szOriginalCalling, SC_NUM_LENGTH);
    if (ulRet != DOS_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_HTTP_API), "Agent signin customID(%u) get caller number FAIL by agent(%u)", pstAgent->pstAgentInfo->ulCustomerID, pstAgent->pstAgentInfo->ulAgentID);

        return DOS_FAIL;
    }

    dos_snprintf(pstLegCB->stCall.stNumInfo.szRealCallee, sizeof(pstLegCB->stCall.stNumInfo.szRealCallee), pstLegCB->stCall.stNumInfo.szOriginalCallee);
    dos_snprintf(pstLegCB->stCall.stNumInfo.szRealCalling, sizeof(pstLegCB->stCall.stNumInfo.szRealCalling), pstLegCB->stCall.stNumInfo.szOriginalCalling);

    dos_snprintf(pstLegCB->stCall.stNumInfo.szCallee, sizeof(pstLegCB->stCall.stNumInfo.szCallee), pstLegCB->stCall.stNumInfo.szOriginalCallee);
    dos_snprintf(pstLegCB->stCall.stNumInfo.szCalling, sizeof(pstLegCB->stCall.stNumInfo.szCalling), pstLegCB->stCall.stNumInfo.szOriginalCalling);

    pstAgent->pstAgentInfo->ulLegNo = pstLegCB->ulCBNo;
    pstAgent->pstAgentInfo->bNeedConnected = DOS_TRUE;
    pstAgent->pstAgentInfo->bConnected = DOS_TRUE;

    pstLegCB->ulIndSCBNo = pstSCB->ulSCBNo;
    pstLegCB->ulSCBNo = U32_BUTT;

    sc_scb_init(pstSCB);
    pstSCB->bValid = DOS_TRUE;
    pstSCB->ulAllocTime = time(NULL);

    pstSCB->stSigin.stSCBTag.bValid = DOS_TRUE;
    pstSCB->stSigin.pstAgentNode = pstAgent;
    pstSCB->stSigin.ulLegNo = pstLegCB->ulCBNo;
    pstSCB->ulAgentID = pstAgent->pstAgentInfo->ulAgentID;
    pstSCB->ulCustomerID = pstAgent->pstAgentInfo->ulCustomerID;

    pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = (SC_SCB_TAG_ST *)&pstSCB->stSigin;

    pstSCB->stSigin.stSCBTag.usStatus = SC_SIGIN_ALERTING;

    pstAgent->pstAgentInfo->ucWorkStatus = SC_ACD_WORK_IDEL;

    sc_agent_status_notify(pstAgent->pstAgentInfo);

    return DOS_SUCC;
}

U32 sc_agent_set_signout(SC_AGENT_INFO_ST *pstAgentQueueInfo, U32 ulOperatingType)
{
    SC_SRV_CB   *pstSCB       = NULL;
    SC_LEG_CB   *pstLegCB     = NULL;
    BOOL         bIsPub       = DOS_FALSE;

    if (DOS_ADDR_INVALID(pstAgentQueueInfo))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_agent_stat(SC_AGENT_STAT_SIGNOUT, pstAgentQueueInfo, pstAgentQueueInfo->ulAgentID, 0);
    switch (pstAgentQueueInfo->ucWorkStatus)
    {
        case SC_ACD_WORK_OFFLINE:
        case SC_ACD_WORK_BUSY:
        case SC_ACD_WORK_AWAY:
            pstAgentQueueInfo->ucWorkStatus = SC_ACD_WORK_IDEL;
            bIsPub = DOS_TRUE;
            break;
        case SC_ACD_WORK_IDEL:
            bIsPub = DOS_TRUE;
            break;
        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Agent %u is in an invalid status.", pstAgentQueueInfo->ulAgentID);
            return DOS_FAIL;
            break;
    }

    /* ֻҪ�к��ж��� */
    if ((pstAgentQueueInfo->bConnected || pstAgentQueueInfo->bNeedConnected)
        && pstAgentQueueInfo->ulLegNo <= SC_LEG_CB_SIZE)
    {
        /* ����� */
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
                /* ��ִ��playback_stop������hungupִ�в��� */
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
    if (bIsPub)
    {
        sc_agent_status_notify(pstAgentQueueInfo);
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Request set agnet status to signout. Agent: %u"
                , pstAgentQueueInfo->ulAgentID);

    return DOS_SUCC;
}

U32 sc_agent_work_set_busy(SC_AGENT_INFO_ST *pstAgentQueueInfo)
{
    BOOL bIsPub     = DOS_FALSE;

    if (DOS_ADDR_INVALID(pstAgentQueueInfo))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (pstAgentQueueInfo->ucWorkStatus)
    {
        case SC_ACD_WORK_OFFLINE:
        case SC_ACD_WORK_IDEL:
        case SC_ACD_WORK_AWAY:
            pstAgentQueueInfo->ucWorkStatus = SC_ACD_WORK_BUSY;
            bIsPub = DOS_TRUE;
            break;

        case SC_ACD_WORK_BUSY:
            break;
        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_ACD), "Agent %u is in an invalid status.", pstAgentQueueInfo->ulAgentID);
            return DOS_FAIL;
    }

    if (bIsPub)
    {
        sc_agent_status_notify(pstAgentQueueInfo);
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Request set agnet status to busy. Agent: %u"
                    , pstAgentQueueInfo->ulAgentID);

    return DOS_SUCC;
}

U32 sc_agent_work_set_idle(SC_AGENT_INFO_ST *pstAgentQueueInfo)
{
    BOOL            bIsPub          = DOS_FALSE;
    SC_LEG_CB       *pstCallingCB   = NULL;

    if (DOS_ADDR_INVALID(pstAgentQueueInfo))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (pstAgentQueueInfo->ucWorkStatus)
    {
        case SC_ACD_WORK_OFFLINE:
        case SC_ACD_WORK_BUSY:
        case SC_ACD_WORK_AWAY:
        case SC_ACD_WORK_IDEL:
            pstAgentQueueInfo->ucWorkStatus = SC_ACD_WORK_IDEL;
            pstAgentQueueInfo->ucServStatus = SC_ACD_SERV_IDEL;
            if (!pstAgentQueueInfo->bNeedConnected)
            {
                if (pstAgentQueueInfo->ulLegNo != U32_BUTT)
                {
                    pstCallingCB = sc_lcb_get(pstAgentQueueInfo->ulLegNo);
                    if (DOS_ADDR_VALID(pstCallingCB))
                    {
                        sc_lcb_free(pstCallingCB);
                    }
                    pstAgentQueueInfo->ulLegNo = U32_BUTT;
                }
            }
            bIsPub = DOS_TRUE;
            break;
        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_ACD), "Agent %u is in an invalid status.", pstAgentQueueInfo->ulAgentID);
            return DOS_FAIL;
    }

    if (bIsPub)
    {
        sc_agent_status_notify(pstAgentQueueInfo);
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Request set agnet status to idle. Agent: %u"
                    , pstAgentQueueInfo->ulAgentID);

    return DOS_SUCC;
}

U32 sc_agent_work_set_rest(SC_AGENT_INFO_ST *pstAgentQueueInfo)
{
    BOOL bIsPub     = DOS_FALSE;

    if (DOS_ADDR_INVALID(pstAgentQueueInfo))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (pstAgentQueueInfo->ucWorkStatus)
    {
        case SC_ACD_WORK_OFFLINE:
        case SC_ACD_WORK_IDEL:
        case SC_ACD_WORK_BUSY:
            pstAgentQueueInfo->ucWorkStatus = SC_ACD_WORK_AWAY;
            bIsPub = DOS_TRUE;
            break;

        case SC_ACD_WORK_AWAY:
            break;
        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_ACD), "Agent %u is in an invalid status.", pstAgentQueueInfo->ulAgentID);
            return DOS_FAIL;
    }

    if (bIsPub)
    {
        sc_agent_status_notify(pstAgentQueueInfo);
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Request set agnet status to busy. Agent: %u"
                    , pstAgentQueueInfo->ulAgentID);

    return DOS_SUCC;
}

U32 sc_agent_work_set_login(SC_AGENT_INFO_ST *pstAgentQueueInfo, U32 ulOperatingType)
{
    BOOL         bIsPub       = DOS_FALSE;

    if (DOS_ADDR_INVALID(pstAgentQueueInfo))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_agent_stat(SC_AGENT_STAT_ONLINE, pstAgentQueueInfo, pstAgentQueueInfo->ulAgentID, 0);

    switch (pstAgentQueueInfo->ucWorkStatus)
    {
        case SC_ACD_WORK_OFFLINE:
            pstAgentQueueInfo->ucWorkStatus = SC_ACD_WORK_AWAY;
            bIsPub = DOS_TRUE;
            break;
        case SC_ACD_WORK_IDEL:
        case SC_ACD_WORK_BUSY:
        case SC_ACD_WORK_AWAY:
            break;
        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Agent %u is in an invalid status.", pstAgentQueueInfo->ulAgentID);
            return DOS_FAIL;
            break;
    }

    if (bIsPub)
    {
        sc_agent_status_notify(pstAgentQueueInfo);
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Request set agnet status to login. Agent: %u"
                    , pstAgentQueueInfo->ulAgentID);

    return DOS_SUCC;
}

VOID sc_agent_work_set_logout(U64 p)
{
    SC_AGENT_INFO_ST *pstAgentQueueInfo = (SC_AGENT_INFO_ST *)p;
    BOOL              bIsPub            = DOS_FALSE;

    if (DOS_ADDR_INVALID(pstAgentQueueInfo))
    {
        DOS_ASSERT(0);
        return;
    }

    /* ֻ���ڳ�ǩ�ˣ��Ų������ */
    if (pstAgentQueueInfo->bConnected)
    {
        /* ������� */
        pstAgentQueueInfo->bNeedConnected = DOS_FALSE;
    }

    sc_agent_stat(SC_AGENT_STAT_OFFLINE, pstAgentQueueInfo, pstAgentQueueInfo->ulAgentID, 0);

    switch (pstAgentQueueInfo->ucWorkStatus)
    {
        case SC_ACD_WORK_OFFLINE:
            break;

        case SC_ACD_WORK_IDEL:
        case SC_ACD_WORK_BUSY:
        case SC_ACD_WORK_AWAY:
            pstAgentQueueInfo->ucWorkStatus = SC_ACD_WORK_OFFLINE;
            bIsPub = DOS_TRUE;
            break;
        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Agent %u is in an invalid status.", pstAgentQueueInfo->ulAgentID);
            break;
    }

    /* ��� bConnected Ϊtrue��Ӧ�õȴ����Ϊfalse, �ٽ���ϯ��״̬��Ϊ�ǳ� */
    if (pstAgentQueueInfo->bConnected)
    {
        return;
    }

    if (DOS_ADDR_VALID(pstAgentQueueInfo->htmrLogout))
    {
        dos_tmr_stop(&pstAgentQueueInfo->htmrLogout);
        pstAgentQueueInfo->htmrLogout = NULL;
    }

    if (bIsPub)
    {
        sc_agent_status_notify(pstAgentQueueInfo);

        /* �޸����ݿ� */
        sc_agent_update_status_db(pstAgentQueueInfo);
    }

    pstAgentQueueInfo->bNeedConnected = DOS_FALSE;
    pstAgentQueueInfo->bConnected = DOS_FALSE;

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Request set agnet status to logout. Agent:%u"
                , pstAgentQueueInfo->ulAgentID);
}

U32 sc_agent_work_set_force_logout(SC_AGENT_INFO_ST *pstAgentQueueInfo, U32 ulOperatingType)
{
    BOOL              bIsPub            = DOS_FALSE;

    if (DOS_ADDR_INVALID(pstAgentQueueInfo))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_agent_stat(SC_AGENT_STAT_OFFLINE, pstAgentQueueInfo, pstAgentQueueInfo->ulAgentID, 0);

    switch (pstAgentQueueInfo->ucWorkStatus)
    {
        case SC_ACD_WORK_OFFLINE:
        case SC_ACD_WORK_IDEL:
        case SC_ACD_WORK_BUSY:
        case SC_ACD_WORK_AWAY:
            pstAgentQueueInfo->ucWorkStatus = SC_ACD_WORK_OFFLINE;
            bIsPub = DOS_TRUE;
            break;

        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Agent %u is in an invalid status.", pstAgentQueueInfo->ulAgentID);
            return DOS_FAIL;
    }

    /* ֻҪ�к��ж��� */
    if (pstAgentQueueInfo->bConnected || pstAgentQueueInfo->ulLegNo != U32_BUTT)
    {
        /*  ������� */
    }

    if (bIsPub)
    {
        sc_agent_status_notify(pstAgentQueueInfo);
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Request force logout for agnet %u. "
                , pstAgentQueueInfo->ulAgentID);

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
 * ����: U32 sc_acd_http_agent_update_proc(U32 ulAction, U32 ulAgentID, S8 *pszUserID)
 * ����: ����HTTP������������
 * ����:
 *      U32 ulAction : ����
 *      U32 ulAgentID : ��ϯID
 *      S8 *pszUserID : ��ϯSIP User ID
 * ����ֵ: �ɹ�����DOS_SUCC,���򷵻�DOS_FAIL
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
    SC_AGENT_NODE_ST        *pstAgentQueueNode  = NULL;
    SC_AGENT_INFO_ST        *pstAgentInfo       = NULL;     /* ��ϯ��Ϣ */
    HASH_NODE_S             *pstHashNode        = NULL;
    U32                     ulHashIndex         = 0;
    U32                     ulResult            = DOS_FAIL;

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
    //ulOldStatus = pstAgentInfo->ucStatus;
    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_ACD), "Agent status changed. Agent: %u, Action: %u", ulAgentID, ulAction);

    switch(ulAction)
    {
        case SC_ACTION_AGENT_BUSY:
            ulResult = sc_agent_work_set_busy(pstAgentInfo);
            break;

        case SC_ACTION_AGENT_IDLE:
            ulResult = sc_agent_work_set_idle(pstAgentInfo);
            break;

        case SC_ACTION_AGENT_REST:
            ulResult = sc_agent_work_set_rest(pstAgentInfo);
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
            ulResult = sc_agent_work_set_login(pstAgentInfo, ulOperatingType);
            break;

        case SC_ACTION_AGENT_LOGOUT:
            if (pstAgentInfo->htmrLogout)
            {
                dos_tmr_stop(&pstAgentInfo->htmrLogout);
                pstAgentInfo->htmrLogout = NULL;
            }

            ulResult = dos_tmr_start(&pstAgentInfo->htmrLogout, 2000, sc_agent_work_set_logout, (U64)pstAgentInfo, TIMER_NORMAL_LOOP);
            break;

        case SC_ACTION_AGENT_FORCE_OFFLINE:
            ulResult = sc_agent_work_set_force_logout(pstAgentInfo, ulOperatingType);
            break;

        case SC_ACTION_AGENT_QUERY:
            ulResult = sc_agent_status_http_get_rsp(ulAgentID);
            break;

        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_ACD), "Invalid action for agent. Action:%u", ulAction);
            ulResult = DOS_FAIL;
    }

    /* �޸����ݿ� */
    sc_agent_update_status_db(pstAgentInfo);

    return ulResult;
}


/* ��¼��ϯͳ����Ϣ */
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
                /* �п��ܱ�ɾ���ˣ�����Ҫ���� */
                continue;
            }

            pstAgentInfo = pstAgentQueueNode->pstAgentInfo;
            if (pstAgentInfo->bWaitingDelete)
            {
                /* ��ɾ����*/
                continue;
            }

            /* �������ʱ�䳬��3���ӣ��ʹ���һ�� */
            if (pstAgentInfo->ucServStatus == SC_ACD_SERV_PROC
                && pstAgentInfo->ulLastProcTime != 0
                && time(NULL) - pstAgentInfo->ulLastProcTime > 3 * 60)
            {
                pstAgentInfo->ucServStatus = SC_ACD_SERV_IDEL;
                sc_agent_status_notify(pstAgentInfo);
            }
        }
    }

    return DOS_SUCC;
}


/*
 * ��  ��: sc_acd_agent_stat
 * ��  ��: ͳ����ϯ������Ϣ
 * ��  ��:
 *       U32 ulType : ͳ������
 *       U32 ulAgentID : ��ϯID
 *       SC_AGENT_INFO_ST *pstAgentInfo: ��ϯ�ṹ
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
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
            pstAgentInfo->stStat.ulCallCnt++;
            pstAgentInfo->stStat.ulIncomingCall++;
            break;

        case SC_AGENT_STAT_CALL_OUT:
            pstAgentInfo->stStat.ulCallCnt++;
            pstAgentInfo->stStat.ulOutgoingCall++;
            break;

        case SC_AGENT_STAT_CALL:
            pstAgentInfo->stStat.ulCallCnt++;
            break;

        case SC_AGENT_STAT_CALL_OK:
            pstAgentInfo->stStat.ulCallConnected++;
            pstAgentInfo->ulLastCallTime = ulCurrentTime;
            break;

        case SC_AGENT_STAT_CALL_FINISHED:
            if (pstAgentInfo->ulLastCallTime != 0 && ulCurrentTime >= pstAgentInfo->ulLastCallTime)
            {
                pstAgentInfo->stStat.ulTotalDuration = ulCurrentTime - pstAgentInfo->ulLastCallTime;
            }

            pstAgentInfo->ulLastCallTime = 0;
            break;

        case SC_AGENT_STAT_ONLINE:
            if (pstAgentInfo->ulLastOnlineTime == 0)
            {
                pstAgentInfo->ulLastOnlineTime = ulCurrentTime;
            }
            break;

        case SC_AGENT_STAT_OFFLINE:
            if (pstAgentInfo->ulLastOnlineTime != 0
                && ulCurrentTime >= pstAgentInfo->ulLastOnlineTime)
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
            pstAgentInfo->ulLastSignInTime = ulCurrentTime;
            break;

        case SC_AGENT_STAT_SIGNOUT:
            if (pstAgentInfo->ulLastSignInTime != 0
                && ulCurrentTime >= pstAgentInfo->ulLastSignInTime)
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


/* �����ϯ����ʵ�� д��ͳ����Ϣ 2h */
U32 sc_agent_audit(U32 ulCycle, VOID *ptr)
{
    HASH_NODE_S                 *pstHashNode       = NULL;
    SC_AGENT_NODE_ST  *pstAgentQueueNode = NULL;
    SC_AGENT_INFO_ST        *pstAgentInfo      = NULL;
    U32             ulHashIndex  = 0;

    static U32 ulLastWriteTime = 0;

    if (ulLastWriteTime == 0)
    {
        ulLastWriteTime = time(NULL);
    }
    else
    {
        if (time(NULL) - ulLastWriteTime < 10 * 60)
        {
            return DOS_SUCC;
        }

        ulLastWriteTime = time(NULL);
    }

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
                /* �п��ܱ�ɾ���ˣ�����Ҫ���� */
                continue;
            }

            pstAgentInfo = pstAgentQueueNode->pstAgentInfo;
            if (pstAgentInfo->bWaitingDelete)
            {
                /* ��ɾ����*/
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
    stSiteInfo.ucWorkStatus = SC_ACD_WORK_OFFLINE;
    stSiteInfo.ucServStatus = SC_ACD_SERV_IDEL;
    stSiteInfo.ulCustomerID = ulCustomID;
    stSiteInfo.aulGroupID[0] = ulGroupID0;
    stSiteInfo.aulGroupID[1] = ulGroupID1;
    stSiteInfo.bValid = DOS_TRUE;
    stSiteInfo.bRecord = ulRecordFlag;
    stSiteInfo.ulLegNo = U32_BUTT;
    stSiteInfo.bGroupHeader = ulIsHeader;
    stSiteInfo.ucBindType = (U8)ulSelectType;
    stSiteInfo.ucCallStatus = SC_ACD_CALL_NONE;
    if (stSiteInfo.ucWorkStatus != SC_ACD_WORK_OFFLINE)
    {
        stSiteInfo.bLogin = DOS_TRUE;
    }
    else
    {
        stSiteInfo.bLogin = DOS_FALSE;
    }
    stSiteInfo.bWaitingDelete = DOS_FALSE;
    stSiteInfo.bSelected = DOS_FALSE;
    stSiteInfo.bConnected = DOS_FALSE;
    stSiteInfo.bMarkCustomer = DOS_FALSE;
    stSiteInfo.bIsInterception = DOS_FALSE;
    stSiteInfo.bIsWhisper = DOS_FALSE;
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

    /* �鿴��ǰҪ��ӵ���ϯ�Ƿ��Ѿ����ڣ�������ڣ���׼�����¾ͺ� */
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
            /* ������ϯ��û��ȥ�˱���飬����ǣ�����Ҫ����ϯ��������� */
            for (ulIndex = 0; ulIndex < MAX_GROUP_PER_SITE; ulIndex++)
            {
                if (pstAgentQueueNode->pstAgentInfo->aulGroupID[ulIndex] != U32_BUTT
                   && pstAgentQueueNode->pstAgentInfo->aulGroupID[ulIndex] != 0)
                {
                    /* �޸�֮ǰ��ID�Ϸ����޸�֮����ID�Ϸ�������Ҫ����ǰ������ID�Ƿ���ͬ����ͬ�Ͳ���ʲô�� */
                    if (stSiteInfo.aulGroupID[ulIndex] != U32_BUTT
                       && stSiteInfo.aulGroupID[ulIndex] != 0 )
                    {
                        if (pstAgentQueueNode->pstAgentInfo->aulGroupID[ulIndex] != stSiteInfo.aulGroupID[ulIndex])
                        {
                            /* �ӱ����ɾ�� */
                            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ACD), "Agent %u will be removed from Group %u."
                                             , pstAgentQueueNode->pstAgentInfo->ulAgentID
                                             , pstAgentQueueNode->pstAgentInfo->aulGroupID[ulIndex]);

                            ulRest = sc_agent_group_remove_agent(pstAgentQueueNode->pstAgentInfo->aulGroupID[ulIndex]
                                                                , pstAgentQueueNode->pstAgentInfo->ulAgentID);
                            if (DOS_SUCC == ulRest)
                            {
                                /* ��ӵ��µ��� */
                                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ACD), "Agent %u will be added into Group %u."
                                                , stSiteInfo.aulGroupID[ulIndex]
                                                , pstAgentQueueNode->pstAgentInfo->ulAgentID);

                                pstAgentQueueNode->pstAgentInfo->aulGroupID[ulIndex] = stSiteInfo.aulGroupID[ulIndex];

                                sc_agent_group_add_agent(stSiteInfo.aulGroupID[ulIndex], pstAgentQueueNode->pstAgentInfo);
                            }
                        }
                    }
                    /* �޸�֮ǰ��ID�Ϸ����޸�֮����ID���Ϸ�������Ҫ��agent��֮ǰ��������ɾ���� */
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
                    /* �޸�֮ǰ��ID���Ϸ����޸�֮����ID�Ϸ�������Ҫ��agent��ӵ������õ��� */
                    if (stSiteInfo.aulGroupID[ulIndex] != U32_BUTT
                        && stSiteInfo.aulGroupID[ulIndex] != 0)
                    {
                        /* ��ӵ��µ��� */
                        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_ACD), "Agent %u will be add into group %u."
                                        , pstAgentQueueNode->pstAgentInfo->ulAgentID
                                        , stSiteInfo.aulGroupID[ulIndex]);

                        pstAgentQueueNode->pstAgentInfo->aulGroupID[ulIndex] = stSiteInfo.aulGroupID[ulIndex];
                        sc_agent_group_add_agent(stSiteInfo.aulGroupID[ulIndex], pstAgentQueueNode->pstAgentInfo);
                    }
                    else
                    {
                        /* �޸�֮ǰ��ID���Ϸ����޸�֮����ID���Ϸ�����ɶҲ������ */
                    }
                }
            }

            pstAgentQueueNode->pstAgentInfo->bRecord = stSiteInfo.bRecord;
            pstAgentQueueNode->pstAgentInfo->bAllowAccompanying = stSiteInfo.bAllowAccompanying;
            pstAgentQueueNode->pstAgentInfo->bGroupHeader = stSiteInfo.bGroupHeader;
            pstAgentQueueNode->pstAgentInfo->ucBindType = stSiteInfo.ucBindType;
            if (ulAgentIndex == SC_INVALID_INDEX)
            {
                /* ���µ�����ϯʱ��״̬����Ҫ�仯����ʼ��ʱ������Ϊ���� */
                pstAgentQueueNode->pstAgentInfo->ucWorkStatus = stSiteInfo.ucWorkStatus;
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

    /* ����ϯ���뵽�� */
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


