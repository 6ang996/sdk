/**
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  �ļ���: sc_task_pub.h
 *
 *  ����ʱ��: 2014��12��16��10:15:56
 *  ��    ��: Larry
 *  ��    ��: ҵ�����ģ�飬Ⱥ��������ض���
 *  �޸���ʷ:
 */

#ifndef __SC_TASK_PUB_H__
#define __SC_TASK_PUB_H__

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* include public header files */
extern BOOL                 g_blSCInitOK;


/* include private header files */

/* define marcos */
/* ����HTTP������������ */
#define SC_MAX_HTTPD_NUM               1


/* ���������16���ͻ�������HTTP������ */
#define SC_MAX_HTTP_CLIENT_NUM         16

/* ����HTTP API������������ */
#define SC_API_PARAMS_MAX_NUM          24

/* ����HTTP�������У��ļ�������󳤶� */
#define SC_HTTP_REQ_MAX_LEN            64

/* ����ESLģ���������󳤶� */
#define SC_ESL_CMD_BUFF_LEN            1024

/* �������ģ�飬���������� */
#define SC_DEFAULT_PLAYCNT             3

/* �������ģ�飬������˽�����ʱʱ�� */
#define SC_MAX_TIMEOUT4NOANSWER        10

/* �绰���볤�� */
#define SC_TEL_NUMBER_LENGTH           24

/* IP��ַ���� */
#define SC_IP_ADDR_LEN                 32

/* ���ų������ֵ */
#define SC_EMP_NUMBER_LENGTH           12

/* ÿ���������ʱ��������ڵ� */
#define SC_MAX_PERIOD_NUM              4

/* UUID ��󳤶� */
#define SC_MAX_UUID_LENGTH             40

/* �����ļ������� */
#define SC_MAX_AUDIO_FILENAME_LEN      128

/* �������еı��� */
#define SC_MAX_CALL_MULTIPLE           3

#define SC_MAX_CALL_PRE_SEC            200


#define SC_MAX_SRV_TYPE_PRE_LEG        4

/* ���������� */
#define SC_MAX_CALL                    3000

/* ��������� */
#define SC_MAX_TASK_NUM                1024

/* �������� */
#define SC_MAX_SCB_NUM                 SC_MAX_CALL*2

/* SCB hash������� */
#define SC_MAX_SCB_HASH_NUM            4096

/* ÿ���������ϯ����� */
#define SC_MAX_SITE_NUM                1024

/* ÿ���������к���������� */
#define SC_MAX_CALLER_NUM              1024

/* ÿ�����񱻽к�����С����(һ��LOAD�����������ʵ�ʻ����ʵ���������) */
#define SC_MIN_CALLEE_NUM              65535

/* ����һ�����������Գ���ʱ����12Сʱ(43200s) */
#define SC_MAX_CALL_DURATION           43200

/* ����ͬһ�����뱻�ظ����е����ʱ������4Сʱ */
#define SC_MAX_CALL_INTERCAL           60 * 4

#define SC_CALL_THRESHOLD_VAL0         40         /* ��ֵ0���ٷֱ� ����Ҫ�������з�����*/
#define SC_CALL_THRESHOLD_VAL1         80         /* ��ֵ1���ٷֱ� ����Ҫ�������з�����*/

#define SC_CALL_INTERVAL_VAL0          300        /* ��ǰ������/��������*100 < ��ֵ0��300����һ�� */
#define SC_CALL_INTERVAL_VAL1          500        /* ��ǰ������/��������*100 < ��ֵ1��500����һ�� */
#define SC_CALL_INTERVAL_VAL2          1000       /* ��ǰ������/��������*100 > ��ֵ1��1000����һ�� */

#define SC_MEM_THRESHOLD_VAL0          90         /* ϵͳ״̬��ֵ���ڴ�ռ���ʷ�ֵ0 */
#define SC_MEM_THRESHOLD_VAL1          95         /* ϵͳ״̬��ֵ���ڴ�ռ���ʷ�ֵ0 */
#define SC_CPU_THRESHOLD_VAL0          90         /* ϵͳ״̬��ֵ��CPUռ���ʷ�ֵ0 */
#define SC_CPU_THRESHOLD_VAL1          95         /* ϵͳ״̬��ֵ��CPUռ���ʷ�ֵ0 */

#define SC_INVALID_INDEX               0

#define SC_NUM_VERIFY_TIME             3          /* ������֤�벥�Ŵ��� */
#define SC_NUM_VERIFY_TIME_MAX         10         /* ������֤�벥�Ŵ��� */
#define SC_NUM_VERIFY_TIME_MIN         2          /* ������֤�벥�Ŵ��� */

#define SC_MASTER_TASK_INDEX           0
#define SC_EP_TASK_NUM                 2


#define SC_BGJOB_HASH_SIZE             128


/* ������Ӫ�̵�ID */
#define SC_TOP_USER_ID                 1

#define SC_LIST_MIN_CNT                3

#define SC_TASK_AUDIO_PATH             "/var/www/html/data/audio"

#define SC_RECORD_FILE_PATH            "/var/record"

#define SC_NOBODY_UID                  99
#define SC_NOBODY_GID                  99

#define SC_NUM_TRANSFORM_PREFIX_LEN    16

/* ���һ��TCB����������Task��CustomID */
#define SC_TCB_HAS_VALID_OWNER(pstTCB)                        \
    ((pstTCB)                                                 \
    && (pstTCB)->ulTaskID != 0                                \
    && (pstTCB)->ulTaskID != U32_BUTT                         \
    && (pstTCB)->ulCustomID != 0                              \
    && (pstTCB)->ulCustomID != U32_BUTT)

#define SC_SCB_HAS_VALID_OWNER(pstSCB)                        \
    ((pstSCB)                                                 \
    && (pstSCB)->ulTaskID != 0                                \
    && (pstSCB)->ulTaskID != U32_BUTT                         \
    && (pstSCB)->ulCustomID != 0                              \
    && (pstSCB)->ulCustomID != U32_BUTT)


