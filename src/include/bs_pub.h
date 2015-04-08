/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  �ļ���
 *
 *  ����ʱ��:
 *  ��    ��:
 *  ��    ��:
 *  �޸���ʷ:
 */

#ifndef __BS_PUB_H__
#define __BS_PUB_H__

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include "json-c/json.h"


#define BS_MSG_INTERFACE_VERSION        0x101   /* ��Ϣ�汾�� */
#define BS_MAX_PASSWORD_LEN             16      /* ������볤�� */
#define BS_MAX_SESSION_ID_LEN           40      /* �ỰID��󳤶� */
#define BS_MAX_CALL_NUM_LEN             24      /* ������󳤶� */
#define BS_MAX_AGENT_NUM_LEN            8       /* ��ϯ������󳤶� */
#define BS_MAX_SERVICE_TYPE_IN_SESSION  3       /* �����Ự��ҵ���������������� */
#define BS_MAX_SESSION_LEG_IN_BILL      2       /* ���������лỰleg��������,����С��2 */
#define BS_MAX_RECORD_FILE_NAME_LEN     64      /* ¼���ļ�����󳤶� */
#define BS_MAX_AGENT_LEVEL              5       /* ���������㼶 */
#define BS_MAX_REMARK_LEN               32      /* ��ע��󳤶� */

#define BS_MAX_RULE_ITEM                20      /* �ʷѹ�����ֶ��� */



/* ��������� */
enum BS_ERRCODE_E
{
    BS_ERR_SUCC                 = 0,        /* �ɹ� */
    BS_ERR_NOT_EXIST            = 1,        /* ������ */
    BS_ERR_EXPIRE               = 2,        /* ����/ʧЧ */
    BS_ERR_FROZEN               = 3,        /* ������ */
    BS_ERR_LACK_FEE             = 4,        /* ���� */
    BS_ERR_PASSWORD             = 5,        /* ������� */
    BS_ERR_RESTRICT             = 6,        /* ҵ������ */
    BS_ERR_OVER_LIMIT           = 7,        /* �������� */
    BS_ERR_TIMEOUT              = 8,        /* ��ʱ */
    BS_ERR_LINK_DOWN            = 9,        /* �����ж� */
    BS_ERR_SYSTEM               = 10,       /* ϵͳ���� */
    BS_ERR_MAINTAIN             = 11,       /* ϵͳά���� */
    BS_ERR_DATA_ABNORMAL        = 12,       /* �����쳣*/
    BS_ERR_PARAM_ERR            = 13,       /* �������� */
    BS_ERR_NOT_MATCH            = 14,       /* ��ƥ�� */

    BS_ERR_BUTT                 = 255
};

enum BS_CALL_TERMINATE_CAUSE_E
{
    BS_TERM_NONE                = 0,
    BS_TERM_SCB_LEEK,
    BS_TERM_AUTH_FAIL,
    BS_TERM_INIT_FAIL,
    BS_TERM_SERV_FAIL,
    BS_TERM_COMM_FAIL,
    BS_TERM_SERV_FORBID,
    BS_TERM_INTERNAL_ERR,
    BS_TERM_TASK_PARAM_ERR,
    BS_TERM_NUM_INVALID,
    BS_TERM_SERV_INVALID,
    BS_TERM_CUSTOM_INVALID,
    BS_TERM_QUEUE_INVALID,
    BS_TERM_HANGUP,
    BS_TERM_UNKNOW,

    BS_TERM_BUTT
};

/* ��Ϣ���Ͷ���,0Ϊ��Чֵ */
enum BS_MSG_TYPE_E
{
    BS_MSG_HELLO_REQ            = 1,        /* �������� */
    BS_MSG_HELLO_RSP            = 2,        /* ������Ӧ */
    BS_MSG_START_UP_NOTIFY      = 3,        /* ����֪ͨ */

