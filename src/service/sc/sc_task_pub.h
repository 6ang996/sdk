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

/* ���һ��TCB����������Task��CustomID */
#define SC_TCB_HAS_VALID_OWNER(pstTCB)                        \
    ((pstTCB)                                                 \
    && (pstTCB)->ulTaskID != 0                                \
    && (pstTCB)->ulTaskID != U32_BUTT                         \
    && (pstTCB)->ulCustomID != 0                              \
    && (pstTCB)->ulCustomID != U32_BUTT)

#define SC_CCB_HAS_VALID_OWNER(pstCCB)                        \
    ((pstCCB)                                                 \
    && (pstCCB)->ulTaskID != 0                                \
    && (pstCCB)->ulTaskID != U32_BUTT                         \
    && (pstCCB)->ulCustomID != 0                              \
    && (pstCCB)->ulCustomID != U32_BUTT)


#define SC_TCB_VALID(pstTCB)                                  \
    ((pstTCB)                                                 \
    && (pstTCB)->ulTaskID != 0)

#define SC_CCB_IS_VALID(pstCCB)                               \
    ((pstCCB) && (pstCCB)->bValid)

#define SC_CCB_SET_STATUS(pstCCB, ulStatus)                   \
do                                                            \
{                                                             \
    if (DOS_ADDR_INVALID(pstCCB)                              \
        || ulStatus >= SC_CCB_BUTT)                           \
    {                                                         \
        break;                                                \
    }                                                         \
    pthread_mutex_lock(&(pstCCB)->mutexCCBLock);              \
    sc_call_trace((pstCCB), "CCB Status Change %s -> %s"      \
                , sc_ccb_get_status((pstCCB)->usStatus)       \
                , sc_ccb_get_status(ulStatus));               \
    (pstCCB)->usStatus = ulStatus;                            \
    pthread_mutex_unlock(&(pstCCB)->mutexCCBLock);            \
}while(0)

#define SC_CCB_SET_SERVICE(pstCCB, ulService)                 \
do                                                            \
{                                                             \
    if (DOS_ADDR_INVALID(pstCCB)                              \
        || ulService >= SC_MAX_SRV_TYPE_PRE_LEG)              \
    {                                                         \
        break;                                                \
    }                                                         \
    pthread_mutex_lock(&(pstCCB)->mutexCCBLock);              \
    sc_call_trace((pstCCB), "CCB Add service.");              \
    (pstCCB)->aucServiceType[(pstCCB)->ulCurrentSrvInd++]     \
                = ulService;                                  \
    pthread_mutex_unlock(&(pstCCB)->mutexCCBLock);            \
}while(0)



#define SC_TRACE_HTTPD                  (1<<1)
#define SC_TRACE_HTTP                   (1<<2)
#define SC_TRACE_TASK                   (1<<3)
#define SC_TRACE_SC                     (1<<4)
#define SC_TRACE_ACD                    (1<<5)
#define SC_TRACE_DIAL                   (1<<6)
#define SC_TRACE_FUNC                   (1<<7)
#define SC_TRACE_ALL                    0xFFFFFFFF

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
    SC_TASK_STATUS_DB_START            = 0,       /* ���ݿ�������״̬ */
    SC_TASK_STATUS_DB_STOP,                       /* ���ݿ�������״̬ */

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


typedef enum tagCCBStatus
{
    SC_CCB_IDEL                           = 0,     /* CCB״̬������״̬ */
    SC_CCB_INIT,                                   /* CCB״̬�����г�ʼ��״̬ */
    SC_CCB_AUTH,                                   /* CCB״̬��������֤״̬ */
    SC_CCB_EXEC,                                   /* CCB״̬������ִ��״̬ */
    SC_CCB_ACTIVE,                                 /* CCB״̬�����н�ͨ״̬״̬ */
    SC_CCB_RELEASE,                                /* CCB״̬�������ͷ�״̬ */

    SC_CCB_BUTT
}SC_CCB_STATUS_EN;

