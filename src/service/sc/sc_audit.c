/**
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  �ļ���: sc_audit.h
 *
 *  ����ʱ��: 2015��8��6��10:03:57
 *  ��    ��: Larry
 *  ��    ��: ҵ�����ģ�������ع��ܼ���
 *  �޸���ʷ:
 */

/*
 * ���·�Ϊ�����࣬��Ȼʱ��ִ�У�����ִ�С�
 * ����ִ�������������������:
 *    1. 10�����������
 *    2. 30�����������
 *    3. 60�����������
 *    4. 120�����������
 *    5. 1���������
 *  ��ʼ��ʱ��¼ʱ�����֮��ÿ���ӻ�ȡһ��ʱ�����ʹ�õ�ǰʱ����ͳ�
 *  ʼ����ʱ���֮����������࣬���Ϊ0����Ҫִ�ж�Ӧ������
 *
 * ��Ȼ�¼�ִ������:
 *    ������ʱִ�У���Ҫע��ƫ����
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */


#include <dos.h>
#include <esl.h>
#include <time.h>
#include "sc_def.h"
#include "sc_debug.h"
#include "sc_acd_def.h"

#define SC_AUDIT_TEST 0

typedef enum tagAuditType{
    SC_AUDIT_TYPE_ONCE  = 0,  /* ѭ��ִ�У���ϵͳ����˳��Ϊ׼��ƫ����Ϊs */
    SC_AUDIT_TYPE_CYCLE,      /* ѭ��ִ�У���ϵͳ����˳��Ϊ׼��ƫ������Ϊ���ڳ��� */
    SC_AUDIT_TYPE_N_HOUR,     /* ��ȻСʱ��ƫ����Ϊ���� 0-59 */
    SC_AUDIT_TYPE_N_2HOUR,    /* ��Ȼ��Сʱ��ƫ����Ϊ���� 0-199 */
    SC_AUDIT_TYPE_N_DAY,      /* ��Ȼ�죬ƫ����ΪСʱ 0-23 */
    SC_AUDIT_TYPE_N_WEEK,     /* ��Ȼ�ܣ�ƫ����Ϊ�� 0-6 */
    SC_AUDIT_TYPE_N_MONTH,    /* ��Ȼ�£�ƫ����Ϊ�� 0-31 */
    SC_AUDIT_TYPE_N_YEAR,     /* ��Ȼ�꣬ƫ����Ϊ�� 0-365 */
}SC_AUDIT_TYPE_EN;

typedef enum tagAuditCycle
{
    SC_AUDIT_CYCLE_10MIN  = 10 * 60,
    SC_AUDIT_CYCLE_30MIN  = 30 * 60,
    SC_AUDIT_CYCLE_60MIN  = 60 * 60,
    SC_AUDIT_CYCLE_120MIN = 120 * 60,
    SC_AUDIT_CYCLE_1DAY   = 24 * 60 * 60,
}SC_AUDIT_CYCLE_EN;

typedef struct tagAuditTask
{
    /* ���� refer SC_AUDIT_TYPE_EN */
    U32 ulType;

    /* ʱ��ƫ��,���typeΪSC_AUDIT_TYPE_CYCLE���������ڣ�������Ϊƫ����
        ������ƫ���������������: ƫ�����ĵ�λ�����͵ĵ�λС1�� ��:
        typeΪSC_AUDIT_TYPE_N_HOUR����ƫ����Ϊ0-59�룬���ڷ�Χ��ȷ���ģ���
        ���£����£�С�£���ƫ����Ϊ��Сֵ*/
    U32 ulOffset;

    /* ���������� */
    U32 (*task)(U32 ulType, VOID *ptr);

    /* �������� */
    S8  *pszName;
}SC_AUDIT_TASK_ST;

static U32 g_ulWaitingStop = DOS_TRUE;
static U32 g_ulStartTimestamp = 0;
static pthread_t g_pthAuditTask;

#if SC_AUDIT_TEST
U32 sc_test_startup(U32 ulType, VOID *ptr)
{
    sc_logr_notice(SC_AUDIT, "Test for startup. Startup timestamp: %u", g_ulStartTimestamp);

    return DOS_SUCC;
}
U32 sc_test_startup_delay(U32 ulType, VOID *ptr)
{
    sc_logr_notice(SC_AUDIT, "Test for startup delay. Startup timestamp: %u, Current: %u", g_ulStartTimestamp, time(NULL));

    return DOS_SUCC;
}

U32 sc_test_cycle(U32 ulType, VOID *ptr)
{
    sc_logr_notice(SC_AUDIT, "Test for startup cycle. Startup timestamp: %u, Current: %u", g_ulStartTimestamp, time(NULL));

    return DOS_SUCC;
}

U32 sc_test_hour(U32 ulType, VOID *ptr)
{
    sc_logr_notice(SC_AUDIT, "Test hour. Startup timestamp: %u, Current: %u", g_ulStartTimestamp, time(NULL));

    return DOS_SUCC;
}

U32 sc_test_hour_delay(U32 ulType, VOID *ptr)
{
    sc_logr_notice(SC_AUDIT, "Test hour delay. Startup timestamp: %u, Current: %u", g_ulStartTimestamp, time(NULL));

    return DOS_SUCC;
}
#endif


/* ע��������� */
static SC_AUDIT_TASK_ST g_stAuditTaskList[] =
{
#if SC_AUDIT_TEST
    {SC_AUDIT_TYPE_ONCE,     0,                     sc_test_startup,            "Audit test for once"},
    {SC_AUDIT_TYPE_ONCE,     20,                    sc_test_startup_delay,      "Audit test for delay"},
    {SC_AUDIT_TYPE_CYCLE,    SC_AUDIT_CYCLE_10MIN,  sc_test_cycle,              "Audit test for cycle"},
    {SC_AUDIT_TYPE_N_HOUR,   0,                     sc_test_hour,               "Audit test for hour"},
    {SC_AUDIT_TYPE_N_HOUR,   1,                     sc_test_hour_delay,         "Audit test for hour delay"},
#endif
    {SC_AUDIT_TYPE_CYCLE,    SC_AUDIT_CYCLE_60MIN,  sc_acd_agent_audit,         "Agent audit"},
    {SC_AUDIT_TYPE_N_HOUR,   0,                     sc_num_lmt_stat,            "Number usage stat"},
    {SC_AUDIT_TYPE_N_DAY,    0,                     sc_num_lmt_update,          "Number limitation"}
};


static VOID *sc_audit_task(VOID *ptr)
{
    time_t ulCurrentTime = 0;
    time_t ulLastTime    = 0;
    U32 i, j;
    struct tm stTime;
    SC_AUDIT_TASK_ST *pstAuditTask = NULL;

    g_ulWaitingStop = DOS_FALSE;

    while (1)
    {
        dos_task_delay(1000);
        if (g_ulWaitingStop)
        {
            break;
        }

        dos_memzero(&stTime, sizeof(stTime));

        ulCurrentTime = time(NULL);
        localtime_r((time_t *)&ulCurrentTime, &stTime);

        if (0 == g_ulStartTimestamp)
        {
            g_ulStartTimestamp = ulCurrentTime;
            ulLastTime = ulCurrentTime - 1;
        }

        /* ���ܳ���ʱ�Ӳ�ͬ�������λ�ȡ��ʱ���֮�����1�ˣ�����Ҫ�����֮���ÿһ�� */
        for (j=ulLastTime+1; j<=ulCurrentTime; j++)
        {
            /* ������ж�ʱ���� */
            for (i=0; i<sizeof(g_stAuditTaskList)/sizeof(SC_AUDIT_TASK_ST); i++)
            {
                pstAuditTask = NULL;

                switch (g_stAuditTaskList[i].ulType)
                {
                    case SC_AUDIT_TYPE_ONCE:
                        if (g_stAuditTaskList[i].ulOffset + g_ulStartTimestamp == ulCurrentTime)
                        {
                            pstAuditTask = &g_stAuditTaskList[i];
                        }
                        break;

                    case SC_AUDIT_TYPE_CYCLE:
                        if (((ulCurrentTime - g_ulStartTimestamp) % g_stAuditTaskList[i].ulOffset)
                            || (g_ulStartTimestamp == ulCurrentTime))
                        {
                            break;
                        }

                        pstAuditTask = &g_stAuditTaskList[i];
                        break;

                    case SC_AUDIT_TYPE_N_HOUR:
                        if (g_stAuditTaskList[i].ulOffset >= 59)
                        {
                            if (0 == stTime.tm_sec && 0 == stTime.tm_min)
                            {
                                pstAuditTask = &g_stAuditTaskList[i];
                            }
                        }
                        else
                        {
                            if (0 == stTime.tm_sec && stTime.tm_min == g_stAuditTaskList[i].ulOffset)
                            {
                                pstAuditTask = &g_stAuditTaskList[i];
                            }
                        }
                        break;

                    case SC_AUDIT_TYPE_N_2HOUR:
                        if (g_stAuditTaskList[i].ulOffset >= 119)
                        {
                            if (0 == stTime.tm_sec && 0 == stTime.tm_min && 0 == stTime.tm_hour%2)
                            {
                                pstAuditTask = &g_stAuditTaskList[i];
                            }
                        }
                        else
                        {
                            if (g_stAuditTaskList[i].ulOffset >= 60)
                            {
                                if (0 == stTime.tm_sec
                                    && stTime.tm_hour%2
                                    && stTime.tm_min == g_stAuditTaskList[i].ulOffset - 60)
                                {
                                    pstAuditTask = &g_stAuditTaskList[i];
                                }
                            }
                            else
                            {
                                if (0 == stTime.tm_sec
                                    && 0 == stTime.tm_hour%2
                                    && stTime.tm_min == g_stAuditTaskList[i].ulOffset)
                                {
                                    pstAuditTask = &g_stAuditTaskList[i];
                                }
                            }
                        }
                        break;

                    case SC_AUDIT_TYPE_N_DAY:
                        if (g_stAuditTaskList[i].ulOffset >= 23)
                        {
                            if (0 == stTime.tm_sec
                                && 0 == stTime.tm_min
                                && 0 == stTime.tm_hour)
                            {
                                pstAuditTask = &g_stAuditTaskList[i];
                            }
                        }
                        else
                        {
                            if (0 == stTime.tm_sec
                                && 0 == stTime.tm_min
                                && stTime.tm_hour == g_stAuditTaskList[i].ulOffset)
                            {
                                pstAuditTask = &g_stAuditTaskList[i];
                            }
                        }
                        break;

                    case SC_AUDIT_TYPE_N_WEEK:
                        if (g_stAuditTaskList[i].ulOffset >= 6)
                        {
                            if (0 == stTime.tm_sec
                                && 0 == stTime.tm_min
                                && 0 == stTime.tm_hour
                                && 0 == stTime.tm_wday)
                            {
                                pstAuditTask = &g_stAuditTaskList[i];
                            }
                        }
                        else
                        {
                            if (0 == stTime.tm_sec
                                && 0 == stTime.tm_min
                                && 0 == stTime.tm_hour
                                && stTime.tm_wday == g_stAuditTaskList[i].ulOffset)
                            {
                                pstAuditTask = &g_stAuditTaskList[i];
                            }
                        }
                        break;

                    case SC_AUDIT_TYPE_N_MONTH:
                        if (g_stAuditTaskList[i].ulOffset >= 28)
                        {
                            if (0 == stTime.tm_sec
                                && 0 == stTime.tm_min
                                && 0 == stTime.tm_hour
                                && 0 == stTime.tm_mday)
                            {
                                pstAuditTask = &g_stAuditTaskList[i];
                            }
                        }
                        else
                        {
                            if (0 == stTime.tm_sec
                                && 0 == stTime.tm_min
                                && 0 == stTime.tm_hour
                                && stTime.tm_mday == g_stAuditTaskList[i].ulOffset)
                            {
                                pstAuditTask = &g_stAuditTaskList[i];
                            }
                        }
                        break;

                    case SC_AUDIT_TYPE_N_YEAR:
                        if (g_stAuditTaskList[i].ulOffset >= 365)
                        {
                            if (0 == stTime.tm_sec
                                && 0 == stTime.tm_min
                                && 0 == stTime.tm_hour
                                && 0 == stTime.tm_yday)
                            {
                                pstAuditTask = &g_stAuditTaskList[i];
                            }
                        }
                        else
                        {
                            if (0 == stTime.tm_sec
                                && 0 == stTime.tm_min
                                && 0 == stTime.tm_hour
                                && stTime.tm_yday == g_stAuditTaskList[i].ulOffset)
                            {
                                pstAuditTask = &g_stAuditTaskList[i];
                            }
                        }
                        break;

                    default:
                        DOS_ASSERT(0);
                        break;
                }

                if (pstAuditTask)
                {
                    sc_logr_notice(SC_AUDIT, "Start audit. Current Timestamp:%u, Type: %u Offset: %u, Task: %s(Handle:%p)"
                                        , ulCurrentTime
                                        , pstAuditTask->ulType
                                        , pstAuditTask->ulOffset
                                        , pstAuditTask->pszName
                                        , pstAuditTask->task);

                    if (pstAuditTask->task)
                    {
                        pstAuditTask->task(pstAuditTask->ulType, NULL);
                    }
                }
            }
        }

        ulLastTime = ulCurrentTime;
    }

    return 0;
}

U32 sc_audit_init()
{
    g_ulWaitingStop = DOS_TRUE;

    return DOS_SUCC;
}

U32 sc_audit_start()
{
    if (pthread_create(&g_pthAuditTask, NULL, sc_audit_task, NULL) < 0)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

U32 sc_audit_stop()
{
    g_ulWaitingStop = DOS_TRUE;

    return DOS_SUCC;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */


