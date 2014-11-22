/**
 *            (C) Copyright 2014, ����Ѷͨ . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  _log_console.h
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: ʵ�ֽ���־����������еĹ���
 *     History:
 */

#ifndef __LOG_CONSOLE_H__
#define __LOG_CONSOLE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dos/dos_types.h>
#include <syscfg.h>
extern "C"{
#include <dos/dos_debug.h>
#include <dos/dos_def.h>
}

#if (INCLUDE_SYSLOG_ENABLE && INCLUDE_SYSLOG_CONSOLE)

#include "_log_base.h"

#define MAX_FILENAME_LENGTH 64

class CLogConsole : public CLog
{
private:
    U32 ulLogLevel;
public:
    CLogConsole();
    ~CLogConsole();
    S32 log_init();
    S32 log_init(S32 _lArgc, S8 **_pszArgv);
    S32 log_set_level(U32 ulLevel);
    VOID log_write(const S8 *_pszTime, const S8 *_pszType, const S8 *_pszLevel, const S8 *_pszMsg, U32 _ulLevel);
    VOID log_write(const S8 *_pszTime, const S8 *_pszOpterator, const S8 *_pszOpterand, const S8* _pszResult, const S8 *_pszMsg);
};

#endif

#endif