    BS_MSG_BALANCE_QUERY_REQ    = 11,       /* ��ѯ��� */
    BS_MSG_BALANCE_QUERY_RSP    = 12,       /* ��ѯ��� */
    BS_MSG_USER_AUTH_REQ        = 13,       /* �û���Ȩ */
    BS_MSG_USER_AUTH_RSP        = 14,       /* �û���Ȩ */
    BS_MSG_ACCOUNT_AUTH_REQ     = 15,       /* �ʺż�Ȩ */
    BS_MSG_ACCOUNT_AUTH_RSP     = 16,       /* �ʺż�Ȩ */
    BS_MSG_BILLING_START_REQ    = 17,       /* ��ʼ�Ʒ� */
    BS_MSG_BILLING_START_RSP    = 18,       /* ��ʼ�Ʒ� */
    BS_MSG_BILLING_UPDATE_REQ   = 19,       /* �м�Ʒ� */
    BS_MSG_BILLING_UPDATE_RSP   = 10,       /* �м�Ʒ� */
    BS_MSG_BILLING_STOP_REQ     = 21,       /* �����Ʒ� */
    BS_MSG_BILLING_STOP_RSP     = 22,       /* �����Ʒ� */
    BS_MSG_BILLING_RELEASE_IND  = 23,       /* �ͷ�ָʾ */
    BS_MSG_BILLING_RELEASE_ACK  = 24,       /* �ͷ�ȷ�� */

    BS_MSG_BUTT                 = 255
};


/* ҵ�����Ͷ���,0Ϊ��Чֵ;��󲻳���31��ҵ������ */
enum BS_SERV_TYPE_E
{
    BS_SERV_OUTBAND_CALL        = 1,        /* ���ֺ��� */
    BS_SERV_INBAND_CALL         = 2,        /* ��ֺ��� */
    BS_SERV_INTER_CALL          = 3,        /* �ڲ����� */
    BS_SERV_AUTO_DIALING        = 4,        /* �Զ����(�������) */
    BS_SERV_PREVIEW_DIALING     = 5,        /* Ԥ��ʽ��� */
    BS_SERV_PREDICTIVE_DIALING  = 6,        /* Ԥ��ʽ��� */
    BS_SERV_RECORDING           = 7,        /* ¼��ҵ�� */
    BS_SERV_CALL_FORWARD        = 8,        /* ����ǰת */
    BS_SERV_CALL_TRANSFER       = 9,        /* ����ת�� */
    BS_SERV_PICK_UP             = 10,       /* ���д��� */
    BS_SERV_CONFERENCE          = 11,       /* ����绰 */
    BS_SERV_VOICE_MAIL          = 12,       /* voice mail */
    BS_SERV_SMS_SEND            = 13,       /* ���ŷ���ҵ�� */
    BS_SERV_SMS_RECV            = 14,       /* ���Ž���ҵ�� */
    BS_SERV_MMS_SEND            = 15,       /* ���ŷ���ҵ�� */
    BS_SERV_MMS_RECV            = 16,       /* ���Ž���ҵ�� */
    BS_SERV_RENT                = 17,       /* ���ҵ�� */
    BS_SERV_SETTLE              = 18,       /* ����ҵ�� */

    BS_SERV_VERIFY              = 19,       /* ������֤��ҵ�� */

    BS_SERV_BUTT                = 31
};

/* �����ͷŷ�����,0Ϊ��Чֵ */
enum BS_SESSION_RELEASE_PART_E
{
    BS_SESS_RELEASE_BY_CALLER   = 1,        /* �����ͷ� */
    BS_SESS_RELEASE_BY_CALLEE   = 2,        /* �����ͷ� */
    BS_SESS_RELEASE_BY_SYS      = 3,        /* ϵͳ�ͷ� */

    BS_SESS_RELEASE_PART_BUTT   = 255
};

/*
������Ϣ����ͷ,���ֽ��ֶ�ͳһʹ��������
������Ϣ/����&�Ʒ���Ӧ��Ϣ/����֪ͨ��Ϣ���ñ��ṹ
*/
typedef struct
{
    U16     usVersion;          /* �ӿ�Э��汾�� */
    U16     usPort;             /* ͨѶ�˿�,��дΪ���Ͳ�����˿� */
    U32     aulIPAddr[4];       /* IP��ַ,��дΪ���Ͳ�IP;����IPv6 */
    U32     ulMsgSeq;           /* ��Ϣ���к� */
    U32     ulCRNo;             /* �ỰID */
    U8      ucMsgType;          /* ��Ϣ���� */
    U8      ucErrcode;          /* ������ */
    U16     usMsgLen;           /* ��Ϣ�ܳ��� */
    U8      aucReserv[40];      /* ����,����������Ҫ����,���Դ��MD5����ժҪ */
}BS_MSG_TAG;


/*
��ѯ��Ϣ�ṹ��,��������Ӧ����
�ر�ע��:�漰��ID��������ֶ�,0Ϊ��Чֵ
*/
typedef struct
{
    BS_MSG_TAG      stMsgTag;

    U32             ulUserID;                   /* �û�ID,Ҫ��ȫ����,������10λ,���λС��4 */
    U32             ulCustomerID;               /* �ͻ�ID,Ҫ��ȫ����,������10λ,���λС��4 */
    U32             ulAccountID;                /* �˻�ID,Ҫ��ȫ����,������10λ,���λС��4 */
    S32             lBalance;                   /* ��� */
    U8              ucBalanceWarning;           /* �Ƿ����澯 */
    U8              ucServType;                 /* ҵ������ */
    U8              aucReserv[32];              /* ���� */

} BS_MSG_BALANCE_QUERY;


/*
��֤��Ϣ�ṹ��,�û���֤/�˺���֤����,��������Ӧ����
�ر�ע��:�漰��ID��������ֶ�,0Ϊ��Чֵ
�ʷ��ײ�ID:��BS�ش���APP,session��ֻҪ�ͻ���ҵ������δ��,APP�������͵ļƷ���ϢҪЯ�����ֶ�
*/
typedef struct
{
    BS_MSG_TAG      stMsgTag;

    U32             ulUserID;                   /* �û�ID,Ҫ��ȫ����,������10λ,���λС��4 */
    U32             ulAgentID;                  /* ��ϯID,Ҫ��ȫ����,������10λ,���λС��4 */
    U32             ulCustomerID;               /* �ͻ�ID,Ҫ��ȫ����,������10λ,���λС��4 */
    U32             ulAccountID;                /* �˻�ID,Ҫ��ȫ����,������10λ,���λС��4 */
    U32             ulTaskID;                   /* ����ID,Ҫ��ȫ����,������10λ,���λС��4 */
    S32             lBalance;                   /* ���,��λ:�� */
    S8              szPwd[BS_MAX_PASSWORD_LEN]; /* ���� */
    U32             ulTimeStamp;                /* ʱ��� */
    U32             ulSessionNum;               /* �Ự����,������SMS��ҵ�� */
    U32             ulMaxSession;               /* ��ͨ��ʱ��/�������,��λ:��/�� */
    S8              szSessionID[BS_MAX_SESSION_ID_LEN];             /* �ỰID */
    S8              szCaller[BS_MAX_CALL_NUM_LEN];                  /* ���к��� */
    S8              szCallee[BS_MAX_CALL_NUM_LEN];                  /* ���к��� */
    S8              szCID[BS_MAX_CALL_NUM_LEN];                     /* ���Ժ��� */
    S8              szAgentNum[BS_MAX_AGENT_NUM_LEN];               /* ��ϯ���� */

    U8              ucBalanceWarning;           /* �Ƿ����澯 */
    U8              aucServType[BS_MAX_SERVICE_TYPE_IN_SESSION];    /* ҵ������ */
    U8              aucReserv[32];              /* ���� */

} BS_MSG_AUTH;

/*
����SESSION LEG�ṹ��
Ҫ��:
1.�����еĺ���,ʹ����ʵ����,�Ǳ任���ĺ���;
2.�����ҵ��,��������Ʒ���ʹ��,��Ϣ��һ�㲻�����;
*/
typedef struct
{
    U32             ulCDRMark;                  /* �������,BSʹ�� */
    U32             ulUserID;                   /* �û�ID,Ҫ��ȫ����,������10λ,���λС��4 */
    U32             ulAgentID;                  /* ��ϯID,Ҫ��ȫ����,������10λ,���λС��4 */
    U32             ulCustomerID;               /* �ͻ�ID,Ҫ��ȫ����,������10λ,���λС��4 */
    U32             ulAccountID;                /* �˻�ID,Ҫ��ȫ����,������10λ,���λС��4 */
    U32             ulTaskID;                   /* ����ID,Ҫ��ȫ����,������10λ,���λС��4 */
    S8              szRecordFile[BS_MAX_RECORD_FILE_NAME_LEN];      /* Ϊ�մ���δ¼�� */
    S8              szCaller[BS_MAX_CALL_NUM_LEN];                  /* ���к��� */
    S8              szCallee[BS_MAX_CALL_NUM_LEN];                  /* ���к��� */
    S8              szCID[BS_MAX_CALL_NUM_LEN];                     /* ���Ժ��� */
    S8              szAgentNum[BS_MAX_AGENT_NUM_LEN];               /* ��ϯ���� */
    S8              szSessionID[BS_MAX_SESSION_ID_LEN];             /* �ỰID */
    U32             ulStartTimeStamp;           /* ��ʼʱ��� */
    U32             ulRingTimeStamp;            /* ����ʱ��� */
    U32             ulAnswerTimeStamp;          /* Ӧ��ʱ��� */
    U32             ulIVRFinishTimeStamp;       /* IVR�������ʱ��� */
    U32             ulDTMFTimeStamp;            /* (��һ��)���β���ʱ��� */
    U32             ulBridgeTimeStamp;          /* LEG�Ž�ʱ��� */
    U32             ulByeTimeStamp;             /* �ͷ�ʱ��� */
    U32             ulHoldCnt;                  /* ���ִ��� */
    U32             ulHoldTimeLen;              /* ������ʱ��,��λ:�� */
    U32             aulPeerIP[4];               /* �Զ�IP��ַ,��дΪ���Ͳ�IP;����IPv6 */
    U16             usPeerTrunkID;              /* �Զ��м�ID */
    U16             usTerminateCause;           /* ��ֹԭ�� */
    U8              ucReleasePart;              /* �Ự�ͷŷ� */
    U8              ucPayloadType;              /* ý������ */
    U8              ucPacketLossRate;           /* �հ�������,0-100 */
    U8              aucServType[BS_MAX_SERVICE_TYPE_IN_SESSION];    /* ҵ������ */
    U8              aucReserv[2];               /* ���� */

}BS_BILL_SESSION_LEG;


/*
������Ϣ�ṹ��
�ر�ע��:�漰��ID��������ֶ�,0Ϊ��Чֵ
Ҫ��:
1.SESSION LEG����,��ЧLEG��ǰ,��ЧLEG�ں�,Ҳ��,һ��LEG����Ч��,�������Ч;
2.������ҵ����Ŀ������LEGΪ��LEG,������ҵ����Դ����LEGΪ��LEG;�ڲ������Ա�������LEGΪ��LEG;
3.���й����п�����Ϊ������ת�Ӻ��е���ҵ������,��ʱ,�ʼ����ҵ����LEGδת�Ƶ��򱣳�,����ת�Ƶ���LEG��;
4.��LEG����ڻ�����Ϣ�еĵ�һ��LEG��,������Ҫ�ļƷѲο�����;
5.ҵ����������,������ڶ��,������ҵ����ʱ���Ⱥ�������д;���ͬʱ����,������ҵ�����ں���(����¼��,ת�ӵ�);
6.���ֺ���/�ڲ�����,������¼�Ա�������legΪ��;e.g.
  a.A����B,��������LEG B��Ϣ(��LEG);
  b.A����B,A������ת�ӵ�C,���ҹ�����A�һ�,��ôA�һ�ʱ����A-C���ڲ����л���(A��B�ĺ��й��̰�����LEG B��);
    B/C�һ�ʱ����2��LEG, B LEG����, C LEGת��,�ֱ�����2�Ż���,B LEG����������ȻΪA;
    ���CΪPSTN����,��ôC LEG������һ�����ֺ���ҵ��,��Ӧ����һ�ų��ֻ���;
    ���Aת�ӵ�C,��C�ȹһ�,A��B�ָ�ͨ��,������һ��B��C�ĺ��л���,ҵ������Ϊ�ڲ�����,A/B�����a����;
7.��ֺ���,������¼����������legΪ��;e.g.
  a.A����B,��������leg A��Ϣ(��LEG);
  b.A����B,B������ǰתC,��ô����LEG A�ĺ��뻰����B-C��ǰת���л���;
  b.A����B,B������ת�ӵ�C,���ҹ�����B�һ�,��ôB�һ�ʱ����B-C���ڲ����л���(B��A�ĺ��й��̰�����LEG A��);;
    A/C�һ�ʱ����2��LEG, A LEG���, C LEGת��,�ֱ�����2�Ż���,A LEG����������ȻΪB;
    ���CΪPSTN����,��ôC LEG������һ�����ֺ���ҵ��,��Ӧ����һ�ų��ֻ���;
    ���Bת�ӵ�C��C�ȹһ�,B��A�ָ�ͨ��,������һ��B��C�ĺ��л���,ҵ������Ϊ�ڲ�����,A/B�����a����;
8.���,������¼��ϵ�һ���͵ڶ���session leg;
  �������/Ԥ�����,��һ��leg��ӦPSTN�û�,�ڶ���leg��Ӧ��ϯ;
  Ԥ�����,��һ��leg��Ӧ��ϯ(ҵ������:�ڲ�����/���ֺ���),�ڶ���leg��ӦPSTN����;
  �ر��,���ҵ��,ת�Ӻ���ʱ,����ת�ӷ�,��Ҫ�����������ת�ӷ����ڲ����л���;e.g.
  ���A,������ϯB,Bת�ӵ�C,Bת�Ӻ�һ�,������B��C���ڲ����л�����,
  ��Ҫ����A��B���ڲ����л���(����ʹ���ض�����,CIDΪA����,��������Ϊ�ڲ�����),���ڼ�¼ת�ӹ���;
9.¼��ҵ����Ϊ����ҵ��,һ��������ҵ�����ͽ�ϳ���;
  ͬһ��¼��,Ҫ��������LEG��ֻ�ܳ���һ��;
  ����δ����,¼���ѽ����������,���ȷ���¼������;�����Ļ����в����ٰ���ԭ¼��ҵ��,��ֹ�ظ��Ʒ�;
10.����ǰ��/����ת��,ֻ��Ŀ�ĺ������ڵ�leg�в��д�ҵ��,����leg��Ӧ���ָ�ҵ��;����Ϊת�ӷ�,CIDΪת�Ӻ�Է�����;
11.���д���,ֻ�д�������leg�д�ҵ��,����leg��Ӧ���ָ�ҵ��;ע��:��Ҫʹ���������LEG��;
12.����绰,ÿһ���绰����������һ������;
13.voice mail,��������,�ڵڶ���leg�г���;�û���ȡ����,�ڵ�һ��leg�г���;
14.��Ϣ��ҵ��,ֻ��Ҫһ��LEG����;
*/
typedef struct
{
    BS_MSG_TAG      stMsgTag;

    /* �ỰLEG,Ҫ��ҵ����,�κ�һ���ỰLEG�ͷ�,��Ҫ����һ������ */
    BS_BILL_SESSION_LEG astSessionLeg[BS_MAX_SESSION_LEG_IN_BILL];
    U8              ucLegNum;                   /* LEG���� */
    U8              aucReserv[31];              /* ���� */

} BS_MSG_CDR;


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif


