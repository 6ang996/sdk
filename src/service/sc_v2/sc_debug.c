/**
 * @file : sc_debug.c
 *
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 * ���Ժ�������
 *
 * @date: 2016��1��9��
 * @arthur: Larry
 */

#ifdef __cplusplus
extern "C" {
#endif /* End of __cplusplus */

#include <dos.h>
#include "sc_def.h"

U32         g_ulSCLogLevel = LOG_LEVEL_DEBUG;

S32 cli_cc_process(U32 ulIndex, S32 argc, S8 **argv)
{
    return -1;
}


/**
 * SCģ���ӡ����
 *
 * @param const S8 *pszFormat ��ʽ�б�
 *
 * return NULL
 */
VOID sc_log(U32 ulLevel, const S8 *pszFormat, ...)
{
    va_list         Arg;
    U32             ulTraceTagLen = 0;
    S8              szTraceStr[1024] = {0, };
    BOOL            bIsOutput = DOS_FALSE;

    if (ulLevel >= LOG_LEVEL_INVAILD)
    {
        return;
    }

    /* warning��������ǿ����� */
    if (ulLevel <= LOG_LEVEL_WARNING)
    {
        bIsOutput = DOS_TRUE;
    }

    if (!bIsOutput
        && ulLevel <= g_ulSCLogLevel)
    {
        bIsOutput = DOS_TRUE;
    }

    if(!bIsOutput)
    {
        return;
    }

    ulTraceTagLen = dos_strlen(szTraceStr);

    va_start(Arg, pszFormat);
    vsnprintf(szTraceStr + ulTraceTagLen, sizeof(szTraceStr) - ulTraceTagLen, pszFormat, Arg);
    va_end(Arg);
    szTraceStr[sizeof(szTraceStr) -1] = '\0';

    dos_log(ulLevel, LOG_TYPE_RUNINFO, szTraceStr);

}

/**
 * SCģ���ӡ����
 *
 * @param const S8 *pszFormat ��ʽ�б�
 *
 * return NULL
 */
VOID sc_printf(const S8 *pszFormat, ...)
{
    va_list         Arg;
    S8              szTraceStr[1024] = {0, };
    U32             ulTraceTagLen = 0;

    va_start(Arg, pszFormat);
    vsnprintf(szTraceStr + ulTraceTagLen, sizeof(szTraceStr) - ulTraceTagLen, pszFormat, Arg);
    va_end(Arg);
    szTraceStr[sizeof(szTraceStr) -1] = '\0';

    dos_log(LOG_LEVEL_DEBUG, LOG_TYPE_RUNINFO, szTraceStr);
}

/**
 * ���ٴ�ӡҵ����ƿ�
 *
 * @parma SC_SRV_CB *pstSCB ҵ����ƿ�
 * @param const S8 *pszFormat ��ʽ�б�
 *
 * return NULL
 */
VOID sc_trace_scb(SC_SRV_CB *pstSCB, const S8 *pszFormat, ...)
{
    va_list         Arg;
    S8              szTraceStr[1024] = {0, };
    U32             ulTraceTagLen = 0;

    va_start(Arg, pszFormat);
    vsnprintf(szTraceStr + ulTraceTagLen, sizeof(szTraceStr) - ulTraceTagLen, pszFormat, Arg);
    va_end(Arg);
    szTraceStr[sizeof(szTraceStr) -1] = '\0';

    dos_log(LOG_LEVEL_DEBUG, LOG_TYPE_RUNINFO, szTraceStr);
}

/**
 * ���ٴ�ӡLEG���ƿ�
 *
 * @parma SC_LEG_CB *pstLCB   LEG���ƿ�
 * @param const S8 *pszFormat ��ʽ�б�
 *
 * return NULL
 */
VOID sc_trace_leg(SC_LEG_CB *pstLCB, const S8 *pszFormat, ...)
{
    va_list         Arg;
    S8              szTraceStr[1024] = {0, };
    U32             ulTraceTagLen = 0;

    va_start(Arg, pszFormat);
    vsnprintf(szTraceStr + ulTraceTagLen, sizeof(szTraceStr) - ulTraceTagLen, pszFormat, Arg);
    va_end(Arg);
    szTraceStr[sizeof(szTraceStr) -1] = '\0';

    dos_log(LOG_LEVEL_DEBUG, LOG_TYPE_RUNINFO, szTraceStr);
}

#ifdef __cplusplus
}
#endif /* End of __cplusplus */


