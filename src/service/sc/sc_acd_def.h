/**
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  �ļ���: sc_acd_pub.h
 *
 *  ����ʱ��: 2015��1��14��14:42:12
 *  ��    ��: Larry
 *  ��    ��: ACDģ�鹫��ͷ�ļ�
 *  �޸���ʷ:
 */

#ifndef __SC_ACD_PUB_H__
#define __SC_ACD_PUB_H__

/* ������ϯ������������ĸ��� */
#define MAX_GROUP_PER_SITE      2

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

/* IP��ϯ:
 * 1. ��ʼ��ΪOFFLINE״̬
 * 2. ��½֮��ʹ���AWAY״̬/��ϯ��æҲ����AWAY״̬
 * 3. ��ϯ���оʹ���IDEL״̬
 * 4. ��ͨ���оʹ���BUSY״̬
 */
enum {
    SC_ACD_OFFLINE = 0,                 /* ����״̬�������� */
    SC_ACD_IDEL,                        /* ��ϯ�ѿ��� */
    SC_ACD_AWAY,                        /* ��ϯ��ʱ�뿪�������� */
    SC_ACD_BUSY,                        /* ��ϯæ�������� */
    SC_ACD_PROC,                        /* ��ϯ���ڴ���ͨ����� */

    SC_ACD_BUTT
};

enum {
    SC_ACD_SITE_ACTION_DELETE = 0,       /* ��ϯ����������ɾ�� */
    SC_ACD_SITE_ACTION_ADD,              /* ��ϯ����������ɾ�� */
    SC_ACD_SITE_ACTION_UPDATE,           /* ��ϯ����������ɾ�� */

    SC_ACD_SITE_ACTION_SIGNIN,           /* ��ǩ */
    SC_ACD_SITE_ACTION_SIGNOUT,          /* �˳���ǩ */
    SC_ACD_SITE_ACTION_ONLINE,           /* ��ϯ��½��WEBҳ�� */
    SC_ACD_SITE_ACTION_OFFLINE,          /* ��ϯ��WEBҳ���˳� */
    SC_ACD_SITE_ACTION_EN_QUEUE,         /* ���� */
    SC_ACD_SITE_ACTION_DN_QUEUE,         /* ��æ */

    SC_ACD_SITE_ACTION_CONNECTED,         /* ��ϯ�����ɹ� */
    SC_ACD_SITE_ACTION_DISCONNECT,        /* ��ϯ����ʧ�� */
    SC_ACD_SITE_ACTION_CONNECT_FAIL,      /* �쳣����ʧ���ˣ��޷��ٴγ����ڽ��г����� */

    SC_API_CMD_ACTION_AGENTGREP_ADD,
    SC_API_CMD_ACTION_AGENTGREP_DELETE,
    SC_API_CMD_ACTION_AGENTGREP_UPDATE,

    SC_ACD_SITE_ACTION_BUTT              /* ��ϯǩ��(����) */
};

typedef enum tagSCACDPolicy{
    SC_ACD_POLICY_IN_ORDER,               /* ˳��ѡ�� */
    SC_ACD_POLICY_MIN_CALL,               /* ���ٺ��� */
    SC_ACD_POLICY_RANDOM,                 /* ��� */
    SC_ACD_POLICY_RECENT,                 /* ������� */
    SC_ACD_POLICY_GROUP,                  /* ȫ�� */
    SC_ACD_POLICY_MEMORY,                 /* ������� */

    SC_ACD_POLICY_BUTT
}SC_ACD_POLICY_EN;

typedef enum tagAgentBindType{
    AGENT_BIND_SIP        = 0,
    AGENT_BIND_TELE,
    AGENT_BIND_MOBILE,
    AGENT_BIND_TT_NUMBER,

    AGENT_BIND_BUTT
}SC_AGENT_BIND_TYPE_EN;