typedef struct tagCallerQueryNode{
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


/* ���п��ƿ� */
typedef struct tagSCCCB{
    U16       usCCBNo;                            /* ��� */
    U16       usOtherCCBNo;                       /* ��һ��leg�ı�� */

    U8        ucTerminationFlag;                  /* ҵ����ֹ��־ */
    U8        ucTerminationCause;                 /* ҵ����ֹԭ�� */

    U32       bValid:1;                           /* �Ƿ�Ϸ� */
    U32       bTraceNo:1;                         /* �Ƿ���� */
    U32       bBanlanceWarning:1;                 /* �Ƿ����澯 */
    U32       bNeedConnSite:1;                    /* ��ͨ���Ƿ���Ҫ��ͨ��ϯ */
    U32       bWaitingOtherRelase:1;              /* �Ƿ��ڵȴ�����һ�����ͷ� */
    U32       ulRes:27;

    U16       usTCBNo;                            /* ������ƿ���ID */
    U16       usSiteNo;                           /* ��ϯ��� */
    U16       usCallerNo;                         /* ���к����� */
    U16       usStatus;                           /* ���п��ƿ��ţ�refer to SC_CCB_STATUS_EN */

    U8        aucServiceType[SC_MAX_SRV_TYPE_PRE_LEG];        /* ҵ������ */
    U32       ulCurrentSrvInd;                                /* ��ǰ���е�ҵ���������� */

    U32       ulCustomID;                         /* ��ǰ���������ĸ��ͻ� */
    U32       ulAgentID;                          /* ��ǰ���������ĸ��ͻ� */
    U32       ulTaskID;                           /* ��ǰ����ID */
    U32       ulTrunkID;                          /* �м�ID */
    U32       ulAuthToken;                        /* ��Ʒ�ģ��Ĺ����ֶ� */
    U32       ulCallDuration;                     /* ����ʱ������ֹ�����ã�ÿ������ʱ���� */
    U32       ulOtherLegID;                       /* ����һ��leg��CCB��� */

    U16       usHoldCnt;                          /* ��hold�Ĵ��� */
    U16       usHoldTotalTime;                    /* ��hold����ʱ�� */
    U32       ulLastHoldTimetamp;                 /* �ϴ�hold�ǵ�ʱ��������hold��ʱ��ֵ�� */

    S8        szCallerNum[SC_TEL_NUMBER_LENGTH];  /* ���к��� */
    S8        szCalleeNum[SC_TEL_NUMBER_LENGTH];  /* ���к��� */
    S8        szANINum[SC_TEL_NUMBER_LENGTH];     /* ���к��� */
    S8        szDialNum[SC_TEL_NUMBER_LENGTH];    /* �û����� */
    S8        szSiteNum[SC_TEL_NUMBER_LENGTH];    /* ��ϯ���� */
    S8        szUUID[SC_MAX_UUID_LENGTH];         /* Leg-A UUID */

    U32       ulBSMsgNo;                          /* ��BSͨѶʹ�õ���Դ�� */

    sem_t     semCCBSyn;                          /* ����ͬ����CCB */
    pthread_mutex_t mutexCCBLock;                 /* ����CCB���� */
}SC_CCB_ST;

/* ����CCBhash�� */
typedef struct tagCCBHashNode
{
    HASH_NODE_S     stNode;                       /* hash����ڵ� */

    S8              szUUID[SC_MAX_UUID_LENGTH];   /* UUID */
    SC_CCB_ST       *pstCCB;                      /* CCBָ�� */

    sem_t           semCCBSyn;
}SC_CCB_HASH_NODE_ST;


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

    U16        usSiteCount;                       /* ��ϯ���� */
    U16        usCallerCount;                     /* ��ǰ���к������� */
    U32        ulLastCalleeIndex;                 /* �������ݷ�ҳ */

    list_t     stCalleeNumQuery;                  /* ���к��뻺�� refer to struct tagTelNumQueryNode */
    S8         szAudioFileLen[SC_MAX_AUDIO_FILENAME_LEN];      /* �����ļ��ļ��� */
    SC_CALLER_QUERY_NODE_ST *pstCallerNumQuery;    /* ���к��뻺�� refer to struct tagTelNumQueryNode */
    SC_SITE_QUERY_NODE_ST   *pstSiteQuery;         /* ���к��뻺�� refer to struct tagSiteQueryNode*/
    SC_TASK_ALLOW_PERIOD_ST astPeriod[SC_MAX_PERIOD_NUM];   /* ����ִ��ʱ��� */

    /* ͳ����� */
    U32        ulTotalCall;                       /* �ܺ����� */
    U32        ulCallFailed;                      /* ����ʧ���� */
    U32        ulCallConnected;                   /* ���н�ͨ�� */
}SC_TASK_CB_ST;


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
    pthread_cond_t       condCMDList;             /* ����������ݵ���֪ͨ������ */
    U32                  blWaitingExitFlag;       /* �ȴ��˳���ʾ */

    list_t               stCMDList;               /* �������(�ڵ���HTTP Server��������HTTP Server�ͷ�) */
    SC_CCB_ST            *pstCallCCBList;         /* ���п��ƿ��б� (��Ҫhash��洢) */
    HASH_TABLE_S         *pstCallCCBHash;         /* ���п��ƿ��hash���� */
    SC_TASK_CB_ST        *pstTaskList;            /* �����б� refer to struct tagTaskCB*/
    U32                  ulTaskCount;             /* ��ǰ����ִ�е������� */

    U32                  ulMaxCall;               /* ��ʷ�����в����� */
    DB_HANDLE_ST         *pstDBHandle;            /* ���ݿ��� */

    SC_SYS_STATUS_EN     enSystemStatus;          /* ϵͳ״̬ */
}SC_TASK_MNGT_ST;


