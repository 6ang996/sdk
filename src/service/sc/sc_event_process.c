/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  �ļ���: sc_event_process.c
 *
 *  ����ʱ��: 2015��1��5��16:18:41
 *  ��    ��: Larry
 *  ��    ��: ����FS���ķ������ĸ����¼�
 *  �޸���ʷ:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */


#include <dos.h>
#include <esl.h>
#include <sys/time.h>
#include <pthread.h>
#include <bs_pub.h>
#include <libcurl/curl.h>
#include "sc_def.h"
#include "sc_debug.h"
#include "sc_acd_def.h"
#include "sc_ep.h"

/* Ӧ���ⲿ���� */
extern DB_HANDLE_ST         *g_pstSCDBHandle;

/* ESL ���ά�� */
SC_EP_HANDLE_ST          *g_pstHandle = NULL;
pthread_mutex_t          g_mutexEventList = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t           g_condEventList = PTHREAD_COND_INITIALIZER;

/* �¼����� REFER TO SC_EP_EVENT_NODE_ST */
list_t                   g_stEventList;

/* ·���������� REFER TO SC_ROUTE_NODE_ST */
DLL_S                    g_stRouteList;
pthread_mutex_t          g_mutexRouteList = PTHREAD_MUTEX_INITIALIZER;

/* �����б� refer to SC_GW_NODE_ST (ʹ��HASH) */
HASH_TABLE_S             *g_pstHashGW = NULL;
pthread_mutex_t          g_mutexHashGW = PTHREAD_MUTEX_INITIALIZER;

/* �������б� refer to SC_GW_GRP_NODE_ST (ʹ��HASH) */
HASH_TABLE_S             *g_pstHashGWGrp = NULL;
pthread_mutex_t          g_mutexHashGWGrp = PTHREAD_MUTEX_INITIALIZER;

/*
 * ������������ڴ��еĽṹ:
 * ʹ������hash�������غ���������
 * ÿ��������ڵ���ʹ��˫���������ڵ�ǰ����������ش洢������
 * ��ÿһ˫������ڵ���������Ǵ洢����hash��hash�ڵ��������
 */

/* SIP�˻�HASH�� REFER TO SC_USER_ID_NODE_ST */
HASH_TABLE_S             *g_pstHashSIPUserID  = NULL;
pthread_mutex_t          g_mutexHashSIPUserID = PTHREAD_MUTEX_INITIALIZER;

/* DID����hash�� REFER TO SC_DID_NODE_ST */
HASH_TABLE_S             *g_pstHashDIDNum  = NULL;
pthread_mutex_t          g_mutexHashDIDNum = PTHREAD_MUTEX_INITIALIZER;

/* ������HASH�� */
HASH_TABLE_S             *g_pstHashBlackList  = NULL;
pthread_mutex_t          g_mutexHashBlackList = PTHREAD_MUTEX_INITIALIZER;


CURL *g_pstCurlHandle;


U32 sc_ep_call_notify(SC_ACD_AGENT_INFO_ST *pstAgentInfo, S8 *szCaller)
{
    S8 szURL[256] = { 0, };
    U32 ulTimeout = 2;
    U32 ulRet = 0;

    if (DOS_ADDR_INVALID(pstAgentInfo)
        || DOS_ADDR_INVALID(szCaller))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if (DOS_ADDR_INVALID(g_pstCurlHandle))
    {
        g_pstCurlHandle = curl_easy_init();
        if (DOS_ADDR_INVALID(g_pstCurlHandle))
        {
            DOS_ASSERT(0);

            return DOS_FAIL;
        }
    }

    dos_snprintf(szURL, sizeof(szURL), "http://localhost/pub?id=%d_%s_%s"
                , pstAgentInfo->ulSiteID
                , pstAgentInfo->szEmpNo
                , pstAgentInfo->szExtension);

    curl_easy_reset(g_pstCurlHandle);
    curl_easy_setopt(g_pstCurlHandle, CURLOPT_URL, szURL);
    curl_easy_setopt(g_pstCurlHandle, CURLOPT_POSTFIELDS, szCaller);
    curl_easy_setopt(g_pstCurlHandle, CURLOPT_TIMEOUT, ulTimeout);
    ulRet = curl_easy_perform(g_pstCurlHandle);
    if(CURLE_OK != ulRet)
    {
        sc_logr_notice(SC_ESL, "CURL post FAIL.Caller:%d.", szCaller);

        return DOS_FAIL;
    }
    else
    {
        sc_logr_notice(SC_ESL, "CURL post SUCC.Caller:%d.", szCaller);

        return DOS_SUCC;
    }

}

/**
 * ����: VOID sc_ep_sip_userid_init(SC_USER_ID_NODE_ST *pstUserID)
 * ����: ��ʼ��pstUserID��ָ���User ID�����ṹ
 * ����:
 *      SC_USER_ID_NODE_ST *pstUserID ��Ҫ���ʼ���Ľṹ
 * ����ֵ: VOID
 */
VOID sc_ep_sip_userid_init(SC_USER_ID_NODE_ST *pstUserID)
{
    if (pstUserID)
    {
        dos_memzero(pstUserID, sizeof(SC_USER_ID_NODE_ST));

        pstUserID->ulCustomID = U32_BUTT;
        pstUserID->ulSIPID = U32_BUTT;
    }
}

/**
 * ����: VOID sc_ep_did_init(SC_DID_NODE_ST *pstDIDNum)
 * ����: ��ʼ��pstDIDNum��ָ���DID���������ṹ
 * ����:
 *      SC_DID_NODE_ST *pstDIDNum ��Ҫ���ʼ����DID����ṹ
 * ����ֵ: VOID
 */
VOID sc_ep_did_init(SC_DID_NODE_ST *pstDIDNum)
{
    if (pstDIDNum)
    {
        dos_memzero(pstDIDNum, sizeof(SC_DID_NODE_ST));
        pstDIDNum->ulBindID = U32_BUTT;
        pstDIDNum->ulBindType = U32_BUTT;
        pstDIDNum->ulCustomID = U32_BUTT;
        pstDIDNum->ulDIDID = U32_BUTT;
    }
}

/**
 * ����: VOID sc_ep_route_init(SC_ROUTE_NODE_ST *pstRoute)
 * ����: ��ʼ��pstRoute��ָ���·�������ṹ
 * ����:
 *      SC_ROUTE_NODE_ST *pstRoute ��Ҫ���ʼ����·�������ṹ
 * ����ֵ: VOID
 */
VOID sc_ep_route_init(SC_ROUTE_NODE_ST *pstRoute)
{
    if (pstRoute)
    {
        dos_memzero(pstRoute, sizeof(SC_ROUTE_NODE_ST));
        pstRoute->ulID = U32_BUTT;

        pstRoute->ucHourBegin = 0;
        pstRoute->ucMinuteBegin = 0;
        pstRoute->ucHourEnd = 0;
        pstRoute->ucMinuteEnd = 0;
    }
}

/**
 * ����: VOID sc_ep_gw_init(SC_GW_NODE_ST *pstGW)
 * ����: ��ʼ��pstGW��ָ������������ṹ
 * ����:
 *      SC_GW_NODE_ST *pstGW ��Ҫ���ʼ�������������ṹ
 * ����ֵ: VOID
 */
VOID sc_ep_gw_init(SC_GW_NODE_ST *pstGW)
{
    if (pstGW)
    {
        dos_memzero(pstGW, sizeof(SC_GW_NODE_ST));
        pstGW->ulGWID = U32_BUTT;
    }
}

/**
 * ����: VOID sc_ep_gw_init(SC_GW_NODE_ST *pstGW)
 * ����: ��ʼ��pstGW��ָ������������ṹ
 * ����:
 *      SC_GW_NODE_ST *pstGW ��Ҫ���ʼ�������������ṹ
 * ����ֵ: VOID
 */
VOID sc_ep_black_init(SC_BLACK_LIST_NODE *pstBlackListNode)
{
    if (pstBlackListNode)
    {
        dos_memzero(pstBlackListNode, sizeof(SC_BLACK_LIST_NODE));
        pstBlackListNode->ulID = U32_BUTT;
        pstBlackListNode->ulCustomerID = U32_BUTT;
        pstBlackListNode->ulType = U32_BUTT;
        pstBlackListNode->szNum[0] = '\0';
    }
}


/* �������hash���� */
U32 sc_ep_gw_grp_hash_func(U32 ulGWGrpID)
{
    return ulGWGrpID % SC_GW_GRP_HASH_SIZE;
}

/* ���ص�hash���� */
U32 sc_ep_gw_hash_func(U32 ulGWID)
{
    return ulGWID % SC_GW_GRP_HASH_SIZE;
}

/**
 * ����: static U32 sc_sip_userid_hash_func(S8 *pszUserID)
 * ����: ͨ��pszUserID����SIP User IDHash���HASHֵ
 * ����:
 *      S8 *pszUserID ��ǰHASH�ڵ��User ID
 * ����ֵ: U32 ����hashֵ
 */
static U32 sc_sip_userid_hash_func(S8 *pszUserID)
{
    U32 ulIndex;
    U32 ulHashIndex;

    ulIndex = 0;
    for (;;)
    {
        if ('\0' == pszUserID[ulIndex])
        {
            break;
        }

        ulHashIndex += (pszUserID[ulIndex] << 3);

        ulIndex++;
    }

    return ulHashIndex % SC_IP_USERID_HASH_SIZE;
}

/**
 * ����: static U32 sc_sip_did_hash_func(S8 *pszDIDNum)
 * ����: ͨ��pszDIDNum����DID����Hash���HASHֵ
 * ����:
 *      S8 *pszDIDNum ��ǰHASH�ڵ��DID����
 * ����ֵ: U32 ����hashֵ
 */
static U32 sc_sip_did_hash_func(S8 *pszDIDNum)
{
    U32 ulIndex;
    U32 ulHashIndex;

    ulIndex = 0;
    for(;;)
    {
        if ('\0' == pszDIDNum[ulIndex])
        {
            break;
        }

        ulHashIndex += (pszDIDNum[ulIndex] << 3);

        ulIndex++;
    }

    return ulHashIndex % SC_IP_DID_HASH_SIZE;
}



/**
 * ����: static U32 sc_black_list_hash_func(S8 *pszNum)
 * ����: ���������hash�ڵ��hashֵ
 * ����:
 *      S8 *pszNum : ��ǰ����������
 * ����ֵ: U32 ����hashֵ
 */
static U32 sc_ep_black_list_hash_func(U32 ulID)
{
    return ulID % SC_IP_USERID_HASH_SIZE;
}


