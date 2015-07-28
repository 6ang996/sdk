/*
 *            (C) Copyright 2014, ����Ѷͨ . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  heartbeat.h
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: ���ļ���������ģ����ص�����
 *     History:
 */

#ifndef __HEARTBEAT_H__
#define __HEARTBEAT_H__


/* define local macros */
#define MAX_BUFF_LENGTH     1024

/* define heartbeat interval */
#define DEFAULT_HB_INTERVAL_MIN 3
#define DEFAULT_HB_INTERVAL_MAX 30

/* define mac fail count */
#define DEFAULT_HB_FAIL_CNT_MIN 10
#define DEFAULT_HB_FAIL_CNT_MAX 15

/* define heartbeat timeout length */
#define HB_TIMEOUT          3


/* ��Ϣ���� */
enum HEARTBEAT_DATA_TYPE{
    HEARTBEAT_DATA_REG  = 0,
    HEARTBEAT_DATA_REG_RESPONSE,

    HEARTBEAT_DATA_UNREG,
    HEARTBEAT_DATA_UNREG_RESPONSE,

    HEARTBEAT_WARNING_SEND,
    HEARTBEAT_WARNING_SEND_RESPONSE,

    HEARTBEAT_DATA_HB,
};

/* ģ��״̬ */
enum PROCESS_HB_STATUS{
    PROCESS_HB_INIT,
    PROCESS_HB_WORKING,
    PROCESS_HB_DEINIT,

    PROCESS_HB_BUTT
};

#define HB_MAX_PROCESS_NAME_LEN DOS_MAX_PROCESS_NAME_LEN
#define HB_MAX_PROCESS_VER_LEN  DOS_MAX_PROCESS_VER_LEN

/* ���� */
typedef struct tagHeartbeatData{
    U32 ulCommand;                                  /* ���������� */
    U32 ulLength;                                   /* ��Ϣ���� */
    S8  szProcessName[HB_MAX_PROCESS_NAME_LEN];     /* �������� */
    S8  szProcessVersion[HB_MAX_PROCESS_VER_LEN];   /* �汾�� */
    S8  szResv[8];                                  /* ���� */
}HEARTBEAT_DATA_ST;

/* ���ƿ� */
typedef struct tagProcessInfo{
    U32                ulStatus;                                  /* ��ǰ����״̬ */

    socklen_t          ulPeerAddrLen;                             /* �Զ˵�ַ���� */
    struct sockaddr_un stPeerAddr;                                /* �Զ˵�ַ */
    S32                lSocket;                                   /* �Զ�socket */

    S8                 szProcessName[HB_MAX_PROCESS_NAME_LEN];    /* �������� */
    S8                 szProcessVersion[HB_MAX_PROCESS_VER_LEN];  /* �汾�� */

    U32                ulHBCnt;
    U32                ulHBFailCnt;                               /* ʧ��ͳ�� */
#if INCLUDE_BH_CLIENT
    DOS_TMR_ST         hTmrRegInterval;                           /* ע���� */
    DOS_TMR_ST         hTmrSendInterval;                          /* ���ͼ�� */
    DOS_TMR_ST         hTmrRecvTimeout;                           /* ���ͳ�ʱ��ʱ�� */
#endif
#if INCLUDE_BH_SERVER
    U32                ulProcessCBNo;                             /* ���ƿ��� */
    U32                ulVilad;                                   /* �ÿ��ƿ��Ƿ���Ч */
    U32                ulActive;                                  /* �ý����Ƿ��ڼ���״̬ */
    DOS_TMR_ST         hTmrHBTimeout;                             /* �������ж�ʱ����ÿ���յ��������� */
#endif
}PROCESS_INFO_ST;

S32 hb_heartbeat_proc(PROCESS_INFO_ST *pstProcessInfo);
S32 hb_send_heartbeat(PROCESS_INFO_ST *pstProcessInfo);

#if INCLUDE_BH_SERVER
S32 hb_recv_external_warning(VOID *pMsg);
S32 hb_unreg_proc(PROCESS_INFO_ST *pstProcessInfo);
S32 hb_reg_proc(PROCESS_INFO_ST *pstProcessInfo);
VOID* hb_external_warning_proc(VOID* ptr);
#endif
#if INCLUDE_BH_CLIENT
S32 hb_send_reg(PROCESS_INFO_ST *pstProcessInfo);
S32 hb_unreg_response_proc(PROCESS_INFO_ST *pstProcessInfo);
S32 hb_reg_response_proc(PROCESS_INFO_ST *pstProcessInfo);
#endif
#endif

