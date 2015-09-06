/**
 *            (C) Copyright 2014, ����Ѷͨ . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  log_console.c
 *
 *  Created on: 2015��9��4��10:35:09
 *      Author: Larry
 *        Todo: ʵ�ֽ���־����������еĹ���
 *     History:
 */
#include <dos.h>
#include "log_def.h"

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#define LOG_CONSOLE_DEFAULT_LEVEL LOG_LEVEL_DEBUG

static DOS_LOG_MODULE_ST m_stConsoleLog;

U32 log_console_init()
{
    return DOS_SUCC;
}

U32 log_console_start()
{
    return DOS_SUCC;
}

U32 log_console_stop()
{
    return DOS_SUCC;
}


VOID log_console_write_rlog(const S8 *_pszTime, const S8 *_pszType, const S8 *_pszLevel, const S8 *_pszMsg, U32 _ulLevel)
{
    if (!m_stConsoleLog.blInited || !m_stConsoleLog.blIsRunning)
    {
        return;
    }

    if (_ulLevel > m_stConsoleLog.ulCurrentLevel)
    {
        return;
    }

    printf("%-20s[%-8s][%-10s][%-8s] %s\n"
            , _pszTime, _pszLevel, dos_get_process_name(), _pszType, _pszMsg);
}

VOID log_console_write_olog(const S8 *_pszTime, const S8 *_pszOpterator, const S8 *_pszOpterand, const S8* _pszResult, const S8 *_pszMsg)
{
    if (!m_stConsoleLog.blInited || !m_stConsoleLog.blIsRunning)
    {
        return;
    }

    printf("%-20s[%-8s][%-10s][%-8s][%s][%s][%s]%s\n"
            , _pszTime
            , "NOTICE"
            , dos_get_process_name()
            , "OPTERATION"
            , _pszOpterator
            , _pszOpterator
            , _pszResult
            , _pszMsg);
}

U32 log_console_set_level(U32 ulLevel)
{
    if (ulLevel >= LOG_LEVEL_INVAILD)
    {
        return DOS_FAIL;
    }

    m_stConsoleLog.ulCurrentLevel = ulLevel;

    return DOS_SUCC;
}


U32 log_console_reg(DOS_LOG_MODULE_ST **pstModuleCB)
{
    m_stConsoleLog.ulCurrentLevel = LOG_CONSOLE_DEFAULT_LEVEL;                /* ��ǰlog���� */

    m_stConsoleLog.fnInit      = log_console_init;            /* ģ���ʼ������ */
    m_stConsoleLog.fnStart     = log_console_start;           /* ģ���������� */
    m_stConsoleLog.fnStop      = log_console_stop;            /* ģ��ֹͣ���� */
    m_stConsoleLog.fnWriteRlog = log_console_write_rlog;      /* ģ��д������־ */
    m_stConsoleLog.fnWriteOlog = log_console_write_olog;      /* ģ��д������־ */
    m_stConsoleLog.fnSetLevel  = log_console_set_level;

    m_stConsoleLog.blWaitingExit = DOS_FALSE;             /* �Ƿ��ڵȴ��˳� */
    m_stConsoleLog.blIsRunning = DOS_TRUE;                /* �Ƿ����������� */
    m_stConsoleLog.blInited    = DOS_TRUE;                /* �Ƿ񱻳�ʼ���� */

    *pstModuleCB = &m_stConsoleLog;

    return DOS_SUCC;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */


