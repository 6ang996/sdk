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
    SC_ERR_SUCC                             = 200,          /* �ɹ� */
    SC_ERR_BAD_REQUEST                      = 400,          /* �������� */
    SC_ERR_UNAUTHORIZED                     = 401,          /* δ��Ȩ */
    SC_ERR_PAYMENT_REQUIRED                 = 402,          /* ����Ҫ�� */
    SC_ERR_FORBIDDEN                        = 403,          /* ��ֹ */
    SC_ERR_NOT_FOUND                        = 404,          /* δ���� */
    SC_ERR_METHOD_NO_ALLOWED                = 405,          /* ���������� */
    SC_ERR_NOT_ACCEPTABLE                   = 406,          /* ���ɽ��� */
    SC_ERR_PROXY_AUTHENTICATION_REQUIRED    = 407,          /* ������Ҫ��֤ */
    SC_ERR_REQUEST_TIMEOUT                  = 408,          /* ����ʱ */
    SC_ERR_GONE                             = 410,          /* �뿪 */
    SC_ERR_REQUEST_ENTITY_TOO_LARGE         = 413,          /* ����ʵ��̫�� */
    SC_ERR_REQUEST_URL_TOO_LONG             = 414,          /* ����URL̫�� */
    SC_ERR_UNSUPPORTED_MEDIA_TYPE           = 415,          /* ��֧�ֵ�ý������ */
    SC_ERR_UNSUPPORTED_URL_SCHEME           = 416,          /* ��֧�ֵ�URL�ƻ� */
    SC_ERR_BAD_EXTENSION                    = 420,          /* ������չ */
    SC_ERR_EXTENSION_REQUIRED               = 421,          /* ��Ҫ��չ */
    SC_ERR_INTERVAL_TOO_BRIEF               = 423,          /* ���̫�� */
    SC_ERR_TEMPORARILY_UNAVAILABLE          = 480,          /* ��ʱʧЧ */
    SC_ERR_CALL_TRANSACTION_DOES_NOT_EXIST  = 481,          /* ����/���񲻴��� */
    SC_ERR_LOOP_DETECTED                    = 482,          /* ���ֻ�· */
    SC_ERR_TOO_MANY_HOPS                    = 483,          /* ����̫�� */
    SC_ERR_ADDRESS_INCOMPLETE               = 484,          /* ��ַ������ */
    SC_ERR_AMBIGUOUS                        = 485,          /* ������ */
    SC_ERR_BUSY_HERE                        = 486,          /* ����æ */
    SC_ERR_REQUEST_TERMINATED               = 487,          /* ������ֹ */
    SC_ERR_NOT_ACCEPTABLE_HERE              = 488,          /* �������󲻿ɽ��� */
    SC_ERR_REQUEST_PENDING                  = 491,          /* δ������ */
    SC_ERR_UNDECIPHERABLE                   = 493,          /* ���ɱ�ʶ */
    SC_ERR_SERVER_INTERNAL_ERROR            = 500,          /* �������ڲ����� */
    SC_ERR_NOT_IMPLEMENTED                  = 501,          /* ����ִ�� */
    SC_ERR_BAD_GATEWAY                      = 502,          /* ������ */
    SC_ERR_SERVICE_UNAVAILABLE              = 503,          /* ������Ч */
    SC_ERR_SERVER_TIME_OUT                  = 504,          /* ��������ʱ */
    SC_ERR_VERSION_NOT_SUPPORTED            = 505,          /* �汾��֧�� */
    SC_ERR_MESSAGE_TOO_LARGE                = 513,          /* ��Ϣ̫�� */
    SC_ERR_BUSY_EVERYWHERE                  = 600,          /* ȫæ */
    SC_ERR_DECLINE                          = 603,          /* ���� */
    SC_ERR_DOES_NOT_EXIST_ANYWHERE          = 604,          /* ������ */
    SC_ERR_NOT_ACCEPTABLE_606               = 606,          /* ���ɽ��� */

    SC_ERR_BUTT
};


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* end of __SC_PUB_H__ */


