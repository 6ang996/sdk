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

#ifndef __SC_PUB_H__
#define __SC_PUB_H__

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */


/* ��������� */
enum SC_ERRCODE_E
{
    CC_ERR_NORMAL_CLEAR                         = 0,            /* �����ͷ� */
    CC_ERR_NO_REASON                            = 1,            /* ��ԭ�� */

    /* SIP ״̬�� */
    CC_ERR_SIP_SUCC                             = 200,          /* �ɹ� */
    CC_ERR_SIP_BAD_REQUEST                      = 400,          /* �������� */
    CC_ERR_SIP_UNAUTHORIZED                     = 401,          /* δ��Ȩ */
    CC_ERR_SIP_PAYMENT_REQUIRED                 = 402,          /* ����Ҫ�� */
    CC_ERR_SIP_FORBIDDEN                        = 403,          /* ��ֹ */
    CC_ERR_SIP_NOT_FOUND                        = 404,          /* δ���� */
    CC_ERR_SIP_METHOD_NO_ALLOWED                = 405,          /* ���������� */
    CC_ERR_SIP_NOT_ACCEPTABLE                   = 406,          /* ���ɽ��� */
    CC_ERR_SIP_PROXY_AUTHENTICATION_REQUIRED    = 407,          /* ������Ҫ��֤ */
    CC_ERR_SIP_REQUEST_TIMEOUT                  = 408,          /* ����ʱ */
    CC_ERR_SIP_GONE                             = 410,          /* �뿪 */
    CC_ERR_SIP_REQUEST_ENTITY_TOO_LARGE         = 413,          /* ����ʵ��̫�� */
    CC_ERR_SIP_REQUEST_URL_TOO_LONG             = 414,          /* ����URL̫�� */
    CC_ERR_SIP_UNSUPPORTED_MEDIA_TYPE           = 415,          /* ��֧�ֵ�ý������ */
    CC_ERR_SIP_UNSUPPORTED_URL_SCHEME           = 416,          /* ��֧�ֵ�URL�ƻ� */
    CC_ERR_SIP_BAD_EXTENSION                    = 420,          /* ������չ */
    CC_ERR_SIP_EXTENSION_REQUIRED               = 421,          /* ��Ҫ��չ */
    CC_ERR_SIP_INTERVAL_TOO_BRIEF               = 423,          /* ���̫�� */
    CC_ERR_SIP_TEMPORARILY_UNAVAILABLE          = 480,          /* ��ʱʧЧ */
    CC_ERR_SIP_CALL_TRANSACTION_NOT_EXIST       = 481,          /* ����/���񲻴��� */
    CC_ERR_SIP_LOOP_DETECTED                    = 482,          /* ���ֻ�· */
    CC_ERR_SIP_TOO_MANY_HOPS                    = 483,          /* ����̫�� */
    CC_ERR_SIP_ADDRESS_INCOMPLETE               = 484,          /* ��ַ������ */
    CC_ERR_SIP_AMBIGUOUS                        = 485,          /* ������ */
    CC_ERR_SIP_BUSY_HERE                        = 486,          /* ����æ */
    CC_ERR_SIP_REQUEST_TERMINATED               = 487,          /* ������ֹ */
    CC_ERR_SIP_NOT_ACCEPTABLE_HERE              = 488,          /* �������󲻿ɽ��� */
    CC_ERR_SIP_REQUEST_PENDING                  = 491,          /* δ������ */
    CC_ERR_SIP_UNDECIPHERABLE                   = 493,          /* ���ɱ�ʶ */
    CC_ERR_SIP_INTERNAL_SERVER_ERROR            = 500,          /* �������ڲ����� */
    CC_ERR_SIP_NOT_IMPLEMENTED                  = 501,          /* ����ִ�� */
    CC_ERR_SIP_BAD_GATEWAY                      = 502,          /* ������ */
    CC_ERR_SIP_SERVICE_UNAVAILABLE              = 503,          /* ������Ч */
    CC_ERR_SIP_SERVER_TIME_OUT                  = 504,          /* ��������ʱ */
    CC_ERR_SIP_VERSION_NOT_SUPPORTED            = 505,          /* �汾��֧�� */
    CC_ERR_SIP_MESSAGE_TOO_LARGE                = 513,          /* ��Ϣ̫�� */
    CC_ERR_SIP_BUSY_EVERYWHERE                  = 600,          /* ȫæ */
    CC_ERR_SIP_DECLINE                          = 603,          /* ���� */
    CC_ERR_SIP_NOT_EXIST_ANYWHERE               = 604,          /* ������ */
    CC_ERR_SIP_NOT_ACCEPTABLE_606               = 606,          /* ���ɽ��� */


