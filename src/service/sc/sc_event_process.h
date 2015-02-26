/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  �ļ���: sc_event_process.c
 *
 *  ����ʱ��: 2015��1��8��20:28:04
 *  ��    ��: Larry
 *  ��    ��: ����FS���ķ������ĸ����¼�
 *  �޸���ʷ:
 */

#ifndef _SC_EVENT_PROCESS_H__
#define _SC_EVENT_PROCESS_H__

#define SC_EP_EVENT_LIST \
            "CHANNEL_PARK " \
            "CHANNEL_CREATE " \
            "CHANNEL_EXECUTE_COMPLETE " \
            "CHANNEL_HANGUP " \
            "CHANNEL_HANGUP_COMPLETE " \
            "DTMF " \
            "SESSION_HEARTBEAT "


enum {
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
    SC_SERV_INTERNAL_SERVICE        = 22,  /* ����ҵ�� */

    SC_SERV_BUTT                    = 255
};

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

/* ����SCBhash�� */
typedef struct tagHMapSCBCalleNode
{
    HASH_NODE_S     stNode;                       /* hash����ڵ� */

    S8              szCllee[SC_TEL_NUMBER_LENGTH]; /* �绰���� */
    S8              szCller[SC_TEL_NUMBER_LENGTH]; /* �绰���� */
    SC_SCB_ST       *pstSCB;                      /* SCBָ�� */
}SC_HMAP_SCB_CALLEE_NODE_ST;


#endif /* end of _SC_EVENT_PROCESS_H__ */

