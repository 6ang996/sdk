/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  �ļ���: sc_ep_pub.h
 *
 *  ����ʱ��: 2015��1��9��11:09:15
 *  ��    ��: Larry
 *  ��    ��: ����FS����ģ�鷢�͹�������Ϣģ��Ĺ���ͷ�ļ�
 *  �޸���ʷ:
 */

#ifndef __SC_EP_PUB_H__
#define __SC_EP_PUB_H__

/* ����·����Ϣ�к���ǰ����׺�ĳ��� */
#define SC_NUM_PREFIX_LEN       16

/* ����������ĳ��� */
#define SC_GW_DOMAIN_LEG        32

/* ����SIP User ID��hash���С */
#define SC_IP_USERID_HASH_SIZE  1024

/* ����DID�����hash���С */
#define SC_IP_DID_HASH_SIZE     128

/* ��������������hash���С */
#define SC_BLACK_LIST_HASH_SIZE 1024

/* TT�Ŷ�Ӧ��ϵ */
#define SC_TT_NUMBER_HASH_SIZE  128

/* �����������hash���С */
#define SC_GW_GRP_HASH_SIZE     128

/* �������ص�hash���С */
#define SC_GW_HASH_SIZE         128

/* �������к����hash���С */
#define SC_CALLER_HASH_SIZE     128

/* �������к������hash���С */
#define SC_CALLER_GRP_HASH_SIZE 128

/* �������к����趨��hash���С */
#define SC_CALLER_SETTING_HASH_SIZE   128

/* ·�� �м����������� */
#define SC_ROUT_GW_GRP_MAX_SIZE   5


/*  DID��������� */
typedef enum tagSCDIDBindType{
    SC_DID_BIND_TYPE_SIP     = 1,               /* DID���뱻�󶨵�SIP�˻� */
    SC_DID_BIND_TYPE_QUEUE   = 2,               /* DID���뱻�󶨵���ϯ���� */
    SC_DID_BIND_TYPE_AGENT   = 3,               /* DID����󶨵���ϯ */
    SC_DID_BIND_TYPE_BUTT
}SC_DID_BIND_TYPE_EN;

/*  ·��Ŀ�ĵ����� */
typedef enum tagSCDestType{
    SC_DEST_TYPE_GATEWAY     = 1,               /* ����Ŀ��Ϊ���� */
    SC_DEST_TYPE_GW_GRP      = 2,               /* ����Ŀ��Ϊ������ */
    SC_DEST_TYPE_BUTT
}SC_DEST_TYPE_EN;

/* ���к���ѡ����� */
typedef enum tagCallerSelectPolicy
{
    SC_CALLER_SELECT_POLICY_IN_ORDER = 0,   /* ˳��ѡ����� */
    SC_CALLER_SELECT_POLICY_RANDOM          /* ���ѡ�� */
}SC_CALLER_SELECT_POLICY_EN;


/* User ID �����ڵ� */
typedef struct tagSCUserIDNode{
    U32  ulCustomID;                             /* �û� ID */
    U32  ulSIPID;                                /* �˻� ID */
    S8   szUserID[SC_TEL_NUMBER_LENGTH];         /* SIP�˻� */
    S8   szExtension[SC_TEL_NUMBER_LENGTH];      /* �ֻ��� */

    SC_STATUS_TYPE_EN  enStatus;                 /* ״̬ */

    SC_SIP_ACCT_ST stStat;
}SC_USER_ID_NODE_ST;

/* ������HASH��ڵ� */
typedef struct tagSCBlackListNode{
    U32  ulID;                                   /* ���� */
    U32  ulFileID;                                   /* ���� */
    U32  ulCustomerID;                           /* �û�ID */
    S8   szNum[SC_TEL_NUMBER_LENGTH];            /* ���ʽ */
    U32  ulType;                                 /* ���ͣ��������������ʽ */
}SC_BLACK_LIST_NODE;

/* DIDI�����������ڵ� */
typedef struct tagSCDIDNode{
    U32   ulCustomID;                             /* �û�ID */
    U32   ulDIDID;                                /* DID ����ID */
    S8    szDIDNum[SC_TEL_NUMBER_LENGTH];         /* DID ���� */
    U32   ulBindType;                             /* ������ refer to SC_DID_BIND_TYPE_EN */
    U32   ulBindID;                               /* �󶨽�� */
}SC_DID_NODE_ST;

/* TT����Ϣ */
typedef struct tagSCTTNumNode{
    U32 ulID;
    S8  szPrefix[SC_TEL_NUMBER_LENGTH];
    S8  szAddr[SC_IP_ADDR_LEN];
}SC_TT_NODE_ST;

/* ���������ڵ� */
typedef struct tagSCGWNode
{
    U32 ulGWID;                                    /* ����ID */
    S8  szGWDomain[SC_GW_DOMAIN_LEG];              /* ���ص�����ʱû���õ� */
    BOOL bExist;                                   /* �ñ�ʶ�����ж����ݿ��Ƿ��и����� */

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

/* ·�������ڵ� */
typedef struct tagSCRouteNode
{
    U32        ulID;
    BOOL       bExist;                            /* �ñ����������Ƿ����������ݿ� */

    U8         ucHourBegin;                       /* ��ʼʱ�䣬Сʱ */
    U8         ucMinuteBegin;                     /* ��ʼʱ�䣬���� */
    U8         ucHourEnd;                         /* ����ʱ�䣬Сʱ */
    U8         ucMinuteEnd;                       /* ����ʱ�䣬���� */

    S8         szCallerPrefix[SC_NUM_PREFIX_LEN]; /* ǰ׺���� */
    S8         szCalleePrefix[SC_NUM_PREFIX_LEN]; /* ǰ׺���� */

    U32        ulDestType;                        /* Ŀ������ */
    U32        aulDestID[SC_ROUT_GW_GRP_MAX_SIZE];/* Ŀ��ID */

    U16        usCallOutGroup;
    U8         aucReserv[2];

}SC_ROUTE_NODE_ST;

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
    pthread_mutex_t  mutexCallerList;   /* �� */
}SC_CALLER_GRP_NODE_ST;

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

/* �¼����� */
typedef struct tagSCEventNode
{
    list_t stLink;
    esl_event_t *pstEvent;
}SC_EP_EVENT_NODE_ST;

/* ESL�ͻ��˿��ƿ� */
typedef struct tagSCEventProcessHandle
{
    pthread_t           pthID;
    esl_handle_t        stRecvHandle;                /*  esl ���� ��� */
    esl_handle_t        stSendHandle;                /*  esl ���� ��� */

    BOOL                blIsESLRunning;              /* ESL�Ƿ��������� */
    BOOL                blIsWaitingExit;             /* �����Ƿ����ڵȴ��˳� */
    U32                 ulESLDebugLevel;             /* ESL���Լ��� */
}SC_EP_HANDLE_ST;


#endif