#define SC_TCB_VALID(pstTCB)                                  \
    ((pstTCB)                                                 \
    && (pstTCB)->ulTaskID != 0)

#define SC_SCB_IS_VALID(pstSCB)                               \
    ((pstSCB) && (pstSCB)->bValid)

#define SC_SCB_SET_STATUS(pstSCB, ulStatus)                   \
do                                                            \
{                                                             \
    if (DOS_ADDR_INVALID(pstSCB)                              \
        || ulStatus >= SC_SCB_BUTT)                           \
    {                                                         \
        break;                                                \
    }                                                         \
    pthread_mutex_lock(&(pstSCB)->mutexSCBLock);              \
    sc_call_trace((pstSCB), "SCB Status Change %s -> %s"      \
                , sc_scb_get_status((pstSCB)->ucStatus)       \
                , sc_scb_get_status(ulStatus));               \
    (pstSCB)->ucStatus = (U8)ulStatus;                        \
    pthread_mutex_unlock(&(pstSCB)->mutexSCBLock);            \
}while(0)

#define SC_SCB_SET_SERVICE(pstSCB, ulService)                 \
do                                                            \
{                                                             \
    if (DOS_ADDR_INVALID(pstSCB)                              \
      || (pstSCB)->ucCurrentSrvInd >= SC_MAX_SRV_TYPE_PRE_LEG \
      || ulService >= SC_SERV_BUTT)                           \
    {                                                         \
        break;                                                \
    }                                                         \
    pthread_mutex_lock(&(pstSCB)->mutexSCBLock);              \
    sc_call_trace((pstSCB), "SCB Add service.");              \
    (pstSCB)->aucServiceType[(pstSCB)->ucCurrentSrvInd++]     \
                = (U8)ulService;                              \
    pthread_mutex_unlock(&(pstSCB)->mutexSCBLock);            \
}while(0)


#define SC_TRACE_HTTPD                  (1<<1)
#define SC_TRACE_HTTP                   (1<<2)
#define SC_TRACE_TASK                   (1<<3)
#define SC_TRACE_SC                     (1<<4)
#define SC_TRACE_ACD                    (1<<5)
#define SC_TRACE_DIAL                   (1<<6)
#define SC_TRACE_FUNC                   (1<<7)
#define SC_TRACE_ALL                    0xFFFFFFFF


#define SC_EP_EVENT_LIST \
            "BACKGROUND_JOB " \
            "CHANNEL_PARK " \
            "CHANNEL_CREATE " \
            "CHANNEL_ANSWER " \
            "PLAYBACK_STOP " \
            "CHANNEL_HANGUP " \
            "CHANNEL_HANGUP_COMPLETE " \
            "CHANNEL_HOLD " \
            "CHANNEL_UNHOLD " \
            "DTMF " \
            "SESSION_HEARTBEAT "


enum tagCallServiceType{
    SC_SERV_OUTBOUND_CALL           = 0,   /* ��FS���ⲿ(����SIP��PSTN)����ĺ��� */
    SC_SERV_INBOUND_CALL            = 1,   /* ���ⲿ(����SIP��PSTN)��FS����ĺ��� */
    SC_SERV_INTERNAL_CALL           = 2,   /* FS��SIP�ն�֮��ĺ��� */
    SC_SERV_EXTERNAL_CALL           = 3,   /* FS��PSTN֮��ĺ��� */

    SC_SERV_AUTO_DIALING            = 4,   /* �Զ������������������ */
    SC_SERV_PREVIEW_DIALING         = 5,   /* Ԥ����� */
    SC_SERV_PREDICTIVE_DIALING      = 6,   /* Ԥ����� */

    SC_SERV_RECORDING               = 7,   /* ¼��ҵ�� */
    SC_SERV_FORWORD_CFB             = 8,   /* æתҵ�� */
    SC_SERV_FORWORD_CFU             = 9,   /* ������תҵ�� */
    SC_SERV_FORWORD_CFNR            = 10,   /* ��Ӧ��תҵ�� */
    SC_SERV_BLIND_TRANSFER          = 11,  /* æתҵ�� */
    SC_SERV_ATTEND_TRANSFER         = 12,  /* Э��תҵ�� */

    SC_SERV_PICK_UP                 = 13,  /* ����ҵ�� */        /* ** */
    SC_SERV_CONFERENCE              = 14,  /* ���� */
    SC_SERV_VOICE_MAIL_RECORD       = 15,  /* �������� */
    SC_SERV_VOICE_MAIL_GET          = 16,  /* �������� */

    SC_SERV_SMS_RECV                = 17,  /* ���ն��� */
    SC_SERV_SMS_SEND                = 18,  /* ���Ͷ��� */
    SC_SERV_MMS_RECV                = 19,  /* ���ղ��� */
    SC_SERV_MMS_SNED                = 20,  /* ���Ͳ��� */

    SC_SERV_FAX                     = 21,  /* ����ҵ�� */
    SC_SERV_INTERNAL_SERVICE        = 22,  /* �ڲ�ҵ�� */

    SC_SERV_AGENT_CALLBACK          = 23,  /* �غ���ϯ */
    SC_SERV_AGENT_SIGNIN            = 24,  /* ��ϯǩ�� */
    SC_SERV_AGENT_CLICK_CALL        = 25,  /* ��ϯǩ�� */

    SC_SERV_NUM_VERIFY              = 26,  /* ������֤ */

    SC_SERV_CALL_INTERCEPT          = 27,  /* ������֤ */
    SC_SERV_CALL_WHISPERS           = 28,  /* ������֤ */

    SC_SERV_BUTT                    = 255
}SC_CALL_SERVICE_TYPE_EN;

enum {
    SC_NUM_TYPE_USERID              = 0,   /* ����ΪSIP User ID */
    SC_NUM_TYPE_EXTENSION,                 /* ����Ϊ�ֻ��� */
    SC_NUM_TYPE_OUTLINE,                   /* ����Ϊ���� */

    SC_NUM_TYPE_BUTT
};