/* ������hash����Һ��� */
S32 sc_ep_gw_grp_hash_find(VOID *pObj, HASH_NODE_S *pstHashNode)
{
    SC_GW_GRP_NODE_ST *pstGWGrpNode;

    U32 ulGWGrpIndex = 0;

    if (DOS_ADDR_INVALID(pObj)
        || DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    ulGWGrpIndex = *(U32 *)pObj;
    pstGWGrpNode = pstHashNode->pHandle;

    if (ulGWGrpIndex == pstGWGrpNode->ulGWGrpID)
    {
        return DOS_SUCC;
    }
    else
    {
        return DOS_FAIL;
    }
}

/* ����hash����Һ��� */
S32 sc_ep_gw_hash_find(VOID *pObj, HASH_NODE_S *pstHashNode)
{
    SC_GW_NODE_ST *pstGWNode;

    U32 ulGWIndex = 0;

    if (DOS_ADDR_INVALID(pObj)
        || DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    ulGWIndex = *(U32 *)pObj;
    pstGWNode = pstHashNode->pHandle;

    if (ulGWIndex == pstGWNode->ulGWID)
    {
        return DOS_SUCC;
    }
    else
    {
        return DOS_FAIL;
    }

}


/* ����DID���� */
S32 sc_ep_did_hash_find(VOID *pObj, HASH_NODE_S *pstHashNode)
{
    S8 *pszDIDNum = NULL;
    SC_DID_NODE_ST *pstDIDInfo = NULL;

    if (DOS_ADDR_INVALID(pObj)
        || DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pszDIDNum = (S8 *)pObj;
    pstDIDInfo = pstHashNode->pHandle;

    if (dos_strnicmp(pstDIDInfo->szDIDNum, pszDIDNum, sizeof(pstDIDInfo->szDIDNum)))
    {
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/* ����SIP User ID */
S32 sc_ep_sip_userid_hash_find(VOID *pObj, HASH_NODE_S *pstHashNode)
{
    S8 *pszDIDNum = NULL;
    SC_USER_ID_NODE_ST *pstSIPInfo = NULL;

    if (DOS_ADDR_INVALID(pObj)
        || DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pszDIDNum = (S8 *)pObj;
    pstSIPInfo = pstHashNode->pHandle;

    if (dos_strnicmp(pstSIPInfo->szUserID, pszDIDNum, sizeof(pstSIPInfo->szUserID)))
    {
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/* ����·�� */
S32 sc_ep_route_find(VOID *pKey, DLL_NODE_S *pstDLLNode)
{
    SC_ROUTE_NODE_ST *pstRouteCurrent;
    U32 ulIndex;

    if (DOS_ADDR_INVALID(pKey)
        || DOS_ADDR_INVALID(pstDLLNode)
        || DOS_ADDR_INVALID(pstDLLNode->pHandle))
    {
        return DOS_FAIL;
    }

    ulIndex = *(U32 *)pKey;
    pstRouteCurrent = pstDLLNode->pHandle;

    if (ulIndex == pstRouteCurrent->ulID)
    {
        return DOS_SUCC;
    }

    return DOS_FAIL;
}

/* ���Һ����� */
S32 sc_ep_black_list_find(VOID *pObj, HASH_NODE_S *pstHashNode)
{
    SC_BLACK_LIST_NODE *pstBlackList = NULL;
    U32  ulID;

    if (DOS_ADDR_INVALID(pObj)
        || DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        return DOS_FAIL;
    }

    ulID = *(U32 *)pObj;
    pstBlackList = pstHashNode->pHandle;

    if (ulID == pstBlackList->ulID)
    {
        return DOS_SUCC;
    }

    return DOS_FAIL;
}

U32 sc_gateway_delete(U32 ulGatewayID)
{
    HASH_NODE_S   *pstHashNode = NULL;
    SC_GW_NODE_ST *pstGateway  = NULL;
    U32  ulIndex = U32_BUTT;

    ulIndex = sc_ep_gw_hash_func(ulGatewayID);
    pthread_mutex_lock(&g_mutexHashGW);
    pstHashNode = hash_find_node(g_pstHashGW, ulIndex, (VOID *)&ulGatewayID, sc_ep_gw_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        pthread_mutex_unlock(&g_mutexHashGW);
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstGateway = pstHashNode->pHandle;
    pstHashNode->pHandle = NULL;

    hash_delete_node(g_pstHashGW, pstHashNode, ulIndex);

    if (pstHashNode)
    {
        dos_dmem_free(pstHashNode);
        pstHashNode = NULL;
    }

    if (pstGateway)
    {
        dos_dmem_free(pstGateway);
        pstHashNode = NULL;
    }
    
    pthread_mutex_unlock(&g_mutexHashGW);

    return DOS_SUCC;
}

U32 sc_route_delete(U32 ulRouteID)
{
	DLL_NODE_S       *pstDLLNode   = NULL;
	SC_ROUTE_NODE_ST *pstRouteNode = NULL;

	pthread_mutex_lock(&g_mutexRouteList);
	pstDLLNode = dll_find(&g_stRouteList, (VOID *)&ulRouteID, sc_ep_route_find);
	if (DOS_ADDR_INVALID(pstDLLNode))
	{
	    pthread_mutex_unlock(&g_mutexRouteList);
	    DOS_ASSERT(0);
	    return DOS_FAIL;
	}
	pstRouteNode = pstDLLNode->pHandle;
	pstDLLNode->pHandle =  NULL;
	
	dll_delete(&g_stRouteList, pstDLLNode);

	if (pstRouteNode)
	{
		dos_dmem_free(pstRouteNode);
		pstRouteNode = NULL;
	}

	if (pstDLLNode)
	{
		dos_dmem_free(pstDLLNode);
		pstDLLNode = NULL;
	}
	
	pthread_mutex_unlock(&g_mutexRouteList);

	return DOS_SUCC;
}

U32 sc_gateway_grp_delete(U32 ulGwGroupID)
{
    HASH_NODE_S        *pstHashNode    = NULL;
    SC_GW_GRP_NODE_ST  *pstGwGroupNode = NULL;
    U32 ulIndex = U32_BUTT;

    /* ������������� */
    ulIndex = sc_ep_gw_grp_hash_func(ulGwGroupID);
    
    pthread_mutex_lock(&g_mutexHashGWGrp); 

    /* ����������ڵ��׵�ַ */
    pstHashNode = hash_find_node(g_pstHashGWGrp, ulIndex, (VOID *)&ulGwGroupID, sc_ep_gw_grp_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
    	|| DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
	    pthread_mutex_unlock(&g_mutexHashGW); 
	    DOS_ASSERT(0);
		return DOS_FAIL;
    }

	pstGwGroupNode = pstHashNode->pHandle;
	pstHashNode->pHandle = NULL;

    /* ɾ���ڵ� */
    hash_delete_node(g_pstHashGWGrp, pstHashNode, ulIndex);

    if (pstGwGroupNode)
    {
        dos_dmem_free(pstGwGroupNode);
        pstGwGroupNode = NULL;
    }

    if (pstHashNode)
    {
       dos_dmem_free(pstHashNode);
       pstHashNode = NULL;
    }
    
    pthread_mutex_unlock(&g_mutexHashGWGrp);

    return DOS_SUCC;
}

U32 sc_did_delete(U32 ulDidID, S8* pszDidNum)
{
    HASH_NODE_S     *pstHashNode = NULL;
    SC_DID_NODE_ST  *pstDidNode  = NULL;
    U32 ulIndex = U32_BUTT;

    #if 0
    if (dos_ltoa(ulDidID, szDidNum, sizeof(szDidNum)) < 0)
    {
       DOS_ASSERT(0);
       return DOS_FAIL;
    }
    szDidNum[sizeof(szDidNum) - 1] = '\0'; 
    #endif

    ulIndex = sc_sip_did_hash_func(pszDidNum);

    pthread_mutex_lock(&g_mutexHashDIDNum);
    pstHashNode = hash_find_node(g_pstHashDIDNum, ulIndex, (VOID *)pszDidNum, sc_ep_did_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        pthread_mutex_unlock(&g_mutexHashDIDNum);
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstDidNode = pstHashNode->pHandle;
    pstHashNode->pHandle = NULL;

    /* ɾ���ڵ� */
    hash_delete_node(g_pstHashDIDNum, pstHashNode, ulIndex);

    if (pstHashNode)
    {
        dos_dmem_free(pstHashNode);
        pstHashNode = NULL;
    }

    if (pstDidNode)
    {
       dos_dmem_free(pstDidNode);
       pstDidNode = NULL;
    }
    
    pthread_mutex_unlock(&g_mutexHashDIDNum);

    return DOS_SUCC;
}

U32 sc_black_list_delete(U32 ulBlackListID)
{
    HASH_NODE_S        *pstHashNode  = NULL;
    SC_BLACK_LIST_NODE *pstBlackList = NULL;
    U32  ulIndex = U32_BUTT;

    ulIndex = sc_ep_black_list_hash_func(ulBlackListID);

    pthread_mutex_lock(&g_mutexHashBlackList);

    pstHashNode = hash_find_node(g_pstHashBlackList, ulIndex, (VOID *)&ulBlackListID, sc_ep_black_list_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        pthread_mutex_unlock(&g_mutexHashBlackList);
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    pstBlackList = pstHashNode->pHandle;
    pstHashNode->pHandle = NULL;

    hash_delete_node(g_pstHashBlackList, pstHashNode, ulIndex);
    
    if (pstHashNode)
    {
        dos_dmem_free(pstHashNode);
        pstHashNode = NULL;
    }

    if (pstBlackList)
    {
        dos_dmem_free(pstBlackList);
        pstBlackList = NULL;
    }
    
    pthread_mutex_unlock(&g_mutexHashBlackList);

    return DOS_SUCC;
}


/**
 * ����: S32 sc_load_sip_userid_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
 * ����: ����SIP�˻�ʱ���ݿ��ѯ�Ļص������������ݼ���SIP�˻���HASH����
 * ����:
 *      VOID *pArg: ����
 *      S32 lCount: ������
 *      S8 **aszValues: ֵ�ѱ�
 *      S8 **aszNames: �ֶ����б�
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 */
S32 sc_load_sip_userid_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    SC_USER_ID_NODE_ST *pstSIPUserIDNodeNew = NULL;
    SC_USER_ID_NODE_ST *pstSIPUserIDNode = NULL;
    HASH_NODE_S        *pstHashNode      = NULL;
    BOOL               blProcessOK       = DOS_FALSE;
    U32                ulHashIndex       = 0;
    S32                lIndex            = 0;

    if (DOS_ADDR_INVALID(aszNames)
        || DOS_ADDR_INVALID(aszValues))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstSIPUserIDNodeNew = (SC_USER_ID_NODE_ST *)dos_dmem_alloc(sizeof(SC_USER_ID_NODE_ST));
    if (DOS_ADDR_INVALID(pstSIPUserIDNodeNew))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }
    sc_ep_sip_userid_init(pstSIPUserIDNodeNew);

    for (lIndex=0, blProcessOK=DOS_TRUE; lIndex<lCount; lIndex++)
    {
        if (0 == dos_strnicmp(aszNames[lIndex], "id", dos_strlen("id")))
        {
            if (dos_atoul(aszValues[lIndex], &pstSIPUserIDNodeNew->ulSIPID) < 0)
            {
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "customer_id", dos_strlen("customer_id")))
        {
            if (dos_atoul(aszValues[lIndex], &pstSIPUserIDNodeNew->ulCustomID) < 0)
            {
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "username", dos_strlen("username")))
        {
            if (DOS_ADDR_INVALID(aszValues[lIndex])
                || '\0' == aszValues[lIndex][0])
            {
                blProcessOK = DOS_FALSE;
                break;
            }

            dos_strncpy(pstSIPUserIDNodeNew->szUserID, aszValues[lIndex], sizeof(pstSIPUserIDNodeNew->szUserID));
            pstSIPUserIDNodeNew->szUserID[sizeof(pstSIPUserIDNodeNew->szUserID) - 1] = '\0';
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "extension", dos_strlen("extension")))
        {
            if (DOS_ADDR_INVALID(aszValues[lIndex])
                || '\0' == aszValues[lIndex][0])
            {
                blProcessOK = DOS_FALSE;
                break;
            }

            dos_strncpy(pstSIPUserIDNodeNew->szExtension, aszValues[lIndex], sizeof(pstSIPUserIDNodeNew->szExtension));
            pstSIPUserIDNodeNew->szExtension[sizeof(pstSIPUserIDNodeNew->szExtension) - 1] = '\0';
        }
    }

    if (!blProcessOK)
    {
        DOS_ASSERT(0);

        dos_dmem_free(pstSIPUserIDNodeNew);
        pstSIPUserIDNodeNew = NULL;
        return DOS_FALSE;
    }

    pthread_mutex_lock(&g_mutexHashSIPUserID);
    ulHashIndex = sc_sip_userid_hash_func(pstSIPUserIDNodeNew->szUserID);
    pstHashNode = hash_find_node(g_pstHashSIPUserID, ulHashIndex, (VOID *)pstSIPUserIDNodeNew->szUserID, sc_ep_sip_userid_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        pstHashNode = dos_dmem_alloc(sizeof(HASH_NODE_S));
        if (DOS_ADDR_INVALID(pstHashNode))
        {
            DOS_ASSERT(0);

            dos_dmem_free(pstSIPUserIDNodeNew);
            pstSIPUserIDNodeNew = NULL;
            pthread_mutex_unlock(&g_mutexHashSIPUserID);
            return DOS_FALSE;
        }

        sc_logr_debug(SC_ESL, "Load SIP User. ID: %d, Customer: %d, UserID: %s, Extension: %s"
                    , pstSIPUserIDNodeNew->ulSIPID
                    , pstSIPUserIDNodeNew->ulCustomID
                    , pstSIPUserIDNodeNew->szUserID
                    , pstSIPUserIDNodeNew->szExtension);

        HASH_Init_Node(pstHashNode);
        pstHashNode->pHandle = pstSIPUserIDNodeNew;
        ulHashIndex = sc_sip_userid_hash_func(pstSIPUserIDNodeNew->szUserID);

        hash_add_node(g_pstHashSIPUserID, (HASH_NODE_S *)pstHashNode, ulHashIndex, NULL);
    }
    else
    {
        pstSIPUserIDNode = pstHashNode->pHandle;
        if (DOS_ADDR_INVALID(pstSIPUserIDNode))
        {
            sc_logr_debug(SC_ESL, "%d", "Invalid data while update the sip user id.");

            dos_dmem_free(pstSIPUserIDNodeNew);
            pstSIPUserIDNodeNew = NULL;

            pthread_mutex_unlock(&g_mutexHashSIPUserID);
            return DOS_FAIL;
        }

        pstSIPUserIDNode->ulCustomID = pstSIPUserIDNodeNew->ulCustomID;
        pstSIPUserIDNode->ulSIPID = pstSIPUserIDNodeNew->ulSIPID;

        dos_strncpy(pstSIPUserIDNode->szUserID, pstSIPUserIDNodeNew->szUserID, sizeof(pstSIPUserIDNode->szUserID));
        pstSIPUserIDNode->szUserID[sizeof(pstSIPUserIDNode->szUserID) - 1] = '\0';

        dos_strncpy(pstSIPUserIDNode->szExtension, pstSIPUserIDNodeNew->szExtension, sizeof(pstSIPUserIDNode->szExtension));
        pstSIPUserIDNode->szExtension[sizeof(pstSIPUserIDNode->szExtension) - 1] = '\0';

        dos_dmem_free(pstSIPUserIDNodeNew);
        pstSIPUserIDNodeNew = NULL;
    }

    pthread_mutex_unlock(&g_mutexHashSIPUserID);

    return DOS_SUCC;
}

/**
 * ����: U32 sc_load_sip_userid()
 * ����: ����SIP�˻�����
 * ����:
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 */
U32 sc_load_sip_userid(U32 ulIndex)
{
    S8 szSQL[1024] = { 0, };

    if (SC_INVALID_INDEX == ulIndex)
    {
        dos_snprintf(szSQL, sizeof(szSQL), "SELECT id, customer_id, extension,username FROM tbl_sip where tbl_sip.status = 1;");
    }
    else
    {
        dos_snprintf(szSQL, sizeof(szSQL), "SELECT id, customer_id, extension,username FROM tbl_sip where tbl_sip.status = 1 AND id=%d;", ulIndex);
    }

    if (db_query(g_pstSCDBHandle, szSQL, sc_load_sip_userid_cb, NULL, NULL) != DB_ERR_SUCC)
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_ESL, "%s", "Load sip account fail.");
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/**
 * ����: S32 sc_load_black_list_cb()
 * ����: ����Blackʱ���ݿ��ѯ�Ļص������������ݼ��������hash��
 * ����:
 *      VOID *pArg: ����
 *      S32 lCount: ������
 *      S8 **aszValues: ֵ�ѱ�
 *      S8 **aszNames: �ֶ����б�
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 */
S32 sc_load_black_list_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    SC_BLACK_LIST_NODE *pstBlackListNode = NULL;
    SC_BLACK_LIST_NODE *pstBlackListTmp  = NULL;
    HASH_NODE_S        *pstHashNode      = NULL;
    BOOL               blProcessOK       = DOS_TRUE;
    S32                lIndex            = 0;
    U32                ulHashIndex       = 0;


    if (DOS_ADDR_INVALID(aszValues)
        || DOS_ADDR_INVALID(aszNames))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstBlackListNode = dos_dmem_alloc(sizeof(SC_BLACK_LIST_NODE));
    if (DOS_ADDR_INVALID(pstBlackListNode))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_ep_black_init(pstBlackListNode);

    for (blProcessOK = DOS_TRUE, lIndex=0; lIndex<lCount; lIndex++)
    {
        if (0 == dos_strnicmp(aszNames[lIndex], "id", dos_strlen("id")))
        {
            if (DOS_ADDR_INVALID(aszValues[lIndex])
                || dos_atoul(aszValues[lIndex], &pstBlackListNode->ulID) < 0)
            {
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "customer_id", dos_strlen("customer_id")))
        {
            if (DOS_ADDR_INVALID(aszValues[lIndex])
                || dos_atoul(aszValues[lIndex], &pstBlackListNode->ulCustomerID) < 0)
            {
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "regex_number", dos_strlen("regex_number")))
        {
            if (DOS_ADDR_INVALID(aszValues[lIndex])
                || '\0' == aszValues[lIndex][0])
            {
                blProcessOK = DOS_FALSE;
                break;
            }

            dos_strncpy(pstBlackListNode->szNum, aszValues[lIndex], sizeof(pstBlackListNode->szNum));
            pstBlackListNode->szNum[sizeof(pstBlackListNode->szNum) - 1] = '\0';
        }
    }

    if (!blProcessOK)
    {
        DOS_ASSERT(0);

        dos_dmem_free(pstBlackListNode);
        return DOS_FAIL;
    }

    ulHashIndex = sc_ep_black_list_hash_func(pstBlackListNode->ulID);
    pstHashNode = hash_find_node(g_pstHashBlackList, ulHashIndex, (VOID *)&pstBlackListNode->ulID, sc_ep_black_list_find);
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        pstHashNode = dos_dmem_alloc(sizeof(HASH_NODE_S));
        if (DOS_ADDR_INVALID(pstHashNode ))
        {
            DOS_ASSERT(0);

            dos_dmem_free(pstBlackListNode);
            return DOS_FAIL;
        }

        HASH_Init_Node(pstHashNode);
        pstHashNode->pHandle = pstBlackListNode;
        ulHashIndex = sc_ep_black_list_hash_func(pstBlackListNode->ulID);

        pthread_mutex_lock(&g_mutexHashBlackList);
        hash_add_node(g_pstHashBlackList, pstHashNode, ulHashIndex, NULL);
        pthread_mutex_unlock(&g_mutexHashBlackList);
    }
    else
    {
        pstBlackListTmp = pstHashNode->pHandle;
        if (DOS_ADDR_INVALID(pstBlackListTmp))
        {
            DOS_ASSERT(0);
            dos_dmem_free(pstBlackListNode);
            pstBlackListNode = 0;
            return DOS_FAIL;
        }

        pstBlackListTmp->ulType = pstBlackListNode->ulType;

        dos_strncpy(pstBlackListTmp->szNum, pstBlackListNode->szNum, sizeof(pstBlackListTmp->szNum));
        pstBlackListTmp->szNum[sizeof(pstBlackListTmp->szNum) - 1] = '\0';

        dos_dmem_free(pstBlackListNode);
        pstBlackListNode = 0;
    }

    return DOS_SUCC;
}


/**
 * ����: U32 sc_load_black_list()
 * ����: ���غ���������
 * ����:
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 */
U32 sc_load_black_list(U32 ulIndex)
{
    S8 szSQL[1024] = { 0, };

    if (SC_INVALID_INDEX == ulIndex)
    {
        dos_snprintf(szSQL, sizeof(szSQL), "SELECT id, customer_id, regex_number FROM tbl_blacklist;");
    }
    else
    {
        dos_snprintf(szSQL, sizeof(szSQL), "SELECT id, customer_id, regex_number FROM tbl_blacklist id=%u;", ulIndex);
    }

    if (db_query(g_pstSCDBHandle, szSQL, sc_load_black_list_cb, NULL, NULL) != DB_ERR_SUCC)
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_ESL, "%s", "Load sip account fail.");
        return DOS_FAIL;
    }

    return DOS_SUCC;
}


/**
 * ����: S32 sc_load_did_number_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
 * ����: ����DID����ʱ���ݿ��ѯ�Ļص������������ݼ���DID�����HASH����
 * ����:
 *      VOID *pArg: ����
 *      S32 lCount: ������
 *      S8 **aszValues: ֵ�ѱ�
 *      S8 **aszNames: �ֶ����б�
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 */
S32 sc_load_did_number_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    SC_DID_NODE_ST     *pstDIDNumNode    = NULL;
    SC_DID_NODE_ST     *pstDIDNumTmp     = NULL;
    HASH_NODE_S        *pstHashNode      = NULL;
    BOOL               blProcessOK       = DOS_FALSE;
    U32                ulHashIndex       = 0;
    S32                lIndex            = 0;

    if (DOS_ADDR_INVALID(aszNames)
        || DOS_ADDR_INVALID(aszValues))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstDIDNumNode = (SC_DID_NODE_ST *)dos_dmem_alloc(sizeof(SC_DID_NODE_ST));
    if (DOS_ADDR_INVALID(pstDIDNumNode))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_ep_did_init(pstDIDNumNode);

    for (lIndex=0, blProcessOK=DOS_TRUE; lIndex<lCount; lIndex++)
    {
        if (0 == dos_strnicmp(aszNames[lIndex], "id", dos_strlen("id")))
        {
            if (dos_atoul(aszValues[lIndex], &pstDIDNumNode->ulDIDID) < 0)
            {
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "customer_id", dos_strlen("customer_id")))
        {
            if (dos_atoul(aszValues[lIndex], &pstDIDNumNode->ulCustomID) < 0)
            {
                blProcessOK = DOS_FALSE;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "did_number", dos_strlen("did_number")))
        {
            if (DOS_ADDR_INVALID(aszValues[lIndex])
                || '\0' == aszValues[lIndex][0])
            {
                blProcessOK = DOS_FALSE;
                break;
            }

            dos_strncpy(pstDIDNumNode->szDIDNum, aszValues[lIndex], sizeof(pstDIDNumNode->szDIDNum));
            pstDIDNumNode->szDIDNum[sizeof(pstDIDNumNode->szDIDNum) - 1] = '\0';
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "bind_type", dos_strlen("bind_type")))
        {
            if (dos_atoul(aszValues[lIndex], &pstDIDNumNode->ulBindType) < 0
                || pstDIDNumNode->ulBindType >= SC_DID_BIND_TYPE_BUTT)
            {
                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "bind_id", dos_strlen("bind_id")))
        {
            if (dos_atoul(aszValues[lIndex], &pstDIDNumNode->ulBindID) < 0)
            {
                blProcessOK = DOS_FALSE;
                break;
            }
        }

    }

    if (!blProcessOK)
    {
        DOS_ASSERT(0);

        dos_dmem_free(pstDIDNumNode);
        pstDIDNumNode = NULL;
        return DOS_FALSE;
    }

    pthread_mutex_lock(&g_mutexHashDIDNum);
    ulHashIndex = sc_sip_did_hash_func(pstDIDNumNode->szDIDNum);
    pstHashNode = hash_find_node(g_pstHashDIDNum, ulHashIndex, (VOID *)pstDIDNumNode->szDIDNum, sc_ep_did_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        pstHashNode = dos_dmem_alloc(sizeof(HASH_NODE_S));
        if (DOS_ADDR_INVALID(pstHashNode))
        {
            DOS_ASSERT(0);

            dos_dmem_free(pstDIDNumNode);
            pstDIDNumNode = NULL;
            pthread_mutex_unlock(&g_mutexHashDIDNum);
            return DOS_FALSE;
        }

        HASH_Init_Node(pstHashNode);
        pstHashNode->pHandle = pstDIDNumNode;
        ulHashIndex = sc_sip_did_hash_func(pstDIDNumNode->szDIDNum);

        hash_add_node(g_pstHashDIDNum, (HASH_NODE_S *)pstHashNode, ulHashIndex, NULL);

    }
    else
    {
        pstDIDNumTmp = pstHashNode->pHandle;
        if (DOS_ADDR_INVALID(pstDIDNumTmp))
        {
            DOS_ASSERT(0);

            dos_dmem_free(pstDIDNumNode);
            pstDIDNumNode = NULL;

            pthread_mutex_unlock(&g_mutexHashDIDNum);
            return DOS_FAIL;
        }

        pstDIDNumTmp->ulCustomID = pstDIDNumNode->ulCustomID;
        pstDIDNumTmp->ulDIDID = pstDIDNumNode->ulDIDID;
        pstDIDNumTmp->ulBindType = pstDIDNumNode->ulBindType;
        pstDIDNumTmp->ulBindID  = pstDIDNumNode->ulBindID;
        dos_strncpy(pstDIDNumTmp->szDIDNum, pstDIDNumNode->szDIDNum, sizeof(pstDIDNumTmp->szDIDNum));
        pstDIDNumTmp->szDIDNum[sizeof(pstDIDNumTmp->szDIDNum) - 1] = '\0';

        dos_dmem_free(pstDIDNumNode);
        pstDIDNumNode = NULL;
    }
    pthread_mutex_unlock(&g_mutexHashDIDNum);

    return DOS_SUCC;
}

/**
 * ����: U32 sc_load_did_number()
 * ����: ����DID��������
 * ����:
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 */
U32 sc_load_did_number(U32 ulIndex)
{
    S8 szSQL[1024] = { 0, };

    if (SC_INVALID_INDEX == ulIndex)
    {
        dos_snprintf(szSQL, sizeof(szSQL), "SELECT id, customer_id, did_number, bind_type, bind_id FROM tbl_sipassign where tbl_sipassign.status = 1;");
    }
    else
    {
        dos_snprintf(szSQL, sizeof(szSQL), "SELECT id, customer_id, did_number, bind_type, bind_id FROM tbl_sipassign where tbl_sipassign.status = 1 AND id=%u;", ulIndex);
    }

    db_query(g_pstSCDBHandle, szSQL, sc_load_did_number_cb, NULL, NULL);

    return DOS_SUCC;
}

/**
 * ����: S32 sc_load_gateway_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
 * ����: ���������б�����ʱ���ݿ��ѯ�Ļص������������ݼ������ؼ������·�ɵ��б���
 * ����:
 *      VOID *pArg: ����
 *      S32 lCount: ������
 *      S8 **aszValues: ֵ�ѱ�
 *      S8 **aszNames: �ֶ����б�
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 */
S32 sc_load_gateway_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    SC_GW_NODE_ST        *pstGWNode     = NULL;
    HASH_NODE_S          *pstHashNode   = NULL;
    S8 *pszGWID    = NULL;
    S8 *pszDomain  = NULL;
    U32 ulID;
    U32 ulHashIndex = 0;

    if (DOS_ADDR_INVALID(aszNames)
        || DOS_ADDR_INVALID(aszValues))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pszGWID = aszValues[0];
    pszDomain = aszValues[1];
    if (DOS_ADDR_INVALID(pszGWID)
        || DOS_ADDR_INVALID(pszDomain)
        || dos_atoul(pszGWID, &ulID) < 0)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pthread_mutex_lock(&g_mutexHashGW);
    ulHashIndex = sc_ep_gw_hash_func(ulID);
    pstHashNode = hash_find_node(g_pstHashGW, ulHashIndex, (VOID *)&ulID, sc_ep_gw_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        pstHashNode = dos_dmem_alloc(sizeof(HASH_NODE_S));
        if (DOS_ADDR_INVALID(pstHashNode))
        {
            DOS_ASSERT(0);

            pthread_mutex_unlock(&g_mutexHashGW);
            return DOS_FAIL;
        }

        pstGWNode = dos_dmem_alloc(sizeof(SC_GW_NODE_ST));
        if (DOS_ADDR_INVALID(pstGWNode))
        {
            DOS_ASSERT(0);

            dos_dmem_free(pstHashNode);

            pthread_mutex_unlock(&g_mutexHashGW);
            return DOS_FAIL;
        }

        sc_ep_gw_init(pstGWNode);

        pstGWNode->ulGWID = ulID;
        if ('\0' == pszDomain[0])
        {
            dos_strncpy(pstGWNode->szGWDomain, pszDomain, sizeof(pstGWNode->szGWDomain));
            pstGWNode->szGWDomain[sizeof(pstGWNode->szGWDomain) - 1] = '\0';
        }
        else
        {
            pstGWNode->szGWDomain[0] = '\0';
        }

        HASH_Init_Node(pstHashNode);
        pstHashNode->pHandle = pstGWNode;

        ulHashIndex = sc_ep_gw_hash_func(pstGWNode->ulGWID);
        hash_add_node(g_pstHashGW, pstHashNode, ulHashIndex, NULL);
    }
    else
    {
        pstGWNode = pstHashNode->pHandle;
        if (DOS_ADDR_INVALID(pstGWNode))
        {
            DOS_ASSERT(0);

            pthread_mutex_unlock(&g_mutexHashGW);
            return DOS_FAIL;
        }

        dos_strncpy(pstGWNode->szGWDomain, pszDomain, sizeof(pstGWNode->szGWDomain));
        pstGWNode->szGWDomain[sizeof(pstGWNode->szGWDomain) - 1] = '\0';
    }
    pthread_mutex_unlock(&g_mutexHashGW);

    return DOS_SUCC;
}

/**
 * ����: U32 sc_load_did_number()
 * ����: ������������
 * ����:
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 */
U32 sc_load_gateway(U32 ulIndex)
{
    S8 szSQL[1024];

    if (SC_INVALID_INDEX == ulIndex)
    {
        dos_snprintf(szSQL, sizeof(szSQL)
                        , "SELECT id, realm FROM tbl_relaygrp WHERE tbl_relaygrp.status = 1;");
    }
    else
    {
        dos_snprintf(szSQL, sizeof(szSQL)
                        , "SELECT id, realm FROM tbl_relaygrp WHERE tbl_relaygrp.status = 1 AND id=%d;", ulIndex);
    }

    db_query(g_pstSCDBHandle, szSQL, sc_load_gateway_cb, NULL, NULL);

    return DOS_SUCC;
}

/**
 * ����: S32 sc_load_gateway_grp_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
 * ����: �����������б�����ʱ���ݿ��ѯ�Ļص�����
 * ����:
 *      VOID *pArg: ����
 *      S32 lCount: ������
 *      S8 **aszValues: ֵ�ѱ�
 *      S8 **aszNames: �ֶ����б�
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 **/
S32 sc_load_gateway_grp_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    SC_GW_GRP_NODE_ST    *pstGWGrpNode  = NULL;
    HASH_NODE_S          *pstHashNode   = NULL;
    S8 *pszGWGrpID = NULL;
    U32 ulID = 0;
    U32 ulHashIndex = 0;

    if (DOS_ADDR_INVALID(aszNames)
        || DOS_ADDR_INVALID(aszValues))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pszGWGrpID = aszValues[0];
    if (DOS_ADDR_INVALID(pszGWGrpID)
        || dos_atoul(pszGWGrpID, &ulID) < 0)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstHashNode = dos_dmem_alloc(sizeof(HASH_NODE_S));
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstGWGrpNode = dos_dmem_alloc(sizeof(SC_GW_GRP_NODE_ST));
    if (DOS_ADDR_INVALID(pstGWGrpNode))
    {
        DOS_ASSERT(0);

        dos_dmem_free(pstHashNode);
        return DOS_FAIL;
    }

    pstGWGrpNode->ulGWGrpID = ulID;

    HASH_Init_Node(pstHashNode);
    pstHashNode->pHandle = pstGWGrpNode;
    DLL_Init(&pstGWGrpNode->stGWList);
    pthread_mutex_init(&pstGWGrpNode->mutexGWList, NULL);

    ulHashIndex = sc_ep_gw_grp_hash_func(pstGWGrpNode->ulGWGrpID);

    pthread_mutex_lock(&g_mutexHashGWGrp);
    hash_add_node(g_pstHashGWGrp, pstHashNode, ulHashIndex, NULL);
    pthread_mutex_unlock(&g_mutexHashGWGrp);

    return DOS_SUCC;
}

