/**
 *            (C) Copyright 2014, ����Ѷͨ . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  _log_db.h
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: ʵ�ֽ���־��������ݵĹ���
 *     History:
 */

#include "_log_db.h"

#if (INCLUDE_SYSLOG_ENABLE && INCLUDE_SYSLOG_DB)

CLogDB::CLogDB()
{
    blInited = 0;
    pstDBHandle = NULL;

    this->ulLogLevel = LOG_LEVEL_INFO;
}


CLogDB::~CLogDB()
{
    if (pstDBHandle)
    {
        db_destroy(&pstDBHandle);
    }
}


S32 CLogDB::log_init()
{
    U16 usDBPort;
    S8  szDBHost[MAX_DB_INFO_LEN] = {0, };
    S8  szDBUsername[MAX_DB_INFO_LEN] = {0, };
    S8  szDBPassword[MAX_DB_INFO_LEN] = {0, };
    S8  szDBName[MAX_DB_INFO_LEN] = {0, };

    if (config_get_db_host(szDBHost, MAX_DB_INFO_LEN) < 0)
    {
        DOS_ASSERT(0);
        goto errno_proc;
    }

    if (config_get_db_user(szDBUsername, MAX_DB_INFO_LEN) < 0)
    {
        DOS_ASSERT(0);
        goto errno_proc;
    }

    if (config_get_db_password(szDBPassword, MAX_DB_INFO_LEN) < 0)
    {
        DOS_ASSERT(0);
        goto errno_proc;
    }

    usDBPort = config_get_db_port();
    if (0 == usDBPort || U16_BUTT == usDBPort)
    {
        usDBPort = 3306;
    }

    if (config_get_db_dbname(szDBName, MAX_DB_INFO_LEN) < 0)
    {
        DOS_ASSERT(0);
        goto errno_proc;
    }


    pstDBHandle = db_create(DB_TYPE_MYSQL);
    if (!pstDBHandle)
    {
        DOS_ASSERT(0);
        return -1;
    }

    dos_strncpy(pstDBHandle->szHost, szDBHost, sizeof(pstDBHandle->szHost));
    pstDBHandle->szHost[sizeof(pstDBHandle->szHost) - 1] = '\0';

    dos_strncpy(pstDBHandle->szUsername, szDBUsername, sizeof(pstDBHandle->szUsername));
    pstDBHandle->szUsername[sizeof(pstDBHandle->szUsername) - 1] = '\0';

    dos_strncpy(pstDBHandle->szPassword, szDBPassword, sizeof(pstDBHandle->szPassword));
    pstDBHandle->szPassword[sizeof(pstDBHandle->szPassword) - 1] = '\0';

    dos_strncpy(pstDBHandle->szDBName, szDBName, sizeof(pstDBHandle->szDBName));
    pstDBHandle->szDBName[sizeof(pstDBHandle->szDBName) - 1] = '\0';

    pstDBHandle->usPort = usDBPort;

    if (db_open(pstDBHandle) < 0)
    {
        DOS_ASSERT(0);

        db_destroy(&pstDBHandle);
        pstDBHandle = NULL;
        return -1;
    }

    blInited = 1;

    return 0;

errno_proc:

    return -1;
}

/**
 * ���س�ʼ�����������ӵ�mysql���ݿ⣬�����ֳ�����
 * ��������
 *    ����1�����ݿ��������ַ��ip��
 *    ����2���˿� �������д0����ʾʹ��Ĭ�϶˿ڣ�
 *    ����3�����ݿ�
 *    ����4���û���
 *    ����5������
 * ����ֵ��
 *   �ɹ�����0��ʧ�ܷ��أ�1
 */
S32 CLogDB::log_init(S32 argc, S8 **argv)
{
    return 0;
}

S32 CLogDB::log_set_level(U32 ulLevel)
{
    if (ulLevel >= LOG_LEVEL_INVAILD)
    {
        return -1;
    }

    this->ulLogLevel = ulLevel;

    return 0;
}

/**
 * д��־����
 */
void CLogDB::log_write(const S8 *_pszTime, const S8 *_pszType, const S8 *_pszLevel, const S8 *_pszMsg, U32 _ulLevel)
{
    S8 szQuery[1024];

    if (!blInited)
    {
        return;
    }

    if (_ulLevel > this->ulLogLevel)
    {
        return;
    }

    dos_snprintf(szQuery, sizeof(szQuery), "INSERT INTO %s VALUES(NULL, \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\");"
                , "tbl_log"
                , _pszTime
                , _pszLevel
                , _pszType
                , "init"
                , dos_get_process_name()
                , _pszMsg);

    //printf("%s\n", szQuery);

    db_query(pstDBHandle, szQuery, NULL, NULL, NULL);
}

VOID CLogDB::log_write(const S8 *_pszTime, const S8 *_pszOpterator, const S8 *_pszOpterand, const S8* _pszResult, const S8 *_pszMsg)
{
    S8 szQuery[1024];

    if (!blInited)
    {
        return;
    }

    dos_snprintf(szQuery, sizeof(szQuery), "INSERT INTO %s VALUES(NULL, \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\");"
                    , "tbl_olog"
                    , _pszTime
                    , _pszOpterator
                    , dos_get_process_name()
                    , _pszOpterand
                    , _pszResult
                    , _pszMsg);
    db_query(pstDBHandle, szQuery, NULL, NULL, NULL);

    //printf("%s", szQuery);

}

#endif

