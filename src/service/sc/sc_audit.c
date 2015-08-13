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
 * ���������������������:
 *    1. 10�����������
 *    2. 30�����������
 *    3. 60�����������
 *    4. 120�����������
 *    5. 1���������
 *
 *  ��ʼ��ʱ��¼ʱ�����֮��ÿ���ӻ�ȡһ��ʱ�����ʹ�õ�ǰʱ����ͳ�
 *  ʼ����ʱ���֮����������࣬���Ϊ0����Ҫִ�ж�Ӧ������
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */


#include <dos.h>
#include <esl.h>
#include "sc_def.h"
#include "sc_debug.h"
#include "sc_acd_def.h"

typedef enum tagAuditCycel
{
    SC_AUDIT_CYCLE_10MIN  = 10 * 60,
    SC_AUDIT_CYCLE_30MIN  = 30 * 60,
    SC_AUDIT_CYCLE_60MIN  = 60 * 60,
    SC_AUDIT_CYCLE_120MIN = 120 * 60,
    SC_AUDIT_CYCLE_1DAY   = 24 * 60 * 60,
}SC_AUDIT_CYCLE_EN;

typedef struct tagAuditTask
{
    U32 ulCycel;                         /* ����������� */
    U32 (*task)(U32 ulCycle, VOID *ptr); /* ���������� */
    S8  *pszName;                        /* �������� */
}SC_AUDIT_TASK_ST;

static U32 g_ulWaitingStop = DOS_TRUE;
static U32 g_ulStartTimestamp = 0;
static pthread_t g_pthAuditTask;

/* ע��������� */
static SC_AUDIT_TASK_ST g_stAuditTaskList[] =
{
    {SC_AUDIT_CYCLE_60MIN,  sc_acd_agent_audit,         "Agent audit"}
};

static VOID *sc_audit_task(VOID *ptr)
{
    U32 ulCurrentTime = 0;
    U32 i;
    g_ulWaitingStop = DOS_FALSE;

    while (1)
    {
        dos_task_delay(1000);
        if (g_ulWaitingStop)
        {
            break;
        }

        ulCurrentTime = time(NULL);

        for (i=0; i<sizeof(g_stAuditTaskList)/sizeof(SC_AUDIT_TASK_ST); i++)
        {
            if (((g_ulStartTimestamp - ulCurrentTime) / g_stAuditTaskList[i].ulCycel))
            {
                continue;
            }

            sc_logr_notice(SC_AUDIT, "Start audit. Current Timestamp:%u, Cycle: %u, Task: %s(Handle:%P)"
                    , ulCurrentTime
                    , g_stAuditTaskList[i].ulCycel
                    , g_stAuditTaskList[i].pszName
                    , g_stAuditTaskList[i].task);
            if (g_stAuditTaskList[i].task)
            {
                g_stAuditTaskList[i].task(g_stAuditTaskList[i].ulCycel, NULL);
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


