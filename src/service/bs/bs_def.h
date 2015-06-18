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

#ifndef __BS_DEF_H__
#define __BS_DEF_H__

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#define BS_ONLY_LISTEN_LOCAL                DOS_FALSE/* �Ƿ�ֻ�������� */
#define BS_LOCAL_IP_ADDR                    ((127<<24)|1)

#define BS_HASH_TBL_CUSTOMER_SIZE           (1<<9)      /* �ͻ���ϢHASH���С */
#define BS_HASH_TBL_AGENT_SIZE              (1<<12)     /* ��ϯ��ϢHASH���С */
#define BS_HASH_TBL_BILLING_PACKAGE_SIZE    (1<<8)      /* �ƷѰ�HASH���С */
#define BS_HASH_TBL_SETTLE_SIZE             (1<<6)      /* ����HASH���С */
#define BS_HASH_TBL_TASK_SIZE               (1<<8)      /* ����HASH���С */
#define BS_MAX_CUSTOMER_NAME_LEN            48          /* �ͻ�������󳤶� */
#define BS_MAX_PACKAGE_NAME_LEN             48          /* �ʷѰ�������󳤶� */
#define BS_MAX_BILLING_RULE_IN_PACKAGE      10          /* ����ҵ��Ʒѹ����������� */
#define BS_TRACE_BUFF_LEN                   1024        /* buff�ַ�����󳤶� */
#define BS_MAX_APP_CONN_NUM                 4           /* Ӧ�ò��������ӵ�������� */
#define BS_MAX_MSG_LEN                      1024        /* ��Ϣ��󳤶� */
#define BS_MAX_VOICE_SESSION_TIME           7200        /* ��������ͨ�����ʱ�� */
#define BS_MAX_MESSAGE_NUM                  10000       /* ������Ϣ������� */
#define BS_MASK_REGION_COUNTRY              0xFFFF0000  /* ���Ҵ���(32-17bit),ʡ���(16-11bit),����(10-1bit) */
#define BS_MASK_REGION_PROVINCE             0xFC00      /* ���Ҵ���(32-17bit),ʡ���(16-11bit),����(10-1bit) */
#define BS_MASK_REGION_CITY                 0x3FF       /* ���Ҵ���(32-17bit),ʡ���(16-11bit),����(10-1bit) */
#define BS_ACCOUNTING_INTERVAL              (2*60)      /* ����ʱ����,��λ:�� */
#define BS_AUDIT_INTERVAL                   (10*60)     /* ���ʱ����,��λ:�� */
#define BS_BACKUP_INTERVAL                  (60*60)     /* ����ʱ����,��λ:�� */
#define BS_SYS_OPERATOR_ID                  0           /* ������Ϊϵͳʱ�Ĳ�����ID */



/* ��������� */
enum BS_INTER_ERRCODE_E
{
    BS_INTER_ERR_FAIL               = -1,       /* ʧ�� */
    BS_INTER_ERR_SUCC               = 0,        /* �ɹ� */
    BS_INTER_ERR_NOT_EXIST          = 1,        /* ������ */
    BS_INTER_ERR_MEM_ALLOC_FAIL     = 2,        /* ����/ʧЧ */

};

enum BS_WEB_CMD_OPTERATOR
{
    BS_CMD_INSERT,
    BS_CMD_UPDATE,
    BS_CMD_DELETE,

    BS_CMD_BUTT
};

/* �������� */
enum BS_TRACE_TARGET_E
{
    BS_TRACE_FS                     = 1,            /* freeswitch��Ϣ */
    BS_TRACE_WEB                    = (1<<1),       /* WEB��Ϣ */
    BS_TRACE_DB                     = (1<<2),       /* DB */
    BS_TRACE_CDR                    = (1<<3),       /* CDR */
    BS_TRACE_BILLING                = (1<<4),       /* �Ʒ� */
    BS_TRACE_ACCOUNT                = (1<<5),       /* �˻� */
    BS_TRACE_STAT                   = (1<<6),       /* ͳ�� */
    BS_TRACE_AUDIT                  = (1<<7),       /* ��� */
    BS_TRACE_MAINTAIN               = (1<<8),       /* ά�� */
    BS_TRACE_RUN                    = (1<<9),       /* ���� */

    BS_TRACE_ALL                    = 0xFFFFFFFF
};

/* �ͻ����� */
enum BS_CUSTOMER_TYPE_E
{
    BS_CUSTOMER_TYPE_CONSUMER           = 0,        /* ������ */
    BS_CUSTOMER_TYPE_AGENT              = 1,        /* ������ */
    BS_CUSTOMER_TYPE_TOP                = 2,        /* �����ͻ�(��ϵͳ������) */
    BS_CUSTOMER_TYPE_SP                 = 3,        /* ҵ���ṩ�� */

    BS_CUSTOMER_TYPE_BUTT               = 255
};

/* �ͻ�״̬ */
enum BS_CUSTOMER_STATE_E
{
    BS_CUSTOMER_STATE_ACTIVE            = 0,        /* � */
    BS_CUSTOMER_STATE_FROZEN            = 1,        /* ���� */
    BS_CUSTOMER_STATE_INACTIVE          = 2,        /* ������ */

    BS_CUSTOMER_STATE_BUTT              = 255
};

/* �Ʒ����ڶ���,0Ϊ��Чֵ */
enum BS_BILLING_CYCLE_E
{
    BS_BILLING_CYCLE_DAY                = 1,        /* �� */
    BS_BILLING_CYCLE_WEEK               = 2,        /* �� */
    BS_BILLING_CYCLE_MONTH              = 3,        /* �� */
    BS_BILLING_CYCLE_YEAR               = 4,        /* �� */

    BS_BILLING_CYCLE_BUTT               = 255
};

/* ��Ӫ�̶��� */
enum BS_OPERATOR_E
{
    BS_OPERATOR_UNKNOWN                 = 0,        /* δ֪��Ӫ�� */
    BS_OPERATOR_CHINANET                = 1,        /* �й����� */
    BS_OPERATOR_CHINAUNICOM             = 2,        /* �й���ͨ */
    BS_OPERATOR_CMCC                    = 3,        /* �й��ƶ� */
    BS_OPERATOR_VNO                     = 4,        /* ������Ӫ�� */

    BS_OPERATOR_BUTT                    = 255
};

/* ��������,0Ϊ��Чֵ */
enum BS_NUMBER_TYPE_E
{
    BS_NUMBER_TYPE_MOBILE               = 1,        /* �����ƶ��绰���� */
    BS_NUMBER_TYPE_FIXED                = 2,        /* ���ڹ̶��绰���� */
    BS_NUMBER_TYPE_VOIP                 = 3,        /* VOIP����(�����ڲ��ֻ�����) */
    BS_NUMBER_TYPE_INTERNATIONAL        = 4,        /* ���ʺ��� */
    BS_NUMBER_TYPE_EMERGENCY            = 5,        /* �������� */
    BS_NUMBER_TYPE_SPECIAL              = 6,        /* �������(������Ѷ̨��) */

    BS_NUMBER_TYPE_BUTT                 = 255
};

/* �ۼƶ������� */
enum BS_OBJECT_E
{
    BS_OBJECT_SYSTEM                    = 0,        /* ϵͳ */
    BS_OBJECT_CUSTOMER                  = 1,        /* �ͻ� */
    BS_OBJECT_ACCOUNT                   = 2,        /* �˻� */
    BS_OBJECT_AGENT                     = 3,        /* ��ϯ */
    BS_OBJECT_NUMBER                    = 4,        /* ���� */
    BS_OBJECT_LINE                      = 5,        /* �û���(�豸�˿�) */
    BS_OBJECT_TASK                      = 6,        /* ���� */
    BS_OBJECT_TRUNK                     = 7,        /* �м� */

    BS_OBJECT_BUTT                      = 255
};

/*
�Ʒѷ�ʽ
�ݲ�֧�������Ʒ�
*/
enum BS_BILLING_TYPE_E
{
    BS_BILLING_BY_TIMELEN               = 0,        /* ��ʱ���Ʒ� */
    BS_BILLING_BY_COUNT                 = 1,        /* ���μƷ� */
    BS_BILLING_BY_TRAFFIC               = 2,        /* �������Ʒ� */
    BS_BILLING_BY_CYCLE                 = 3,        /* �����ڼƷ� */

    BS_BILLING_TYPE_BUTT                = 255
};

/* �˻��������� */
enum enBS_ACCOUNT_OPERATE_TYPE_E
{
    BS_ACCOUNT_RECHARGE             = 1,        /* ��ֵ */
    BS_ACCOUNT_DEDUCTION            = 2,        /* �ۿ� */
    BS_ACCOUNT_TRANSFER_IN          = 3,        /* ת�� */
    BS_ACCOUNT_TRANSFER_OUT         = 4,        /* ת�� */
    BS_ACCOUNT_REBATE_GET           = 5,        /* ��ȡ���� */
    BS_ACCOUNT_REBATE_PAY           = 6,        /* ֧������ */

    BS_ACCOUNT_OPERATE_TYPE_BUTT    = 255
};

enum enBS_ACCOUNT_OPERATE_DIR_E
{
    BS_ACCOUNT_PAY                  = 1,
    BS_ACCOUNT_GET,

    BS_ACCOUNT_OPERATE_DIR_BUTT
};

/* �ڲ���Ϣ���Ͷ���,0Ϊ��Чֵ */
enum BS_INTER_MSG_TYPE_E
{
    BS_INTER_MSG_WALK_REQ               = 1,        /* ��������� */
    BS_INTER_MSG_WALK_RSP               = 2,        /* �������Ӧ */
    BS_INTER_MSG_ORIGINAL_CDR           = 3,        /* ԭʼ���� */
    BS_INTER_MSG_VOICE_CDR              = 4,        /* �������� */
    BS_INTER_MSG_RECORDING_CDR          = 5,        /* ¼������ */
    BS_INTER_MSG_MESSAGE_CDR            = 6,        /* ��Ϣ���� */
    BS_INTER_MSG_SETTLE_CDR             = 7,        /* ���㻰�� */
    BS_INTER_MSG_RENT_CDR               = 8,        /* ��𻰵� */
    BS_INTER_MSG_ACCOUNT_CDR            = 9,        /* ���񻰵� */
    BS_INTER_MSG_OUTBAND_STAT           = 10,       /* ����ͳ�� */
    BS_INTER_MSG_INBAND_STAT            = 11,       /* ����ͳ�� */
    BS_INTER_MSG_OUTDIALING_STAT        = 12,       /* ���ͳ�� */
    BS_INTER_MSG_MESSAGE_STAT           = 13,       /* ��Ϣͳ�� */
    BS_INTER_MSG_ACCOUNT_STAT           = 14,       /* ����ͳ�� */

    BS_INTER_MSG_BUTT                   = 255
};

/*
�Ʒ�����,0Ϊ��Чֵ;�������,������31��;
�÷�˵��:
1.����������,32bit����:3���ֶ�,���Ҵ���(32-17bit),ʡ���(16-11bit),����(10-1bit)
2.����������,ƥ�����:
  a.0-ͨ������;
  b.�ֶ�ֵ:0-ͨ���ֶδ�������е���;
  c.��ȫƥ��:�����ֶξ���ͬ;
3.��Ӫ������,ƥ�����:
  a.0-ͨ������;
  b.Ҫôͨ��,Ҫôƥ�䵥����Ӫ��;
4.������������,ƥ�����:
  a.0-ͨ������;
  b.Ҫôͨ��,Ҫôƥ�䵥����������;
5.ʱ������,ƥ�����:
  a.0-���޶�ʱ��;
  b.��������ֵ,��ѯ���ݿ�,�ж�ʱ���Ƿ������ݿ��ʱ�䶨����;
6.��Դ������,ֻ���������ҵ����ʹ��,��ϵͳ���м��㴦��,����ƥ��.
  a.Դ����1��д��Դ������;
  b.Դ����ֵ1Ϊ��Դ����,Դ����ֵ2Ϊ�Ʒ�����,����Ϊ��Դ����;
  c.Դ����ֵ1Ϊ0��ʾ��ϵͳͳ������;
  d.Ŀ�����Լ�����ֵ,�Ժ���ݸ�����Դ���ж���;
*/
enum BS_BILLING_ATTRIBUTE_E
{
    BS_BILLING_ATTR_REGION              = 1,        /* ������ */
    BS_BILLING_ATTR_TEL_OPERATOR        = 2,        /* ��Ӫ�� */
    BS_BILLING_ATTR_NUMBER_TYPE         = 3,        /* �������� */
    BS_BILLING_ATTR_TIME                = 4,        /* ʱ������ */
    BS_BILLING_ATTR_ACCUMULATE_TIMELEN  = 5,        /* �ۼ�ʱ�� */
    BS_BILLING_ATTR_ACCUMULATE_COUNT    = 6,        /* �ۼƴ��� */
    BS_BILLING_ATTR_ACCUMULATE_TRAFFIC  = 7,        /* �ۼ����� */
    BS_BILLING_ATTR_CONCURRENT          = 8,        /* ������ */
    BS_BILLING_ATTR_SET                 = 9,        /* �ײ� */
    BS_BILLING_ATTR_RESOURCE_AGENT      = 10,       /* ��ϯ��Դ */
    BS_BILLING_ATTR_RESOURCE_NUMBER     = 11,       /* ������Դ */
    BS_BILLING_ATTR_RESOURCE_LINE       = 12,       /* �û���(�豸�˿�)��Դ */

    BS_BILLING_ATTR_LAST                = 13,       /* ������,���һ����Ч���� */

    BS_BILLING_ATTR_BUTT                = 31
};

/*
�ۼƹ���
�ر��,ҵ�����ͼ�ͨ��1����ҵ������ֵ��ȡ�����,��ζ��Ŀǰֻ֧��31�����ڵ�ҵ������
*/
typedef struct stBS_ACCUMULATE_RULE
{
    U32     ulRuleID;                               /* ����ID */
    U8      ucObject;                               /* �ۼƶ���,�ο�:BS_OBJECT_E */
    U8      ucCycle;                                /* �ۼ�����,�ο�:BS_BILLING_CYCLE_E */
    U32     ulServTypeSet;                          /* ҵ�����ͼ� */
    U32     ulMaxValue;                             /* ���ֵ */
}BS_ACCUMULATE_RULE;

/*
�Ʒѹ���ƥ��ṹ��
ע��:
1.��������������,�ͻ�ID�������߿ͻ�ID��ͬ;
2.���ڴ�����,ƥ���ۼ�������ʱ,��Ȼʹ�����������ߵĿͻ�ID����ƥ��;
*/
typedef struct stBS_BILLING_MATCH_ST
{
    BS_ACCUMULATE_RULE  stAccuRule;

    U32     ulCustomerID;                           /* �ͻ�ID */
    U32     ulComsumerID;                           /* �����߿ͻ�ID */
    U32     ulAgentID;
    U32     ulUserID;
    U32     ulTimeStamp;
    S8      szCaller[BS_MAX_CALL_NUM_LEN];
    S8      szCallee[BS_MAX_CALL_NUM_LEN];

}BS_BILLING_MATCH_ST;


/*
�Ʒѹ���ṹ��
�÷�˵��:
1.һ���Ʒ�package�������ҵ��ļƷѹ���,һ��ҵ�����������ͬ���ȼ��ļƷѹ���;
2.�ƷѴ���ʱ,�������ȼ��Ӹߵ���ƥ��,ƥ��ɹ���Ʒѽ���;
3.�����Żݷ�������,��ͨ�����ø����ȼ���ʵ��;
4.��������Ϊ255ʱ,��ʾ���޶�����;
5.�ײ�����ͨ���趨����һ�����ڼƷѹ���ʵ�ּ���;
6.�ײ����������ʱ,���趨�ۼ����Բ��趨����Ϊ0;
7.�������������,��ͨ���������ڼƷ�������ʵ��(�ȼ����ײ������,�ײ����շ�);
8.���趨Ϊ�ۼ�����,������ֵΪ0ʱ,��ζ����������;
9.����Ϊ������ʱ,32bit����ֵ�ֶζ���:���Ҵ���(32-17bit),ʡ���(16-11bit),����(10-1bit)
10.����������ֵ����ͨ��,��������Ϊ0ʱ��ζ��ͨ����Ϲ��Ҵ��뼰ʡ���������е�����;
11.����Ϊʱ��ʱ,����ֵ��λΪ��;����Ϊ����ʱ,����ֵ��λΪKB;
12.����������,����ֵΪ0��ʾ������;����,��Ӧ�ĺ���,��Ҫ�����ݿ��в���ƥ��;
13.ʱ��������,����ֵ��ʾһ���趨��ʱ�䷶Χ,��Ҫ�����ݿ���ƥ�����(��ʱ���Żݿ���ʹ��);
14.���ڼƷѹ���,ͨ�����ڵļƷѴ������,�����ɻ����ƷѴ���;
15.��Դ��۷ѹ���(������ϯ����),ҵ�������������ҵ��,Դ��������Դ���,
   ����ֵΪ��Դ����,��0��ʾ��ϵͳͳ������,����Ϊ��Դ����;�����ֶο��Բ���;
16.���μƷѹ���,���мƷѵ�λ��Ϊ1,�Ʒѵ�λ�ڼƷѴ���Ҳ��Ϊ1;
17.���η���ΪU32_BUTT,��ʾ��Ҫ���ݱ��к��뵥����ѯ����,��Ҫ���ڹ��ʳ�;����;��δʵ��;
���ʵ��:
1.�ڴ�����PackageID+ҵ������Ϊ����������ϣ��,�����еļƷѹ�������ڴ���;
2.ͬһPackageID+ҵ�����͵����мƷѹ���,��ŵ��ṹ��������;
3.���ݿ����ݵ�ά�����²���,��Ҫʵʱͬ�����ڴ���;
*/
typedef struct stBS_BILLING_RULE_ST
{
    U32     ulPackageID;                    /* �����Ʒ�packageID */
    U32     ulRuleID;                       /* ����ID,���ݿ���ȫ��ͳһ��� */
    U8      ucSrcAttrType1;                 /* Դ�������� */
    U8      ucSrcAttrType2;                 /* Դ�������� */
    U8      ucDstAttrType1;                 /* Ŀ���������� */
    U8      ucDstAttrType2;                 /* Ŀ���������� */
    U32     ulSrcAttrValue1;                /* Դ������ֵ */
    U32     ulSrcAttrValue2;                /* Դ������ֵ */
    U32     ulDstAttrValue1;                /* Ŀ��������ֵ */
    U32     ulDstAttrValue2;                /* Ŀ��������ֵ */
    U32     ulFirstBillingUnit;             /* �״μƷѵ�λ,��λ:���� */
    U32     ulNextBillingUnit;              /* �����Ʒѵ�λ,��λ:���� */
    U8      ucFirstBillingCnt;              /* �״μƷѵ�λ�ڼƷѴ��� */
    U8      ucNextBillingCnt;               /* �����Ʒѵ�λ�ڼƷѴ��� */
    U8      ucServType;                     /* ҵ������,�ο�:BS_SERV_TYPE_E */
    U8      ucBillingType;                  /* �Ʒѷ�ʽ,�ο�:BS_BILLING_TYPE_E */
    U32     ulBillingRate;                  /* ���η���,��λ:1/100��,�� */
    U32     ulEffectTimestamp;              /* ��Чʱ���,0Ϊ������Ч */
    U32     ulExpireTimestamp;              /* ʧЧʱ���,0Ϊ������Ч */
    U8      ucPriority;                     /* ���ȼ�,0-9,ԽС���ȼ�Խ�� */

    U8      ucValid;
    U8      aucReserv[2];

}BS_BILLING_RULE_ST;


/*
�ʷѰ��ṹ��
���ʵ��:
1.���е��ʷ���Ϣ��ȡ���ڴ���;
2.���е��ʷ���Ϣ,�����ڹ�ϣ��Ľڵ���;
3.��ͬ��ҵ������,�Ʒѹ����ŵ���ϣͰ��;
4.���ݿ����ݵ�ά�����²���,��Ҫʵʱͬ�����ڴ���;
*/
typedef struct stBS_BILLING_PACKAGE_ST
{
    U32     ulPackageID;                    /* �ʷѰ�ID */
    U8      ucServType;                     /* ҵ������,�ο�:BS_SERV_TYPE_E */
    U8      aucReserv[3];

    BS_BILLING_RULE_ST  astRule[BS_MAX_BILLING_RULE_IN_PACKAGE];        /* �Ʒѹ��� */

}BS_BILLING_PACKAGE_ST;

/* �˻���Ϣ�ṹ�� */
typedef struct stBS_ACCOUNT_ST
{
    pthread_mutex_t     mutexAccount;       /* �˻��� */

    BS_STAT_ACCOUNT_ST  stStat;             /* ͳ����Ϣ */

    U32     ulAccountID;                    /* �˻�ID */
    S8      szPwd[BS_MAX_PASSWORD_LEN];     /* ���� */
    U32     ulCustomerID;                   /* �����ͻ�ID */
    S32     lCreditLine;                    /* ���ö��;��λ:1/100�� */
    U32     ulBillingPackageID;             /* �����Ʒ�packageID */
    S64     LBalance;                       /* �˻����,��λ:1/100��,��;ע���˻����Խ������ */
    S32     lBalanceWarning;                /* �澯��ֵ;��λ:1/100�� */
    S32     lRebate;                        /* δ���˷���,��λ:1/100��,�� */
    S64     LBalanceActive;                 /* ��̬�˻����,����ʵʱ�۷���δ���˵���ʱ״̬;��λ:1/100�� */
    U32     ulAccountingTime;               /* ����ʱ�� */
    U32     ulExpiryTime;                   /* ʧЧʱ�� */

}BS_ACCOUNT_ST;


/*
�ͻ���Ϣ�ṹ��
���ʵ��:
1.���еĿͻ���Ϣ��ȡ���ڴ���(��Ӧ��,�˻���ϢҲ��ŵ��ڴ���);
2.���еĿͻ���Ϣ,�����ڹ�ϣ��Ľڵ���;
3.�����ͻ���,�ӿͻ�ͬ����Ľڵ�ʹ����������;
4.������ڵ��м�¼�ÿͻ��ڹ�ϣ���е�λ��;
5.���ݿ����ݵ�ά�����²���,��Ҫʵʱͬ�����ڴ���;
*/
typedef struct stBS_CUSTOMER_ST
{
    DLL_NODE_S          stNode;             /* ����ڵ�,ͬһ���ͻ���ͬ���ͻ���ͬһ������
                                               pHandle��¼hash��ڵ��ַ;Ҫ����ֶ�Ϊ�ṹ���һ�� */
    DLL_S               stChildrenList;     /* �ӽڵ����� */
    struct stBS_CUSTOMER_ST *pstParent;     /* ���ڵ� */

    BS_ACCOUNT_ST       stAccount;          /* �����˻�ID,Ŀǰֻ����1���ͻ���Ӧ1���˻������ */
    BS_STAT_SERV_ST     stStat;             /* ͳ����Ϣ */

    U32     ulCustomerID;                   /* �ͻ�ID */
    S8      szCustomerName[BS_MAX_CUSTOMER_NAME_LEN];   /* �ͻ����� */
    U32     ulParentID;                     /* ���ͻ�ID */

    U32     ulAgentNum;                     /* ��ϯ���� */
    U32     ulNumberNum;                    /* ����(����)���� */
    U32     ulUserLineNum;                  /* �û������� */

    U8      ucCustomerType;                 /* �ͻ����� */
    U8      ucCustomerState;                /* �ͻ�״̬ */

    U8      aucReserv[2];

}BS_CUSTOMER_ST;
/*
��ϯ��Ϣ�ṹ��
���ʵ��:
1.���е���ϯ��Ϣ��ȡ���ڴ���
2.�ڴ��е���ϯ������Ϣʹ��ʹ��hash�����洢;
3.���ݿ����ݵ�ά�����²���,��Ҫʵʱͬ�����ڴ���;
*/
typedef struct stBS_AGENT_ST
{
    BS_STAT_SERV_ST     stStat;             /* ͳ����Ϣ */

    U32     ulAgentID;                      /* ��ϯID,Ҫ��ȫ����,������10λ,���λС��4 */
    U32     ulCustomerID;                   /* �ͻ�ID */
    U32     ulGroup1;                       /* ��������1 */
    U32     ulGroup2;                       /* ��������2 */

}BS_AGENT_ST;