enum tagCallDirection{
    SC_DIRECTION_PSTN,                     /* ���з���������PSTN */
    SC_DIRECTION_SIP,                       /* ���з���������SIP UA */

    SC_DIRECTION_INVALID                    /* �Ƿ�ֵ */
};

enum tagCalleeStatus
{
    SC_CALLEE_UNCALLED  =  0,           /* ���к���û�б����й� */
    SC_CALLEE_NORMAL,                   /* ���к������������ˣ��ͻ�Ҳ��ͨ�� */
    SC_CALLEE_NOT_CONN,                 /* ���к�������ˣ��ͻ�û�н��� */
    SC_CALLEE_NOT_EXIST,                /* ���к���Ϊ�պ� */
    SC_CALLEE_REJECT,                   /* ���к��뱻�ͻ��ܾ��� */
};

typedef enum tagSiteAccompanyingStatus{
    SC_SITE_ACCOM_DISABLE              = 0,       /* �ֻ����У���ֹ */
    SC_SITE_ACCOM_ENABLE,                         /* �ֻ����У����� */

    SC_SITE_ACCOM_BUTT                 = 255      /* �ֻ����У��Ƿ�ֵ */
}SC_SITE_ACCOM_STATUS_EN;

typedef enum tagIntervalService
{
    SC_INTER_SRV_NUMBER_VIRFY            = 0,  /* �������ҵ�� */
    SC_INTER_SRV_INGROUP_CALL,                 /* ���ں��� */        /* * */
    SC_INTER_SRV_OUTGROUP_CALL,                /* ������� */        /* *9 */
    SC_INTER_SRV_HOTLINE_CALL,                 /* ���ߺ��� */
    SC_INTER_SRV_SITE_SIGNIN,                  /* ��ϯǩ����� */    /* *2 */

    SC_INTER_SRV_WARNING,                      /* �澯��Ϣ */
    SC_INTER_SRV_QUERY_IP,                     /* ��IP��Ϣ */        /* *158 */
    SC_INTER_SRV_QUERY_EXTENTION,              /* ��ֻ���Ϣ */      /* *114 */
    SC_INTER_SRV_QUERY_AMOUNT,                 /* �����ҵ�� */      /* *199 */

    SC_INTER_SRV_BUTT
}SC_INTERVAL_SERVICE_EN;

typedef enum tagSysStatus{
    SC_SYS_NORMAL                      = 3,
    SC_SYS_BUSY                        = 4,       /* ����״̬��ϵͳæ����ͣ��������80%���ϵ���������� */
    SC_SYS_ALERT,                                 /* ����״̬��ϵͳæ��ֻ��������ȼ��ͻ������Һ�������80%���µĿͻ�������� */
    SC_SYS_EMERG,                                 /* ����״̬��ϵͳæ����ͣ�������� */

    SC_SYS_BUTT                        = 255      /* ����״̬���Ƿ�ֵ */
}SC_SYS_STATUS_EN;

typedef enum tagTaskStatusInDB{
    SC_TASK_STATUS_DB_STOP             = 0,/* ���ݿ�������״̬ */
    SC_TASK_STATUS_DB_START,               /* ���ݿ�������״̬ */
    SC_TASK_STATUS_DB_PAUSED,              /* ���ݿ�������״̬ */
    SC_TASK_STATUS_DB_CONTINUE,            /* ���ݿ�������״̬ */

    SC_TASK_STATUS_DB_BUTT
}SC_TASK_STATUS_DB_EN;

typedef enum tagTaskStatus{
    SC_TASK_INIT                       = 0,       /* ����״̬����ʼ�� */
    SC_TASK_WORKING,                              /* ����״̬������ */
    SC_TASK_STOP,                                 /* ����״̬��ֹͣ�����ٷ�����У�������к��н��������ͷ���Դ */
    SC_TASK_PAUSED,                               /* ����״̬����ͣ */
    SC_TASK_SYS_BUSY,                             /* ����״̬��ϵͳæ����ͣ��������80%���ϵ���������� */
    SC_TASK_SYS_ALERT,                            /* ����״̬��ϵͳæ��ֻ��������ȼ��ͻ������Һ�������80%���µĿͻ�������� */
    SC_TASK_SYS_EMERG,                            /* ����״̬��ϵͳæ����ͣ�������� */

    SC_TASK_BUTT                       = 255      /* ����״̬���Ƿ�ֵ */
}SC_TASK_STATUS_EN;

typedef enum tagTaskMode{
    SC_TASK_MODE_KEY4AGENT           = 0,         /* ��������ģʽ������������֮��ת��ϯ */
    SC_TASK_MODE_KEY4AGENT1          = 1,         /* ��������ģʽ������������֮��ת��ϯ */
    SC_TASK_MODE_DIRECT4AGETN,                    /* ��������ģʽ����ͨ��ֱ��ת��ϯ */
    SC_TASK_MODE_AUDIO_ONLY,                      /* ��������ģʽ����������� */
    SC_TASK_MODE_AGENT_AFTER_AUDIO,               /* ��������ģʽ��������ת��ϯ */

    SC_TASK_MODE_BUTT
}SC_TASK_MODE_EN;

typedef enum tagTaskPriority{
    SC_TASK_PRI_LOW                       = 0,    /* �������ȼ��������ȼ� */
    SC_TASK_PRI_NORMAL,                           /* �������ȼ����������ȼ� */
    SC_TASK_PRI_HIGHT,                            /* �������ȼ��������ȼ� */
}SC_TASK_PRI_EN;

typedef enum tagCallNumType{
    SC_CALL_NUM_TYPE_NORMOAL              = 0,       /* �������ͣ������ĺ��� */
    SC_CALL_NUM_TYPE_EXPR,                           /* �������ͣ�������ʽ */

    SC_CALL_NUM_TYPE_BUTT                 = 255      /* �������ͣ��Ƿ�ֵ */
}SC_CALL_NUM_TYPE_EN;


