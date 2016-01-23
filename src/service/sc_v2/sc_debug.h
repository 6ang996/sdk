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

/* define enums */
typedef enum tagSCSUBMod{
    SC_MOD_DB        = 0,
    SC_MOD_DB_WQ,
    SC_MOD_DIGIST,
    SC_MOD_RES,
    SC_MOD_ACD,
    SC_MOD_EVENT,
    SC_MOD_SU,
    SC_MOD_BS,
    SC_MOD_EXT_MNGT,
    SC_MOD_CWQ,
    SC_MOD_PUBLISH,
    SC_MOD_ESL,
    SC_MOD_HTTP_API,
    SC_MOD_DATA_SYN,
    SC_MOD_TASK,

    SC_MOD_BUTT
}SC_SUBMOD_EN;

typedef struct tagSCModList{
    U32   ulIndex;
    S8    *pszName;
    U32   ulDefaultLogLevel;
    BOOL  bTrace;                       /* ģ����� */
    U32   (*fn_init)();
    U32   (*fn_start)();
    U32   (*fn_stop)();
}SC_MOD_LIST_ST;


typedef enum tagSCLogFlags{
    SC_LOG_DISIST  = 1 << 11,        /* �Ƿ��¼ժҪ */

    SC_LOG_NONE    = 0,        /* �Ƿ��¼ժҪ */
}SC_LOG_FLAG_EN;

/* ��־��� */
#define SC_LOG_SET_MOD(level, mod) (level & 0xF) | ((mod << 4) & 0x3F0)

#define SC_LOG_SET_FLAG(level, mod, flags) (level & 0xF) | ((mod << 4) & 0x3F0) | ((flags << 10) & 0xFFFFFC00)

/** ��¼��־
 |----------------------|  ------  |   ----    |
 |      �������        |   ģ��   | ��־����  |
 |       22BIT          |   6BIT   |   4BIT    |
 */
VOID sc_log(U32 ulLevel, const S8 *pszFormat, ...);
VOID sc_printf(const S8 *pszFormat, ...);
VOID sc_trace_scb(SC_SRV_CB *pstSCB, const S8 *pszFormat, ...);
VOID sc_trace_leg(SC_LEG_CB *pstLCB, const S8 *pszFormat, ...);
VOID sc_trace_task(SC_TASK_CB *pstLCB, const S8 *pszFormat, ...);

S8 *sc_event_str(U32 ulEvent);
S8 *sc_command_str(U32 ulCommand);

#endif /* __SC_DEBUG_H__ */

