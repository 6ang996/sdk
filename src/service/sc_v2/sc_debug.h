/**
 * @file : sc_debug.h
 *
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 * ҵ�����ģ����־��ض���
 *
 * @date: 2016��1��13��
 * @arthur: Larry
 */

#ifndef __SC_DEBUG_H__
#define __SC_DEBUG_H__

VOID sc_log(U32 ulLevel, const S8 *pszFormat, ...);
VOID sc_printf(const S8 *pszFormat, ...);
VOID sc_trace_scb(SC_SRV_CB *pstSCB, const S8 *pszFormat, ...);
VOID sc_trace_leg(SC_LEG_CB *pstLCB, const S8 *pszFormat, ...);
VOID sc_trace_task(SC_TASK_CB *pstLCB, const S8 *pszFormat, ...);

#endif /* __SC_DEBUG_H__ */

