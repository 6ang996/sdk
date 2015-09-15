/**
 *            (C) Copyright 2014, ����Ѷͨ . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  dos_db.h
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: �������ݲ�����غꡢ���ݽṹ������
 *     History:
 */

#ifndef __DOS_DB_H__
#define __DOS_DB_H__

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#if DB_MYSQL
#include <mysql/mysql.h>
#endif

#if DB_SQLITE
#include <sqlite3.h>
#endif

#define DB_DIGIST_TYPE_LENGTH 32

#define DB_MAX_STR_LEN        48

/* �ض���һЩ���ú��� */
#define db_mem_alloc   dos_dmem_alloc
#define db_mem_free    dos_dmem_free
#define db_memset      memset
#define db_assert      do{;}while
#define db_strlen      dos_strlen
#define db_strncpy     dos_strncpy
#define db_snprintf    dos_snprintf
#define db_log_debug   do{;}while
#define db_log_info    do{;}while
#define db_log_notice  do{;}while
#define db_log_warning do{;}while

/* ���������Σ�Ĭ��Ϊ���� */
#define FUNCATTR DLLEXPORT

/* ���β�ѯ�����ֶ��������ڵ��ֶν��ᱻ���� */
#define MAX_DB_FIELDS 32

/* ������ */
typedef enum tagDBERRNo
{
    DB_ERR_SUCC = 0,
    DB_ERR_NOT_INIT,
    DB_ERR_NOT_CONNECTED,
    DB_ERR_INVALID_DB_TYPE,
    DB_ERR_INVALID_PARAMS,
    DB_ERR_QUERY_FAIL,
    DB_ERR_QUERY_MEM_LEAK,
    DB_ERR_QUERY_FETCH_FAIL,
    DB_ERR_UNKNOW,
}DB_ERR_NO_EN;

/* ���ݿ������б� */
typedef enum tagDBType
{
    DB_TYPE_MYSQL,
    DB_TYPE_SQLITE,

    DB_TYPE_BUTT
}DB_TYPE_EN;

/* ���ݿ�״̬ */
typedef enum {
    DB_STATE_INIT,
    DB_STATE_DOWN,
    DB_STATE_CONNECTED,
    DB_STATE_ERROR
}SC_ODBC_STATUS_EN;

/* ���ݿ�����handle */
typedef struct tagDBHandle
{
#if DB_MYSQL
    MYSQL   *pstMYSql;     /* MYSQL���ݿ��� */
    S8      szHost[DB_MAX_STR_LEN];
    S8      szUsername[DB_MAX_STR_LEN];
    S8      szPassword[DB_MAX_STR_LEN];
    S8      szDBName[DB_MAX_STR_LEN];
    S8      szSockPath[DB_MAX_STR_LEN];
    U16     usPort;
#endif
#if DB_SQLITE
    sqlite3 *pstSQLite;    /* SQLite���ݿ��� */
    S8      *pszFilepath;
#endif
    U32     ulDBType;      /* Refer to tagDBType */
    U32     ulDBStatus;    /* Refer to SC_ODBC_STATUS_EN */
    S8      *pzsErrMsg;    /* ������ϢBUFFER */
    pthread_mutex_t  mutexHandle;
}DB_HANDLE_ST;

/* ���ݿ�API */

/**
 * ������db_create
 * ���ܣ����ݲ����������ݿ���
 * ������
 *      DB_TYPE_EN enType��Ҫ���������ݿ�����
 * ����ֵ��
 *      ����ɹ��������ݿ�����ָ�룬���ʧ�ܷ���NULL
 */
FUNCATTR DB_HANDLE_ST *db_create(DB_TYPE_EN enType);

/**
 * ������db_destroy
 * ���ܣ��������ݿ���
 * ������
 *      DB_HANDLE_ST **ppstDBHandle�����ݿ�����ָ��
 * ����ֵ��
 *      �ɹ�����0��ʧ�ܷ���-1
 */
FUNCATTR S32 db_destroy(DB_HANDLE_ST **pstDBHandle);

/**
 * ������db_open
 * ���ܣ������ݿ�
 * ������
 *      DB_HANDLE_ST *pstDBHandle�����ݿ�����ָ��
 * ����ֵ��
 *      �ɹ�����0��ʧ�ܷ���-1
 */
FUNCATTR S32 db_open(DB_HANDLE_ST *pstDBHandle);

/**
 * ������db_close
 * ���ܣ��ر����ݿ�
 * ������DB_HANDLE_ST *pstDBHandle�����ݿ�����ָ��
 * ����ֵ��
 *      �ɹ�����0��ʧ�ܷ���-1
 * ע�⣺
 *      �ú��������ͷ��ڴ棬��Ҫ����destroy�������پ��
 */
FUNCATTR S32 db_close(DB_HANDLE_ST *pstDBHandle);

/**
 * ������db_query
 * ���ܣ�ִ��sql���
 * ������
 *      DB_HANDLE_ST *pstDBHandle�����ݿ�����ָ��
 *      S8 *pszSQL, SQL���
 *      S32 (*callback)(VOID*, S32, S8**, S8**), �д�������û��ѯ��һ�л���øú���һ��
 *      VOID *pParamObj, �ص������Ķ����
 *      S8 **pszErrMsg��������Ϣ
 * ����ֵ��
 *      ������Ӧ�Ĵ�����refer DB_ERR_NO_EN
 */
FUNCATTR S32 db_query(DB_HANDLE_ST *pstDBHandle, S8 *pszSQL, S32 (*callback)(VOID*, S32, S8**, S8**), VOID *pParamObj, S8 **pszErrMsg);

/**
 * ������db_affect_raw
 * ���ܣ���ȡ��ѯ������еĺ���
 * ������
 *      DB_HANDLE_ST *pstDBHandle�����ݿ�����ָ��
 * ����ֵ��
 *      �ɹ����غ�����ʧ�ܷ���-1
 */
FUNCATTR S32 db_affect_raw(DB_HANDLE_ST *pstDBHandle);

/**
 * ������db_transaction_begin
 * ���ܣ���ʼִ������
 * ������
 *      DB_HANDLE_ST *pstDBHandle�����ݿ�����ָ��
 * ����ֵ���ɹ����غ�����ʧ�ܷ���-1
 */
FUNCATTR S32 db_transaction_begin(DB_HANDLE_ST *pstDBHandle);

/**
 * ������db_transaction_commit
 * ���ܣ��ύ����
 * ������
 *      DB_HANDLE_ST *pstDBHandle�����ݿ�����ָ��
 * ����ֵ��
 *      �ɹ����غ�����ʧ�ܷ���-1
 */
FUNCATTR S32 db_transaction_commit(DB_HANDLE_ST *pstDBHandle);

/**
 * ������db_transaction_rollback
 * ���ܣ��ع�����
 * ������
 *      DB_HANDLE_ST *pstDBHandle�����ݿ�����ָ��
 * ����ֵ��
 *      �ɹ����غ�����ʧ�ܷ���-1
 */
FUNCATTR S32 db_transaction_rollback(DB_HANDLE_ST *pstDBHandle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DOS_DB_H__ */