typedef struct tagACDSiteDesc{
    U16        usSCBNo;
    U8         ucStatus;                          /* ��ϯ״̬ refer to SC_SITE_STATUS_EN */
    U8         ucBindType;                        /* ��ϯ������ refer to SC_AGENT_BIND_TYPE_EN */
    U32        ulSiteID;                          /* ��ϯ���ݿ��� */
    U32        ulCallCnt;                         /* �������� */
    U32        ulCustomerID;                      /* �ͻ�id */
    U32        ulSIPUserID;                       /* SIP�˻�ID */
    U32        aulGroupID[MAX_GROUP_PER_SITE];    /* ��ID */

    U32        ulLastOnlineTime;
    U32        ulLastSignInTime;

    U32        bValid:1;                          /* �Ƿ���� */
    U32        bRecord:1;                         /* �Ƿ�¼�� */
    U32        bTraceON:1;                        /* �Ƿ���Ը��� */
    U32        bAllowAccompanying:1;              /* �Ƿ�����ֻ����� refer to SC_SITE_ACCOM_STATUS_EN */
    U32        bGroupHeader:1;                    /* �Ƿ����鳤 */

    U32        bLogin:1;                          /* �Ƿ��Ѿ���¼ */
    U32        bConnected:1;                      /* �Ƿ��Ѿ����� */
    U32        bNeedConnected:1;                  /* �Ƿ��Ѿ����� */
    U32        bWaitingDelete:1;                  /* �Ƿ��Ѿ���ɾ�� */

    U32        ucProcesingTime:8;                 /* ��ϯ������н��ʱ�� */
    U32        ucRes1:14;

    S8         szUserID[SC_TEL_NUMBER_LENGTH];    /* SIP User ID */
    S8         szExtension[SC_TEL_NUMBER_LENGTH]; /* �ֻ��� */
    S8         szEmpNo[SC_EMP_NUMBER_LENGTH];     /* ���� */
    S8         szTelePhone[SC_TEL_NUMBER_LENGTH]; /* �̻����� */
    S8         szMobile[SC_TEL_NUMBER_LENGTH];    /* �ƶ��绰 */
    S8         szTTNumber[SC_TEL_NUMBER_LENGTH];    /* TT���� */

    pthread_mutex_t  mutexLock;

    SC_SITE_STAT_ST stStat;
}SC_ACD_AGENT_INFO_ST;

U32 sc_acd_get_agent_by_grpid(SC_ACD_AGENT_INFO_ST *pstAgentInfo, U32 ulGroupID, S8 *szCallerNum);
U32 sc_acd_agent_update_status(U32 ulSiteID, U32 ulStatus, U32 ulSCBNo);
S32 sc_acd_grp_hash_find(VOID *pSymName, HASH_NODE_S *pNode);
U32 sc_acd_hash_func4grp(U32 ulGrpID, U32 *pulHashIndex);
U32 sc_acd_query_idel_agent(U32 ulAgentGrpID, BOOL *pblResult);
U32 sc_acd_update_agent_status(U32 ulAction, U32 ulAgentID);
U32 sc_acd_get_idel_agent(U32 ulGroupID);
U32 sc_acd_get_total_agent(U32 ulGroupID);
U32 sc_acd_get_agent_by_id(SC_ACD_AGENT_INFO_ST *pstAgentInfo, U32 ulAgentID);
U32 sc_acd_get_agent_by_userid(SC_ACD_AGENT_INFO_ST *pstAgentInfo, S8 *szUserID);
U32 sc_acd_update_agent_scbno_by_userid(S8 *szUserID, SC_SCB_ST *pstSCB);
U32 sc_acd_update_agent_scbno_by_siteid(U32 ulAgentID, SC_SCB_ST *pstSCB);
U32 sc_acd_agent_audit(U32 ulCycle, VOID *ptr);
U32 sc_ep_query_agent_status(CURL *curl, SC_ACD_AGENT_INFO_ST *pstAgentInfo);

#endif

