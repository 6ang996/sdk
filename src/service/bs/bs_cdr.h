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

#ifndef __BS_CDR_H__
#define __BS_CDR_H__

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* �������� */
enum BS_CDR_TYPE_E
{
    BS_CDR_ORIGINAL                 = 0,        /* ԭʼ���� */
    BS_CDR_VOICE                    = 1,        /* �������� */
    BS_CDR_RECORDING                = 2,        /* ¼������ */
    BS_CDR_MESSAGE                  = 3,        /* ��Ϣ���� */
    BS_CDR_SETTLE                   = 4,        /* ���㻰�� */
    BS_CDR_RENT                     = 5,        /* ��𻰵� */
    BS_CDR_ACCOUNT                  = 6,        /* ���񻰵� */


    BS_CDR_TYPE_BUTT                = 255
};


/* ����TAG */
typedef struct
{
    U32             ulCDRMark;                  /* �������,ͬһԭʼ������ֵĻ������������ͬ */
    U32             ulAccountMark;              /* ����ͳһ�ͻ���˵��ͬһ�γ��˹�����CDR��Account Mark����ͬ�� */
    U8              ucCDRType;                  /* �������� */
    U8              aucReserv[3];
}BS_CDR_TAG;

/*
��������
ʹ��˵��:
1.����/����/�ڲ�����ֱ��ʹ��;
2.����ת��/ǰת��,��ת��Ŀ�ķ����ɻ���,ע����д����ҵ������;
3.������,ÿ��LEG���ɻ���;
4.��ҵ��Ĳ�ͬ,����ͬһ������,�����ɶ��Ż�������;
5.���������ҵ��:
  ���к�����д�������;
  �����ҵ��,��������2�Ż���,һ�Ŷ�Ӧ�������(ҵ������Ϊ���),һ�Ŷ�Ӧ��ϯ��PSTN����(���ֺ���);
*/
typedef struct
{
    BS_CDR_TAG      stCDRTag;

    U32             ulUserID;                   /* �û�ID,Ҫ��ȫ����,������10λ,���λС��4 */
    U32             ulAgentID;                  /* ��ϯID,Ҫ��ȫ����,������10λ,���λС��4 */
    U32             ulCustomerID;               /* �ͻ�ID,Ҫ��ȫ����,������10λ,���λС��4 */
    U32             ulAccountID;                /* �˻�ID,Ҫ��ȫ����,������10λ,���λС��4 */
    U32             ulTaskID;                   /* ����ID,Ҫ��ȫ����,������10λ,���λС��4 */
    U32             ulRuleID;                   /* ʹ�õļƷѹ��� */

    U32             aulFee[BS_MAX_AGENT_LEVEL];                     /* ÿ���㼶�ķ���,��λ:1/100��,�� */
    S8              szCaller[BS_MAX_CALL_NUM_LEN];                  /* ���к��� */
    S8              szCallee[BS_MAX_CALL_NUM_LEN];                  /* ���к��� */
    S8              szCID[BS_MAX_CALL_NUM_LEN];                     /* ���Ժ��� */
    S8              szAgentNum[BS_MAX_AGENT_NUM_LEN];               /* ��ϯ����(����) */
    S8              szRecordFile[BS_MAX_RECORD_FILE_NAME_LEN];      /* Ϊ�մ���δ¼�� */
    U32             ulPDDLen;                   /* ����ʱ��:�ӷ�����е��յ����� */
    U32             ulRingTime;                 /* ����ʱ��,��λ:�� */
    U32             ulAnswerTimeStamp;          /* Ӧ��ʱ��� */
    U32             ulDTMFTime;                 /* ��һ�����β���ʱ��,��λ:�� */
    U32             ulIVRFinishTime;            /* IVR�������ʱ��,��λ:�� */
    U32             ulWaitAgentTime;            /* �ȴ���ϯ����ʱ��,��λ:�� */
    U32             ulTimeLen;                  /* ����ʱ��,��λ:�� */
    U32             ulHoldCnt;                  /* ���ִ��� */
    U32             ulHoldTimeLen;              /* ������ʱ��,��λ:�� */
    U32             aulPeerIP[4];               /* �Զ�IP��ַ,��дΪ���Ͳ�IP;����IPv6 */
    U16             usPeerTrunkID;              /* �Զ��м�ID */
    U16             usTerminateCause;           /* ��ֹԭ�� */

    U8              ucServType;                 /* ҵ������ */
    U8              ucAgentLevel;               /* �����㼶,[0,BS_MAX_AGENT_LEVEL) */
    U8              ucRecordFlag;               /* �Ƿ�¼����� */
    U8              ucReleasePart;              /* �Ự�ͷŷ� */

    U8              ucPayloadType;              /* ý������ */
    U8              ucPacketLossRate;           /* �հ�������,0-100 */
    U8              ucNeedCharge;               /* �Ƿ���Ҫ�Ʒ� */
    U8              ucRes;                      /* ���� */
}BS_CDR_VOICE_ST;

