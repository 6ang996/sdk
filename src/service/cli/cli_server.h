/*
 *            (C) Copyright 2014, ����Ѷͨ . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  cli_server.h
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: ʵ�������з�������ع���
 *     History:
 */

#ifndef __CLI_SERVER_H__
#define __CLI_SERVER_H__

#define MAX_RECV_BUFF 1400

typedef struct tagProcessInfoNode
{
    U32  ulIndex;              /* ���ƿ����� */
    BOOL bVaild;              /* �Ƿ����� */
    BOOL bActive;             /* �Ƿ���active״̬ */

    /* ������ */
    S8   szProcessName[DOS_MAX_PROCESS_NAME_LEN];

    /* ���̰汾�� */
    S8   szProcessVersion[DOS_MAX_PROCESS_VER_LEN];

    /* �ͻ��������ַ�ĳ��� */
    U32  uiClientAddrLen;
    /* �ͻ��������ַ */
    struct sockaddr_un stClientAddr;
}PROCESS_INFO_NODE_ST;

S32 cli_server_cmd_analyse(U32 ulClientIndex, U32 ulMode, S8 *szBuffer, U32 ulLength);
S32 cli_server_send_broadcast_cmd(U32 ulIndex, S8 *pszCmd, U32 ullength);
U32 cli_cmdset_get_group_num();

#endif
