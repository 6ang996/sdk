#ifndef __SC_BS_PUB_H__
#define __SC_BS_PUB_H__

/* BS�ͻ��˸��� */
#define SC_MAX_BS_CLIENT     1

/* ������ʱ����ش��� */
#define SC_BS_HB_TIMEOUT     3000
#define SC_BS_HB_INTERVAL    4000
#define SC_BS_HB_FAIL_CNT    5

/* ҵ��ʱ����ض��� */
#define SC_BS_SEND_INTERVAL  1500
#define SC_BS_SEND_TIMEOUT   1000
#define SC_BS_SEND_FAIL_CNT  3

#define SC_BS_MSG_HASH_SIZE  1024

#define SC_BS_MEG_BUFF_LEN   1024

#define SC_BS_NEED_RESEND    0

/* �ͻ���״̬ */
enum{
    SC_BS_STATUS_INVALID         = 0, /* û�г�ʼ�� */
    SC_BS_STATUS_INIT,                /* �Ѿ���ʼ���� */
    SC_BS_STATUS_CONNECT,             /* �Ѿ����� */
    SC_BS_STATUS_LOST,                /* ������ */
    SC_BS_STATUS_REALSE,              /* ������ */
};

/* �ͻ��˿��ƿ� */
typedef struct tagBSClient
{
    U32 ulIndex;
    U32 blValid;
    U32 ulStatus;
    S32 lSocket;

    U16 usPort;
    U16 usCommProto;

    union tagAddr{
        struct sockaddr_in  stInAddr;
        struct sockaddr_un  stUnAddr;
    }stAddr;

    U32 aulIPAddr[4];
    U32 ulHBFailCnt;
    DOS_TMR_ST hTmrHBInterval;
    DOS_TMR_ST hTmrHBTimeout;
    U32 ulCtrlFailCnt;
    DOS_TMR_ST hTmrCtrlInterval;
    DOS_TMR_ST hTmrCtrlTimeout;
}SC_BS_CLIENT_ST;

typedef struct tagBSMsgNode
{
    HASH_NODE_S   stLink;          /*  ����ڵ� */

    U32           ulFailCnt;       /* ����ʧ�ܴ��� */
    U32           ulRCNo;          /* ��Դ��ţ�SCB�ı�� */
    U32           ulSeq;           /* ��Դ��ţ�SCB�ı�� */

    VOID          *pData;          /* ���� */
    U32           ulLength;

    U32           blNeedSyn;
    DOS_TMR_ST    hTmrSendInterval;
}SC_BS_MSG_NODE;

U32 sc_send_release_ind2bs(BS_MSG_TAG *pstMsg);
U32 sc_send_hello2bs(U32 ulClientIndex);
U32 sc_send_billing_stop2bs(SC_SRV_CB *pstSCB, SC_LEG_CB *pstFristLeg, SC_LEG_CB *pstSecondLeg);
U32 sc_send_billing_stop2bs_record(SC_SRV_CB *pstSCB, SC_LEG_CB *pstLeg);
#if SC_BS_NEED_RESEND
U32 sc_bs_msg_free(U32 ulSeq);
#endif
#endif