/**
 * ����: U32 sc_load_did_number()
 * ����: ��������������
 * ����:
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 */
U32 sc_load_gateway_grp(U32 ulIndex)
{
    S8 szSQL[1024];

    if (SC_INVALID_INDEX == ulIndex)
    {
        dos_snprintf(szSQL, sizeof(szSQL)
                        , "SELECT id FROM tbl_gateway_grp WHERE tbl_gateway_grp.status = 1;");
    }
    else
    {
        dos_snprintf(szSQL, sizeof(szSQL)
                        , "SELECT id FROM tbl_gateway_grp WHERE tbl_gateway_grp.status = 1 AND id = %d;"
                        , ulIndex);
    }

    db_query(g_pstSCDBHandle, szSQL, sc_load_gateway_grp_cb, NULL, NULL);

    return DOS_SUCC;
}

/**
 * ����: S32 sc_load_relationship_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
 * ����: ����·���������ϵ����
 * ����:
 *      VOID *pArg: ����
 *      S32 lCount: ������
 *      S8 **aszValues: ֵ�ѱ�
 *      S8 **aszNames: �ֶ����б�
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 */
S32 sc_load_relationship_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    SC_GW_GRP_NODE_ST    *pstGWGrp      = NULL;
    SC_GW_NODE_ST        *pstGWNode     = NULL;
    DLL_NODE_S           *pstListNode   = NULL;
    HASH_NODE_S          *pstHashNode   = NULL;
    U32                  ulGatewayID    = U32_BUTT;
    U32                  ulHashIndex    = U32_BUTT;
    S32                  lIndex         = 0;
    BOOL                 blProcessOK    = DOS_TRUE;

    if (DOS_ADDR_INVALID(pArg)
        || lCount <= 0
        || DOS_ADDR_INVALID(aszValues)
        || DOS_ADDR_INVALID(aszNames))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    for (lIndex=0; lIndex<lCount; lIndex++)
    {
        if (DOS_ADDR_INVALID(aszNames[lIndex])
            || DOS_ADDR_INVALID(aszValues[lIndex]))
        {
            break;
        }

        if (0 == dos_strncmp(aszNames[lIndex], "gateway_id", dos_strlen("gateway_id")))
        {
            if (dos_atoul(aszValues[lIndex], &ulGatewayID) < 0)
            {
                blProcessOK    = DOS_FALSE;
                break;
            }
        }
    }

    if (!blProcessOK
        || U32_BUTT == ulGatewayID)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    ulHashIndex = sc_ep_gw_hash_func(ulGatewayID);
    pstHashNode = hash_find_node(g_pstHashGW, ulHashIndex, &ulGatewayID, sc_ep_gw_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstListNode = dos_dmem_alloc(sizeof(DLL_NODE_S));
    if (DOS_ADDR_INVALID(pstListNode))
    {
        DOS_ASSERT(0);

        dos_dmem_free(pstGWNode);
        pstGWNode = NULL;
        return DOS_FAIL;
    }
    DLL_Init_Node(pstListNode);
    pstListNode->pHandle = pstHashNode->pHandle;

    pstGWGrp = (SC_GW_GRP_NODE_ST *)pArg;

    pthread_mutex_lock(&pstGWGrp->mutexGWList);
    DLL_Add(&pstGWGrp->stGWList, pstListNode);
    pthread_mutex_unlock(&pstGWGrp->mutexGWList);

    return DOS_FAIL;
}


/**
 * ����: U32 sc_load_did_number()
 * ����: ����·���������ϵ����
 * ����:
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 */
U32 sc_load_relationship()
{
    SC_GW_GRP_NODE_ST    *pstGWGrp      = NULL;
    HASH_NODE_S          *pstHashNode   = NULL;
    U32                  ulHashIndex = 0;
    S8 szSQL[1024] = { 0, };

    HASH_Scan_Table(g_pstHashGWGrp, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashGWGrp, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            pstGWGrp = pstHashNode->pHandle;
            if (DOS_ADDR_INVALID(pstGWGrp))
            {
                DOS_ASSERT(0);
                continue;
            }

            dos_snprintf(szSQL, sizeof(szSQL), "SELECT gateway_id FROM tbl_gateway_assign WHERE route_grp_id=%d;", pstGWGrp->ulGWGrpID);

            db_query(g_pstSCDBHandle, szSQL, sc_load_relationship_cb, (VOID *)pstGWGrp, NULL);
        }
    }

    return DOS_SUCC;
}

/**
 * ����: S32 sc_load_route_group_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
 * ����: ����·������ʱ���ݿ��ѯ�Ļص������������ݼ���·���б���
 * ����:
 *      VOID *pArg: ����
 *      S32 lCount: ������
 *      S8 **aszValues: ֵ�ѱ�
 *      S8 **aszNames: �ֶ����б�
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 */
S32 sc_load_route_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    SC_ROUTE_NODE_ST     *pstRoute      = NULL;
    SC_ROUTE_NODE_ST     *pstRouteTmp   = NULL;
    DLL_NODE_S           *pstListNode   = NULL;
    S32                  lIndex;
    S32                  lSecond;
    S32                  lRet;
    BOOL                 blProcessOK = 0;

    if (DOS_ADDR_INVALID(aszValues)
        || DOS_ADDR_INVALID(aszNames))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstRoute = dos_dmem_alloc(sizeof(SC_ROUTE_NODE_ST));
    if (DOS_ADDR_INVALID(pstRoute))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_ep_route_init(pstRoute);


    for (blProcessOK=DOS_TRUE, lIndex=0; lIndex<lCount; lIndex++)
    {
        if (0 == dos_strnicmp(aszNames[lIndex], "id", dos_strlen("id")))
        {
            if (dos_atoul(aszValues[lIndex], &pstRoute->ulID) < 0)
            {
                DOS_ASSERT(0);

                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "start_time", dos_strlen("start_time")))
        {
            lRet = dos_sscanf(aszValues[lIndex]
                        , "%d:%d:%d"
                        , &pstRoute->ucHourBegin
                        , &pstRoute->ucMinuteBegin
                        , &lSecond);
            if (3 != lRet)
            {
                DOS_ASSERT(0);

                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "end_time", dos_strlen("end_time")))
        {
            lRet = dos_sscanf(aszValues[lIndex]
                    , "%d:%d:%d"
                    , &pstRoute->ucHourEnd
                    , &pstRoute->ucMinuteEnd
                    , &lSecond);
            if (3 != lRet)
            {
                DOS_ASSERT(0);

                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "callee_prefix", dos_strlen("callee_prefix")))
        {
            if (aszValues[lIndex] && '\0' != aszValues[lIndex][0])
            {
                dos_strncpy(pstRoute->szCalleePrefix, aszValues[lIndex], sizeof(pstRoute->szCalleePrefix));
                pstRoute->szCalleePrefix[sizeof(pstRoute->szCalleePrefix) - 1] = '\0';
            }
            else
            {
                pstRoute->szCalleePrefix[0] = '\0';
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "caller_prefix", dos_strlen("caller_prefix")))
        {
            if (aszValues[lIndex] && '\0' != aszValues[lIndex][0])
            {
                dos_strncpy(pstRoute->szCallerPrefix, aszValues[lIndex], sizeof(pstRoute->szCallerPrefix));
                pstRoute->szCallerPrefix[sizeof(pstRoute->szCallerPrefix) - 1] = '\0';
            }
            else
            {
                pstRoute->szCallerPrefix[0] = '\0';
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "dest_type", dos_strlen("dest_type")))
        {
            if (DOS_ADDR_INVALID(aszValues[lIndex])
                || '\0' == aszValues[lIndex][0]
                || dos_atoul(aszValues[lIndex], &pstRoute->ulDestType) < 0)
            {
                DOS_ASSERT(0);

                blProcessOK = DOS_FALSE;
                break;
            }
        }
        else if (0 == dos_strnicmp(aszNames[lIndex], "dest_id", dos_strlen("dest_id")))
        {
            if (DOS_ADDR_INVALID(aszValues[lIndex])
                || '\0' == aszValues[lIndex][0]
                || dos_atoul(aszValues[lIndex], &pstRoute->ulDestID) < 0)
            {
                DOS_ASSERT(0);

                blProcessOK = DOS_FALSE;
                break;
            }
        }
    }

    if (!blProcessOK)
    {
        DOS_ASSERT(0);
        dos_dmem_free(pstRoute);
        return DOS_FAIL;
    }

    pthread_mutex_lock(&g_mutexRouteList);
    pstListNode = dll_find(&g_stRouteList, &pstRoute->ulID, sc_ep_route_find);
    if (DOS_ADDR_INVALID(pstListNode))
    {
        pstListNode = (DLL_NODE_S *)dos_dmem_alloc(sizeof(DLL_NODE_S));
        if (DOS_ADDR_INVALID(pstListNode))
        {
            DOS_ASSERT(0);

            dos_dmem_free(pstRoute);
            pstRoute = NULL;

            pthread_mutex_unlock(&g_mutexRouteList);
            return DOS_FAIL;
        }

        DLL_Init_Node(pstListNode);
        pstListNode->pHandle = pstRoute;
        DLL_Add(&g_stRouteList, pstListNode);
    }
    else
    {
        pstRouteTmp = pstListNode->pHandle;
        if (DOS_ADDR_INVALID(pstRouteTmp))
        {
            DOS_ASSERT(0);

            dos_dmem_free(pstRoute);
            pstRoute = NULL;

            pthread_mutex_unlock(&g_mutexRouteList);
            return DOS_FAIL;
        }

        dos_memcpy(pstRouteTmp, pstListNode, sizeof(SC_ROUTE_NODE_ST));

        dos_dmem_free(pstRoute);
        pstRoute = NULL;
    }
    pthread_mutex_unlock(&g_mutexRouteList);

    return DOS_TRUE;
}


/**
 * ����: U32 sc_load_route()
 * ����: ����·������
 * ����:
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 */
U32 sc_load_route(U32 ulIndex)
{
    S8 szSQL[1024];

    if (SC_INVALID_INDEX == ulIndex)
    {
        dos_snprintf(szSQL, sizeof(szSQL)
                    , "SELECT id, start_time, end_time, callee_prefix, caller_prefix, dest_type, dest_id FROM tbl_route WHERE tbl_route.status = 1;");
    }
    else
    {
        dos_snprintf(szSQL, sizeof(szSQL)
                    , "SELECT id, start_time, end_time, callee_prefix, caller_prefix, dest_type, dest_id FROM tbl_route WHERE tbl_route.status = 1 AND id=%d;"
                    , ulIndex);
    }

    db_query(g_pstSCDBHandle, szSQL, sc_load_route_cb, NULL, NULL);

    return DOS_SUCC;
}

/**
 * ����: U32 sc_ep_esl_execute(const S8 *pszApp, const S8 *pszArg, const S8 *pszUUID)
 * ����: ʹ��pstHandle��ָ���ESL���ָ������pszApp������ΪpszArg������ΪpszUUID
 * ����:
 *      esl_handle_t *pstHandle: ESL���
 *      const S8 *pszApp: ִ�е�����
 *      const S8 *pszArg: �������
 *      const S8 *pszUUID: channel��UUID
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 *
 * ע��: ���ú�����ִ������ʱ��������ֵ�ǰ����Ѿ�ʧȥ���ӣ�������������ESL������
 */
U32 sc_ep_esl_execute(const S8 *pszApp, const S8 *pszArg, const S8 *pszUUID)
{
    U32 ulRet;

    if (DOS_ADDR_INVALID(pszApp)
        || DOS_ADDR_INVALID(pszUUID))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if (!g_pstHandle->stSendHandle.connected)
    {
        ulRet = esl_connect(&g_pstHandle->stSendHandle, "127.0.0.1", 8021, NULL, "ClueCon");
        if (ESL_SUCCESS != ulRet)
        {
            esl_disconnect(&g_pstHandle->stSendHandle);
            sc_logr_notice(SC_ESL, "ELS for send event re-connect fail, return code:%d, Msg:%s. Will be retry after 1 second.", ulRet, g_pstHandle->stSendHandle.err);

            DOS_ASSERT(0);

            return DOS_FAIL;
        }

        g_pstHandle->stSendHandle.event_lock = 1;
    }

    if (ESL_SUCCESS != esl_execute(&g_pstHandle->stSendHandle, pszApp, pszArg, pszUUID))
    {
        DOS_ASSERT(0);
        sc_logr_notice(SC_ESL, "ESL execute command fail. Result:%d, APP: %s, ARG : %s, UUID: %s"
                        , ulRet
                        , pszApp
                        , DOS_ADDR_VALID(pszArg) ? pszArg : "NULL"
                        , DOS_ADDR_VALID(pszUUID) ? pszUUID : "NULL");

        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/**
 * ����: U32 sc_ep_esl_execute_cmd(const S8 *pszCmd)
 * ����: ʹ��pstHandle��ָ���ESL���ִ������
 * ����:
 *      const S8 *pszCmd:
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 *
 * ע��: ���ú�����ִ������ʱ��������ֵ�ǰ����Ѿ�ʧȥ���ӣ�������������ESL������
 */
U32 sc_ep_esl_execute_cmd(const S8 *pszCmd)
{
    U32 ulRet;

    if (DOS_ADDR_INVALID(pszCmd))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if (!g_pstHandle->stSendHandle.connected)
    {
        ulRet = esl_connect(&g_pstHandle->stSendHandle, "127.0.0.1", 8021, NULL, "ClueCon");
        if (ESL_SUCCESS != ulRet)
        {
            esl_disconnect(&g_pstHandle->stSendHandle);
            sc_logr_notice(SC_ESL, "ELS for send event re-connect fail, return code:%d, Msg:%s. Will be retry after 1 second.", ulRet, g_pstHandle->stSendHandle.err);

            DOS_ASSERT(0);

            return DOS_FAIL;
        }

        g_pstHandle->stSendHandle.event_lock = 1;
    }

    if (ESL_SUCCESS != esl_send(&g_pstHandle->stSendHandle, pszCmd))
    {
        DOS_ASSERT(0);
        sc_logr_notice(SC_ESL, "ESL execute command fail. Result:%d, CMD: %s"
                        , ulRet
                        , pszCmd);

        return DOS_FAIL;
    }

    sc_logr_notice(SC_ESL, "ESL execute command SUCC. CMD: %s", pszCmd);

    return DOS_SUCC;
}


/**
 * ����: U32 sc_ep_parse_event(esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
 * ����: ��ESL�¼�pstEvent�л�ȡ���������洢��pstSCB
 * ����:
 *          esl_event_t *pstEvent : ����Դ ESL�¼�
 *          SC_SCB_ST *pstSCB     : SCB���洢����
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 */
U32 sc_ep_parse_event(esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
{
    S8         *pszCaller    = NULL;
    S8         *pszCallee    = NULL;
    S8         *pszANI       = NULL;
    S8         *pszCallSrc   = NULL;
    S8         *pszTrunkIP   = NULL;
    S8         *pszGwName    = NULL;
    S8         *pszCallDirection = NULL;
    S8         *pszOtherLegUUID  = NULL;
    SC_SCB_ST  *pstSCB2 = NULL;

    SC_TRACE_IN(pstEvent, pstSCB, 0, 0);

    if (DOS_ADDR_INVALID(pstEvent) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_SUCC;
    }

    /* ��ESL EVENT�л�ȥ��غ�����Ϣ */
    /*
     * 1. PSTN����
     *   ����:
     *       Call Direction: Inbound;
     *       Profile Name:   external
     *   ��ȡ����Ϣ:
     *       �Զ�IP����gateway name��ȡ���е�����ID
     *       ��������Ϣ, ��������Ϣ
     * 2. ������PSTN
     *   ����:
     *       Call Direction: outbount;
     *       Profile Name:   external;
     *   ��ȡ����Ϣ:
     *       �Զ�IP����gateway name��ȡ���е�����ID
     *       ��������Ϣ, ��������Ϣ
     *       ��ȡ�û���Ϣ��ʾ
     * 3. �ڲ�����
     *   ����:
     *       Call Direction: Inbound;
     *       Profile Name:   internal;
     *   ��ȡ����Ϣ:
     *       ��������Ϣ, ��������Ϣ
     *       ��ȡ�û���Ϣ��ʾ
     */

    pszCallSrc = esl_event_get_header(pstEvent, "variable_sofia_profile_name");
    if (DOS_ADDR_INVALID(pszCallSrc))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pszCallDirection = esl_event_get_header(pstEvent, "Call-Direction");
    if (DOS_ADDR_INVALID(pszCallDirection))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pszGwName = esl_event_get_header(pstEvent, "variable_sip_gateway_name");
    pszTrunkIP = esl_event_get_header(pstEvent, "Caller-Network-Addr");
    pszCaller = esl_event_get_header(pstEvent, "Caller-Caller-ID-Number");
    pszCallee = esl_event_get_header(pstEvent, "Caller-Destination-Number");
    pszANI    = esl_event_get_header(pstEvent, "Caller-ANI");
    pszOtherLegUUID = esl_event_get_header(pstEvent, "Other-Leg-Unique-ID");
    if (DOS_ADDR_INVALID(pszCaller) || DOS_ADDR_INVALID(pszCallee) || DOS_ADDR_INVALID(pszANI))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    if (DOS_ADDR_VALID(pszOtherLegUUID))
    {
        pstSCB2 = sc_scb_hash_tables_find(pszOtherLegUUID);
    }

    /* ���������д��SCB�� */
    pthread_mutex_lock(&pstSCB->mutexSCBLock);
    if (DOS_ADDR_VALID(pstSCB2))
    {
        pstSCB->usOtherSCBNo = pstSCB2->usSCBNo;
        pstSCB2->usOtherSCBNo = pstSCB->usSCBNo;
    }
    dos_strncpy(pstSCB->szCalleeNum, pszCallee, sizeof(pstSCB->szCalleeNum));
    pstSCB->szCalleeNum[sizeof(pstSCB->szCalleeNum) -1] = '\0';
    dos_strncpy(pstSCB->szCallerNum, pszCaller, sizeof(pstSCB->szCallerNum));
    pstSCB->szCallerNum[sizeof(pstSCB->szCallerNum) -1] = '\0';
    dos_strncpy(pstSCB->szANINum, pszANI, sizeof(pstSCB->szANINum));
    pstSCB->szANINum[sizeof(pstSCB->szANINum) -1] = '\0';
    pthread_mutex_unlock(&pstSCB->mutexSCBLock);

    SC_TRACE_OUT();
    return DOS_SUCC;
}

U32 sc_rp_parse_extra_data(esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
{
    S8 *pszTmp = NULL;
    U32 ulTmp  = 0;

    if (DOS_ADDR_INVALID(pstEvent)
        || DOS_ADDR_INVALID(pstSCB)
        || DOS_ADDR_INVALID(pstSCB->pstExtraData))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pszTmp = esl_event_get_header(pstEvent, "Caller-Channel-Created-Time");
    if (DOS_ADDR_VALID(pszTmp)
        && dos_atoul(pszTmp, &ulTmp) == 0)
    {
        pstSCB->pstExtraData->ulStartTimeStamp = ulTmp;
    }

    pszTmp = esl_event_get_header(pstEvent, "Caller-Channel-Answered-Time");
    if (DOS_ADDR_VALID(pszTmp)
        && dos_atoul(pszTmp, &ulTmp) == 0)
    {
        pstSCB->pstExtraData->ulAnswerTimeStamp = ulTmp;
    }

    pszTmp = esl_event_get_header(pstEvent, "Caller-Channel-Progress-Time");
    if (DOS_ADDR_VALID(pszTmp)
        && dos_atoul(pszTmp, &ulTmp) == 0)
    {
        pstSCB->pstExtraData->ulRingTimeStamp = ulTmp;
    }

    pszTmp = esl_event_get_header(pstEvent, "Caller-Channel-Progress-Media-Time");
    if (DOS_ADDR_VALID(pszTmp)
        && dos_atoul(pszTmp, &ulTmp) == 0)
    {
        pstSCB->pstExtraData->ulBridgeTimeStamp= ulTmp;
    }

    pszTmp = esl_event_get_header(pstEvent, "Caller-Channel-Hangup-Time");
    if (DOS_ADDR_VALID(pszTmp)
        && dos_atoul(pszTmp, &ulTmp) == 0)
    {
        pstSCB->pstExtraData->ulByeTimeStamp = ulTmp;
    }


    return DOS_SUCC;
}

U32 sc_ep_terminate_call(SC_SCB_ST *pstSCB)
{
    SC_SCB_ST *pstSCBOther = NULL;

    if (DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstSCBOther = sc_scb_get(pstSCB->usSCBNo);
    if (DOS_ADDR_VALID(pstSCBOther))
    {
        if ('\0' != pstSCBOther->szUUID[0])
        {
            sc_ep_esl_execute("hangup", NULL, pstSCB->szUUID);
            sc_logr_notice(SC_ESL, "Hangup Call for Auth FAIL. SCB No : %d, UUID: %d.", pstSCBOther->usSCBNo, pstSCBOther->szUUID);
        }
        else
        {
            SC_SCB_SET_STATUS(pstSCBOther, SC_SCB_RELEASE);
            sc_call_trace(pstSCBOther, "Terminate call.");
            sc_logr_notice(SC_ESL, "Call terminate call. SCB No : %d.", pstSCBOther->usSCBNo);
            sc_scb_free(pstSCBOther);
        }
    }

    if ('\0' != pstSCB->szUUID[0])
    {
        /* ��FSͨѶ�Ļ�����ֱ�ӹҶϺ��оͺ� */
        sc_ep_esl_execute("hangup", NULL, pstSCB->szUUID);
        sc_logr_notice(SC_ESL, "Hangup Call for Auth FAIL. SCB No : %d, UUID: %d. *", pstSCB->usSCBNo, pstSCB->szUUID);
    }
    else
    {
        SC_SCB_SET_STATUS(pstSCB, SC_SCB_RELEASE);
        sc_call_trace(pstSCB, "Terminate call.");
        sc_logr_notice(SC_ESL, "Call terminate call. SCB No : %d. *", pstSCB->usSCBNo);
        sc_scb_free(pstSCB);
    }

    return DOS_SUCC;
}


/**
 * ����: U32 sc_ep_internal_service_check(esl_event_t *pstEvent)
 * ����: ��鵱ǰ�¼��Ƿ�����ִ���ڲ�ҵ��
 * ����:
 *          esl_event_t *pstEvent : ����Դ ESL�¼�
 * ����ֵ: �ɹ������ڲ�ҵ��ö��ֵ�����򷵻���Чҵ��ö��
 */
U32 sc_ep_internal_service_check(esl_event_t *pstEvent)
{
    return SC_INTER_SRV_BUTT;
}

/**
 * ����: BOOL sc_ep_check_extension(S8 *pszNum, U32 ulCustomerID)
 * ����: ���pszNum��ִ�еķֻ��ţ��Ƿ�������ΪulCustomerID�Ŀͻ�
 * ����:
 *      S8 *pszNum: �ֻ���
 *      U32 ulCustomerID: �ͻ�ID
 * ����ֵ: �ɹ�����DOS_TRUE�����򷵻�DOS_FALSE
 */
BOOL sc_ep_check_extension(S8 *pszNum, U32 ulCustomerID)
{
    SC_USER_ID_NODE_ST *pstUserIDNode = NULL;
    HASH_NODE_S        *pstHashNode   = NULL;
    U32                ulHashIndex    = 0;

    if (DOS_ADDR_INVALID(pszNum))
    {
        DOS_ASSERT(0);

        return DOS_FALSE;
    }

    pthread_mutex_lock(&g_mutexHashSIPUserID);
    HASH_Scan_Table(g_pstHashSIPUserID, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashSIPUserID, ulHashIndex, pstHashNode, HASH_NODE_S*)
        {
            if (DOS_ADDR_INVALID(pstHashNode))
            {
                continue;
            }

            pstUserIDNode = pstHashNode->pHandle;
            if (DOS_ADDR_INVALID(pstUserIDNode))
            {
                continue;
            }

            if (ulCustomerID == pstUserIDNode->ulCustomID
                && 0 == dos_strnicmp(pstUserIDNode->szExtension, pszNum, sizeof(pstUserIDNode->szExtension)))
            {
                pthread_mutex_unlock(&g_mutexHashSIPUserID);
                return DOS_TRUE;
            }
        }
    }
    pthread_mutex_unlock(&g_mutexHashSIPUserID);

    return DOS_FALSE;
}

/**
 * ����: U32 sc_ep_get_extension_by_userid(S8 *pszUserID, S8 *pszExtension, U32 ulLength)
 * ����: ��ȡUserID pszUserID��Ӧ�ķֻ��ţ���copy������pszExtension�У�ʹ��ulLengthָ������ĳ���
 * ����:
 *      S8 *pszUserID    : User ID
 *      S8 *pszExtension : �洢�ֻ��ŵĻ���
 *      U32 ulLength     : ���泤��
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 */
U32 sc_ep_get_extension_by_userid(S8 *pszUserID, S8 *pszExtension, U32 ulLength)
{
    SC_USER_ID_NODE_ST *pstUserIDNode = NULL;
    HASH_NODE_S        *pstHashNode   = NULL;
    U32                ulHashIndex    = 0;

    if (DOS_ADDR_INVALID(pszUserID)
        || DOS_ADDR_INVALID(pszExtension)
        || ulLength <= 0)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pthread_mutex_lock(&g_mutexHashSIPUserID);
    ulHashIndex = sc_sip_userid_hash_func(pszUserID);
    pstHashNode = hash_find_node(g_pstHashSIPUserID, ulHashIndex, (VOID *)pszUserID, sc_ep_sip_userid_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        pthread_mutex_unlock(&g_mutexHashSIPUserID);
        return DOS_FAIL;
    }

    pstUserIDNode = pstHashNode->pHandle;

    dos_strncpy(pszExtension, pstUserIDNode->szExtension, ulLength);
    pszExtension[ulLength - 1] = '\0';

    pthread_mutex_unlock(&g_mutexHashSIPUserID);

    return DOS_SUCC;
}

/**
 * ����: U32 sc_ep_get_custom_by_sip_userid(S8 *pszNum)
 * ����: ��ȡpszNum��ָ��UserID�����Ŀͻ���ID
 * ����:
 *      S8 *pszNum    : User ID
 * ����ֵ: �ɹ����ؿͻ�IDֵ�����򷵻�U32_BUTT
 */
U32 sc_ep_get_custom_by_sip_userid(S8 *pszNum)
{
    SC_USER_ID_NODE_ST *pstUserIDNode = NULL;
    HASH_NODE_S        *pstHashNode   = NULL;
    U32                ulHashIndex    = 0;
    U32                ulCustomerID   = 0;

    if (DOS_ADDR_INVALID(pszNum))
    {
        DOS_ASSERT(0);

        return U32_BUTT;
    }

    pthread_mutex_lock(&g_mutexHashSIPUserID);
    ulHashIndex = sc_sip_userid_hash_func(pszNum);
    pstHashNode = hash_find_node(g_pstHashSIPUserID, ulHashIndex, (VOID *)pszNum, sc_ep_sip_userid_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        pthread_mutex_unlock(&g_mutexHashSIPUserID);
        return U32_BUTT;
    }

    pstUserIDNode = pstHashNode->pHandle;

    ulCustomerID = pstUserIDNode->ulCustomID;
    pthread_mutex_unlock(&g_mutexHashSIPUserID);

    return ulCustomerID;
}

/**
 * ����: U32 sc_ep_get_custom_by_did(S8 *pszNum)
 * ����: ͨ��pszNum��ָ����DID���룬�ҵ���ǰDID���������Ǹ��ͻ�
 * ����:
 *      S8 *pszNum : DID����
 * ����ֵ: �ɹ����ؿͻ�ID�����򷵻�U32_BUTT
 */
U32 sc_ep_get_custom_by_did(S8 *pszNum)
{
    SC_DID_NODE_ST     *pstDIDNumNode = NULL;
    HASH_NODE_S        *pstHashNode   = NULL;
    U32                ulHashIndex    = 0;
    U32                ulCustomerID   = 0;

    if (DOS_ADDR_INVALID(pszNum))
    {
        DOS_ASSERT(0);

        return U32_BUTT;
    }

    ulHashIndex = sc_sip_did_hash_func(pszNum);
    pthread_mutex_lock(&g_mutexHashDIDNum);
    pstHashNode = hash_find_node(g_pstHashDIDNum, ulHashIndex, (VOID *)pszNum, sc_ep_did_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        pthread_mutex_unlock(&g_mutexHashDIDNum);
        return U32_BUTT;
    }

    pstDIDNumNode = pstHashNode->pHandle;

    ulCustomerID = pstDIDNumNode->ulCustomID;

    pthread_mutex_unlock(&g_mutexHashDIDNum);

    return ulCustomerID;
}


/**
 * ����: U32 sc_ep_get_bind_info4did(S8 *pszDidNum, U32 *pulBindType, U32 *pulBindID)
 * ����: ��ȡpszDidNum��ִ�е�DID����İ���Ϣ
 * ����:
 *      S8 *pszDidNum    : DID����
 *      U32 *pulBindType : ��ǰDID����󶨵�����
 *      U32 *pulBindID   : ��ǰDID����󶨵�ID
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 */
U32 sc_ep_get_bind_info4did(S8 *pszDidNum, U32 *pulBindType, U32 *pulBindID)
{
    SC_DID_NODE_ST     *pstDIDNumNode = NULL;
    HASH_NODE_S        *pstHashNode   = NULL;
    U32                ulHashIndex    = 0;

    if (DOS_ADDR_INVALID(pszDidNum)
        || DOS_ADDR_INVALID(pulBindType)
        || DOS_ADDR_INVALID(pulBindID))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    ulHashIndex = sc_sip_did_hash_func(pszDidNum);
    pthread_mutex_lock(&g_mutexHashDIDNum);
    pstHashNode = hash_find_node(g_pstHashDIDNum, ulHashIndex, (VOID *)pszDidNum, sc_ep_did_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        pthread_mutex_unlock(&g_mutexHashDIDNum);
        return DOS_FAIL;
    }

    pstDIDNumNode = pstHashNode->pHandle;

    *pulBindType = pstDIDNumNode->ulBindType;
    *pulBindID = pstDIDNumNode->ulBindID;

    pthread_mutex_unlock(&g_mutexHashDIDNum);

    return DOS_SUCC;
}


/**
 * ����: U32 sc_ep_get_userid_by_id(U32 ulSIPUserID, S8 *pszUserID, U32 ulLength)
 * ����: ��ȡIDΪulSIPUserID SIP User ID������SIP USER ID Copy������pszUserID��
 * ����:
 *      U32 ulSIPUserID : SIP User ID������
 *      S8 *pszUserID   : �˻�ID����
 *      U32 ulLength    : ���泤��
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 */
U32 sc_ep_get_userid_by_id(U32 ulSIPUserID, S8 *pszUserID, U32 ulLength)
{
    SC_USER_ID_NODE_ST *pstUserIDNode = NULL;
    HASH_NODE_S        *pstHashNode   = NULL;
    U32                ulHashIndex    = 0;

    if (DOS_ADDR_INVALID(pszUserID)
        || ulLength <= 0)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pthread_mutex_lock(&g_mutexHashSIPUserID);
    HASH_Scan_Table(g_pstHashSIPUserID, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashSIPUserID, ulHashIndex, pstHashNode, HASH_NODE_S*)
        {
            if (DOS_ADDR_INVALID(pstHashNode))
            {
                continue;
            }

            pstUserIDNode = pstHashNode->pHandle;
            if (DOS_ADDR_INVALID(pstUserIDNode))
            {
                continue;
            }

            if (ulSIPUserID == pstUserIDNode->ulSIPID)
            {
                dos_strncpy(pszUserID, pstUserIDNode->szUserID, ulLength);
                pszUserID[ulLength - 1] = '\0';
                pthread_mutex_unlock(&g_mutexHashSIPUserID);
                return DOS_SUCC;
            }
        }
    }
    pthread_mutex_unlock(&g_mutexHashSIPUserID);
    return DOS_FAIL;

}

/**
 * ����: U32 sc_ep_get_userid_by_extension(U32 ulCustomID, S8 *pszExtension, S8 *pszUserID, U32 ulLength)
 * ����: ��ȡ�ͻ�IDΪulCustomID���ֻ���ΪpszExtension��User ID������User ID Copy������pszUserID��
 * ����:
 *      U32 ulCustomID  : �ͻ�ID
 *      S8 *pszExtension: �ֻ���
 *      S8 *pszUserID   : �˻�ID����
 *      U32 ulLength    : ���泤��
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 */
U32 sc_ep_get_userid_by_extension(U32 ulCustomID, S8 *pszExtension, S8 *pszUserID, U32 ulLength)
{
    SC_USER_ID_NODE_ST *pstUserIDNode = NULL;
    HASH_NODE_S        *pstHashNode   = NULL;
    U32                ulHashIndex    = 0;

    if (DOS_ADDR_INVALID(pszExtension)
        || DOS_ADDR_INVALID(pszUserID)
        || ulLength <= 0)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pthread_mutex_lock(&g_mutexHashSIPUserID);
    HASH_Scan_Table(g_pstHashSIPUserID, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashSIPUserID, ulHashIndex, pstHashNode, HASH_NODE_S*)
        {
            if (DOS_ADDR_INVALID(pstHashNode))
            {
                continue;
            }

            pstUserIDNode = pstHashNode->pHandle;
            if (DOS_ADDR_INVALID(pstUserIDNode))
            {
                continue;
            }

            if (ulCustomID != pstUserIDNode->ulCustomID)
            {
                continue;
            }

            if (0 != dos_strnicmp(pstUserIDNode->szExtension, pszExtension, sizeof(pstUserIDNode->szExtension)))
            {
                continue;
            }

            dos_strncpy(pszUserID, pstUserIDNode->szUserID, ulLength);
            pszUserID[ulLength - 1] = '\0';
            pthread_mutex_unlock(&g_mutexHashSIPUserID);
            return DOS_SUCC;
        }
    }
    pthread_mutex_unlock(&g_mutexHashSIPUserID);
    return DOS_FAIL;
}

/**
 * ����: U32 sc_ep_search_route(SC_SCB_ST *pstSCB)
 * ����: ��ȡ·����
 * ����:
 *      SC_SCB_ST *pstSCB : ���п��ƿ飬ʹ�������к���
 * ����ֵ: �ɹ�����·����ID�����򷵻�U32_BUTT
 */
U32 sc_ep_search_route(SC_SCB_ST *pstSCB)
{
    SC_ROUTE_NODE_ST     *pstRouetEntry = NULL;
    DLL_NODE_S           *pstListNode   = NULL;
    struct tm            *pstTime;
    time_t               timep;
    U32                  ulRouteGrpID;

    timep = time(NULL);
    pstTime = localtime(&timep);
    if (DOS_ADDR_INVALID(pstTime))
    {
        DOS_ASSERT(0);

        return U32_BUTT;
    }

    ulRouteGrpID = U32_BUTT;
    pthread_mutex_lock(&g_mutexRouteList);
    DLL_Scan(&g_stRouteList, pstListNode, DLL_NODE_S *)
    {
        pstRouetEntry = (SC_ROUTE_NODE_ST *)pstListNode->pHandle;
        if (DOS_ADDR_INVALID(pstRouetEntry))
        {
            continue;
        }

        sc_logr_debug(SC_ESL, "Search Route: %d:%d, %d:%d, %s, %s"
                , pstRouetEntry->ucHourBegin, pstRouetEntry->ucMinuteBegin
                , pstRouetEntry->ucHourEnd, pstRouetEntry->ucMinuteEnd
                , pstRouetEntry->szCalleePrefix
                , pstRouetEntry->szCallerPrefix);

        /* �ȿ���Сʱ�Ƿ�ƥ�� */
        if (pstTime->tm_hour < pstRouetEntry->ucHourBegin
            || pstTime->tm_hour > pstRouetEntry->ucHourEnd)
        {
            continue;
        }

        /* �жϷ��ӶԲ��� */
        if (pstTime->tm_min < pstRouetEntry->ucMinuteBegin
            || pstTime->tm_min > pstRouetEntry->ucMinuteEnd)
        {
            continue;
        }

        if ('\0' == pstRouetEntry->szCalleePrefix[0])
        {
            if ('\0' == pstRouetEntry->szCallerPrefix[0])
            {
                ulRouteGrpID = pstRouetEntry->ulID;
                break;
            }
            else
            {
                if (0 == dos_strnicmp(pstRouetEntry->szCalleePrefix, pstSCB->szCalleeNum, dos_strlen(pstRouetEntry->szCalleePrefix)))
                {
                    ulRouteGrpID = pstRouetEntry->ulID;
                    break;
                }
            }
        }
        else
        {
            if ('\0' == pstRouetEntry->szCallerPrefix[0])
            {
                if (0 == dos_strnicmp(pstRouetEntry->szCallerPrefix, pstSCB->szCallerNum, dos_strlen(pstRouetEntry->szCallerPrefix)))
                {
                    ulRouteGrpID = pstRouetEntry->ulID;
                    break;
                }
            }
            else
            {
                if (0 == dos_strnicmp(pstRouetEntry->szCalleePrefix, pstSCB->szCalleeNum, dos_strlen(pstRouetEntry->szCalleePrefix))
                    && 0 == dos_strnicmp(pstRouetEntry->szCallerPrefix, pstSCB->szCallerNum, dos_strlen(pstRouetEntry->szCallerPrefix)))
                {
                    ulRouteGrpID = pstRouetEntry->ulID;
                    break;
                }
            }
        }
    }

    sc_logr_debug(SC_ESL, "Search Route Finished. Result: %s, Route ID: %d"
            , U32_BUTT == ulRouteGrpID ? "FAIL" : "SUCC"
            , ulRouteGrpID);

    pthread_mutex_unlock(&g_mutexRouteList);

    return ulRouteGrpID;
}

/**
 * ����: U32 sc_ep_get_callee_string(U32 ulRouteGroupID, S8 *pszNum, S8 *szCalleeString, U32 ulLength)
 * ����: ͨ��·����ID���ͱ��к����ȡ���ֺ��еĺ����ַ�������������洢��szCalleeString��
 * ����:
 *      U32 ulRouteGroupID : ·����ID
 *      S8 *pszNum         : ���к���
 *      S8 *szCalleeString : �����ַ�������
 *      U32 ulLength       : ���泤��
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 */
U32 sc_ep_get_callee_string(U32 ulRouteID, S8 *pszNum, S8 *szCalleeString, U32 ulLength)
{
    SC_ROUTE_NODE_ST     *pstRouetEntry = NULL;
    DLL_NODE_S           *pstListNode   = NULL;
    DLL_NODE_S           *pstListNode1  = NULL;
    HASH_NODE_S          *pstHashNode   = NULL;
    SC_GW_GRP_NODE_ST    *pstGWGrp      = NULL;
    SC_GW_NODE_ST        *pstGW         = NULL;
    U32                  ulCurrentLen;
    U32                  ulGWCount;
    U32                  ulHashIndex;
    BOOL                 blIsFound = DOS_FALSE;

    if (DOS_ADDR_INVALID(pszNum)
        || DOS_ADDR_INVALID(szCalleeString)
        || ulLength <= 0)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulCurrentLen = 0;
    pthread_mutex_lock(&g_mutexRouteList);
    DLL_Scan(&g_stRouteList, pstListNode, DLL_NODE_S *)
    {
        pstRouetEntry = (SC_ROUTE_NODE_ST *)pstListNode->pHandle;
        if (DOS_ADDR_INVALID(pstRouetEntry))
        {
            continue;
        }

        if (pstRouetEntry->ulID == ulRouteID)
        {
            switch (pstRouetEntry->ulDestType)
            {
                case SC_DEST_TYPE_GATEWAY:
                    ulCurrentLen = dos_snprintf(szCalleeString + ulCurrentLen
                                    , ulLength - ulCurrentLen
                                    , "sofia/gateway/%d/%s|"
                                    , pstRouetEntry->ulDestID
                                    , pszNum);

                    blIsFound = DOS_TRUE;
                    break;
                case SC_DEST_TYPE_GW_GRP:
                    /* ���������� */
                    ulHashIndex = sc_ep_gw_grp_hash_func(pstRouetEntry->ulDestID);
                    pstHashNode = hash_find_node(g_pstHashGWGrp, ulHashIndex, (VOID *)&pstRouetEntry->ulDestID, sc_ep_gw_grp_hash_find);
                    if (DOS_ADDR_INVALID(pstHashNode)
                        || DOS_ADDR_INVALID(pstHashNode->pHandle))
                    {
                        blIsFound = DOS_FALSE;
                        break;
                    }

                    /* �������� */
                    /* ���ɺ����ַ��� */
                    pstGWGrp= pstHashNode->pHandle;
                    ulGWCount = 0;
                    pthread_mutex_lock(&pstGWGrp->mutexGWList);
                    DLL_Scan(&pstGWGrp->stGWList,pstListNode1, DLL_NODE_S *)
                    {
                        if (DOS_ADDR_VALID(pstListNode1)
                            && DOS_ADDR_VALID(pstListNode1->pHandle))
                        {
                            pstGW = pstListNode1->pHandle;
                            ulCurrentLen = dos_snprintf(szCalleeString + ulCurrentLen
                                            , ulLength - ulCurrentLen
                                            , "sofia/gateway/%d/%s|"
                                            , pstGW->ulGWID
                                            , pszNum);

                            ulGWCount++;
                        }
                    }
                    pthread_mutex_unlock(&pstGWGrp->mutexGWList);

                    if (ulGWCount > 0)
                    {
                        blIsFound = DOS_TRUE;
                    }
                    else
                    {
                        blIsFound = DOS_FALSE;
                    }
                    break;
                default:
                    DOS_ASSERT(0);
                    blIsFound = DOS_FALSE;
                    break;
            }
        }
    }
    pthread_mutex_unlock(&g_mutexRouteList);

    if (blIsFound)
    {
        /* ������һ��  | */
        szCalleeString[dos_strlen(szCalleeString) - 1] = '\0';
        return DOS_SUCC;
    }
    else
    {
        szCalleeString[0] = '\0';
        return DOS_FAIL;
    }
}

/**
 * ����: BOOL sc_ep_dst_is_black(S8 *pszNum)
 * ����: �ж�pszNum��ָ���ĺ����Ƿ��ں�������
 * ����:
 *      S8 *pszNum : ��Ҫ������ĺ���
 * ����ֵ: �ɹ���DOS_TRUE�����򷵻�DOS_FALSE
 */
BOOL sc_ep_dst_is_black(S8 *pszNum)
{
    return DOS_FALSE;
}

/**
 * ����: U32 sc_ep_get_source(esl_event_t *pstEvent)
 * ����: ͨ��esl�¼�pstEvent�жϵ�ǰ���е���Դ
 * ����:
 *      esl_event_t *pstEvent : ��Ҫ�������ʱ��
 * ����ֵ: ö��ֵ enum tagCallDirection
 */
U32 sc_ep_get_source(esl_event_t *pstEvent)
{
    const S8 *pszCallSource;

    pszCallSource = esl_event_get_header(pstEvent, "variable_sofia_profile_name");
    if (DOS_ADDR_INVALID(pszCallSource))
    {
        DOS_ASSERT(0);
        return SC_DIRECTION_INVALID;
    }

    if (0 == dos_strcmp(pszCallSource, "internal"))
    {
        return SC_DIRECTION_SIP;
    }

    return SC_DIRECTION_PSTN;
}

/**
 * ����: U32 sc_ep_get_source(esl_event_t *pstEvent)
 * ����: ͨ��esl�¼�pstEvent�жϵ�ǰ���е�Ŀ�ĵ�
 * ����:
 *      esl_event_t *pstEvent : ��Ҫ�������ʱ��
 * ����ֵ: ö��ֵ enum tagCallDirection
 */
U32 sc_ep_get_destination(esl_event_t *pstEvent)
{
    S8 *pszDstNum     = NULL;
    S8 *pszSrcNum     = NULL;
    S8 *pszCallSource = NULL;
    U32 ulCustomID    = U32_BUTT;
    U32 ulCustomID1   = U32_BUTT;

    if (DOS_ADDR_INVALID(pstEvent))
    {
        DOS_ASSERT(0);

        return SC_DIRECTION_INVALID;
    }

    pszDstNum = esl_event_get_header(pstEvent, "Caller-Destination-Number");
    pszSrcNum = esl_event_get_header(pstEvent, "Caller-Caller-ID-Number");
    pszCallSource = esl_event_get_header(pstEvent, "variable_sofia_profile_name");
    if (DOS_ADDR_INVALID(pszDstNum)
        || DOS_ADDR_INVALID(pszSrcNum)
        || DOS_ADDR_INVALID(pszCallSource))
    {
        DOS_ASSERT(0);

        return SC_DIRECTION_INVALID;
    }

    if (sc_ep_dst_is_black(pszDstNum))
    {
        sc_logr_notice(SC_ESL, "The destination is in black list. %s", pszDstNum);

        return SC_DIRECTION_INVALID;
    }

    if (0 == dos_strcmp(pszCallSource, "internal"))
    {
        /* IP�ⷢ��ĺ��У�����һ��ΪĳSIP�˻� */
        ulCustomID = sc_ep_get_custom_by_sip_userid(pszSrcNum);
        if (U32_BUTT == ulCustomID)
        {
            DOS_ASSERT(0);

            sc_logr_info(SC_ESL, "Source number %s is not invalid sip user id. Reject Call", pszSrcNum);
            return DOS_FAIL;
        }

        /*  ���Ա����Ƿ��Ƿֻ��� */
        if (sc_ep_check_extension(pszDstNum, ulCustomID))
        {
            return SC_DIRECTION_SIP;
        }

        /* ���к����Ƿ���ͬһ���ͻ��µ�SIP User ID */
        ulCustomID1 = sc_ep_get_custom_by_sip_userid(pszDstNum);
        if (ulCustomID == ulCustomID1)
        {
            return SC_DIRECTION_SIP;
        }

        return SC_DIRECTION_PSTN;
    }
    else
    {
        ulCustomID = sc_ep_get_custom_by_did(pszDstNum);
        if (U32_BUTT == ulCustomID)
        {
            DOS_ASSERT(0);

            sc_logr_notice(SC_ESL, "The destination %s is not a DID number. Reject Call.", pszDstNum);
            return SC_DIRECTION_INVALID;
        }

        return SC_DIRECTION_SIP;
    }
}

/**
 * ����: U32 sc_ep_call_agent(SC_SCB_ST *pstSCB)
 * ����: Ⱥ������֮���ͨ��ϯ
 * ����:
 *      SC_SCB_ST *pstSCB       : ҵ����ƿ�
 * ����ֵ: �ɹ�����DOS_SUCC,ʧ�ܷ���DOS_FAIL
 */
U32 sc_ep_call_agent(SC_SCB_ST *pstSCB)
{
    U32 ulTaskAgentQueueID = U32_BUTT;
    S8            szAPPParam[512] = { 0 };
    SC_ACD_AGENT_INFO_ST stAgentInfo;
    SC_SCB_ST *pstSCBNew = NULL;

    if (DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    /* 1.��ȡ��ϯ���У�2.������ϯ��3.��ͨ��ϯ */
    ulTaskAgentQueueID = sc_task_get_agent_queue(pstSCB->usTCBNo);
    if (U32_BUTT == ulTaskAgentQueueID)
    {
        DOS_ASSERT(0);

        sc_logr_info(SC_ESL, "Cannot get the agent queue for the task %d", pstSCB->ulTaskID);
        goto proc_error;
    }

    if (sc_acd_get_agent_by_grpid(&stAgentInfo, ulTaskAgentQueueID) != DOS_SUCC)
    {
        DOS_ASSERT(0);

        sc_logr_notice(SC_ESL, "There is no useable agent for the task %d. Queue: %d. ", pstSCB->ulTaskID, ulTaskAgentQueueID);
        goto proc_error;
    }

    sc_logr_info(SC_ESL, "Select agent for call OK. Agent ID: %d, User ID: %s, Externsion: %s, Job-Num: %s"
                    , stAgentInfo.ulSiteID
                    , stAgentInfo.szUserID
                    , stAgentInfo.szExtension
                    , stAgentInfo.szEmpNo);

    pstSCBNew = sc_scb_alloc();
    if (DOS_ADDR_INVALID(pstSCBNew))
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_ESL, "%s", "Allc SCB FAIL.");
        goto proc_error;
    }

    pthread_mutex_lock(&pstSCBNew->mutexSCBLock);

    pstSCB->usOtherSCBNo = pstSCBNew->usSCBNo;
    pstSCBNew->ulCustomID = pstSCB->ulCustomID;
    pstSCBNew->ulAgentID = stAgentInfo.ulSiteID;
    pstSCBNew->usOtherSCBNo = pstSCB->usSCBNo;
    pstSCBNew->ucLegRole = SC_CALLEE;

    dos_strncpy(pstSCBNew->szCallerNum, pstSCB->szCalleeNum, sizeof(pstSCBNew->szCallerNum));
    pstSCBNew->szCallerNum[sizeof(pstSCBNew->szCallerNum) - 1] = '\0';
    dos_strncpy(pstSCBNew->szANINum, pstSCB->szCalleeNum, sizeof(pstSCBNew->szANINum));
    pstSCBNew->szANINum[sizeof(pstSCBNew->szANINum) - 1] = '\0';
    dos_strncpy(pstSCBNew->szCalleeNum, stAgentInfo.szUserID, sizeof(pstSCBNew->szCalleeNum));
    pstSCBNew->szCalleeNum[sizeof(pstSCBNew->szCalleeNum) - 1] = '\0';
    dos_strncpy(pstSCBNew->szCalleeNum, stAgentInfo.szUserID, sizeof(pstSCBNew->szCalleeNum));
    pstSCBNew->szCalleeNum[sizeof(pstSCBNew->szCalleeNum) - 1] = '\0';
    dos_strncpy(pstSCBNew->szSiteNum, stAgentInfo.szEmpNo, sizeof(pstSCBNew->szSiteNum));
    pstSCBNew->szSiteNum[sizeof(pstSCBNew->szSiteNum) - 1] = '\0';
    pthread_mutex_unlock(&pstSCB->mutexSCBLock);
    SC_SCB_SET_SERVICE(pstSCBNew, SC_SERV_OUTBOUND_CALL);
    SC_SCB_SET_SERVICE(pstSCBNew, SC_SERV_INTERNAL_CALL);
    SC_SCB_SET_SERVICE(pstSCBNew, SC_SERV_AGENT_CALLBACK);

    SC_SCB_SET_STATUS(pstSCBNew, SC_SCB_EXEC);


    dos_snprintf(szAPPParam, sizeof(szAPPParam)
                    , "bgapi originate {scb_number=%d,main_service=%d,origination_caller_id_number=%s,origination_caller_id_name=%s,waiting_park=true}user/%s &park() \r\n"
                    , pstSCBNew->usSCBNo
                    , SC_SERV_AGENT_CALLBACK
                    , pstSCB->szCalleeNum
                    , pstSCB->szCalleeNum
                    , stAgentInfo.szUserID);

    if (sc_ep_esl_execute_cmd(szAPPParam) != DOS_SUCC)
    {
        /* @TODO �û������Ż� */
        goto proc_error;
    }
    else
    {
        /* @TODO �Ż�  �ȷ������ٴ���ϯ����ϯ��֮ͨ�������ӵ���ϯ */
        sc_ep_esl_execute_cmd(szAPPParam);
        sc_acd_agent_update_status(stAgentInfo.szUserID, SC_ACD_BUSY);

        sc_ep_esl_execute("sleep", "1000", pstSCB->szUUID);
        sc_ep_esl_execute("speak", "flite|kal|Is to connect you with an agent, please wait.", pstSCB->szUUID);
    }

    if (sc_ep_call_notify(&stAgentInfo, pstSCBNew->szCallerNum))
    {
        DOS_ASSERT(0);
    }


    return DOS_SUCC;

proc_error:
    if (pstSCBNew)
    {
        sc_scb_free(pstSCB);
    }

    sc_ep_esl_execute("hangup", NULL, pstSCB->szUUID);
    return DOS_FAIL;
}

/**
 * ����: U32 sc_ep_incoming_call_proc(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
 * ����: ������PSTN���뵽SIP��ĺ���
 * ����:
 *      esl_event_t *pstEvent   : ESL �¼�
 *      esl_handle_t *pstHandle : �������ݵ�handle
 *      SC_SCB_ST *pstSCB       : ҵ����ƿ�
 * ����ֵ: �ɹ�����DOS_SUCC,ʧ�ܷ���DOS_FAIL
 */
U32 sc_ep_incoming_call_proc(SC_SCB_ST *pstSCB)
{
    U32   ulCustomerID = U32_BUTT;
    U32   ulBindType = U32_BUTT;
    U32   ulBindID = U32_BUTT;
    S8    szCallString[512] = { 0, };
    S8    szCallee[32] = { 0, };

    if (DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        goto proc_fail;
    }

    ulCustomerID = sc_ep_get_custom_by_did(pstSCB->szCalleeNum);
    if (U32_BUTT != ulCustomerID)
    {
        if (sc_ep_get_bind_info4did(pstSCB->szCalleeNum, &ulBindType, &ulBindID) != DOS_SUCC
            || ulBindType >=SC_DID_BIND_TYPE_BUTT
            || U32_BUTT == ulBindID)
        {
            DOS_ASSERT(0);

            sc_logr_info(SC_ESL, "Cannot get the bind info for the DID number %s, Reject Call.", pstSCB->szCalleeNum);
            goto proc_fail;
        }

        switch (ulBindType)
        {
            case SC_DID_BIND_TYPE_SIP:
                if (DOS_SUCC != sc_ep_get_userid_by_id(ulBindID, szCallee, sizeof(szCallee)))
                {
                    DOS_ASSERT(0);

                    sc_logr_info(SC_ESL, "DID number %s seems donot bind a SIP User ID, Reject Call.", pstSCB->szCalleeNum);
                    goto proc_fail;
                }

                dos_snprintf(szCallString, sizeof(szCallString), "{other_leg_scb=%d}user/%s", pstSCB->usSCBNo,szCallee);

                sc_ep_esl_execute("answer", "", pstSCB->szUUID);
                sc_ep_esl_execute("bridge", szCallString, pstSCB->szUUID);
                sc_ep_esl_execute("hangup", szCallString, pstSCB->szUUID);
                break;

            case SC_DID_BIND_TYPE_QUEUE:
                sc_ep_call_agent(pstSCB);
                break;

            default:
                DOS_ASSERT(0);

                sc_logr_info(SC_ESL, "DID number %s has bind an error number, Reject Call.", pstSCB->szCalleeNum);
                goto proc_fail;
        }
    }
    else
    {
        DOS_ASSERT(0);

        sc_logr_info(SC_ESL, "Destination is not a DID number, Reject Call. Destination: %s", pstSCB->szCalleeNum);
        goto proc_fail;

    }

    SC_SCB_SET_STATUS(pstSCB, SC_SCB_EXEC);

    return DOS_SUCC;

proc_fail:
    if (pstSCB)
    {
        sc_ep_esl_execute("hangup", szCallString, pstSCB->szUUID);
    }

    return DOS_FAIL;
}

/**
 * ����: U32 sc_ep_outgoing_call_proc(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
 * ����: ������SIP���뵽PSTN��ĺ���
 * ����:
 *      esl_handle_t *pstHandle : �������ݵ�handle
 *      esl_event_t *pstEvent   : ESL �¼�
 *      SC_SCB_ST *pstSCB       : ҵ����ƿ�
 * ����ֵ: �ɹ�����DOS_SUCC,ʧ�ܷ���DOS_FAIL
 */
U32 sc_ep_outgoing_call_proc(SC_SCB_ST *pstSCB)
{
    SC_SCB_ST *pstSCBNew  = NULL;

    if (DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        goto proc_fail;
    }

    pstSCBNew = sc_scb_alloc();
    if (DOS_ADDR_INVALID(pstSCBNew))
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_ESL, "Alloc SCB FAIL, %s:%d", __FUNCTION__, __LINE__);

        goto proc_fail;
    }

    pthread_mutex_lock(&pstSCBNew->mutexSCBLock);
    pstSCB->usOtherSCBNo = pstSCBNew->usSCBNo;
    pstSCBNew->ulCustomID = pstSCB->ulCustomID;
    pstSCBNew->usOtherSCBNo = pstSCB->usSCBNo;
    pstSCBNew->ucLegRole = SC_CALLEE;

    /* @todo ���к���Ӧ��ʹ�ÿͻ���DID���� */
    dos_strncpy(pstSCBNew->szCallerNum, pstSCB->szCallerNum, sizeof(pstSCBNew->szCallerNum));
    pstSCBNew->szCallerNum[sizeof(pstSCBNew->szCallerNum) - 1] = '\0';
    dos_strncpy(pstSCBNew->szCalleeNum, pstSCB->szCalleeNum, sizeof(pstSCBNew->szCalleeNum));
    pstSCBNew->szCalleeNum[sizeof(pstSCBNew->szCalleeNum) - 1] = '\0';
    dos_strncpy(pstSCBNew->szANINum, pstSCB->szCallerNum, sizeof(pstSCBNew->szANINum));
    pstSCBNew->szANINum[sizeof(pstSCBNew->szANINum) - 1] = '\0';

    pthread_mutex_unlock(&pstSCBNew->mutexSCBLock);
    SC_SCB_SET_SERVICE(pstSCBNew, SC_SERV_OUTBOUND_CALL);
    SC_SCB_SET_SERVICE(pstSCBNew, SC_SERV_EXTERNAL_CALL);
    SC_SCB_SET_STATUS(pstSCBNew, SC_SCB_INIT);

    if (sc_send_usr_auth2bs(pstSCBNew) != DOS_SUCC)
    {
        sc_logr_notice(SC_ESL, "Send auth msg FAIL. SCB No: %d", pstSCBNew->usSCBNo);

        goto proc_fail;
    }

#if 0
    if (sc_ep_dst_is_black(pstSCB->szCalleeNum))
    {
        DOS_ASSERT(0);

        sc_call_trace(pstSCB,"The callee is in BLACK LIST. The call will be hungup later. UUID: %s", pstSCB->szUUID);
        goto proc_fail;
    }

    ulRouteID = sc_ep_search_route(pstSCB);
    if (U32_BUTT == ulRouteID)
    {
        DOS_ASSERT(0);

        sc_call_trace(pstSCB,"Find trunk gruop FAIL. The call will be hungup later. UUID: %s", pstSCB->szUUID);
        goto proc_fail;
    }
    sc_logr_info(SC_ESL, "Search Route SUCC. Route ID: %d", ulRouteID);

    pstSCB->ulTrunkID = ulRouteID;
    if (DOS_SUCC != sc_ep_get_callee_string(ulRouteID, pstSCB->szCalleeNum, szCallString, sizeof(szCallString)))
    {
        DOS_ASSERT(0);

        sc_call_trace(pstSCB,"Make call string FAIL. The call will be hungup later. UUID: %s", pstSCB->szUUID);
        goto proc_fail;
    }
    sc_logr_info(SC_ESL, "Make Call String SUCC. Call String: %s", szCallString);

    dos_snprintf(szCallParam, sizeof(szCallParam), "{other_leg_scb=%d}%s", pstSCB->usSCBNo, szCallString);

    /* �������з����� */
    sc_ep_esl_execute("answer", NULL, pstSCB->szUUID);
    sc_ep_esl_execute("bridge", szCallParam, pstSCB->szUUID);
    sc_ep_esl_execute("hangup", "", pstSCB->szUUID);

    SC_SCB_SET_STATUS(pstSCB, SC_SCB_EXEC);
