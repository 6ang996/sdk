/**
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  �ļ���: sc_schedule_task.c
 *
 *  ����ʱ��: 2015��8��6��10:03:57
 *  ��    ��: Larry
 *  ��    ��: ҵ�����ģ�������ع��ܼ���
 *  �޸���ʷ:
 */

/**
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
#include <time.h>
#include "sc_def.h"
#include "sc_debug.h"

/** �����Ƿ��ڲ�����ȥ���� */
#define SC_SCHEDULE_TEST 0

/** ����������������� */
typedef enum tagScheduleType{
    SC_SCHEDULE_TYPE_ONCE  = 0,  /**< ѭ��ִ�У���ϵͳ����˳��Ϊ׼��ƫ����Ϊs */
    SC_SCHEDULE_TYPE_CYCLE,      /**< ѭ��ִ�У���ϵͳ����˳��Ϊ׼��ƫ������Ϊ���ڳ��� */
    SC_SCHEDULE_TYPE_N_HOUR,     /**< ��ȻСʱ��ƫ����Ϊ���� 0-59 */
    SC_SCHEDULE_TYPE_N_2HOUR,    /**< ��Ȼ��Сʱ��ƫ����Ϊ���� 0-199 */
    SC_SCHEDULE_TYPE_N_DAY,      /**< ��Ȼ�죬ƫ����ΪСʱ 0-23 */
    SC_SCHEDULE_TYPE_N_WEEK,     /**< ��Ȼ�ܣ�ƫ����Ϊ�� 0-6 */
    SC_SCHEDULE_TYPE_N_MONTH,    /**< ��Ȼ�£�ƫ����Ϊ�� 0-31 */
    SC_SCHEDULE_TYPE_N_YEAR,     /**< ��Ȼ�꣬ƫ����Ϊ�� 0-365 */
}SC_AUDIT_TYPE_EN;

/** ����һЩ����ʱ�� */
typedef enum tagScheduleCycle
{
    SC_SCHEDULE_CYCLE_5MIN   = 5 * 60,
    SC_SCHEDULE_CYCLE_10MIN  = 10 * 60,
    SC_SCHEDULE_CYCLE_30MIN  = 30 * 60,
    SC_SCHEDULE_CYCLE_60MIN  = 60 * 60,
    SC_SCHEDULE_CYCLE_120MIN = 120 * 60,
    SC_SCHEDULE_CYCLE_1DAY   = 24 * 60 * 60,
}SC_SCHEDULE_CYCLE_EN;

typedef struct tagScheduleTask
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
}SC_SCHEDULE_TASK_ST;

static U32 g_ulWaitingStop = DOS_TRUE;
static U32 g_ulStartTimestamp = 0;
static pthread_t g_pthAuditTask;

#if SC_SCHEDULE_TEST
U32 sc_test_startup(U32 ulType, VOID *ptr)
{
    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_SCH), "Test for startup. Startup timestamp: %u", g_ulStartTimestamp);

    return DOS_SUCC;
}
U32 sc_test_startup_delay(U32 ulType, VOID *ptr)
{
    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_SCH), "Test for startup delay. Startup timestamp: %u, Current: %u", g_ulStartTimestamp, time(NULL));

    return DOS_SUCC;
}

U32 sc_test_cycle(U32 ulType, VOID *ptr)
{
    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_SCH), "Test for startup cycle. Startup timestamp: %u, Current: %u", g_ulStartTimestamp, time(NULL));

    return DOS_SUCC;
}

U32 sc_test_hour(U32 ulType, VOID *ptr)
{
    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_SCH), "Test hour. Startup timestamp: %u, Current: %u", g_ulStartTimestamp, time(NULL));

    return DOS_SUCC;
}

U32 sc_test_hour_delay(U32 ulType, VOID *ptr)
{
    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_SCH), "Test hour delay. Startup timestamp: %u, Current: %u", g_ulStartTimestamp, time(NULL));

    return DOS_SUCC;
}
#endif

U32 sc_num_lmt_stat(U32 ulType, VOID *ptr);
U32 sc_num_lmt_update(U32 ulType, VOID *ptr);
U32 sc_stat_write(U32 ulType, VOID *ptr);
U32 sc_stat_syn(U32 ulType, VOID *ptr);
U32 sc_task_write_stat(U32 ulType, VOID *ptr);


/* ע��������� */
static SC_SCHEDULE_TASK_ST g_stScheduleTaskList[] =
{
#if SC_SCHEDULE_TEST
    {SC_SCHEDULE_TYPE_ONCE,     0,                        sc_test_startup,            "Schedule test for once"},
    {SC_SCHEDULE_TYPE_ONCE,     20,                       sc_test_startup_delay,      "Schedule test for delay"},
    {SC_SCHEDULE_TYPE_CYCLE,    SC_SCHEDULE_CYCLE_10MIN,  sc_test_cycle,              "Schedule test for cycle"},
    {SC_SCHEDULE_TYPE_N_HOUR,   0,                        sc_test_hour,               "Schedule test for hour"},
    {SC_SCHEDULE_TYPE_N_HOUR,   1,                        sc_test_hour_delay,         "Schedule test for hour delay"},
#endif
    {SC_SCHEDULE_TYPE_N_DAY,    1,                        sc_num_lmt_stat,            "Number usage stat"},
    {SC_SCHEDULE_TYPE_N_DAY,    1,                        sc_num_lmt_update,          "Number limitation"},

    /* 6Сʱдͳ�����ݵ����� */
    {SC_SCHEDULE_TYPE_CYCLE,    6 * 60 * 60,              sc_stat_write,              "Write stat info"},

    /* 20����ͬ�����ݵ�licenseģ�� */
    {SC_SCHEDULE_TYPE_CYCLE,    20 * 60,                  sc_stat_syn,                "Stat data syn "},

    {SC_SCHEDULE_TYPE_CYCLE,    60,                       sc_task_write_stat,         "Write task stat"},
};


static VOID *sc_schedule_task(VOID *ptr)
{
    time_t ulCurrentTime = 0;
    time_t ulLastTime    = 0;
    U32 i, j;
    struct tm stTime;
    SC_SCHEDULE_TASK_ST *pstScheduleTask = NULL;

    g_ulWaitingStop = DOS_FALSE;

    while (1)
    {
        dos_task_delay(1000);
        if (g_ulWaitingStop)
        {
            break;
        }

        ulCurrentTime = time(NULL);

        if (0 == g_ulStartTimestamp)
        {
            g_ulStartTimestamp = ulCurrentTime;
            ulLastTime = ulCurrentTime - 1;
        }

        /* ���ܳ���ʱ�Ӳ�ͬ�������λ�ȡ��ʱ���֮�����1�ˣ�����Ҫ�����֮���ÿһ�� */
        for (j=ulLastTime+1; j<=ulCurrentTime; j++)
        {
            /* ���ݵ�ǰʱ�������ȡʱ�� */
            dos_memzero(&stTime, sizeof(stTime));
            localtime_r((time_t *)&j, &stTime);

            /* ������ж�ʱ���� */
            for (i=0; i<sizeof(g_stScheduleTaskList)/sizeof(SC_SCHEDULE_TASK_ST); i++)
            {
                pstScheduleTask = NULL;

                switch (g_stScheduleTaskList[i].ulType)
                {
                    case SC_SCHEDULE_TYPE_ONCE:
                        if (g_stScheduleTaskList[i].ulOffset + g_ulStartTimestamp == ulCurrentTime)
                        {
                            pstScheduleTask = &g_stScheduleTaskList[i];
                        }
                        break;

                    case SC_SCHEDULE_TYPE_CYCLE:
                        if (((ulCurrentTime - g_ulStartTimestamp) % g_stScheduleTaskList[i].ulOffset)
                            || (g_ulStartTimestamp == ulCurrentTime))
                        {
                            break;
                        }

                        pstScheduleTask = &g_stScheduleTaskList[i];
                        break;

                    case SC_SCHEDULE_TYPE_N_HOUR:
                        if (g_stScheduleTaskList[i].ulOffset >= 59)
                        {
                            if (0 == stTime.tm_sec && 0 == stTime.tm_min)
                            {
                                pstScheduleTask = &g_stScheduleTaskList[i];
                            }
                        }
                        else
                        {
                            if (0 == stTime.tm_sec && stTime.tm_min == g_stScheduleTaskList[i].ulOffset)
                            {
                                pstScheduleTask = &g_stScheduleTaskList[i];
                            }
                        }
                        break;

                    case SC_SCHEDULE_TYPE_N_2HOUR:
                        if (g_stScheduleTaskList[i].ulOffset >= 119)
                        {
                            if (0 == stTime.tm_sec && 0 == stTime.tm_min && 0 == stTime.tm_hour%2)
                            {
                                pstScheduleTask = &g_stScheduleTaskList[i];
                            }
                        }
                        else
                        {
                            if (g_stScheduleTaskList[i].ulOffset >= 60)
                            {
                                if (0 == stTime.tm_sec
                                    && stTime.tm_hour%2
                                    && stTime.tm_min == g_stScheduleTaskList[i].ulOffset - 60)
                                {
                                    pstScheduleTask = &g_stScheduleTaskList[i];
                                }
                            }
                            else
                            {
                                if (0 == stTime.tm_sec
                                    && 0 == stTime.tm_hour%2
                                    && stTime.tm_min == g_stScheduleTaskList[i].ulOffset)
                                {
                                    pstScheduleTask = &g_stScheduleTaskList[i];
                                }
                            }
                        }
                        break;

                    case SC_SCHEDULE_TYPE_N_DAY:
                        if (g_stScheduleTaskList[i].ulOffset >= 23)
                        {
                            if (0 == stTime.tm_sec
                                && 0 == stTime.tm_min
                                && 0 == stTime.tm_hour)
                            {
                                pstScheduleTask = &g_stScheduleTaskList[i];
                            }
                        }
                        else
                        {
                            if (0 == stTime.tm_sec
                                && 0 == stTime.tm_min
                                && stTime.tm_hour == g_stScheduleTaskList[i].ulOffset)
                            {
                                pstScheduleTask = &g_stScheduleTaskList[i];
                            }
                        }
                        break;

                    case SC_SCHEDULE_TYPE_N_WEEK:
                        if (g_stScheduleTaskList[i].ulOffset >= 6)
                        {
                            if (0 == stTime.tm_sec
                                && 0 == stTime.tm_min
                                && 0 == stTime.tm_hour
                                && 0 == stTime.tm_wday)
                            {
                                pstScheduleTask = &g_stScheduleTaskList[i];
                            }
                        }
                        else
                        {
                            if (0 == stTime.tm_sec
                                && 0 == stTime.tm_min
                                && 0 == stTime.tm_hour
                                && stTime.tm_wday == g_stScheduleTaskList[i].ulOffset)
                            {
                                pstScheduleTask = &g_stScheduleTaskList[i];
                            }
                        }
                        break;

                    case SC_SCHEDULE_TYPE_N_MONTH:
                        if (g_stScheduleTaskList[i].ulOffset >= 28)
                        {
                            if (0 == stTime.tm_sec
                                && 0 == stTime.tm_min
                                && 0 == stTime.tm_hour
                                && 0 == stTime.tm_mday)
                            {
                                pstScheduleTask = &g_stScheduleTaskList[i];
                            }
                        }
                        else
                        {
                            if (0 == stTime.tm_sec
                                && 0 == stTime.tm_min
                                && 0 == stTime.tm_hour
                                && stTime.tm_mday == g_stScheduleTaskList[i].ulOffset)
                            {
                                pstScheduleTask = &g_stScheduleTaskList[i];
                            }
                        }
                        break;

                    case SC_SCHEDULE_TYPE_N_YEAR:
                        if (g_stScheduleTaskList[i].ulOffset >= 365)
                        {
                            if (0 == stTime.tm_sec
                                && 0 == stTime.tm_min
                                && 0 == stTime.tm_hour
                                && 0 == stTime.tm_yday)
                            {
                                pstScheduleTask = &g_stScheduleTaskList[i];
                            }
                        }
                        else
                        {
                            if (0 == stTime.tm_sec
                                && 0 == stTime.tm_min
                                && 0 == stTime.tm_hour
                                && stTime.tm_yday == g_stScheduleTaskList[i].ulOffset)
                            {
                                pstScheduleTask = &g_stScheduleTaskList[i];
                            }
                        }
                        break;

                    default:
                        DOS_ASSERT(0);
                        break;
                }

                if (pstScheduleTask)
                {
                    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_SCH)
                                , "Start audit. Current Timestamp:%u, Type: %u Offset: %u, Task: %s(Handle:%p)"
                                , ulCurrentTime
                                , pstScheduleTask->ulType
                                , pstScheduleTask->ulOffset
                                , pstScheduleTask->pszName
                                , pstScheduleTask->task);

                    if (pstScheduleTask->task)
                    {
                        pstScheduleTask->task(pstScheduleTask->ulType, NULL);
                    }
                }
            }
        }

        ulLastTime = ulCurrentTime;
    }

    return 0;
}

U32 sc_schedule_init()
{
    g_ulWaitingStop = DOS_TRUE;

    return DOS_SUCC;
}

U32 sc_schedule_start()
{
    if (pthread_create(&g_pthAuditTask, NULL, sc_schedule_task, NULL) < 0)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

U32 sc_schedule_stop()
{
    g_ulWaitingStop = DOS_TRUE;

    return DOS_SUCC;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */


