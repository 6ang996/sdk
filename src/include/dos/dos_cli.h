/*
 *            (C) Copyright 2014, ����Ѷͨ . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  dos_cli.h
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: cliģ�����ⲿ�ṩ�Ľӿں���
 *     History:
 */

#ifndef __DBEUG_CLI_H__
#define __DBEUG_CLI_H__

/* ����Ƿ��ͻ���index�����Ա��������Ϳ�����Ϣ */
#define INVALID_CLIENT_INDEX   0xFFFF

/* ���������пͻ������ڼ��� */
enum CLIENT_LEVEL {
    CLIENT_LEVEL_CONFIG = 0,
    CLIENT_LEVEL_LOG,

    CLIENT_LEVEL_BUTT
};

enum MSG_TYPE_E
{
    MSG_TYPE_PROCESS_REG,             /* ע�� */
    MSG_TYPE_PROCESS_REG_RSP,         /* ע����Ӧ */

    MSG_TYPE_PROCESS_NAME,            /* �������� */
    MSG_TYPE_PROCESS_VERSION,         /* ���̰汾 */

    MSG_TYPE_PROCESS_UNREG,           /* ����ȡ��ע�� */
    MSG_TYPE_PROCESS_UNREG_RESPONCE,  /* ����ȡ��ע����Ӧ */

    MSG_TYPE_CMD,                     /* ���� */
    MSG_TYPE_CMD_RESPONCE,            /* ������Ӧ */

    MSG_TYPE_LOG,                     /* ��־ */

    MSG_TYPE_BUTT
};

/**
 * �������Ԫ
 */
typedef struct tagMsgUnit{
    U16 usType;      /* ���Ԫ���� */
    U16 usLength;    /* ���Ԫ���ݳ���, ֻ��ʾ���ݳ��� */
    U8  pszData[0];  /* ���Ԫ���� */
}MSG_UNIT_ST;

/**
 * ����ؿ���̨�ͽ���֮��ͨѶ����Ϣ�ṹ
 */
typedef struct tagCliMsgHeader{
    U16 usProteVersion;        /* ��Ϣ�汾 */
    U16 usClientIndex;         /* �ͻ���index */

    U16 usResv;                /* ���� */
    U16 usLength;              /* ���Ȳ�����Э��ͷ */

    U8  pszData[0];
}CLI_MSG_HEADER;


struct tagCommandGroup;
struct tagCommand;

typedef struct tagCommandGroup
{
    struct tagCommandGroup *pstGroup;
    struct tagCommand      *pstCmdSet;
    U32 ulSize;
}COMMAND_GROUP_ST;

typedef struct tagCommand
{
    struct tagCommandGroup *pstGroup;
    S8                     *pszCommand;
    S8                     *pszHelpText;
    S32 (*func)(U32 ulIndex, S32 argc, S8 **argv);
}COMMAND_ST;

#if INCLUDE_DEBUG_CLI_SERVER

S32 cli_server_init();
S32 cli_server_start();
S32 cli_server_stop();
S32 telnetd_init();
S32 telnetd_start();
S32 telnetd_stop();
S32 telnet_out_string(U32 ulIndex, S8 *pszBuffer);

#define cli_out_string(_lCIndex, _pMsg) \
        telnet_out_string((_lCIndex), (_pMsg))

#elif INCLUDE_DEBUG_CLI

S32 debug_cli_init(S32 _argc, S8 **_argv);
S32 debug_cli_start();
S32 debug_cli_stop();
S32 debug_cli_send_log(S8 * _ucMsg, S32 _iLength);
S32 debug_cli_send_cmd_responce(const S8 * _pszMsg, const S32 _lLength, const S32 _lClientIndex);

#define cli_out_string(_lClientIndex, _pszMsg) \
        debug_cli_send_cmd_responce((_pszMsg), dos_strlen(_pszMsg) + 1, (_lClientIndex))

#endif

#if (INCLUDE_DEBUG_CLI || INCLUDE_DEBUG_CLI_SERVER)
VOID cli_log_set_level(U32 ulLevel);
VOID cli_logr_write(U32 ulLevel, S8 *pszFormat, ...);


#define cli_logr_debug(_pszFormat, args...) \
        cli_logr_write(LOG_LEVEL_DEBUG, (_pszFormat), ##args)

#define cli_logr_info(_pszFormat, args...) \
        cli_logr_write(LOG_LEVEL_INFO, (_pszFormat), ##args)

#define cli_logr_notice(_pszFormat, args...) \
        cli_logr_write(LOG_LEVEL_NOTIC, (_pszFormat), ##args)

#define cli_logr_warning(_pszFormat, args...) \
        cli_logr_write(LOG_LEVEL_WARNING, (_pszFormat), ##args)

#define cli_logr_error(_pszFormat, args...) \
        cli_logr_write(LOG_LEVEL_ERROR, (_pszFormat), ##args)

#define cli_logr_cirt(_pszFormat, args...) \
        cli_logr_write(LOG_LEVEL_CIRT, (_pszFormat), ##args)

#define cli_logr_alert(_pszFormat, args...) \
        cli_logr_write(LOG_LEVEL_ALERT, (_pszFormat), ##args)

#define cli_logr_emerg(_pszFormat, args...) \
        cli_logr_write(LOG_LEVEL_EMERG, (_pszFormat), ##args)

#endif

#endif

