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

#ifndef __BS_STAT_H__
#define __BS_STAT_H__

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */




/* ����ͳ�� */
typedef struct
{
    U32             ulTimeStamp;            /* ͳ�ƻ���ʱ��� */
    U32             ulCallCnt;              /* �����ܴ��� */
    U32             ulRingCnt;              /* ������� */
    U32             ulBusyCnt;              /* �û�æ���� */
    U32             ulNotExistCnt;          /* ���벻���ڴ��� */
    U32             ulNoAnswerCnt;          /* ��Ӧ����� */
    U32             ulRejectCnt;            /* �ܾ����� */
    U32             ulEarlyReleaseCnt;      /* ���ʹ��� */
    U32             ulAnswerCnt;            /* �������� */
    U32             ulPDD;                  /* ������ʱ�� */
    U32             ulAnswerTime;           /* ������ʱ�� */

}BS_STAT_OUTBAND;


/* ����ͳ�� */
typedef struct
{
    U32             ulTimeStamp;            /* ͳ�ƻ���ʱ��� */
    U32             ulCallCnt;              /* �����ܴ��� */
    U32             ulRingCnt;              /* ������� */
    U32             ulBusyCnt;              /* �û�æ���� */
    U32             ulNoAnswerCnt;          /* ��Ӧ����� */
    U32             ulEarlyReleaseCnt;      /* ���ʹ��� */
    U32             ulAnswerCnt;            /* ��ͨ���� */
    U32             ulConnAgentCnt;         /* ������ϯ���� */
    U32             ulAgentAnswerCnt;       /* ��ϯ�������� */
    U32             ulHoldCnt;              /* ���ִ��� */
    U32             ulAnswerTime;           /* ��ͨ��ʱ�� */
    U32             ulWaitAgentTime;        /* �ȴ���ϯ������ʱ�� */
    U32             ulAgentAnswerTime;      /* ��ϯ������ʱ�� */
    U32             ulHoldTime;             /* ������ʱ��,��λ:���� */

}BS_STAT_INBAND;


/* ���ͳ��,����������͹��� */
typedef struct
{
    U32             ulTimeStamp;            /* ͳ�ƻ���ʱ��� */
    U32             ulCallCnt;              /* �����ܴ��� */
    U32             ulRingCnt;              /* ������� */
    U32             ulBusyCnt;              /* �û�æ���� */
    U32             ulNotExistCnt;          /* ���벻���ڴ��� */
    U32             ulNoAnswerCnt;          /* ��Ӧ����� */
    U32             ulRejectCnt;            /* �ܾ����� */
    U32             ulEarlyReleaseCnt;      /* ���ʹ��� */
    U32             ulAnswerCnt;            /* �������� */
    U32             ulConnAgentCnt;         /* ������ϯ���� */
    U32             ulAgentAnswerCnt;       /* ��ϯ�������� */
    U32             ulPDD;                  /* ������ʱ�� */
    U32             ulAnswerTime;           /* ��ͨ��ʱ�� */
    U32             ulWaitAgentTime;        /* �ȴ���ϯ������ʱ�� */
    U32             ulAgentAnswerTime;      /* ��ϯ������ʱ�� */

}BS_STAT_OUTDIALING;


/* ��Ϣͳ��,����/���Ź��� */
typedef struct
{
    U32             ulTimeStamp;            /* ͳ�ƻ���ʱ��� */
    U32             ulSendCnt;              /* ���ʹ��� */
    U32             ulRecvCnt;              /* ���մ��� */
    U32             ulSendSucc;             /* ���ͳɹ����� */
    U32             ulSendFail;             /* ����ʧ�ܴ��� */
    U32             ulSendLen;              /* �������ֽ��� */
    U32             ulRecvLen;              /* �������ֽ��� */

}BS_STAT_MESSAGE;

