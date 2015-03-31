/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  �ļ���: sc_httpd_pub.h
 *
 *  ����ʱ��: 2014��12��16��10:10:52
 *  ��    ��: Larry
 *  ��    ��: FS�����ⲿ�������õ�HTTP Server���ⲿ�ṩ�Ľӿ�
 *  �޸���ʷ:
 */

#ifndef __SC_HTTPD_PUB_H__
#define __SC_HTTPD_PUB_H__

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* include public header files */
#include <dos.h>

/* include private header files */

/* define marcos */

/* define enums */
/* HTTP��Ӧ��������ö�� */
enum SC_HTTP_DATA
{
    SC_HTTP_DATA_ERRNO = 0,
    SC_HTTP_DATA_MSG,

    SC_HTTP_DATA_BUTT
};

enum SC_HTTP_RSP_CODE
{
    SC_HTTP_1XX          = 0,
    SC_HTTP_OK,
    SC_HTTP_300,
    SC_HTTP_302,
    SC_HTTP_FORBIDDEN,
    SC_HTTP_NOTFOUND,
    SC_HTTP_500,

    SC_HTTP_BUTT
};


/* define structs */
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

/* declare functions */
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SC_HTTPD_PUB_H__ */

