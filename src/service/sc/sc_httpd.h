/*
*            (C) Copyright 2014, DIPCC . Co., Ltd.
*                    ALL RIGHTS RESERVED
*
*  �ļ���: httpd.c
*
*  ����ʱ��: 2014��12��16��10:10:52
*  ��    ��: Larry
*  ��    ��: FS�����ⲿ�������õ�HTTP Server����صĶ���
*  �޸���ʷ:
*/

#ifndef __SC_HTTPD_H__
#define __SC_HTTPD_H__

#if 0
#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */
#endif
/* include public header files */

/* include private header files */

/* define marcos */
/* ���������ջ��� */
#define SC_HTTP_MAX_RECV_BUFF_LEN      1024
#define SC_HTTP_MAX_SEND_BUFF_LEN      512


/* define enums */

/* define structs */
typedef struct tagHTTPDServerCB
{
    U32 ulValid;            /* �Ƿ����״̬ */
    U32 ulIndex;            /* ��ǰ��� */

	S32 lListenSocket;      /* ��ǰsocket������socket */
	U32 aulIPAddr[4];       /* ip��ַ�������IPv4��ֻʹ�õ�һ��U32�������IPv6��ȫ��ʹ�� */
	U16 usPort;             /* ��ǰhttpd�����Ķ˿� */
    U32 ulStatus;           /* ״̬ */

    U32 ulReqCnt;           /* ����ͳ�� */
}SC_HTTPD_CB_ST;

/* declare functions */

#if 0
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif

#endif /* __DIPCC_HTTPD_H__ */

