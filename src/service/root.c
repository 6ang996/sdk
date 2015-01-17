/*
 *            (C) Copyright 2014, 天天讯通 . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  root.c
 *
 *  Created on: 2014-11-3
 *      Author: Larry
 *        Todo: 启动该进程的主服务程序
 *     History:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>

#if INCLUDE_PTC
    #include <ptc_pts/ipcc.h>
#endif


S32 root(S32 _argc, S8 ** _argv)
{
#if INCLUDE_DEBUG_CLI_SERVER
    telnetd_start();
#elif INCLUDE_BH_SERVER

    if (heartbeat_init())
    {
        DOS_ASSERT(0);
        exit(0);
    }
    dos_printf("%s", "Heartbeat init successfully.");

    heartbeat_start();
#endif

#if INCLUDE_PTC
    ptc_main();
#endif

#if INCLUDE_SERVICE_BS
    if (bs_start() != DOS_SUCC)
    {
        DOS_ASSERT(0);
        return -1;
    }
#endif
    while(1)
    {
        sleep(1);
    }

    return 0;
}


#ifdef __cplusplus
}
#endif /* __cplusplus */

