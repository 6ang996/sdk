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

/* ���ų������ֵ */
#define SC_EMP_NUMBER_LENGTH           8

/* ÿ���������ʱ��������ڵ� */
#define SC_MAX_PERIOD_NUM              4

/* UUID ��󳤶� */
#define SC_MAX_UUID_LENGTH             40

/* �����ļ������� */
#define SC_MAX_AUDIO_FILENAME_LEN      128

/* �������еı��� */
#define SC_MAX_CALL_MULTIPLE           3

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

typedef enum tagSiteAccompanyingStatus
{
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

typedef enum tagSysStatus
{
    SC_SYS_NORMAL                      = 3,
    SC_SYS_BUSY                        = 4,       /* ����״̬��ϵͳæ����ͣ��������80%���ϵ���������� */
    SC_SYS_ALERT,                                 /* ����״̬��ϵͳæ��ֻ��������ȼ��ͻ������Һ�������80%���µĿͻ�������� */
    SC_SYS_EMERG,                                 /* ����״̬��ϵͳæ����ͣ�������� */

    SC_SYS_BUTT                        = 255      /* ����״̬���Ƿ�ֵ */
}SC_SYS_STATUS_EN;

typedef enum tagTaskStatusInDB
{
    SC_TASK_STATUS_DB_START            = 0,       /* ���ݿ�������״̬ */
    SC_TASK_STATUS_DB_STOP,                       /* ���ݿ�������״̬ */

    SC_TASK_STATUS_DB_BUTT
}SC_TASK_STATUS_DB_EN;

typedef enum tagTaskStatus
{
    SC_TASK_INIT                       = 0,       /* ����״̬����ʼ�� */
    SC_TASK_WORKING,                              /* ����״̬������ */
    SC_TASK_STOP,                                 /* ����״̬��ֹͣ�����ٷ�����У�������к��н��������ͷ���Դ */
    SC_TASK_PAUSED,                               /* ����״̬����ͣ */
    SC_TASK_SYS_BUSY,                             /* ����״̬��ϵͳæ����ͣ��������80%���ϵ���������� */
    SC_TASK_SYS_ALERT,                            /* ����״̬��ϵͳæ��ֻ��������ȼ��ͻ������Һ�������80%���µĿͻ�������� */
    SC_TASK_SYS_EMERG,                            /* ����״̬��ϵͳæ����ͣ�������� */

    SC_TASK_BUTT                       = 255      /* ����״̬���Ƿ�ֵ */
}SC_TASK_STATUS_EN;

typedef enum tagTaskPriority
{
    SC_TASK_PRI_LOW                       = 0,    /* �������ȼ��������ȼ� */
    SC_TASK_PRI_NORMAL,                           /* �������ȼ����������ȼ� */
    SC_TASK_PRI_HIGHT,                            /* �������ȼ��������ȼ� */
}SC_TASK_PRI_EN;

typedef enum tagCallNumType
{
    SC_CALL_NUM_TYPE_NORMOAL              = 0,       /* �������ͣ������ĺ��� */
    SC_CALL_NUM_TYPE_EXPR,                           /* �������ͣ�������ʽ */

    SC_CALL_NUM_TYPE_BUTT                 = 255      /* �������ͣ��Ƿ�ֵ */
}SC_CALL_NUM_TYPE_EN;


typedef enum tagSCBStatus
{
    SC_SCB_IDEL                           = 0,     /* SCB״̬������״̬ */
    SC_SCB_INIT,                                   /* SCB״̬�����г�ʼ��״̬ */
    SC_SCB_AUTH,                                   /* SCB״̬��������֤״̬ */
    SC_SCB_EXEC,                                   /* SCB״̬������ִ��״̬ */
    SC_SCB_ACTIVE,                                 /* SCB״̬�����н�ͨ״̬״̬ */
    SC_SCB_RELEASE,                                /* SCB״̬�������ͷ�״̬ */

    SC_SCB_BUTT
}SC_SCB_STATUS_EN;

typedef struct tagCallerQueryNode
{
    U16        usNo;                              /* ��� */
    U8         bValid;
    U8         bTraceON;                          /* �Ƿ���� */

    U32        ulIndexInDB;                       /* ���ݿ��е�ID */

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

typedef struct tagTaskAllowPeriod
{
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


/* ���п��ƿ� */
typedef struct tagSCSCB
{
    U16       usSCBNo;                            /* ��� */
    U16       usOtherSCBNo;                       /* ����һ��leg��SCB��� */

    U16       usTCBNo;                            /* ������ƿ���ID */
    U16       usSiteNo;                           /* ��ϯ��� */

    U32       ulCustomID;                         /* ��ǰ���������ĸ��ͻ� */
    U32       ulAgentID;                          /* ��ǰ���������ĸ��ͻ� */
    U32       ulTaskID;                           /* ��ǰ����ID */
    U32       ulTrunkID;                          /* �м�ID */

    U8        ucStatus;                           /* ���п��ƿ��ţ�refer to SC_SCB_STATUS_EN */
    U8        ucServStatus;                       /* ҵ��״̬ */
    U8        ucTerminationFlag;                  /* ҵ����ֹ��־ */
    U8        ucTerminationCause;                 /* ҵ����ֹԭ�� */

    U8        aucServiceType[SC_MAX_SRV_TYPE_PRE_LEG];        /* ҵ������ �б�*/
    U8        ucCurrentSrvInd;                    /* ��ǰ���е�ҵ���������� */
    U8        ucLegRole;                          /* �����б�ʾ */
    U8        ucCurrentPlyCnt;                    /* ��ǰ�������� */
    U8        aucRes[1];

    U16       usHoldCnt;                          /* ��hold�Ĵ��� */
    U16       usHoldTotalTime;                    /* ��hold����ʱ�� */
    U32       ulLastHoldTimetamp;                 /* �ϴ�hold�ǵ�ʱ��������hold��ʱ��ֵ�� */

    U32       bValid:1;                           /* �Ƿ�Ϸ� */
    U32       bTraceNo:1;                         /* �Ƿ���� */
    U32       bBanlanceWarning:1;                 /* �Ƿ����澯 */
    U32       bNeedConnSite:1;                    /* ��ͨ���Ƿ���Ҫ��ͨ��ϯ */
    U32       bWaitingOtherRelase:1;              /* �Ƿ��ڵȴ�����һ�����ͷ� */
    U32       ulRes:27;

    U32       ulCallDuration;                     /* ����ʱ������ֹ�����ã�ÿ������ʱ���� */

    U32       ulStartTimeStamp;                   /* ��ʼʱ��� */
    U32       ulRingTimeStamp;                    /* ����ʱ��� */
    U32       ulAnswerTimeStamp;                  /* Ӧ��ʱ��� */
    U32       ulIVRFinishTimeStamp;               /* IVR�������ʱ��� */
    U32       ulDTMFTimeStamp;                    /* (��һ��)���β���ʱ��� */
    U32       ulBridgeTimeStamp;                  /* LEG�Ž�ʱ��� */
    U32       ulByeTimeStamp;                     /* �ͷ�ʱ��� */

    U32       ulRes1;

    S8        szCallerNum[SC_TEL_NUMBER_LENGTH];  /* ���к��� */
    S8        szCalleeNum[SC_TEL_NUMBER_LENGTH];  /* ���к��� */
    S8        szANINum[SC_TEL_NUMBER_LENGTH];     /* ���к��� */
    S8        szDialNum[SC_TEL_NUMBER_LENGTH];    /* �û����� */
    S8        szSiteNum[SC_TEL_NUMBER_LENGTH];    /* ��ϯ���� */
    S8        szUUID[SC_MAX_UUID_LENGTH];         /* Leg-A UUID */

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
    U8         ucPriority;                        /* �������ȼ� */
    U8         ucAudioPlayCnt;                    /* ���Բ��Ŵ��� */
    U8         bTraceON;                          /* �Ƿ���� */
    U8         bTraceCallON;                      /* �Ƿ���ٺ��� */

    pthread_t  pthID;                             /* �߳�ID */
    pthread_mutex_t  mutexTaskList;               /* �����������ʹ�õĻ����� */

    U32        ulTaskID;                          /* ��������ID */
    U32        ulCustomID;                        /* ������������ */
    U32        ulConcurrency;                     /* ��ǰ������ */
    U32        ulAgentQueueID;                    /* ��ϯ���б�� */

    U16        usSiteCount;                       /* ��ϯ���� */
    U16        usCallerCount;                     /* ��ǰ���к������� */
    U32        ulLastCalleeIndex;                 /* �������ݷ�ҳ */

    list_t     stCalleeNumQuery;                  /* ���к��뻺�� refer to struct tagTelNumQueryNode */
    S8         szAudioFileLen[SC_MAX_AUDIO_FILENAME_LEN];  /* �����ļ��ļ��� */
    SC_CALLER_QUERY_NODE_ST *pstCallerNumQuery;            /* ���к��뻺�� refer to struct tagTelNumQueryNode */
    SC_TASK_ALLOW_PERIOD_ST astPeriod[SC_MAX_PERIOD_NUM];  /* ����ִ��ʱ��� */

    /* ͳ����� */
    U32        ulTotalCall;                       /* �ܺ����� */
    U32        ulCallFailed;                      /* ����ʧ���� */
    U32        ulCallConnected;                   /* ���н�ͨ�� */
}SC_TASK_CB_ST;


typedef struct tagTaskCtrlCMD
{
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


typedef struct tagTaskMngtInfo
{
    pthread_t            pthID;                   /* �߳�ID */
    pthread_mutex_t      mutexCMDList;            /* �����������ʹ�õĻ����� */
    pthread_mutex_t      mutexTCBList;            /* ����������ƿ�ʹ�õĻ����� */
    pthread_mutex_t      mutexCallList;           /* �������п��ƿ�ʹ�õĻ����� */
    pthread_mutex_t      mutexCallHash;           /* �������п��ƿ�ʹ�õĻ����� */
    pthread_cond_t       condCMDList;             /* ����������ݵ���֪ͨ������ */
    U32                  blWaitingExitFlag;       /* �ȴ��˳���ʾ */

    list_t               stCMDList;               /* �������(�ڵ���HTTP Server��������HTTP Server�ͷ�) */
    SC_SCB_ST            *pstCallSCBList;         /* ���п��ƿ��б� (��Ҫhash��洢) */
    HASH_TABLE_S         *pstCallSCBHash;         /* ���п��ƿ��hash���� */
    SC_TASK_CB_ST        *pstTaskList;            /* �����б� refer to struct tagTaskCB*/
    U32                  ulTaskCount;             /* ��ǰ����ִ�е������� */

    U32                  ulMaxCall;               /* ��ʷ�����в����� */

    SC_SYS_STATUS_EN     enSystemStatus;          /* ϵͳ״̬ */
}SC_TASK_MNGT_ST;


/* declare functions */
SC_SCB_ST *sc_scb_alloc();
VOID sc_scb_free(SC_SCB_ST *pstSCB);
U32 sc_scb_init(SC_SCB_ST *pstSCB);
U32 sc_call_set_owner(SC_SCB_ST *pstSCB, U32  ulTaskID, U32 ulCustomID);
U32 sc_call_set_trunk(SC_SCB_ST *pstSCB, U32 ulTrunkID);
SC_TASK_CB_ST *sc_tcb_find_by_taskid(U32 ulTaskID);
SC_SCB_ST *sc_scb_get(U32 ulIndex);

SC_TASK_CB_ST *sc_tcb_alloc();
VOID sc_tcb_free(SC_TASK_CB_ST *pstTCB);
U32 sc_tcb_init(SC_TASK_CB_ST *pstTCB);
VOID sc_task_set_owner(SC_TASK_CB_ST *pstTCB, U32 ulTaskID, U32 ulCustomID);
VOID sc_task_set_current_call_cnt(SC_TASK_CB_ST *pstTCB, U32 ulCurrentCall);
U32 sc_task_get_current_call_cnt(SC_TASK_CB_ST *pstTCB);
S32 sc_task_load_caller(SC_TASK_CB_ST *pstTCB);
S32 sc_task_load_callee(SC_TASK_CB_ST *pstTCB);
U32 sc_task_load_period(SC_TASK_CB_ST *pstTCB);
U32 sc_task_load_agent_info(SC_TASK_CB_ST *pstTCB);
S32 sc_task_load_other_info(SC_TASK_CB_ST *pstTCB);
U32 sc_task_update_stat(SC_TASK_CB_ST *pstTCB);
U32 sc_task_check_can_call_by_time(SC_TASK_CB_ST *pstTCB);
U32 sc_task_check_can_call_by_status(SC_TASK_CB_ST *pstTCB);
U32 sc_task_get_call_interval(SC_TASK_CB_ST *pstTCB);
U32 sc_task_set_recall(SC_TASK_CB_ST *pstTCB);
U32 sc_task_cmd_queue_add(SC_TASK_CTRL_CMD_ST *pstCMD);
U32 sc_task_cmd_queue_del(SC_TASK_CTRL_CMD_ST *pstCMD);
U32 sc_task_audio_playcnt(U32 ulTCBNo);
U32 sc_task_get_timeout_for_noanswer(U32 ulTCBNo);
U32 sc_dialer_add_call(SC_SCB_ST *pstSCB);
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
U32 sc_scb_hash_tables_add(S8 *pszUUID, SC_SCB_ST *pstSCB);
U32 sc_scb_hash_tables_delete(S8 *pszUUID);
SC_SCB_ST *sc_scb_hash_tables_find(S8 *pszUUID);
U32 sc_scb_syn_post(S8 *pszUUID);
U32 sc_scb_syn_wait(S8 *pszUUID);

SC_SYS_STATUS_EN sc_check_sys_stat();

SC_SCB_ST *sc_scb_hash_tables_find(S8 *pszUUID);
U32 sc_scb_hash_tables_delete(S8 *pszUUID);
U32 sc_scb_hash_tables_add(S8 *pszUUID, SC_SCB_ST *pstSCB);




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SC_TASK_PUB_H__ */