typedef struct
{
    U32             ulObjectID;             /* ͳ�ƶ���ID,Ҫ��ȫ����,������10λ,���λС��4 */
    U8              ucObjectType;           /* ͳ�ƶ�������,�ο�:BS_OBJECT_E */
    U8              aucReserv[3];

}BS_STAT_TAG;

/*
ҵ��ͳ����Ϣ
ʹ��˵��:
1.ÿ���������ɺ�,����һ��ͳ������;
2.ͳ������̶�Ϊ1��Сʱ,ÿ��Сʱ����һ��Сʱͳ�Ƶ����ݴ��뵽���ݿ�;
3.ż��Сʱ��ͳ�ƴ�ŵ�����0���±���,����Сʱ��ͳ�����ݴ�ŵ�1���±���;
4.�쳣���:
  �����������,ͳ����Ϣֻ��ͳ�Ƶ��ӳ�����������������;
  ǰһ��Сʱ�ڵĻ���,��ǰһСʱ��ͳ�������Ѿ����,Ҳ���ٸ���ͳ��,����������;
*/
typedef struct
{
    BS_STAT_TAG             stStatTag;

    BS_STAT_OUTBAND         astOutBand[2];          /* ����ͳ�� */
    BS_STAT_INBAND          astInBand[2];           /* ����ͳ�� */
    BS_STAT_OUTDIALING      astOutDialing[2];       /* ���ͳ�� */
    BS_STAT_MESSAGE         astMS[2];               /* ��Ϣͳ�� */

}BS_STAT_SERV_ST;

typedef struct
{
    BS_STAT_TAG             stStatTag;

    BS_STAT_OUTBAND         stOutBand;              /* ����ͳ�� */

}BS_STAT_OUTBAND_ST;

typedef struct
{
    BS_STAT_TAG             stStatTag;

    BS_STAT_INBAND          stInBand;               /* ����ͳ�� */

}BS_STAT_INBAND_ST;

typedef struct
{
    BS_STAT_TAG             stStatTag;

    BS_STAT_OUTDIALING      stOutDialing;           /* ���ͳ�� */

}BS_STAT_OUTDIALING_ST;

typedef struct
{
    BS_STAT_TAG             stStatTag;

    BS_STAT_MESSAGE         stMS;                   /* ��Ϣͳ�� */

}BS_STAT_MESSAGE_ST;



/*
�˻��˻�ͳ��
ʹ��˵��:
1.ÿ���������ɺ�,����һ��ͳ������;
2.���˻��ṹ��Ϣ���д��ͳ��ֵ;
3.ͳ������Ϊÿ��,ÿ�������ͳ����Ϣ���뵽���ݿ�;
5.�쳣���:
  �����������,ͳ����Ϣֻ��ͳ�Ƶ��ӳ�����������������;
*/
typedef struct
{
    BS_STAT_TAG             stStatTag;

    U32     ulTimeStamp;                    /* ͳ�ƻ���ʱ��� */
    S32     lProfit;                        /* ����,��λ:1/100��,�� */
    S32     lOutBandCallFee;                /* ����/�¼���������,��λ:1/100��,�� */
    S32     lInBandCallFee;                 /* ����/�¼��������,��λ:1/100��,�� */
    S32     lAutoDialingFee;                /* ����/�¼��Զ��������,��λ:1/100��,�� */
    S32     lPreviewDailingFee;             /* ����/�¼�Ԥ���������,��λ:1/100��,�� */
    S32     lPredictiveDailingFee;          /* ����/�¼�Ԥ���������,��λ:1/100��,�� */
    S32     lRecordFee;                     /* ����/�¼�¼������,��λ:1/100��,�� */
    S32     lConferenceFee;                 /* ����/�¼��������,��λ:1/100��,�� */
    S32     lSmsFee;                        /* ����/�¼����ŷ���,��λ:1/100��,�� */
    S32     lMmsFee;                        /* ����/�¼����ŷ���,��λ:1/100��,�� */
    S32     lRentFee;                       /* ����/�¼�������,��λ:1/100��,�� */

}BS_STAT_ACCOUNT_ST;


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif



