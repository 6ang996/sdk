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

static U32 g_ulAppCRC32Val = U32_BUTT;

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

U32 dos_calc_app_crc(S8 *pszFileName, U32 *pulCRC)
{
    if (DOS_ADDR_INVALID(pszFileName) || DOS_ADDR_INVALID(pulCRC))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    *pulCRC = 0;

    return *pulCRC;
}

/**
 * ����: dos_db_detect
 * ����: ����MYSQL�Ƿ��ܹ���������
 * ����:
 * ����ֵ:
 *     ���С��0����ʾʧ�ܣ����������ڳ���
 *     �������0, ��ʾʧ�ܣ����������Ը������ٴγ���
 *     �������0����ʾMYSQL�ܹ�����������
 */
S32 dos_db_detect()
{
    U16 usDBPort;
    S8  szDBHost[DB_MAX_STR_LEN] = {0, };
    S8  szDBUsername[DB_MAX_STR_LEN] = {0, };
    S8  szDBPassword[DB_MAX_STR_LEN] = {0, };
    S8  szDBName[DB_MAX_STR_LEN] = {0, };
    S8  szDBSockPath[DB_MAX_STR_LEN] = {0, };
    DB_HANDLE_ST * pstDBHandle = NULL;

    if (config_get_db_host(szDBHost, DB_MAX_STR_LEN) < 0)
    {
        DOS_ASSERT(0);
        goto errno_proc;
    }

    if (config_get_db_user(szDBUsername, DB_MAX_STR_LEN) < 0)
    {
        DOS_ASSERT(0);
        goto errno_proc;
    }

    if (config_get_db_password(szDBPassword, DB_MAX_STR_LEN) < 0)
    {
        DOS_ASSERT(0);
        goto errno_proc;
    }

    usDBPort = config_get_db_port();
    if (0 == usDBPort || U16_BUTT == usDBPort)
    {
        usDBPort = 3306;
    }

    if (config_get_db_dbname(szDBName, DB_MAX_STR_LEN) < 0)
    {
        DOS_ASSERT(0);
        goto errno_proc;
    }

    if (config_get_mysqlsock_path(szDBSockPath, DB_MAX_STR_LEN) < 0)
    {
        DOS_ASSERT(0);
        goto errno_proc;
    }


    pstDBHandle = db_create(DB_TYPE_MYSQL);
    if (!pstDBHandle)
    {
        DOS_ASSERT(0);
        return -1;
    }

    dos_strncpy(pstDBHandle->szHost, szDBHost, sizeof(pstDBHandle->szHost));
    pstDBHandle->szHost[sizeof(pstDBHandle->szHost) - 1] = '\0';

    dos_strncpy(pstDBHandle->szUsername, szDBUsername, sizeof(pstDBHandle->szUsername));
    pstDBHandle->szUsername[sizeof(pstDBHandle->szUsername) - 1] = '\0';

    dos_strncpy(pstDBHandle->szPassword, szDBPassword, sizeof(pstDBHandle->szPassword));
    pstDBHandle->szPassword[sizeof(pstDBHandle->szPassword) - 1] = '\0';

    dos_strncpy(pstDBHandle->szDBName, szDBName, sizeof(pstDBHandle->szDBName));
    pstDBHandle->szDBName[sizeof(pstDBHandle->szDBName) - 1] = '\0';

    dos_strncpy(pstDBHandle->szSockPath, szDBSockPath, sizeof(pstDBHandle->szSockPath));
    pstDBHandle->szSockPath[sizeof(pstDBHandle->szSockPath) - 1] = '\0';

    pstDBHandle->usPort = usDBPort;

    if (db_open(pstDBHandle) < 0)
    {
        db_destroy(&pstDBHandle);
        pstDBHandle = NULL;
        return 1;
    }

    db_close(pstDBHandle);
    db_destroy(&pstDBHandle);
    pstDBHandle = NULL;

    return 0;

errno_proc:

    return -1;

}

/**
 * ����: main(int argc, char ** argv)
 * ����: ϵͳ���������
 */
int main(int argc, char ** argv)
{
    S8  szBuff[256] = { 0 };

    dos_set_process_name(argv[0]);

    printf("\n======================================================\n");
    snprintf(szBuff, sizeof(szBuff), " Process Name: %s\n", dos_get_process_name());
    printf("%s", szBuff);
    snprintf(szBuff, sizeof(szBuff), " Process Version: %s\n", dos_get_process_version());
    printf("%s", szBuff);
    printf("======================================================\n");

    dos_random_init();

#if INCLUDE_MEMORY_MNGT
    /* �ڴ����ģ�� */
    /* ����֮ǰ�벻Ҫʹ���Խ��ڴ������غ��� */
    if (dos_mem_mngt_init() < 0)
    {
        dos_printf("%s", "Init memory management fail. exit");
        exit(2);
    }
#endif

    /* ����֮ǰ�벻Ҫʹ��DOS_ASSERT */
    if (dos_assert_init() < 0)
    {
        dos_printf("%s", "Init assert module fail.");
        exit(1);
    }

    dos_calc_app_crc(argv[0], &g_ulAppCRC32Val);

#if (INCLUDE_XML_CONFIG)
    if (config_init() < 0)
    {
        dos_printf("%s", "Init config fail. exit");
        exit(1);
    }
#endif

#if (INCLUDE_DB_CLIENT)
    {
        S32 lRet        = 0;

        lRet = dos_db_detect();
        if (lRet != 0)
        {
            dos_printf("%s", "MySQL is temporary unavailable. while be try every 1 second");
            while (1)
            {
                dos_task_delay(1000);
                lRet = dos_db_detect();
                if (0 == lRet)
                {
                    break;
                }
                else if (lRet < 0)
                {
                    dos_printf("%s", "MySQL is not available. Please check you configure for MySQL.");
                    exit(100);
                }
            }

            dos_printf("%s", "MySQL is available.");
        }
    }
#endif

#if (INCLUDE_DB_CONFIG)
    if (dos_db_config_init() != DOS_SUCC)
    {
        dos_printf("%s", "Init db config fail. exit");
        exit(1);
    }
#endif

#if INCLUDE_EXCEPTION_CATCH
    /* �ź�ץȡ��ע�� */
    dos_signal_handle_reg();
#endif

#if (DOS_INCLUDE_SYS_STAT)
    /* CPUռ����� */
    dos_sysstat_cpu_start();
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
    /* ???��cli��????��?�� */
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

#if (INCLUDE_PTS)
    if (telnetd_init() < 0)
    {
        dos_printf("%s", "Init the telnet server task fail.");
        exit(255);
    }
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

    #if INCLUDE_LICENSE_CLIENT
    if (licc_init() != DOS_SUCC)
    {
        dos_printf("%s", "Init the liense mod FAIL.");
        exit(0);
    }
    #endif

    /* ����dos������ */
    if (root(argc, argv) < 0)
    {
        sleep(2);
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

    return 0;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

