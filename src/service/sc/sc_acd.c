/**
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  �ļ���: sc_acd.c
 *
 *  ����ʱ��: 2015��1��14��14:42:07
 *  ��    ��: Larry
 *  ��    ��: ACDģ����ع��ܺ���ʵ��
 *  �޸���ʷ:
 *
 * ��ϯ�������ڴ��еĽṹ
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
#include "sc_debug.h"
#include "sc_acd_def.h"
#include "sc_acd.h"

extern DB_HANDLE_ST         *g_pstSCDBHandle;


/* ��ϯ���hash�� */
HASH_TABLE_S      *g_pstAgentList      = NULL;
pthread_mutex_t   g_mutexAgentList     = PTHREAD_MUTEX_INITIALIZER;

HASH_TABLE_S      *g_pstGroupList      = NULL;
pthread_mutex_t   g_mutexGroupList     = PTHREAD_MUTEX_INITIALIZER;

/* ��ϯ����� */
U32               g_ulGroupCount       = 0;


/*
 * ��  ��: sc_acd_hash_func4agent
 * ��  ��: ��ϯ��hash������ͨ���ֻ��ż���һ��hashֵ
 * ��  ��:
 *         S8 *pszExension  : �ֻ���
 *         U32 *pulHashIndex: �������������֮���hashֵ
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 **/
static U32 sc_acd_hash_func4agent(S8 *pszUserID, U32 *pulHashIndex)
{
    U32   ulHashVal = 0;
    U32   i         = 0;
    S8    *pszStr   = NULL;

    SC_TRACE_IN(pszUserID, pulHashIndex, 0, 0);

    if (DOS_ADDR_INVALID(pszUserID) || DOS_ADDR_INVALID(pulHashIndex))
    {
        DOS_ASSERT(0);

        pulHashIndex = 0;
        SC_TRACE_OUT();
        return DOS_FAIL;
    }


    ulHashVal = 0;

    for (i = 0; i < dos_strlen(pszStr); i ++)
    {
        ulHashVal += (ulHashVal << 5) + (U8)pszStr[i];
    }

    *pulHashIndex = ulHashVal % SC_ACD_HASH_SIZE;
    SC_TRACE_OUT();
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
static U32 sc_acd_hash_func4grp(U32 ulGrpID, U32 *pulHashIndex)
{
    if (DOS_ADDR_INVALID(pulHashIndex))
    {
        return DOS_FAIL;
    }

    SC_TRACE_IN(ulGrpID, pulHashIndex, 0, 0);

    *pulHashIndex = ulGrpID % SC_ACD_HASH_SIZE;

    SC_TRACE_OUT();
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
static S32 sc_acd_grp_hash_find(VOID *pSymName, HASH_NODE_S *pNode)
{
    SC_ACD_GRP_HASH_NODE_ST    *pstGrpHashNode = NULL;
    U32                        ulIndex         = 0;

    if (DOS_ADDR_INVALID(pNode)
        || DOS_ADDR_INVALID(pNode))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstGrpHashNode = (SC_ACD_GRP_HASH_NODE_ST  *)pNode->pHandle;
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
static S32 sc_acd_agent_hash_find(VOID *pSymName, HASH_NODE_S *pNode)
{
    SC_ACD_AGENT_QUEUE_NODE_ST   *pstAgentQueueNode = NULL;
    S8                           *pszUserID         = NULL;

    if (DOS_ADDR_INVALID(pSymName)
        || DOS_ADDR_INVALID(pNode))
    {
        return DOS_FAIL;
    }

    pstAgentQueueNode = pNode->pHandle;
    pszUserID = (S8 *)pSymName;

    if (DOS_ADDR_INVALID(pstAgentQueueNode)
        || DOS_ADDR_INVALID(pszUserID)
        || DOS_ADDR_INVALID(pstAgentQueueNode->pstAgentInfo))
    {
        return DOS_FAIL;
    }

    if (0 != dos_strnicmp(pstAgentQueueNode->pstAgentInfo->szUserID, pszUserID, sizeof(pstAgentQueueNode->pstAgentInfo->szExtension)))
    {
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/*
 * ��  ��: S32 sc_acd_site_dll_find(VOID *pSymName, DLL_NODE_S *pNode)
 * ��  ��: ��ϯ��hash���Һ���
 * ��  ��:
 *         VOID *pSymName  : ��ϯ�ֻ���
 *         DLL_NODE_S *pNode: ����ڵ�
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 **/
static S32 sc_acd_agent_dll_find(VOID *pSymName, DLL_NODE_S *pNode)
{
    SC_ACD_AGENT_QUEUE_NODE_ST   *pstAgentQueueNode = NULL;
    S8                           *pszUserID         = NULL;

    if (DOS_ADDR_INVALID(pSymName)
        || DOS_ADDR_INVALID(pNode))
    {
        return DOS_FAIL;
    }

    pstAgentQueueNode = pNode->pHandle;
    pszUserID = (S8 *)pSymName;

    if (DOS_ADDR_INVALID(pstAgentQueueNode)
        || DOS_ADDR_INVALID(pszUserID)
        || DOS_ADDR_INVALID(pstAgentQueueNode->pstAgentInfo))
    {
        return DOS_FAIL;
    }

    if (0 != dos_strnicmp(pstAgentQueueNode->pstAgentInfo->szUserID, pszUserID, sizeof(pstAgentQueueNode->pstAgentInfo->szExtension)))
    {
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/*
 * ��  ��: U32 sc_acd_agent_update_status(S8 *pszUserID, U32 ulStatus)
 * ��  ��: ������ϯ״̬
 * ��  ��:
 *      S8 *pszUserID : ��ϯ��SIP USER ID
 *      U32 ulStatus  : ��״̬
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 **/
U32 sc_acd_agent_update_status(S8 *pszUserID, U32 ulStatus)
{
    SC_ACD_AGENT_QUEUE_NODE_ST  *pstAgentQueueInfo = NULL;
    HASH_NODE_S                 *pstHashNode       = NULL;
    U32                         ulHashIndex        = 0;

    if (DOS_ADDR_INVALID(pszUserID)
        || ulStatus >= SC_ACD_BUTT)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_acd_hash_func4agent(pszUserID, &ulHashIndex);
    pstHashNode = hash_find_node(g_pstAgentList, ulHashIndex, (VOID *)pszUserID, sc_acd_agent_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstAgentQueueInfo = pstHashNode->pHandle;
    if (DOS_ADDR_INVALID(pstAgentQueueInfo->pstAgentInfo))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pthread_mutex_lock(&pstAgentQueueInfo->pstAgentInfo->mutexLock);
    pstAgentQueueInfo->pstAgentInfo->usStatus = (U16)ulStatus;
    pthread_mutex_unlock(&pstAgentQueueInfo->pstAgentInfo->mutexLock);

    return DOS_SUCC;
}

/*
 * ��  ��: U32 sc_acd_group_remove_agent(U32 ulGroupID, S8 *pszUserID)
 * ��  ��: ����ϯ�������Ƴ���ϯ
 * ��  ��:
 *       U32 ulGroupID: ��ϯ��ID
 *       S8 *pszUserID: ��ϯΨһ��ʾ sip�˻�
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 **/
U32 sc_acd_group_remove_agent(U32 ulGroupID, S8 *pszUserID)
{
    SC_ACD_AGENT_QUEUE_NODE_ST   *pstAgentQueueNode  = NULL;
    SC_ACD_GRP_HASH_NODE_ST      *pstGroupNode       = NULL;
    HASH_NODE_S                  *pstHashNode        = NULL;
    DLL_NODE_S                   *pstDLLNode         = NULL;
    U32                          ulHashVal           = 0;

    if (DOS_ADDR_INVALID(pszUserID))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    /* ������ڶ����Ƿ���� */
    sc_acd_hash_func4grp(ulGroupID, &ulHashVal);
    pthread_mutex_lock(&g_mutexGroupList);
    pstHashNode = hash_find_node(g_pstGroupList, ulHashVal , &ulGroupID, sc_acd_grp_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_ACD, "Cannot find the group \"%d\" for the site %s.", ulGroupID, pszUserID);

        pthread_mutex_unlock(&g_mutexGroupList);
        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pstGroupNode = pstHashNode->pHandle;

    pstDLLNode = dll_find(&pstGroupNode->stAgentList, (VOID *)pszUserID, sc_acd_agent_dll_find);
    if (DOS_ADDR_INVALID(pstDLLNode)
        || DOS_ADDR_INVALID(pstDLLNode->pHandle))
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_ACD, "Cannot find the agent \"%s\" in the group %d.", pszUserID, ulGroupID);

        pthread_mutex_unlock(&g_mutexGroupList);
        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    dll_delete(&pstGroupNode->stAgentList, pstDLLNode);

    pstAgentQueueNode = pstDLLNode->pHandle;
    pstDLLNode->pHandle = NULL;
    dos_dmem_free(pstDLLNode);
    pstDLLNode = NULL;

    /*
     * pstAgentQueueNode->pstAgentInfo �����Ա�����ͷţ������Ա��agent hash�����������
     */
     
    pstAgentQueueNode->pstAgentInfo = NULL;
    dos_dmem_free(pstAgentQueueNode);
    pstAgentQueueNode = NULL;

    pstGroupNode->usCount--;

    pthread_mutex_unlock(&g_mutexGroupList);

    return DOS_SUCC;
}

/*
 * ��  ��: U32 sc_acd_group_add_agent(U32 ulGroupID, SC_ACD_AGENT_INFO_ST *pstAgentInfo)
 * ��  ��: ����ϯ��ӵ���ϯ����
 * ��  ��:
 *       U32 ulGroupID: ��ϯ��ID
 *       SC_ACD_AGENT_INFO_ST *pstAgentInfo : ��ϯ����Ϣ
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 **/
U32 sc_acd_group_add_agent(U32 ulGroupID, SC_ACD_AGENT_INFO_ST *pstAgentInfo)
{
    SC_ACD_AGENT_QUEUE_NODE_ST   *pstAgentQueueNode  = NULL;
    SC_ACD_GRP_HASH_NODE_ST      *pstGroupNode       = NULL;
    HASH_NODE_S                  *pstHashNode        = NULL;
    DLL_NODE_S                   *pstDLLNode         = NULL;
    U32                          ulHashVal           = 0;

    if (DOS_ADDR_INVALID(pstAgentInfo))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    /* ������ڶ����Ƿ���� */
    sc_acd_hash_func4grp(ulGroupID, &ulHashVal);
    pthread_mutex_lock(&g_mutexGroupList);
    pstHashNode = hash_find_node(g_pstGroupList, ulHashVal , &ulGroupID, sc_acd_grp_hash_find);
    pthread_mutex_unlock(&g_mutexGroupList);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_ACD, "Cannot find the group \"%d\" for the site %s.", ulGroupID, pstAgentInfo->szUserID);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }
    pstGroupNode = pstHashNode->pHandle;

    pstDLLNode = (DLL_NODE_S *)dos_dmem_alloc(sizeof(DLL_NODE_S));
    if (DOS_ADDR_INVALID(pstDLLNode))
    {
        sc_logr_error(SC_ACD, "Add agent to group FAILED, Alloc memory for list Node fail. Agent ID: %d, Group ID:%d"
                , pstAgentInfo->ulSiteID
                , ulGroupID);
        pthread_mutex_unlock(&g_mutexGroupList);
        return DOS_FAIL;
    }

    pstAgentQueueNode = (SC_ACD_AGENT_QUEUE_NODE_ST *)dos_dmem_alloc(sizeof(SC_ACD_AGENT_QUEUE_NODE_ST));
    if (DOS_ADDR_INVALID(pstAgentQueueNode))
    {
        sc_logr_error(SC_ACD, "Add agent to group FAILED, Alloc memory fail. Agent ID: %d, Group ID:%d"
                , pstAgentInfo->ulSiteID
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

    return DOS_SUCC;
}


/*
 * ��  ��: sc_acd_add_agent
 * ��  ��: �����ϯ
 * ��  ��:
 *         SC_ACD_AGENT_INFO_ST *pstAgentInfo, ��ϯ��Ϣ����
 *         U32 ulGrpID ������
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 * !!!!!�ú���������Ҫ��ӵ���ϯ�������Ƿ���ڣ������Ὣ��ϯ��ӵ���!!!!!!
 **/
U32 sc_acd_add_agent(SC_ACD_AGENT_INFO_ST *pstAgentInfo, U32 ulGrpID)
{
    SC_ACD_AGENT_QUEUE_NODE_ST   *pstAgentQueueNode  = NULL;
    SC_ACD_AGENT_INFO_ST         *pstAgentData       = NULL;
    HASH_NODE_S                  *pstHashNode        = NULL;
    U32                          ulHashVal           = 0;

    SC_TRACE_IN(pstAgentInfo, 0, 0, 0);

    if (DOS_ADDR_INVALID(pstAgentInfo))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    /* ������ڶ����Ƿ���� */
    sc_acd_hash_func4grp(ulGrpID, &ulHashVal);
    pthread_mutex_lock(&g_mutexGroupList);
    pstHashNode = hash_find_node(g_pstGroupList, ulHashVal , &ulGrpID, sc_acd_grp_hash_find);
    pthread_mutex_unlock(&g_mutexGroupList);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_ACD, "Cannot find the group \"%d\" for the site %s.", ulGrpID, pstAgentInfo->szExtension);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pstAgentQueueNode = (SC_ACD_AGENT_QUEUE_NODE_ST *)dos_dmem_alloc(sizeof(SC_ACD_AGENT_QUEUE_NODE_ST));
    if (DOS_ADDR_INVALID(pstAgentQueueNode))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pstAgentData = (SC_ACD_AGENT_INFO_ST *)dos_dmem_alloc(sizeof(SC_ACD_AGENT_INFO_ST));
    if (DOS_ADDR_INVALID(pstAgentData))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pstHashNode = (HASH_NODE_S *)dos_dmem_alloc(sizeof(HASH_NODE_S));
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    /* ������ϯ����hash�� */
    HASH_Init_Node(pstHashNode);
    pstHashNode->pHandle = pstAgentQueueNode;
    sc_acd_hash_func4agent(pstAgentInfo->szUserID, &ulHashVal);
    pthread_mutex_lock(&g_mutexAgentList);
    hash_add_node(g_pstAgentList, pstHashNode, ulHashVal, NULL);
    pthread_mutex_unlock(&g_mutexAgentList);

    sc_logr_debug(SC_ACD, "Load Agent. ID: %d, Customer: %d, Group1: %d, Group2: %d", pstAgentInfo->ulSiteID, pstAgentInfo->ulCustomerID, pstAgentInfo->aulGroupID[0], pstAgentInfo->aulGroupID[1]);

    /* ��ӵ����� */
    dos_memcpy(pstAgentData, pstAgentInfo, sizeof(SC_ACD_AGENT_INFO_ST));
    pthread_mutex_init(&pstAgentData->mutexLock, NULL);
    pstAgentQueueNode->pstAgentInfo = pstAgentData;

    SC_TRACE_OUT();
    return DOS_SUCC;
}


/*
 * ��  ��: sc_acd_grp_wolk4delete_site
 * ��  ��: ��ĳһ����ϯ������ɾ����ϯ
 * ��  ��:
 *         HASH_NODE_S *pNode : ��ǰ�ڵ�
 *         VOID *pszExtensition : ��ɾ����ϯ�ķֻ���
 * ����ֵ: VOID
 **/
static VOID sc_acd_grp_wolk4delete_agent(HASH_NODE_S *pNode, VOID *pszUserID)
{
    SC_ACD_GRP_HASH_NODE_ST      *pstGroupListNode  = NULL;
    SC_ACD_AGENT_QUEUE_NODE_ST   *pstAgentQueueNode = NULL;
    DLL_NODE_S                   *pstDLLNode        = NULL;
    U32                          ulHashVal          = 0;

    if (DOS_ADDR_INVALID(pNode) || DOS_ADDR_INVALID(pszUserID))
    {
        return;
    }

    if (DOS_ADDR_INVALID(pNode)
        || DOS_ADDR_INVALID(pNode->pHandle))
    {
        DOS_ASSERT(0);
        return;
    }

    pstGroupListNode = (SC_ACD_GRP_HASH_NODE_ST *)pNode->pHandle;
    sc_acd_hash_func4agent(pszUserID, &ulHashVal);
    pthread_mutex_lock(&pstGroupListNode->mutexSiteQueue);
    pstDLLNode = dll_find(&pstGroupListNode->stAgentList, (VOID *)pszUserID, sc_acd_agent_dll_find);
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

U32 sc_acd_delete_agent(S8 *pszUserID)
{
    SC_ACD_AGENT_QUEUE_NODE_ST   *pstAgentQueueNode = NULL;
    HASH_NODE_S                  *pstHashNode       = NULL;
    U32                          ulHashVal          = 0;

    SC_TRACE_IN(pszUserID, 0, 0, 0);

    if (DOS_ADDR_INVALID(pszUserID))
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    /* ���������飬��ɾ�������ϯ */
    pthread_mutex_lock(&g_mutexGroupList);
    hash_walk_table(g_pstGroupList, pszUserID, sc_acd_grp_wolk4delete_agent);
    pthread_mutex_unlock(&g_mutexGroupList);

    /* ������ϯ��Ȼ����ֵΪɾ��״̬ */
    pthread_mutex_lock(&g_mutexAgentList);
    sc_acd_hash_func4agent(pszUserID, &ulHashVal);
    pstHashNode = hash_find_node(g_pstAgentList, ulHashVal, pszUserID, sc_acd_agent_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_ACD, "Connot find the Site \"%s\" while delete", pszUserID);
        pthread_mutex_unlock(&g_mutexAgentList);

        SC_TRACE_OUT();
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


U32 sc_acd_update_agent_status(U32 ulAction, U32 ulAgentID, S8 *pszUserID)
{
    SC_ACD_AGENT_QUEUE_NODE_ST   *pstAgentQueueNode = NULL;
    HASH_NODE_S                  *pstHashNode       = NULL;
    U32                          ulHashVal          = 0;

    SC_TRACE_IN(pszUserID, ulAgentID, ulAction, 0);

    if (DOS_ADDR_INVALID(pszUserID))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    if (ulAction >= SC_ACD_SITE_ACTION_BUTT)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    /* �ҵ���ϯ */
    sc_acd_hash_func4agent(pszUserID, &ulHashVal);
    pthread_mutex_lock(&g_mutexAgentList);
    pstHashNode = hash_find_node(g_pstAgentList, ulHashVal , &pszUserID, sc_acd_agent_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        pthread_mutex_unlock(&g_mutexAgentList);
        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pstAgentQueueNode = pstHashNode->pHandle;
    if (DOS_ADDR_VALID(pstAgentQueueNode)
        && DOS_ADDR_VALID(pstAgentQueueNode->pstAgentInfo))
    {
        pthread_mutex_lock(&pstAgentQueueNode->pstAgentInfo->mutexLock);
        switch (ulAction)
        {
            case SC_ACD_SITE_ACTION_DELETE:
                pstAgentQueueNode->pstAgentInfo->bWaitingDelete = DOS_TRUE;
                break;
            case SC_ACD_SITE_ACTION_ONLINE:
                pstAgentQueueNode->pstAgentInfo->bLogin = DOS_TRUE;
                pstAgentQueueNode->pstAgentInfo->bConnected = DOS_FALSE;
                break;
            case SC_ACD_SITE_ACTION_OFFLINE:
                pstAgentQueueNode->pstAgentInfo->bLogin = DOS_TRUE;
                pstAgentQueueNode->pstAgentInfo->bConnected = DOS_FALSE;
                break;
            case SC_ACD_SITE_ACTION_SIGNIN:
                pstAgentQueueNode->pstAgentInfo->bConnected = DOS_TRUE;
                pstAgentQueueNode->pstAgentInfo->bConnected = DOS_FALSE;
                break;
            case SC_ACD_SITE_ACTION_SIGNOUT:
                pstAgentQueueNode->pstAgentInfo->bConnected = DOS_FALSE;
                pstAgentQueueNode->pstAgentInfo->bConnected = DOS_FALSE;
                break;
            case SC_ACD_SITE_ACTION_EN_QUEUE:
                pstAgentQueueNode->pstAgentInfo->bConnected = DOS_TRUE;
                pstAgentQueueNode->pstAgentInfo->bConnected = DOS_TRUE;
                break;
            case SC_ACD_SITE_ACTION_DN_QUEUE:
                pstAgentQueueNode->pstAgentInfo->bConnected = DOS_TRUE;
                pstAgentQueueNode->pstAgentInfo->bConnected = DOS_FALSE;
                break;
            default:
                DOS_ASSERT(0);
                break;
        }
        pthread_mutex_unlock(&pstAgentQueueNode->pstAgentInfo->mutexLock);
    }
    pthread_mutex_unlock(&g_mutexAgentList);

    return DOS_SUCC;
}

U32 sc_acd_add_queue(U32 ulGroupID, U32 ulCustomID, U32 ulPolicy, S8 *pszGroupName)
{
    SC_ACD_GRP_HASH_NODE_ST    *pstGroupListNode = NULL;
    HASH_NODE_S                *pstHashNode      = NULL;
    U32                        ulHashVal         = 0;

    SC_TRACE_IN(ulGroupID, ulCustomID, ulPolicy, pszGroupName);

    if (DOS_ADDR_INVALID(pszGroupName))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    if (0 == ulGroupID
        || 0 == ulCustomID
        || ulPolicy >= SC_ACD_POLICY_BUTT)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    sc_logr_debug(SC_ACD, "Load Group. ID:%d, Customer:%d, Policy: %d, Name: %s", ulGroupID, ulCustomID, ulPolicy, pszGroupName);

    /* ȷ������ */
    sc_acd_hash_func4grp(ulGroupID, &ulHashVal);
    pthread_mutex_lock(&g_mutexGroupList);
    pstHashNode = hash_find_node(g_pstGroupList, ulHashVal , &ulGroupID, sc_acd_grp_hash_find);
    if (DOS_ADDR_VALID(pstHashNode)
        && DOS_ADDR_VALID(pstHashNode->pHandle))
    {
        pstGroupListNode = pstHashNode->pHandle;

        pstGroupListNode->ucACDPolicy = (U8)ulPolicy;
        pstGroupListNode->usCount = 0;
        pstGroupListNode->usLastUsedAgent = 0;

        sc_logr_error(SC_ACD, "Group \"%d\" Already in the list. Update", ulGroupID);
        pthread_mutex_unlock(&g_mutexGroupList);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }
    pthread_mutex_unlock(&g_mutexGroupList);

    pstGroupListNode = (SC_ACD_GRP_HASH_NODE_ST *)dos_dmem_alloc(sizeof(SC_ACD_GRP_HASH_NODE_ST));
    if (DOS_ADDR_INVALID(pstGroupListNode))
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_ACD, "%s", "Add group fail. Alloc memory fail");

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pstHashNode = (HASH_NODE_S *)dos_dmem_alloc(sizeof(HASH_NODE_S));
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        DOS_ASSERT(0);

        dos_dmem_free(pstHashNode);
        pstHashNode = NULL;
        sc_logr_error(SC_ACD, "%s", "Add group fail. Alloc memory for hash node fail");

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    HASH_Init_Node(pstHashNode);
    DLL_Init(&pstGroupListNode->stAgentList);
    if (DOS_ADDR_INVALID(pstGroupListNode))
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_ACD, "%s", "Add group fail. Alloc memory fail");
        dos_dmem_free(pstGroupListNode);
        pstGroupListNode = NULL;

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pthread_mutex_init(&pstGroupListNode->mutexSiteQueue, NULL);
    pstGroupListNode->ulCustomID = ulCustomID;
    pstGroupListNode->ulGroupID  = ulGroupID;
    pstGroupListNode->ucACDPolicy = (U8)ulPolicy;
    pstGroupListNode->usCount = 0;
    pstGroupListNode->usLastUsedAgent = 0;
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

    SC_TRACE_OUT();
    return DOS_SUCC;
}

U32 sc_acd_delete_queue(U32 ulGroupID)
{
    SC_ACD_GRP_HASH_NODE_ST    *pstGroupListNode = NULL;
    HASH_NODE_S                *pstHashNode      = NULL;
    U32                        ulHashVal         = 0;

    SC_TRACE_IN(ulGroupID, 0, 0, 0);

    if (0 == ulGroupID)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    /* ȷ������ */
    sc_acd_hash_func4grp(ulGroupID, &ulHashVal);
    pthread_mutex_lock(&g_mutexGroupList);
    pstHashNode = hash_find_node(g_pstGroupList, ulHashVal , &ulGroupID, sc_acd_grp_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_ACD, "Connot find the Group \"%d\".", ulGroupID);
        pthread_mutex_unlock(&g_mutexGroupList);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pstGroupListNode = pstHashNode->pHandle;
    pthread_mutex_lock(&pstGroupListNode->mutexSiteQueue);
    pstGroupListNode->ucWaitingDelete = DOS_TRUE;
    pthread_mutex_unlock(&pstGroupListNode->mutexSiteQueue);
    pthread_mutex_unlock(&g_mutexGroupList);

    SC_TRACE_OUT();
    return DOS_SUCC;
}


SC_ACD_AGENT_QUEUE_NODE_ST * sc_acd_get_agent_by_random(SC_ACD_GRP_HASH_NODE_ST *pstGroupListNode)
{
    U32     ulRandomAgent      = 0;
    SC_ACD_AGENT_QUEUE_NODE_ST *pstAgentQueueNode = NULL;
    SC_ACD_AGENT_INFO_ST       *pstAgentInfo      = NULL;
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

    sc_logr_debug(SC_ACD, "Select agent in random. Start find agent %d in group %d, count: %d."
                    , ulRandomAgent
                    , pstGroupListNode->ulGroupID
                    , pstGroupListNode->stAgentList.ulCount);

    DLL_Scan(&pstGroupListNode->stAgentList, pstDLLNode, DLL_NODE_S*)
    {
        if (DOS_ADDR_INVALID(pstDLLNode)
            || DOS_ADDR_INVALID(pstDLLNode->pHandle))
        {
            sc_logr_debug(SC_ACD, "Group List node has no data. Group: %d."
                                        , pstGroupListNode->ulGroupID);
            continue;
        }

        pstAgentQueueNode = pstDLLNode->pHandle;
        if (DOS_ADDR_INVALID(pstAgentQueueNode)
            || DOS_ADDR_INVALID(pstAgentQueueNode->pstAgentInfo))
        {
            sc_logr_debug(SC_ACD, "Group List node has no data. Maybe the data has been deleted. Group: %d."
                            , pstGroupListNode->ulGroupID);
            continue;
        }

        if (pstAgentQueueNode->ulID <= ulRandomAgent)
        {
            sc_logr_debug(SC_ACD, "Found an agent. But the agent's order(%d) is less then last agent order(%d). coutinue.(Agent %d in Group %d)"
                            , pstAgentQueueNode->ulID
                            , ulRandomAgent
                            , pstAgentQueueNode->pstAgentInfo->ulSiteID
                            , pstGroupListNode->ulGroupID);
            continue;
        }

        if (!SC_ACD_SITE_IS_USEABLE(pstAgentQueueNode->pstAgentInfo))
        {
            sc_logr_debug(SC_ACD, "There found an agent. But the agent is not useable. coutinue.(Agent %d in Group %d)"
                            , pstAgentQueueNode->pstAgentInfo->ulSiteID
                            , pstGroupListNode->ulGroupID);

            continue;
        }

        sc_logr_notice(SC_ACD, "Found an uaeable agent.(Agent %d in Group %d)"
                        , pstAgentInfo->ulSiteID
                        , pstGroupListNode->ulGroupID);

        pstAgentInfo = pstAgentQueueNode->pstAgentInfo;
        break;
    }

    if (DOS_ADDR_INVALID(pstAgentInfo))
    {
        sc_logr_debug(SC_ACD, "Select agent in random form header. Start find agent %d in group %d, count: %d."
                        , ulRandomAgent
                        , pstGroupListNode->ulGroupID
                        , pstGroupListNode->stAgentList.ulCount);

        DLL_Scan(&pstGroupListNode->stAgentList, pstDLLNode, DLL_NODE_S*)
        {
            if (DOS_ADDR_INVALID(pstDLLNode)
                || DOS_ADDR_INVALID(pstDLLNode->pHandle))
            {
                sc_logr_debug(SC_ACD, "Group List node has no data. Group: %d."
                                            , pstGroupListNode->ulGroupID);
                continue;
            }

            pstAgentQueueNode = pstDLLNode->pHandle;
            if (DOS_ADDR_INVALID(pstAgentQueueNode)
                || DOS_ADDR_INVALID(pstAgentQueueNode->pstAgentInfo))
            {
                sc_logr_debug(SC_ACD, "Group List node has no data. Maybe the data has been deleted. Group: %d."
                                , pstGroupListNode->ulGroupID);
                continue;
            }

            /* �嵽�����Ѿ����ҹ����еĵ���ϯ�� */
            if (pstAgentQueueNode->ulID >= ulRandomAgent)
            {
                sc_logr_debug(SC_ACD, "The end of the select loop.(Group %d)"
                                , pstGroupListNode->ulGroupID);
                break;
            }

            if (!SC_ACD_SITE_IS_USEABLE(pstAgentQueueNode->pstAgentInfo))
            {
                sc_logr_debug(SC_ACD, "There found an agent. But the agent is not useable. coutinue.(Agent %d in Group %d)"
                                , pstAgentQueueNode->pstAgentInfo->ulSiteID
                                , pstGroupListNode->ulGroupID);
                continue;
            }

            pstAgentInfo = pstAgentQueueNode->pstAgentInfo;
            sc_logr_notice(SC_ACD, "Found an uaeable agent.(Agent %d in Group %d)"
                            , pstAgentInfo->ulSiteID
                            , pstGroupListNode->ulGroupID);

            break;
        }
    }

    if (DOS_ADDR_INVALID(pstAgentInfo))
    {
        return NULL;
    }

    return pstAgentQueueNode;
}

SC_ACD_AGENT_QUEUE_NODE_ST * sc_acd_get_agent_by_inorder(SC_ACD_GRP_HASH_NODE_ST *pstGroupListNode)
{
    U32     usLastUsedAgent      = 0;
    SC_ACD_AGENT_QUEUE_NODE_ST *pstAgentQueueNode = NULL;
    SC_ACD_AGENT_INFO_ST       *pstAgentInfo      = NULL;
    DLL_NODE_S                 *pstDLLNode        = NULL;

    if (DOS_ADDR_INVALID(pstGroupListNode))
    {
        DOS_ASSERT(0);
        return NULL;
    }

    /*
     * ���ϴ�ʹ�õı�ſ�ʼ����һ�����õ���ϯ���������β�˻�û���ҵ����ٴ�ͷ��
     */

    usLastUsedAgent = pstGroupListNode->usLastUsedAgent;

    sc_logr_debug(SC_ACD, "Select agent in order. Start find agent %d in group %d, Count : %d"
                    , usLastUsedAgent
                    , pstGroupListNode->ulGroupID
                    , pstGroupListNode->stAgentList.ulCount);

    DLL_Scan(&pstGroupListNode->stAgentList, pstDLLNode, DLL_NODE_S*)
    {
        if (DOS_ADDR_INVALID(pstDLLNode)
            || DOS_ADDR_INVALID(pstDLLNode->pHandle))
        {
            sc_logr_debug(SC_ACD, "Group List node has no data. Group: %d."
                            , pstGroupListNode->ulGroupID);
            continue;
        }

        pstAgentQueueNode = pstDLLNode->pHandle;
        if (DOS_ADDR_INVALID(pstAgentQueueNode)
            || DOS_ADDR_INVALID(pstAgentQueueNode->pstAgentInfo))
        {
            sc_logr_debug(SC_ACD, "Group List node has no data. Maybe the data has been deleted. Group: %d."
                            , pstGroupListNode->ulGroupID);
            continue;
        }

        if (pstAgentQueueNode->ulID <= usLastUsedAgent)
        {
            sc_logr_debug(SC_ACD, "Found an agent. But the agent's order(%d) is less then last agent order(%d). coutinue.(Agent %d in Group %d)"
                            , pstAgentQueueNode->ulID
                            , usLastUsedAgent
                            , pstAgentQueueNode->pstAgentInfo->ulSiteID
                            , pstGroupListNode->ulGroupID);

            continue;
        }

        if (!SC_ACD_SITE_IS_USEABLE(pstAgentQueueNode->pstAgentInfo))
        {

            sc_logr_debug(SC_ACD, "There found an agent. But the agent is not useable. coutinue.(Agent %d in Group %d)"
                            , pstAgentQueueNode->pstAgentInfo->ulSiteID
                            , pstGroupListNode->ulGroupID);
            continue;
        }

        pstAgentInfo = pstAgentQueueNode->pstAgentInfo;
        sc_logr_notice(SC_ACD, "Found an uaeable agent.(Agent %d in Group %d)"
                        , pstAgentInfo->ulSiteID
                        , pstGroupListNode->ulGroupID);

        break;
    }

    if (DOS_ADDR_INVALID(pstAgentInfo))
    {
        sc_logr_debug(SC_ACD, "Select agent in order from head. Start find agent %d in group %d, Count : %d"
                        , usLastUsedAgent
                        , pstGroupListNode->ulGroupID
                        , pstGroupListNode->stAgentList.ulCount);

        DLL_Scan(&pstGroupListNode->stAgentList, pstDLLNode, DLL_NODE_S*)
        {
            if (DOS_ADDR_INVALID(pstDLLNode)
                || DOS_ADDR_INVALID(pstDLLNode->pHandle))
            {
                sc_logr_debug(SC_ACD, "Group List node has no data. Maybe the data has been deleted. Group: %d."
                                , pstGroupListNode->ulGroupID);
                continue;
            }

            pstAgentQueueNode = pstDLLNode->pHandle;
            if (DOS_ADDR_INVALID(pstAgentQueueNode)
                || DOS_ADDR_INVALID(pstAgentQueueNode->pstAgentInfo))
            {
                sc_logr_debug(SC_ACD, "Group List node has no data. Maybe the data has been deleted. Group: %d."
                                , pstGroupListNode->ulGroupID);
                continue;
            }

            /* �嵽�����Ѿ����ҹ����еĵ���ϯ�� */
            if (pstAgentQueueNode->ulID >= usLastUsedAgent)
            {
                sc_logr_debug(SC_ACD, "The end of the select loop.(Group %d)"
                                , pstGroupListNode->ulGroupID);
                break;
            }

            if (!SC_ACD_SITE_IS_USEABLE(pstAgentQueueNode->pstAgentInfo))
            {
                sc_logr_debug(SC_ACD, "There found an agent. But the agent is not useable. coutinue.(Agent %d in Group %d)"
                            , pstAgentQueueNode->pstAgentInfo->ulSiteID
                            , pstGroupListNode->ulGroupID);
                continue;
            }

            pstAgentInfo = pstAgentQueueNode->pstAgentInfo;
            sc_logr_notice(SC_ACD, "Found an uaeable agent.(Agent %d in Group %d)"
                            , pstAgentInfo->ulSiteID
                            , pstGroupListNode->ulGroupID);
            break;
        }
    }

    if (DOS_ADDR_INVALID(pstAgentInfo))
    {
        return NULL;
    }

    return pstAgentQueueNode;
}

SC_ACD_AGENT_QUEUE_NODE_ST * sc_acd_get_agent_by_call_count(SC_ACD_GRP_HASH_NODE_ST *pstGroupListNode)
{
    SC_ACD_AGENT_QUEUE_NODE_ST *pstAgentQueueNode = NULL;
    SC_ACD_AGENT_QUEUE_NODE_ST *pstAgentNode      = NULL;
    DLL_NODE_S                 *pstDLLNode        = NULL;

    if (DOS_ADDR_INVALID(pstGroupListNode))
    {
        DOS_ASSERT(0);
        return NULL;
    }

    sc_logr_debug(SC_ACD, "Select agent by the min call count. Start find agent in group %d."
                    , pstGroupListNode->ulGroupID);

    DLL_Scan(&pstGroupListNode->stAgentList, pstDLLNode, DLL_NODE_S*)
    {
        if (DOS_ADDR_INVALID(pstDLLNode)
            || DOS_ADDR_INVALID(pstDLLNode->pHandle))
        {
            sc_logr_debug(SC_ACD, "Group List node has no data. Maybe the data has been deleted. Group: %d."
                            , pstGroupListNode->ulGroupID);
            continue;
        }

        pstAgentQueueNode = pstDLLNode->pHandle;
        if (DOS_ADDR_INVALID(pstAgentQueueNode)
            || DOS_ADDR_INVALID(pstAgentQueueNode->pstAgentInfo))
        {
            sc_logr_debug(SC_ACD, "Group List node has no data. Maybe the data has been deleted. Group: %d."
                            , pstGroupListNode->ulGroupID);
            continue;
        }

        if (!SC_ACD_SITE_IS_USEABLE(pstAgentQueueNode->pstAgentInfo))
        {
            sc_logr_debug(SC_ACD, "There found an agent. But the agent is not useable. coutinue.(Agent %d in Group %d)"
                        , pstAgentQueueNode->pstAgentInfo->ulSiteID
                        , pstGroupListNode->ulGroupID);
            continue;
        }

        if (DOS_ADDR_INVALID(pstAgentNode))
        {
            pstAgentNode = pstAgentQueueNode;
        }

        sc_logr_notice(SC_ACD, "Found an uaeable agent. Call Count: %d. (Agent %d in Group %d)"
                        , pstAgentQueueNode->pstAgentInfo->ulCallCnt
                        , pstAgentQueueNode->pstAgentInfo->ulSiteID
                        , pstGroupListNode->ulGroupID);
        if (pstAgentNode->pstAgentInfo->ulCallCnt > pstAgentQueueNode->pstAgentInfo->ulCallCnt)
        {
            pstAgentNode = pstAgentQueueNode;
        }
    }

    return pstAgentNode;
}

U32 sc_acd_get_agent_by_grpid(SC_ACD_AGENT_INFO_ST *pstAgentBuff, U32 ulGroupID)
{
    SC_ACD_AGENT_QUEUE_NODE_ST *pstAgentNode      = NULL;
    SC_ACD_GRP_HASH_NODE_ST    *pstGroupListNode  = NULL;
    HASH_NODE_S                *pstHashNode       = NULL;
    U32                        ulHashVal          = 0;
    U32                        ulResult           = DOS_SUCC;

    if (DOS_ADDR_INVALID(pstAgentBuff)
        || 0 == ulGroupID
        || U32_BUTT == ulGroupID)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_acd_hash_func4grp(ulGroupID, &ulHashVal);
    pthread_mutex_lock(&g_mutexGroupList);
    pstHashNode = hash_find_node(g_pstGroupList, ulHashVal , &ulGroupID, sc_acd_grp_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_ACD, "Cannot fine the group with the ID \"%s\".", ulGroupID);
        pthread_mutex_unlock(&g_mutexGroupList);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pstGroupListNode = pstHashNode->pHandle;

    pthread_mutex_lock(&pstGroupListNode->mutexSiteQueue);

    switch (pstGroupListNode->ucACDPolicy)
    {
        case SC_ACD_POLICY_IN_ORDER:
            pstAgentNode = sc_acd_get_agent_by_inorder(pstGroupListNode);
            break;

        case SC_ACD_POLICY_MIN_CALL:
            pstAgentNode = sc_acd_get_agent_by_call_count(pstGroupListNode);
            break;

        case SC_ACD_POLICY_RANDOM:
            pstAgentNode = sc_acd_get_agent_by_random(pstGroupListNode);
            break;

        case SC_ACD_POLICY_RECENT:
        case SC_ACD_POLICY_GROUP:
            sc_logr_notice(SC_ACD, "Template not support policy %d", pstGroupListNode->ucACDPolicy);
            break;
        default:
            break;
    }

    pthread_mutex_unlock(&pstGroupListNode->mutexSiteQueue);

    if (DOS_ADDR_VALID(pstAgentNode)
        && DOS_ADDR_VALID(pstAgentNode->pstAgentInfo))
    {
        pstGroupListNode->usLastUsedAgent = pstAgentNode->ulID;
        pstAgentNode->pstAgentInfo->ulCallCnt++;

        dos_memcpy(pstAgentBuff, pstAgentNode->pstAgentInfo, sizeof(SC_ACD_AGENT_INFO_ST));
        ulResult = DOS_SUCC;
    }
    else
    {
        dos_memzero(pstAgentBuff, sizeof(SC_ACD_AGENT_INFO_ST));
        ulResult = DOS_FAIL;
    }
    pthread_mutex_unlock(&g_mutexGroupList);

    return ulResult;
}

static S32 sc_acd_init_agent_queue_cb(VOID *PTR, S32 lCount, S8 **pszData, S8 **pszField)
{
    SC_ACD_AGENT_QUEUE_NODE_ST  *pstAgentQueueNode  = NULL;
    HASH_NODE_S                 *pstHashNode = NULL;
    S8                          *pszSiteID     = NULL, *pszCustomID = NULL;
    S8                          *pszGroupID1   = NULL, *pszGroupID2 = NULL;
    S8                          *pszExten      = NULL, *pszGroupID  = NULL;
    S8                          *pszJobNum     = NULL, *pszUserID   = NULL;
    S8                          *pszRecordFlag = NULL, *pszIsHeader = NULL;
    SC_ACD_AGENT_INFO_ST        stSiteInfo;
    U32                         ulSiteID   = 0, ulCustomID   = 0, ulGroupID  = 0;
    U32                         ulGroupID1 = 0, ulRecordFlag = 0, ulIsHeader = 0;
    U32                         ulHashIndex = 0, ulIndex = 0, ulRest = 0;

    if (DOS_ADDR_INVALID(pszData)
        || DOS_ADDR_INVALID(pszField))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pszSiteID = pszData[0];
    pszCustomID = pszData[1];
    pszJobNum = pszData[2];
    pszUserID = pszData[3];
    pszExten = pszData[4];
    pszGroupID1 = pszData[5];
    pszGroupID2 = pszData[6];
    pszGroupID = pszData[7];
    pszRecordFlag = pszData[8];
    pszIsHeader = pszData[9];

    if (DOS_ADDR_INVALID(pszSiteID)
        || DOS_ADDR_INVALID(pszCustomID)
        || DOS_ADDR_INVALID(pszJobNum)
        || DOS_ADDR_INVALID(pszUserID)
        || DOS_ADDR_INVALID(pszExten)
        || DOS_ADDR_INVALID(pszGroupID)
        || DOS_ADDR_INVALID(pszRecordFlag)
        || DOS_ADDR_INVALID(pszIsHeader)
        || dos_atoul(pszSiteID, &ulSiteID) < 0
        || dos_atoul(pszCustomID, &ulCustomID) < 0
        || dos_atoul(pszGroupID1, &ulGroupID) < 0
        || dos_atoul(pszGroupID2, &ulGroupID1) < 0
        || dos_atoul(pszRecordFlag, &ulRecordFlag) < 0
        || dos_atoul(pszIsHeader, &ulIsHeader) < 0)
    {
        return 0;
    }

    dos_memzero(&stSiteInfo, sizeof(stSiteInfo));
    stSiteInfo.ulSiteID = ulSiteID;
    stSiteInfo.ulCustomerID = ulCustomID;
    stSiteInfo.aulGroupID[0] = ulGroupID;
    stSiteInfo.aulGroupID[1] = ulGroupID1;
    stSiteInfo.bValid = DOS_TRUE;
    stSiteInfo.usStatus = SC_ACD_OFFLINE;
    stSiteInfo.bRecord = ulRecordFlag;
    stSiteInfo.bGroupHeader = ulIsHeader;

    dos_strncpy(stSiteInfo.szUserID, pszUserID, sizeof(stSiteInfo.szUserID));
    stSiteInfo.szUserID[sizeof(stSiteInfo.szUserID) - 1] = '\0';
    dos_strncpy(stSiteInfo.szExtension, pszExten, sizeof(stSiteInfo.szExtension));
    stSiteInfo.szExtension[sizeof(stSiteInfo.szExtension) - 1] = '\0';
    dos_strncpy(stSiteInfo.szEmpNo, pszExten, sizeof(stSiteInfo.szEmpNo));
    stSiteInfo.szEmpNo[sizeof(stSiteInfo.szEmpNo) - 1] = '\0';
    pthread_mutex_init(&stSiteInfo.mutexLock, NULL);

    /* �鿴��ǰҪ��ӵ���ϯ�Ƿ��Ѿ����ڣ�������ڣ���׼�����¾ͺ� */
    sc_acd_hash_func4agent(stSiteInfo.szUserID, &ulHashIndex);
    pthread_mutex_lock(&g_mutexAgentList);
    pstHashNode = hash_find_node(g_pstAgentList, ulHashIndex , &stSiteInfo.szUserID, sc_acd_agent_hash_find);
    if (DOS_ADDR_VALID(pstHashNode)
        && DOS_ADDR_VALID(pstHashNode->pHandle))
    {
        sc_logr_info(SC_ACD, "Agent \"%d\" exist. Update", stSiteInfo.ulSiteID);

        pstAgentQueueNode = pstHashNode->pHandle;
        if (pstAgentQueueNode->pstAgentInfo)
        {
            /* ������ϯ��û��ȥ�˱���飬����ǣ�����Ҫ����ϯ��������� */
            for (ulIndex=0; ulIndex<MAX_GROUP_PER_SITE; ulIndex++)
            {
                if (pstAgentQueueNode->pstAgentInfo->aulGroupID[ulIndex] != U32_BUTT
                    && pstAgentQueueNode->pstAgentInfo->aulGroupID[ulIndex] != 0)
                {
                    /* �޸�֮ǰ��ID�Ϸ����޸�֮����ID�Ϸ�������Ҫ����ǰ������ID�Ƿ���ͬ����ͬ�Ͳ���ʲô�� */
                    if (stSiteInfo.aulGroupID[ulIndex] != U32_BUTT
                        && stSiteInfo.aulGroupID[ulIndex] != 0)
                    {
                        if (pstAgentQueueNode->pstAgentInfo->aulGroupID[ulIndex] != stSiteInfo.aulGroupID[ulIndex])
                        {
                            /* �ӱ����ɾ�� */
                            ulRest = sc_acd_group_remove_agent(pstAgentQueueNode->pstAgentInfo->aulGroupID[ulIndex]
                                                                , pstAgentQueueNode->pstAgentInfo->szUserID);
                            if (DOS_SUCC == ulRest)
                            {
                                /* ��ӵ��µ��� */
                                sc_acd_group_add_agent(stSiteInfo.aulGroupID[ulIndex], pstAgentQueueNode->pstAgentInfo);

                                pstAgentQueueNode->pstAgentInfo->aulGroupID[ulIndex] = stSiteInfo.aulGroupID[ulIndex];
                            }
                        }
                    }
                    /* �޸�֮ǰ��ID�Ϸ����޸�֮����ID���Ϸ�������Ҫ��agent��֮ǰ��������ɾ���� */
                    else
                    {
                        sc_acd_group_remove_agent(pstAgentQueueNode->pstAgentInfo->aulGroupID[ulIndex]
                                                    , pstAgentQueueNode->pstAgentInfo->szUserID);
                    }
                }
                else
                {
                    /* �޸�֮ǰ��ID���Ϸ����޸�֮����ID�Ϸ�������Ҫ��agent��ӵ������õ��� */
                    if (stSiteInfo.aulGroupID[ulIndex] != U32_BUTT
                        && stSiteInfo.aulGroupID[ulIndex] != 0)
                    {
                        /* ��ӵ��µ��� */
                        sc_acd_group_add_agent(stSiteInfo.aulGroupID[ulIndex], pstAgentQueueNode->pstAgentInfo);
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

            dos_strncpy(pstAgentQueueNode->pstAgentInfo->szEmpNo, stSiteInfo.szEmpNo,SC_EMP_NUMBER_LENGTH);
            pstAgentQueueNode->pstAgentInfo->szEmpNo[SC_EMP_NUMBER_LENGTH - 1] = '\0';
            dos_strncpy(pstAgentQueueNode->pstAgentInfo->szExtension, stSiteInfo.szExtension,SC_TEL_NUMBER_LENGTH);
            pstAgentQueueNode->pstAgentInfo->szExtension[SC_TEL_NUMBER_LENGTH - 1] = '\0';
            dos_strncpy(pstAgentQueueNode->pstAgentInfo->szUserID, stSiteInfo.szUserID,SC_TEL_NUMBER_LENGTH);
            pstAgentQueueNode->pstAgentInfo->szUserID[SC_TEL_NUMBER_LENGTH - 1] = '\0';
        }


        SC_TRACE_OUT();
        pthread_mutex_unlock(&g_mutexAgentList);
        return 0;
    }
    pthread_mutex_unlock(&g_mutexAgentList);

    return sc_acd_add_agent(&stSiteInfo, ulGroupID);
}

static U32 sc_acd_init_agent_queue(U32 ulIndex)
{
    S8 szSQL[1024] = { 0, };

    if (SC_INVALID_INDEX == ulIndex)
    {
        dos_snprintf(szSQL, sizeof(szSQL)
                    ,"SELECT " \
                     "    a.id, a.customer_id, a.job_number, a.username, a.extension, a.group1_id, a.group2_id, b.id, a.voice_record, a.class class " \
                     "FROM " \
                     "    (SELECT " \
                     "         tbl_agent.id id, tbl_agent.customer_id customer_id, tbl_agent.job_number job_number, " \
                     "         tbl_agent.group1_id group1_id, tbl_agent.group2_id group2_id, tbl_sip.extension extension, " \
                     "         tbl_sip.username username, tbl_agent.voice_record voice_record, tbl_agent.class class" \
                     "     FROM " \
                     "         tbl_agent, tbl_sip " \
                     "     WHERE tbl_agent.sip_id = tbl_sip.id and tbl_sip.status = 1) a " \
                     "LEFT JOIN " \
                     "    tbl_group b " \
                     "ON " \
                     "    a.group1_id = b.id OR a.group2_id = b.id;");
    }
    else
    {
        dos_snprintf(szSQL, sizeof(szSQL)
                   , "SELECT " \
                     "    a.id, a.customer_id, a.job_number, a.username, a.extension, a.group1_id, a.group2_id, b.id, a.voice_record, a.class class " \
                     "FROM " \
                     "    (SELECT " \
                     "         tbl_agent.id id, tbl_agent.customer_id customer_id, tbl_agent.job_number job_number, " \
                     "         tbl_agent.group1_id group1_id, tbl_agent.group2_id group2_id, tbl_sip.extension extension, " \
                     "         tbl_sip.username username, tbl_agent.voice_record voice_record, tbl_agent.class class" \
                     "     FROM " \
                     "         tbl_agent, tbl_sip " \
                     "     WHERE tbl_agent.sip_id = tbl_sip.id and tbl_sip.status = 1 AND tbl_agent.id = %d) a " \
                     "LEFT JOIN " \
                     "    tbl_group b " \
                     "ON " \
                     "    a.group1_id = b.id OR a.group2_id = b.id;"
                   , ulIndex);
    }

    if (db_query(g_pstSCDBHandle, szSQL, sc_acd_init_agent_queue_cb, NULL, NULL) < 0)
    {
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

static S32 sc_acd_init_group_queue_cb(VOID *PTR, S32 lCount, S8 **pszData, S8 **pszField)
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

    return sc_acd_add_queue(ulGroupID, ulCustomID, ulPolicy,pszGroupName);
}

static U32 sc_acd_init_group_queue(U32 ulIndex)
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

    if (db_query(g_pstSCDBHandle, szSql, sc_acd_init_group_queue_cb, NULL, NULL) < 0)
    {
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

static U32 sc_acd_deinit_agent_queue()
{
    return DOS_SUCC;
}

static U32 sc_acd_deinit_group_queue()
{
    return DOS_SUCC;
}

static VOID sc_acd_agent_wolk4init(HASH_NODE_S *pNode, VOID *pParam)
{
    SC_ACD_AGENT_QUEUE_NODE_ST  *pstAgentQueueNode    = NULL;
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
            sc_acd_group_add_agent(pstAgentQueueNode->pstAgentInfo->aulGroupID[ulIndex], pstAgentQueueNode->pstAgentInfo);
        }
    }
}

static U32 sc_acd_init_relationship()
{
    HASH_NODE_S     *pstHashNode = NULL;
    U32             ulHashIndex  = 0;

    SC_TRACE_IN(0, 0, 0, 0);

    HASH_Scan_Table(g_pstAgentList, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstAgentList, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            sc_acd_agent_wolk4init(pstHashNode, NULL);
        }
    }

    SC_TRACE_OUT();
    return DOS_SUCC;
}

U32 sc_acd_init()
{
    SC_TRACE_IN(0, 0, 0, 0);

    g_pstGroupList = hash_create_table(SC_ACD_HASH_SIZE, NULL);
    if (DOS_ADDR_INVALID(g_pstGroupList))
    {
        DOS_ASSERT(0);
        sc_logr_error(SC_ACD, "%s", "Init Group Hash Table Fail.");

        SC_TRACE_OUT();
        return DOS_FAIL;
    }
    g_pstGroupList->NodeNum = 0;

    g_pstAgentList = hash_create_table(SC_ACD_HASH_SIZE, NULL);
    if (DOS_ADDR_INVALID(g_pstAgentList))
    {
        DOS_ASSERT(0);
        sc_logr_error(SC_ACD, "%s", "Init Site Hash Table Fail.");

        hash_delete_table(g_pstGroupList, NULL);
        g_pstGroupList = NULL;

        SC_TRACE_OUT();
        return DOS_FAIL;
    }
    g_pstAgentList->NodeNum = 0;

    if (sc_acd_init_group_queue(SC_INVALID_INDEX) != DOS_SUCC)
    {
        DOS_ASSERT(0);
        sc_logr_error(SC_ACD, "%s", "Init group list fail in ACD.");

        hash_delete_table(g_pstAgentList, NULL);
        g_pstAgentList = NULL;

        hash_delete_table(g_pstGroupList, NULL);
        g_pstGroupList = NULL;


        SC_TRACE_OUT();
        return DOS_FAIL;
    }
    sc_logr_info(SC_ACD, "Init group list finished. Load %d agent group(s).", g_pstGroupList->NodeNum);


    if (sc_acd_init_agent_queue(SC_INVALID_INDEX) != DOS_SUCC)
    {
        DOS_ASSERT(0);
        sc_logr_error(SC_ACD, "%s", "Init sites list fail in ACD.");

        sc_acd_deinit_agent_queue();

        hash_delete_table(g_pstAgentList, NULL);
        g_pstAgentList = NULL;

        hash_delete_table(g_pstGroupList, NULL);
        g_pstGroupList = NULL;

        SC_TRACE_OUT();
        return DOS_FAIL;
    }
    sc_logr_info(SC_ACD, "Init agent list finished. Load %d agent(s).", g_pstAgentList->NodeNum);

    if (sc_acd_init_relationship() != DOS_SUCC)
    {
        DOS_ASSERT(0);
        sc_logr_error(SC_ACD, "%s", "Init ACD Data FAIL.");

        sc_acd_deinit_group_queue();
        sc_acd_deinit_agent_queue();

        hash_delete_table(g_pstAgentList, NULL);
        g_pstAgentList = NULL;

        hash_delete_table(g_pstGroupList, NULL);
        g_pstGroupList = NULL;

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/**
 * ����: U32 sc_acd_http_req_proc(U32 ulAction, U32 ulAgentID, S8 *pszUserID)
 * ����: ����HTTP������������
 * ����:
 *      U32 ulAction : ����
 *      U32 ulAgentID : ��ϯID
 *      S8 *pszUserID : ��ϯSIP User ID
 * ����ֵ: �ɹ�����DOS_SUCC,���򷵻�DOS_FAIL
 **/
U32 sc_acd_http_req_proc(U32 ulAction, U32 ulAgentID, S8 *pszUserID)
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
            sc_acd_update_agent_status(ulAction, ulAgentID, pszUserID);
            break;
        case SC_ACD_SITE_ACTION_ADD:
        case SC_ACD_SITE_ACTION_UPDATE:
            sc_acd_init_agent_queue(ulAgentID);
            break;
        default:
            DOS_ASSERT(0);
            return DOS_FAIL;
            break;
    }

    return DOS_SUCC;
}


