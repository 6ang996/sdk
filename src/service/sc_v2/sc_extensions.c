/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  �ļ���: sc_ep_extensions.c
 *
 *  ����ʱ��: 2015��6��18��15:25:18
 *  ��    ��: Larry
 *  ��    ��: ����FS���ķ������ĸ����¼�
 *  �޸���ʷ:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>
#include <esl.h>
#include <sys/time.h>
#include <pthread.h>
#include <bs_pub.h>
#include <libcurl/curl.h>
#include "sc_def.h"
#include "sc_db.h"
#include "sc_res.h"
#include "sc_debug.h"

DLL_S            g_stExtMngtMsg;
pthread_mutex_t  g_mutexExtMngtMsg = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t   g_condExtMngtMsg  = PTHREAD_COND_INITIALIZER;
pthread_t        g_pthExtMngtProcTask;


VOID* sc_ext_mgnt_runtime(VOID *ptr)
{
    struct timespec         stTimeout;

    for (;;)
    {
        pthread_mutex_lock(&g_mutexExtMngtMsg);
        stTimeout.tv_sec = time(0) + 1;
        stTimeout.tv_nsec = 0;
        pthread_cond_timedwait(&g_condExtMngtMsg, &g_mutexExtMngtMsg, &stTimeout);
        pthread_mutex_unlock(&g_mutexExtMngtMsg);
    }
}


/* ��ʼ���¼�����ģ�� */
U32 sc_ext_mngt_init()
{
    return DOS_SUCC;
}

/* �����¼�����ģ�� */
U32 sc_ext_mngt_start()
{
    return DOS_SUCC;
}

/* ֹͣ�¼�����ģ�� */
U32 sc_ext_mngt_stop()
{
    return DOS_SUCC;
}


#ifdef __cplusplus
}
#endif /* __cplusplus */