typedef enum tagSCBStatus
{
    SC_SCB_IDEL                           = 0,     /* SCB״̬������״̬ */
    SC_SCB_INIT,                                   /* SCB״̬��ҵ���ʼ��״̬ */
    SC_SCB_AUTH,                                   /* SCB״̬��ҵ����֤״̬ */
    SC_SCB_EXEC,                                   /* SCB״̬��ҵ��ִ��״̬ */
    SC_SCB_ACTIVE,                                 /* SCB״̬��ҵ�񼤻�״̬ */
    SC_SCB_RELEASE,                                /* SCB״̬��ҵ���ͷ�״̬ */

    SC_SCB_BUTT
}SC_SCB_STATUS_EN;

typedef enum tagSCStatusType
{
    SC_STATUS_TYPE_UNREGISTER,
    SC_STATUS_TYPE_REGISTER,

    SC_STATUS_TYPE_BUTT
}SC_STATUS_TYPE_EN;

typedef enum tagSCCallRole
{
    SC_CALLEE,
    SC_CALLER,

    SC_CALL_ROLE_BUTT
}SC_CALL_ROLE_EN;

typedef enum tagTransferRole
{
    SC_TRANS_ROLE_NOTIFY        = 0,
    SC_TRANS_ROLE_PUBLISH       = 1,
    SC_TRANS_ROLE_SUBSCRIPTION  = 2,

    SC_TRANS_ROLE_BUTT,
}SC_TRANSFER_ROLE_EN;


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

/* ������������ĺ������� */
typedef enum tagSCNumberType
{
    SC_NUMBER_TYPE_CFG = 0,         /* ϵͳ�ֶ����õ����к��� */
    SC_NUMBER_TYPE_DID,             /* ϵͳ��did���� */
}SC_NUMBER_TYPE_EN;

#define SC_EP_STAT_RECV 0
#define SC_EP_STAT_PROC 1

typedef struct tagEPMsgStat
{
    U32   ulCreate;
    U32   ulPark;
    U32   ulAnswer;
    U32   ulHungup;
    U32   ulHungupCom;
    U32   ulDTMF;
    U32   ulBGJob;
    U32   ulHold;
    U32   ulUnhold;
}SC_EP_MSG_STAT_ST;

typedef struct tagBSMsgStat
{
    U32  ulAuthReq;
    U32  ulAuthReqSend;
    U32  ulAuthRsp;
    U32  ulBillingReq;
    U32  ulBillingReqSend;
    U32  ulBillingRsp;
    U32  ulHBReq;
    U32  ulHBRsp;
}SC_BS_MSG_STAT_ST;

typedef struct tagSystemStat
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
    U32  ulSystemUpTime;
    U32  ulSystemIsWorking;
}SC_SYSTEM_STAT_ST;

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

typedef struct tagSiteStat
{
    U32  ulSelectCnt;
    U32  ulCallCnt;      /* ��ʱ�� ulSelectCnt ����һ�� */
    U32  ulIncomingCall; /* ��ʱû��ʵ�� */
    U32  ulOutgoingCall; /* ��ʱû��ʵ�� */
    U32  ulTimeOnSignin;             /* ��ǩ��ʱ�� */
    U32  ulTimeOnthePhone;           /* ������ʱ�� */
}SC_SITE_STAT_ST;

typedef struct tagSiteGrpStat
{
    U32  ulCallCnt;
    U32  ulCallinQueue;
    U32  ulTotalWaitingTime;      /* ��ʱû��ʵ�� */
    U32  ulTotalWaitingCall;
}SC_SITE_GRP_STAT_ST;

typedef struct tagSIPAcctStat
{
    U32   ulRegisterCnt;
    U32   ulRegisterFailCnt;
    U32   ulUnregisterCnt;
}SC_SIP_ACCT_ST;


typedef struct tagCallerQueryNode{
    U16        usNo;                              /* ��� */
    U8         bValid;
    U8         bTraceON;                          /* �Ƿ���� */

    U32        ulIndexInDB;                       /* ���ݿ��е�ID */
    U32        ulCustomerID;                      /* �����ͻ�id */
    U32        ulTimes;                           /* ���뱻����ѡ�еĴ���������ͳ�� */

    S8         szNumber[SC_TEL_NUMBER_LENGTH];    /* ���뻺�� */
}SC_CALLER_QUERY_NODE_ST;

/* define structs */
typedef struct tagTelNumQueryNode
{
    list_t     stLink;                            /* ��������ڵ� */

    U32        ulIndex;                           /* ���ݿ��е�ID */

    U8         ucTraceON;                         /* �Ƿ���� */
    U8         ucCalleeType;                      /* ���к������ͣ� refer to enum SC_CALL_NUM_TYPE_EN */
    U8         aucRes[2];

    S8         szNumber[SC_TEL_NUMBER_LENGTH];    /* ���뻺�� */
}SC_TEL_NUM_QUERY_NODE_ST;

typedef struct tagSiteQueryNode
{
    U16        usSCBNo;
    U16        usRec;

    U32        bValid:1;
    U32        bRecord:1;
    U32        bTraceON:1;
    U32        bAllowAccompanying:1;              /* �Ƿ�����ֻ����� refer to SC_SITE_ACCOM_STATUS_EN */
    U32        ulRes1:28;

    U32        ulStatus;                          /* ��ϯ״̬ refer to SC_SITE_STATUS_EN */
    U32        ulSiteID;                          /* ��ϯ���ݿ��� */

    S8         szUserID[SC_TEL_NUMBER_LENGTH];    /* SIP User ID */
    S8         szExtension[SC_TEL_NUMBER_LENGTH]; /* �ֻ��� */
    S8         szEmpNo[SC_EMP_NUMBER_LENGTH];     /* ���뻺�� */
}SC_SITE_QUERY_NODE_ST;

typedef struct tagTaskAllowPeriod{
    U8         ucValid;
    U8         ucWeekMask;                        /* �ܿ��ƣ�ʹ��λ��������0λΪ������ */
    U8         ucHourBegin;                       /* ��ʼʱ�䣬Сʱ */
    U8         ucMinuteBegin;                     /* ��ʼʱ�䣬���� */

    U8         ucSecondBegin;                     /* ��ʼʱ�䣬�� */
    U8         ucHourEnd;                         /* ����ʱ�䣬Сʱ */
    U8         ucMinuteEnd;                       /* ����ʱ�䣬���� */
    U8         ucSecondEnd;                       /* ����ʱ�䣬�� */

    U32        ulRes;
}SC_TASK_ALLOW_PERIOD_ST;

typedef struct tagSCBExtraData
{
    U32             ulStartTimeStamp;           /* ��ʼʱ��� */
    U32             ulRingTimeStamp;            /* ����ʱ��� */
    U32             ulAnswerTimeStamp;          /* Ӧ��ʱ��� */
    U32             ulIVRFinishTimeStamp;       /* IVR�������ʱ��� */
    U32             ulDTMFTimeStamp;            /* (��һ��)���β���ʱ��� */
    U32             ulBridgeTimeStamp;          /* LEG�Ž�ʱ��� */
    U32             ulByeTimeStamp;             /* �ͷ�ʱ��� */
    U32             ulPeerTrunkID;              /* �Զ��м�ID */

    U8              ucPayloadType;              /* ý������ */
    U8              ucPacketLossRate;           /* �հ�������,0-100 */
}SC_SCB_EXTRA_DATA_ST;


/* ���п��ƿ� */
typedef struct tagSCSCB{
    U16       usSCBNo;                            /* ��� */
    U16       usOtherSCBNo;                       /* ����һ��leg��SCB��� */

    U16       usTCBNo;                            /* ������ƿ���ID */
    U16       usSiteNo;                           /* ��ϯ��� */

    U32       ulAllocTime;
    U32       ulCustomID;                         /* ��ǰ���������ĸ��ͻ� */
    U32       ulAgentID;                          /* ��ǰ���������ĸ��ͻ� */
    U32       ulTaskID;                           /* ��ǰ����ID */
    U32       ulTrunkID;                          /* �м�ID */

    U8        ucStatus;                           /* ���п��ƿ��ţ�refer to SC_SCB_STATUS_EN */
    U8        ucServStatus;                       /* ҵ��״̬ */
    U8        ucTerminationFlag;                  /* ҵ����ֹ��־ */
    U8        ucTerminationCause;                 /* ҵ����ֹԭ�� */

    U8        aucServiceType[SC_MAX_SRV_TYPE_PRE_LEG];        /* ҵ������ �б�*/

    U8        ucMainService;
    U8        ucCurrentSrvInd;                    /* ��ǰ���е�ҵ���������� */
    U8        ucLegRole;                          /* �����б�ʾ */
    U8        ucCurrentPlyCnt;                    /* ��ǰ�������� */

    U8        ucTranforRole;                      /* transfer��ɫ */
    U8        ucRes;
    U16       usPublishSCB;                       /* �����SCBNo */

    U16       usHoldCnt;                          /* ��hold�Ĵ��� */
    U16       usHoldTotalTime;                    /* ��hold����ʱ�� */
    U32       ulLastHoldTimetamp;                 /* �ϴ�hold�ǵ�ʱ��������hold��ʱ��ֵ�� */

    U32       bValid:1;                           /* �Ƿ�Ϸ� */
    U32       bTraceNo:1;                         /* �Ƿ���� */
    U32       bBanlanceWarning:1;                 /* �Ƿ����澯 */
    U32       bNeedConnSite:1;                    /* ��ͨ���Ƿ���Ҫ��ͨ��ϯ */
    U32       bWaitingOtherRelase:1;              /* �Ƿ��ڵȴ�����һ�����ͷ� */
    U32       bRecord:1;                          /* �Ƿ�¼�� */
    U32       bIsAgentCall:1;                     /* �Ƿ��ں�����ϯ */
    U32       bIsInQueue:1;                       /* �Ƿ��Ѿ�������� */
    U32       bChannelCreated:1;                  /* FREESWITCH �Ƿ�Ϊ��ͬ���д�����ͨ�� */
    U32       ulRes:25;

    U32       ulCallDuration;                     /* ����ʱ������ֹ�����ã�ÿ������ʱ���� */

    U32       ulRes1;

    S32       lBalance;                           /* ���,��λ:�� */

    S8        szCallerNum[SC_TEL_NUMBER_LENGTH];  /* ���к��� */
    S8        szCalleeNum[SC_TEL_NUMBER_LENGTH];  /* ���к��� */
    S8        szANINum[SC_TEL_NUMBER_LENGTH];     /* ���к��� */
    S8        szDialNum[SC_TEL_NUMBER_LENGTH];    /* �û����� */
    S8        szSiteNum[SC_TEL_NUMBER_LENGTH];    /* ��ϯ���� */
    S8        szUUID[SC_MAX_UUID_LENGTH];         /* Leg-A UUID */

    S8        *pszRecordFile;

    SC_SCB_EXTRA_DATA_ST *pstExtraData;           /* ���㻰������Ҫ�Ķ������� */

    pthread_mutex_t mutexSCBLock;                 /* ����SCB���� */
}SC_SCB_ST;

/* ����SCBhash�� */
typedef struct tagSCBHashNode
{
    HASH_NODE_S     stNode;                       /* hash����ڵ� */

    S8              szUUID[SC_MAX_UUID_LENGTH];   /* UUID */
    SC_SCB_ST       *pstSCB;                      /* SCBָ�� */

    sem_t           semSCBSyn;
}SC_SCB_HASH_NODE_ST;


typedef struct tagTaskCB
{
    U16        usTCBNo;                           /* ��� */
    U8         ucValid;                           /* �Ƿ�ʹ�� */
    U8         ucTaskStatus;                      /* ����״̬ refer to SC_TASK_STATUS_EN */

    U32        ulAllocTime;
    U8         ucPriority;                        /* �������ȼ� */
    U8         ucAudioPlayCnt;                    /* ���Բ��Ŵ��� */
    U8         bTraceON;                          /* �Ƿ���� */
    U8         bTraceCallON;                      /* �Ƿ���ٺ��� */

    U8         ucMode;                            /* ����ģʽ refer to SC_TASK_MODE_EN*/
    U8         aucRess[3];

    U32        ulTaskID;                          /* ��������ID */
    U32        ulCustomID;                        /* ������������ */
    U32        ulCurrentConcurrency;              /* ��ǰ������ */
    U32        ulMaxConcurrency;                  /* ��ǰ������ */
    U32        ulAgentQueueID;                    /* ��ϯ���б�� */

    U16        usSiteCount;                       /* ��ϯ���� */
    U16        usCallerCount;                     /* ��ǰ���к������� */
    U32        ulCalleeCount;
    U32        ulLastCalleeIndex;                 /* �������ݷ�ҳ */

    list_t     stCalleeNumQuery;                  /* ���к��뻺�� refer to struct tagTelNumQueryNode */
    S8         szAudioFileLen[SC_MAX_AUDIO_FILENAME_LEN];  /* �����ļ��ļ��� */
    SC_CALLER_QUERY_NODE_ST *pstCallerNumQuery;            /* ���к��뻺�� refer to struct tagTelNumQueryNode */
    SC_TASK_ALLOW_PERIOD_ST astPeriod[SC_MAX_PERIOD_NUM];  /* ����ִ��ʱ��� */

    /* ͳ����� */
    U32        ulTotalCall;                       /* �ܺ����� */
    U32        ulCallFailed;                      /* ����ʧ���� */
    U32        ulCallConnected;                   /* ���н�ͨ�� */

    pthread_t  pthID;                             /* �߳�ID */
    pthread_mutex_t  mutexTaskList;               /* �����������ʹ�õĻ����� */
}SC_TASK_CB_ST;

typedef struct tagBGJobHash{
    S8       szJobUUID[SC_MAX_UUID_LENGTH];

    U32      ulRCNo;                 /* ��Ӧ��Դ��� */
}SC_BG_JOB_HASH_NODE_ST;

typedef struct tagTaskCtrlCMD{
    list_t      stLink;
    U32         ulTaskID;                         /* ����ID */
    U32         ulCMD;                            /* ������ */
    U32         ulCustomID;                       /* �ͻ�ID */
    U32         ulAction;                         /* action */
    U32         ulCMDSeq;                         /* ������� */
    U32         ulCMDErrCode;                     /* ������ */
    S8          *pszErrMSG;                       /* ������Ϣ */
    sem_t       semCMDExecNotify;                 /* ����ִ�����֪ͨʹ�õ��ź��� */
}SC_TASK_CTRL_CMD_ST;

typedef struct tagTaskMngtInfo{
    pthread_t            pthID;                   /* �߳�ID */
    pthread_mutex_t      mutexCMDList;            /* �����������ʹ�õĻ����� */
    pthread_mutex_t      mutexTCBList;            /* ����������ƿ�ʹ�õĻ����� */
    pthread_mutex_t      mutexCallList;           /* �������п��ƿ�ʹ�õĻ����� */
    pthread_mutex_t      mutexCallHash;           /* �������п��ƿ�ʹ�õĻ����� */
    pthread_mutex_t      mutexHashBGJobHash;
    pthread_cond_t       condCMDList;             /* ����������ݵ���֪ͨ������ */
    U32                  blWaitingExitFlag;       /* �ȴ��˳���ʾ */

    list_t               stCMDList;               /* �������(�ڵ���HTTP Server��������HTTP Server�ͷ�) */
    SC_SCB_ST            *pstCallSCBList;         /* ���п��ƿ��б� (��Ҫhash��洢) */
    HASH_TABLE_S         *pstCallSCBHash;         /* ���п��ƿ��hash���� */
    SC_TASK_CB_ST        *pstTaskList;            /* �����б� refer to struct tagTaskCB*/
    U32                  ulTaskCount;             /* ��ǰ����ִ�е������� */
    HASH_TABLE_S         *pstHashBGJobHash;       /* background-job hash�� */

    SC_SYSTEM_STAT_ST    stStat;
}SC_TASK_MNGT_ST;


/*****************���жԴ��������********************/
typedef struct tagCallWaitQueueNode{
    U32                 ulAgentGrpID;                     /* ��ϯ��ID */
    U32                 ulStartWaitingTime;               /* ��ʼ�ȴ���ʱ�� */

    pthread_mutex_t     mutexCWQMngt;
    DLL_S               stCallWaitingQueue;               /* ���еȴ����� refer to SC_SCB_ST */
}SC_CWQ_NODE_ST;
/***************���жԴ�������ؽ���********************/

/* dialerģ����ƿ� */
typedef struct tagSCDialerHandle
{
    esl_handle_t        stHandle;                /*  esl ��� */
    pthread_t           pthID;
    U32                 ulCallCnt;
    pthread_mutex_t     mutexCallQueue;          /* ������ */
    pthread_cond_t      condCallQueue;           /* �������� */
    list_t              stCallList;              /* ���ж��� */

    BOOL                blIsESLRunning;          /* ESL�Ƿ��������� */
    BOOL                blIsWaitingExit;         /* �����Ƿ����ڵȴ��˳� */
    S8                  *pszCMDBuff;
}SC_DIALER_HANDLE_ST;

typedef struct tagEPTaskCB
{
    DLL_S            stMsgList;
    pthread_t        pthTaskID;
    pthread_mutex_t  mutexMsgList;
    pthread_cond_t   contMsgList;
}SC_EP_TASK_CB;

/***************����任��ʼ********************/

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

    S8                              szReplaceNum[SC_TEL_NUMBER_LENGTH]; /* �����*�ſ�ͷ���������������id */
    S8                              szCallerPrefix[SC_NUM_TRANSFORM_PREFIX_LEN];  /* ����ǰ׺ */
    S8                              szCalleePrefix[SC_NUM_TRANSFORM_PREFIX_LEN];  /* ����ǰ׺ */
    S8                              szAddPrefix[SC_NUM_TRANSFORM_PREFIX_LEN];     /* ����ǰ׺ */
    S8                              szAddSuffix[SC_NUM_TRANSFORM_PREFIX_LEN];     /* ���Ӻ�׺ */

    U32                             ulExpiry;                           /* ��Ч�� */

}SC_NUM_TRANSFORM_NODE_ST;

/***************����任����********************/
/* declare functions */
SC_SCB_ST *sc_scb_alloc();
VOID sc_scb_free(SC_SCB_ST *pstSCB);
U32 sc_scb_init(SC_SCB_ST *pstSCB);
U32 sc_call_set_owner(SC_SCB_ST *pstSCB, U32  ulTaskID, U32 ulCustomID);
U32 sc_call_set_trunk(SC_SCB_ST *pstSCB, U32 ulTrunkID);
SC_TASK_CB_ST *sc_tcb_find_by_taskid(U32 ulTaskID);
SC_SCB_ST *sc_scb_get(U32 ulIndex);
U32 sc_ep_terminate_call(SC_SCB_ST *pstSCB);
U32 sc_ep_outgoing_call_proc(SC_SCB_ST *pstSCB);
U32 sc_ep_incoming_call_proc(SC_SCB_ST *pstSCB);
U32 sc_dialer_make_call2pstn(SC_SCB_ST *pstSCB, U32 ulMainService);
SC_TASK_CB_ST *sc_tcb_alloc();
VOID sc_tcb_free(SC_TASK_CB_ST *pstTCB);
U32 sc_tcb_init(SC_TASK_CB_ST *pstTCB);
VOID sc_task_set_owner(SC_TASK_CB_ST *pstTCB, U32 ulTaskID, U32 ulCustomID);
U32 sc_task_get_current_call_cnt(SC_TASK_CB_ST *pstTCB);
U32 sc_task_load_caller(SC_TASK_CB_ST *pstTCB);
U32 sc_task_load_callee(SC_TASK_CB_ST *pstTCB);
U32 sc_task_load_period(SC_TASK_CB_ST *pstTCB);
U32 sc_task_load_agent_info(SC_TASK_CB_ST *pstTCB);
S32 sc_task_load_other_info(SC_TASK_CB_ST *pstTCB);
U32 sc_task_update_stat(SC_TASK_CB_ST *pstTCB);
U32 sc_task_save_status(U32 ulTaskID, U32 ulStatus, S8 *pszStatus);
U32 sc_task_check_can_call_by_time(SC_TASK_CB_ST *pstTCB);
U32 sc_task_check_can_call_by_status(SC_TASK_CB_ST *pstTCB);
U32 sc_task_get_call_interval(SC_TASK_CB_ST *pstTCB);
U32 sc_task_set_recall(SC_TASK_CB_ST *pstTCB);
U32 sc_task_cmd_queue_add(SC_TASK_CTRL_CMD_ST *pstCMD);
U32 sc_task_cmd_queue_del(SC_TASK_CTRL_CMD_ST *pstCMD);
S8 *sc_task_get_audio_file(U32 ulTCBNo);
U32 sc_task_audio_playcnt(U32 ulTCBNo);
U32 sc_task_get_mode(U32 ulTCBNo);
U32 sc_task_get_timeout_for_noanswer(U32 ulTCBNo);
U32 sc_task_get_agent_queue(U32 ulTCBNo);
U32 sc_dialer_add_call(SC_SCB_ST *pstSCB);
U32 sc_task_concurrency_minus (U32 ulTCBNo);
U32 sc_task_concurrency_add(U32 ulTCBNo);
VOID sc_call_trace(SC_SCB_ST *pstSCB, const S8 *szFormat, ...);
U32 sc_task_callee_set_recall(SC_TASK_CB_ST *pstTCB, U32 ulIndex);
U32 sc_task_load_audio(SC_TASK_CB_ST *pstTCB);
BOOL sc_scb_is_valid(SC_SCB_ST *pstSCB);
U32 sc_task_init(SC_TASK_CB_ST *pstTCB);
BOOL sc_call_check_service(SC_SCB_ST *pstSCB, U32 ulService);
U32 sc_task_continue(SC_TASK_CB_ST *pstTCB);
U32 sc_task_pause(SC_TASK_CB_ST *pstTCB);
U32 sc_task_start(SC_TASK_CB_ST *pstTCB);
U32 sc_task_stop(SC_TASK_CB_ST *pstTCB);
S8 *sc_scb_get_status(U32 ulStatus);
SC_SYS_STATUS_EN sc_check_sys_stat();
U32 sc_ep_search_route(SC_SCB_ST *pstSCB);
U32 sc_ep_get_callee_string(U32 ulRouteID, SC_SCB_ST *pstSCB, S8 *szCalleeString, U32 ulLength);
U32 sc_get_record_file_path(S8 *pszBuff, U32 ulMaxLen, U32 ulCustomerID, S8 *pszCaller, S8 *pszCallee);
U32 sc_dial_make_call_for_verify(U32 ulCustomer, S8 *pszCaller, S8 *pszNumber, S8 *pszPassword, U32 ulPlayCnt);
U32 sc_send_usr_auth2bs(SC_SCB_ST *pstSCB);
U32 sc_send_billing_stop2bs(SC_SCB_ST *pstSCB);
U32 sc_http_gateway_update_proc(U32 ulAction, U32 ulGatewayID);
U32 sc_http_caller_update_proc(U32 ulAction, U32 ulCallerID);
U32 sc_http_eix_update_proc(U32 ulAction, U32 ulEixID);
U32 sc_http_num_lmt_update_proc(U32 ulAction, U32 ulNumlmtID);
U32 sc_http_num_transform_update_proc(U32 ulAction, U32 ulNumTransID);
U32 sc_http_customer_update_proc(U32 ulAction, U32 ulCustomerID);
U32 sc_http_sip_update_proc(U32 ulAction, U32 ulSipID, U32 ulCustomerID);
U32 sc_http_route_update_proc(U32 ulAction, U32 ulRouteID);
U32 sc_http_gw_group_update_proc(U32 ulAction, U32 ulGwGroupID);
U32 sc_http_did_update_proc(U32 ulAction, U32 ulDidID);
U32 sc_ep_update_sip_status(S8 *szUserID, SC_STATUS_TYPE_EN enStatus, U32 *pulSipID);
U32 sc_gateway_delete(U32 ulGatewayID);
U32 sc_load_sip_userid(U32 ulIndex);
U32 sc_load_gateway(U32 ulIndex);
U32 sc_load_route(U32 ulIndex);
U32 sc_route_delete(U32 ulRouteID);
U32 sc_load_gateway_grp(U32 ulIndex);
U32 sc_refresh_gateway_grp(U32 ulIndex);
U32 sc_load_did_number(U32 ulIndex);
U32 sc_load_black_list(U32 ulIndex);
U32 sc_load_num_transform(U32 ulIndex);
U32 sc_load_tt_number(U32 ulIndex);
U32 sc_del_tt_number(U32 ulIndex);
U32 sc_load_number_lmt(U32 ulIndex);
U32 sc_del_number_lmt(U32 ulIndex);
U32 sc_load_customer(U32 ulIndex);
U32 sc_load_caller_setting(U32 ulIndex);
U32 sc_load_caller_grp(U32 ulIndex);
U32 sc_load_caller(U32 ulIndex);
U32 sc_gateway_grp_delete(U32 ulGwGroupID);
U32 sc_black_list_delete(U32 ulBlackListID);
U32 sc_caller_grp_delete(U32 ulCallerGrpID);
U32 sc_did_delete(U32 ulDidID);
U32 sc_caller_delete(U32 ulCallerID);
U32 sc_caller_setting_delete(U32 ulSettingID);
U32 sc_ep_sip_userid_delete(S8 * pszSipID);
U32 sc_caller_delete(U32 ulCallerID);
U32 sc_transform_delete(U32 ulTransformID);
U32 sc_customer_delete(U32 ulCustomerID);
U32 sc_http_black_update_proc(U32 ulAction, U32 ulBlackID);
U32 sc_del_invalid_gateway();
U32 sc_del_invalid_gateway_grp();
U32 sc_del_invalid_route();
U32 sc_ep_esl_execute(const S8 *pszApp, const S8 *pszArg, const S8 *pszUUID);
U32 sc_ep_esl_execute_cmd(const S8* pszCmd);
U32 sc_ep_get_userid_by_id(U32 ulSipID, S8 *pszUserID, U32 ulLength);
S32 sc_ep_gw_grp_hash_find(VOID *pObj, HASH_NODE_S *pstHashNode);
S32 sc_ep_caller_hash_find(VOID *pObj, HASH_NODE_S *pstHashNode);
S32 sc_ep_caller_setting_hash_find(VOID *pObj, HASH_NODE_S *pstHashNode);
S32 sc_ep_caller_grp_hash_find(VOID *pObj, HASH_NODE_S *pstHashNode);
U32 sc_ep_gw_grp_hash_func(U32 ulGWGrpID);
U32 sc_ep_caller_hash_func(U32 ulCustomerID);
U32 sc_ep_caller_grp_hash_func(U32 ulCustomerID);
U32 sc_ep_caller_setting_hash_func(U32 ulCustomerID);
U32 sc_ep_esl_execute(const S8 *pszApp, const S8 *pszArg, const S8 *pszUUID);
U32 sc_ep_hangup_call(SC_SCB_ST *pstSCB, U32 ulTernmiteCase);
BOOL sc_ep_black_list_check(U32 ulCustomerID, S8 *pszNum);
U32 sc_ep_call_agent_by_grpid(SC_SCB_ST *pstSCB, U32 ulTaskAgentQueueID);
U32 sc_update_callee_status(U32 ulTaskID, S8 *pszCallee, U32 ulStatsu);
U32 sc_update_task_status(U32 ulTaskID,  U32 ulStatsu);
U32 sc_ep_ext_init();
U32 sc_cwq_init();
U32 sc_cwq_start();
U32 sc_cwq_stop();
U32 sc_cwq_add_call(SC_SCB_ST *pstSCB, U32 ulAgentGrpID);
U32 sc_cwq_del_call(SC_SCB_ST *pstSCB);
U32 sc_bg_job_hash_add(S8 *pszUUID, U32 ulUUIDLen, U32 ulRCNo);
U32 sc_bg_job_hash_delete(U32 ulRCNo);
BOOL sc_bg_job_find(U32 ulRCNo);
U32 sc_scb_hash_tables_add(S8 *pszUUID, SC_SCB_ST *pstSCB);
U32 sc_scb_hash_tables_delete(S8 *pszUUID);
SC_SCB_ST *sc_scb_hash_tables_find(S8 *pszUUID);
U32 sc_ep_call_ctrl_proc(U32 ulAction, U32 ulTaskID, U32 ulAgent, U32 ulCustomerID, S8 *pszCallee);
U32 sc_ep_get_custom_by_sip_userid(S8 *pszNum);
BOOL sc_ep_check_extension(S8 *pszNum, U32 ulCustomerID);
U32 sc_dial_make_call2ip(SC_SCB_ST *pstSCB, U32 ulMainService);
U32 sc_ep_num_transform(SC_SCB_ST *pstSCB, U32 ulTrunkID, SC_NUM_TRANSFORM_TIMING_EN enTiming, SC_NUM_TRANSFORM_SELECT_EN enNumSelect);
U32 sc_ep_get_eix_by_tt(S8 *pszTTNumber, S8 *pszEIX, U32 ulLength);
U32 sc_dial_make_call2eix(SC_SCB_ST *pstSCB, U32 ulMainService);
U32 sc_ep_transfer_publish_release(SC_SCB_ST * pstSCBPublish);

/* �����Ǻͺ������趨��ص�API */
U32  sc_caller_setting_select_number(U32 ulCustomerID, U32 ulSrcID, U32 ulSrcType, U32 ulPolicy, S8 *pszNumber, U32 ulLen);

/* �������� */
U32 sc_num_lmt_stat(U32 ulType, VOID *ptr);
U32 sc_num_lmt_update(U32 ulType, VOID *ptr);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SC_TASK_PUB_H__ */

