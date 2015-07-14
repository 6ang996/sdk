/**
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  �ļ���: sc_acd.h
 *
 *  ����ʱ��: 2014��12��16��10:15:56
 *  ��    ��: Larry
 *  ��    ��: ACDģ��˽��ͷ�ļ�
 *  �޸���ʷ:
 */


#ifndef __SC_ACD_H__
#define __SC_ACD_H__

#define SC_ACD_GROUP_NAME_LEN         24

#define SC_ACD_HASH_SIZE              128

/* 1. û�б�ɾ��
   2. �Ѿ���½��
   3. ��Ҫ���ӣ����Ҵ�������״̬
   4. ״̬ΪEDL*/
#define SC_ACD_SITE_IS_USEABLE(pstSiteDesc)                             \
    (DOS_ADDR_VALID(pstSiteDesc)                                       \
    && !(pstSiteDesc)->bWaitingDelete                                  \
    && (pstSiteDesc)->bLogin                                           \
    && SC_ACD_IDEL == (pstSiteDesc)->ucStatus                          \
    && !((pstSiteDesc)->bNeedConnected && !(pstSiteDesc)->bConnected))

typedef struct tagACDQueueNode{
    U32                     ulID;             /* ��ǰ�ڵ��ڵ�ǰ��������ı�� */

    SC_ACD_AGENT_INFO_ST   *pstAgentInfo;     /* ��ϯ��Ϣ */
}SC_ACD_AGENT_QUEUE_NODE_ST;

typedef struct tagACDMemoryNode{
    U32        ulSiteID;                            /* ��ϯ���ݿ��� */

    S8        szCallerNum[SC_TEL_NUMBER_LENGTH];    /* ���к��� */
}SC_ACD_MEMORY_RELATION_QUEUE_NODE_ST;

typedef struct tagACDQueryMngtNode
{
    U16       usID;                                /* ��ǰ��ϯ���� */
    U16       usCount;                             /* ��ǰ��ϯ����ϯ���� */
    U16       usLastUsedAgent;                     /* ��һ�νӵ绰����ϯ��� */
    U8        ucACDPolicy;                         /* ����з������ */
    U8        ucWaitingDelete;                     /* �Ƿ�ȴ�ɾ�� */

    U32       ulCustomID;                          /* ��ǰ��ϯ������ */
    U32       ulGroupID;                           /* ��ϯ�������ݿ��еı�� */
    S8        szGroupName[SC_ACD_GROUP_NAME_LEN];  /* ��ϯ���� */

    DLL_S     stAgentList;                         /* ��ϯhash�� */

    HASH_TABLE_S     *pstRelationList;             /* ���к������ϯ�Ķ�Ӧ��ϵ�б� */
    pthread_mutex_t  mutexSiteQueue;

    SC_SITE_GRP_STAT_ST stStat;
}SC_ACD_GRP_HASH_NODE_ST;

typedef struct tagACDFindSiteParam
{
    U32                  ulPolocy;
    U32                  ulLastSieUsed;
    U32                  ulResult;

    SC_ACD_AGENT_INFO_ST *pstResult;
}SC_ACD_FIND_SITE_PARAM_ST;


U32 sc_acd_agent_grp_add_call(U32 ulGrpID);
U32 sc_acd_agent_grp_del_call(U32 ulGrpID);
U32 sc_acd_agent_grp_stat(U32 ulGrpID, U32 ulWaitingTime);

#endif

