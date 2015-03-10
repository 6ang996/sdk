/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  �ļ���: mod_dipcc_sc.c
 *
 *  ����ʱ��: 2014��12��16��14:03:55
 *  ��    ��: Larry
 *  ��    ��: ҵ��ģ�����������
 *  �޸���ʷ:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* include public header files */
#include <dos.h>
#include <bs_pub.h>

/* include private header files */
#include "sc_def.h"
#include "sc_debug.h"
#include "sc_httpd.h"
#include "sc_httpd_def.h"
#include "sc_debug.h"
#include "sc_event_process.h"
#include "dos/dos_py.h"

/* ���ݿ��� */
DB_HANDLE_ST         *g_pstSCDBHandle = NULL;


U32 sc_httpd_init();
U32 sc_task_mngt_init();
U32 sc_dialer_init();
U32 sc_ep_init();
U32 sc_acd_init();
U32 sc_bs_fsm_init();
U32 sc_httpd_start();
U32 sc_task_mngt_start();
U32 sc_dialer_start();
U32 sc_bs_fsm_start();
U32 sc_ep_start();
U32 sc_httpd_shutdown();
U32 sc_task_mngt_shutdown();
U32 sc_dialer_shutdown();

/* define marcos */

/* define enums */

/* define structs */

/* global variables */


/* declare functions */

BOOL g_blSCIsRunning = DOS_FALSE;

U32 sc_init_db()
{
    U16             usDBPort;
    S8              szDBHost[DB_MAX_STR_LEN] = {0, };
    S8              szDBUsername[DB_MAX_STR_LEN] = {0, };
    S8              szDBPassword[DB_MAX_STR_LEN] = {0, };
    S8              szDBName[DB_MAX_STR_LEN] = {0, };

    SC_TRACE_IN(0, 0, 0, 0);

    if (config_get_db_host(szDBHost, DB_MAX_STR_LEN) < 0)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    if (config_get_db_user(szDBUsername, DB_MAX_STR_LEN) < 0)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    if (config_get_db_password(szDBPassword, DB_MAX_STR_LEN) < 0)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    usDBPort = config_get_db_port();
    if (0 == usDBPort || U16_BUTT == usDBPort)
    {
        usDBPort = 3306;
    }

    if (config_get_db_dbname(szDBName, DB_MAX_STR_LEN) < 0)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    g_pstSCDBHandle = db_create(DB_TYPE_MYSQL);
    if (!g_pstSCDBHandle)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    dos_strncpy(g_pstSCDBHandle->szHost, szDBHost, sizeof(g_pstSCDBHandle->szHost));
    g_pstSCDBHandle->szHost[sizeof(g_pstSCDBHandle->szHost) - 1] = '\0';

    dos_strncpy(g_pstSCDBHandle->szUsername, szDBUsername, sizeof(g_pstSCDBHandle->szUsername));
    g_pstSCDBHandle->szUsername[sizeof(g_pstSCDBHandle->szUsername) - 1] = '\0';

    dos_strncpy(g_pstSCDBHandle->szPassword, szDBPassword, sizeof(g_pstSCDBHandle->szPassword));
    g_pstSCDBHandle->szPassword[sizeof(g_pstSCDBHandle->szPassword) - 1] = '\0';

    dos_strncpy(g_pstSCDBHandle->szDBName, szDBName, sizeof(g_pstSCDBHandle->szDBName));
    g_pstSCDBHandle->szDBName[sizeof(g_pstSCDBHandle->szDBName) - 1] = '\0';

    g_pstSCDBHandle->usPort = usDBPort;

    /* �������ݿ� */
    if (db_open(g_pstSCDBHandle) != DOS_SUCC)
    {
        DOS_ASSERT(0);
        db_destroy(&g_pstSCDBHandle);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    SC_TRACE_OUT();
    return DOS_SUCC;
}


U32 mod_dipcc_sc_load()
{
#if 1
    if (py_init_py() != DOS_SUCC)
    {
       DOS_ASSERT(0);
       return DOS_FAIL;
    }
#endif

    if (sc_init_db() != DOS_SUCC)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

#if 1
    if (sc_httpd_init() != DOS_SUCC)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
#endif
    if (sc_task_mngt_init() != DOS_SUCC)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
#if 1
    if (sc_dialer_init() != DOS_SUCC)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
#endif
    if (sc_ep_init() != DOS_SUCC)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (DOS_SUCC != sc_acd_init())
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
#if 1
    if (DOS_SUCC != sc_bs_fsm_init())
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
#endif
    return DOS_SUCC;
}

U32 mod_dipcc_sc_runtime()
{
#if 1
    if (sc_httpd_start() != DOS_SUCC)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;;
    }
#endif
    if (sc_task_mngt_start() != DOS_SUCC)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
#if 1
    if (sc_dialer_start() != DOS_SUCC)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (DOS_SUCC != sc_bs_fsm_start())
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
#endif


    if (sc_ep_start() != DOS_SUCC)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

U32 mod_dipcc_sc_shutdown()
{
    sc_httpd_shutdown();
    sc_task_mngt_shutdown();
    sc_dialer_shutdown();
    py_deinit_py();
    return DOS_SUCC;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */


