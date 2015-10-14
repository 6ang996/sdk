/*
 *            (C) Copyright 2014, ����Ѷͨ . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  dos_log.h
 *
 *  Created on: 2015��9��4��10:19:31
 *      Author: Larry
 *        Todo: logģ��˽��ͷ�ļ�
 *     History:
 */

#ifndef __LOG_DEF_H__
#define __LOG_DEF_H__

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* �������󻺴泤�� */
#define MAX_OPERAND_LENGTH    32

/* ����Ա���泤�� */
#define MAX_OPERATOR_LENGTH   32

/* ��־��󳤶ȣ�����������Ƚ����ض� */
#define LOG_MAX_LENGTH        1000

#define MAX_DB_INFO_LEN       100

enum tagLogMod{
    LOG_MOD_CONSOLE,
    LOG_MOD_CLI,
    LOG_MOD_DB,

    LOG_MOD_BUTT
}LOG_MOD_LIST;


/* ��־ģ���ʼ���������� */
typedef U32 (*log_init)();

/* ��־ģ�������������� */
typedef U32 (*log_start)();

/* ��־ģ��ֹͣ�������� */
typedef U32 (*log_stop)();

/* ���ø澯�������� */
typedef U32 (*log_set_level)(U32);

/* ��־ģ��д�������� */
typedef VOID (*log_write_rlog)(time_t, const S8 *, const S8 *, const S8 *, U32);

/* ��־ģ��д�������� */
typedef VOID (*log_write_olog)(const S8 *, const S8 *, const S8 *, const S8*, const S8 *);

typedef struct tagLogModule
{
    BOOL blInited;                      /* �Ƿ񱻳�ʼ���� */
    BOOL blIsRunning;                   /* �Ƿ����������� */
    BOOL blWaitingExit;                 /* �Ƿ��ڵȴ��˳� */

    U32 ulCurrentLevel;                 /* ��ǰlog���� */

    log_init        fnInit;            /* ģ���ʼ������ */
    log_start       fnStart;           /* ģ���������� */
    log_stop        fnStop;            /* ģ��ֹͣ���� */
    log_write_rlog  fnWriteRlog;       /* ģ��д������־ */
    log_write_olog  fnWriteOlog;       /* ģ��д������־ */
    log_set_level   fnSetLevel;
}DOS_LOG_MODULE_ST;


/* ��־ģ��ע�ắ�� */
typedef U32 (*log_reg)(DOS_LOG_MODULE_ST **);
typedef struct tagLogModuleNode
{
    U32                ulLogModule;    /* ģ���� */
    log_reg            fnReg;          /* ģ��ע�ắ�� */
    DOS_LOG_MODULE_ST  *pstLogModule;  /* ģ����ƿ� */
}DOS_LOG_MODULE_NODE_ST;

typedef struct tagLogDataNode
{
    DLL_NODE_S  stNode;
    time_t stTime;
    S32    lLevel;
    S32    lType;
    S8     *pszMsg;

    S8     szOperand[MAX_OPERAND_LENGTH];
    S8     szOperator[MAX_OPERATOR_LENGTH];
    U32    ulResult;
}LOG_DATA_NODE_ST;


U32 log_console_reg(DOS_LOG_MODULE_ST **pstModuleCB);
U32 log_db_reg(DOS_LOG_MODULE_ST **pstModuleCB);
U32 log_cli_reg(DOS_LOG_MODULE_ST **pstModuleCB);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* end of __LOG_DEF_H__ */

