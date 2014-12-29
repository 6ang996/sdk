/*
 *            (C) Copyright 2014, ����Ѷͨ . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  main.c
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: SDK ��װ�������������������������ģ��
 *     History:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* include system header file */

/* include sdk header file */
#include <dos.h>

/* extern the sdk functions */
extern S32 root(S32 _argc, S8 ** _argv);

/**
 * ����: dos_create_pid_file()
 * ����: ����pid�ļ�
 */
S32 dos_create_pid_file()
{
    FILE *fp;
    S8  szBuffPath[256] = { 0 };
    S8  szBuffCmd[256] = { 0 };
    S8  *pszProcessName;

    /* ����pid�ļ�Ŀ¼ */
    if (dos_get_pid_file_path(szBuffPath, sizeof(szBuffPath)))
    {
        snprintf(szBuffCmd, sizeof(szBuffCmd), "mkdir -p %s", szBuffPath);
        system(szBuffCmd);
    }
    else
    {
        DOS_ASSERT(0);
    }

    /* ����̿��� */
    pszProcessName = dos_get_process_name();
    if (!pszProcessName)
    {
        DOS_ASSERT(0);
        dos_printf("%s", "Cannot create the pid file. Process name invalid");
        exit(0);
    }

    /* ����pid�ļ� */
    snprintf(szBuffCmd, sizeof(szBuffCmd), "%s/%s.pid", szBuffPath, pszProcessName);
    fp = fopen(szBuffCmd, "w+");
    if (!fp)
    {
        dos_printf("%s", "Create the pid file fial.");
        DOS_ASSERT(0);
        exit(0);
    }
    fprintf(fp, "%d", getpid());
    fclose(fp);

    return DOS_SUCC;
}

/**
 * ����: dos_destroy_pid_file()
 * ����: ɾ��pid�ļ�
 */
S32 dos_destroy_pid_file()
{
    S8  szBuff[256] = { 0 };
    S8  szBuffCmd[256] = { 0 };
    S8  *pszProcessName;

    /* ��ȡpid�ļ�·�� */
    if (!dos_get_pid_file_path(szBuff, sizeof(szBuff)))
    {
        DOS_ASSERT(0);
        dos_printf("%s", "Pid file path invalid");
        return -1;
    }

    /* ��ȡ������ */
    pszProcessName = dos_get_process_name();
    if (!pszProcessName)
    {
        DOS_ASSERT(0);
        dos_printf("%s", "Cannot destroy the pid file. Process name invalid");
        return -1;
    }

    snprintf(szBuffCmd, sizeof(szBuffCmd), "%s/%s.pid", szBuff, pszProcessName);
    unlink(szBuffCmd);

    return DOS_SUCC;
}

/**
 * ����: main(int argc, char ** argv)
 * ����: ϵͳ���������
 */
#ifdef DIPCC_FREESWITCH
DLLEXPORT int dos_main(int argc, char ** argv)
#else
int main(int argc, char ** argv)
#endif
{
    S8  szBuff[256] = { 0 };

    dos_set_process_name(argv[0]);

    printf("\n======================================================\n");
    snprintf(szBuff, sizeof(szBuff), " Process Name: %s\n", dos_get_process_name());
    printf("%s", szBuff);
    snprintf(szBuff, sizeof(szBuff), " Process Version: %s\n", dos_get_process_version());
    printf("%s", szBuff);
    printf("======================================================\n");

#if INCLUDE_MEMORY_MNGT
    /* �ڴ����ģ�� */
    if (dos_mem_mngt_init() < 0)
    {
        dos_printf("%s", "Init memory management fail. exit");
        exit(2);
    }
#endif

    if (dos_assert_init() < 0)
    {
        dos_printf("%s", "Init assert module fail.");
        exit(1);
    }

#if (INCLUDE_XML_CONFIG)
    if (config_init() < 0)
    {
        dos_printf("%s", "Init config fail. exit");
        exit(1);
    }
#endif

#if INCLUDE_EXCEPTION_CATCH
    /* �ź�ץȡ��ע�� */
    dos_signal_handle_reg();
#endif


#if INCLUDE_SERVICE_TIMER
    /* ������ʱ��ģ�� */
    if (tmr_task_init() < 0)
    {
        dos_printf("%s", "Init the timer task fail.");
        exit(3);
    }

    if (tmr_task_start() < 0)
    {
        dos_printf("%s", "Start the timer task fail.");
        exit(1);
    }
#endif

#if (INCLUDE_DEBUG_CLI_SERVER)
    if (telnetd_init() < 0)
    {
        dos_printf("%s", "Init the telnet server task fail.");
        exit(255);
    }

    if (cli_server_init() < 0)
    {
        dos_printf("%s", "Init the debug cli server task fail.");
        exit(255);
    }

    if (cli_server_start() < 0)
    {
        dos_printf("%s", "Start the debug cli server task fail.");
        exit(255);
    }
#endif

#if (INCLUDE_DEBUG_CLI)
    /* ����cli��־ģ�� */
    if (debug_cli_init(argc, argv) !=  DOS_SUCC)
    {
        dos_printf("%s", "Init the debug cli task fail.");
        exit(255);
    }
    dos_printf("%s", "Debug cli init successfully.");

    if (debug_cli_start() < 0)
    {
        dos_printf("%s", "Start the debug cli task fail.");
        exit(255);
    }
    dos_printf("%s", "Debug cli start successfully.");
#endif

#if INCLUDE_SYSLOG_ENABLE
    /* ������־ģ�� */
    if (dos_log_init() < 0)
    {
        dos_printf("%s", "Init the log task fail.");
        exit(255);
    }
    dos_printf("%s", "Log cli init successfully.");

    if (dos_log_start() !=  DOS_SUCC)
    {
        dos_printf("%s", "Start the log task fail.");
        exit(255);
    }
    dos_printf("%s", "Log cli start successfully.");
#endif

#if INCLUDE_BH_CLIENT
    /* ������־ģ�� */
    if (hb_client_init() < 0)
    {
        dos_printf("%s", "Heartbeat client init failed.");
        exit(255);
    }
    dos_printf("%s", "Heartbeat client init successfully.");

    if (hb_client_start() !=  DOS_SUCC)
    {
        dos_printf("%s", "Heartbeat client start failed.");
        exit(255);
    }
    dos_printf("%s", "Heartbeat client start successfully.");
#endif

    /* ����pid�ļ� */
    dos_create_pid_file();

    /* ����dos������ */
    if (root(argc, argv) < 0)
    {
        exit(254);
    }

#ifndef DIPCC_FREESWITCH

#if INCLUDE_SERVICE_TIMER
    /* ֹͣ��ʱ��ģ�� */
    tmr_task_stop();

#ifdef DEBUG_VERSION
    dos_printf("%s", "Timer task returned");
#endif
#endif


#if INCLUDE_DEBUG_CLI
    /* ֹͣcli����ģ�� */
    debug_cli_stop();
#ifdef DEBUG_VERSION
    dos_printf("%s", "Cli client task returned");
#endif
#endif

#if (INCLUDE_DEBUG_CLI_SERVER)
    /* ֹͣcli����ģ�� */
    cli_server_stop();
#ifdef DEBUG_VERSION
    dos_printf("%s", "Cli server task returned");
#endif
#endif

#if INCLUDE_SYSLOG_ENABLE
    /* ֹͣ��־ģ�� */
    dos_log_stop();
#ifdef DEBUG_VERSION
    printf("%s", "Log task returned");
#endif
#endif

#if (INCLUDE_DEBUG_CLI_SERVER)
    cli_server_stop();
#endif

    dos_destroy_pid_file();


#if (INCLUDE_XML_CONFIG)
    config_deinit();
#endif
    exit(0);
#endif
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