/*
������Ϣ�ṹ��
���ʵ��:
1.���еĽ�����Ϣ��ȡ���ڴ���
2.�ڴ��еĽ���������Ϣʹ��ʹ��hash�����洢;
3.���ݿ����ݵ�ά�����²���,��Ҫʵʱͬ�����ڴ���;
*/
typedef struct stBS_SETTLE_ST
{
    BS_STAT_SERV_ST     stStat;             /* ͳ����Ϣ */

    U32     ulSPID;                         /* �����ṩ��ID,Ҫ��ȫ����,������10λ,���λС��4 */
    U32     ulPackageID;                    /* �����Ʒ�packageID */
    U16     usTrunkID;                      /* �м�ID */
    U8      ucSettleFlag;                   /* �����־,1:����,0:������ */

    U8      ucReserv;

}BS_SETTLE_ST;

/*
 * WEBAPI���ݽṹ��
 **/
typedef struct tagBSWEBCMDInfo
{
    U32         ulTblIndex;                     /* �����µı� */
    U32         ulTimestamp;                    /* ��ǰ���ݴ�����ʱ�� */
    JSON_OBJ_ST *pstData;                   /* JSON���� */
}BS_WEB_CMD_INFO_ST;

/*
������Ϣ�ṹ��
���ʵ��:
1.��Ҫ���ڼ�¼�����ͳ����Ϣ;
2.����ʶ�𵽵��������ɹ�ϣ��ڵ�洢;
3.ֻ��¼���������ͳ����Ϣ,����ȫ��������0;
*/
typedef struct stBS_TASK_ST
{
    BS_STAT_SERV_ST     stStat;             /* ͳ����Ϣ */

    U32     ulTaskID;                       /* ����ID,Ҫ��ȫ����,������10λ,���λС��4 */
    U8      aucReserv[3];

}BS_TASK_ST;

/* ���ݲ���ҵ���֮�����Ϣ�ṹ�� */
typedef struct stBS_INTER_MSG_TAG
{
    pthread_t   srcTid;
    pthread_t   dstTid;
    U8      ucMsgType;                      /* ��Ϣ���� */
    U8      ucReserv;
    U16     usMsgLen;                       /* ��Ϣ�ܳ��� */
    S32     lInterErr;                      /* �ڲ������� */
}BS_INTER_MSG_TAG;

typedef S32 (*BS_FN)(U32, VOID *);
/* ���������Ϣ�ṹ�� */
typedef struct stBS_INTER_MSG_WALK
{
    BS_INTER_MSG_TAG    stMsgTag;

    U32     ulTblType;                      /* ������ */
    BS_FN   FnCallback;                     /* ���ݿ�����Ļص����� */
    VOID    *param;                         /* �ص��������� */
}BS_INTER_MSG_WALK;

/* ������Ϣ�ṹ�� */
typedef struct stBS_INTER_MSG_CDR
{
    BS_INTER_MSG_TAG    stMsgTag;

    VOID    *pCDR;                          /* cdr�ṹ��ָ�� */
}BS_INTER_MSG_CDR;

/* ͳ����Ϣ�ṹ�� */
typedef struct stBS_INTER_MSG_STAT
{
    BS_INTER_MSG_TAG    stMsgTag;

    VOID    *pStat;                         /*  ͳ����Ϣ�ṹ��ָ�� */
}BS_INTER_MSG_STAT;




typedef struct stBSS_APP_CONN
{
    U32                 bIsValid:1;
    U32                 bIsConn:1;
    U32                 ucCommType:8;
    U32                 ulReserv:22;

    U32                 ulMsgSeq;

    U32                 aulIPAddr[4];       /* APP����IP��ַ,������ */
    S32                 lSockfd;            /* APP��Ӧ�ڱ�����socket */
    S32                 lAddrLen;
    union tagSocketAddr{
        struct sockaddr_in  stInAddr;             /* APP��Ӧ�ڱ��������ӵ�ַ */
        struct sockaddr_un  stUnAddr;             /* APP��Ӧ�ڱ��������ӵ�ַ */
    }stAddr;
}BSS_APP_CONN;

/* ҵ�����ƿ� */
typedef struct stBSS_CB
{
    U32         bIsMaintain:1;              /* �Ƿ�ϵͳά���� */
    U32         bIsBillDay:1;               /* �Ƿ��˵��� */
    U32         bIsDayCycle:1;              /* ��ѭ����ʱ */
    U32         bIsHourCycle:1;             /* ÿСʱѭ����ʱ */
    U32         ulHour:5;                   /* ��ǰСʱ�� */
    U32         ulTraceLevel:3;             /* ���ټ��� */
    U32         ulReserv:20;
    U32         ulTraceFlag;                /* ���ٱ�־ */

    U32         ulCDRMark;                  /* CDR���,����ԭʼ�������ʱ���ͬһ��Դ */
    U32         ulMaxVocTime;               /* ϵͳ�������������ʱ��,��λ:�� */
    U32         ulMaxMsNum;                 /* ϵͳ��������Ϣ������� */
    U32         ulStatDayBase;              /* ����ͳ�ƻ���ʱ��� */
    U32         ulStatHourBase;             /* ��ʱͳ�ƻ���ʱ��� */

    U32         ulCommProto;                /* ʹ������Э��ͨѶ */
    U16         usUDPListenPort;            /* �����˿�,������ */
    U16         usTCPListenPort;            /* �����˿�,������ */

    BSS_APP_CONN    astAPPConn[BS_MAX_APP_CONN_NUM];
    BS_CUSTOMER_ST  *pstTopCustomer;        /* �����ͻ���Ϣ */

    DOS_TMR_ST  hDayCycleTmr;               /* ѭ����ʱ�� */
    DOS_TMR_ST  hHourCycleTmr;              /* ѭ����ʱ�� */
}BSS_CB;

extern pthread_mutex_t g_mutexTableUpdate;
extern BOOL                 g_bTableUpdate;

extern pthread_mutex_t  g_mutexCustomerTbl;
extern HASH_TABLE_S     *g_astCustomerTbl;
extern pthread_mutex_t  g_mutexBillingPackageTbl;
extern HASH_TABLE_S     *g_astBillingPackageTbl;
extern pthread_mutex_t  g_mutexSettleTbl;
extern HASH_TABLE_S     *g_astSettleTbl;
extern pthread_mutex_t  g_mutexAgentTbl;
extern HASH_TABLE_S     *g_astAgentTbl;
extern pthread_mutex_t  g_mutexTaskTbl;
extern HASH_TABLE_S     *g_astTaskTbl;

extern pthread_cond_t   g_condBSS2DList;
extern pthread_mutex_t  g_mutexBSS2DMsg;
extern DLL_S            g_stBSS2DMsgList;
extern pthread_cond_t   g_condBSD2SList;
extern pthread_mutex_t  g_mutexBSD2SMsg;
extern DLL_S            g_stBSD2SMsgList;
extern pthread_cond_t   g_condBSAppSendList;
extern pthread_mutex_t  g_mutexBSAppMsgSend;
extern DLL_S            g_stBSAppMsgSendList;
extern pthread_cond_t   g_condBSAAAList;
extern pthread_mutex_t  g_mutexBSAAAMsg;
extern DLL_S            g_stBSAAAMsgList;
extern pthread_cond_t   g_condBSCDRList;
extern pthread_mutex_t  g_mutexBSCDR;
extern DLL_S            g_stBSCDRList;
extern pthread_cond_t   g_condBSBillingList;
extern pthread_mutex_t  g_mutexBSBilling;
extern DLL_S            g_stBSBillingList;
extern pthread_mutex_t  g_mutexWebCMDTbl;
extern DLL_S            g_stWebCMDTbl;
extern U32              g_ulLastCMDTimestamp;


extern BSS_CB g_stBssCB;

/* bs_ini.c */
VOID bs_init_customer_st(BS_CUSTOMER_ST *pstCustomer);
VOID bs_init_agent_st(BS_AGENT_ST *pstAgent);
VOID bs_init_billing_package_st(BS_BILLING_PACKAGE_ST *pstBillingPackage);
VOID bs_init_settle_st(BS_SETTLE_ST *pstSettle);
VOID bs_build_customer_tree(VOID);


/* bs_lib.c */
VOID bs_free_node(VOID *pNode);
S32 bs_customer_hash_node_match(VOID *pKey, HASH_NODE_S *pNode);
S32 bs_agent_hash_node_match(VOID *pKey, HASH_NODE_S *pNode);
S32 bs_billing_package_hash_node_match(VOID *pKey, HASH_NODE_S *pNode);
S32 bs_settle_hash_node_match(VOID *pKey, HASH_NODE_S *pNode);
S32 bs_task_hash_node_match(VOID *pKey, HASH_NODE_S *pNode);
U32 bs_hash_get_index(U32 ulHashTblSize, U32 ulID);
HASH_NODE_S *bs_get_customer_node(U32 ulCustomerID);
BS_CUSTOMER_ST *bs_get_customer_st(U32 ulCustomerID);
BS_AGENT_ST *bs_get_agent_st(U32 ulAgentID);
BS_SETTLE_ST *bs_get_settle_st(U16 usTrunkID);
BS_TASK_ST *bs_get_task_st(U32 ulTaskID);
VOID bs_customer_add_child(BS_CUSTOMER_ST *pstCustomer, BS_CUSTOMER_ST *pstChildCustomer);
BSS_APP_CONN *bs_get_app_conn(BS_MSG_TAG *pstMsgTag);
BSS_APP_CONN *bs_save_app_conn(S32 lSockfd, U8 *pstAddrIn, U32 ulAddrinHeader, S32 lAddrLen);
VOID bs_stat_agent_num(VOID);
U32 bs_get_settle_packageid(U16 usTrunkID);
BS_BILLING_PACKAGE_ST *bs_get_billing_package(U32 ulPackageID, U8 ucServType);
BOOL bs_billing_rule_is_properly(BS_BILLING_RULE_ST  *pstRule);
U32 bs_pre_billing(BS_CUSTOMER_ST *pstCustomer, BS_MSG_AUTH *pstMsg, BS_BILLING_PACKAGE_ST *pstPackage);
BOOL bs_cause_is_busy(U16 usCause);
BOOL bs_cause_is_not_exist(U16 usCause);
BOOL bs_cause_is_no_answer(U16 usCause);
BOOL bs_cause_is_reject(U16 usCause);
VOID bs_account_stat_refresh(BS_ACCOUNT_ST *pstAccount, S32 lFee, S32 lPorfit, U8 ucServType);
VOID bs_stat_voice(BS_CDR_VOICE_ST *pstCDR);
VOID bs_stat_message(BS_CDR_MS_ST *pstCDR);
BS_BILLING_RULE_ST *bs_match_billing_rule(BS_BILLING_PACKAGE_ST *pstPackage,
                                              BS_BILLING_MATCH_ST *pstBillingMatch);

/* bs_msg.c */
VOID bsd_send_walk_rsp2sl(DLL_NODE_S *pMsgNode, S32 lReqRet);
VOID bsd_inherit_rule_req2sl(DLL_NODE_S *pMsgNode);
VOID bss_send_walk_req2dl(U32 ulTblType, BS_FN callback, VOID *param);
VOID bss_send_cdr2dl(DLL_NODE_S *pMsgNode, U8 ucMsgType);
VOID bss_send_stat2dl(DLL_NODE_S *pMsgNode, U8 ucMsgType);
VOID bss_send_rsp_msg2app(BS_MSG_TAG *pstMsgTag, U8 ucMsgType);
VOID bss_send_aaa_rsp2app(DLL_NODE_S *pMsgNode);

/* bs_debug.c */
VOID bs_trace(U32 ulTraceTarget, U8 ucTraceLevel, const S8 * szFormat, ...);
S32  bs_command_proc(U32 ulIndex, S32 argc, S8 **argv);
S32  bs_update_test(U32 ulIndex, S32 argc, S8 **argv);



/* bsd_mngt.c */
VOID *bsd_recv_bss_msg(VOID * arg);
VOID *bsd_backup(VOID *arg);

/* bss_mngt.c */
VOID *bss_recv_bsd_msg(VOID * arg);
VOID *bss_send_msg2app(VOID *arg);
VOID *bss_recv_msg_from_app(VOID *arg);
VOID *bss_recv_msg_from_web(VOID *arg);
VOID *bss_web_msg_proc(VOID *arg);
VOID *bss_aaa(VOID *arg);
VOID *bss_cdr(VOID *arg);
VOID *bss_billing(VOID *arg);
VOID *bss_stat(VOID *arg);
VOID *bss_accounting(VOID *arg);
VOID *bss_audit(VOID *arg);
VOID bss_day_cycle_proc(U64 uLParam);
VOID bss_hour_cycle_proc(U64 uLParam);

/* bsd_db.c */
S32 bs_init_db();
S32 bsd_walk_customer_tbl(BS_INTER_MSG_WALK *pstMsg);
S32 bsd_walk_agent_tbl(BS_INTER_MSG_WALK *pstMsg);
S32 bsd_walk_billing_package_tbl(BS_INTER_MSG_WALK *pstMsg);
S32 bsd_walk_billing_package_tbl_bak(U32 ulPkgID);
S32 bsd_walk_settle_tbl(BS_INTER_MSG_WALK *pstMsg);
S32 bsd_walk_web_cmd_tbl(BS_INTER_MSG_WALK *pstMsg);
S32 bsd_delete_web_cmd_tbl(BS_INTER_MSG_WALK *pstMsg);
VOID bsd_save_original_cdr(BS_INTER_MSG_CDR *pstMsg);
VOID bsd_save_voice_cdr(BS_INTER_MSG_CDR *pstMsg);
VOID bsd_save_recording_cdr(BS_INTER_MSG_CDR *pstMsg);
VOID bsd_save_message_cdr(BS_INTER_MSG_CDR *pstMsg);
VOID bsd_save_settle_cdr(BS_INTER_MSG_CDR *pstMsg);
VOID bsd_save_rent_cdr(BS_INTER_MSG_CDR *pstMsg);
VOID bsd_save_account_cdr(BS_INTER_MSG_CDR *pstMsg);
VOID bsd_save_outband_stat(BS_INTER_MSG_STAT *pstMsg);
VOID bsd_save_inband_stat(BS_INTER_MSG_STAT *pstMsg);
VOID bsd_save_outdialing_stat(BS_INTER_MSG_STAT *pstMsg);
VOID bsd_save_message_stat(BS_INTER_MSG_STAT *pstMsg);
VOID bsd_save_account_stat(BS_INTER_MSG_STAT *pstMsg);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif



