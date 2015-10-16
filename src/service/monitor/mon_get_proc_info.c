#ifdef __cplusplus
extern "C" {
#endif

#include <dos.h>
#include <dos/dos_mem.h>

#if (INCLUDE_BH_SERVER)
#if INCLUDE_RES_MONITOR
#if INCLUDE_PROC_MONITOR

#include <dirent.h>
#include "mon_get_proc_info.h"
#include "mon_warning_msg_queue.h"
#include "mon_lib.h"
#include "mon_def.h"


extern MON_PROC_STATUS_S * g_pastProc[MAX_PROC_CNT];
extern MON_WARNING_MSG_S*  g_pstWarningMsg;

static U32   mon_proc_reset_data();
static BOOL  mon_is_pid_valid(U32 ulPid);
static U32   mon_get_cpu_mem_time_info(U32 ulPid, MON_PROC_STATUS_S * pstProc);
static U32   mon_get_openfile_count(U32 ulPid);
static U32   mon_get_threads_count(U32 ulPid);
static U32   mon_get_proc_pid_list();

extern U32 mon_get_msg_index(U32 ulNo);
extern U32 mon_add_warning_record(U32 ulResId,S8 * szInfoDesc);


/**
 * ����:Ϊ��ؽ�����������ڴ�
 * ��������
 *   �޲���
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
U32  mon_proc_malloc()
{
    U32 ulRows = 0;
    MON_PROC_STATUS_S * pstProc = NULL;

    pstProc = (MON_PROC_STATUS_S *)dos_dmem_alloc(MAX_PROC_CNT * sizeof(MON_PROC_STATUS_S));
    if(DOS_ADDR_INVALID(pstProc))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    dos_memzero(pstProc, MAX_PROC_CNT * sizeof(MON_PROC_STATUS_S));

    for(ulRows = 0; ulRows < MAX_PROC_CNT; ulRows++)
    {
        g_pastProc[ulRows] = &(pstProc[ulRows]);
    }

   return DOS_SUCC;
}

/**
 * ����:�ͷ�Ϊ��ؽ������������ڴ�
 * ��������
 *   �޲���
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
U32 mon_proc_free()
{
    U32 ulRows = 0;

    MON_PROC_STATUS_S * pstProc = g_pastProc[0];
    if(DOS_ADDR_INVALID(pstProc))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    dos_dmem_free(pstProc);
    for(ulRows = 0; ulRows < MAX_PROC_CNT; ulRows++)
    {
        g_pastProc[ulRows] = NULL;
    }

    return DOS_SUCC;
}

static U32   mon_proc_reset_data()
{
    MON_PROC_STATUS_S * pstProc = g_pastProc[0];

    if(DOS_ADDR_INVALID(pstProc))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    dos_memzero(pstProc, MAX_PROC_CNT * sizeof(MON_PROC_STATUS_S));

    return DOS_SUCC;
}

/**
 * ����:�жϽ���idֵ�Ƿ��ڽ���idֵ����Ч��Χ��
 * ��������
 *   �޲���
 * ����ֵ��
 *   �Ƿ���DOS_TRUE��ʧ�ܷ���DOS_FALSE
 */
static BOOL mon_is_pid_valid(U32 ulPid)
{
    if(ulPid > MAX_PID_VALUE || ulPid <= MIN_PID_VALUE)
    {
        DOS_ASSERT(0);
        return DOS_FALSE;
    }
    return DOS_TRUE;
}

/** ps aux
 * USER       PID %CPU %MEM    VSZ   RSS TTY      STAT START   TIME COMMAND
 * root         1  0.0  0.1  19364  1528 ?        Ss   06:34   0:01 /sbin/init
 * root         2  0.0  0.0      0     0 ?        S    06:34   0:00 [kthreadd]
 * root         3  0.0  0.0      0     0 ?        S    06:34   0:00 [migration/0]
 * root         4  0.0  0.0      0     0 ?        S    06:34   0:00 [ksoftirqd/0]
 * root         5  0.0  0.0      0     0 ?        S    06:34   0:00 [migration/0]
 * ...........................
 * ����:���ݽ���id��ȡ���̵�cpu��Ϣ���ڴ���Ϣ��ʱ����Ϣ
 * ��������
 *   ����1: S32 lPid ����id
 *   ����2: MON_PROC_STATUS_S * pstProc
 * ����ֵ��
 *   �Ƿ���DOS_TRUE��ʧ�ܷ���DOS_FALSE
 */

static U32  mon_get_cpu_mem_time_info(U32 ulPid, MON_PROC_STATUS_S * pstProc)
{
    S8  szPsCmd[32] = {0};
    S8  szBuff[1024] = {0};
    S8  *pszToker = 0;
    S8  szCPURate[8] = {0}, szMemRate[8] = {0};
    U32 ulCount = 0;
    F64 fCPURate = 0, fMemRate = 0;
    FILE *fp = NULL;

    if (DOS_FALSE == mon_is_pid_valid(ulPid))
    {
        mon_trace(MON_TRACE_PROCESS, LOG_LEVEL_ERROR, "Pid %u does not exist.", ulPid);
        return DOS_FAIL;
    }
    dos_snprintf(szPsCmd, sizeof(szPsCmd), "ps aux | grep %u", ulPid);
    pstProc->ulProcId = ulPid;

    fp = popen(szPsCmd, "r");
    if (DOS_ADDR_INVALID(fp))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    while(!feof(fp))
    {
        if (NULL != fgets(szBuff, sizeof(szBuff), fp))
        {
            if (ulPid == mon_first_int_from_str(szBuff))
            {
                break;
            }
        }
    }

    pszToker = strtok(szBuff, " \t\n");
    while (pszToker)
    {
        pszToker = strtok(NULL, " \t\n");
        switch(ulCount)
        {
            case 1:
                dos_snprintf(szCPURate, sizeof(szCPURate), "%s", pszToker);
                fCPURate = atof(szCPURate);
                pstProc->fCPURate = fCPURate;
                break;
            case 2:
                dos_snprintf(szMemRate, sizeof(szMemRate), "%s", pszToker);
                fMemRate = atof(szMemRate);
                pstProc->fMemoryRate = fMemRate;
                break;
            case 8:
                dos_snprintf(pstProc->szProcCPUTime, sizeof(pstProc->szProcCPUTime), "%s", pszToker);
                goto success;
        }
        ++ulCount;
    }
success:
    pclose(fp);
    fp = NULL;
    return DOS_SUCC;
}


/**
 * ����:��ȡ���̴򿪵��ļ�����������
 * ��������
 *   ����1: S32 lPid ����id
 * ����ֵ��
 *   �ɹ��򷵻ش򿪵��ļ�������ʧ�ܷ���-1
 */
static U32  mon_get_openfile_count(U32 ulPid)
{
    U32 ulCount = 0;
    return ulCount;
}


/**
 * ����:��ȡ���̵����ݿ����Ӹ���
 * ��������
 *   ����1: S32 lPid ����id
 * ����ֵ��
 *   �ɹ��������ݿ����Ӹ�����ʧ�ܷ���-1
 */
static U32   mon_get_db_conn_count(U32 ulPid)
{  /* Ŀǰû���ҵ���Ч�Ľ������ */
    if(DOS_FALSE == mon_is_pid_valid(ulPid))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/**
 * Name:   init
 * State:  S (sleeping)
 * Tgid:   1
 * Pid:    1
 * PPid:   0
 * TracerPid:      0
 * ......
 * VmPTE:        56 kB
 * VmSwap:        0 kB
 * Threads:        1
 * SigQ:   3/10771
 * SigPnd: 0000000000000000
 * ShdPnd: 0000000000000000
 * ......
 * ����:��ȡ���̵��̸߳���
 * ��������
 *   ����1: S32 lPid ����id
 * ����ֵ��
 *   �ɹ��������ݿ����Ӹ�����ʧ�ܷ���-1
 */
static U32   mon_get_threads_count(U32 ulPid)
{
    S8  szPidFile[MAX_CMD_LENGTH] = {0};
    S8  szLine[MAX_BUFF_LENGTH] = {0};
    U32 ulThreadsCount = 0;
    FILE * fp;
    S8* pszAnalyseRslt[2] = {0};

    if(DOS_FALSE == mon_is_pid_valid(ulPid))
    {
        mon_trace(MON_TRACE_PROCESS, LOG_LEVEL_ERROR, "Pid %u does not exist.", ulPid);
        return DOS_FAIL;
    }

    dos_snprintf(szPidFile, MAX_CMD_LENGTH, "/proc/%u/status", ulPid);

    fp = fopen(szPidFile, "r");
    if (DOS_ADDR_INVALID(fp))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    fseek(fp, 0, SEEK_SET);
    while (!feof(fp))
    {
        dos_memzero(szLine, MAX_BUFF_LENGTH * sizeof(S8));
        if (NULL != (fgets(szLine, MAX_BUFF_LENGTH, fp)))
        {
            /* Threads������ߵ��Ǹ����־��ǵ�ǰ���̵��߳����� */
            if (0 == (dos_strncmp(szLine, "Threads", dos_strlen("Threads"))))
            {
                U32 ulData = 0;
                U32 ulRet = 0;
                S32 lRet = 0;
                ulRet = mon_analyse_by_reg_expr(szLine, " \t\n", pszAnalyseRslt, sizeof(pszAnalyseRslt) / sizeof(pszAnalyseRslt[0]));
                if(DOS_SUCC != ulRet)
                {
                    mon_trace(MON_TRACE_PROCESS, LOG_LEVEL_ERROR, "Analyse buffer by regular expression FAIL.");
                    goto failure;
                }

                lRet = dos_atoul(pszAnalyseRslt[1], &ulData);
                if(0 != lRet)
                {
                    DOS_ASSERT(0);
                    goto failure;
                }
                ulThreadsCount = ulData;
                goto success;
            }
        }
    }
failure:
    fclose(fp);
    fp = NULL;
    return DOS_FAIL;
success:
    fclose(fp);
    fp = NULL;
    return ulThreadsCount;
}

/**
 * ����:��ȡ��Ҫ����صĽ����б�
 * ��������
 *   �޲���
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
static U32 mon_get_proc_pid_list()
{
    U32  ulCfgProcCnt = 0;
    U32  ulIndex = 0, ulRet = U32_BUTT;
    S32  lRet;
    FILE *fpPid = NULL;
    S8   szServRoot[16] = {0}, *pszRet = NULL, szPid[8] = {0};
    S8   szPidFile[64] = {0}, szPidFileTemp[64] = {0}, *pszPos = NULL;

    /* ���Ȼ�ȡ����ؽ��̸��� */
    ulCfgProcCnt = config_hb_get_process_cfg_cnt();
    /* ��ȡdipcc�����Ŀ¼ */
    pszRet = config_get_service_root(szServRoot, sizeof(szServRoot));
    if (DOS_ADDR_INVALID(pszRet))
    {
        DOS_ASSERT(0);
        mon_trace(MON_TRACE_PROCESS, LOG_LEVEL_ERROR, "Get Dipcc Service Root Directory FAIL.");
        return DOS_FAIL;
    }

    for (ulIndex = 0; ulIndex < ulCfgProcCnt; ++ulIndex)
    {
        /* ��ȡ������������������� */
        lRet = config_hb_get_process_list(ulIndex
                , g_pastProc[ulIndex]->szProcName
                , sizeof(g_pastProc[ulIndex]->szProcName)
                , g_pastProc[ulIndex]->szVersion
                , sizeof(g_pastProc[ulIndex]->szVersion));
        if (lRet < 0)
        {
            mon_trace(MON_TRACE_PROCESS, LOG_LEVEL_ERROR, "Get the name and version of process FAIL.");
            DOS_ASSERT(0);
            return DOS_FAIL;
        }
        /* ��ȡ���̵��������� */
        lRet = config_hb_get_start_cmd(ulIndex, g_pastProc[ulIndex]->szAbsPath, sizeof(g_pastProc[ulIndex]->szAbsPath));
        if (lRet < 0)
        {
            mon_trace(MON_TRACE_PROCESS, LOG_LEVEL_ERROR, "Get start command of Process FAIL.");
            DOS_ASSERT(0);
            return DOS_FAIL;
        }

        /* ��������ļ�·�� */
        dos_snprintf(szPidFileTemp, sizeof(szPidFileTemp), "%s", szServRoot);
        if ('/' != szPidFileTemp[dos_strlen(szPidFileTemp) - 1])
        {
            dos_strcat(szPidFileTemp, "/");
        }
        dos_strcat(szPidFileTemp, "var/run/pid/");
        dos_snprintf(szPidFile, sizeof(szPidFile), "%s%s.pid", szPidFileTemp, g_pastProc[ulIndex]->szProcName);

        /* ��ȡ����id */
        fpPid = fopen(szPidFile, "r");
        if (DOS_ADDR_INVALID(fpPid))
        {
            DOS_ASSERT(0);
            mon_trace(MON_TRACE_PROCESS, LOG_LEVEL_ERROR, "Pid file of Process \'%s\' does not exist.", g_pastProc[ulIndex]->szProcName);
            g_pastProc[ulIndex]->bStatus = DOS_FALSE;
            continue;
        }

        /* ��ȡ����id��Ϣ */
        if (!fgets(szPid, sizeof(szPid), fpPid))
        {
            /* ����ļ������ڣ�˵�����̿϶�û������ */
            g_pastProc[ulIndex]->bStatus = DOS_FALSE;
            fclose(fpPid);
            fpPid = NULL;
            mon_trace(MON_TRACE_PROCESS, LOG_LEVEL_ERROR, "Get pid of Process \'%s\' FAIL.", g_pastProc[ulIndex]->szProcName);
            continue;
        }

        /* �����ļ�β���Ļس����� */
        if (NULL != (pszPos = dos_strstr(szPid, "\r")))
        {
            *pszPos = '\0';
        }
        if (NULL != (pszPos = dos_strstr(szPid, "\n")))
        {
            *pszPos = '\0';
        }

        if (dos_atoul(szPid, &g_pastProc[ulIndex]->ulProcId) < 0)
        {
            g_pastProc[ulIndex]->bStatus = DOS_FALSE;
            DOS_ASSERT(0);
            fclose(fpPid);
            fpPid = NULL;
            continue;
        }
        fclose(fpPid);
        fpPid = NULL;

        if (mon_is_proc_dead(g_pastProc[ulIndex]->ulProcId))
        {
            g_pastProc[ulIndex]->bStatus = DOS_FALSE;
            continue;
        }

        /* ��ȡ����CPU���ڴ���Ϣ */
        ulRet = mon_get_cpu_mem_time_info(g_pastProc[ulIndex]->ulProcId, g_pastProc[ulIndex]);
        if (DOS_SUCC != ulRet)
        {
            g_pastProc[ulIndex]->bStatus = DOS_FALSE;
            mon_trace(MON_TRACE_PROCESS, LOG_LEVEL_ERROR, "Get CPU MEM TIME Info of Process FAIL.");
            continue;
        }

        /* ��ȡ�򿪵��ļ����� */
        g_pastProc[ulIndex]->ulOpenFileCnt = mon_get_openfile_count(g_pastProc[ulIndex]->ulProcId);
        /* ��ȡ���ݿ����Ӹ��� */
        g_pastProc[ulIndex]->ulDBConnCnt = mon_get_db_conn_count(g_pastProc[ulIndex]->ulProcId);
        /* ��ȡ�����ڲ����̸߳��� */
        g_pastProc[ulIndex]->ulThreadsCnt = mon_get_threads_count(g_pastProc[ulIndex]->ulProcId);
        g_pastProc[ulIndex]->bStatus = DOS_TRUE;
    }
    return DOS_SUCC;
}

/**
 * ����:��ȡ��������������Ϣ
 * ��������
 *   �޲���
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
U32 mon_get_process_data()
{
    U32 ulRet = 0;

    ulRet = mon_proc_reset_data();
    if (DOS_SUCC != ulRet)
    {
        mon_trace(MON_TRACE_PROCESS, LOG_LEVEL_ERROR, "Process Module Reset Data FAIL.");
        return DOS_FAIL;
    }

    ulRet = mon_get_proc_pid_list();
    if(DOS_SUCC != ulRet)
    {
        mon_trace(MON_TRACE_PROCESS, LOG_LEVEL_ERROR, "Get Process pid list FAIL.");
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/**
 * ����:�����û�е��ߵĽ��̣����������������֮
 * ��������
 *   �޲���
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
U32  mon_check_all_process()
{
    U32  ulCount = 0, ulRet = 0;
    S32  lCfgProcCnt = 0, lLoop = 0;
    S8   szStartCmd[64] = {0};

    /* ��ȡ���ü�ؽ��̸��� */
    lCfgProcCnt = config_hb_get_process_cfg_cnt();
    if (lCfgProcCnt < 0)
    {
        DOS_ASSERT(0);
        mon_trace(MON_TRACE_PROCESS, LOG_LEVEL_ERROR, "Get config Process count FAIL.");
        return DOS_FAIL;
    }

    for (lLoop = 0; lLoop < MAX_PROC_CNT; ++lLoop)
    {
        if (DOS_FALSE == g_pastProc[lLoop]->bStatus)
        {
            if (g_pastProc[lLoop]->szAbsPath
                && '\0' != g_pastProc[lLoop]->szAbsPath[0])
            {
                mon_trace(MON_TRACE_PROCESS, LOG_LEVEL_DEBUG, "Process \'%s\' has been dead.", g_pastProc[lLoop]->szProcName);
                dos_snprintf(szStartCmd, sizeof(szStartCmd), "(%s > /dev/null 2>&1 &)", g_pastProc[lLoop]->szAbsPath);
                ulRet = mon_system(szStartCmd);
                if (DOS_SUCC != ulRet)
                {
                    mon_trace(MON_TRACE_PROCESS, LOG_LEVEL_ERROR, "Start Process \'%s\' FAIL.", g_pastProc[lLoop]->szProcName);
                }
            }
        }
        else
        {
            ++ulCount;
        }
    }

    if (ulCount < lCfgProcCnt)
    {
        mon_trace(MON_TRACE_PROCESS, LOG_LEVEL_DEBUG, "%u Process(s) has been dead.", lCfgProcCnt - ulCount);
    }
    return DOS_SUCC;
}

/**
 * ����:���ݽ���id��ý�����
 * ��������
 *   ����1:S32 lPid  ����id
 *   ����2:S8 * pszPidName   ������
 *   ����3:U32 ulLen
 * ����ֵ��
 *   �ɹ��򷵻ؽ�������ʧ���򷵻�NULL
 */
U32 mon_get_proc_name_by_id(U32 ulPid, S8 * pszPidName, U32 ulLen)
{
    S8  szPath[64] = {0};
    S8  szLine[256] = {0};
    FILE *fp = NULL;
    S8 *pszPos = NULL;

    dos_snprintf(szPath, sizeof(szPath), "/proc/%u/task/%u/status", ulPid, ulPid);

    fp = fopen(szPath, "r");
    if (DOS_ADDR_INVALID(fp))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    if (!fgets(szLine, sizeof(szLine), fp))
    {
        DOS_ASSERT(0);
        fclose(fp);
        fp = NULL;
        return DOS_FAIL;
    }

    pszPos = dos_strstr(szLine, ":");
    if (DOS_ADDR_INVALID(pszPos))
    {
        DOS_ASSERT(0);
        fclose(fp);
        fp = NULL;
        return DOS_FAIL;
    }
    pszPos++;

    while (' ' == *pszPos || '\t' == *pszPos || '\r' == *pszPos || '\n' == *pszPos)
    {
        pszPos++;
    }

    if ('\0' ==  *pszPos)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    dos_snprintf(pszPidName, ulLen, "%s", pszPos);

    fclose(fp);
    fp = NULL;
    return DOS_SUCC;
}


/**
 * ����:�жϽ����Ƿ�����
 * ��������
 *   ����1: S32 lPid ����id
 * ����ֵ��
 *   �����򷵻�DOS_TRUE��ʧ�ܷ���DOS_FALSE
 */
BOOL mon_is_proc_dead(U32 ulPid)
{
    S8 szPath[16] = {0};

    dos_snprintf(szPath, sizeof(szPath), "/proc/%u/", ulPid);

    if (0 != access(szPath, F_OK))
    {
        /* Ŀ¼���������������������true */
        return DOS_TRUE;
    }
    return DOS_FALSE;
}

/**
 * ����:��ȡ���̵ľ���·��
 * ��������
 *   ����1: S32 lPid ����id
 *   ����2: S8* szPath�����̵ľ���·�����������
 *   ����3: U32 ulLen   ·������
 * ����ֵ��
 *   �����򷵻�DOS_TRUE��ʧ�ܷ���DOS_FALSE
 */
U32 mon_get_process_abspath(U32  ulPid, S8 *szPath, U32 ulLen)
{
    S8  szInfoPath[64] = {0};
    S8  szLine[256] = {0};
    S8* pszPos = NULL, *pszTemp = NULL;
    FILE  *fpPath = NULL;

    if (DOS_ADDR_INVALID(szPath)
        || ulLen == 0)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    dos_snprintf(szInfoPath, sizeof(szInfoPath), "/proc/%u/task/%u/numa_maps", ulPid, ulPid);
    fpPath = fopen(szInfoPath, "r");
    if (DOS_ADDR_INVALID(fpPath))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (!fgets(szLine, sizeof(szLine), fpPath))
    {
        DOS_ASSERT(0);
        fclose(fpPath);
        fpPath = NULL;
        return DOS_FAIL;
    }

    pszPos = dos_strstr(szLine, "file=");
    if (DOS_ADDR_INVALID(pszPos))
    {
        DOS_ASSERT(0);
        fclose(fpPath);
        fpPath = NULL;
        return DOS_FAIL;
    }
    while ('/' != *pszPos)
    {
        ++pszPos;
    }
    pszTemp = pszPos;
    while (' ' != *pszPos
            && '\t' != *pszPos)
    {
        ++pszPos;
    }
    *pszPos = '\0';

    dos_snprintf(szPath, ulLen, "%s", pszTemp);
    fclose(fpPath);
    fpPath = NULL;

    return DOS_SUCC;
}

/**
 * ����:��ȡ���м�ؽ��̵���cpuռ����
 * ��������
 *   �޲���
 * ����ֵ��
 *   �ɹ��򷵻���CPUռ���ʣ�ʧ�ܷ���-1
 */
U32  mon_get_proc_total_cpu_rate()
{
    F64 fTotalCPURate = 0.0;
    U32 ulRows = 0;

    for (ulRows = 0; ulRows < MAX_PROC_CNT; ulRows++)
    {
        if (!g_pastProc[ulRows]->bStatus)
        {
            continue;
        }
        fTotalCPURate += g_pastProc[ulRows]->fCPURate;
    }

    /* ռ���ʵĽ��ȡ������������ֵ�����溯��ͬ�� */
    return (S32)(fTotalCPURate + 0.5);
}

/**
 * ����:��ȡ���м�ؽ��̵����ڴ�ռ����
 * ��������
 *   �޲���
 * ����ֵ��
 *   �ɹ��򷵻����ڴ�ռ���ʣ�ʧ�ܷ���DOS_FAIL
 */
U32  mon_get_proc_total_mem_rate()
{
    F64 fTotalMemRate = 0.0;
    U32 ulRows = 0;

    for (ulRows = 0; ulRows < MAX_PROC_CNT; ulRows++)
    {
        if (!g_pastProc[ulRows])
        {
            continue;
        }
        fTotalMemRate += g_pastProc[ulRows]->fMemoryRate;
    }

    return (U32)(fTotalMemRate + 0.5);
}


#endif //end #if INCLUDE_PROC_MONITOR
#endif //end #if INCLUDE_RES_MONITOR
#endif //end #if (INCLUDE_BH_SERVER)

#ifdef __cplusplus
}
#endif

