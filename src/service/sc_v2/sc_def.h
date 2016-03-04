/**
 * @file : sc_def.h
 *
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 * ����������ݽṹ
 *
 *
 * @date: 2016��1��9��
 * @arthur: Larry
 */

#ifdef __cplusplus
extern "C" {
#endif /* End of __cplusplus */

#ifndef __SC_DEF_V2_H__
#define __SC_DEF_V2_H__

/**
 * ��������
 * SU -- ҵ��Ԫ
 * SC -- ҵ�����
 * SC_LEG -- ҵ���Ӳ���ص���
 */

/* ���������� */
#define SC_MAX_CALL                  3000

/** ������λ�� */
#define SC_NUM_LENGTH                32

/** UUID���� */
#define SC_UUID_LENGTH               44

/** ¼���ļ����� */
#define SC_RECORD_FILENAME_LENGTH    128

/** ���������ļ����г��� */
#define SC_MAX_PLAY_QUEUE            32

/** ������֤����󳤶� */
#define SC_VOICE_VERIFY_CODE_LENGTH  8

/** ��������󲦺ų��� */
#define SC_MAX_ACCESS_CODE_LENGTH    40

/** LEG���ƿ����� */
#define SC_LEG_CB_SIZE               6000

/** SCB���ƿ����� */
#define SC_SCB_SIZE                  3000

/** ���������� */
#define SC_MAX_CODEC_NUM             8

/** ����м̸��� */
#define SC_MAX_TRUCK_NUM             20

/** ���η����������������� */
#define SC_MAX_AUDIO_NUM             48

/** IP��ַ��󳤶� */
#define SC_MAX_IP_LEN                24

/** ���ҵ����� */
#define SC_MAX_SERVICE_TYPE          4

#define SC_MAX_DTMF_INTERVAL         3

/* ���н�����ʾ�����Ŵ��� */
#define SC_CALL_HIT_LOOP             3

/* ���н�����ʾ�����ż�� MS */
#define SC_CALL_HIT_INTERVAL         500

/* ���н�����ʾ�����ż�� MS */
#define SC_CALL_HIT_SILENCE          300

/* ���ų������ֵ */
#define SC_EMP_NUMBER_LENGTH         12

#define SC_ACD_GROUP_NAME_LEN        24

#define SC_ACD_HASH_SIZE             128

#define SC_ACD_CALLER_NUM_RELATION_HASH_SIZE    512         /* ���к������ϯ��Ӧ��ϵhash�Ĵ�С */


/* ������ϯ������������ĸ��� */
#define MAX_GROUP_PER_SITE           2


#define SC_NOBODY_UID                99
#define SC_NOBODY_GID                99

#define SC_UUID_HASH_LEN             18

#define SC_BG_JOB_HASH_SIZE          1024
#define SC_UUID_HASH_SIZE            1024

/* ������ϯ������������ĸ��� */
#define SC_MAX_FILELIST_LEN          4096

/* ���������16���ͻ�������HTTP������ */
#define SC_MAX_HTTP_CLIENT_NUM         16

/* ����HTTP API������������ */
#define SC_API_PARAMS_MAX_NUM          24

/* ����HTTP�������У��ļ�������󳤶� */
#define SC_HTTP_REQ_MAX_LEN            64

/* ����HTTP������������ */
#define SC_MAX_HTTPD_NUM               1

#define SC_NUM_VERIFY_TIME             3          /* ������֤�벥�Ŵ��� */
#define SC_NUM_VERIFY_TIME_MAX         10         /* ������֤�벥�Ŵ��� */
#define SC_NUM_VERIFY_TIME_MIN         2          /* ������֤�벥�Ŵ��� */

#define SC_MAX_CALL_PRE_SEC            120

#define SC_TASK_AUDIO_PATH             "/home/ipcc/data/audio"

#define SC_RECORD_FILE_PATH            "/home/ipcc/data/voicerecord"

/* ÿ���������ʱ��������ڵ� */
#define SC_MAX_PERIOD_NUM              4

/* �����ļ������� */
#define SC_MAX_AUDIO_FILENAME_LEN      128

/* ���н���ͬ��ʱ�� */
#define SC_TASK_UPDATE_DB_TIMER        2

/* ��������� */
#define SC_MAX_TASK_NUM                3000

/* ����Ⱥ��������󲢷��� */
#define SC_MAX_TASK_MAX_CONCURRENCY    80

#define SC_ACCESS_CODE_LEN             8

#define SC_DEMOE_TASK_COUNT            3
#define SC_DEMOE_TASK_FILE             "/usr/local/freeswitch/sounds/okcc/CC_demo.wav"

/**
 * 1. û�б�ɾ��
 * 2. �Ѿ���½��     && (pstSiteDesc)->bLogin ���״̬���ж���
 * 3. ��Ҫ���ӣ����Ҵ�������״̬
 * 4. ״̬ΪEDL
 */
#define SC_ACD_SITE_IS_USEABLE(pstSiteDesc)                             \
            (DOS_ADDR_VALID(pstSiteDesc)                                \
            && !(pstSiteDesc)->bWaitingDelete                           \
            && SC_ACD_WORK_IDEL == (pstSiteDesc)->ucWorkStatus          \
            && SC_ACD_SERV_IDEL == (pstSiteDesc)->ucServStatus          \
            && !(pstSiteDesc)->bSelected)

enum {
    ACD_MSG_TYPE_CALL_NOTIFY   = 0,
    ACD_MSG_TYPE_STATUS        = 1,
    ACD_MSG_TYPE_QUERY         = 2,

    ACD_MSG_TYPE_BUTT
};

enum {
    ACD_MSG_SUBTYPE_LOGIN      = 1,
    ACD_MSG_SUBTYPE_LOGINOUT   = 2,
    ACD_MSG_SUBTYPE_IDLE       = 3,
    ACD_MSG_SUBTYPE_AWAY       = 4,
    ACD_MSG_SUBTYPE_BUSY       = 5,
    ACD_MSG_SUBTYPE_SIGNIN     = 6,
    ACD_MSG_SUBTYPE_SIGNOUT    = 7,
    ACD_MSG_SUBTYPE_CALLIN     = 8,
    ACD_MSG_SUBTYPE_CALLOUT    = 9,
    ACD_MSG_SUBTYPE_PROC       = 10,
    ACD_MSG_SUBTYPE_RINGING    = 11,

    ACD_MSG_TYPE_QUERY_STATUS  = 12,

    ACD_MSG_SUBTYPE_BUTT
};

enum {
    MSG_CALL_STATE_CONNECTING  = 0,
    MSG_CALL_STATE_CONNECTED   = 1,
    MSG_CALL_STATE_HOLDING     = 2,
    MSG_CALL_STATE_TRANSFER    = 3,
    MSG_CALL_STATE_HUNGUP      = 4,
};

/* IP��ϯ:
* 1. ��ʼ��ΪOFFLINE״̬
* 2. ��½֮��ʹ���AWAY״̬/��ϯ��æҲ����AWAY״̬
* 3. ��ϯ���оʹ���IDEL״̬
* 4. ��ͨ���оʹ���BUSY״̬
*/
enum {
    SC_ACD_WORK_OFFLINE     = 0,             /* ���� */
    SC_ACD_WORK_IDEL,                        /* ���� */
    SC_ACD_WORK_BUSY,                        /* æ */
    SC_ACD_WORK_AWAY,                        /* �뿪 */

    SC_ACD_WORK_BUTT
};

enum {
    SC_ACD_SERV_IDEL        = 0,              /* ���� */
    SC_ACD_SERV_CALL_OUT,                     /* ���� */
    SC_ACD_SERV_CALL_IN,                      /* ���� */
    SC_ACD_SERV_RINGING,                      /* ���� */
    SC_ACD_SERV_RINGBACK,                     /* ���� */
    SC_ACD_SERV_PROC,                         /* ���� */

    SC_ACD_SERV_BUTT
};

/*
* ��ϯ����״̬
* û�к���ʱ����NONE, ������CALLIN����CALLOUT�ɵ�ǰ��ϯ���еĶԶ˾�����
* ����Զ��Ǳ��������绰���Ǻ���������Ϊ����
* Ԥ�������Ⱥ�����񣬺�����ϯ���Ǻ���
* �ͻ����룬������ϯ�绰���Ǻ���
*/
enum {
    SC_ACD_CALL_NONE = 0,  /* û�к��� */
    SC_ACD_CALL_IN,        /* ���� */
    SC_ACD_CALL_OUT,       /* ���� */

    SC_ACD_CALL_BUTT
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

    SC_API_CMD_ACTION_QUERY,

    SC_ACD_SITE_ACTION_BUTT              /* ��ϯǩ��(����) */
};

enum {
    SC_ACTION_AGENT_BUSY,
    SC_ACTION_AGENT_IDLE,
    SC_ACTION_AGENT_REST,
    SC_ACTION_AGENT_SIGNIN,
    SC_ACTION_AGENT_SIGNOUT,
    SC_ACTION_AGENT_CALL,
    SC_ACTION_AGENT_HANGUP,
    SC_ACTION_AGENT_HOLD,
    SC_ACTION_AGENT_UNHOLD,
    SC_ACTION_AGENT_TRANSFER,
    SC_ACTION_AGENT_ATRANSFER,
    SC_ACTION_AGENT_LOGIN,
    SC_ACTION_AGENT_LOGOUT,
    SC_ACTION_AGENT_FORCE_OFFLINE,

    SC_ACTION_AGENT_QUERY,

    SC_ACTION_AGENT_BUTT
};

enum {
    SC_ACTION_CALL_TYPE_CLICK,
    SC_ACTION_CALL_TYPE_CALL_NUM,
    SC_ACTION_CALL_TYPE_CALL_AGENT,

    SC_ACTION_CALL_TYPE_BUTT
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

enum tagOperatingType{
    OPERATING_TYPE_WEB,
    OPERATING_TYPE_PHONE,
    OPERATING_TYPE_CHECK
};

enum tagAgentStat{
    SC_AGENT_STAT_SELECT,
    SC_AGENT_STAT_CALL,
    SC_AGENT_STAT_CALL_OK,
    SC_AGENT_STAT_CALL_FINISHED,
    SC_AGENT_STAT_CALL_IN,
    SC_AGENT_STAT_CALL_OUT,
    SC_AGENT_STAT_ONLINE,
    SC_AGENT_STAT_OFFLINE,
    SC_AGENT_STAT_SIGNIN,
    SC_AGENT_STAT_SIGNOUT,

    SC_AGENT_STAT_BUTT
};

enum tagSCLegDirection{
    SC_DIRECTION_PSTN,                      /* ���з���������PSTN */
    SC_DIRECTION_SIP,                       /* ���з���������SIP UA */

    SC_DIRECTION_INVALID                    /* �Ƿ�ֵ */
}SC_LEG_DIRACTION_EN;

typedef enum tagTaskStatus{
    SC_TASK_INIT                       = 0,       /* ����״̬����ʼ�� */
    SC_TASK_WORKING,                              /* ����״̬������ */
    SC_TASK_PAUSED,                               /* ����״̬����ͣ */
    SC_TASK_STOP,                                 /* ����״̬��ֹͣ�����ٷ�����У�������к��н��������ͷ���Դ */

    SC_TASK_SYS_BUSY,                             /* ����״̬��ϵͳæ����ͣ��������80%���ϵ���������� */
    SC_TASK_SYS_ALERT,                            /* ����״̬��ϵͳæ��ֻ��������ȼ��ͻ������Һ�������80%���µĿͻ�������� */
    SC_TASK_SYS_EMERG,                            /* ����״̬��ϵͳæ����ͣ�������� */

    SC_TASK_BUTT                       = 255      /* ����״̬���Ƿ�ֵ */
}SC_TASK_STATUS_EN;

/* ���ݿ�������״̬ */
typedef enum tagTaskStatusInDB{
    SC_TASK_STATUS_DB_INIT             = 0,/* ����(δ��ʼ) */
    SC_TASK_STATUS_DB_START,               /* ��ʼ */
    SC_TASK_STATUS_DB_PAUSED,              /* ��ͣ */
    SC_TASK_STATUS_DB_STOP,                /* ���� */

    SC_TASK_STATUS_DB_BUTT
}SC_TASK_STATUS_DB_EN;

typedef enum tagTaskMode{
    SC_TASK_MODE_KEY4AGENT           = 0,         /* ��������ģʽ���������������֮��ת��ϯ */
    SC_TASK_MODE_KEY4AGENT1          = 1,         /* ��������ģʽ�����������ض���0֮��ת��ϯ */
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


typedef enum tagSCRTPPayloadType{
    PT_PCMU            = 0,
    PT_G723            = 4,
    PT_PCMA            = 8,
    PT_G729            = 18,
}SC_RTP_PAYLOAD_TYPE_EN;

/**
 * ҵ����Ʋ㷢�͸�ҵ���Ӳ�������
 */
typedef enum tagSCSUCommand{
    SC_CMD_CALL,               /**< ��������� */
    SC_CMD_RINGBACK,           /**< ���󲥷Ż��� */
    SC_CMD_ANSWER_CALL,        /**< ����������� */
    SC_CMD_BRIDGE_CALL,        /**< �����ŽӺ��� */
    SC_CMD_RELEASE_CALL,       /**< �����ͷź��� */
    SC_CMD_PLAYBACK,           /**< ������� */
    SC_CMD_PLAYBACK_STOP,      /**< �������ֹͣ */
    SC_CMD_RECORD,             /**< ����¼�� */
    SC_CMD_RECORD_STOP,        /**< ����¼������ */
    SC_CMD_HOLD,               /**< ������б��� */
    SC_CMD_UNHOLD,             /**< ������н������ */
    SC_CMD_IVR_CTRL,           /**< IVR�������� */
    SC_CMD_MUX,                /**< �������� */

    SC_CMD_BUTT,
}SC_SU_COMMANGEN;

/**
 * ҵ���Ӳ㷢�͸�ҵ����Ʋ������
 */
typedef enum tagSCSUEvent{
    SC_EVT_CALL_SETUP,          /**< ���б����� */
    SC_EVT_CALL_RINGING,        /**< ���б�Ӧ�� */
    SC_EVT_CALL_AMSWERED,       /**< ���б�Ӧ�� */
    SC_EVT_BRIDGE_START,        /**< �Žӿ�ʼ */
    SC_EVT_HOLD,                /**< ���б��� */
    SC_EVT_BRIDGE_STOP,         /**< �Žӽ��� */
    SC_EVT_CALL_RERLEASE,       /**< ���б��ͷ� */
    SC_EVT_CALL_STATUS,         /**< ����״̬�ϱ� */
    SC_EVT_DTMF,                /**< ���β��� */
    SC_EVT_RECORD_START,        /**< ¼����ʼ */
    SC_EVT_RECORD_END,          /**< ¼������ */
    SC_EVT_PLAYBACK_START,      /**< ������ʼ */
    SC_EVT_PLAYBACK_END,        /**< �������� */
    SC_EVT_AUTH_RESULT,         /**< ��֤��� */
    SC_EVT_LEACE_CALL_QUEUE,    /**< �����ж��� */

    SC_EVT_ERROR_PORT,          /**< �����ϱ��¼� */

    SC_EVT_BUTT,
}SC_SU_EVENT_EN;

typedef enum tagSCInterErr{
    SC_ERR_INVALID_MSG,
    SC_ERR_ALLOC_RES_FAIL,
    SC_ERR_EXEC_FAIL,
    SC_ERR_LEG_NOT_EXIST,
    SC_ERR_CALL_FAIL,
    SC_ERR_BRIDGE_FAIL,
    SC_ERR_RECORD_FAIL,
    SC_ERR_BREAK_FAIL,

    SC_ERR_BUTT
}SC_INTER_ERR_EN;


/** �Զ����� */
typedef enum tagSCLegPeerType{
    SC_LEG_PEER_INBOUND,              /**< ���� */
    SC_LEG_PEER_OUTBOUND,             /**< ���� */
    SC_LEG_PEER_INBOUND_TT,           /**< �Զ���TT�� */
    SC_LEG_PEER_OUTBOUND_TT,          /**< �Զ���TT�� */
    SC_LEG_PEER_INBOUND_INTERNAL,     /**< �ڲ����к��� */
    SC_LEG_PEER_OUTBOUND_INTERNAL,    /**< �ڲ����к��� */

    SC_LEG_PEER_BUTT
}SC_LEG_PEER_TYPE_EN;

/** ����ģʽ */
typedef enum tagSCLegLocalType{
    SC_LEG_LOCAL_NORMAL,      /**< �������� */
    SC_LEG_LOCAL_SIGNIN,      /**< ��ǩ */

    SC_LEG_LOCAL_BUTT
}SC_LEG_LOCAL_TYPE_EN;

typedef enum tagCallNumType{
    SC_CALL_NUM_TYPE_NORMOAL              = 0,       /* �������ͣ������ĺ��� */
    SC_CALL_NUM_TYPE_EXPR,                           /* �������ͣ�������ʽ */

    SC_CALL_NUM_TYPE_BUTT                 = 255      /* �������ͣ��Ƿ�ֵ */
}SC_CALL_NUM_TYPE_EN;

/**
 * ҵ������ö��
 *
 * @warning ö��ֵ�����������䲻���ظ�
 */
typedef enum tagSCSrvType{
    SC_SRV_CALL                 = 0,   /**< ��������ҵ�� */
    SC_SRV_PREVIEW_CALL         = 1,   /**< Ԥ�����ҵ�� */
    SC_SRV_AUTO_CALL            = 2,   /**< Ⱥ������ҵ�� */
    SC_SRV_VOICE_VERIFY         = 3,   /**< ������֤��ҵ�� */
    SC_SRV_ACCESS_CODE          = 4,   /**< ������ҵ�� */
    SC_SRV_HOLD                 = 5,   /**< HOLDҵ�� */
    SC_SRV_TRANSFER             = 6,   /**< ת��ҵ�� */
    SC_SRV_INCOMING_QUEUE       = 7,   /**< �������ҵ�� */
    SC_SRV_INTERCEPTION         = 8,   /**< ����ҵ�� */
    SC_SRV_WHISPER              = 9,   /**< ����ҵ�� */
    SC_SRV_MARK_CUSTOM          = 10,  /**< �ͻ����ҵ�� */
    SC_SRV_AGENT_SIGIN          = 11,  /**< ��ϯ��ǩҵ�� ���Ǹ�����ҵ�� */
    SC_SRV_DEMO_TASK            = 12,  /**< Ⱥ������demo */

    SC_SRV_BUTT,
}SC_SRV_TYPE_EN;

typedef struct tagSCAgentStat
{
    U32  ulSelectCnt;
    U32  ulCallCnt;      /* ��ʱ�� ulSelectCnt ����һ�� */
    U32  ulCallConnected;/* ��ͨ�ĺ��� */
    U32  ulTotalDuration;/* ��ͨ�ĺ��� */

    U32  ulIncomingCall; /* ��ʱû��ʵ�� */
    U32  ulOutgoingCall; /* ��ʱû��ʵ�� */
    U32  ulTimesSignin;  /* ��ǩ��ʱ�� */
    U32  ulTimesOnline;  /* ������ʱ�� */
}SC_AGENT_STAT_ST;

typedef struct tagSCAgentGrpStat
{
    U32  ulCallCnt;
    U32  ulCallinQueue;
    U32  ulTotalWaitingTime;      /* ��ʱû��ʵ�� */
    U32  ulTotalWaitingCall;
}SC_AGENT_GRP_STAT_ST;


typedef struct tagACDSiteDesc{
    /* ������Ϣ����Ҫ�洢SCB����Ҫ���ں�SCB�Ľ�������Ҫ��SCB�ͷ�ʱ��飬����� */
    U32        ulLegNo;

    U8         ucWorkStatus;                      /* ��ϯ����״̬ refer to L200 */
    U8         ucServStatus;                      /* ��ϯҵ��״̬ refer to L209 */
    U8         ucBindType;                        /* ��ϯ������ refer to SC_AGENT_BIND_TYPE_EN */
    U8         usResl;

    U32        ulAgentID;                          /* ��ϯ���ݿ��� */
    U32        ulCallCnt;                         /* �������� */
    U32        ulCustomerID;                      /* �ͻ�id */
    U32        ulSIPUserID;                       /* SIP�˻�ID */

    U32        aulGroupID[MAX_GROUP_PER_SITE];    /* ��ID */

    U32        ulLastOnlineTime;
    U32        ulLastSignInTime;
    U32        ulLastIdelTime;
    U32        ulLastProcTime;
    U32        ulLastCallTime;

    U32        bValid:1;                          /* �Ƿ���� */
    U32        bRecord:1;                         /* �Ƿ�¼�� */
    U32        bTraceON:1;                        /* �Ƿ���Ը��� */
    U32        bAllowAccompanying:1;              /* �Ƿ�����ֻ����� refer to SC_SITE_ACCOM_STATUS_EN */

    U32        bGroupHeader:1;                    /* �Ƿ����鳤 */
    U32        bLogin:1;                          /* �Ƿ��Ѿ���¼ */
    U32        bConnected:1;                      /* �Ƿ��Ѿ����� */
    U32        bNeedConnected:1;                  /* �Ƿ��Ѿ����� */

    U32        bWaitingDelete:1;                  /* �Ƿ��Ѿ���ɾ�� */
    U32        bSelected:1;                       /* �Ƿ񱻺��ж���ѡ�� */
    U32        bMarkCustomer:1;                   /* �Ƿ����˿ͻ� */
    U32        bIsInterception:1;                 /* �Ƿ���� */

    U32        bIsWhisper:1;                      /* �Ƿ���� */
    U32        ucRes1:11;

    U8         ucProcesingTime;                   /* ��ϯ������н��ʱ�� */
    U8         ucCallStatus;                      /* ����״̬ */
    U32        usRes;

    S8         szUserID[SC_NUM_LENGTH];    /* SIP User ID */
    S8         szExtension[SC_NUM_LENGTH]; /* �ֻ��� */
    S8         szEmpNo[SC_NUM_LENGTH];     /* ���� */
    S8         szTelePhone[SC_NUM_LENGTH]; /* �̻����� */
    S8         szMobile[SC_NUM_LENGTH];    /* �ƶ��绰 */
    S8         szTTNumber[SC_NUM_LENGTH];    /* TT���� */

    S8         szLastCustomerNum[SC_NUM_LENGTH];    /* ���һ��ͨ���Ŀͻ��ĺ��� */

    DOS_TMR_ST htmrLogout;

    pthread_mutex_t  mutexLock;

    SC_AGENT_STAT_ST stStat;
}SC_AGENT_INFO_ST;


typedef struct tagACDQueueNode{
    U32                     ulID;             /* ��ǰ�ڵ��ڵ�ǰ��������ı�� */

    SC_AGENT_INFO_ST   *pstAgentInfo;     /* ��ϯ��Ϣ */
}SC_AGENT_NODE_ST;

typedef struct tagACDMemoryNode{
    U32        ulAgentID;                            /* ��ϯ���ݿ��� */

    S8        szCallerNum[SC_NUM_LENGTH];    /* ���к��� */
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
    S8        szLastEmpNo[SC_NUM_LENGTH];   /* ���һ���ӵ绰����ϯ�Ĺ��� */

    DLL_S     stAgentList;                         /* ��ϯhash�� */
    pthread_mutex_t  mutexSiteQueue;

    HASH_TABLE_S     *pstRelationList;             /* ���к������ϯ�Ķ�Ӧ��ϵ�б� */

    SC_AGENT_GRP_STAT_ST stStat;
}SC_AGENT_GRP_NODE_ST;

typedef struct tagACDFindSiteParam
{
    U32                  ulPolocy;
    U32                  ulLastSieUsed;
    U32                  ulResult;

    SC_AGENT_INFO_ST *pstResult;
}SC_ACD_FIND_SITE_PARAM_ST;


/**
 * ����LEG��ʱ����Ϣ
 */
typedef struct tagSCSUTimeInfo{
    U32          ulStartTime;          /**< ��ʼʱ�䣬���Ǵ���ʱ�� */
    U32          ulRingTime;           /**< ��ʼ����ʱ��(������LEG�ǶԶ˿�ʼ�����¼��������LEG�Ǹ���180��ʱ��) */
    U32          ulAnswerTime;         /**< Ӧ��ʱ��(������LEG�ǶԶ˽�ͨ��ʱ�䣬�����LEG�Ƿ���200OK��ʱ��) */
    U32          ulBridgeTime;         /**< LEG���Ž��Žӵ�ʱ�� */
    U32          ulByeTime;            /**< LEG���Ҷ�ʱ�� */
    U32          ulIVREndTime;         /**< ����IVR����ʱ�� */
    U32          ulDTMFStartTime;      /**< ��һ��DTMF��ʱ�� */
    U32          ulDTMFLastTime;       /**< ���һ��DTMF��ʱ�� */
    U32          ulRecordStartTime;    /**< ¼����ʼ��ʱ�� */
    U32          ulRecordStopTime;     /**< ¼��������ʱ�� */

}SC_SU_TIME_INFO_ST;

/**
 * ����LEG��ʱ����Ϣ
 */
typedef struct tagSCSUNumInfo{
    S8      szOriginalCallee[SC_NUM_LENGTH]; /**< ԭʼ���к���(ҵ����ʱ��) */
    S8      szOriginalCalling[SC_NUM_LENGTH];/**< ԭʼ���к���(ҵ����ʱ��) */

    S8      szRealCallee[SC_NUM_LENGTH];     /**< ����任֮ǰ������(ҵ��Ӧ��ʹ�õ�) */
    S8      szRealCalling[SC_NUM_LENGTH];    /**< ����任֮ǰ������(ҵ��Ӧ��ʹ�õ�) */

    S8      szCallee[SC_NUM_LENGTH];         /**< ����任֮�������(��������任֮���) */
    S8      szCalling[SC_NUM_LENGTH];        /**< ����任֮�������(��������任֮���) */

    S8      szANI[SC_NUM_LENGTH];            /**< ���к��� */
    S8      szCID[SC_NUM_LENGTH];            /**< ������ʾ */

    S8      szDial[SC_NUM_LENGTH];           /**< ���β��ŵ� */

}SC_SU_NUM_INFO_ST;

/**< ����LEG��״̬  */
typedef enum tagSCSULegStatus{
    SC_LEG_INIT,          /**< ��ʼ��״̬ */
    SC_LEG_CALLING,       /**< ���ں���״̬������ϵͳ����ĺ��У�ר�� */
    SC_LEG_PROC,          /**< ����״̬ */
    SC_LEG_ALERTING,      /**< ����״̬ */
    SC_LEG_ACTIVE,        /**< ����״̬ */
    SC_LEG_RELEASE,       /**< �ͷ�״̬ */
}SC_SU_LEG_SATUE_EN;

/**
 * ҵ��Ԫ,  ��������
 */
typedef struct tagSCSUCall{
    U32                  bValid:1;       /**< �Ƿ��Ѿ����� */
    U32                  bTrace:1;       /**< �Ƿ���Ը��� */
    U32                  bEarlyMedia:1;  /**< �Ƿ�������ý�� */
    U32                  bIsTTCall:1;    /**< �Ƿ���TT�ź��� */
    U32                  bRes:28;

    U8                   ucStatus;       /**< ��ǰLEG��״̬ */
    U8                   ucHoldCnt;      /**< HOLD�Ĵ��� */
    U8                   ucPeerType;     /**< �Զ����ͣ���ʾ�Ǻ��룬���Ǻ��� */
    U8                   ucLocalMode;    /**< ����ģʽ����ʾ�Ƿ�ǩ */

    U32                  ulHoldTotalTime;/**< ���б�����ʱ�� */
    U32                  ulTrunkID;      /**< �м�ID */
    U32                  ulTrunkCount;   /**< �м̵ĸ��� */
    U32                  ulCause;        /**< �Ҷ�ԭ�� */
    SC_SU_TIME_INFO_ST   stTimeInfo;     /**< ������Ϣ */
    SC_SU_NUM_INFO_ST    stNumInfo;      /**< ʱ����Ϣ */


    U8      aucCodecList[SC_MAX_CODEC_NUM];  /**< ������б� */
    U32     ulCodecCnt;
    U32     aulTrunkList[SC_MAX_TRUCK_NUM];  /**< �м��б� */
    U32     ulTrunkCnt;

    S8      szEIXAddr[SC_MAX_IP_LEN];        /**< TT�ź���ʱ��ҪEIXIP��ַ */

    U32     ulRes;
}SC_SU_CALL_ST;


/**< ����LEG¼��״̬  */
typedef enum tagSCSURecordStatus{
    SC_SU_RECORD_INIT,          /**< ��ʼ��״̬ */
    SC_SU_RECORD_PROC,          /**< ��ʼ��״̬ */
    SC_SU_RECORD_ACTIVE,        /**< ��ʼ¼��״̬ */
    SC_SU_RECORD_RELEASE,       /**< ¼������״̬ */
}SC_SU_RECORD_SATUE_EN;

/**
 * ҵ��Ԫ,  ¼��ҵ��
 */
typedef struct tagSCSURecord{

    U32      bValid:1;   /**< �Ƿ��Ѿ����� */
    U32      bTrace:1;   /**< �Ƿ���Ը��� */
    U32      bRes:30;
    U16      usStatus;   /**< ��ǰLEG��״̬ */
    U16      usRes;

    /** ¼���ļ��� */
    S8      szRecordFilename[SC_RECORD_FILENAME_LENGTH];
}SC_SU_RECORD_ST;

/**< ����LEG����״̬״̬  */
typedef enum tagSCSUPlaybackStatus{
    SC_SU_PLAYBACK_INIT,          /**< ��ʼ��״̬ */
    SC_SU_PLAYBACK_PROC,          /**< ��ʼ��״̬ */
    SC_SU_PLAYBACK_ACTIVE,        /**< ����״̬ */
    SC_SU_PLAYBACK_RELEASE,       /**< �ͷ�״̬ */
}SC_SU_PLAYBACK_SATUE_EN;

/**
 * ҵ��Ԫ,  ����ҵ��
 */
typedef struct tagSCSUPlayback{
    U32      bValid:1;   /**< �Ƿ��Ѿ����� */
    U32      bTrace:1;   /**< �Ƿ���Ը��� */
    U32      bRes:30;
    U16      usStatus;   /**< ��ǰLEG��״̬ */
    U16      usRes;

    /** ¼���ļ����� */
    U32      ulTotal;

    /** ��ǰ¼���ļ��������� */
    U32      ulCurretnIndex;
}SC_SU_PLAYBACK_ST;


/**< ����LEG�Ž�״̬״̬  */
typedef enum tagSCSUBridgeStatus{
    SC_SU_BRIDGE_INIT,          /**< ��ʼ��״̬ */
    SC_SU_BRIDGE_PROC,          /**< ������ */
    SC_SU_BRIDGE_ACTIVE,        /**< ����״̬ */
    SC_SU_BRIDGE_RELEASE,       /**< �ͷ�״̬ */
}SC_SU_BRIDGE_SATUE_EN;

/**
 * ҵ��Ԫ,  �Ž�ҵ��
 */
typedef struct tagSCSUBridge{
    U32      bValid:1;      /**< �Ƿ��Ѿ����� */
    U32      bTrace:1;      /**< �Ƿ���Ը��� */
    U32      bRes:30;

    U16      usStatus;      /**< ��ǰLEG��״̬ */
    U16      usRes;
    U32      ulOtherLEGNo;  /**< ��ǰ¼���ļ��������� */
    U32      ulRes;
}SC_SU_BRIDGE_ST;


/**< ����LEG HOLD״̬״̬  */
typedef enum tagSCSUHoldStatus{
    SC_SU_HOLD_INIT,          /**< ��ʼ��״̬ */
    SC_SU_HOLD_PROC,          /**< ������ */
    SC_SU_HOLD_ACTIVE,        /**< ����״̬ */
    SC_SU_HOLD_RELEASE,       /**< �ͷ�״̬ */
}SC_SU_HOLD_SATUE_EN;

/**
 * ҵ��Ԫ,  ���б���ҵ��
 */
typedef struct tagSCSUHold{
    U32      bValid:1;      /**< �Ƿ��Ѿ����� */
    U32      bTrace:1;      /**< �Ƿ���Ը��� */
    U32      bRes:30;
    U16      usStatus;      /**< ��ǰLEG��״̬ */
    U16      usMode;        /**< HOLDģʽ��������HOLD�����Ǳ��Զ�HOLD */
    U32      ulRes;
}SC_SU_HOLD_ST;


/**< ����LEG ����״̬״̬  */
typedef enum tagSCSUMUXStatus{
    SC_SU_MUX_INIT,          /**< ��ʼ��״̬ */
    SC_SU_MUX_ACTIVE,        /**< ����״̬ */
    SC_SU_MUX_RELEASE,       /**< �ͷ�״̬ */
}SC_SU_MUX_SATUE_EN;

/**
 * ҵ��Ԫ,  ����ҵ��
 */
typedef struct tagSCSUMux{
    /** �Ƿ��Ѿ����� */
    U32                   bValid:1;
    /** �Ƿ���Ը��� */
    U32                   bTrace:1;
    U32                   bRes:30;

    U16      usStatus;                   /**< ��ǰLEG��״̬ */
    U16      usMode;                     /**< ����ģʽ */

    U32     ulOtherLegNo;
    U32     ulRes;
}SC_SU_MUX_ST;

/**< ����LEG �޸�����״̬״̬  */
typedef enum tagSCSUMCXStatus{
    SC_SU_MCX_INIT,          /**< ��ʼ��״̬ */
    SC_SU_MCX_ACTIVE,        /**< ����״̬ */
    SC_SU_MCX_RELEASE,       /**< �ͷ�״̬ */
}SC_SU_MCX_SATUE_EN;


/**
 * ҵ��Ԫ,  �޸�����ҵ��
 */
typedef struct tagSCSUMCX{
    U32      bValid:1;  /**< �Ƿ��Ѿ����� */
    U32      bTrace:1;  /**< �Ƿ���Ը��� */
    U32      bRes:30;
    U16      usStatus;  /**< ��ǰLEG��״̬ */
    U16      usRes;
}SC_SU_MCX_ST;


/**< ����LEG �޸�����״̬״̬  */
typedef enum tagSCSUIVRStatus{
    SC_SU_IVR_INIT,          /**< ��ʼ��״̬ */
    SC_SU_IVR_ACTIVE,        /**< ����״̬ */
    SC_SU_IVR_RELEASE,       /**< �ͷ�״̬ */
}SC_SU_IVR_SATUE_EN;

/**
 * ҵ��Ԫ,  IVRҵ��
 */
typedef struct tagSCSUIVR{
    U32      bValid:1;       /**< �Ƿ��Ѿ����� */
    U32      bTrace:1;       /**< �Ƿ���Ը��� */
    U32      bRes:30;

    U16      usStatus;       /**< ��ǰLEG��״̬ */
    U16      usRes;
    U32      ulIVRIndex;     /**< IVR��� */
    U32      ulRes;
}SC_SU_IVR_ST;

/**
 * ����LEG���ƿ�
 */
typedef struct tagSCLegCB{
    /** LEG���ƿ��� */
    U32                   ulCBNo;
    /** ��¼��ǰLEG�����ҵ����ƿ� */
    U32                   ulSCBNo;
    /** ��¼����ҵ����ƿ� */
    U32                   ulIndSCBNo;
    /** �Ƿ��Ѿ����� */
    U32                   bValid:1;
    /** �Ƿ���Ը��� */
    U32                   bTrace:1;
    U32                   bRes:30;

    /** UUID */
    S8                    szUUID[SC_UUID_LENGTH];

    /** ����ʱ�� */
    U32                   ulAllocTime;

    /** ԭʼ������б� */
    U8                    aucCodecList[SC_MAX_CODEC_NUM];
    /** ʹ�õı���� */
    U8                    ucCodec;
    /** ���ʱ�� */
    U8                    ucPtime;
    U16                   usRes;

    /** ��֤�¼���ҵ����ƿ鴦������ٱ�����ҵ����ƿ鴦����ʱ����Ҫ��������һ���߳��ڴ��� */
    sem_t                 stEventSem;

    /** ��������ҵ��Ԫ���ƿ� */
    SC_SU_CALL_ST         stCall;
    /** ¼��ҵ��Ԫ���ƿ� */
    SC_SU_RECORD_ST       stRecord;
    /** ����ҵ��Ԫ���ƿ� */
    SC_SU_PLAYBACK_ST     stPlayback;
    /** �Ž�ҵ��Ԫ���ƿ� */
    SC_SU_BRIDGE_ST       stBridge;
    /** ���б���ҵ��Ԫ���ƿ� */
    SC_SU_HOLD_ST         stHold;
    /** ����ҵ��Ԫ���ƿ� */
    SC_SU_MUX_ST          stMux;
    /** �޸�����ҵ��Ԫ���ƿ� */
    SC_SU_MCX_ST          stMcx;
    /** IVRҵ��Ԫ���ƿ� */
    SC_SU_IVR_ST          stIVR;

}SC_LEG_CB;

/**
 * ҵ����ƿ�ͷ
 */
typedef struct tagSCBTag{
    /** �Ƿ񱻷��� */
    U32                      bValid:1;

    /** �Ƿ���Ҫ���Ը��� */
    U32                      bTrace:1;

    /** �Ƿ��Ѿ����� */
    U32                      bWaitingExit:1;

    U32                      bRes:29;

    /** ҵ������ */
    U16                      usSrvType;
    /** ״̬ */
    U16                      usStatus;
}SC_SCB_TAG_ST;


/** ��������ҵ��״̬ */
typedef enum tagSCCallStatus{
    SC_CALL_IDEL,       /**< ״̬��ʼ�� */
    SC_CALL_PORC,       /**< ����Ԥ����ȷ���ͻ�����Ϣ */
    SC_CALL_AUTH,       /**< ��֤ */
    SC_CALL_AUTH2,      /**< ��֤���� */
    SC_CALL_EXEC,       /**< ��ʼ���б��� */
    SC_CALL_ALERTING,   /**< ���п�ʼ���� */
    SC_CALL_TONE,       /**< ��ϯ��ǩʱ������ǩ����ϯ����ʾ�� */
    SC_CALL_ACTIVE,     /**< ���н�ͨ */
    SC_CALL_PROCESS,    /**< ����֮��Ĵ��� */
    SC_CALL_RELEASE,    /**< ���� */
}SC_CALL_STATE_EN;

/**
 * ҵ����ƿ�, ��������
 */
typedef struct tagSCSrvCall{
    /** ������Ϣ */
    SC_SCB_TAG_ST     stSCBTag;

    /* ��һ�δ�����LEG��� */
    U32               ulCallingLegNo;

    /* ����LEG��� */
    U32               ulCalleeLegNo;

    /* ������Դ SC_LEG_DIRACTION_EN */
    U32               ulCallSrc;

    /* ����Ŀ�� SC_LEG_DIRACTION_EN */
    U32               ulCallDst;

    /* ·��ID */
    U32               ulRouteID;

    /** ������ϯָ�� */
    SC_AGENT_NODE_ST *pstAgentCalling;

    /** ������ϯָ�� */
    SC_AGENT_NODE_ST *pstAgentCallee;
}SC_SRV_CALL_ST;


/** ���ҵ��״̬������
 *   AGENT Leg                     SC                         Custom Leg          BS
 *   |                             |     Auth Request         |                    |
 *   |                      (AUTH) | --------------------------------------------> |
 *   |                             |      Auth Rsp            |                    |
 *   |                      (EXEC) | <-------------------------------------------- |
 *   |        Call request         |                          |                    |
 *   | <-------------------------- |                          |
 *   |      Call Setup Event       |                          |
 *   | --------------------------> | (PROC)                   |
 *   |     Call Ringing Event      |                          |
 *   | --------------------------> | (ALERTING)               |
 *   |      Call Answer Event      |                          |
 *   | --------------------------> | (ACTIVE)                 |
 *   |                             |     Call request         |
 *   |                             | -----------------------> |
 *   |                             |     Call Setup Event     |
 *   |                (CONNECTING) | <----------------------- |
 *   |                             |    Call Ringing Event    |
 *   |                 (ALERTING2) | <----------------------- |
 *   |                             |     Call Answer Event    |
 *   |                 (CONNECTED) | <----------------------- |
 *   |                          Talking                       |
 *   | <----------------------------------------------------> |
 *   |                             |       Hungup Event       |
 *   |                   (PROCESS) | <----------------------- |
 *   |      Hungup Event           |                          |
 *   | --------------------------> | (IDEL)                   |
 *   |                             |                          |
 *
 *   �������ͻ���һ��ĺ���ʧ�ܣ����ܻ�������ý���������Ҫ������LEG�Ž�
 */
typedef enum tagSCPreviewCallStatus{
    SC_PREVIEW_CALL_IDEL,       /**< ״̬��ʼ�� */
    SC_PREVIEW_CALL_AUTH,       /**< ״̬��ʼ�� */
    SC_PREVIEW_CALL_EXEC,       /**< ״̬��ʼ�� */
    SC_PREVIEW_CALL_PORC,       /**< ������ϯ�ĺ��� */
    SC_PREVIEW_CALL_ALERTING,   /**< ��ϯ�������� */
    SC_PREVIEW_CALL_ACTIVE,     /**< ��ϯ��ͨ */
    SC_PREVIEW_CALL_CONNECTING, /**< ���пͻ� */
    SC_PREVIEW_CALL_ALERTING2,  /**< �ͻ���ʼ������ */
    SC_PREVIEW_CALL_CONNECTED,  /**< ���н�ͨ�� */
    SC_PREVIEW_CALL_PROCESS,    /**< ͨ������֮������пͻ���ǣ��Ϳ�ʼ��ǣ�û��ֱ�ӵ��ͷ� */
    SC_PREVIEW_CALL_RELEASE
}SC_PREVIEW_CALL_STATE_EN;

/**
 * ҵ����ƿ�, Ԥ�����
 */
typedef struct tagSCPreviewCall{
    /** ������Ϣ */
    SC_SCB_TAG_ST     stSCBTag;

    /** ������ϯ��LEG */
    U32               ulCallingLegNo;

    /** ���пͻ���LEG */
    U32               ulCalleeLegNo;

    /** ��ϯID */
    U32               ulAgentID;
}SC_PREVIEW_CALL_ST;

/** �Զ����ҵ�� */
typedef enum tagSCAutoCallStatus{
    SC_AUTO_CALL_IDEL,          /**< ״̬��ʼ�� */
    SC_AUTO_CALL_AUTH,
    SC_AUTO_CALL_EXEC,
    SC_AUTO_CALL_PORC,          /**< ������� */
    SC_AUTO_CALL_ALERTING,
    SC_AUTO_CALL_ACTIVE,        /**< �������� */
    SC_AUTO_CALL_AFTER_KEY,     /**< ����֮�󣬼��뵽���ж��� */
    SC_AUTO_CALL_AUTH2,         /**< ��ϯ���ֻ����绰ʱ��������ϯ����Ҫ��֤ */
    SC_AUTO_CALL_EXEC2,         /**< ������ϯ */
    SC_AUTO_CALL_PORC2,         /**< ��ϯ��channel���� */
    SC_AUTO_CALL_ALERTING2,     /**< ��ϯ���� */
    SC_AUTO_CALL_TONE,          /**< ��ϯ��ǩʱ������ϯ����ʾ�� */
    SC_AUTO_CALL_CONNECTED,     /**< ����ϯ��ʼͨ�� */
    SC_AUTO_CALL_PROCESS,       /**< ͨ������֮�����������ȵ��飬����Ҫ���״̬��û��ֱ�ӵ��ͷ� */
    SC_AUTO_CALL_RELEASE,       /**< ���� */

}SC_AUTO_CALL_STATE_EN;

/**
 * ҵ����ƿ�, �Զ����
 */
typedef struct tagSCAUTOCall{
    /** ������Ϣ */
    SC_SCB_TAG_ST     stSCBTag;

    /** ���пͻ���LEG */
    U32               ulCallingLegNo;

    /** ������ϯ��LEG */
    U32               ulCalleeLegNo;

    /** �����ļ�ID */
    U32               ulAudioID;

    /** ��ͨ��ϯģʽ */
    U32               ulKeyMode;

    /** ��ϯID */
    U32               ulAgentID;

    /** Ⱥ������ID */
    U32               ulTaskID;
    /** Ⱥ��������ƿ�ID */
    U32               ulTcbID;
}SC_AUTO_CALL_ST;


/** ������֤��ҵ��״̬��
* Leg                      SC                       BS
* |                        |         Auth           |
* |                 (AUTH) | ---------------------> |
* |                        |        Auth Rsp        |
* |        Call Req        | <--------------------- |
* | <--------------------- | (EXEC)                 |
* |    Call setup event    |                        |
* | ---------------------> | (PROC)                 |
* |   Call Ringing event   |                        |
* | ---------------------> | (ALERTING)             |
* |   Call Answer event    |                        |
* | ---------------------> | (ACTIVE)               |
* |    Playback Req        |                        |
* | <--------------------- |                        |
* |     Playback stop      |                        |
* | ---------------------> | (RELEASE->INIT)        |
* |                        |                        |
*/
typedef enum tagSCVoiceVerifyStatus{
    SC_VOICE_VERIFY_INIT,       /**< ״̬��ʼ�� */
    SC_VOICE_VERIFY_AUTH,       /**< ������� */
    SC_VOICE_VERIFY_EXEC,       /**< �������� */
    SC_VOICE_VERIFY_PROC,       /**< ���� */
    SC_VOICE_VERIFY_ALERTING,   /**< ���� */
    SC_VOICE_VERIFY_ACTIVE,     /**< ���� */
    SC_VOICE_VERIFY_RELEASE,    /**< ���� */
}SC_VOICE_VERIFY_STATE_EN;

/**
 * ҵ����ƿ�, ������֤��
 */
typedef struct tagSCVoiceVerify{
    /** ������Ϣ */
    SC_SCB_TAG_ST     stSCBTag;

    /* ���пͻ���LEG */
    U32               ulLegNo;
    /* ������֤��ǰ����ʾ��ID */
    U32               ulTipsHitNo1;
    /* ������֤������ʾ��ID */
    U32               ulTipsHitNo2;
    /* ������֤�� */
    S8                szVerifyCode[SC_VOICE_VERIFY_CODE_LENGTH];
}SC_VOICE_VERIFY_ST;

/** ������ҵ�� */
typedef enum tagSCAccessCodeStatus{
    SC_ACCESS_CODE_IDEL,       /**< ״̬��ʼ�� */
    SC_ACCESS_CODE_OVERLAP,    /**< �պ� */
    SC_ACCESS_CODE_ACTIVE,     /**< ִ��ҵ�� */
    SC_ACCESS_CODE_RELEASE,    /**< ���� */
}SC_ACCESS_CODE_STATE_EN;


/**
 * ҵ����ƿ�, ������ҵ��
 */
typedef struct tagSCAccessCode{
    /** ������Ϣ */
    SC_SCB_TAG_ST     stSCBTag;

    /* ���пͻ���LEG */
    U32               ulLegNo;

    /** �Ƿ��Ƕ��β��� */
    BOOL              bIsSecondDial;

    /** ���������� */
    U32               ulSrvType;

    /** �����뻺�� */
    S8                szDialCache[SC_MAX_ACCESS_CODE_LENGTH];

    /** ��ϯID */
    U32               ulAgentID;

}SC_ACCESS_CODE_ST;

/** ������� */
typedef enum tagSCInQueueStatus{
    SC_INQUEUE_IDEL,       /**< ״̬��ʼ�� */
    SC_INQUEUE_ACTIVE,     /**< ������гɹ� */
    SC_INQUEUE_RELEASE,    /**< ���� */
}SC_IN_QUEUE_STATE_EN;

/**
 * ҵ����ƿ�, �������ҵ��
 */
typedef struct tagSCIncomingQueue{
    /** ������Ϣ */
    SC_SCB_TAG_ST     stSCBTag;

    /* ���пͻ���LEG */
    U32               ulLegNo;

    /** �������ͣ����飬�����ܻ�ҵ�� */
    U32               ulQueueType;

    /** ������е�ʱ�� */
    U32               ulEnqueuTime;

    /** �����е�ʱ�� */
    U32               ulDequeuTime;
}SC_INCOMING_QUEUE_ST;

/** ����ҵ��״̬ */
typedef enum tagSCInterStatus{
    SC_INTERCEPTION_IDEL,       /**< ״̬��ʼ�� */
    SC_INTERCEPTION_AUTH,       /**< ������� */
    SC_INTERCEPTION_EXEC,       /**< �������� */
    SC_INTERCEPTION_PROC,       /**< ����leg */
    SC_INTERCEPTION_ALERTING,   /**< ���� */
    SC_INTERCEPTION_ACTIVE,     /**< ��ʼ���� */
    SC_INTERCEPTION_RELEASE,    /**< ���� */
}SC_INTERCEPTION_STATE_EN;

/**
 * ҵ����ƿ�, ����ҵ��
 */
typedef struct tagSCInterception{
    /** ������Ϣ */
    SC_SCB_TAG_ST     stSCBTag;

    /* ���м���������LEG */
    U32               ulLegNo;

    /* ��������ϯ��LEG */
    U32               ulAgentLegNo;

    /* ��ϯ */
    SC_AGENT_NODE_ST  *pstAgentInfo;

}SC_INTERCEPTION_ST;

/** ����ҵ��״̬ */
typedef enum tagSCWhisperStatus{
    SC_WHISPER_IDEL,       /**< ״̬��ʼ�� */
    SC_WHISPER_AUTH,       /**< ������� */
    SC_WHISPER_EXEC,       /**< �������� */
    SC_WHISPER_PROC,       /**< ����leg */
    SC_WHISPER_ALERTING,   /**< ���� */
    SC_WHISPER_ACTIVE,     /**< ��ʼ���� */
    SC_WHISPER_RELEASE,    /**< ���� */
}SC_WHISPER_STATE_EN;

/**
 * ҵ����ƿ�, ����ҵ��
 */
typedef struct tagSCWhisper{
    /** ������Ϣ */
    SC_SCB_TAG_ST     stSCBTag;

    /* ���пͻ���LEG */
    U32               ulLegNo;

    /* ��������ϯ��LEG */
    U32               ulAgentLegNo;

    /* ��ϯ */
    SC_AGENT_NODE_ST  *pstAgentInfo;

}SC_SRV_WHISPER_ST;

/** �ͻ���� */
typedef enum tagSCMarkCustomStatus{
    SC_MAKR_CUSTOM_IDEL,       /**< ״̬��ʼ�� */
    SC_MAKR_CUSTOM_PROC,       /**< �������� */
    SC_MAKR_CUSTOM_ACTIVE,     /**< �������OK */
    SC_MAKR_CUSTOM_RELEASE,    /**< �ͷ� */
}SC_MAKR_CUSTOM_STATE_EN;

/**
 * ҵ����ƿ�, �ͻ����ҵ��
 */
typedef struct tagSCMarkCustom{
    /** ������Ϣ */
    SC_SCB_TAG_ST     stSCBTag;

    /* ������ϯ��LEG */
    U32               ulLegNo;

    /* ��ϯ����Ϣ */
    SC_AGENT_NODE_ST *pstAgentCall;


    /** �����뻺�� */
    S8                szDialCache[SC_MAX_ACCESS_CODE_LENGTH];

    /* ��ʱ������ʱ֮��������� */
    DOS_TMR_ST        stTmrHandle;
}SC_MARK_CUSTOM_ST;

/** ����ת��ҵ��״̬ */
typedef enum tagSCTransStatus{
    SC_TRANSFER_IDEL,       /**< ״̬��ʼ�� */
    SC_TRANSFER_AUTH,       /**< ��֤ת��ҵ�����������ΪPSTN��ҲҪһ����֤ */
    SC_TRANSFER_EXEC,
    SC_TRANSFER_PROC,       /**< ����ת�Ӵ���(������е�����������) */
    SC_TRANSFER_ALERTING,
    SC_TRANSFER_ACTIVE,     /**< ��������ͨ */
    SC_TRANSFER_TRANSFER,   /**< Э��תʱ��ת�Ӻ��� */
    SC_TRANSFER_FINISHED,   /**< ת����� */
    SC_TRANSFER_RELEASE,    /**< ת��ҵ����� */
}SC_TRANS_STATE_EN;

typedef enum tagSCTransType{
    SC_TRANSFER_BLIND,      /**< äת */
    SC_TRANSFER_ATTENDED,   /**< Э��ת */
}SC_TRANS_TYPE_EN;

/**
 * ҵ����ƿ�, ת��ҵ��
 */
typedef struct tagSCCallTransfer{
    /** ������Ϣ */
    SC_SCB_TAG_ST     stSCBTag;

    /** ת������ */
    U32               ulType;

    /** ���ķ����ڵ�SCB */
    U32               ulSubLegNo;

    /** ������LEG����ת�ӵ�Ŀ�ĵ�LEG */
    U32               ulPublishLegNo;
    /** ֪ͨ��LEG��������ת�ӵ�LEG */
    U32               ulNotifyLegNo;

    /** Ҫת�ӵ���ϯ��Ҫ�޸���ϯ��״̬ */
    U32               ulSubAgentID;
    U32               ulPublishAgentID;
    U32               ulNotifyAgentID;
}SC_CALL_TRANSFER_ST;

/** ���б���ҵ��״̬ */
typedef enum tagSCHoldStatus{
    SC_HOLD_IDEL,       /**< ���б���״̬��ʼ�� */
    SC_HOLD_PROC,       /**< ���б��ִ�����(�����˺��б�������֮��) */
    SC_HOLD_ACTIVE,     /**< �յ�HOLD�¼�֮�� */
    SC_HOLD_RELEASE,    /**< �յ�UNHOLD�¼�֮�� */
}SC_HOLD_STATE_EN;

/**
 * ҵ����ƿ�, ���б���ҵ��
 */
typedef struct tagSCCallHold{
    /** ������Ϣ */
    SC_SCB_TAG_ST     stSCBTag;

    /* ���пͻ���LEG */
    U32               ulCallLegNo;
}SC_CALL_HOLD_ST;

/**< ��ϯ��ǩҵ��״̬  */
typedef enum tagSCSiginStatus{
    SC_SIGIN_IDEL,          /**< ��ʼ��״̬ */
    SC_SIGIN_AUTH,          /**< ��ʼ��״̬ */
    SC_SIGIN_EXEC,          /**< ��ʼ���� */
    SC_SIGIN_PORC,          /**<  */
    SC_SIGIN_ALERTING,      /**< ��ʼ���� */
    SC_SIGIN_ACTIVE,        /**< ����״̬ */
    SC_SIGIN_RELEASE,       /**< �ͷ�״̬ */
}SC_SIGIN_SATUE_EN;

/**
 * ҵ��Ԫ,  ���г�ǩҵ��
 */
typedef struct tagSCSigin{
     /** ������Ϣ */
    SC_SCB_TAG_ST     stSCBTag;

    /* ���пͻ���LEG */
    U32               ulLegNo;

    /* ��ϯ����Ϣ */
    SC_AGENT_NODE_ST *pstAgentNode;

}SC_SIGIN_ST;

typedef struct tagSCBalanceWarning{
    SC_SCB_TAG_ST     stSCBTag;
}SC_BALANCE_WARNING_ST;

#define SC_SCB_IS_VALID(pstSCB) \
            (DOS_ADDR_VALID((pstSCB)) && (pstSCB)->bValid)

/**
 * ҵ����ƿ�
 */
typedef struct tagSCSrvCB{
    /** ��¼��ǰLEG�����ҵ����ƿ� */
    U32                      ulSCBNo;

    /** �Ƿ��Ѿ����� */
    U32                      bValid:1;

    /** �Ƿ���Ը��� */
    U32                      bTrace:1;
    U32                      bRes:30;

    /** ҵ����ƿ���� */
    SC_SCB_TAG_ST            *pstServiceList[SC_SRV_BUTT];
    /** ������ָ�� */
    U32                      ulCurrentSrv;

    /** ��ҵ�ͻ�ID */
    U32                      ulCustomerID;

    /** ��ϯID */
    U32                      ulAgentID;

    /** ����ʱ�� */
    U32                      ulAllocTime;

    /** ҵ������ */
    U8                       aucServType[SC_MAX_SERVICE_TYPE];

    /** ��������ҵ����ƿ� */
    SC_SRV_CALL_ST       stCall;
    /** Ԥ�����ҵ����ƿ� */
    SC_PREVIEW_CALL_ST   stPreviewCall;
    /** �Զ����ҵ����ƿ� */
    SC_AUTO_CALL_ST      stAutoCall;
    /** ������֤��ҵ����ƿ� */
    SC_VOICE_VERIFY_ST   stVoiceVerify;
    /** ������ҵ����ƿ� */
    SC_ACCESS_CODE_ST    stAccessCode;
    /** ���б���ҵ����ƿ� */
    SC_CALL_HOLD_ST      stHold;
    /** ����ת��ҵ����ƿ� */
    SC_CALL_TRANSFER_ST  stTransfer;
    /** �������ҵ����ƿ� */
    SC_INCOMING_QUEUE_ST stIncomingQueue;
    /** ����ҵ����ƿ� */
    SC_INTERCEPTION_ST   stInterception;
    /** ����ҵ����ƿ� */
    SC_SRV_WHISPER_ST    stWhispered;
    /** �ͻ����ҵ����ƿ� */
    SC_MARK_CUSTOM_ST    stMarkCustom;
    /** ��ϯ��ǩҵ����ƿ� */
    SC_SIGIN_ST          stSigin;
    /** Ⱥ������demo���ƿ� */
    SC_AUTO_CALL_ST      stDemoTask;
    /** ���澯ҵ���Ƿ����� */
    SC_BALANCE_WARNING_ST stBalanceWarning;

}SC_SRV_CB;


/** SC��ģ��֮��ͨѶ����Ϣͷ */
typedef struct tagSCMsgTAG{
    U32   ulMsgType;   /**< Ŀ�� */

    U16   usMsgLen;    /**< ��Ϣ�ܳ��� */
    U16   usInterErr;  /**< �ڲ������� */

    U32   ulSCBNo;     /**< ҵ�����ģ���� */
}SC_MSG_TAG_ST;

/** ����������� */
typedef struct tagSCMsgCmdCall{
    SC_MSG_TAG_ST      stMsgTag;             /**< ��Ϣͷ */

    SC_SU_NUM_INFO_ST  stNumInfo;            /**< ������Ϣ */
    U32     ulSCBNo;
    U32     ulLCBNo;                         /**< ��LEG��� */
}SC_MSG_CMD_CALL_ST;

/** �Ž����� */
typedef struct tagSCMsgCmdBridge{
    SC_MSG_TAG_ST    stMsgTag;              /**< ��Ϣͷ */

    U32     ulSCBNo;                         /**< SCB��� */
    U32     ulCallingLegNo;                  /**< ����LEG */
    U32     ulCalleeLegNo;                   /**< ����LEG */
}SC_MSG_CMD_BRIDGE_ST;

/** Ӧ������ */
typedef struct tagSCMsgCmdRingback{
    SC_MSG_TAG_ST    stMsgTag;              /**< ��Ϣͷ */

    U32     ulSCBNo;                        /**< SCB��� */
    U32     ulLegNo;                        /**< ����LEG */
    U32     ulMediaConnected;               /**< ����LEG */
}SC_MSG_CMD_RINGBACK_ST;

/** Ӧ������ */
typedef struct tagSCMsgCmdAnswer{
    SC_MSG_TAG_ST    stMsgTag;              /**< ��Ϣͷ */

    U32     ulSCBNo;                        /**< SCB��� */
    U32     ulLegNo;                        /**< ����LEG */
}SC_MSG_CMD_ANSWER_ST;

typedef enum tagSCCmdPlaybackType{
    SC_CND_PLAYBACK_TONE,                   /**< �������� */
    SC_CND_PLAYBACK_SYSTEM,                 /**< ϵͳ���� */
    SC_CND_PLAYBACK_FILE,                   /**< ָ�������ļ��� */

    SC_CND_PLAYBACK_BUTT

}SC_CND_PLAYBACK_TYPE_EN;

/** �������� */
typedef struct tagSCMsgCmdPlayback{
    SC_MSG_TAG_ST    stMsgTag;               /**< ��Ϣͷ */
    U32     ulMode;                          /**< ��ʾ�Ƿ�����ֹͣ������ֹͣ��ǰ���� */
    U32     ulSCBNo;                         /**< SCB��� */
    U32     ulLegNo;                         /**< LEG��� */
    U32     aulAudioList[SC_MAX_AUDIO_NUM];  /**< �����ļ��б� */
    U32     ulTotalAudioCnt;                 /**< �����ļ����� */
    U32     ulLoopCnt;                       /**< �Ƿ�ѭ�����ţ������ָ��ѭ�����ŵĴ��� */
    U32     ulInterval;                      /**< ÿ��ѭ��ʱ���� ms */
    U32     ulSilence;                       /**< ����֮ǰSilence��ʱ�� ms */

    U32     blNeedDTMF;                      /**< �Ƿ���ҪDTMF */

    S8      szAudioFile[SC_MAX_AUDIO_FILENAME_LEN];  /**< �����ļ��ļ��� */

    U32     enType;                          /**< SC_CND_PLAYBACK_TYPE_EN */
}SC_MSG_CMD_PLAYBACK_ST;

/** ¼������ */
typedef struct tagSCMsgCmdRecord{
    SC_MSG_TAG_ST    stMsgTag;              /**< ��Ϣͷ */

    /** ��ʾ·����ʼ/���� */
    U32     ulFlag;

    /** LEG��� */
    U32     ulLegNo;

    /** ҵ����ƿ��� */
    U32     ulSCBNo;

    /** ¼���ļ��� */
    S8      szRecordFile[SC_RECORD_FILENAME_LENGTH];
}SC_MSG_CMD_RECORD_ST;

/** �Ҷ����� */
typedef struct tagSCMsgCmdHungup{
    SC_MSG_TAG_ST    stMsgTag;              /**< ��Ϣͷ */

    /** LEG��� */
    U32     ulLegNo;

    /** ҵ����ƿ��� */
    U32     ulSCBNo;

    /** �Ҷ�ԭ�� */
    U32     ulErrCode;
}SC_MSG_CMD_HUNGUP_ST;

typedef enum tagSCHoldFlag{
    SC_HOLD_FLAG_HOLD      = 0,
    SC_HOLD_FLAG_UNHOLD
}SC_CMD_HOLD_FLAG_EN;

/** ���б������� */
typedef struct tagSCMsgCmdHold{
    SC_MSG_TAG_ST    stMsgTag;              /**< ��Ϣͷ */

    /** ��ʾ HOLD/UNHOLD */
    U32     ulFlag;

    /** LEG��� */
    U32     ulLegNo;

    /** ҵ����ƿ��� */
    U32     ulSCBNo;
}SC_MSG_CMD_HOLD_ST;

/** IRV���� */
typedef struct tagSCMsgCmdIVR{
    SC_MSG_TAG_ST    stMsgTag;              /**< ��Ϣͷ */

    /** ��ʾ IVR�������� */
    U32     ulOpter;

    /** LEG��� */
    U32     ulLegNo;

    /** ҵ����ƿ��� */
    U32     ulSCBNo;
}SC_MSG_CMD_IVR_ST;

typedef enum tagScMuxCmdMode{
    SC_MUX_CMD_INTERCEPT,               /**< ���� */
    SC_MUX_CMD_WHISPER,                 /**< ���� */

    SC_MUX_CMD_BUTT
}SC_MUX_CMD_MODE_EN;


/** ���� ���� */
typedef struct tagSCMsgCmdMux{
    SC_MSG_TAG_ST    stMsgTag;              /**< ��Ϣͷ */

    /** ���͡�SC_MUX_CMD_MODE_EN */
    U32     ulMode;

    /** LEG��� */
    U32     ulLegNo;

    /** ��������ϯ��LEG��� */
    U32     ulAgentLegNo;

    /** ҵ����ƿ��� */
    U32     ulSCBNo;
}SC_MSG_CMD_MUX_ST;

/** ���н����¼� */
typedef struct tagSCMsgEvtCall{
    SC_MSG_TAG_ST    stMsgTag;              /**< ��Ϣͷ */

    /** ҵ����ƿ��� */
    U32     ulSCBNo;

    /** LEG��� */
    U32     ulLegNo;
}SC_MSG_EVT_CALL_ST;

/** �����¼��¼� */
typedef struct tagSCMsgEvtRinging{
    SC_MSG_TAG_ST    stMsgTag;              /**< ��Ϣͷ */

    /** ҵ����ƿ��� */
    U32     ulSCBNo;

    /** LEG��� */
    U32     ulLegNo;

    /** �Ƿ����ý�� */
    U32     ulWithMedia;
}SC_MSG_EVT_RINGING_ST;

/** ���н����¼� */
typedef struct tagSCMsgEvtAnswer{
    SC_MSG_TAG_ST    stMsgTag;              /**< ��Ϣͷ */

    /** ҵ����ƿ��� */
    U32     ulSCBNo;

    /** LEG��� */
    U32     ulLegNo;
}SC_MSG_EVT_ANSWER_ST;

/** ���йҶ��¼� */
typedef struct tagSCMsgEvtHungup{
    SC_MSG_TAG_ST    stMsgTag;              /**< ��Ϣͷ */

    /** ҵ����ƿ��� */
    U32     ulSCBNo;

    /** LEG��� */
    U32     ulLegNo;

    /** �Ҷ�ԭ�� */
    U32     ulErrCode;
}SC_MSG_EVT_HUNGUP_ST;

/** DTMF�¼� */
typedef struct tagSCMsgEvtDTMF{
    SC_MSG_TAG_ST    stMsgTag;              /**< ��Ϣͷ */

    /** ҵ����ƿ��� */
    U32     ulSCBNo;

    /** LEG��� */
    U32     ulLegNo;

    /** �Ҷ�ԭ�� */
    U32     ulErrCode;

    /** ��ǰDTMFֵ(����1���Ĵ洢��LEG��Ϣ���棬��ǰ���Ҳ����LEG�б���) */
    S8      cDTMFVal;
}SC_MSG_EVT_DTMF_ST;

typedef enum tagSCMsgEvtPlayMode{
    SC_MSG_EVT_PLAY_START,
    SC_MSG_EVT_PLAY_STOP,

    SC_MSG_EVT_PLAY_BUTT
}SC_MSG_EVT_PLAY_MODE_EN;

/** ���������¼� */
typedef struct tagSCMsgEvtPlayback{
    SC_MSG_TAG_ST    stMsgTag;              /**< ��Ϣͷ */

    /** ҵ����ƿ��� */
    U32     ulSCBNo;

    /** LEG��� */
    U32     ulLegNo;

    /** ��ʾ�Ƿ��ǿ�ʼ��ֹͣ */
    U32     ulFlag;
}SC_MSG_EVT_PLAYBACK_ST;

typedef enum tagSCMsgEvtRecordMode{
    SC_MSG_EVT_RECORD_START,
    SC_MSG_EVT_RECORD_STOP,

    SC_MSG_EVT_RECORD_BUTT
}SC_MSG_EVT_RECORD_MODE_EN;

/** ¼���¼� */
typedef struct tagSCMsgEvtRecord{
    SC_MSG_TAG_ST    stMsgTag;              /**< ��Ϣͷ */

    /** ҵ����ƿ��� */
    U32     ulSCBNo;

    /** LEG��� */
    U32     ulLegNo;

    /** ��ʾ�Ƿ��ǿ�ʼ��ֹͣ */
    U32     ulFlag;
}SC_MSG_EVT_RECORD_ST;

/** HOLD�¼� */
typedef struct tagSCMsgEvtHold{
    SC_MSG_TAG_ST    stMsgTag;              /**< ��Ϣͷ */

    /** ҵ����ƿ��� */
    U32     ulSCBNo;

    /** LEG��� */
    U32     ulLegNo;

    /** ��ʾ�Ƿ���HOLD/UNHOLD */
    U32     blIsHold;
}SC_MSG_EVT_HOLD_ST;

/** HOLD�¼� */
typedef struct tagSCMsgEvtErrReport{
    SC_MSG_TAG_ST    stMsgTag;              /**< ��Ϣͷ */

    /** ҵ����ƿ��� */
    U32     ulSCBNo;

    /** ҵ����ƿ��� */
    U32     ulCMD;

    /** LEG��� */
    U32     ulLegNo;
}SC_MSG_EVT_ERR_REPORT_ST;

/** ��֤��� */
typedef struct tagSCMsgEvtAuthResult{
    SC_MSG_TAG_ST    stMsgTag;              /**< ��Ϣͷ */

    /** ҵ����ƿ��� */
    U32     ulSCBNo;

    /** LEG���ƿ��� */
    U32     ulLCBNo;

    /** ���,��λ:�� */
    S64             LBalance;
    /** �Ự����,������SMS��ҵ�� */
    U32             ulSessionNum;
    /** ��ͨ��ʱ��/�������,��λ:��/�� */
    U32             ulMaxSession;
    /** �Ƿ����澯 */
    U32              ucBalanceWarning;
}SC_MSG_EVT_AUTH_RESULT_ST;

typedef enum tagSCLeaveCallQueReason{
    SC_LEAVE_CALL_QUE_SUCC,
    SC_LEAVE_CALL_QUE_TIMEOUT,

    SC_LEAVE_CALL_QUE_BUTT
}SC_LEAVE_CALL_QUE_REASON_EN;

/** ���ж��н�� */
typedef struct tagSCMsgEvtLeaveCallQue{
    SC_MSG_TAG_ST    stMsgTag;              /**< ��Ϣͷ */

    /** ҵ����ƿ��� */
    U32     ulSCBNo;

    /** ѡ�е���ϯ */
    SC_AGENT_NODE_ST        *pstAgentNode;

}SC_MSG_EVT_LEAVE_CALLQUE_ST;

/**
 * backgroup job�����hash��ڵ�
 */
typedef struct tagSCBGJobHashNode{
    HASH_NODE_S  stHashNode;    /**< HASH�ڵ� */

    U32 ulLegNo;                /**< LEG��� */
    S8  szUUID[SC_UUID_LENGTH]; /**< JOB UUID */
}SC_BG_JOB_HASH_NODE_ST;

/* ����SCBhash�� */
typedef struct tagLCBHashNode
{
    HASH_NODE_S     stNode;                       /* hash����ڵ� */

    S8              szUUID[SC_UUID_LENGTH];   /* UUID */
    SC_LEG_CB       *pstLCB;                      /* LCBָ�� */
}SC_LCB_HASH_NODE_ST;

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

typedef struct tagCallWaitQueueNode{
    U32                 ulAgentGrpID;                     /* ��ϯ��ID */
    U32                 ulStartWaitingTime;               /* ��ʼ�ȴ���ʱ�� */

    pthread_mutex_t     mutexCWQMngt;
    DLL_S               stCallWaitingQueue;               /* ���еȴ����� refer to SC_SCB_ST */
}SC_CWQ_NODE_ST;

/* ���һ��TCB����������Task��CustomID */
#define SC_TCB_HAS_VALID_OWNER(pstTCB)                        \
    ((pstTCB)                                                 \
    && (pstTCB)->ulTaskID != 0                                \
    && (pstTCB)->ulTaskID != U32_BUTT                         \
    && (pstTCB)->ulCustomID != 0                              \
    && (pstTCB)->ulCustomID != U32_BUTT)

#define SC_TCB_VALID(pstTCB)                                  \
    ((pstTCB)                                                 \
    && (pstTCB)->ulTaskID != 0)

typedef struct tagTelNumQueryNode
{
    list_t     stLink;                            /* ��������ڵ� */

    U32        ulIndex;                           /* ���ݿ��е�ID */

    U8         ucTraceON;                         /* �Ƿ���� */
    U8         ucCalleeType;                      /* ���к������ͣ� refer to enum SC_CALL_NUM_TYPE_EN */
    U8         aucRes[2];

    S8         szNumber[SC_NUM_LENGTH];    /* ���뻺�� */
}SC_TEL_NUM_QUERY_NODE_ST;

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

typedef struct tagTaskCB
{
    U16        usTCBNo;                           /* ��� */
    U8         ucValid;                           /* �Ƿ�ʹ�� */
    U8         ucTaskStatus;                      /* ����״̬ refer to SC_TASK_STATUS_EN */

    U32        ulAllocTime;
    U32        ulModifyTime;
    U8         ucPriority;                        /* �������ȼ� */
    U8         ucAudioPlayCnt;                    /* ���Բ��Ŵ��� */
    U8         bTraceON;                          /* �Ƿ���� */
    U8         bTraceCallON;                      /* �Ƿ���ٺ��� */
    S8         szTaskName[64];                    /* �������� */

    U8         ucMode;                            /* ����ģʽ refer to SC_TASK_MODE_EN*/
    U8         bThreadRunning;                    /* �߳��Ƿ������� */
    U8         aucRess[2];

    U32        ulTaskID;                          /* ��������ID */
    U32        ulCustomID;                        /* ������������ */
    U32        ulCurrentConcurrency;              /* ��ǰ������ */
    U32        ulMaxConcurrency;                  /* ��ǰ������ */
    U32        ulAgentQueueID;                    /* ��ϯ���б�� */

    U16        usSiteCount;                       /* ��ϯ���� */
    U16        usCallerCount;                     /* ��ǰ���к������� */
    U32        ulCalleeCount;                     /* ��ǰ���к������� */
    U32        ulLastCalleeIndex;                 /* �������ݷ�ҳ */
    U32        ulCalleeCountTotal;                /* ���������ܵı��к������� */
    U32        ulCalledCount;                     /* �Ѿ����й��ĺ������� */
    U32        ulCalledCountLast;                 /* ��һ��ͬ��ʱ�������Ѿ����й��ĺ������� */
    U32        ulCallerGrpID;                     /* ���к������ID */
    U32        ulCallRate;                        /* ���б��� */
    list_t     stCalleeNumQuery;                  /* ���к��뻺�� refer to struct tagTelNumQueryNode */
    S8         szAudioFileLen[SC_MAX_AUDIO_FILENAME_LEN];  /* �����ļ��ļ��� */
    SC_TASK_ALLOW_PERIOD_ST astPeriod[SC_MAX_PERIOD_NUM];  /* ����ִ��ʱ��� */

    /* ͳ����� */
    U32        ulTotalCall;                       /* �ܺ����� */
    U32        ulCallFailed;                      /* ����ʧ���� */
    U32        ulCallConnected;                   /* ���н�ͨ�� */

    DOS_TMR_ST pstTmrHandle;                      /* ��ʱ�����������ݿ��У��Ѿ����й��ĺ������� */

    pthread_t  pthID;                             /* �߳�ID */
    pthread_mutex_t  mutexTaskList;               /* �����������ʹ�õĻ����� */
}SC_TASK_CB;

typedef struct tagSCInconmingCallNode
{
    SC_SRV_CB   *pstSCB;
    S8          szCaller[SC_NUM_LENGTH];
}SC_INCOMING_CALL_NODE_ST;

typedef enum tagAccessCodeType{
    SC_ACCESS_BALANCE_ENQUIRY   = 0,          /**< ����ѯ */
    SC_ACCESS_AGENT_ONLINE      = 1,          /**< ��ϯ��½ */
    SC_ACCESS_AGENT_OFFLINE     = 2,          /**< ��ϯ�ǳ� */
    SC_ACCESS_AGENT_EN_QUEUE    = 3,          /**< ��ϯ���� */
    SC_ACCESS_AGENT_DN_QUEUE    = 4,          /**< ��ϯ��æ */
    SC_ACCESS_AGENT_SIGNIN      = 5,          /**< ��ϯ��ǩ */
    SC_ACCESS_AGENT_SIGNOUT     = 6,          /**< ��ϯ�˳���ǩ */
    SC_ACCESS_MARK_CUSTOMER     = 7,          /**< ��ǿͻ� */
    SC_ACCESS_BLIND_TRANSFER    = 8,          /**< äת */
    SC_ACCESS_ATTENDED_TRANSFER = 9,          /**< Э��ת */
    SC_ACCESS_HANGUP_CUSTOMER1  = 10,         /**< �ҶϿͻ��ĵ绰1 */
    SC_ACCESS_HANGUP_CUSTOMER2  = 11,         /**< �ҶϿͻ��ĵ绰2 */

    SC_ACCESS_BUTT

}SC_ACCESS_CODE_TYPE_EN;

typedef struct tagAccessCodeList
{
    U32  ulType;                                /**< ���������� */
    S8   szCodeFormat[SC_ACCESS_CODE_LEN];      /**< �������ʽ */
    BOOL bSupportDirectDial;                    /**< �Ƿ�֧��ֱ�Ӳ��� */
    BOOL bSupportSecondDial;                    /**< �Ƿ�֧�ֶ��β��� */
    BOOL bExactMatch;                           /**< �Ƿ�����ȫƥ�� */
    BOOL bValid;                                /**< �Ƿ���Ч */
    U32   (*fn_init)(SC_SRV_CB *, SC_LEG_CB *); /**< ����Ŵ����� */
}SC_ACCESS_CODE_LIST_ST;

/** ����ͳ����� */
typedef struct tagSysStat{
    U32      ulCRC;

    U32      ulCurrentCalls;       /**< ��ǰ��������ҵ����ƿ����ĸ��� */
    U32      ulIncomingCalls;      /**< ��ǰ������������/�ͷ�LEGʱ���� */
    U32      ulOutgoingCalls;      /**< ��ǰ������������/�ͷ�LEGʱ���� */

    U32      ulTotalTime;          /**< ��ʱ�� */
    U32      ulOutgoingTime;       /**< ���� */
    U32      ulIncomingTime;       /**< ���� */
    U32      ulAutoCallTime;       /**< �Զ�����ʱ�� */
    U32      ulPreviewCallTime;    /**< Ԥ�����ʱ�� */
    U32      ulPredictiveCallTime; /**< Ԥ�����ʱ�� */
    U32      ulInternalCallTime;   /**< �ڲ�����ʱ�� */
}SC_SYS_STAT_ST;


extern SC_SYS_STAT_ST       g_stSysStat;
extern SC_SYS_STAT_ST       g_stSysStatLocal;


VOID sc_scb_init(SC_SRV_CB *pstSCB);
SC_SRV_CB *sc_scb_alloc();
VOID sc_scb_free(SC_SRV_CB *pstSCB);
SC_SRV_CB *sc_scb_get(U32 ulCBNo);
U32 sc_scb_set_service(SC_SRV_CB *pstSCB, U32 ulService);
U32 sc_scb_check_service(SC_SRV_CB *pstSCB, U32 ulService);
U32 sc_scb_remove_service(SC_SRV_CB *pstSCB, U32 ulService);
BOOL sc_scb_is_exit_service(SC_SRV_CB *pstSCB, U32 ulService);

VOID sc_lcb_init(SC_LEG_CB *pstLCB);
SC_LEG_CB *sc_lcb_alloc();
VOID sc_lcb_free(SC_LEG_CB *pstLCB);
SC_LEG_CB *sc_lcb_get(U32 ulCBNo);
SC_LEG_CB *sc_lcb_hash_find(S8 *pszUUID);
U32 sc_lcb_hash_add(S8 *pszUUID, SC_LEG_CB *pstLCB);
U32 sc_lcb_hash_delete(S8 *pszUUID);
U32 sc_leg_get_destination(SC_SRV_CB *pstSCB, SC_LEG_CB  *pstLegCB);
U32 sc_leg_get_source(SC_SRV_CB *pstSCB, SC_LEG_CB  *pstLegCB);

U32 sc_bgjob_hash_add(U32 ulLegNo, S8 *pszUUID);

U32 sc_get_call_limitation();

VOID sc_lcb_playback_init(SC_SU_PLAYBACK_ST *pstPlayback);


U32 sc_send_event_call_create(SC_MSG_EVT_CALL_ST *pstEvent);
U32 sc_send_event_err_report(SC_MSG_EVT_ERR_REPORT_ST *pstEvent);
U32 sc_send_event_auth_rsp(SC_MSG_EVT_AUTH_RESULT_ST *pstEvent);
U32 sc_send_event_answer(SC_MSG_EVT_ANSWER_ST *pstEvent);
U32 sc_send_event_release(SC_MSG_EVT_HUNGUP_ST *pstEvent);
U32 sc_send_event_ringing(SC_MSG_EVT_RINGING_ST *pstEvent);
U32 sc_send_event_hold(SC_MSG_EVT_HOLD_ST *pstEvent);
U32 sc_send_event_dtmf(SC_MSG_EVT_DTMF_ST *pstEvent);
U32 sc_send_event_record(SC_MSG_EVT_RECORD_ST *pstEvent);
U32 sc_send_event_playback(SC_MSG_EVT_PLAYBACK_ST *pstEvent);
U32 sc_send_event_leave_call_queue_rsp(SC_MSG_EVT_LEAVE_CALLQUE_ST *pstEvent);
U32 sc_send_usr_auth2bs(SC_SRV_CB *pstSCB, SC_LEG_CB *pstLegCB);
U32 sc_send_balance_query2bs(SC_SRV_CB *pstSCB, SC_LEG_CB *pstLegCB);

U32 sc_req_hungup(U32 ulSCBNo, U32 ulLegNo, U32 ulErrNo);
U32 sc_req_bridge_call(U32 ulSCBNo, U32 ulCallingLegNo, U32 ulCalleeLegNo);
U32 sc_req_ringback(U32 ulSCBNo, U32 ulLegNo, BOOL blHasMedia);
U32 sc_req_answer_call(U32 ulSCBNo, U32 ulLegNo);
U32 sc_req_play_sound(U32 ulSCBNo, U32 ulLegNo, U32 ulSndInd, U32 ulLoop, U32 ulInterval, U32 ulSilence);
U32 sc_req_play_sounds(U32 ulSCBNo, U32 ulLegNo, U32 *pulSndInd, U32 ulSndCnt, U32 ulLoop, U32 ulInterval, U32 ulSilence);
U32 sc_req_playback_stop(U32 ulSCBNo, U32 ulLegNo);
U32 sc_req_hungup_with_sound(U32 ulSCBNo, U32 ulLegNo, U32 ulErrNo);
U32 sc_req_hold(U32 ulSCBNo, U32 ulLegNo, U32 ulFlag);
U32 sc_send_cmd_new_call(SC_MSG_TAG_ST *pstMsg);
U32 sc_send_cmd_playback(SC_MSG_TAG_ST *pstMsg);
U32 sc_send_cmd_mux(SC_MSG_TAG_ST *pstMsg);
U32 sc_send_cmd_record(SC_MSG_TAG_ST *pstMsg);

U32 sc_agent_group_agent_count(U32 ulGroupID);
U32 sc_agent_stat(U32 ulType, SC_AGENT_INFO_ST *pstAgentInfo, U32 ulAgentID, U32 ulParam);
SC_AGENT_NODE_ST *sc_agent_get_by_id(U32 ulAgentID);
SC_AGENT_NODE_ST *sc_agent_get_by_emp_num(U32 ulCustomID, S8 *pszEmpNum);
SC_AGENT_NODE_ST *sc_agent_get_by_sip_acc(S8 *szUserID);
SC_AGENT_NODE_ST *sc_agent_get_by_tt_num(S8 *szTTNumber);
SC_AGENT_NODE_ST *sc_agent_select_by_grpid(U32 ulGroupID, S8 *szCallerNum);
U32 sc_agent_hash_func4grp(U32 ulGrpID, U32 *pulHashIndex);
S32 sc_agent_group_hash_find(VOID *pSymName, HASH_NODE_S *pNode);
U32 sc_agent_group_stat_by_id(U32 ulGroupID, U32 *pulTotal, U32 *pulWorking, U32 *pulIdel, U32 *pulBusy);
U32 sc_agent_init(U32 ulIndex);
U32 sc_agent_group_init(U32 ulIndex);
U32 sc_agent_status_update(U32 ulAction, U32 ulAgentID, U32 ulOperatingType);
U32 sc_agent_http_update_proc(U32 ulAction, U32 ulAgentID, S8 *pszUserID);
U32 sc_agent_query_idel(U32 ulAgentGrpID, BOOL *pblResult);
U32 sc_agent_auto_callback(SC_SRV_CB *pstSCB, SC_AGENT_NODE_ST *pstAgentNode);
U32 sc_demo_task_callback(SC_SRV_CB *pstSCB, SC_AGENT_NODE_ST *pstAgentNode);
U32 sc_agent_call_by_id(SC_SRV_CB *pstSCB, SC_LEG_CB *pstCallingLegCB, U32 ulAgentID, U32 *pulErrCode);
U32 sc_agent_call_notify(SC_AGENT_INFO_ST *pstAgentInfo, S8 *szCaller);
U32 sc_agent_serv_status_update(SC_AGENT_INFO_ST *pstAgentQueueInfo, U32 ulServStatus);
U32 sc_agent_update_status_db(SC_AGENT_INFO_ST *pstAgentInfo);

U32 sc_agent_access_set_sigin(SC_AGENT_NODE_ST *pstAgent, SC_SRV_CB *pstSCB, SC_LEG_CB *pstLegCB);
void sc_agent_mark_custom_callback(U64 arg);

U32 sc_call_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_call_setup(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_call_answer(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_call_ringing(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_call_bridge(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_call_unbridge(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_call_hold(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_call_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_call_dtmf(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_call_record_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_call_playback_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_call_queue_leave(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_call_error(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);

U32 sc_sigin_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_sigin_setup(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_sigin_ringing(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_sigin_answer(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_sigin_playback_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_sigin_dtmf(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_sigin_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_sigin_error(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);

U32 sc_preview_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_preview_setup(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_preview_answer(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_preview_ringing(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_preview_hold(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_preview_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_preview_dtmf(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_preview_record_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_preview_playback_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_preview_error(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);

U32 sc_voice_verify_proc(U32 ulCustomer, S8 *pszNumber, S8 *pszPassword, U32 ulPlayCnt);
U32 sc_voice_verify_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_voice_verify_setup(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_voice_verify_answer(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_voice_verify_ringing(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_voice_verify_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_voice_verify_playback_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_voice_verify_error(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);

U32 sc_interception_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_interception_setup(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_interception_answer(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_interception_ringing(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_interception_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_interception_error(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);

U32 sc_whisper_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_whisper_setup(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_whisper_answer(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_whisper_ringing(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_whisper_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_whisper_error(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);

U32 sc_auto_call_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_auto_call_setup(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_auto_call_ringing(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_auto_call_answer(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_auto_call_dtmf(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_auto_call_palayback_end(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_auto_call_queue_leave(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_auto_call_record_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_auto_call_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_auto_call_error(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);

U32 sc_transfer_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_transfer_setup(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_transfer_answer(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_transfer_ringing(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_transfer_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_transfer_error(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);

U32 sc_incoming_playback_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_incoming_queue_leave(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);

U32 sc_mark_custom_dtmf(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_mark_custom_playback_start(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_mark_custom_playback_end(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_mark_custom_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_mark_custom_error(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);

U32 sc_access_code_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_access_code_dtmf(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_access_code_playback_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_access_code_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_access_code_error(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);

U32 sc_demo_task_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_demo_task_setup(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_demo_task_answer(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_demo_task_ringing(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_demo_task_dtmf(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_demo_task_record_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_demo_task_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_demo_task_palayback_end(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);
U32 sc_demo_task_error(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB);

U32 sc_agent_marker_update_req(U32 ulCustomID, U32 ulAgentID, S32 lKey, S8 *szCallerNum);

SC_TASK_CB *sc_tcb_alloc();
SC_TASK_CB *sc_tcb_find_by_taskid(U32 ulTaskID);
VOID sc_tcb_free(SC_TASK_CB *pstTCB);
VOID sc_task_set_owner(SC_TASK_CB *pstTCB, U32 ulTaskID, U32 ulCustomID);
U32 sc_task_check_can_call(SC_TASK_CB *pstTCB);
U32 sc_task_check_can_call_by_time(SC_TASK_CB *pstTCB);
U32 sc_task_check_can_call_by_status(SC_TASK_CB *pstTCB);
S32 sc_task_and_callee_load(U32 ulIndex);
U32 sc_task_get_mode(U32 ulTCBNo);
U32 sc_task_get_playcnt(U32 ulTCBNo);
S8 *sc_task_get_audio_file(U32 ulTCBNo);
U32 sc_task_get_agent_queue(U32 ulTCBNo);

U32 sc_internal_call_process(SC_SRV_CB *pstSCB, SC_LEG_CB *pstLegCB);
U32 sc_outgoing_call_process(SC_SRV_CB *pstSCB, SC_LEG_CB *pstCallingLegCB);
U32 sc_incoming_call_proc(SC_SRV_CB *pstSCB, SC_LEG_CB *pstCallingLegCB);
U32 sc_make_call2pstn(SC_SRV_CB *pstSCB, SC_LEG_CB *pstLCB);
U32 sc_make_call2eix(SC_SRV_CB *pstSCB, SC_LEG_CB *pstLCB);
U32 sc_make_call2sip(SC_SRV_CB *pstSCB, SC_LEG_CB *pstLCB);

U32 sc_call_ctrl_call_agent(U32 ulCurrentAgent, SC_AGENT_NODE_ST  *pstAgentNode);
U32 sc_call_ctrl_call_sip(U32 ulAgent, S8 *pszSipNumber);
U32 sc_call_ctrl_call_out(U32 ulAgent, U32 ulTaskID, S8 *pszNumber);
U32 sc_call_ctrl_transfer(U32 ulAgent, U32 ulAgentCalled, BOOL bIsAttend);
U32 sc_call_ctrl_hold(U32 ulAgent, BOOL isHold);
U32 sc_call_ctrl_unhold(U32 ulAgent);
U32 sc_call_ctrl_hangup(U32 ulAgent);
U32 sc_call_ctrl_proc(U32 ulAction, U32 ulTaskID, U32 ulAgent, U32 ulCustomerID, U32 ulType, S8 *pszCallee, U32 ulFlag, U32 ulCalleeAgentID);
U32 sc_demo_task(U32 ulCustomerID, S8 *pszCallee, S8 *pszAgentNum, U32 ulAgentID);
U32 sc_demo_preview(U32 ulCustomerID, S8 *pszCallee, S8 *pszAgentNum, U32 ulAgentID);

U32 sc_did_bind_info_get(S8 *pszDidNum, U32 *pulBindType, U32 *pulBindID);
U32 sc_sip_account_get_by_id(U32 ulSipID, S8 *pszUserID, U32 ulLength);
BOOL sc_customer_is_exit(U32 ulCustomerID);
U32 sc_log_digest_print(const S8 *szTraceStr);
U32 sc_cwq_add_call(SC_SRV_CB *pstSCB, U32 ulAgentGrpID, S8 *szCaller);

U32 sc_access_balance_enquiry(SC_SRV_CB *pstSCB, SC_LEG_CB *pstLegCB);
U32 sc_access_agent_proc(SC_SRV_CB *pstSCB, SC_LEG_CB *pstLegCB);
U32 sc_access_transfer(SC_SRV_CB *pstSCB, SC_LEG_CB *pstLegCB);
U32 sc_access_hungup(SC_SRV_CB *pstSCB, SC_LEG_CB *pstLegCB);

U32 sc_send_sip_update_req(U32 ulID, U32 ulAction);
U32 sc_send_gateway_update_req(U32 ulID, U32 ulAction);

#endif  /* end of __SC_DEF_V2_H__ */

#ifdef __cplusplus
}
#endif /* End of __cplusplus */


