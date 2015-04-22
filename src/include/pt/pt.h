#ifndef __PT_H__
#define __PT_H__

#ifdef  __cplusplus
extern "C"{
#endif

#include <dos.h>
#include <pt/pt_msg.h>
#include <netinet/in.h>
#include <sys/syscall.h>

#define PT_DATA_BUFF_10             10
#define PT_DATA_BUFF_16             16
#define PT_DATA_BUFF_32             32
#define PT_DATA_BUFF_64             64
#define PT_DATA_BUFF_128            128
#define PT_DATA_BUFF_256            256
#define PT_DATA_BUFF_512            512
#define PT_DATA_BUFF_1024           1024
#define PT_DATA_BUFF_2048           2048
#define MAX_FILENAME_LEN            24
#define PT_DATA_RECV_CACHE_SIZE     1024    /* ���ͻ���ĸ��� */
#define PT_DATA_SEND_CACHE_SIZE     512     /* ���ջ���ĸ��� */
#define PT_SEND_DATA_SIZE           700     /* �������ݰ�ʱ��ʹ�õ�����Ĵ�С */
#define PT_RECV_DATA_SIZE           512     /* socket ÿ���ݵĴ�С */
#define PT_SEND_LOSE_DATA_TIMER     1000    /* ���Ͷ�������ļ�� */
#define PT_RESEND_RSP_COUNT         3       /* �յ���������ʱ�����Ͷ�ʧ���ĸ��� */
#define PT_RESEND_REQ_MAX_COUNT     3       /* ��ʧ�İ������������� */
#define PT_SEND_CONFIRM_COUNT       3       /* ����ȷ����Ϣ�Ĵ��� */
#define PT_CONFIRM_RECV_MSG_SIZE    64      /* �յ����ٸ���ʱ������ȷ����Ϣ */
#define PT_LOGIN_VERIFY_SIZE        12      /* ptc��½ʱ����֤�ַ����Ĵ�С */
#define PT_SAVE_DATA_FAIL           -1      /* �������ݵ�����ʧ�� */
#define PT_SAVE_DATA_SUCC           0       /* �������ݵ�����ɹ� */
#define PT_NEED_CUT_PTHREAD         1       /* �����������Ҫ�л��߳� */
#define PT_VERSION_LEN              4       /* ptc�汾�ŵĳ��� */
#define PT_PTC_NAME_LEN             16      /* ptc���� */
#define PT_IP_ADDR_SIZE             16      /* ���ʮ���ƣ�ip�ĳ��� */
#define PT_MD5_LEN                  33      /* md5���ܵĳ��� 32+1 */
#define PT_MD5_ARRAY_SIZE           16
#define pt_logr_emerg(_pszFormat, args...)   pt_logr_write(LOG_LEVEL_ALERT, (_pszFormat), ##args)
#define pt_logr_cirt(_pszFormat, args...)    pt_logr_write(LOG_LEVEL_CIRT, (_pszFormat), ##args)
#define pt_logr_error(_pszFormat, args...)   pt_logr_write(LOG_LEVEL_ERROR, (_pszFormat), ##args)
#define pt_logr_warning(_pszFormat, args...) pt_logr_write(LOG_LEVEL_WARNING, (_pszFormat), ##args)
#define pt_logr_notic(_pszFormat, args...)   pt_logr_write(LOG_LEVEL_NOTIC, (_pszFormat), ##args)
#define pt_logr_info(_pszFormat, args...)    pt_logr_write(LOG_LEVEL_INFO, (_pszFormat), ##args)
#define pt_logr_debug(_pszFormat, args...)   pt_logr_write(LOG_LEVEL_DEBUG, (_pszFormat), ##args)
#define CRC16_POLY                  0x1021
#define PT_MUTEX_DEBUG              0
#define PT_PTC_MUTEX_DEBUG          0
#define PT_WEB_MUTEX_DEBUG          0

#define gettid() syscall(__NR_gettid)       /* ����̺߳� */

extern S32 g_ulUdpSocket;
extern U8  gucIsTableInit;
extern U16 g_crc_tab[256];

/* ������������� */
typedef enum
{
    PT_DATA_CTRL = 0,          /* ���� */
    PT_DATA_WEB,               /* web */
    PT_DATA_CMD,               /* ������ */
    PT_DATA_BUTT

}PT_DATA_TYPE_EN;

/* ͨ�����䷽ʽ */
typedef enum
{
    PT_TCP = 0,
    PT_UDP,
    PT_BUTT

}PT_PROT_TYPE_EN;


/* ���ݰ� */
typedef struct tagDataUdp
{
    S32                     lSeq;                             /* ����� */
    U32                     ulLen;                            /* ����С */
    U8                      ExitNotifyFlag;                   /* �Ƿ���Ӧ���� */
    S8                      Reserver[3];
    S8*                     pcBuff;                           /* ������ */

}PT_DATA_UDP_ST;


typedef struct tagDataTcp
{
    S32                     lSeq;                             /* ����� */
    U32                     ulLen;                            /* ����С */
    U8                      ExitNotifyFlag;                   /* �Ƿ���Ӧ���� */
    S8                      Reserver[3];

    S8                      szBuff[PT_RECV_DATA_SIZE];        /* ������ */

}PT_DATA_TCP_ST;


typedef union tagDataQue
{
    PT_DATA_UDP_ST* pstDataUdp;
    PT_DATA_TCP_ST* pstDataTcp;
}PT_DATA_QUE_HEAD_UN;


typedef struct tagStreamCB
{
    list_t                  stStreamListNode;
    PT_DATA_QUE_HEAD_UN     unDataQueHead;                    /* ���ݵ�ַ */
    DOS_TMR_ST              hTmrHandle;                       /* ��ʱ�� */
    struct tagLoseBagMsg    *pstLostParam;                    /* ��ʱ��������ַ�������ͷ���Դ */
    U32                     ulStreamID;                       /* stream ID */
    S32                     lMaxSeq;                          /* ������� */
    S32                     lCurrSeq;                         /* �ѷ��Ͱ��ı�� */
    S32                     lConfirmSeq;                      /* ȷ�Ͻ��յ������ */
    U32                     ulCountResend;
    U8                      aulServIp[IPV6_SIZE];
    U16                     usServPort;
    S8                      Reserver[2];

}PT_STREAM_CB_ST;

typedef struct tagChannelCB
{
    PT_DATA_TYPE_EN         enDataType;                        /*����*/

    list_t*                 pstStreamQueHead;                  /*stream list*/
    U32                     ulStreamNumInQue;

    BOOL                    bValid;                            /*��Ч��*/
    S8                      Reserver[3];

}PT_CHAN_CB_ST;

typedef struct tagCCCB
{
    list_t                  stCCListNode;
    U8                      aucID[PTC_ID_LEN];                /* IPCC ID */
    S32                     lSocket;                          /* UDP socket */
    struct sockaddr_in      stDestAddr;                       /* ipcc addr */
    PT_CHAN_CB_ST           astDataTypes[PT_DATA_BUTT];       /* �������� */
    U8                      aucPtcIp[IPV6_SIZE];
    DOS_TMR_ST              stHBTmrHandle;                    /* ������ʱ�� */
    U16                     usHBOutTimeCount;                 /* ����������ʱ���� */
    U16                     usPtcPort;
    U32                     ulUdpLostDataCount;
    U32                     ulUdpRecvDataCount;
    //S8                    Reserver[8];
}PT_CC_CB_ST;


/* ������������� */
typedef enum
{
    PT_CTRL_LOGIN_REQ = 0,
    PT_CTRL_LOGIN_RSP,
    PT_CTRL_LOGIN_ACK,
    PT_CTRL_LOGOUT,
    PT_CTRL_HB_REQ,
    PT_CTRL_HB_RSP,
    PT_CTRL_SWITCH_PTS,
    PT_CTRL_PTS_MAJOR_DOMAIN,
    PT_CTRL_PTS_MINOR_DOMAIN,
    PT_CTRL_PTC_PACKAGE,
    PT_CTRL_PING,
    PT_CTRL_PING_ACK,
    PT_CTRL_COMMAND,                /* pts�������ptc����ȡptc����Ϣ */

    PT_CTRL_BUTT

}PT_CTRL_TYPE_EN;

/* ������������� */
typedef enum
{
    PT_PTC_TYPE_EMBEDDED = 0,        /* EMBEDDED */
    PT_PTC_TYPE_WINDOWS,        /* WINDOWS */
    PT_PTC_TYPE_LINUX,           /* LINUX */

    PT_PTC_TYPE_BUTT

}PT_PTC_TYPE_EN;

/* ��Ʒ���� */
typedef enum
{
    PT_TYPE_PTS_LINUX = 1,
    PT_TYPE_PTS_WINDOWS,
    PT_TYPE_PTC_LINUX,
    PT_TYPE_PTC_WINDOWS,
    PT_TYPE_PTC_EMBEDDED,

    PT_TYPE_BUTT

}PT_PRODUCT_TYPE_EN;


/* ������Ϣ���� */
typedef struct tagCtrlData
{
    S8      szLoginVerify[PT_LOGIN_VERIFY_SIZE];    /* ���ڵ�½��֤������ַ��� */
    U8      szVersion[PT_VERSION_LEN];              /* �汾�� */
    S8      szPtcName[PT_PTC_NAME_LEN];             /* ptc���� */
    U32     ulLginVerSeq;                           /* ��½��֤����ַ����ڷ��ͻ����д洢�İ������ */
    S8      achPtsMajorDomain[PT_DATA_BUFF_64];     /* pts��������ַ */
    S8      achPtsMinorDomain[PT_DATA_BUFF_64];     /* pts��������ַ */
    S8      szPtsHistoryIp1[IPV6_SIZE];             /* ptcע���pts����ʷ��¼ */
    S8      szPtsHistoryIp2[IPV6_SIZE];             /* ptcע���pts����ʷ��¼ */
    S8      szPtsHistoryIp3[IPV6_SIZE];             /* ptcע���pts����ʷ��¼ */
    S8      szMac[PT_DATA_BUFF_64];
    U16     usPtsHistoryPort1;
    U16     usPtsHistoryPort2;
    U16     usPtsHistoryPort3;
    U16     usPtsMajorPort;                         /* pts�������˿� */
    U16     usPtsMinorPort;                         /* pts�������˿� */

    S32     lHBTimeInterval;                        /* ������������Ӧ�ļ�����鿴������� */

    U8      enCtrlType;                             /* ������Ϣ���� PT_CTRL_TYPE_EN */
    U8      ucLoginRes;                             /* ��½��� */
    U8      enPtcType;                              /* ptc���� PT_PTC_TYPE_EN */
    S8      Reserver[1];

}PT_CTRL_DATA_ST;

/* ��ʱ���ص������Ĳ��� */
typedef struct tagLoseBagMsg
{
    PT_MSG_TAG          stMsg;
    PT_STREAM_CB_ST     *pstStreamNode;
    S32                 lSocket;

}PT_LOSE_BAG_MSG_ST;

/* ��¼���ջ����п��Խ��յ����ݰ���ptc��stream�ڵ� */
typedef struct tagNendRecvNode
{
    list_t              stListNode;
    U8                  aucID[PTC_ID_LEN];          /* PTC ID */
    PT_DATA_TYPE_EN     enDataType;                 /* ���� */
    U32                 ulStreamID;                 /* stream ID */
    S32                 lSeq;
    U8                  ExitNotifyFlag;
    S8                  Reserver[3];

}PT_NEND_RECV_NODE_ST;

/* ��¼���ͻ�������Ҫ���͵����ݰ���ptc��stream�ڵ� */
typedef struct tagNendSendNode
{
    list_t              stListNode;
    U8                  aucID[PTC_ID_LEN];          /* PTC ID */
    PT_DATA_TYPE_EN     enDataType;                 /* ���� */
    U32                 ulStreamID;                 /* stream ID */
    S32                 lSeqResend;                 /* Ҫ���ط������ط��İ���� */
    PT_CMD_EN           enCmdValue;                 /* �ش�/ȷ��/���� */
    BOOL                bIsResend;                  /* �Ƿ����ط����� */
    U8                  ExitNotifyFlag;
    S8                  Reserver[3];

}PT_NEND_SEND_NODE_ST;


/* ping */
typedef struct tagPingPacket
{
   U32              ulSeq;
   U32              ulPacketSize;
   U32              ulPingNum;
   struct timeval   stStartTime;
   void             *pWpHandle;
   DOS_TMR_ST       hTmrHandle;     /* ��ʱ�� */

}PT_PING_PACKET_ST;

typedef struct tagPtcCommand
{
    U32 ulIndex;
    S8 szCommand[PT_DATA_BUFF_16];

}PT_PTC_COMMAND_ST;


/* ptc����������ͷ�Ľṹ�� */
typedef  struct tagFileHeadStruct
{
    U32 usCRC;                           /* CRCУ���� */
    S8  szFileName[MAX_FILENAME_LEN];    /* �汾�ļ��� */
    U32 ulVision;                        /* �汾�� */
    U32 ulFileLength;                    /* �ļ�����,����tag */
    U32 ulDate;                          /* ���� */
    U32 ulOSType;                        /* ����ϵͳ���� */

}LOAD_FILE_TAG;

PT_CC_CB_ST *pt_ptc_node_create(U8 *pcIpccId, S8 *szPtcVersion, struct sockaddr_in stDestAddr);
list_t *pt_delete_ptc_node(list_t *stPtcListHead, PT_CC_CB_ST *pstPtcNode);
list_t *pt_ptc_list_insert(list_t *pstPtcListHead, PT_CC_CB_ST *pstPtcNode);
PT_CC_CB_ST *pt_ptc_list_search(list_t* pstHead, U8 *pucID);
void pt_delete_ptc_resource(PT_CC_CB_ST *pstPtcNode);

PT_STREAM_CB_ST *pt_stream_node_create(U32 ulStreamID);
list_t *pt_stream_queue_insert(list_t *pstHead, list_t *pstStreamNode);
PT_STREAM_CB_ST *pt_stream_queue_search(list_t *pstStreamListHead, U32 ulStreamID);
list_t *pt_delete_stream_node(list_t *pstStreamListHead, list_t *pstStreamNode, PT_DATA_TYPE_EN enDataType);

PT_DATA_TCP_ST *pt_data_tcp_queue_create(U32 ulCacheSize);
S32 pt_send_data_tcp_queue_insert(PT_STREAM_CB_ST *pstStreamNode, S8 *acSendBuf, S32 lDataLen, U32 lCacheSize);
S32 pt_recv_data_tcp_queue_insert(PT_STREAM_CB_ST *pstStreamNode, PT_MSG_TAG *pstMsgDes, S8 *acRecvBuf, S32 lDataLen, U32 lCacheSize);

list_t *pt_need_recv_node_list_insert(list_t *pstHead, PT_MSG_TAG *pstMsgDes);
list_t *pt_need_recv_node_list_search(list_t *pstHead, U32 ulStreamID);
list_t *pt_need_send_node_list_insert(list_t *pstHead, U8 *aucID, PT_MSG_TAG *pstMsgDes, PT_CMD_EN enCmdValue, BOOL bIsResend);
list_t *pt_need_send_node_list_search(list_t *pstHead, U32 ulStreamID);

VOID pt_log_set_level(U32 ulLevel);
U32 pt_log_current_level();
VOID pt_logr_write(U32 ulLevel, S8 *pszFormat, ...);
S8 *pt_get_gmt_time(U32 ulExpiresTime);
BOOL pt_is_or_not_ip(S8 *szIp);
BOOL pts_is_ptc_sn(S8* pcUrl);
BOOL pts_is_int(S8* pcUrl);
S32 pt_DNS_analyze(S8 *szDomainName, U8 paucIPAddr[IPV6_SIZE]);
S32 pt_md5_encrypt(S8 *szEncrypt, S8 *szDecrypt);
U16 load_calc_crc(U8* pBuffer, U32 ulCount);

#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */

#endif
