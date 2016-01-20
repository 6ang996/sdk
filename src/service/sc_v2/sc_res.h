/**
 * @file : sc_res.h
 *
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 * ������Դ���ݶ���
 *
 * @date: 2016��1��14��
 * @arthur: Larry
 */

#ifdef __cplusplus
extern "C" {
#endif /* End of __cplusplus */

#ifndef __SC_RES_H__
#define __SC_RES_H__

/** �����ⲿ���� */
extern HASH_TABLE_S             *g_pstHashSIPUserID;
extern pthread_mutex_t          g_mutexHashSIPUserID;
extern HASH_TABLE_S             *g_pstHashDIDNum;
extern pthread_mutex_t          g_mutexHashDIDNum;
extern DLL_S                    g_stRouteList;
extern pthread_mutex_t          g_mutexRouteList;
extern DLL_S                    g_stCustomerList;
extern pthread_mutex_t          g_mutexCustomerList;
extern HASH_TABLE_S             *g_pstHashGW;
extern pthread_mutex_t          g_mutexHashGW;
extern HASH_TABLE_S             *g_pstHashGWGrp;
extern pthread_mutex_t          g_mutexHashGWGrp;
extern DLL_S                    g_stNumTransformList;
extern pthread_mutex_t          g_mutexNumTransformList;
extern HASH_TABLE_S             *g_pstHashBlackList;
extern pthread_mutex_t          g_mutexHashBlackList;
extern HASH_TABLE_S             *g_pstHashCaller;
extern pthread_mutex_t          g_mutexHashCaller;
extern HASH_TABLE_S             *g_pstHashCallerGrp;
extern pthread_mutex_t          g_mutexHashCallerGrp;
extern HASH_TABLE_S             *g_pstHashTTNumber;
extern pthread_mutex_t          g_mutexHashTTNumber;
extern HASH_TABLE_S             *g_pstHashNumberlmt;
extern pthread_mutex_t          g_mutexHashNumberlmt;
extern HASH_TABLE_S             *g_pstHashServCtrl;
extern pthread_mutex_t          g_mutexHashServCtrl;
extern HASH_TABLE_S             *g_pstHashCallerSetting;
extern pthread_mutex_t          g_mutexHashCallerSetting;


#define SC_INVALID_INDEX       U32_BUTT

#define SC_STRING_HASH_LIMIT     20

#define SC_SIP_ACCOUNT_HASH_SIZE 512
#define SC_DID_HASH_SIZE         512

/* ·�� �м����������� */
#define SC_ROUT_GW_GRP_MAX_SIZE  5

#define SC_GW_DOMAIN_LEG        32

#define SC_GW_GRP_HASH_SIZE     128

#define SC_GW_HASH_SIZE         128
#define SC_BLACK_LIST_HASH_SIZE 1024

/** �������к����hash���С */
#define SC_CALLER_HASH_SIZE     128

/* �������к������hash���С */
#define SC_CALLER_GRP_HASH_SIZE 128

/* TT�Ŷ�Ӧ��ϵ */
#define SC_EIX_DEV_HASH_SIZE   128

/* SCB hash������� */
#define SC_MAX_SCB_HASH_NUM            4096

/* ҵ�����HASH�� */
#define SC_SERV_CTRL_HASH_SIZE        512

/* ���볬Ƶ����hash���С */
#define SC_NUMBER_LMT_HASH_SIZE 128

/* �������к����趨��hash���С */
#define SC_CALLER_SETTING_HASH_SIZE   128



/* sip �ֻ���״̬ */
typedef enum tagSCSipStatusType
{
    SC_SIP_STATUS_TYPE_UNREGISTER,
    SC_SIP_STATUS_TYPE_REGISTER,

    SC_SIP_STATUS_TYPE_BUTT
}SC_SIP_STATUS_TYPE_EN;

/* �м̵�״̬ */
typedef enum tagSCTrunkStateType
{
    SC_TRUNK_STATE_TYPE_UNREGED     = 0,
    SC_TRUNK_STATE_TYPE_TRYING,
    SC_TRUNK_STATE_TYPE_REGISTER,
    SC_TRUNK_STATE_TYPE_REGED,
    SC_TRUNK_STATE_TYPE_FAILED,
    SC_TRUNK_STATE_TYPE_FAIL_WAIT,
    SC_TRUNK_STATE_TYPE_NOREG,

    SC_TRUNK_STATE_TYPE_BUTT
}SC_TRUNK_STATE_TYPE_EN;


/*  ·��Ŀ�ĵ����� */
typedef enum tagSCDestType{
    SC_DEST_TYPE_GATEWAY     = 1,               /* ����Ŀ��Ϊ���� */
    SC_DEST_TYPE_GW_GRP      = 2,               /* ����Ŀ��Ϊ������ */
    SC_DEST_TYPE_BUTT
}SC_DEST_TYPE_EN;


/*  DID��������� */
typedef enum tagSCDIDBindType{
    SC_DID_BIND_TYPE_SIP     = 1,               /* DID���뱻�󶨵�SIP�˻� */
    SC_DID_BIND_TYPE_QUEUE   = 2,               /* DID���뱻�󶨵���ϯ���� */
    SC_DID_BIND_TYPE_AGENT   = 3,               /* DID����󶨵���ϯ */
    SC_DID_BIND_TYPE_BUTT
}SC_DID_BIND_TYPE_EN;

/* ����任�����ö��� */
typedef enum tagSCNumTransformObject{
    SC_NUM_TRANSFORM_OBJECT_SYSTEM = 0,        /* ϵͳ */
    SC_NUM_TRANSFORM_OBJECT_TRUNK,             /* �м� */
    SC_NUM_TRANSFORM_OBJECT_CUSTOMER,          /* �ͻ� */

    SC_NUM_TRANSFORM_OBJECT_BUTT

}SC_NUM_TRANSFORM_OBJECT_EN;

typedef enum tagSCNumTransformDirection{
    SC_NUM_TRANSFORM_DIRECTION_IN = 0,        /* ���� */
    SC_NUM_TRANSFORM_DIRECTION_OUT,           /* ���� */

    SC_NUM_TRANSFORM_DIRECTION_BUTT

}SC_NUM_TRANSFORM_DIRECTION_EN;

typedef enum tagSCNumTransformTiming{
    SC_NUM_TRANSFORM_TIMING_BEFORE = 0,       /* ·��ǰ */
    SC_NUM_TRANSFORM_TIMING_AFTER,            /* ·�ɺ� */

    SC_NUM_TRANSFORM_TIMING_BUTT

}SC_NUM_TRANSFORM_TIMING_EN;

typedef enum tagSCNumTransformSelect{
    SC_NUM_TRANSFORM_SELECT_CALLER = 0,        /* ���� */
    SC_NUM_TRANSFORM_SELECT_CALLEE,            /* ���� */

    SC_NUM_TRANSFORM_SELECT_BUTT

}SC_NUM_TRANSFORM_SELECT_EN;

typedef enum tagSCNumTransformPriority{        /* ����任�����ȼ� */
    SC_NUM_TRANSFORM_PRIORITY_0 = 0,
    SC_NUM_TRANSFORM_PRIORITY_1,
    SC_NUM_TRANSFORM_PRIORITY_2,
    SC_NUM_TRANSFORM_PRIORITY_3,
    SC_NUM_TRANSFORM_PRIORITY_4,
    SC_NUM_TRANSFORM_PRIORITY_5,
    SC_NUM_TRANSFORM_PRIORITY_6,
    SC_NUM_TRANSFORM_PRIORITY_7,
    SC_NUM_TRANSFORM_PRIORITY_8,
    SC_NUM_TRANSFORM_PRIORITY_9,

    SC_NUM_TRANSFORM_PRIORITY_BUTT

}SC_NUM_TRANSFORM_PRIORITY_EN;

typedef enum tagSCBlackNumType
{
    SC_NUM_BLACK_FILE       = 0,            /* �����ļ��к��� */
    SC_NUM_BLACK_REGULAR,                   /* ������� */
    SC_NUM_BLACK_NUM,                       /* �������� */

    SC_NUM_BLACK_BUTT

}SC_NUM_BLACK_TYPE_EN;

/* ���к���ѡ����� */
typedef enum tagCallerSelectPolicy
{
    SC_CALLER_SELECT_POLICY_IN_ORDER = 0,   /* ˳��ѡ����� */
    SC_CALLER_SELECT_POLICY_RANDOM          /* ���ѡ�� */
}SC_CALLER_SELECT_POLICY_EN;

typedef enum tagNumberLmtType
{
    SC_NUM_LMT_TYPE_DID     = 0,
    SC_NUM_LMT_TYPE_CALLER  = 1,

    SC_NUM_LMT_TYPE_BUTT
}SC_NUMBER_LMT_TYPE_EN;

typedef enum tagNumberLmtHandle
{
    SC_NUM_LMT_HANDLE_REJECT     = 0,

    SC_NUM_LMT_HANDLE_BUTT
}SC_NUMBER_LMT_HANDLE_EN;

typedef enum tagNumberLmtCycle
{
    SC_NUMBER_LMT_CYCLE_DAY   = 0,
    SC_NUMBER_LMT_CYCLE_WEEK  = 1,
    SC_NUMBER_LMT_CYCLE_MONTH = 2,
    SC_NUMBER_LMT_CYCLE_YEAR  = 3,

    SC_NUMBER_LMT_CYCLE_BUTT
}SC_NUMBER_LMT_CYCLE_EN;


typedef struct tagSIPAcctStat
{
    U32   ulRegisterCnt;
    U32   ulRegisterFailCnt;
    U32   ulUnregisterCnt;
}SC_SIP_ACCT_ST;

/* User ID �����ڵ� */
typedef struct tagSCUserIDNode{
    U32  ulCustomID;                             /* �û� ID */
    U32  ulSIPID;                                /* �˻� ID */
    S8   szUserID[SC_NUM_LENGTH];         /* SIP�˻� */
    S8   szExtension[SC_NUM_LENGTH];      /* �ֻ��� */

    SC_SIP_STATUS_TYPE_EN  enStatus;             /* ״̬ */

    SC_SIP_ACCT_ST stStat;
}SC_USER_ID_NODE_ST;

/* DIDI�����������ڵ� */
typedef struct tagSCDIDNode{
    U32   ulCustomID;                             /* �û�ID */
    U32   ulDIDID;                                /* DID ����ID */
    BOOL  bValid;                                 /* �Ƿ���ñ�ʶ */
    S8    szDIDNum[SC_NUM_LENGTH];         /* DID ���� */
    U32   ulBindType;                             /* ������ refer to SC_DID_BIND_TYPE_EN */
    U32   ulBindID;                               /* �󶨽�� */
    U32   ulTimes;                                /* ���뱻���д���,����ͳ������ */
}SC_DID_NODE_ST;

/* ·�������ڵ� */
typedef struct tagSCRouteNode
{
    U32        ulID;
    BOOL       bExist;                            /* �ñ����������Ƿ����������ݿ� */
    BOOL       bStatus;                           /* ���·���Ƿ���� */

    U8         ucHourBegin;                       /* ��ʼʱ�䣬Сʱ */
    U8         ucMinuteBegin;                     /* ��ʼʱ�䣬���� */
    U8         ucHourEnd;                         /* ����ʱ�䣬Сʱ */
    U8         ucMinuteEnd;                       /* ����ʱ�䣬���� */

    S8         szCallerPrefix[SC_NUM_LENGTH];     /* ǰ׺���� */
    S8         szCalleePrefix[SC_NUM_LENGTH];     /* ǰ׺���� */

    U32        ulDestType;                        /* Ŀ������ */
    U32        aulDestID[SC_ROUT_GW_GRP_MAX_SIZE];/* Ŀ��ID */

    U16        usCallOutGroup;
    U8         ucPriority;                        /* ���ȼ� */
    U8         aucReserv[2];

}SC_ROUTE_NODE_ST;

/* �ͻ������ڵ� */
typedef struct tagSCCustomerNode
{
    U32        ulID;
    BOOL       bExist;                            /* �ñ����������Ƿ����������ݿ� */

    U16        usCallOutGroup;
    U8         bTraceCall;
    U8         aucReserv[1];

}SC_CUSTOMER_NODE_ST;


typedef struct tagTrunkStat
{
    U32  ulMaxCalls;
    U32  ulMaxSession;
    U32  ulCurrentSessions;
    U32  ulCurrentCalls;
    U32  ulTotalSessions;
    U32  ulTotalCalls;
    U32  ulOutgoingSessions;
    U32  ulIncomingSessions;
    U32  ulFailSessions;

    U32  ulRegisterCnt;
    U32  ulUnregisterCnt;
    U32  ulRegisterFailCnt;
    U32  ulKeepAliveCnt;
    U32  ulKeepAliveFailCnt;
}SC_TRUNK_STAT_ST;

/* ���������ڵ� */
typedef struct tagSCGWNode
{
    U32 ulGWID;                                    /* ����ID */
    S8  szGWDomain[SC_GW_DOMAIN_LEG];              /* ���ص�����ʱû���õ� */
    BOOL bExist;                                   /* �ñ�ʶ�����ж����ݿ��Ƿ��и����� */
    BOOL bStatus;                                  /* ����״̬�����û����ǽ��� */
    BOOL bRegister;                                /* �Ƿ�ע�� */
    U32 ulRegisterStatus;                          /* ע��״̬ */

    SC_TRUNK_STAT_ST stStat;
}SC_GW_NODE_ST;

/* �м��� */
typedef struct tagGatewayGrpNode
{
    U32        ulGWGrpID;                         /* ������ID */
    BOOL       bExist;                            /* �ñ����������������Ƿ����������ݿ� */
    DLL_S      stGWList;                          /* �����б� refer to SC_GW_NODE_ST */
    pthread_mutex_t  mutexGWList;                 /* ·������� */
}SC_GW_GRP_NODE_ST;

typedef struct tagSCNumTransformNode
{
    U32                             ulID;
    BOOL                            bExist;

    SC_NUM_TRANSFORM_OBJECT_EN      enObject;
    U32                             ulObjectID;
    SC_NUM_TRANSFORM_DIRECTION_EN   enDirection;                        /* ����/���� */
    SC_NUM_TRANSFORM_TIMING_EN      enTiming;                           /* ·��ǰ/·�ɺ� */
    SC_NUM_TRANSFORM_SELECT_EN      enNumSelect;                        /* ���к���/���к��� */

    U32                             ulDelLeft;                          /* ���ɾ��λ�� */
    U32                             ulDelRight;                         /* �ұ�ɾ��λ�� */
    SC_NUM_TRANSFORM_PRIORITY_EN    enPriority;                         /* ���ȼ� */
    BOOL                            bReplaceAll;                        /* �Ƿ���ȫ��� */

    S8                              szReplaceNum[SC_NUM_LENGTH];    /* �����*�ſ�ͷ���������������id */
    S8                              szCallerPrefix[SC_NUM_LENGTH];  /* ����ǰ׺ */
    S8                              szCalleePrefix[SC_NUM_LENGTH];  /* ����ǰ׺ */
    S8                              szAddPrefix[SC_NUM_LENGTH];     /* ����ǰ׺ */
    S8                              szAddSuffix[SC_NUM_LENGTH];     /* ���Ӻ�׺ */

    U32                             ulExpiry;                       /* ��Ч�� */
}SC_NUM_TRANSFORM_NODE_ST;

/* ������HASH��ڵ� */
typedef struct tagSCBlackListNode{
    U32                     ulID;                                   /* ���� */
    U32                     ulFileID;                               /* �ļ�ID */
    U32                     ulCustomerID;                           /* �û�ID */
    S8                      szNum[SC_NUM_LENGTH];            /* ���ʽ */
    SC_NUM_BLACK_TYPE_EN    enType;                                 /* ���ͣ��������������ʽ */
}SC_BLACK_LIST_NODE;

typedef struct tagCallerQueryNode{
    U16        usNo;                              /* ��� */
    U8         bValid;
    U8         bTraceON;                          /* �Ƿ���� */

    U32        ulIndexInDB;                       /* ���ݿ��е�ID */
    U32        ulCustomerID;                      /* �����ͻ�id */
    U32        ulTimes;                           /* ���뱻����ѡ�еĴ���������ͳ�� */

    S8         szNumber[SC_NUM_LENGTH];    /* ���뻺�� */
}SC_CALLER_QUERY_NODE_ST;

/* ���к����������ڵ� */
typedef struct tagCallerGrpNode
{
    U32   ulID;          /* ���к�����id */
    U32   ulCustomerID;  /* �ͻ�id */
    U32   ulLastNo;      /* ��һ�β�����е����к������к� */
    U32   ulPolicy;      /* ���в��� */
    BOOL  bDefault;      /* �Ƿ�ΪĬ���飬DOS_FALSE��ʾ��Ĭ���飬DOS_TRUE��ʾĬ���� */
    S8    szGrpName[64]; /* ���к��������� */
    DLL_S stCallerList;  /* ���к����б� */

    pthread_mutex_t mutexCallerList;
}SC_CALLER_GRP_NODE_ST;

/* TT����Ϣ */
typedef struct tagSCTTNumNode{
    U32 ulID;
    U32 ulPort;
    S8  szPrefix[SC_NUM_LENGTH];
    S8  szAddr[SC_MAX_IP_LEN];

}SC_TT_NODE_ST;

/* ��������hash�ڵ� */
typedef struct tagSCNumberLmtNode
{
    U32 ulID;           /* ID */
    U32 ulGrpID;        /* ��ID */
    U32 ulHandle;       /* �����ֶ� */
    U32 ulLimit;        /* �������� */
    U32 ulCycle;        /* ���� */
    U32 ulType;         /* ���к��뻹��DID���� */
    U32 ulNumberID;     /* ����ID */

    /* ���� */
    S8  szPrefix[SC_NUM_LENGTH];

    U32 ulStatUsed;
}SC_NUMBER_LMT_NODE_ST;

typedef struct tagSCSrvCtrlFind{
    U32          ulID;               /* ����ID */
    U32          ulCustomerID;       /* CUSTOMER ID */
}SC_SRV_CTRL_FIND_ST;

typedef enum tagSCSrvCtrlAttr
{
    SC_SRV_CTRL_ATTR_INVLID      = 0,

    SC_SRV_CTRL_ATTR_TASK_MODE   = 1,

    SC_SRV_CTRL_ATTR_BUTT
}SC_SRV_CTRL_ATTR_EN;

typedef struct tagSCSrvCtrl
{
    U32          ulID;               /* ����ID */
    U32          ulCustomerID;       /* CUSTOMER ID */
    U32          ulServType;         /* ҵ������ */
    U32          ulEffectTimestamp;  /* ��Ч���� */
    U32          ulExpireTimestamp;  /* ʧЧ���� */
    U32          ulAttr1;            /* ���� */
    U32          ulAttr2;            /* ���� */
    U32          ulAttrValue1;       /* ����ֵ1 */
    U32          ulAttrValue2;       /* ����ֵ2 */

    BOOL         bExist;             /* �ñ�ʶ�����ж����ݿ��Ƿ��и����� */
}SC_SRV_CTRL_ST;

/* ���к����趨�����ڵ� */
typedef struct tagCallerSetting
{
    U32   ulID;               /* ����󶨹�ϵid */
    U32   ulCustomerID;       /* �ͻ�id */
    S8    szSettingName[64];  /* ��ϵ���� */
    U32   ulSrcID;            /* ����Դid */
    U32   ulSrcType;          /* ����Դ���� */
    U32   ulDstID;            /* Ŀ��ID */
    U32   ulDstType;          /* Ŀ������ */
}SC_CALLER_SETTING_ST;

/* ������������ĺ������� */
typedef enum tagSCNumberType
{
    SC_NUMBER_TYPE_CFG = 0,         /* ϵͳ�ֶ����õ����к��� */
    SC_NUMBER_TYPE_DID,             /* ϵͳ��did���� */
}SC_NUMBER_TYPE_EN;

/* �������к���ѡ����� */
typedef enum tagCallerPolicy
{
    SC_CALLER_POLICY_IN_ORDER = 0,   /* ѭ��˳��ѡ����� */
    SC_CALLER_POLICY_RANDOM = 1,     /* ���ѡ����� */

    SC_CALLER_POLICY_BUTT
}SC_CALLER_POLICY_EN;

/* �������Դ���� */
typedef enum tagSrcCallerType
{
    SC_SRC_CALLER_TYPE_AGENT = 1,      /* ��ϯ */
    SC_SRC_CALLER_TYPE_AGENTGRP = 2,   /* ��ϯ�� */
    SC_SRC_CALLER_TYPE_ALL = 10        /* ���к��� */
}SC_SRC_CALLER_TYPE_EN;

/* �������Ŀ������ */
typedef enum tagDstCallerType
{
    SC_DST_CALLER_TYPE_CFG = 0,     /* ϵͳ���ú��� */
    SC_DST_CALLER_TYPE_DID,         /* did���� */
    SC_DST_CALLER_TYPE_CALLERGRP    /* ������ */
}SC_DST_CALLER_TYPE_EN;


typedef struct tagCallerCacheNode
{
    U32   ulType;   /* �������� */
    union stCallerData
    {
        SC_CALLER_QUERY_NODE_ST  *pstCaller;  /* ���к���ָ�� */
        SC_DID_NODE_ST           *pstDid;     /* did����ָ�� */
    }stData;
}SC_CALLER_CACHE_NODE_ST;

U32 sc_int_hash_func(U32 ulVal, U32 ulHashSize);

U32 sc_serv_ctrl_load(U32 ulIndex);
U32 sc_number_lmt_load(U32 ulIndex);
U32 sc_caller_setting_load(U32 ulIndex);
U32 sc_caller_relationship_load();
U32 sc_caller_group_load(U32 ulIndex);
U32 sc_caller_load(U32 ulIndex);
U32 sc_eix_dev_load(U32 ulIndex);
U32 sc_black_list_load(U32 ulIndex);
U32 sc_sip_account_load(U32 ulIndex);
U32 sc_did_load(U32 ulIndex);
U32 sc_customer_load(U32 ulIndex);
U32 sc_transform_load(U32 ulIndex);
U32 sc_route_load(U32 ulIndex);
U32 sc_gateway_relationship_load();
U32 sc_gateway_group_load(U32 ulIndex);
U32 sc_gateway_load(U32 ulIndex);
U32 sc_route_search(SC_SRV_CB *pstSCB, S8 *pszCalling, S8 *pszCallee);
U32 sc_route_get_trunks(U32 ulRouteID, U32 *paulTrunkList, U32 ulTrunkListSize);
U32 sc_sip_account_get_by_extension(U32 ulCustomID, S8 *pszExtension, S8 *pszUserID, U32 ulLength);
S32 sc_caller_setting_hash_find(VOID *pObj, HASH_NODE_S *pstHashNode);
S32 sc_caller_hash_find(VOID *pObj, HASH_NODE_S *pstHashNode);
S32 sc_caller_group_hash_find(VOID *pObj, HASH_NODE_S *pstHashNode);
U32 sc_caller_setting_update_proc(U32 ulAction, U32 ulSettingID);
U32 sc_caller_group_update_proc(U32 ulAction, U32 ulCallerGrpID);
U32 sc_gateway_update_proc(U32 ulAction, U32 ulGatewayID);
U32 sc_sip_account_update_proc(U32 ulAction, U32 ulSipID, U32 ulCustomerID);
U32 sc_route_update_proc(U32 ulAction, U32 ulRouteID);
U32 sc_gateway_group_update_proc(U32 ulAction, U32 ulGwGroupID);
U32 sc_did_update_proc(U32 ulAction, U32 ulDidID);
U32 sc_black_list_update_proc(U32 ulAction, U32 ulBlackID);
U32 sc_caller_update_proc(U32 ulAction, U32 ulCallerID);
U32 sc_eix_dev_update_proc(U32 ulAction, U32 ulEixID);
U32 sc_serv_ctrl_update_proc(U32 ulAction, U32 ulID);
U32 sc_transform_update_proc(U32 ulAction, U32 ulNumTransID);
U32 sc_customer_update_proc(U32 ulAction, U32 ulCustomerID);
BOOL sc_number_lmt_check(U32 ulType, U32 ulCurrentTime, S8 *pszNumber);
U32 sc_number_lmt_update_proc(U32 ulAction, U32 ulNumlmtID);


S32 sc_task_load(U32 ulIndex);
U32 sc_task_load_callee(SC_TASK_CB *pstTCB);
U32 sc_task_mngt_load_task();
U32 sc_task_save_status(U32 ulTaskID, U32 ulStatus, S8 *pszStatus);

U32  sc_get_number_by_callergrp(U32 ulGrpID, S8 *pszNumber, U32 ulLen);
BOOL sc_serv_ctrl_check(U32 ulCustomerID, U32 ulServerType, U32 ulAttr1, U32 ulAttrVal1,U32 ulAttr2, U32 ulAttrVal2);
U32 sc_eix_dev_get_by_tt(S8 *pszTTNumber, S8 *pszEIX, U32 ulLength);
U32  sc_caller_setting_select_number(U32 ulCustomerID, U32 ulSrcID, U32 ulSrcType, S8 *pszNumber, U32 ulLen);

#endif /* __SC_RES_H__ */

#ifdef __cplusplus
}
#endif /* End of __cplusplus */

