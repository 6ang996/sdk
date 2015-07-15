/**
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  �ļ���: sc_sysstat.c
 *
 *  ����ʱ��: 2015��6��29��17:28:24
 *  ��    ��: Larry
 *  ��    ��: ��ȡϵͳ��Ϣ���ļ�
 *  �޸���ʷ:
 */
#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>

#if (DOS_INCLUDE_SYS_STAT)

/* CPU��ؼ��, 1000���� */
#define SYSSTAT_CPU_MONITORD_INTERVAL   100

/* CPU��ؾ���, 20���� */
#define SYSSTAT_CPU_MONITORD_ACCURACY    500

#define SYSSTAT_CPU_IDEL_SOMMTH_DEPTH    30

#define SYSSTAT_FILE_PATH                "/proc/stat"


typedef struct tagCPUIDPercentage{
    U64   ulArvg;
    U64   ulRecordCnt;
    U64   uLUser;
    U64   uLNice;
    U64   uLSystem;
    U64   uLIdle;
    U64   uLIOWait;
    U64   uLIrq;
    U64   uLSoftIrq;
    U64   uLSteal;
    U64   uLGuest;
    U64   aulHistory[SYSSTAT_CPU_IDEL_SOMMTH_DEPTH];
}DOS_CPU_IDEL_HANDLE_ST;

/* CPUռ���� 1/10000 */
DOS_CPU_IDEL_HANDLE_ST  *g_pstIdelHandle = NULL;
U32       g_ulSYSCPUIdelPercentage    = 10000;
BOOL      g_blSYSStatEnable           = DOS_TRUE;
pthread_t g_pthSYSSTSTCpu;


static VOID * dos_sysstat_cpu_monitor(VOID *ptr)
{
    FILE *pStatFile  = NULL;
    S8   szLine[512];
    S32  lRet, lInd;
    U64  uLUser1, uLNice1, uLSystem1, uLIdle1, uLIOWait1, uLIrq1, uLSoftIrq1, uLSteal1, uLGuest1;
    U64  uLUser, uLKernel, uLIdel, uLTotal;
    BOOL blFirstTime = DOS_TRUE;

    if (!g_pstIdelHandle)
    {
        return NULL;
    }

    if (g_pstIdelHandle->ulRecordCnt != 0)
    {
        g_pstIdelHandle->ulRecordCnt = 0;
    }
    g_pstIdelHandle->aulHistory[g_pstIdelHandle->ulRecordCnt++] = 10000;

    while (1)
    {
        dos_task_delay(SYSSTAT_CPU_MONITORD_INTERVAL);

        if (DOS_ADDR_INVALID(pStatFile))
        {
            pStatFile = fopen(SYSSTAT_FILE_PATH, "r");
            if (DOS_ADDR_INVALID(pStatFile))
            {
                continue;
            }
        }

        fflush(pStatFile);
        fseek(pStatFile, 0, SEEK_SET);
        fgets(szLine, sizeof(szLine), pStatFile);
        szLine[sizeof(szLine) - 1] = '\0';
        lRet = dos_sscanf(szLine, "cpu  %llu %llu %llu %llu %llu %llu %llu %llu %llu"
                    , &uLUser1, &uLNice1, &uLSystem1, &uLIdle1, &uLIOWait1, &uLIrq1, &uLSoftIrq1, &uLSteal1, &uLGuest1);
        if (lRet != 9)
        {
            continue;
        }

        uLUser = (uLUser1 - g_pstIdelHandle->uLUser) + uLNice1 - g_pstIdelHandle->uLNice;
        uLKernel = (uLSystem1 - g_pstIdelHandle->uLSystem)
                    + (uLIrq1 - g_pstIdelHandle->uLIrq)
                    + (uLSoftIrq1 - g_pstIdelHandle->uLSoftIrq)
                    + (uLIOWait1 - g_pstIdelHandle->uLIOWait)
                    + (uLSteal1 - g_pstIdelHandle->uLSteal);
        uLIdel = (uLIdle1 - g_pstIdelHandle->uLIdle);
        uLTotal = uLUser + uLKernel + uLIdel;

        g_pstIdelHandle->uLUser = uLUser1;
        g_pstIdelHandle->uLNice = uLNice1;
        g_pstIdelHandle->uLSystem = uLSystem1;
        g_pstIdelHandle->uLIdle = uLIdle1;
        g_pstIdelHandle->uLIOWait = uLIOWait1;
        g_pstIdelHandle->uLIrq = uLIrq1;
        g_pstIdelHandle->uLSoftIrq = uLSoftIrq1;
        g_pstIdelHandle->uLSteal = uLSteal1;
        g_pstIdelHandle->uLGuest = uLGuest1;

        if (blFirstTime)
        {
            blFirstTime = DOS_FALSE;
            continue;
        }

        if (g_pstIdelHandle->ulRecordCnt >= SYSSTAT_CPU_IDEL_SOMMTH_DEPTH)
        {
            g_pstIdelHandle->ulRecordCnt = 0;
        }

        if (!uLTotal)
        {
            DOS_ASSERT(0);
            continue;
        }

        /* ����㷨������Freeswitch����ʱ��֪���Ǹ�ʲô��˼ */
        g_pstIdelHandle->aulHistory[g_pstIdelHandle->ulRecordCnt++] = (100 * (100 * uLIdel + uLTotal / 2)) / uLTotal;

        uLTotal = 0;
        for (lInd = 0; lInd<g_pstIdelHandle->ulRecordCnt; lInd++)
        {
            uLTotal += g_pstIdelHandle->aulHistory[lInd];
        }

        g_pstIdelHandle->ulArvg = uLTotal / lInd;
        if (g_pstIdelHandle->ulArvg > 10000)
        {
            g_pstIdelHandle->ulArvg = 10000;
        }
    }

    if (pStatFile)
    {
        fclose(pStatFile);
    }

    return NULL;
}

U32 dos_sysstat_cpu_start()
{
    g_pstIdelHandle = dos_dmem_alloc(sizeof(DOS_CPU_IDEL_HANDLE_ST));
    if (DOS_ADDR_INVALID(g_pstIdelHandle))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    dos_memzero(g_pstIdelHandle, sizeof(DOS_CPU_IDEL_HANDLE_ST));

    pthread_create(&g_pthSYSSTSTCpu, NULL, dos_sysstat_cpu_monitor, NULL);

    return DOS_SUCC;
}

U32 dos_get_cpu_idel_percentage()
{
    /* ���û�����ø�ģ�飬����Ϊû��CPUռ�� */
    if (!g_blSYSStatEnable)
    {
        return 10000;
    }

    /* ����һ�� */
    if (g_pstIdelHandle->ulArvg > 10000)
    {
        g_pstIdelHandle->ulArvg = 10000;
    }

    return (U32)g_pstIdelHandle->ulArvg;
}

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