    /* SC ģ��Ĵ����� */
    CC_ERR_SC_SERV_NOT_EXIST                    = 1000,         /* ҵ�񲻴���       403 */
    CC_ERR_SC_NO_SERV_RIGHTS                    = 1001,         /* ��ҵ��Ȩ��       403 */
    CC_ERR_SC_USER_OFFLINE                      = 1002,         /* �û�����         480 */
    CC_ERR_SC_USER_BUSY                         = 1003,         /* �û�æ           486 */
    CC_ERR_SC_USER_HAS_BEEN_LEFT                = 1004,         /* �û����뿪       480 */
    CC_ERR_SC_USER_DOES_NOT_EXIST               = 1005,         /* �û�������       403 */
    CC_ERR_SC_CUSTOMERS_NOT_EXIST               = 1006,         /* �ͻ�������       403 */
    CC_ERR_SC_CB_ALLOC_FAIL                     = 1007,         /* ���ƿ����ʧ��   500 */
    CC_ERR_SC_MEMORY_ALLOC_FAIL                 = 1008,         /* �ڴ����ʧ��     500 */
    CC_ERR_SC_IN_BLACKLIST                      = 1009,         /* ������           404 */
    CC_ERR_SC_CALLER_NUMBER_ILLEGAL             = 1010,         /* ���к���Ƿ�     404 */
    CC_ERR_SC_CALLEE_NUMBER_ILLEGAL             = 1011,         /* ���к���Ƿ�     404 */
    CC_ERR_SC_NO_ROUTE                          = 1012,         /* �޿���·��       404 */
    CC_ERR_SC_NO_TRUNK                          = 1013,         /* �޿����м�       404 */
    CC_ERR_SC_PERIOD_EXCEED                     = 1014,         /* ����ʱ������     480 */
    CC_ERR_SC_RESOURCE_EXCEED                   = 1015,         /* ������Դ����     480 */
    CC_ERR_SC_CONFIG_ERR                        = 1016,         /* �������ô���     503 */
    CC_ERR_SC_MESSAGE_PARAM_ERR                 = 1017,         /* ��Ϣ��������     503 */
    CC_ERR_SC_MESSAGE_SENT_ERR                  = 1018,         /* ��Ϣ���ʹ���     503 */
    CC_ERR_SC_MESSAGE_RECV_ERR                  = 1019,         /* ��Ϣ���մ���     503 */
    CC_ERR_SC_MESSAGE_TIMEOUT                   = 1020,         /* ��Ϣ��ʱ         408 */
    CC_ERR_SC_AUTH_TIMEOUT                      = 1021,         /* ��֤��ʱ         408 */
    CC_ERR_SC_QUERY_TIMEOUT                     = 1022,         /* ��ѯ��ʱ         408 */
    CC_ERR_SC_CLEAR_FORCE                       = 1023,         /* ǿ�Ʋ��         503 */
    CC_ERR_SC_SYSTEM_ABNORMAL                   = 1024,         /* ϵͳ�쳣         503 */
    CC_ERR_SC_SYSTEM_BUSY                       = 1025,         /* ϵͳæ           503 */
    CC_ERR_SC_SYSTEM_MAINTAINING                = 1026,         /* ϵͳά��         503 */
    CC_ERR_SC_FORBIDDEN                         = 1027,         /* ϵͳά��         403 */

    /* BS��֤������ */
    CC_ERR_BS_HEAD                              = 1199,         /* BS�����뿪ʼ */
    CC_ERR_BS_NOT_EXIST                         = 1200,         /* ������ */
    CC_ERR_BS_EXPIRE                            = 1201,         /* ����/ʧЧ */
    CC_ERR_BS_FROZEN                            = 1202,         /* ������ */
    CC_ERR_BS_LACK_FEE                          = 1203,         /* ���� */
    CC_ERR_BS_PASSWORD                          = 1204,         /* ������� */
    CC_ERR_BS_RESTRICT                          = 1205,         /* ҵ������ */
    CC_ERR_BS_OVER_LIMIT                        = 1206,         /* �������� */
    CC_ERR_BS_TIMEOUT                           = 1207,         /* ��ʱ */
    CC_ERR_BS_LINK_DOWN                         = 1208,         /* �����ж� */
    CC_ERR_BS_SYSTEM                            = 1209,         /* ϵͳ���� */
    CC_ERR_BS_MAINTAIN                          = 1210,         /* ϵͳά���� */
    CC_ERR_BS_DATA_ABNORMAL                     = 1211,         /* �����쳣*/
    CC_ERR_BS_PARAM_ERR                         = 1212,         /* �������� */
    CC_ERR_BS_NOT_MATCH                         = 1213,         /* ��ƥ�� */

    /* DB ������ */
    CC_ERR_DB_ADD_FAIL                          = 1400,         /* �� */
    CC_ERR_DB_DELETE_FAIL                       = 1401,         /* ɾ */
    CC_ERR_DB_UPDATE_FAIL                       = 1402,         /* �� */
    CC_ERR_DB_SELECT_FAIL                       = 1403,         /* �� */

    CC_ERR_BUTT
};

typedef enum tagCallResult
{
    CC_RST_CONNECT_FAIL          = 0,  /* δ��ͨ */
    CC_RST_NOT_FOUND             = 1,  /* �պ� */
    CC_RST_REJECTED              = 2,  /* ���б��ܾ� */
    CC_RST_BUSY                  = 3,  /* �ͻ�æ  */
    CC_RST_NO_ANSWER             = 4,  /* �ͻ�δ����  */
    CC_RST_HANGUP_WHILE_IVR      = 5,  /* ��������ʱ�Ҷ� */
    CC_RST_HANGUP_AFTER_KEY      = 6,  /* �ͻ������Ҷ� */
    CC_RST_HANGUP_NO_ANSER       = 7,  /* ��ϯ��û�����͹Ҷ��� */
    CC_RST_AGNET_BUSY            = 8,  /* ��ϯȫæ */
    CC_RST_AGENT_NO_ANSER        = 9,  /* ��ϯδ���� */
    CC_RST_QUEUE_TIMEOUT         = 10,  /* �Ŷӳ�ʱ�Ҷ� */
    CC_RST_AGENT_HANGUP          = 11, /* ���гɹ���ϯ�Ҷ� */
    CC_RST_CUSTOMER_HANGUP       = 12, /* ���гɹ��ͻ��Ҷ� */

    CC_RST_BUTT
}CC_CALL_RESULT_EN;

U32 sc_esl_reloadxml();

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* end of __SC_PUB_H__ */


