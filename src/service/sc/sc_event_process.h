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
            "CHANNEL_CREATE " \
            "CHANNEL_DESTROY " \
            "CHANNEL_HANGUP " \
            "CHANNEL_HANGUP_COMPLETE " \
            "CHANNEL_ANSWER " \
            "DTMF " \
            "CHANNEL_PROGRESS " \
            "CHANNEL_PROGRESS_MEDIA " \
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

    SC_SERV_NUMBER_VIRFY            = 22,  /* �������ҵ�� */
    SC_SERV_INGROUP_CALL            = 23,  /* ���ں��� */        /* * */
    SC_SERV_OUTGROUP_CALL           = 24,  /* ������� */        /* *9 */
    SC_SERV_HOTLINE_CALL            = 25,  /* ���ߺ��� */
    SC_SERV_SITE_SIGNIN             = 26,  /* ��ϯǩ����� */    /* *2 */

    SC_SERV_WARNING                 = 27,  /* �澯��Ϣ */
    SC_SERV_QUERY_IP                = 28,  /* ��IP��Ϣ */        /* *158 */
    SC_SERV_QUERY_EXTENTION         = 29,  /* ��ֻ���Ϣ */      /* *114 */
    SC_SERV_QUERY_AMOUNT            = 30,  /* �����ҵ�� */      /* *199 */

    SC_SERV_BUTT                    = 255
};

enum {
    SC_NUM_TYPE_USERID              = 0,   /* ����ΪSIP User ID */
    SC_NUM_TYPE_EXTENSION,                 /* ����Ϊ�ֻ��� */
    SC_NUM_TYPE_OUTLINE,                   /* ����Ϊ���� */

    SC_NUM_TYPE_BUTT
};

/* ����CCBhash�� */
typedef struct tagHMapCCBCalleNode
{
    HASH_NODE_S     stNode;                       /* hash����ڵ� */

    S8              szCllee[SC_TEL_NUMBER_LENGTH]; /* �绰���� */
    S8              szCller[SC_TEL_NUMBER_LENGTH]; /* �绰���� */
    SC_CCB_ST       *pstCCB;                      /* CCBָ�� */
}SC_HMAP_CCB_CALLEE_NODE_ST;


#endif /* end of _SC_EVENT_PROCESS_H__ */

