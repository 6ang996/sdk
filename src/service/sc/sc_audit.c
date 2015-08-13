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
#include "sc_def.h"
#include "sc_debug.h"
#include "sc_acd_def.h"

typedef enum tagAuditType{
    SC_AUDIT_TYPE_CYCLE = 0,  /* ѭ��ִ�У���ϵͳ����˳��Ϊ׼ */
    SC_AUDIT_TYPE_N_HOUR,     /* ��ȻСʱ */
    SC_AUDIT_TYPE_N_2HOUR,    /* ��Ȼ��Сʱ */
    SC_AUDIT_TYPE_N_DAY,      /* ��Ȼ�� */
    SC_AUDIT_TYPE_N_WEEK,     /* ��Ȼ�� */
    SC_AUDIT_TYPE_N_MONTH,    /* ��Ȼ�� */
    SC_AUDIT_TYPE_N_YEAR,     /* ��Ȼ�� */
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

/* ע��������� */
static SC_AUDIT_TASK_ST g_stAuditTaskList[] =
{
    {SC_AUDIT_TYPE_CYCLE,    SC_AUDIT_CYCLE_60MIN,  sc_acd_agent_audit,         "Agent audit"}
};

static VOID *sc_audit_task(VOID *ptr)
{
    U32 ulCurrentTime = 0;
    U32 i;
    struct tm *t = NULL;
    SC_AUDIT_TASK_ST *pstAuditTask = NULL;

    g_ulWaitingStop = DOS_FALSE;

    while (1)
    {
        dos_task_delay(1000);
        if (g_ulWaitingStop)
        {
            break;
        }

        ulCurrentTime = time(NULL);
        t = localtime((time_t *)&ulCurrentTime);
        pstAuditTask = NULL;

        for (i=0; i<sizeof(g_stAuditTaskList)/sizeof(SC_AUDIT_TASK_ST); i++)
        {
            switch (g_stAuditTaskList[i].ulType)
            {
                case SC_AUDIT_TYPE_CYCLE:
                    if (((g_ulStartTimestamp - ulCurrentTime) / g_stAuditTaskList[i].ulOffset))
                    {
                        break;
                    }

                    pstAuditTask = &g_stAuditTaskList[i];
                    break;

                case SC_AUDIT_TYPE_N_HOUR:
                    if (g_stAuditTaskList[i].ulOffset >= 59)
                    {
                        if (0 == t->tm_min)
                        {
                            pstAuditTask = &g_stAuditTaskList[i];
                        }
                    }
                    else
                    {
                        if (t->tm_min == g_stAuditTaskList[i].ulOffset)
                        {
                            pstAuditTask = &g_stAuditTaskList[i];
                        }
                    }
                    break;

                case SC_AUDIT_TYPE_N_2HOUR:
                    if (g_stAuditTaskList[i].ulOffset >= 119)
                    {
                        if (0 == t->tm_min && 0 == t->tm_hour/2)
                        {
                            pstAuditTask = &g_stAuditTaskList[i];
                        }
                    }
                    else
                    {
                        if (g_stAuditTaskList[i].ulOffset >= 60)
                        {
                            if (t->tm_hour/2
                                && t->tm_min == g_stAuditTaskList[i].ulOffset - 60)
                            {
                                pstAuditTask = &g_stAuditTaskList[i];
                            }
                        }
                        else
                        {
                            if (0 == t->tm_hour/2
                                && t->tm_min == g_stAuditTaskList[i].ulOffset)
                            {
                                pstAuditTask = &g_stAuditTaskList[i];
                            }
                        }
                    }
                    break;

                case SC_AUDIT_TYPE_N_DAY:
                    if (g_stAuditTaskList[i].ulOffset >= 23)
                    {
                        if (0 == t->tm_hour)
                        {
                            pstAuditTask = &g_stAuditTaskList[i];
                        }
                    }
                    else
                    {
                        if (t->tm_hour == g_stAuditTaskList[i].ulOffset)
                        {
                            pstAuditTask = &g_stAuditTaskList[i];
                        }
                    }
                    break;

                case SC_AUDIT_TYPE_N_WEEK:
                    if (g_stAuditTaskList[i].ulOffset >= 6)
                    {
                        if (0 == t->tm_wday)
                        {
                            pstAuditTask = &g_stAuditTaskList[i];
                        }
                    }
                    else
                    {
                        if (t->tm_wday == g_stAuditTaskList[i].ulOffset)
                        {
                            pstAuditTask = &g_stAuditTaskList[i];
                        }
                    }
                    break;

                case SC_AUDIT_TYPE_N_MONTH:
                    if (g_stAuditTaskList[i].ulOffset >= 28)
                    {
                        if (0 == t->tm_mday)
                        {
                            pstAuditTask = &g_stAuditTaskList[i];
                        }
                    }
                    else
                    {
                        if (t->tm_mday == g_stAuditTaskList[i].ulOffset)
                        {
                            pstAuditTask = &g_stAuditTaskList[i];
                        }
                    }
                    break;

                case SC_AUDIT_TYPE_N_YEAR:
                    if (g_stAuditTaskList[i].ulOffset >= 365)
                    {
                        if (0 == t->tm_yday)
                        {
                            pstAuditTask = &g_stAuditTaskList[i];
                        }
                    }
                    else
                    {
                        if (t->tm_yday == g_stAuditTaskList[i].ulOffset)
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
                sc_logr_notice(SC_AUDIT, "Start audit. Current Timestamp:%u, Type: %u Offset: %u, Task: %s(Handle:%P)"
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

    return 0;
}

U32 sc_audit_init()
{
    g_ulWaitingStop = DOS_TRUE;
    g_ulStartTimestamp = time(NULL);

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