/* declare functions */
SC_CCB_ST *sc_ccb_alloc();
VOID sc_ccb_free(SC_CCB_ST *pstCCB);
U32 sc_ccb_init(SC_CCB_ST *pstCCB);
U32 sc_call_set_owner(SC_CCB_ST *pstCCB, U32  ulTaskID, U32 ulCustomID);
U32 sc_call_set_trunk(SC_CCB_ST *pstCCB, U32 ulTrunkID);
U32 sc_call_set_auth_token(SC_CCB_ST *pstCCB, U32 ulToken);
SC_TASK_CB_ST *sc_tcb_find_by_taskid(U32 ulTaskID);
SC_CCB_ST *sc_ccb_get(U32 ulIndex);

SC_TASK_CB_ST *sc_tcb_alloc();
VOID sc_tcb_free(SC_TASK_CB_ST *pstTCB);
U32 sc_tcb_init(SC_TASK_CB_ST *pstTCB);
VOID sc_task_set_owner(SC_TASK_CB_ST *pstTCB, U32 ulTaskID, U32 ulCustomID);
VOID sc_task_set_current_call_cnt(SC_TASK_CB_ST *pstTCB, U32 ulCurrentCall);
U32 sc_task_get_current_call_cnt(SC_TASK_CB_ST *pstTCB);
S32 sc_task_load_site(SC_TASK_CB_ST *pstTCB);
S32 sc_task_load_caller(SC_TASK_CB_ST *pstTCB);
S32 sc_task_load_callee(SC_TASK_CB_ST *pstTCB);
U32 sc_task_load_period(SC_TASK_CB_ST *pstTCB);
U32 sc_task_update_stat(SC_TASK_CB_ST *pstTCB);
U32 sc_task_check_can_call_by_time(SC_TASK_CB_ST *pstTCB);
U32 sc_task_check_can_call_by_status(SC_TASK_CB_ST *pstTCB);
U32 sc_task_get_call_interval(SC_TASK_CB_ST *pstTCB);
U32 sc_task_set_recall(SC_TASK_CB_ST *pstTCB);
U32 sc_task_cmd_queue_add(SC_TASK_CTRL_CMD_ST *pstCMD);
U32 sc_task_cmd_queue_del(SC_TASK_CTRL_CMD_ST *pstCMD);
U32 sc_task_audio_playcnt(U32 ulTCBNo);
U32 sc_task_get_timeout_for_noanswer(U32 ulTCBNo);
U32 sc_dialer_add_call(SC_CCB_ST *pstCCB);
VOID sc_call_trace(SC_CCB_ST *pstCCB, const S8 *szFormat, ...);
U32 sc_task_callee_set_recall(SC_TASK_CB_ST *pstTCB, U32 ulIndex);
U32 sc_task_load_audio(SC_TASK_CB_ST *pstTCB);
BOOL sc_ccb_is_valid(SC_CCB_ST *pstCCB);
U32 sc_task_init(SC_TASK_CB_ST *pstTCB);
BOOL sc_call_check_service(SC_CCB_ST *pstCCB, U32 ulService);

U32 sc_task_continue(SC_TASK_CB_ST *pstTCB);
U32 sc_task_pause(SC_TASK_CB_ST *pstTCB);
U32 sc_task_start(SC_TASK_CB_ST *pstTCB);
U32 sc_task_stop(SC_TASK_CB_ST *pstTCB);
S8 *sc_ccb_get_status(U32 ulStatus);
U32 sc_ccb_hash_tables_add(S8 *pszUUID, SC_CCB_ST *pstCCB);
U32 sc_ccb_hash_tables_delete(S8 *pszUUID);
SC_CCB_ST *sc_ccb_hash_tables_find(S8 *pszUUID);
U32 sc_ccb_syn_post(S8 *pszUUID);
U32 sc_ccb_syn_wait(S8 *pszUUID);

SC_SYS_STATUS_EN sc_check_sys_stat();

SC_CCB_ST *sc_ccb_hash_tables_find(S8 *pszUUID);
U32 sc_ccb_hash_tables_delete(S8 *pszUUID);
U32 sc_ccb_hash_tables_add(S8 *pszUUID, SC_CCB_ST *pstCCB);




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SC_TASK_PUB_H__ */

