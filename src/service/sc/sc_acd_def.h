/**
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  �ļ���: sc_acd_pub.h
 *
 *  ����ʱ��: 2015��1��14��14:42:12
 *  ��    ��: Larry
 *  ��    ��: ACDģ�鹫��ͷ�ļ�
 *  �޸���ʷ:
 */

#ifndef __SC_ACD_PUB_H__
#define __SC_ACD_PUB_H__

/* ������ϯ������������ĸ��� */
#define MAX_GROUP_PER_SITE      2

/* IP��ϯ:
 * 1. ��ʼ��ΪOFFLINE״̬
 * 2. ��½֮��ʹ���IDEL״̬
 * 3. ǩ��֮��ͽ���CONNECTED״̬
 * 4. ǩ��֮��ʹ���IEDL״̬
 * 5. WEB�ϵ���뿪�ʹ�������״̬
 * 6. ��ͨ���оʹ���BUSY״̬
 * 7. ���н����������ϯû�йһ��ͽ���CONNECTED״̬����ϯ�һ��ˣ��ʹ���IDEL״̬
 * PSTN��ϯ:
 * 1.��ʼ��ΪSC_ACD_IDEL״̬
 * 2.������ΪBUSY״̬
 * 3.���н����������ϯû�йһ��ͽ���CONNECTED״̬����ϯ�һ��ˣ��ʹ���IDEL״̬
 */
enum {
    SC_ACD_OFFLINE = 0,                 /* ����״̬�������� */
    SC_ACD_IDEL,                        /* ��ϯ�ѿ��� */
    SC_ACD_CONNECTED,                   /* ��ϯ�Ѿ�ǩ�� */
    SC_ACD_AWAY,                        /* ��ϯ��ʱ�뿪�������� */
    SC_ACD_BUSY,                        /* ��ϯæ�������� */

    SC_ACD_BUTT
};

enum {
    SC_ACD_SITE_ACTION_DELETE = 0,       /* ��ϯ����������ɾ�� */
    SC_ACD_SITE_ACTION_ADD,              /* ��ϯ����������ɾ�� */
    SC_ACD_SITE_ACTION_UPDATE,           /* ��ϯ����������ɾ�� */
    SC_ACD_SITE_ACTION_SIGNIN,           /* ��ϯ��½��  */
    SC_ACD_SITE_ACTION_SIGNOUT,          /* ��ϯ�ǳ� */
    SC_ACD_SITE_ACTION_ONLINE,           /* ��ϯ���� */
    SC_ACD_SITE_ACTION_OFFLINE,          /* ��ϯ��æ */
    SC_ACD_SITE_ACTION_EN_QUEUE,         /* ��ϯ���� */
    SC_ACD_SITE_ACTION_DN_QUEUE,         /* ��ϯ�˳����� */

    SC_ACD_SITE_ACTION_BUTT              /* ��ϯǩ��(����) */
};

enum {
    SC_ACD_POLICY_INCREASE               /* ���� */,
    SC_ACD_POLICY_INCREASE_LOOP,         /* ѭ������ */
    SC_ACD_POLICY_REDUCTION,             /* �ݼ� */
    SC_ACD_POLICY_REDUCTION_LOOP,        /* ѭ���ݼ� */
    SC_ACD_POLICY_MIN_CALL,

    SC_ACD_POLICY_BUTT
};

typedef struct tagACDSiteDesc{
    U16        usSCBNo;
    U16        usStatus;                          /* ��ϯ״̬ refer to SC_SITE_STATUS_EN */
    U32        ulSiteID;                          /* ��ϯ���ݿ��� */
    U32        ulCallCnt;                         /* �������� */
    U32        ulCustomerID;                      /* �������� */
    U32        aulGroupID[MAX_GROUP_PER_SITE];    /* ��ID */

    U32        bValid:1;                          /* �Ƿ���� */
    U32        bRecord:1;                         /* �Ƿ�¼�� */
    U32        bTraceON:1;                        /* �Ƿ���Ը��� */
    U32        bAllowAccompanying:1;              /* �Ƿ�����ֻ����� refer to SC_SITE_ACCOM_STATUS_EN */
    U32        bGroupHeader:1;                    /* �Ƿ����鳤 */
    U32        bLogin:1;                          /* �Ƿ��Ѿ���¼ */
    U32        bConnected:1;                      /* �Ƿ��Ѿ����� */
    U32        bWaitingDelete:1;                  /* �Ƿ����鳤 */
    U32        ucRes1:24;

    S8         szUserID[SC_TEL_NUMBER_LENGTH];    /* SIP User ID */
    S8         szExtension[SC_TEL_NUMBER_LENGTH]; /* �ֻ��� */
    S8         szEmpNo[SC_EMP_NUMBER_LENGTH];     /* ���� */

    pthread_mutex_t  mutexLock;
}SC_ACD_SITE_DESC_ST;

U32 sc_acd_init();
SC_ACD_SITE_DESC_ST  *sc_acd_get_site_by_grpid(U32 ulGroupID);
U32 sc_acd_agent_update_status(SC_ACD_SITE_DESC_ST *pstAgent, U32 ulStatus);

#endif

