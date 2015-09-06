/**
 *            (C) Copyright 2014, ����Ѷͨ . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  log_cli.c
 *
 *  Created on: 2015��9��4��10:46:21
 *      Author: Larry
 *        Todo: ʵ�ֽ���־�����ͳһ����ƽ̨
 *     History:
 */

#include <dos.h>
#include "log_def.h"

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */


#if (INCLUDE_SYSLOG_ENABLE && INCLUDE_SYSLOG_CLI && INCLUDE_DEBUG_CLI)

#define LOG_CLI_DEFAULT_LEVEL LOG_LEVEL_INFO

static DOS_LOG_MODULE_ST m_stCliLog;

U32 log_cli_init()
{
    m_stCliLog.blInited      = DOS_TRUE;     /* �Ƿ񱻳�ʼ���� */
    return DOS_SUCC;
}

U32 log_cli_start()
{
     m_stCliLog.blWaitingExit = DOS_FALSE;    /* �Ƿ��ڵȴ��˳� */
     m_stCliLog.blIsRunning   = DOS_TRUE;     /* �Ƿ����������� */
    return DOS_SUCC;
}

U32 log_cli_stop()
{
    return DOS_SUCC;
}


VOID log_cli_write_rlog(const S8 *_pszTime, const S8 *_pszType, const S8 *_pszLevel, const S8 *_pszMsg, U32 _ulLevel)
{
    S8 szBuff[1024] = { 0 };
    U32 ulLength;

    if (!m_stCliLog.blInited || !m_stCliLog.blIsRunning)
    {
        return;
    }

    if (LOG_LEVEL_INVAILD == m_stCliLog.ulCurrentLevel
        || _ulLevel > m_stCliLog.ulCurrentLevel)
    {
        return;
    }

    /* snprintf���ش�����ֽ����������� �ַ����������� */
    ulLength = snprintf(szBuff, sizeof(szBuff), "%-20s[%-8s][%-10s][%-8s] %s\r\n"
                         , _pszTime, _pszLevel, dos_get_process_name(), _pszType, _pszMsg);
    if (ulLength < sizeof(szBuff))
    {
        szBuff[ulLength] = '\0';
        ulLength++;
    }
    else
    {
        szBuff[sizeof(szBuff) - 1] = '\0';
    }

    debug_cli_send_log(szBuff, ulLength);
}

VOID log_cli_write_olog(const S8 *_pszTime, const S8 *_pszOpterator, const S8 *_pszOpterand, const S8* _pszResult, const S8 *_pszMsg)
{
    S8 szBuff[1024] = { 0 };
    U32 ulLength;

    if (!m_stCliLog.blInited || !m_stCliLog.blIsRunning)
    {
        return;
    }

    /* snprintf���ش�����ֽ����������� �ַ����������� */
    ulLength = snprintf(szBuff, sizeof(szBuff)
                         , "%-20s[%-8s][%-10s][%-8s][%s][%s][%s]%s\n"
                         , _pszTime
                         , "NOTICE"
                         , dos_get_process_name()
                         , "OPTERATION"
                         , _pszOpterator
                         , _pszOpterator
                         , _pszResult
                         , _pszMsg);
    if (ulLength < sizeof(szBuff))
    {
        szBuff[ulLength] = '\0';
        ulLength++;
    }
    else
    {
        szBuff[sizeof(szBuff) - 1] = '\0';
    }

    debug_cli_send_log(szBuff, ulLength);
}

U32 log_cli_set_level(U32 ulLevel)
{
    if (ulLevel >= LOG_LEVEL_INVAILD)
    {
        return DOS_FAIL;
    }

    m_stCliLog.ulCurrentLevel = ulLevel;

    return DOS_SUCC;
}


U32 log_cli_reg(DOS_LOG_MODULE_ST **pstModuleCB)
{
    m_stCliLog.ulCurrentLevel = LOG_CLI_DEFAULT_LEVEL;                /* ��ǰlog���� */

    m_stCliLog.fnInit      = log_cli_init;            /* ģ���ʼ������ */
    m_stCliLog.fnStart     = log_cli_start;           /* ģ���������� */
    m_stCliLog.fnStop      = log_cli_stop;            /* ģ��ֹͣ���� */
    m_stCliLog.fnWriteRlog = log_cli_write_rlog;      /* ģ��д������־ */
    m_stCliLog.fnWriteOlog = log_cli_write_olog;      /* ģ��д������־ */
    m_stCliLog.fnSetLevel  = log_cli_set_level;

    *pstModuleCB = &m_stCliLog;

    return DOS_SUCC;
}

#endif /* end of INCLUDE_SYSLOG_ENABLE && INCLUDE_SYSLOG_CLI && INCLUDE_DEBUG_CLI */

#ifdef __cplusplus
}
#endif /* __cplusplus */