typedef struct
{
    BS_CDR_TAG      stCDRTag;

    U32             ulUserID;                   /* �û�ID,Ҫ��ȫ����,������10λ,���λС��4 */
    U32             ulAgentID;                  /* ��ϯID,Ҫ��ȫ����,������10λ,���λС��4 */
    U32             ulCustomerID;               /* �ͻ�ID,Ҫ��ȫ����,������10λ,���λС��4 */
    U32             ulAccountID;                /* �˻�ID,Ҫ��ȫ����,������10λ,���λС��4 */
    U32             ulTaskID;                   /* ����ID,Ҫ��ȫ����,������10λ,���λС��4 */

    S8              szCaller[BS_MAX_CALL_NUM_LEN];                  /* ���к��� */
    S8              szCallee[BS_MAX_CALL_NUM_LEN];                  /* ���к��� */
    S8              szCID[BS_MAX_CALL_NUM_LEN];                     /* ���Ժ��� */
    S8              szAgentNum[BS_MAX_AGENT_NUM_LEN];               /* ��ϯ����(����) */
    S8              szRecordFile[BS_MAX_RECORD_FILE_NAME_LEN];      /* Ϊ�մ���δ¼�� */
    U32             ulPDDLen;                   /* ����ʱ��:�ӷ�����е��յ����� */
    U32             ulRingTime;                 /* ����ʱ��,��λ:�� */
    U32             ulAnswerTimeStamp;          /* Ӧ��ʱ��� */
    U32             ulDTMFTime;                 /* ��һ�����β���ʱ��,��λ:�� */
    U32             ulIVRFinishTime;            /* IVR�������ʱ��,��λ:�� */
    U32             ulWaitAgentTime;            /* �ȴ���ϯ����ʱ��,��λ:�� */
    U32             ulTimeLen;                  /* ����ʱ��,��λ:�� */
    U32             ulHoldCnt;                  /* ���ִ��� */
    U32             ulHoldTimeLen;              /* ������ʱ��,��λ:�� */
    U16             usTerminateCause;           /* ��ֹԭ�� */
    U8              ucServType;                 /* ҵ������ */
    U8              ucReleasePart;              /* �Ự�ͷŷ� */

    U32             ulResult;                   /* ���н�� */
}BS_CDR_CALLTASK_RESULT_ST;


/* ¼������ */
typedef struct
{
    BS_CDR_TAG      stCDRTag;

    U32             ulUserID;                   /* �û�ID,Ҫ��ȫ����,������10λ,���λС��4 */
    U32             ulAgentID;                  /* ��ϯID,Ҫ��ȫ����,������10λ,���λС��4 */
    U32             ulCustomerID;               /* �ͻ�ID,Ҫ��ȫ����,������10λ,���λС��4 */
    U32             ulAccountID;                /* �˻�ID,Ҫ��ȫ����,������10λ,���λС��4 */
    U32             ulTaskID;                   /* ����ID,Ҫ��ȫ����,������10λ,���λС��4 */
    U32             ulRuleID;                   /* ʹ�õļƷѹ��� */

    U32             aulFee[BS_MAX_AGENT_LEVEL];                     /* ÿ���㼶�ķ���,��λ:1/100��,�� */
    S8              szCaller[BS_MAX_CALL_NUM_LEN];                  /* ���к��� */
    S8              szCallee[BS_MAX_CALL_NUM_LEN];                  /* ���к��� */
    S8              szCID[BS_MAX_CALL_NUM_LEN];                     /* ���Ժ��� */
    S8              szAgentNum[BS_MAX_AGENT_NUM_LEN];               /* ��ϯ����(����) */
    S8              szRecordFile[BS_MAX_RECORD_FILE_NAME_LEN];      /* Ϊ�մ���δ¼�� */
    U32             ulRecordTimeStamp;          /* ¼����ʼʱ��� */
    U32             ulTimeLen;                  /* ¼��ʱ��,��λ:�� */
    U8              ucAgentLevel;               /* �����㼶,[0,BS_MAX_AGENT_LEVEL) */

}BS_CDR_RECORDING_ST;

/* ����/���Ż��� */
typedef struct
{
    BS_CDR_TAG      stCDRTag;

    U32             ulUserID;                   /* �û�ID,Ҫ��ȫ����,������10λ,���λС��4 */
    U32             ulAgentID;                  /* ��ϯID,Ҫ��ȫ����,������10λ,���λС��4 */
    U32             ulCustomerID;               /* �ͻ�ID,Ҫ��ȫ����,������10λ,���λС��4 */
    U32             ulAccountID;                /* �˻�ID,Ҫ��ȫ����,������10λ,���λС��4 */
    U32             ulTaskID;                   /* ����ID,Ҫ��ȫ����,������10λ,���λС��4 */
    U32             ulRuleID;                   /* ʹ�õļƷѹ��� */

    U32             aulFee[BS_MAX_AGENT_LEVEL];                     /* ÿ���㼶�ķ���,��λ:1/100��,�� */
    S8              szCaller[BS_MAX_CALL_NUM_LEN];                  /* ���к��� */
    S8              szCallee[BS_MAX_CALL_NUM_LEN];                  /* ���к��� */
    S8              szAgentNum[BS_MAX_AGENT_NUM_LEN];               /* ��ϯ���� */
    U32             ulTimeStamp;                /* ����/����ʱ��� */
    U32             ulArrivedTimeStamp;         /* ����ʱ��� */
    U32             ulLen;                      /* ��Ϣ����,��λ:�ֽ� */
    U32             aulPeerIP[4];               /* �Զ�IP��ַ,��дΪ���Ͳ�IP;����IPv6 */
    U16             usPeerTrunkID;              /* �Զ��м�ID */
    U16             usTerminateCause;           /* ��ֹԭ�� */
    U8              ucServType;                 /* ҵ������ */
    U8              ucAgentLevel;               /* �����㼶,[0,BS_MAX_AGENT_LEVEL) */

}BS_CDR_MS_ST;

/* ��𻰵� */
typedef struct
{
    BS_CDR_TAG      stCDRTag;

    U32             ulCustomerID;               /* �ͻ�ID,Ҫ��ȫ����,������10λ,���λС��4 */
    U32             ulAccountID;                /* �˻�ID,Ҫ��ȫ����,������10λ,���λС��4 */
    U32             ulRuleID;                   /* ʹ�õļƷѹ��� */

    U32             aulFee[BS_MAX_AGENT_LEVEL];                     /* ÿ���㼶�ķ���,��λ:1/100��,�� */
    U32             ulTimeStamp;                /* ʱ��� */
    U8              ucAttrType;                 /* �����������,�ο�BS_BILLING_ATTRIBUTE_E */
    U8              ucAgentLevel;               /* �����㼶,[0,BS_MAX_AGENT_LEVEL) */

}BS_CDR_RENT_ST;

/* ���㻰�� */
typedef struct
{
    BS_CDR_TAG      stCDRTag;

    U32             ulSPID;                     /* �����ṩ��ID,Ҫ��ȫ����,������10λ,���λС��4 */
    U32             ulRuleID;                   /* ʹ�õļƷѹ��� */

    U32             ulFee;                      /* ����,��λ:1/100��,�� */
    S8              szCaller[BS_MAX_CALL_NUM_LEN];                  /* ���к��� */
    S8              szCallee[BS_MAX_CALL_NUM_LEN];                  /* ���к��� */
    U32             ulTimeStamp;                /* ʱ��� */
    U32             ulLen;                      /* ����ʱ������Ϣ����,��λ:��/�ֽ� */
    U32             aulPeerIP[4];               /* �Զ�IP��ַ,��дΪ���Ͳ�IP;����IPv6 */
    U16             usPeerTrunkID;              /* �Զ��м�ID */
    U16             usTerminateCause;           /* ��ֹԭ�� */
    U8              ucServType;                 /* ҵ������ */

}BS_CDR_SETTLE_ST;

/* �˻���¼ */
typedef struct
{
    BS_CDR_TAG      stCDRTag;

    U32             ulCustomerID;               /* �ͻ�ID,Ҫ��ȫ����,������10λ,���λС��4 */
    U32             ulAccountID;                /* �˻�ID,Ҫ��ȫ����,������10λ,���λС��4 */
    U8              ucOperateType;              /* ��������,�ο�:enBS_ACCOUNT_OPERATE_TYPE_E */
    S32             lMoney;                     /* ���,��λ:1/100��,�� */
    S64             LBalance;                   /* ���,��λ:1/100��,�� */
    U32             ulPeeAccount;               /* �Զ��˻�ID,Ҫ��ȫ����,������10λ,���λС��4 */
    U32             ulTimeStamp;                /* ʱ��� */
    U32             ulOperatorID;               /* ����Ա */
    U32             ulOperateDir;               /* ���� */
    S8              szRemark[BS_MAX_REMARK_LEN];                  /* ��ע */

}BS_CDR_ACCOUNT_ST;


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif


