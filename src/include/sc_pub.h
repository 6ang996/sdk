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
    CC_ERR_SC_NO_SERV_RIGHTS,                                   /* ��ҵ��Ȩ��       403 */
    CC_ERR_SC_USER_OFFLINE,                                     /* �û�����         480 */
    CC_ERR_SC_USER_BUSY,                                        /* �û�æ           486 */
    CC_ERR_SC_USER_HAS_BEEN_LEFT,                               /* �û����뿪       480 */
    CC_ERR_SC_USER_DOES_NOT_EXIST,                              /* �û�������       403 */
    CC_ERR_SC_CUSTOMERS_NOT_EXIST,                              /* �ͻ�������       403 */
    CC_ERR_SC_CB_ALLOC_FAIL,                                    /* ���ƿ����ʧ��   500 */
    CC_ERR_SC_MEMORY_ALLOC_FAIL,                                /* �ڴ����ʧ��     500 */
    CC_ERR_SC_IN_BLACKLIST,                                     /* ������           404 */
    CC_ERR_SC_CALLER_NUMBER_ILLEGAL,                            /* ���к���Ƿ�     404 */
    CC_ERR_SC_CALLEE_NUMBER_ILLEGAL,                            /* ���к���Ƿ�     404 */
    CC_ERR_SC_NO_ROUTE,                                         /* �޿���·��       404 */
    CC_ERR_SC_NO_TRUNK,                                         /* �޿����м�       404 */
    CC_ERR_SC_PERIOD_EXCEED,                                    /* ����ʱ������     480 */
    CC_ERR_SC_RESOURCE_EXCEED,                                  /* ������Դ����     480 */
    CC_ERR_SC_CONFIG_ERR,                                       /* �������ô���     503 */
    CC_ERR_SC_MESSAGE_PARAM_ERR,                                /* ��Ϣ��������     503 */
    CC_ERR_SC_MESSAGE_SENT_ERR,                                 /* ��Ϣ���ʹ���     503 */
    CC_ERR_SC_MESSAGE_RECV_ERR,                                 /* ��Ϣ���մ���     503 */
    CC_ERR_SC_MESSAGE_TIMEOUT,                                  /* ��Ϣ��ʱ         408 */
    CC_ERR_SC_AUTH_TIMEOUT,                                     /* ��֤��ʱ         408 */
    CC_ERR_SC_QUERY_TIMEOUT,                                    /* ��ѯ��ʱ         408 */
    CC_ERR_SC_CLEAR_FORCE,                                      /* ǿ�Ʋ��         503 */
    CC_ERR_SC_SYSTEM_ABNORMAL,                                  /* ϵͳ�쳣         503 */
    CC_ERR_SC_SYSTEM_BUSY,                                      /* ϵͳæ           503 */
    CC_ERR_SC_SYSTEM_MAINTAINING,                               /* ϵͳά��         503 */

    /* BS��֤������ */
    CC_ERR_BS_NOT_EXIST                         = 1200,         /* ������ */
    CC_ERR_BS_EXPIRE,                                           /* ����/ʧЧ */
    CC_ERR_BS_FROZEN,                                           /* ������ */
    CC_ERR_BS_LACK_FEE,                                         /* ���� */
    CC_ERR_BS_PASSWORD,                                         /* ������� */
    CC_ERR_BS_RESTRICT,                                         /* ҵ������ */
    CC_ERR_BS_OVER_LIMIT,                                       /* �������� */
    CC_ERR_BS_TIMEOUT,                                          /* ��ʱ */
    CC_ERR_BS_LINK_DOWN,                                        /* �����ж� */
    CC_ERR_BS_SYSTEM,                                           /* ϵͳ���� */
    CC_ERR_BS_MAINTAIN,                                         /* ϵͳά���� */
    CC_ERR_BS_DATA_ABNORMAL,                                    /* �����쳣*/
    CC_ERR_BS_PARAM_ERR,                                        /* �������� */
    CC_ERR_BS_NOT_MATCH,                                        /* ��ƥ�� */

    /* ���ݿ� ��ɾ�Ĳ� ������ */
    CC_ERR_DB_ADD_FAIL                          = 1400,
    CC_ERR_DB_DELETE_FAIL,
    CC_ERR_DB_UPDATE_FAIL,
    CC_ERR_DB_SELECT_FAIL,

    CC_ERR_BUTT
};


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* end of __SC_PUB_H__ */


