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

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* include public header files */

/* include private header files */

/* define marcos */
/* ���������ջ��� */
#define SC_HTTP_MAX_RECV_BUFF_LEN      1024
#define SC_HTTP_MAX_SEND_BUFF_LEN      512

/* define enums */
enum SC_HTTP_DATA{
    SC_HTTP_DATA_ERRNO = 0,
    SC_HTTP_DATA_MSG,

    SC_HTTP_DATA_BUTT
};

enum SC_HTTP_RSP_CODE{
    SC_HTTP_1XX          = 0,
    SC_HTTP_OK,
    SC_HTTP_300,
    SC_HTTP_302,
    SC_HTTP_FORBIDDEN,
    SC_HTTP_NOTFOUND,
    SC_HTTP_500,

    SC_HTTP_BUTT
};

/* ����HTTP��Ӧ���������������б� */
typedef struct tagResultNameList
{
    U32    ulHttpDataType;         /* ��������, refer to enum SC_HTTP_DATA*/
    S8     *pszName;               /* ������������ */
}SC_RESULT_NAME_NODE_ST;

/* ����HTTP������������� */
typedef struct tagHttpCmdErrNODesc
{
    U32    ulHttpErrNO;            /* ��������, refer to enum SC_HTTP_ERRNO*/
    S8     *pszVal;                /* ������������ */
}SC_HTTP_ERRNO_DESC_ST;

/* ����HTTP��Ӧ�� */
typedef struct tagHttpRspCodeDesc
{
    U32    ulIndex;                /* ��� refer to enum SC_HTTP_RSP_CODE */
    S8     *pszDesc;               /* ������������ */
}SC_HTTP_RSP_CODE_ST;


/* ����HTTP��Ӧ�������б� */
typedef struct tagResultDataList
{
    list_t stList;                 /* ��������ڵ� */

    U32    ulHttpDataType;         /* ��������, refer to enum SC_HTTP_DATA*/
    U32    ulIntegerResult;        /* ������������ */

    /* ������������(ֻ�����ַ���)��������ֶκ�ulIntegerResult��Ϊ�Ƿ�������Ը������� */
    S8     *pszResult;
}SC_RESULT_DATA_NODE_ST;


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
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DIPCC_HTTPD_H__ */

