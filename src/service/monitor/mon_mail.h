#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#ifndef __MON_MAIL_H__
#define __MON_MAIL_H__

#include <pthread.h>

#define MON_MAIL_SERV_COUNT     1
#define MON_MAIL_BUFF_64        64
#define MON_MAIL_IP_SIZE        16       /* IP��ַ */
#define MON_MAIL_QUE_COUNT      50       /* ÿ�������������еĸ��� */
#define MON_MAIL_WAIT_TIMEOUT   5        /* �ʼ��ȴ����ͽ���ĳ�ʱʱ�� */
#define MON_MAIL_BSS_MAIL       "kobe@dipcc.com"

typedef enum tagMonWarningType
{
    MON_WARNING_TYPE_MAIL = 0,      /* �ʼ��澯 */
    MON_WARNING_TYPE_SMS,           /* ���Ÿ澯 */
    MON_WARNING_TYPE_VOICE,         /* �����澯 */

    MON_WARNING_TYPE_BUTT
}MON_WARNING_TYPE_EN;

typedef enum tagMonWarningLevel
{
    MON_WARNING_LEVEL_CRITICAL = 0,     /* ���ظ澯 */
    MON_WARNING_LEVEL_MAJOR,            /* ��Ҫ�澯 */
    MON_WARNING_LEVEL_MINOR,            /* ��Ҫ�澯 */

    MON_WARNING_LEVEL_BUTT
}MON_WARNING_LEVEL_EN;

typedef enum tagMonWarnMailOperate
{
    MON_WARN_MAIL_HELO = 0,
    MON_WARN_MAIL_AUTH,
    MON_WARN_MAIL_USERNAME,
    MON_WARN_MAIL_PASSWORD,
    MON_WARN_MAIL_FROM,
    MON_WARN_MAIL_RCPT_TO,
    MON_WARN_MAIL_DATA,
    MON_WARN_MAIL_QUIT,
    MON_WARN_MAIL_BUTT

}MON_WARN_MAIL_OPERATE_EN;


typedef enum tagMonSMTPErrno
{
    MON_SMTP_ERRNO_211 = 211,   /* ϵͳ״̬��ϵͳ������Ӧ  */
    MON_SMTP_ERRNO_214 = 214,   /* ������Ϣ  */
    MON_SMTP_ERRNO_220 = 220,   /* �������  */
    MON_SMTP_ERRNO_221 = 221,   /* ����رմ����ŵ�  */
    MON_SMTP_ERRNO_235 = 235,   /* �û���֤�ɹ�  */
    MON_SMTP_ERRNO_250 = 250,   /* Ҫ����ʼ��������  */
    MON_SMTP_ERRNO_251 = 251,   /* �û��Ǳ��أ���ת����  */
    MON_SMTP_ERRNO_334 = 334,   /* �ȴ��û�������֤��Ϣ*/
    MON_SMTP_ERRNO_354 = 354,   /* ��ʼ�ʼ����룬��.����  */
    MON_SMTP_ERRNO_421 = 421,   /* ����δ�������رմ����ŵ���������ر�ʱ����Ӧ�������Ϊ���κ��������Ӧ��  */
    MON_SMTP_ERRNO_450 = 450,   /* Ҫ����ʼ�����δ��ɣ����䲻���ã����磬����æ��  */
    MON_SMTP_ERRNO_451 = 451,   /* ����Ҫ��Ĳ�������������г���  */
    MON_SMTP_ERRNO_452 = 452,   /* ϵͳ�洢���㣬Ҫ��Ĳ���δִ��  */
    MON_SMTP_ERRNO_500 = 500,   /* ��ʽ���������ʶ��(�˴���Ҳ���������й���)*/
    MON_SMTP_ERRNO_501 = 501,   /* ������ʽ����  */
    MON_SMTP_ERRNO_502 = 502,   /* �����ʵ��  */
    MON_SMTP_ERRNO_503 = 503,   /* �������������  */
    MON_SMTP_ERRNO_504 = 504,   /* �����������ʵ��  */
    MON_SMTP_ERRNO_535 = 535,   /* �û���֤ʧ��  */
    MON_SMTP_ERRNO_550 = 550,   /* Ҫ����ʼ�����δ��ɣ����䲻���ã����磬����δ�ҵ����򲻿ɷ��ʣ�  */
    MON_SMTP_ERRNO_551 = 551,   /* �û��Ǳ��أ��볢��  */
    MON_SMTP_ERRNO_552 = 552,   /* �����Ĵ洢���䣬Ҫ��Ĳ���δִ��  */
    MON_SMTP_ERRNO_553 = 553,   /* �����������ã�Ҫ��Ĳ���δִ�У����������ʽ����  */
    MON_SMTP_ERRNO_554 = 554,   /* ����ʧ��  */

    MON_SMTP_ERRNO_BUTT

}MON_SMTP_ERRNO_EN;

/** �澯��Ϣ�Ľṹ��
*/
typedef struct tagMonWarningMsg
{
    U8  ucWarningType;          /* �澯������ MON_WARNING_TYPE_EN */
    U32 ulWarningLevel;         /* �澯���� MON_WARNING_LEVEL_EN */
    S8  szEmail[32];            /* �ʼ���ַ���õ�ַ�迪ͨSMTP���񣬷��ɷ��ʼ� */
    S8  szTelNo[32];            /* ������Ϣ���ֻ����� */
    S8  szTitle[128];           /* �澯���� */
    S8  szMessage[512];         /* �澯���� */
}MON_WARNING_MSG_ST;

/** �洢�澯��Ϣ������Ľṹ��
*/
typedef struct tagMonWarning
{
    BOOL                    bIsValid;               /* �Ƿ���Ч */
    MON_WARNING_MSG_ST      *pstWarningMsg;         /* �澯��Ϣ�ṹ�� */

}MON_WARNING_ST;

typedef struct tagMonWarningCB
{
    MON_WARNING_LEVEL_EN        enWarningLevel;            /* �澯���� */
    U32                         ulWarningCount;            /* �����и澯������ */
    MON_WARNING_ST              *pstWarningList;           /* �澯���� */
    pthread_mutex_t             stMutex;

}MON_WARNING_CB_ST;

typedef struct tagMonMailGlobal
{
    BOOL                        bIsValid;
    S8                          szMailServ[MON_MAIL_BUFF_64];
    S8                          szUsername[MON_MAIL_BUFF_64];
    S8                          szUsernameBase64[MON_MAIL_BUFF_64];
    S8                          szPasswd[MON_MAIL_BUFF_64];
    S8                          szPasswdBase64[MON_MAIL_BUFF_64];
    S8                          szMailIP[MON_MAIL_IP_SIZE];
    U32                         ulMailPort;
    S32                         lMonMailSockfd;
    MON_WARN_MAIL_OPERATE_EN    enOperate;                              /* ���� */
    U32                         ulErrno;                                /* ������� */
    pthread_t                   pthreadMailSend;
    sem_t                       stSem;

}MON_MAIL_GLOBAL_ST;

U32 mon_mail_init();
U32 mon_mail_start();
U32 mon_mail_send_warning(MON_WARNING_TYPE_EN enWarnType, MON_WARNING_LEVEL_EN enWarnLevel, S8 *szAddr, S8 *szTitle, S8 *szMessage);

#endif /* end of __SC_LOG_DIGEST_H__ */

#ifdef __cplusplus
}
#endif /* __cplusplus */



