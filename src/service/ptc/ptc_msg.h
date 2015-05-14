#ifndef __PTC_LIB_H__
#define __PTC_LIB_H__

#ifdef  __cplusplus
extern "C" {
#endif

#include <semaphore.h>
#include <pt/pt.h>

#define PTC_SQL_STR_SIZE            512        /* ���sql�������Ĵ�С */
#define PTC_SEND_HB_TIME            5000       /* ��pts���������ļ�� */
#define PTC_WAIT_HB_ACK_TIME        3000       /* ptc����pts������Ӧ�ĳ�ʱʱ�� */
#define PTC_HB_TIMEOUT_COUNT_MAX    4          /* ������Ӧ���������ʱ��������������Ϊ���ߣ����·��͵�½���� */
//#define PTC_DNS_IP_SIZE           2          /* ����DNS������IP������Ĵ�С */
#define PTC_DEBUG                   0          /* ���Ժ� */
#define PTC_WAIT_CONFIRM_TIMEOUT    5          /* �ȴ�ȷ�Ͻ�����Ϣ�ĳ�ʱʱ�� s */
#define PTC_WEB_SOCKET_MAX_COUNT    16         /* ����proxy�����socket */
#define PTC_CMD_SOCKET_MAX_COUNT    8          /* ����proxy�����socket */
#define PTC_TIME_SIZE               24
#define PTC_RECV_FROM_PTS_CMD_SIZE  8
#define PTC_HISTORY_PATH            "var/pts_history"      /* history ���ļ�Ŀ¼ */

typedef struct tagServMsg
{
    S8                    szHistoryPath[PT_DATA_BUFF_128];             /* history ��·�� */
    S8                    achPtsMajorDomain[PT_DATA_BUFF_64];         /* pts��������ַ */
    S8                    achPtsMinorDomain[PT_DATA_BUFF_64];         /* pts��������ַ */
    U8                    achPtsMajorIP[IPV6_SIZE];          /* PTS �� IP */
    U8                    achPtsMinorIP[IPV6_SIZE];          /* PTS �� IP */
    S8                    szPtsLasterIP1[PT_IP_ADDR_SIZE];   /* pts��ʷ��¼ */
    S8                    szPtsLasterIP2[PT_IP_ADDR_SIZE];   /* pts��ʷ��¼ */
    S8                    szPtsLasterIP3[PT_IP_ADDR_SIZE];   /* pts��ʷ��¼ */
    U8                    achLocalIP[IPV6_SIZE];             /* ����IP */
    S8                    szMac[PT_DATA_BUFF_64];            /* mac��ַ */

    U16                   usPtsMajorPort;                    /* pts�������˿� */
    U16                   usPtsMinorPort;                    /* pts�������˿� */
    U16                   usPtsLasterPort1;                  /* PTS PORT */
    U16                   usPtsLasterPort2;                  /* PTS PORT */
    U16                   usPtsLasterPort3;                  /* PTS PORT */
    U16                   usLocalPort;                       /* PTS PORT */

} PTC_SERV_MSG_ST;

typedef struct tagPtcClientCB
{
    U32                     ulStreamID;                         /* stream ID */
    S32                     lSocket;                            /* client sockfd */
    BOOL                    bIsValid;

}PTC_CLIENT_CB_ST;


extern PT_CC_CB_ST      *g_pstPtcSend;
extern PT_CC_CB_ST      *g_pstPtcRecv;
extern PTC_SERV_MSG_ST   g_stServMsg;
extern pthread_cond_t    g_cond_recv;
extern pthread_mutex_t   g_mutex_recv;
extern list_t  *g_pstPtcNendRecvNode;
extern sem_t g_SemPtcRecv;
extern PT_PTC_TYPE_EN g_enPtcType;

VOID  ptc_save_msg_into_cache(PT_DATA_TYPE_EN enDataType, U32 ulStreamID, S8 *pcData, S32 lDataLen);
VOID *ptc_send_msg2pts(VOID *arg);
VOID *ptc_recv_msg_from_pts(VOID *arg);
VOID  ptc_send_heartbeat2pts();
VOID  ptc_send_login2pts();
VOID  ptc_send_logout2pts();
VOID  ptc_send_pthread_mutex_lock(S8 *szFileName, U32 ulLine);
VOID  ptc_send_pthread_mutex_unlock(S8 *szFileName, U32 ulLine);
VOID  ptc_recv_pthread_mutex_lock(S8 *szFileName, U32 ulLine);
VOID  ptc_recv_pthread_mutex_unlock(S8 *szFileName, U32 ulLine);
VOID  ptc_recv_pthread_cond_wait(S8 *szFileName, U32 ulLine);
VOID  ptc_send_exit_notify_to_pts(PT_DATA_TYPE_EN enDataType, U32 ulStreamID, S32 lSeq);
VOID  ptc_delete_send_stream_node(U32 ulStreamID, PT_DATA_TYPE_EN enDataType, BOOL bIsMutex);
VOID  ptc_delete_recv_stream_node(U32 ulStreamID, PT_DATA_TYPE_EN enDataType, BOOL bIsMutex);
S32   ptc_get_version(U8 *szVersion);
S32   ptc_create_udp_socket(U32 ulSocketCache);

#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */

#endif

