/**
 *            (C) Copyright 2014, ����Ѷͨ . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  dos_log.cpp
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: ʵ����־ģ����غ�������
 *     History:
 */

/* system header files */
#include <dos.h>
#include "log_def.h"

#if INCLUDE_SYSLOG_ENABLE

static const S8 *g_pszLogType[] =
{
    "RUNINFO",
    "WARNING",
    "SERVICE",
    "OPTERATION",
    ""
};

static const S8 *g_pszLogLevel[] =
{
    "EMERG",
    "ALERT",
    "CIRT",
    "ERROR",
    "WARNING",
    "NOTICE",
    "INFO",
    "DEBUG",
    ""
};

/* ����ģ���ڲ���ȫ�ֱ��� */

/* ��־��ʽ�б� */
static DOS_LOG_MODULE_NODE_ST       g_astLogModList[] = {
#if INCLUDE_SYSLOG_CONSOLE
    {LOG_MOD_CONSOLE,    log_console_reg,        NULL},
#endif
#if INCLUDE_SYSLOG_CLI
    {LOG_MOD_CLI,        log_cli_reg,            NULL},
#endif
#if INCLUDE_SYSLOG_DB
    {LOG_MOD_DB,         log_db_reg,             NULL},
#endif

    {LOG_MOD_BUTT,       NULL,             NULL},
};

/* ��־�����Ƿ��ڵȴ��˳� */
static S32               g_lLogWaitingExit  = 1;

/* ��־ģ���Ƿ��ʼ����� */
static S32               g_lLogInitFinished = 0;

/* ��־�������� */
static DLL_S             g_stLogCacheList;

/* ��־����� */
static pthread_mutex_t   g_mutexLogTask     = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t    g_condLogTask      = PTHREAD_COND_INITIALIZER;
static pthread_t         g_pthIDLogTask;


/* ���غ������� */
//static char * dos_log_get_time(time_t _time, S8 *sz_time, S32 i_len);

/**
 * ������S32 dos_log_set_cli_level(U32 ulLeval)
 * ���ܣ�����cli��־���͵ļ�¼����
 * ������
 *      U32 ulLeval����־����
 * ����ֵ���ɹ�����0��ʧ�ܷ��أ�1
 */
DLLEXPORT S32 dos_log_set_cli_level(U32 ulLeval)
{
    U32 ulLogModule;

    if (ulLeval >= LOG_LEVEL_INVAILD)
    {
        DOS_ASSERT(0);
        return -1;
    }

    for (ulLogModule=0; ulLogModule<sizeof(g_astLogModList)/sizeof(DOS_LOG_MODULE_NODE_ST); ulLogModule++)
    {
        if (LOG_MOD_CLI == g_astLogModList[ulLogModule].ulLogModule)
        {
            if (g_astLogModList[ulLogModule].pstLogModule
                && g_astLogModList[ulLogModule].pstLogModule->fnSetLevel)
            {
                g_astLogModList[ulLogModule].pstLogModule->fnSetLevel(ulLeval);

                break;
            }
        }
    }

    return 0;
}

/**
 * ������S32 dos_log_set_console_level(U32 ulLeval)
 * ���ܣ�����cli��־���͵ļ�¼����
 * ������
 *      U32 ulLeval����־����
 * ����ֵ���ɹ�����0��ʧ�ܷ��أ�1
 */
S32 dos_log_set_console_level(U32 ulLeval)
{
    U32 ulLogModule;

    if (ulLeval >= LOG_LEVEL_INVAILD)
    {
        DOS_ASSERT(0);
        return -1;
    }

    for (ulLogModule=0; ulLogModule<sizeof(g_astLogModList)/sizeof(DOS_LOG_MODULE_NODE_ST); ulLogModule++)
    {
        if (LOG_MOD_CONSOLE == g_astLogModList[ulLogModule].ulLogModule)
        {
            if (g_astLogModList[ulLogModule].pstLogModule
                && g_astLogModList[ulLogModule].pstLogModule->fnSetLevel)
            {
                g_astLogModList[ulLogModule].pstLogModule->fnSetLevel(ulLeval);

                break;
            }
        }
    }

    return 0;

}

/**
 * ������S32 dos_log_set_db_level(U32 ulLeval)
 * ���ܣ�����cli��־���͵ļ�¼����
 * ������
 *      U32 ulLeval����־����
 * ����ֵ���ɹ�����0��ʧ�ܷ��أ�1
 */
S32 dos_log_set_db_level(U32 ulLeval)
{
    U32 ulLogModule;

    if (ulLeval >= LOG_LEVEL_INVAILD)
    {
        DOS_ASSERT(0);
        return -1;
    }

    for (ulLogModule=0; ulLogModule<sizeof(g_astLogModList)/sizeof(DOS_LOG_MODULE_NODE_ST); ulLogModule++)
    {
        if (LOG_MOD_DB == g_astLogModList[ulLogModule].ulLogModule)
        {
            if (g_astLogModList[ulLogModule].pstLogModule
                && g_astLogModList[ulLogModule].pstLogModule->fnSetLevel)
            {
                g_astLogModList[ulLogModule].pstLogModule->fnSetLevel(ulLeval);

                break;
            }
        }
    }

    return 0;

}

/**
 * ������S32 dos_log_init()
 * Todo����ʼ����־ģ��
 * ������
 * ����ֵ��
 *  �ɹ�����0��ʧ�ܷ��أ�1
 * */
S32 dos_log_init()
{
    U32 ulIndex;
    U32 ulRet;

    g_lLogWaitingExit = 0;

    DLL_Init(&g_stLogCacheList);

    /* ��������ģ��ע������ */
    for (ulIndex=0; ulIndex<sizeof(g_astLogModList)/sizeof(DOS_LOG_MODULE_NODE_ST); ulIndex++)
    {
        if (g_astLogModList[ulIndex].fnReg)
        {
            g_astLogModList[ulIndex].fnReg(&g_astLogModList[ulIndex].pstLogModule);
        }
    }

    /* ��ʼ������ģ�� */
    for (ulIndex=0; ulIndex<sizeof(g_astLogModList)/sizeof(DOS_LOG_MODULE_NODE_ST); ulIndex++)
    {
        if (g_astLogModList[ulIndex].pstLogModule
            && g_astLogModList[ulIndex].pstLogModule->fnInit)
        {
            ulRet = g_astLogModList[ulIndex].pstLogModule->fnInit();

            if (DOS_SUCC != ulRet)
            {
                return DOS_FAIL;
            }
        }
    }


    /* ��������ģ�� */
    for (ulIndex=0; ulIndex<sizeof(g_astLogModList)/sizeof(DOS_LOG_MODULE_NODE_ST); ulIndex++)
    {
        if (g_astLogModList[ulIndex].pstLogModule
            && g_astLogModList[ulIndex].pstLogModule->fnStart)
        {
            ulRet = g_astLogModList[ulIndex].pstLogModule->fnStart();

            if (DOS_SUCC != ulRet)
            {
                return DOS_FAIL;
            }
        }
    }

    g_lLogInitFinished = 1;

    return 0;
}

/**
 * ������void *dos_log_main_loop(void *_ptr)
 * Todo����־ģ���̺߳���
 * ������
 * ����ֵ��
 * */
void *dos_log_main_loop(void *_ptr)
{
    U32                 ulIndex;
    struct timespec     stTimeout;
    LOG_DATA_NODE_ST    *pstLogData = NULL;
    DLL_NODE_S          *pstDLLNode = NULL;
    S8                  szTime[32] = { 0 };

    /* �õȴ��˳���־ */
    g_lLogWaitingExit = 0;

    while(1)
    {
        pthread_mutex_lock(&g_mutexLogTask);
        stTimeout.tv_sec = time(0) + 1;
        stTimeout.tv_nsec = 0;
        pthread_cond_timedwait(&g_condLogTask, &g_mutexLogTask, &stTimeout);
        pthread_mutex_unlock(&g_mutexLogTask);

        /* ˳����������־ */
        while(1)
        {
            if (0 == DLL_Count(&g_stLogCacheList))
            {
                break;
            }

            pthread_mutex_lock(&g_mutexLogTask);
            pstDLLNode = dll_fetch(&g_stLogCacheList);
            pthread_mutex_unlock(&g_mutexLogTask);
            if (DOS_ADDR_INVALID(pstDLLNode))
            {
                break;
            }

            pstLogData = (LOG_DATA_NODE_ST *)pstDLLNode;
            for (ulIndex=0; ulIndex<sizeof(g_astLogModList)/sizeof(DOS_LOG_MODULE_NODE_ST); ulIndex++)
            {
                if (!g_astLogModList[ulIndex].pstLogModule)
                {
                    continue;
                }


                if (LOG_TYPE_OPTERATION != pstLogData->lType)
                {
                    if (g_astLogModList[ulIndex].pstLogModule->fnWriteRlog)
                    {
                        g_astLogModList[ulIndex].pstLogModule->fnWriteRlog(pstLogData->stTime
                                            , pstLogData->lType >= LOG_TYPE_INVAILD ? "" : g_pszLogType[pstLogData->lType]
                                            , pstLogData->lLevel >= LOG_LEVEL_INVAILD ? "" : g_pszLogLevel[pstLogData->lLevel]
                                            , pstLogData->pszMsg
                                            , pstLogData->lLevel);
                    }
                }
                else
                {
                    if (g_astLogModList[ulIndex].pstLogModule->fnWriteOlog)
                    {
                        g_astLogModList[ulIndex].pstLogModule->fnWriteOlog(dos_log_get_time(pstLogData->stTime, szTime, sizeof(szTime))
                                            , pstLogData->szOperator
                                            , pstLogData->szOperand
                                            , pstLogData->ulResult ? "SUCC" : "FAIL"
                                            , pstLogData->pszMsg);
                    }
                }
            }

            free(pstLogData->pszMsg);
            pstLogData->pszMsg = NULL;
            free(pstLogData);
            pstLogData = NULL;
            pstDLLNode = NULL;
        }

        /* ����˳���־ */
        if (g_lLogWaitingExit)
        {
            dos_printf("%s", "Log task finished flag has been set.");
            break;
        }
    }


    /* �����˳���־ */
    g_lLogWaitingExit = 0;

    dos_printf("%s", (S8 *)"Log task goodbye!");
    return NULL;
}

/**
 * ������S32 log_task_start()
 * Todo��������־ģ��
 * ������
 * ����ֵ��
 *  �ɹ�����0��ʧ�ܷ��أ�1
 * */
S32 dos_log_start()
{
    S32 lResult = 0;

    lResult = pthread_create(&g_pthIDLogTask, NULL, dos_log_main_loop, NULL);
    if (lResult != 0)
    {
        dos_printf("%s", "Fail to create the log task.");
        return -1;
    }
    return 0;
}

/**
 * ������VOID log_task_stop()
 * Todo��ֹͣ��־ģ��
 * ������
 * ����ֵ��
 * */
VOID dos_log_stop()
{
    pthread_mutex_lock(&g_mutexLogTask);
    g_lLogWaitingExit = 1;
    pthread_mutex_unlock(&g_mutexLogTask);
    g_lLogInitFinished = 0;

    dos_log(LOG_LEVEL_NOTIC, LOG_TYPE_RUNINFO, (S8 *)"log task will be stopped!");

    pthread_join(g_pthIDLogTask, NULL);
}

/**
 * ������void dos_log(S32 _iLevel, S32 _iType, S8 *_pszMsg)
 * Todo����־��¼�������ú�������־������־���棬��ͬ־��־������
 * ������
 * ����ֵ��
 * */
DLLEXPORT VOID dos_log(S32 _lLevel, S32 _lType, const S8 *_pszMsg)
{
    LOG_DATA_NODE_ST *pstLogData = NULL;
    DLL_NODE_S       *pstDLLNode = NULL;

    if (!g_lLogInitFinished)
    {
        dos_syslog(_lLevel, _pszMsg);
        printf("%s\r\n", _pszMsg);

        return;
    }

    if (!_pszMsg)
    {
        /* ��Ҫ���ԣ�����Ķ��Կ��������ѭ�� */
        return;
    }

    if (_lType >= LOG_TYPE_INVAILD)
    {
        /* ��Ҫ���ԣ�����Ķ��Կ��������ѭ�� */
        return;
    }

    if (_lLevel >= LOG_LEVEL_INVAILD)
    {
        /* ��Ҫ���ԣ�����Ķ��Կ��������ѭ�� */
        return;
    }

    pstLogData = (LOG_DATA_NODE_ST *)malloc(sizeof(LOG_DATA_NODE_ST));
    if (!pstLogData)
    {
        return;
    }

    /* �����ڴ� */
    pstLogData->pszMsg = (S8 *)malloc(LOG_MAX_LENGTH);
    if (!pstLogData->pszMsg)
    {
        free(pstLogData);
        return;
    }

    /* ��ʼ�� */
    pstLogData->stTime = time(NULL);
    pstLogData->lLevel = _lLevel;
    pstLogData->lType  = _lType;
    strncpy(pstLogData->pszMsg, _pszMsg, LOG_MAX_LENGTH);
    pstLogData->pszMsg[LOG_MAX_LENGTH - 1] = '\0';

    /* ������� */
    pstDLLNode = (DLL_NODE_S *)pstLogData;
    pthread_mutex_lock(&g_mutexLogTask);
    DLL_Add(&g_stLogCacheList, pstDLLNode);
    pthread_cond_signal(&g_condLogTask);
    pthread_mutex_unlock(&g_mutexLogTask);

    return;
}


/**
 * ������VOID dos_volog(S32 _lLevel, S8 *pszOpterator, S8 *pszOpterand, U32 ulResult, S8 *format, ...)
 * Todo��������־��¼�������ú�������־������־���棬��ͬ־��־������
 * ������
 *      S32 _lLevel�� ����
 *      S8 *pszOpterator�� ����Ա
 *      S8 *pszOpterand����������
 *      U32 ulResult : �������
 * ����ֵ��
 * */
DLLEXPORT VOID dos_olog(S32 _lLevel, S8 *pszOpterator, S8 *pszOpterand, U32 ulResult, S8 *_pszMsg)
{
    LOG_DATA_NODE_ST *pstLogData = NULL;
    DLL_NODE_S       *pstDLLNode = NULL;

    if (!g_lLogInitFinished)
    {
        dos_syslog(_lLevel, _pszMsg);

        return;
    }

    if (!_pszMsg)
    {
        /* ��Ҫ���ԣ�����Ķ��Կ��������ѭ�� */
        return;
    }

    if (_lLevel >= LOG_LEVEL_INVAILD)
    {
        /* ��Ҫ���ԣ�����Ķ��Կ��������ѭ�� */
        return;
    }

    pstLogData = (LOG_DATA_NODE_ST *)malloc(sizeof(LOG_DATA_NODE_ST));
    if (!pstLogData)
    {
        return;
    }

    /* �����ڴ棬���ֽڶ��� */
    pstLogData->pszMsg = (S8 *)malloc(LOG_MAX_LENGTH);
    if (!pstLogData->pszMsg)
    {
        free(pstLogData);
        return;
    }

    /* ��ʼ�� */
    pstLogData->stTime = time(NULL);
    pstLogData->lLevel = _lLevel;
    pstLogData->lType  = LOG_TYPE_OPTERATION;
    pstLogData->ulResult = ulResult;
    if (pszOpterator && pszOpterator[0] != '\0')
    {
        dos_strncpy(pstLogData->szOperator, pszOpterator, sizeof(pstLogData->szOperator));
        pstLogData->szOperator[sizeof(pstLogData->szOperator) - 1] = '\0';
    }
    else
    {
        pstLogData->szOperator[0] = '\0';
    }

    if (pszOpterand && pszOpterand[0] != '\0')
    {
        dos_strncpy(pstLogData->szOperand, pszOpterand, sizeof(pstLogData->szOperand));
        pstLogData->szOperand[sizeof(pstLogData->szOperand) - 1] = '\0';
    }
    else
    {
        pstLogData->szOperator[0] = '\0';
    }
    strncpy(pstLogData->pszMsg, _pszMsg, LOG_MAX_LENGTH);
    pstLogData->pszMsg[LOG_MAX_LENGTH - 1] = '\0';

    /* ������� */
    pstDLLNode = (DLL_NODE_S *)pstLogData;
    pthread_mutex_lock(&g_mutexLogTask);
    DLL_Add(&g_stLogCacheList, pstDLLNode);
    pthread_cond_signal(&g_condLogTask);
    pthread_mutex_unlock(&g_mutexLogTask);

    return;
}

/**
 * ������VOID dos_volog(S32 _lLevel, S8 *pszOpterator, S8 *pszOpterand, U32 ulResult, S8 *format, ...)
 * Todo��������־��¼�������ú�������־������־���棬��ͬ־��־������
 * ������
 *      S32 _lLevel�� ����
 *      S8 *pszOpterator�� ����Ա
 *      S8 *pszOpterand����������
 *      U32 ulResult : �������
 * ����ֵ��
 * */
DLLEXPORT VOID dos_volog(S32 _lLevel, S8 *pszOpterator, S8 *pszOpterand, U32 ulResult, S8 *format, ...)
{
    va_list argptr;
    S8 szBuff[1024];

    va_start( argptr, format );
    vsnprintf( szBuff, sizeof(szBuff), format, argptr );
    va_end( argptr );
    szBuff[sizeof(szBuff) -1] = '\0';

    dos_olog(_lLevel, pszOpterator, pszOpterand, ulResult, szBuff);
}

/**
 * ������void dos_vlog(S32 _iLevel, S32 _iType, S8 *format, ...)
 * Todo����־��¼�������ú�������־������־���棬��ͬ־��־������
 * ������
 * ����ֵ��
 * */
DLLEXPORT VOID dos_vlog(S32 _lLevel, S32 _lType, const S8 *format, ...)
{
    va_list argptr;
    char buf[1024];

    va_start( argptr, format );
    vsnprintf( buf, sizeof(buf), format, argptr );
    va_end( argptr );
    buf[sizeof(buf) -1] = '\0';

    dos_log(_lLevel, _lType, buf);
}


/**
 * ������static char * dos_log_get_time(time_t _stTime, S8 *szTime, S32 iLen)
 * Todo��ʱ���ʽ��Ϊ�ַ���
 * ������
 *      time_t _stTime ʱ���
 *      S8 *szTime ���������buff
 *      S32 iLen   ��������泤��
 * ����ֵ��
 *      �������������׵�ַ
 * */
S8 * dos_log_get_time(time_t _stTime, S8 *szTime, S32 lLen)
{
    struct tm *t        = NULL;
    struct tm stTime;

    t = dos_get_localtime_struct(_stTime, &stTime);

    snprintf(szTime, lLen, "%04d-%02d-%02d %02d:%02d:%02d"
            , t->tm_year + 1900
            , t->tm_mon + 1
            , t->tm_mday
            , t->tm_hour
            , t->tm_min
            , t->tm_sec);

    return szTime;
}


#else

DLLEXPORT S32 dos_log_set_cli_level(U32 ulLeval)
{
    return 1;
}


#endif