#endif
    return DOS_SUCC;

proc_fail:
    if (DOS_ADDR_VALID(pstSCB))
    {
        /* @TODO  �Ż�����Ҫֱ�Ӹ��Ҷ��� */
        sc_ep_esl_execute("hangup", "", pstSCB->szUUID);
    }

    if (DOS_ADDR_VALID(pstSCBNew))
    {
        dos_dmem_free(pstSCBNew);
    }

    return DOS_FAIL;
}

/**
 * ����: U32 sc_ep_auto_dial_proc(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
 * ����: ������ϵͳ�Զ�����ĺ���
 * ����:
 *      esl_handle_t *pstHandle : �������ݵ�handle
 *      esl_event_t *pstEvent   : ESL �¼�
 *      SC_SCB_ST *pstSCB       : ҵ����ƿ�
 * ����ֵ: �ɹ�����DOS_SUCC,ʧ�ܷ���DOS_FAIL
 */
U32 sc_ep_auto_dial_proc(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
{
    S8      szAPPParam[512]    = { 0, };
    U32     ulTaskMode         = U32_BUTT;

    SC_TRACE_IN(pstEvent, pstHandle, pstSCB, 0);

    if (DOS_ADDR_INVALID(pstEvent)
        || DOS_ADDR_INVALID(pstHandle)
        || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        goto auto_call_proc_error;
    }

    sc_call_trace(pstSCB, "Start process event %s.", esl_event_get_header(pstEvent, "Event-Name"));

    if (!sc_call_check_service(pstSCB, SC_SERV_AUTO_DIALING))
    {
        DOS_ASSERT(0);

        sc_logr_debug(SC_ESL, "Process event %s finished. SCB do not include service auto call."
                            , esl_event_get_header(pstEvent, "Event-Name"));
        goto auto_call_proc_error;
    }

    ulTaskMode = sc_task_get_mode(pstSCB->usTCBNo);
    if (ulTaskMode >= SC_TASK_MODE_BUTT)
    {
        DOS_ASSERT(0);

        sc_logr_debug(SC_ESL, "Process event %s finished. Cannot get the task mode for task %d."
                            , esl_event_get_header(pstEvent, "Event-Name")
                            , pstSCB->usTCBNo);
        goto auto_call_proc_error;
    }

    /* �Զ������Ҫ���� */
    /* 1.AOTO CALL�ߵ�����ͻ��Ƕ��Ѿ���ͨ�ˡ��������������������ͣ�����Ӧ�����ͺ� */
    switch (ulTaskMode)
    {
        /* ��Ҫ�����ģ�ͳһ�ȷ������ڷ����������봦��������� */
        case SC_TASK_MODE_KEY4AGENT:
        case SC_TASK_MODE_AUDIO_ONLY:
        case SC_TASK_MODE_AGENT_AFTER_AUDIO:
            sc_ep_esl_execute("set", "ignore_early_media=true", pstSCB->szUUID);
            sc_ep_esl_execute("sleep", "500", pstSCB->szUUID);

            dos_snprintf(szAPPParam, sizeof(szAPPParam)
                            , "+%d %s"
                            , sc_task_audio_playcnt(pstSCB->usTCBNo)
                            , sc_task_get_audio_file(pstSCB->usTCBNo));
            sc_ep_esl_execute("loop_playback", szAPPParam, pstSCB->szUUID);
            pstSCB->ucCurrentPlyCnt = sc_task_audio_playcnt(pstSCB->usTCBNo);

            break;

        /* ֱ�ӽ�ͨ��ϯ */
        case SC_TASK_MODE_DIRECT4AGETN:
            sc_ep_call_agent(pstSCB);

            break;

        default:
            DOS_ASSERT(0);
            goto auto_call_proc_error;
    }

    SC_SCB_SET_STATUS(pstSCB, SC_SCB_ACTIVE);

    sc_call_trace(pstSCB, "Finished to process %s event.", esl_event_get_header(pstEvent, "Event-Name"));

    SC_TRACE_OUT();
    return DOS_SUCC;

auto_call_proc_error:
    sc_call_trace(pstSCB, "FAILED to process %s event.", esl_event_get_header(pstEvent, "Event-Name"));

    if (DOS_ADDR_VALID(pstSCB))
    {
        SC_SCB_SET_STATUS(pstSCB, SC_SCB_RELEASE);
        sc_ep_esl_execute("hangup", NULL, pstSCB->szUUID);
    }

    return DOS_FAIL;
}

/**
 * ����: U32 sc_ep_internal_call_process(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
 * ����: �����ڲ�����
 * ����:
 *      esl_handle_t *pstHandle : �������ݵ�handle
 *      esl_event_t *pstEvent   : ESL �¼�
 *      SC_SCB_ST *pstSCB       : ҵ����ƿ�
 * ����ֵ: �ɹ�����DOS_SUCC,ʧ�ܷ���DOS_FAIL
 */
U32 sc_ep_internal_call_process(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
{
    S8    *pszSrcNum = NULL;
    S8    *pszDstNum = NULL;
    S8    *pszUUID = NULL;
    S8    szSIPUserID[32] = { 0, };
    S8    szCallString[512] = { 0, };
    U32   ulCustomerID = U32_BUTT;
    U32   ulCustomerID1 = U32_BUTT;

    if (DOS_ADDR_INVALID(pstEvent) || DOS_ADDR_INVALID(pstHandle) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    /* ��ȡ�¼���UUID */
    pszUUID = esl_event_get_header(pstEvent, "Unique-ID");
    if (DOS_ADDR_INVALID(pszUUID))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        goto process_fail;
    }

    pszDstNum = esl_event_get_header(pstEvent, "Caller-Destination-Number");
    if (DOS_ADDR_INVALID(pszDstNum))
    {
        DOS_ASSERT(0);

        goto process_fail;
    }

    pszDstNum = esl_event_get_header(pstEvent, "Caller-Destination-Number");
    pszSrcNum = esl_event_get_header(pstEvent, "Caller-Caller-ID-Number");
    if (DOS_ADDR_INVALID(pszDstNum)
        || DOS_ADDR_INVALID(pszSrcNum))
    {
        DOS_ASSERT(0);

        goto process_fail;
    }

    /* �жϱ��к����Ƿ��Ƿֻ��ţ�����Ƿֻ��ţ���Ҫ�ҵ���Ӧ��SIP�˻����ٺ��У�ͬʱ����֮ǰ����Ҫ��ȡ���еķֻ��ţ��޸�ANIΪ���еķֻ��� */
    ulCustomerID = sc_ep_get_custom_by_sip_userid(pszSrcNum);
    if (U32_BUTT == ulCustomerID)
    {
        DOS_ASSERT(0);

        sc_logr_info(SC_ESL, "The source number %s seem not beyound to any customer, Reject Call", pszSrcNum);
        goto process_fail;
    }

    ulCustomerID1 = sc_ep_get_custom_by_sip_userid(pszDstNum);
    if (U32_BUTT != ulCustomerID1)
    {
        if (ulCustomerID == ulCustomerID1)
        {
            dos_snprintf(szCallString, sizeof(szCallString), "user/%s", pszDstNum);
            sc_ep_esl_execute("bridge", szCallString, pszUUID);
            sc_ep_esl_execute("hangup", NULL, pszUUID);
        }
        else
        {
            DOS_ASSERT(0);

            sc_logr_info(SC_ESL, "Cannot call other customer direct, Reject Call. Src %s is owned by customer %d, Dst %s is owned by customer %d"
                            , pszSrcNum, ulCustomerID, pszDstNum, ulCustomerID1);
            goto process_fail;
        }
    }
    else
    {
        if (sc_ep_get_userid_by_extension(ulCustomerID, pszDstNum, szSIPUserID, sizeof(szSIPUserID)) != DOS_SUCC)
        {
            DOS_ASSERT(0);

            sc_logr_info(SC_ESL, "Destination number %s is not seems a SIP User ID or Extension. Reject Call", pszDstNum);
            goto process_fail;
        }

        dos_snprintf(szCallString, sizeof(szCallString), "user/%s", szSIPUserID);
        sc_ep_esl_execute("bridge", szCallString, pszUUID);
        sc_ep_esl_execute("hangup", NULL, pszUUID);
    }

    return DOS_SUCC;

process_fail:
    if (pstSCB)
    {
        sc_ep_esl_execute("hangup", NULL, pstSCB->szUUID);
    }

    return DOS_FAIL;
}

/**
 * ����: U32 sc_ep_internal_call_process(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
 * ����: �����ڲ�����
 * ����:
 *      esl_handle_t *pstHandle : �������ݵ�handle
 *      esl_event_t *pstEvent   : ESL �¼�
 *      SC_SCB_ST *pstSCB       : ҵ����ƿ�
 * ����ֵ: �ɹ�����DOS_SUCC,ʧ�ܷ���DOS_FAIL
 */
U32 sc_ep_internal_service_proc(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
{
    S8    *pszUUID = NULL;

    sc_logr_info(SC_ESL, "%s", "Start exec internal service.");

    pszUUID = esl_event_get_header(pstEvent, "Unique-ID");
    if (DOS_ADDR_INVALID(pszUUID))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    sc_ep_esl_execute("answer", NULL, pszUUID);
    sc_ep_esl_execute("sleep", "1000", pszUUID);
    sc_ep_esl_execute("speak", "flite|kal|Temporary not support.", pszUUID);
    sc_ep_esl_execute("hangup", NULL, pszUUID);
    return DOS_SUCC;
}

/**
 * ����: U32 sc_ep_channel_park_proc(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
 * ����: ����ESL��CHANNEL PARK�¼�
 * ����:
 *      esl_handle_t *pstHandle : ���;��
 *      esl_event_t *pstEvent   : ʱ��
 *      SC_SCB_ST *pstSCB       : SCB
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 */
U32 sc_ep_channel_park_proc(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
{
    S8        *pszIsAutoCall = NULL;
    S8        *pszCaller     = NULL;
    S8        *pszCallee     = NULL;
    S8        *pszOtherSCBNo = NULL;
    S8        *pszUUID       = NULL;
    S8        *pszMainService = NULL;
    U32       ulCallSrc, ulCallDst;
    U32       ulRet = DOS_SUCC;
    U32       ulMainService = U32_BUTT;
    U32       ulOtherSCBNo  = U32_BUTT;
    SC_SCB_ST *pstSCBOther  = NULL;

    if (DOS_ADDR_INVALID(pstEvent)
        || DOS_ADDR_INVALID(pstHandle)
        || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    sc_logr_debug(SC_ESL, "Start process event %s.", esl_event_get_header(pstEvent, "Event-Name"));

    /*  1.������ƿ�
     *  2.�ж��Ƿ����Զ����
     *    ������Զ����: ��ʹ��originate��������
     *  3.��ҵ��
     *    ִ����Ӧҵ��
     *  4.��ͨ����
     *    ����·�ɣ����м��飬���߶�Ӧ��SIP�ֻ�����
     */

    /* ��ȡ�¼���UUID */
    pszUUID = esl_event_get_header(pstEvent, "Caller-Unique-ID");
    if (DOS_ADDR_INVALID(pszUUID))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    /* ҵ����� */
    pszIsAutoCall = esl_event_get_header(pstEvent, "variable_auto_call");
    pszCaller     = esl_event_get_header(pstEvent, "Caller-Caller-ID-Number");
    pszCallee     = esl_event_get_header(pstEvent, "Caller-Destination-Number");
    sc_logr_info(SC_ESL, "Route Call Start: Auto Call Flag: %s, Caller: %s, Callee: %d"
                , NULL == pszIsAutoCall ? "NULL" : pszIsAutoCall
                , NULL == pszCaller ? "NULL" : pszCaller
                , NULL == pszCallee ? "NULL" : pszCallee);

    pszMainService = esl_event_get_header(pstEvent, "variable_main_service");
    if (DOS_ADDR_INVALID(pszMainService)
        || dos_atoul(pszMainService, &ulMainService) < 0)
    {
        ulMainService = U32_BUTT;
    }

    /* �����AUTO Call�Ͳ���Ҫ����SCB����SCBͬ����HASH���оͺ� */
    if (SC_SERV_AUTO_DIALING == ulMainService)
    {
        /* �Զ�������� */
        ulRet = sc_ep_auto_dial_proc(pstHandle, pstEvent, pstSCB);
    }
    /* ����ǻغ�����ϯ�ĺ��С�����Ҫ���ӿͻ�����ϯ */
    else if (SC_SERV_AGENT_CALLBACK == ulMainService
        || SC_SERV_OUTBOUND_CALL == ulMainService)
    {
        S8 szCMDBuff[512] = { 0, };

        pszOtherSCBNo = esl_event_get_header(pstEvent, "variable_other_leg_scb");
        if (DOS_ADDR_INVALID(pszOtherSCBNo)
            || dos_atoul(pszOtherSCBNo, &ulOtherSCBNo) < 0)
        {
            DOS_ASSERT(0);

            sc_ep_esl_execute("hangup", NULL, pszUUID);
            ulRet = DOS_FAIL;

            goto proc_finished;
        }

        pstSCBOther = sc_scb_get(ulOtherSCBNo);
        if (DOS_ADDR_INVALID(pstSCBOther))
        {
            DOS_ASSERT(0);

            sc_ep_esl_execute("hangup", NULL, pszUUID);
            ulRet = DOS_FAIL;

            goto proc_finished;
        }

        /* �������ִ��ʧ�ܣ�����Ҫ�Ҷ�����һͨ���� */
        dos_snprintf(szCMDBuff, sizeof(szCMDBuff), "bgapi uuid_bridge %s %s \r\n", pstSCB->szUUID, pstSCBOther->szUUID);
        pstSCBOther->usOtherSCBNo= pstSCB->usSCBNo;
        pstSCB->usOtherSCBNo = pstSCBOther->usSCBNo;

        if (sc_ep_esl_execute_cmd(szCMDBuff) != DOS_SUCC)
        {
            sc_ep_esl_execute("hangup", NULL, pstSCBOther->szUUID);
            sc_ep_esl_execute("hangup", NULL, pszUUID);
            ulRet = DOS_FAIL;

            goto proc_finished;

        }

        SC_SCB_SET_STATUS(pstSCB, SC_SCB_ACTIVE);

        sc_logr_info(SC_ESL, "Agent has benn connected. UUID: %s <> %s. SCBNo: %d <> %d."
                     , pstSCB->szUUID, pstSCBOther->szUUID
                     , pstSCB->usSCBNo, pstSCBOther->usSCBNo);
    }
    else if (sc_ep_internal_service_check(pstEvent) != SC_INTER_SRV_BUTT)
    {
        /* �������з����� */
        sc_ep_esl_execute("answer", NULL, pszUUID);

        /* �ڲ�ҵ���� */
        SC_SCB_SET_SERVICE(pstSCB, SC_SERV_INBOUND_CALL);
        SC_SCB_SET_SERVICE(pstSCB, SC_SERV_INTERNAL_CALL);
        SC_SCB_SET_SERVICE(pstSCB, SC_SERV_INTERNAL_SERVICE);

        ulRet = sc_ep_internal_service_proc(pstHandle, pstEvent, pstSCB);
    }
    else
    {
        /* �������д��� */
        pstSCB->ucLegRole = SC_CALLEE;
        ulCallSrc = sc_ep_get_source(pstEvent);
        ulCallDst = sc_ep_get_destination(pstEvent);

        sc_logr_info(SC_ESL, "Get call source and dest. Source: %d, Dest: %d", ulCallSrc, ulCallDst);

        if (SC_DIRECTION_SIP == ulCallSrc && SC_DIRECTION_PSTN == ulCallDst)
        {
            sc_ep_esl_execute("answer", NULL, pszUUID);

            SC_SCB_SET_SERVICE(pstSCB, SC_SERV_INBOUND_CALL);
            SC_SCB_SET_SERVICE(pstSCB, SC_SERV_INTERNAL_CALL);

            /* ���Ĳ�ͬ�����У���ȡ��ǰ����ʱ��һ���ͻ� */
            pstSCB->ulCustomID = sc_ep_get_custom_by_sip_userid(pstSCB->szCallerNum);
            if (U32_BUTT == pstSCB->ulCustomID
                || sc_ep_outgoing_call_proc(pstSCB) != DOS_SUCC)
            {
                pstSCB->ucTerminationFlag = DOS_TRUE;
                pstSCB->ucTerminationCause = BS_ERR_SYSTEM;

                sc_ep_esl_execute("hangup", NULL, pszUUID);

                SC_SCB_SET_STATUS(pstSCB, SC_SCB_RELEASE);
                ulRet = DOS_FAIL;
            }
            else
            {
                SC_SCB_SET_STATUS(pstSCB, SC_SCB_EXEC);
            }
        }
        else if (SC_DIRECTION_PSTN == ulCallSrc && SC_DIRECTION_SIP == ulCallDst)
        {
            SC_SCB_SET_SERVICE(pstSCB, SC_SERV_INBOUND_CALL);
            SC_SCB_SET_SERVICE(pstSCB, SC_SERV_EXTERNAL_CALL);

            pstSCB->ulCustomID = sc_ep_get_custom_by_did(pstSCB->szCalleeNum);
            if (pstSCB->ulCustomID != U32_BUTT
                && sc_send_usr_auth2bs(pstSCB) != DOS_SUCC)
            {
                pstSCB->ucTerminationFlag = DOS_TRUE;
                pstSCB->ucTerminationCause = BS_ERR_SYSTEM;

                sc_ep_esl_execute("hangup", NULL, pszUUID);

                SC_SCB_SET_STATUS(pstSCB, SC_SCB_RELEASE);
                ulRet = DOS_FAIL;
            }
            else
            {
                SC_SCB_SET_STATUS(pstSCB, SC_SCB_AUTH);
            }
        }
        else if (SC_DIRECTION_SIP == ulCallSrc && SC_DIRECTION_SIP == ulCallDst)
        {
            SC_SCB_SET_SERVICE(pstSCB, SC_SERV_INTERNAL_CALL);

            ulRet = sc_ep_internal_call_process(pstHandle, pstEvent, pstSCB);
        }
        else
        {
            DOS_ASSERT(0);
            sc_logr_info(SC_ESL, "Invalid call source or destension. Source: %d, Dest: %d", ulCallSrc, ulCallDst);

            ulRet = DOS_FAIL;
        }
    }

proc_finished:
    sc_call_trace(pstSCB, "Finished to process %s event. Result : %s"
                    , esl_event_get_header(pstEvent, "Event-Name")
                    , (DOS_SUCC == ulRet) ? "OK" : "FAILED");

    return ulRet;
}

U32 sc_ep_backgroud_job_proc(esl_handle_t *pstHandle, esl_event_t *pstEvent)
{
    S8    *pszEventBody   = NULL;
    S8    *pszAppName     = NULL;
    S8    *pszAppArg      = NULL;
    S8    *pszStart       = NULL;
    S8    *pszEnd         = NULL;
    S8    szSCBNo[16]      = { 0 };
    U32   ulProcessResult = DOS_SUCC;
    U32   ulOtherSCBNo    = 0;
    SC_SCB_ST   *pstSCB = NULL;

    if (DOS_ADDR_INVALID(pstHandle)
        || DOS_ADDR_INVALID(pstEvent))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pszEventBody = esl_event_get_body(pstEvent);
    pszAppName   = esl_event_get_header(pstEvent, "Job-Command");
    pszAppArg    = esl_event_get_header(pstEvent, "Job-Command-Arg");
    if (DOS_ADDR_VALID(pszEventBody)
        && DOS_ADDR_VALID(pszAppName)
        && DOS_ADDR_VALID(pszAppArg))
    {
        if (0 == dos_strnicmp(pszEventBody, "+OK", dos_strlen("+OK")))
        {
            ulProcessResult = DOS_SUCC;
        }
        else
        {
            ulProcessResult = DOS_FAIL;
        }

        sc_logr_info(SC_ESL, "Execute command %s %s %s, Info: %s."
                        , pszAppName
                        , pszAppArg
                        , DOS_SUCC == ulProcessResult ? "SUCC" : "FAIL"
                        , pszEventBody);

        if (DOS_SUCC == ulProcessResult)
        {
            goto process_finished;
        }

        pszStart = dos_strstr(pszAppArg, "other_leg_scb=");
        if (DOS_ADDR_INVALID(pszStart))
        {
            DOS_ASSERT(0);
            goto process_fail;
        }

        pszStart += dos_strlen("other_leg_scb=");
        if (DOS_ADDR_INVALID(pszStart))
        {
            DOS_ASSERT(0);
            goto process_fail;
        }

        pszEnd = dos_strstr(pszStart, ",");
        if (DOS_ADDR_VALID(pszEnd))
        {
            dos_strncpy(szSCBNo, pszStart, pszEnd-pszStart);
            szSCBNo[pszEnd-pszStart] = '\0';
        }
        else
        {
            dos_strncpy(szSCBNo, pszStart, sizeof(szSCBNo));
            szSCBNo[sizeof(szSCBNo)-1] = '\0';
        }

        if (dos_atoul(szSCBNo, &ulOtherSCBNo) < 0)
        {
            DOS_ASSERT(0);
            goto process_fail;
        }

        pstSCB = sc_scb_get(ulOtherSCBNo);
        if (DOS_ADDR_INVALID(pstSCB))
        {
            DOS_ASSERT(0);
            goto process_fail;
        }

        if (dos_stricmp(pszAppName, "originate") == 0)
        {
            sc_ep_esl_execute("hangup", NULL, pstSCB->szUUID);
        }
    }

process_finished:
    return DOS_SUCC;

process_fail:
    return DOS_FAIL;
}


/**
 * ����: U32 sc_ep_channel_create_proc(esl_handle_t *pstHandle, esl_event_t *pstEvent)
 * ����: ����ESL��CHANNEL CREATE�¼�
 * ����:
 *      esl_event_t *pstEvent   : ʱ��
 *      esl_handle_t *pstHandle : ���;��
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 */
U32 sc_ep_channel_create_proc(esl_handle_t *pstHandle, esl_event_t *pstEvent)
{
    S8          *pszUUID = NULL;
    S8          *pszMainService = NULL;
    S8          *pszSCBNum = NULL;
    S8          *pszOtherSCBNo = NULL;
    SC_SCB_ST   *pstSCB = NULL;
    SC_SCB_ST   *pstSCB1 = NULL;
    S8          szBuffCmd[128] = { 0 };
    U32         ulSCBNo = 0;
    U32         ulOtherSCBNo = 0;
    U32         ulRet = DOS_SUCC;
    U32         ulMainService = U32_BUTT;


    SC_TRACE_IN(pstEvent, pstHandle, pstSCB, 0);

    if (DOS_ADDR_INVALID(pstEvent)
        || DOS_ADDR_INVALID(pstHandle))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_SUCC;
    }

    sc_logr_debug(SC_ESL, "Start process event %s.", esl_event_get_header(pstEvent, "Event-Name"));

    pszUUID = esl_event_get_header(pstEvent, "Caller-Unique-ID");
    if (DOS_ADDR_INVALID(pszUUID))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pszMainService = esl_event_get_header(pstEvent, "variable_main_service");
    if (DOS_ADDR_INVALID(pszMainService)
        || dos_atoul(pszMainService, &ulMainService) < 0)
    {
        ulMainService = U32_BUTT;
    }

    /* �����AUTO Call�Ͳ���Ҫ����SCB����SCBͬ����HASH���оͺ� */
    pszSCBNum = esl_event_get_header(pstEvent, "variable_scb_number");
    if (DOS_ADDR_VALID(pszSCBNum))
    {
        if (dos_atoul(pszSCBNum, &ulSCBNo) < 0)
        {
            DOS_ASSERT(0);

            goto process_fail;
        }

        pstSCB = sc_scb_get(ulSCBNo);
        if (DOS_ADDR_INVALID(pstSCB))
        {
            DOS_ASSERT(0);

            goto process_fail;
        }

        /* ����UUID */
        dos_strncpy(pstSCB->szUUID, pszUUID, sizeof(pstSCB->szUUID));
        pstSCB->szUUID[sizeof(pstSCB->szUUID) - 1] = '\0';

        sc_scb_hash_tables_add(pszUUID, pstSCB);
        sc_task_concurrency_add(pstSCB->usTCBNo);

        goto process_finished;

process_fail:
       ulRet = DOS_FAIL;
    }
    else
    {
        pstSCB = sc_scb_alloc();
        if (DOS_ADDR_INVALID(pstSCB))
        {
            DOS_ASSERT(0);
            sc_logr_error(SC_ESL, "%s", "Alloc SCB FAIL.");

            SC_SCB_SET_STATUS(pstSCB, SC_SCB_RELEASE);

            SC_TRACE_OUT();
            return DOS_FAIL;
        }

        sc_scb_hash_tables_add(pszUUID, pstSCB);
        sc_ep_parse_event(pstEvent, pstSCB);

        dos_strncpy(pstSCB->szUUID, pszUUID, sizeof(pstSCB->szUUID));
        pstSCB->szUUID[sizeof(pstSCB->szUUID) - 1] = '\0';

        /* ��ͨ�����ñ��� */
        dos_snprintf(szBuffCmd, sizeof(szBuffCmd), "scb_number=%d", pstSCB->usSCBNo);
        sc_ep_esl_execute("set", szBuffCmd, pszUUID);

        SC_SCB_SET_STATUS(pstSCB, SC_SCB_INIT);
    }

    /* ���ݲ���  �л�SCB No */
    pszOtherSCBNo = esl_event_get_header(pstEvent, "variable_other_leg_scb");
    if (DOS_ADDR_INVALID(pszOtherSCBNo)
        && dos_atoul(pszOtherSCBNo, &ulOtherSCBNo) < 0)
    {
        goto process_finished;
    }

    pstSCB1 = sc_scb_get(ulOtherSCBNo);
    if (DOS_ADDR_VALID(pstSCB)
        && DOS_ADDR_VALID(pstSCB1))
    {
        pstSCB->usOtherSCBNo = pstSCB1->usSCBNo;
        pstSCB1->usOtherSCBNo = pstSCB->usSCBNo;
    }

process_finished:

    sc_call_trace(pstSCB, "Finished to process %s event.", esl_event_get_header(pstEvent, "Event-Name"));

    SC_TRACE_OUT();
    return ulRet;
}


/**
 * ����: U32 sc_ep_channel_answer(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
 * ����: ����ESL��CHANNEL EXECUTE COMPLETE�¼�
 * ����:
 *      esl_handle_t *pstHandle : ���;��
 *      esl_event_t *pstEvent   : ʱ��
 *      SC_SCB_ST *pstSCB       : SCB
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 */
U32 sc_ep_channel_answer(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
{
    S8 *pszWaitingPark = NULL;

    SC_TRACE_IN(pstEvent, pstHandle, pstSCB, 0);

    if (DOS_ADDR_INVALID(pstEvent)
        || DOS_ADDR_INVALID(pstHandle)
        || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_SUCC;
    }

    sc_call_trace(pstSCB, "Start process event %s.", esl_event_get_header(pstEvent, "Event-Name"));

    /* ���û������waiting park��־����ֱ���л�״̬��active */
    pszWaitingPark = esl_event_get_header(pstEvent, "variable_waiting_park");
    if (DOS_ADDR_INVALID(pszWaitingPark)
        || 0 != dos_strncmp(pszWaitingPark, "true", dos_strlen("true")))
    {
        SC_SCB_SET_STATUS(pstSCB, SC_SCB_ACTIVE);
    }

    sc_call_trace(pstSCB, "Finished to process %s event.", esl_event_get_header(pstEvent, "Event-Name"));

    SC_TRACE_OUT();
    return DOS_SUCC;
}

/**
 * ����: U32 sc_ep_channel_hungup_proc(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
 * ����: ����ESL��CHANNEL HANGUP�¼�
 * ����:
 *      esl_handle_t *pstHandle : ���;��
 *      esl_event_t *pstEvent   : ʱ��
 *      SC_SCB_ST *pstSCB       : SCB
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 */
U32 sc_ep_channel_hungup_proc(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
{
    SC_TRACE_IN(pstEvent, pstHandle, pstSCB, 0);

    if (DOS_ADDR_INVALID(pstEvent)
        || DOS_ADDR_INVALID(pstHandle)
        || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_SUCC;
    }

    sc_call_trace(pstSCB, "Start process event %s.", esl_event_get_header(pstEvent, "Event-Name"));

    SC_SCB_SET_STATUS(pstSCB, SC_SCB_RELEASE);

    sc_call_trace(pstSCB, "Finished to process %s event.", esl_event_get_header(pstEvent, "Event-Name"));

    SC_TRACE_OUT();
    return DOS_SUCC;

}

/**
 * ����: U32 sc_ep_channel_hungup_complete_proc(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
 * ����: ����ESL��CHANNEL HANGUP COMPLETE�¼�
 * ����:
 *      esl_handle_t *pstHandle : ���;��
 *      esl_event_t *pstEvent   : ʱ��
 *      SC_SCB_ST *pstSCB       : SCB
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 */
U32 sc_ep_channel_hungup_complete_proc(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
{
    U32         ulStatus, ulRet = DOS_SUCC;
    SC_SCB_ST   *pstSCBOther = NULL;

    SC_TRACE_IN(pstEvent, pstHandle, pstSCB, 0);

    if (DOS_ADDR_INVALID(pstEvent)
        || DOS_ADDR_INVALID(pstHandle)
        || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_SUCC;
    }

    ulStatus = pstSCB->ucStatus;
    switch (ulStatus)
    {
        case SC_SCB_IDEL:
        case SC_SCB_INIT:
        case SC_SCB_AUTH:
        case SC_SCB_EXEC:
        case SC_SCB_ACTIVE:
        case SC_SCB_RELEASE:
            /* ͳһ����Դ��Ϊrelease״̬ */
            SC_SCB_SET_STATUS(pstSCB, SC_SCB_RELEASE);

            /* ����ǰleg����Ϣdump���� */
            pstSCB->pstExtraData = dos_dmem_alloc(sizeof(SC_SCB_EXTRA_DATA_ST));
            if (DOS_ADDR_VALID(pstSCB->pstExtraData))
            {
                dos_memzero(pstSCB->pstExtraData, sizeof(SC_SCB_EXTRA_DATA_ST));
                sc_rp_parse_extra_data(pstEvent, pstSCB);
            }

            /*
             * 1.���������һ���ȣ��б�Ҫ�ȴ�����һ�����ͷ�
             * 2.��Ҫ����һ����û�д��ڵȴ��ͷ�״̬���Ǿ͵ȴ���
             */
            pstSCBOther = sc_scb_get(pstSCB->usOtherSCBNo);
            if (DOS_ADDR_VALID(pstSCBOther)
                && !pstSCBOther->bWaitingOtherRelase)
            {
                pstSCB->bWaitingOtherRelase = DOS_TRUE;

                sc_ep_esl_execute("hangup", NULL, pstSCBOther->szUUID);

                sc_logr_info(SC_ESL, "Waiting other leg hangup.Curretn Leg UUID: %s, Other Leg UUID: %s"
                                , pstSCB->szUUID ? pstSCB->szUUID : "NULL"
                                , pstSCBOther->szUUID ? pstSCBOther->szUUID : "NULL");
                break;
            }

            /* �Զ��������Ҫά������Ĳ����� */
            if (sc_call_check_service(pstSCB, SC_SERV_AUTO_DIALING))
            {
                sc_task_concurrency_minus(pstSCB->usTCBNo);
            }

            sc_logr_debug(SC_ESL, "Send CDR to bs. SCB1 No:%d, SCB2 No:%d", pstSCB->usSCBNo, pstSCB->usOtherSCBNo);
            /* ���ͻ��� */
            if (sc_send_billing_stop2bs(pstSCB) != DOS_SUCC)
            {
                sc_logr_debug(SC_ESL, "Send CDR to bs FAIL. SCB1 No:%d, SCB2 No:%d", pstSCB->usSCBNo, pstSCB->usOtherSCBNo);
            }
            else
            {
                sc_logr_debug(SC_ESL, "Send CDR to bs SUCC. SCB1 No:%d, SCB2 No:%d", pstSCB->usSCBNo, pstSCB->usOtherSCBNo);
            }

            sc_logr_debug(SC_ESL, "Start release the SCB. SCB1 No:%d, SCB2 No:%d", pstSCB->usSCBNo, pstSCB->usOtherSCBNo);
            /* ά����Դ */
            sc_scb_hash_tables_delete(pstSCB->szUUID);
            if (DOS_ADDR_VALID(pstSCBOther))
            {
                sc_scb_hash_tables_delete(pstSCBOther->szUUID);
            }

            sc_scb_free(pstSCB);
            if (pstSCBOther)
            {
                sc_scb_free(pstSCBOther);
            }
            break;
        default:
            DOS_ASSERT(0);
            ulRet = DOS_FAIL;
            break;
    }


    sc_call_trace(pstSCB, "Finished to process %s event.", esl_event_get_header(pstEvent, "Event-Name"));

    SC_TRACE_OUT();
    return DOS_SUCC;

}

/**
 * ����: U32 sc_ep_dtmf_proc(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
 * ����: ����ESL��CHANNEL DTMF�¼�
 * ����:
 *      esl_handle_t *pstHandle : ���;��
 *      esl_event_t *pstEvent   : ʱ��
 *      SC_SCB_ST *pstSCB       : SCB
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 */
U32 sc_ep_dtmf_proc(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
{
    S8 *pszDTMFDigit = NULL;
    U32 ulTaskMode   = U32_BUTT;


    SC_TRACE_IN(pstEvent, pstHandle, pstSCB, 0);

    if (DOS_ADDR_INVALID(pstEvent)
        || DOS_ADDR_INVALID(pstHandle)
        || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_SUCC;
    }

    sc_call_trace(pstSCB, "Start process event %s.", esl_event_get_header(pstEvent, "Event-Name"));

    pszDTMFDigit = esl_event_get_header(pstEvent, "DTMF-Digit");
    if (DOS_ADDR_INVALID(pszDTMFDigit))
    {
        DOS_ASSERT(0);
        goto process_fail;
    }

    /* �Զ��������0��ͨ��ϯ */
    if (sc_call_check_service(pstSCB, SC_SERV_AUTO_DIALING)
        && '0' == pszDTMFDigit[0])
    {

        ulTaskMode = sc_task_get_mode(pstSCB->usTCBNo);
        if (ulTaskMode >= SC_TASK_MODE_BUTT)
        {
            DOS_ASSERT(0);
            /* Ҫ��Ҫ�Ҷ� ? */
            goto process_fail;
        }

        if (SC_TASK_MODE_KEY4AGENT == ulTaskMode)
        {
            sc_ep_call_agent(pstSCB);
        }
    }
    else if (sc_call_check_service(pstSCB, SC_SERV_AGENT_CALLBACK))
    {
        /* AGENT�����Կͻ����� */

        /* todoд���� */
    }

    sc_call_trace(pstSCB, "Finished to process %s event.", esl_event_get_header(pstEvent, "Event-Name"));

    SC_TRACE_OUT();
    return DOS_SUCC;

process_fail:
    sc_call_trace(pstSCB, "Finished to process %s event. FAIL.", esl_event_get_header(pstEvent, "Event-Name"));

    SC_TRACE_OUT();
    return DOS_FAIL;

}

/**
 * ����: U32 sc_ep_playback_stop(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
 * ����: ������������¼�
 * ����:
 *      esl_handle_t *pstHandle : ���;��
 *      esl_event_t *pstEvent   : ʱ��
 *      SC_SCB_ST *pstSCB       : SCB
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 */
U32 sc_ep_playback_stop(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
{
    U32           ulTaskMode = 0;
    U32           ulMainService = U32_BUTT;
    S8            *pszMainService = NULL;

    SC_TRACE_IN(pstEvent, pstHandle, pstSCB, 0);

    if (DOS_ADDR_INVALID(pstEvent)
        || DOS_ADDR_INVALID(pstHandle)
        || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_SUCC;
    }

    sc_call_trace(pstSCB, "Start process event %s.", esl_event_get_header(pstEvent, "Event-Name"));
    pszMainService = esl_event_get_header(pstEvent, "variable_main_service");

    if (DOS_ADDR_INVALID(pszMainService)
        || dos_atoul(pszMainService, &ulMainService) < 0)
    {
        ulMainService = U32_BUTT;
    }

    /* �����������OK���͸��ݷ�����������������������Ͳ�OK����ֱ�ӹһ��� */
    if (U32_BUTT != ulMainService)
    {
        if (!sc_call_check_service(pstSCB, ulMainService))
        {
            DOS_ASSERT(0);

            sc_logr_error(SC_ESL, "SCB %d donot have the service %d.", pstSCB->usSCBNo, ulMainService);
            goto proc_error;
        }

        switch (ulMainService)
        {
            case SC_SERV_AUTO_DIALING:

                /* �ȼ��ٲ��Ŵ��������жϲ��Ŵ�����������Ŵ����Ѿ�ʹ�������Ҫ�������� */
                pstSCB->ucCurrentPlyCnt--;
                if (pstSCB->ucCurrentPlyCnt <= 0)
                {
                    ulTaskMode = sc_task_get_mode(pstSCB->usTCBNo);
                    if (ulTaskMode >= SC_TASK_MODE_BUTT)
                    {
                        DOS_ASSERT(0);
                        goto proc_error;
                    }

                    switch (ulTaskMode)
                    {
                        /* �����ַ�����������Ҫ�Ҷ� */
                        case SC_TASK_MODE_KEY4AGENT:
                        case SC_TASK_MODE_AUDIO_ONLY:
                            sc_ep_esl_execute("hangup", NULL, pstSCB->szUUID);
                            break;

                        /* �������ͨ��ϯ */
                        case SC_TASK_MODE_AGENT_AFTER_AUDIO:
                            /* 1.��ȡ��ϯ���У�2.������ϯ��3.��ͨ��ϯ */
                            sc_ep_call_agent(pstSCB);
                            break;

                        /* ����ط��������� */
                        case SC_TASK_MODE_DIRECT4AGETN:
                        default:
                            DOS_ASSERT(0);
                            goto proc_error;
                    }
                }

                break;

            default:
                DOS_ASSERT(0);
                break;
        }
    }
    else
    {
        sc_logr_notice(SC_ESL, "SCB %d donot needs handle any playback application.", pstSCB->usSCBNo);
        sc_ep_esl_execute("hangup", NULL, pstSCB->szUUID);
    }

    sc_call_trace(pstSCB, "Finished to process %s event.", esl_event_get_header(pstEvent, "Event-Name"));

    SC_TRACE_OUT();
    return DOS_SUCC;

proc_error:

    sc_call_trace(pstSCB,"FAILED to process %s event. Call will be hangup.", esl_event_get_header(pstEvent, "Event-Name"));

    sc_ep_esl_execute("hangup", NULL, pstSCB->szUUID);

    return DOS_FAIL;
}


/**
 * ����: U32 sc_ep_session_heartbeat(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
 * ����: ����ESL��CHANNEL HEARTBEAT�¼�
 * ����:
 *      esl_handle_t *pstHandle : ���;��
 *      esl_event_t *pstEvent   : ʱ��
 *      SC_SCB_ST *pstSCB       : SCB
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 */
U32 sc_ep_session_heartbeat(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
{
    SC_TRACE_IN(pstEvent, pstHandle, pstSCB, 0);

    if (DOS_ADDR_INVALID(pstEvent)
        || DOS_ADDR_INVALID(pstHandle)
        || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_SUCC;
    }

    sc_call_trace(pstSCB, "Start process event %s.", esl_event_get_header(pstEvent, "Event-Name"));

    sc_call_trace(pstSCB, "Finished to process %s event.", esl_event_get_header(pstEvent, "Event-Name"));

    SC_TRACE_OUT();
    return DOS_SUCC;

}

/**
 * ����: U32 sc_ep_process(esl_handle_t *pstHandle, esl_event_t *pstEvent)
 * ����: �ַ�����ESL�¼�
 * ����:
 *      esl_handle_t *pstHandle : ���;��
 *      esl_event_t *pstEvent   : ʱ��
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 */
U32 sc_ep_process(esl_handle_t *pstHandle, esl_event_t *pstEvent)
{
    S8                     *pszUUID = NULL;
    SC_SCB_ST              *pstSCB = NULL;
    U32                    ulRet = DOS_FAIL;

    SC_TRACE_IN(pstEvent, pstHandle, 0, 0);

    if (DOS_ADDR_INVALID(pstEvent) || DOS_ADDR_INVALID(pstHandle))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    /* ��ȡ�¼���UUID */
    if (ESL_EVENT_BACKGROUND_JOB == pstEvent->event_id)
    {
        return sc_ep_backgroud_job_proc(pstHandle, pstEvent);
    }

    pszUUID = esl_event_get_header(pstEvent, "Caller-Unique-ID");
    if (DOS_ADDR_INVALID(pszUUID))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (ESL_EVENT_CHANNEL_CREATE != pstEvent->event_id)
    {
        pstSCB = sc_scb_hash_tables_find(pszUUID);
        if (DOS_ADDR_INVALID(pstSCB))
        {
            DOS_ASSERT(0);

            return DOS_FAIL;
        }
    }

    sc_logr_info(SC_ESL, "Start process event: %s(%d), SCB No:%d"
                    , esl_event_get_header(pstEvent, "Event-Name")
                    , pstEvent->event_id
                    , esl_event_get_header(pstEvent, "variable_scb_no"));

    switch (pstEvent->event_id)
    {
        /* ��ȡ����״̬ */
        case ESL_EVENT_CHANNEL_PARK:
            ulRet = sc_ep_channel_park_proc(pstHandle, pstEvent, pstSCB);
            if (ulRet != DOS_SUCC)
            {
                //sc_ep_esl_execute("hangup", NULL, pszUUID);
                sc_logr_info(SC_ESL, "Hangup for process event %s fail. UUID: %s", esl_event_get_header(pstEvent, "Event-Name"), pszUUID);
            }
            break;

        case ESL_EVENT_CHANNEL_CREATE:
            ulRet = sc_ep_channel_create_proc(pstHandle, pstEvent);
            if (ulRet != DOS_SUCC)
            {
                sc_ep_esl_execute("hangup", NULL, pszUUID);
                sc_logr_info(SC_ESL, "Hangup for process event %s fail. UUID: %s", esl_event_get_header(pstEvent, "Event-Name"), pszUUID);
            }
            break;

        case ESL_EVENT_CHANNEL_ANSWER:
            ulRet = sc_ep_channel_answer(pstHandle, pstEvent, pstSCB);
            break;

        case ESL_EVENT_CHANNEL_HANGUP:
            ulRet = sc_ep_channel_hungup_proc(pstHandle, pstEvent, pstSCB);
            break;

        case ESL_EVENT_CHANNEL_HANGUP_COMPLETE:
            ulRet = sc_ep_channel_hungup_complete_proc(pstHandle, pstEvent, pstSCB);
            break;

        case ESL_EVENT_DTMF:
            ulRet = sc_ep_dtmf_proc(pstHandle, pstEvent, pstSCB);
            break;

        case ESL_EVENT_PLAYBACK_STOP:
            ulRet = sc_ep_playback_stop(pstHandle, pstEvent, pstSCB);
            break;

        case ESL_EVENT_SESSION_HEARTBEAT:
            ulRet = sc_ep_session_heartbeat(pstHandle, pstEvent, pstSCB);
            break;
        default:
            DOS_ASSERT(0);
            ulRet = DOS_FAIL;

            sc_logr_info(SC_ESL, "Recv unhandled event: %s(%d)", esl_event_get_header(pstEvent, "Event-Name"), pstEvent->event_id);
            break;
    }

    sc_logr_info(SC_ESL, "Process finished event: %s(%d). Result:%s"
                    , esl_event_get_header(pstEvent, "Event-Name")
                    , pstEvent->event_id
                    , (DOS_SUCC == ulRet) ? "Successfully" : "FAIL");

    SC_TRACE_OUT();
    return ulRet;
}

/**
 * ����: VOID*sc_ep_process_runtime(VOID *ptr)
 * ����: ESL�¼��������̺߳���
 * ����:
 * ����ֵ:
 */
VOID*sc_ep_process_runtime(VOID *ptr)
{
    SC_EP_EVENT_NODE_ST *pstListNode = NULL;
    esl_event_t         *pstEvent = NULL;
    list_t              *pstListLink = NULL;
    U32                 ulRet;
    struct timespec     stTimeout;

    for (;;)
    {
        pthread_mutex_lock(&g_mutexEventList);
        stTimeout.tv_sec = time(0) + 1;
        stTimeout.tv_nsec = 0;
        pthread_cond_timedwait(&g_condEventList, &g_mutexEventList, &stTimeout);

        if (dos_list_is_empty(&g_stEventList))
        {
            pthread_mutex_unlock(&g_mutexEventList);
            continue;
        }

        pstListLink = dos_list_fetch(&g_stEventList);
        if (DOS_ADDR_INVALID(pstListLink))
        {
            pthread_mutex_unlock(&g_mutexEventList);
            continue;
        }
        pthread_mutex_unlock(&g_mutexEventList);

        pstListNode = dos_list_entry(pstListLink, SC_EP_EVENT_NODE_ST, stLink);
        if (DOS_ADDR_INVALID(pstListNode))
        {
            continue;
        }

        if (DOS_ADDR_INVALID(pstListNode->pstEvent))
        {
            dos_dmem_free(pstListNode);
            pstListNode = NULL;
            continue;
        }

        pstEvent = pstListNode->pstEvent;

        pstListNode->pstEvent = NULL;
        dos_dmem_free(pstListNode);
        pstListNode = NULL;

        sc_logr_info(SC_ESL, "ESL event process START. %s(%d), SCB No:%s, Channel Name: %s"
                        , esl_event_get_header(pstEvent, "Event-Name")
                        , pstEvent->event_id
                        , esl_event_get_header(pstEvent, "variable_scb_no")
                        , esl_event_get_header(pstEvent, "Channel-Name"));

        ulRet = sc_ep_process(&g_pstHandle->stSendHandle, pstEvent);
        if (ulRet != DOS_SUCC)
        {
            DOS_ASSERT(0);
        }

        sc_logr_info(SC_ESL, "ESL event process FINISHED. %s(%d), SCB No:%s Processed, Result: %d"
                        , esl_event_get_header(pstEvent, "Event-Name")
                        , pstEvent->event_id
                        , esl_event_get_header(pstEvent, "variable_scb_no")
                        , ulRet);

        esl_event_destroy(&pstEvent);
    }

    return NULL;
}

/**
 * ����: VOID* sc_ep_runtime(VOID *ptr)
 * ����: ESL�¼������߳�������
 * ����:
 * ����ֵ:
 */
VOID* sc_ep_runtime(VOID *ptr)
{
    U32                  ulRet = ESL_FAIL;
    S8                   *pszIsLoopbackLeg = NULL;
    S8                   *pszIsAutoCall = NULL;
    SC_EP_EVENT_NODE_ST  *pstListNode = NULL;

    for (;;)
    {
        /* ����˳���־�����ϣ���׼���˳��� */
        if (g_pstHandle->blIsWaitingExit)
        {
            sc_logr_notice(SC_ESL, "%s", "Event process exit flag has been set. the task will be exit.");
            break;
        }

        /*
         * ��������Ƿ�����
         * ������Ӳ���������׼������
         **/
        if (!g_pstHandle->blIsESLRunning)
        {
            sc_logr_notice(SC_ESL, "%s", "ELS for event connection has been down, re-connect.");
            g_pstHandle->stRecvHandle.event_lock = 1;
            ulRet = esl_connect(&g_pstHandle->stRecvHandle, "127.0.0.1", 8021, NULL, "ClueCon");
            if (ESL_SUCCESS != ulRet)
            {
                esl_disconnect(&g_pstHandle->stRecvHandle);
                sc_logr_notice(SC_ESL, "ELS for event re-connect fail, return code:%d, Msg:%s. Will be retry after 1 second.", ulRet, g_pstHandle->stRecvHandle.err);

                sleep(1);
                continue;
            }

            g_pstHandle->blIsESLRunning = DOS_TRUE;
            g_pstHandle->ulESLDebugLevel = ESL_LOG_LEVEL_INFO;
            esl_global_set_default_logger(g_pstHandle->ulESLDebugLevel);
            esl_events(&g_pstHandle->stRecvHandle, ESL_EVENT_TYPE_PLAIN, SC_EP_EVENT_LIST);

            sc_logr_notice(SC_ESL, "%s", "ELS for event connect Back to Normal.");
        }

        ulRet = esl_recv_event(&g_pstHandle->stRecvHandle, 1, NULL);
        if (ESL_FAIL == ulRet)
        {
            continue;
        }

        esl_event_t *pstEvent = g_pstHandle->stRecvHandle.last_ievent;
        if (DOS_ADDR_INVALID(pstEvent))
        {
            continue;
        }

        /* �����AUTO Call, ��Ҫ��loopback call��leg a���� */
        pszIsLoopbackLeg = esl_event_get_header(pstEvent, "variable_loopback_leg");
        pszIsAutoCall = esl_event_get_header(pstEvent, "variable_auto_call");
        if (pszIsLoopbackLeg && pszIsAutoCall
            && 0 == dos_strnicmp(pszIsLoopbackLeg, "A", dos_strlen("A"))
            && 0 == dos_strnicmp(pszIsAutoCall, "true", dos_strlen("true")))
        {
            sc_logr_info(SC_ESL, "%s", "ESL drop loopback call leg A.");
            continue;
        }

        sc_logr_info(SC_ESL, "ESL recv thread recv event %s(%d)."
                        , esl_event_get_header(pstEvent, "Event-Name")
                        , pstEvent->event_id);

        pstListNode = (SC_EP_EVENT_NODE_ST *)dos_dmem_alloc(sizeof(SC_EP_EVENT_NODE_ST));
        if (DOS_ADDR_INVALID(pstListNode))
        {
            DOS_ASSERT(0);

            sc_logr_info(SC_ESL, "ESL recv thread recv event %s(%d). Alloc memory fail. Drop"
                            , esl_event_get_header(pstEvent, "Event-Name")
                            , pstEvent->event_id);

            continue;
        }

        pthread_mutex_lock(&g_mutexEventList);

        dos_list_node_init(&pstListNode->stLink);
        esl_event_dup(&pstListNode->pstEvent, pstEvent);
        dos_list_add_tail(&g_stEventList, (list_t *)pstListNode);

        pthread_cond_signal(&g_condEventList);
        pthread_mutex_unlock(&g_mutexEventList);
    }

    /* @TODO �ͷ���Դ */
    return NULL;
}

/* ��ʼ���¼�����ģ�� */
U32 sc_ep_init()
{
    SC_TRACE_IN(0, 0, 0, 0);

    g_pstHandle = dos_dmem_alloc(sizeof(SC_EP_HANDLE_ST));
    g_pstHashGWGrp = hash_create_table(SC_GW_GRP_HASH_SIZE, NULL);
    g_pstHashGW = hash_create_table(SC_GW_HASH_SIZE, NULL);
    g_pstHashDIDNum = hash_create_table(SC_IP_DID_HASH_SIZE, NULL);
    g_pstHashSIPUserID = hash_create_table(SC_IP_USERID_HASH_SIZE, NULL);
    g_pstHashBlackList = hash_create_table(SC_BLACK_LIST_HASH_SIZE, NULL);
    if (DOS_ADDR_INVALID(g_pstHandle)
        || DOS_ADDR_INVALID(g_pstHashGW)
        || DOS_ADDR_INVALID(g_pstHashGWGrp)
        || DOS_ADDR_INVALID(g_pstHashDIDNum)
        || DOS_ADDR_INVALID(g_pstHashSIPUserID)
        || DOS_ADDR_INVALID(g_pstHashBlackList))
    {
        DOS_ASSERT(0);

        goto init_fail;
    }

    dos_memzero(g_pstHandle, sizeof(SC_EP_HANDLE_ST));
    g_pstHandle->blIsESLRunning = DOS_FALSE;
    g_pstHandle->blIsWaitingExit = DOS_FALSE;

    dos_list_init(&g_stEventList);
    DLL_Init(&g_stRouteList);

    /* �����������˳���ܸ��� */
    sc_load_gateway(SC_INVALID_INDEX);
    sc_load_gateway_grp(SC_INVALID_INDEX);
    sc_load_relationship();

    sc_load_route(SC_INVALID_INDEX);
    sc_load_did_number(SC_INVALID_INDEX);
    sc_load_sip_userid(SC_INVALID_INDEX);

    SC_TRACE_OUT();
    return DOS_SUCC;
init_fail:

    return DOS_FAIL;
}

/* �����¼�����ģ�� */
U32 sc_ep_start()
{
    SC_TRACE_IN(0, 0, 0, 0);

    if (pthread_create(&g_pstHandle->pthID, NULL, sc_ep_runtime, NULL) < 0)
    {
        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    if (pthread_create(&g_pstHandle->pthID, NULL, sc_ep_process_runtime, NULL) < 0)
    {
        SC_TRACE_OUT();
        return DOS_FAIL;
    }




    SC_TRACE_OUT();
    return DOS_SUCC;
}

/* ֹͣ�¼�����ģ�� */
U32 sc_ep_shutdown()
{
    SC_TRACE_IN(0, 0, 0, 0);

    g_pstHandle->blIsWaitingExit = DOS_TRUE;

    SC_TRACE_OUT();
    return DOS_SUCC;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */


