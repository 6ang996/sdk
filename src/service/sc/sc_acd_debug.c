/**
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  �ļ���: sc_acd_debug.c
 *
 *  ����ʱ��: 2015��1��14��14:42:07
 *  ��    ��: Larry
 *  ��    ��: ACDģ����ع��ܺ���ʵ��
 *  �޸���ʷ:
 */

#include <dos.h>

U32  g_ulTraceLevel = 0;

VOID sc_acd_trace(U32 ulLevel, S8 *pszFormat, ...)
{

}

U32 sc_acd_set_trace_level(U32 ulLevel)
{
    g_ulTraceLevel = ulLevel;

    return DOS_SUCC;
}


