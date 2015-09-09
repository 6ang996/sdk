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
extern U32 g_ulPidCnt; //ʵ�����еĽ��̸���

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


/** lsof -p 1  �������1�򿪵������ļ�
 * COMMAND PID USER   FD   TYPE             DEVICE SIZE/OFF   NODE NAME
 * init      1 root  cwd    DIR              253,0     4096      2 /
 * init      1 root  rtd    DIR              253,0     4096      2 /
 * init      1 root  txt    REG              253,0   150352 391923 /sbin/init
 * ........
 * init      1 root    7u  unix 0xffff880037d51cc0      0t0   7637 socket
 * init      1 root    9u  unix 0xffff880037b45980      0t0  12602 socket
 * ........
 * ����:��ȡ���̴򿪵��ļ�����������
 * ��������
 *   ����1: S32 lPid ����id
 * ����ֵ��
 *   �ɹ��򷵻ش򿪵��ļ�������ʧ�ܷ���-1
 */
static U32  mon_get_openfile_count(U32 ulPid)
{
    S8  szLsofCmd[MAX_CMD_LENGTH] = {0};
    S8  szBuff[MAX_BUFF_LENGTH] = {0};
    FILE  *fp = NULL;
    U32 ulCount = 0;

    if (DOS_FALSE == mon_is_pid_valid(ulPid))
    {
        mon_trace(MON_TRACE_PROCESS, LOG_LEVEL_ERROR, "Pid %u does not exist.", ulPid);
        return DOS_FAIL;
    }

    dos_snprintf(szLsofCmd, sizeof(szLsofCmd), "lsof -p %u | wc -l", ulPid);

    fp = popen(szLsofCmd, "r");
    if (DOS_ADDR_INVALID(fp))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (NULL != fgets(szBuff, sizeof(szBuff), fp))
    {
        if (dos_atoul(szBuff, &ulCount) < 0)
        {
            DOS_ASSERT(0);
            pclose(fp);
            fp = NULL;
            return DOS_FAIL;
        }
    }

    pclose(fp);
    fp = NULL;

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
    DIR * pstDir;
    struct dirent * pstEntry;
    /* ���pid��Ŀ¼ */
    S8 szPidDir[1024] = {0};
    S8 szServiceRoot[256] = {0};
    S8 *pszRoot = NULL, *pszPos = NULL;
    U32 ulPid = 0, ulRet = U32_BUTT;

    FILE * fp = NULL;

    pszRoot = config_get_service_root(szServiceRoot, sizeof(szServiceRoot));
    if (DOS_ADDR_INVALID(pszRoot))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if ('/' != szServiceRoot[dos_strlen(szServiceRoot) - 1])
    {
        dos_snprintf(szPidDir, sizeof(szPidDir), "%s%s", szServiceRoot, "/");
        dos_snprintf(szServiceRoot, sizeof(szServiceRoot), "%s", szPidDir);
    }

    dos_snprintf(szPidDir, sizeof(szPidDir), "%s%s", szServiceRoot, "var/run/pid/");

    g_ulPidCnt = 0;
    pstDir = opendir(szPidDir);
    if (DOS_ADDR_INVALID(pstDir))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    dos_memzero(g_pastProc[0], MAX_PROC_CNT * sizeof(MON_PROC_STATUS_S));
    while (NULL != (pstEntry = readdir(pstDir)))
    {
        /*�����ǰ�ļ�����ͨ�ļ�(Ϊ�˹��˵�"."��".."Ŀ¼)��������pid��׺������Ϊ�ǽ���id�ļ�*/
        if (DT_REG == pstEntry->d_type && DOS_TRUE == mon_is_suffix_true(pstEntry->d_name, "pid"))//�������ͨ�ļ�
        {
            S8     szProcAllName[64] = {0};
            S8     szLine[8] = {0};
            S8     szAbsFilePath[64] = {0};

            dos_snprintf(szAbsFilePath, sizeof(szAbsFilePath), "%s%s", szPidDir, pstEntry->d_name);

            fp = fopen(szAbsFilePath, "r");

            if (DOS_ADDR_INVALID(fp))
            {
                DOS_ASSERT(0);
                closedir(pstDir);
                return DOS_FAIL;
            }

            fseek(fp, 0, SEEK_SET);
            if (NULL != fgets(szLine, sizeof(szLine), fp))
            {
                if(DOS_ADDR_INVALID(g_pastProc[g_ulPidCnt]))
                {
                    DOS_ASSERT(0);
                    goto failure;
                }

                if (dos_atoul(szLine, &ulPid) < 0)
                {
                    DOS_ASSERT(0);
                    goto failure;
                }

                if (DOS_TRUE == mon_is_proc_dead(ulPid))
                {
                /* ���˵�����Ѿ������ˣ�������PID�ļ����ڣ�������������� */
                    mon_trace(MON_TRACE_PROCESS, LOG_LEVEL_WARNING, "Process has been dead, but pid(%u) file still exists.", ulPid);
                    fclose(fp);
                    fp  = NULL;
                    continue;
                }

                g_pastProc[g_ulPidCnt]->ulProcId = ulPid;

                ulRet = mon_get_proc_name_by_id(ulPid, szProcAllName, sizeof(szProcAllName));
                if (DOS_SUCC != ulRet)
                {
                    DOS_ASSERT(0);
                    goto failure;
                }
                if (dos_strstr(szProcAllName, "monitor"))
                {
                    continue;
                }
                pszPos = dos_strstr(szProcAllName, "\n");
                if (DOS_ADDR_VALID(pszPos))
                {
                    *pszPos = '\0';
                }

                dos_snprintf(g_pastProc[g_ulPidCnt]->szProcName
                    , sizeof(g_pastProc[g_ulPidCnt]->szProcName)
                    , "%s"
                    , szProcAllName);

                ++g_ulPidCnt;
            }
            else
            {
                fclose(fp);
                fp = NULL;
            }
            fclose(fp);
            fp = NULL;
        }
    }
    closedir(pstDir);
    return DOS_SUCC;

failure:
    closedir(pstDir);
    fclose(fp);
    fp = NULL;
    return DOS_FAIL;
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
    U32 ulRows = 0;
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

    for (ulRows = 0; ulRows < g_ulPidCnt; ulRows++)
    {
        if(DOS_ADDR_INVALID(g_pastProc[ulRows]))
        {
            DOS_ASSERT(0);
            return DOS_FAIL;
        }

        ulRet = mon_get_cpu_mem_time_info(g_pastProc[ulRows]->ulProcId, g_pastProc[ulRows]);
        if(DOS_SUCC != ulRet)
        {
            mon_trace(MON_TRACE_PROCESS, LOG_LEVEL_ERROR, "Get the CPU and Memory Information of Process FAIL.");
            return DOS_FAIL;
        }

        ulRet = mon_get_openfile_count(g_pastProc[ulRows]->ulProcId);
        if(DOS_FAIL == ulRet)
        {
            mon_trace(MON_TRACE_PROCESS, LOG_LEVEL_ERROR, "Get opened file count FAIL.");
            return DOS_FAIL;
        }
        g_pastProc[ulRows]->ulOpenFileCnt = ulRet;

        ulRet = mon_get_db_conn_count(g_pastProc[ulRows]->ulProcId);
        if(DOS_FAIL == ulRet)
        {
            mon_trace(MON_TRACE_PROCESS, LOG_LEVEL_ERROR, "Get Database connections count FAIL.");
            return DOS_FAIL;
        }
        g_pastProc[ulRows]->ulDBConnCnt = ulRet;

        ulRet = mon_get_threads_count(g_pastProc[ulRows]->ulProcId);
        if(DOS_FAIL == ulRet)
        {
            mon_trace(MON_TRACE_PROCESS, LOG_LEVEL_ERROR, "Get Threads count FAIL.");
            return DOS_FAIL;
        }
        g_pastProc[ulRows]->ulThreadsCnt = ulRet;
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
    S32 lRet = 0;
    U32 ulRows = 0, ulIndex = 0, ulNo = 0, ulRet = 0;
    S8  szProcName[32] = {0};
    S8  szProcVersion[16] = {0};
    S8  szProcCmd[1024] = {0};
    U32 ulCfgProcCnt = 0;

    /* ��ȡ��ǰ���ý������� */
    ulCfgProcCnt = config_hb_get_process_cfg_cnt();
    if(0 > ulCfgProcCnt)
    {
        mon_trace(MON_TRACE_PROCESS, LOG_LEVEL_ERROR, "Get config process count FAIL.");
        config_hb_deinit();
        return DOS_FAIL;
    }

    ulNo = mon_generate_warning_id(PROC_RES, 0x00, 0x02);
    if ((U32)0xff == ulNo)
    {
        return DOS_FAIL;
    }

    ulIndex = mon_get_msg_index(ulNo);
    if (U32_BUTT == ulIndex)
    {
        return DOS_FAIL;
    }

    /*
    *  ������õĽ��̸���С�ڻ��ߵ��ڼ�ص��Ľ��̸�����
    *  ��ô��Ϊ���еļ�ؽ��̶��Ѿ�����
    */
   if (ulCfgProcCnt <= g_ulPidCnt)
   {
        if (DOS_TRUE == g_pstWarningMsg[ulIndex].bExcep)
        {
            MON_MSG_S *pstMsg = (MON_MSG_S *)dos_dmem_alloc(sizeof(MON_MSG_S));
            if (DOS_ADDR_INVALID(pstMsg))
            {
                return DOS_FAIL;
            }
            pstMsg->ulWarningId = ulNo;
            pstMsg->ulMsgLen = dos_strlen(g_pstWarningMsg[ulIndex].szNormalDesc);
            pstMsg->msg = g_pstWarningMsg[ulIndex].szNormalDesc;
            g_pstWarningMsg[ulIndex].bExcep = DOS_FALSE;

            /* ����Ϣ�������ݿ����Ϣ���� */
            ulRet = mon_add_warning_record(pstMsg->ulWarningId, (S8*)pstMsg->msg);
            if (DOS_SUCC != ulRet)
            {
                return DOS_FAIL;
            }

            ulRet = mon_warning_msg_en_queue(pstMsg);
            if (DOS_SUCC != ulRet)
            {
                return DOS_FAIL;
            }
        }

        return DOS_SUCC;
   }
   else
   {
       if (DOS_FALSE == g_pstWarningMsg[ulIndex].bExcep)
       {
            MON_MSG_S *pstMsg = (MON_MSG_S *)dos_dmem_alloc(sizeof(MON_MSG_S));
            if (DOS_ADDR_INVALID(pstMsg))
            {
                return DOS_FAIL;
            }

            pstMsg->ulWarningId = ulNo;
            dos_snprintf(g_pstWarningMsg[ulIndex].szWarningDesc, sizeof(g_pstWarningMsg[ulIndex].szWarningDesc)
                        , "%u %s %s down", ulCfgProcCnt - g_ulPidCnt
                        , ulCfgProcCnt - g_ulPidCnt == 1 ? "Process":"Processes"
                        , ulCfgProcCnt - g_ulPidCnt == 1 ? "is":"are");
            pstMsg->ulMsgLen = dos_strlen(g_pstWarningMsg[ulIndex].szWarningDesc);
            pstMsg->msg = g_pstWarningMsg[ulIndex].szWarningDesc;
            g_pstWarningMsg[ulIndex].bExcep = DOS_TRUE;

            /* ����Ϣ�������ݿ����Ϣ���� */
            ulRet = mon_add_warning_record(pstMsg->ulWarningId, (S8*)pstMsg->msg);
            if (DOS_SUCC != ulRet)
            {
                return DOS_FAIL;
            }

            ulRet = mon_warning_msg_en_queue(pstMsg);
            if (DOS_SUCC != ulRet)
            {
                return DOS_FAIL;
            }
       }
   }

   for (ulRows = 0; ulRows < ulCfgProcCnt; ulRows++)
   {
        U32 ulCols = 0;
        /* Ĭ��δ���� */
        S32 bHasStarted = DOS_FALSE;

        /* ��ȡ�������ͽ��̰汾�� */
        lRet = config_hb_get_process_list(ulRows, szProcName, sizeof(szProcName)
                 , szProcVersion, sizeof(szProcVersion));
        if(lRet < 0)
        {
            mon_trace(MON_TRACE_PROCESS, LOG_LEVEL_ERROR, "Get Process Name,Version FAIL.");
            config_hb_deinit();
            return DOS_FAIL;
        }
        szProcName[sizeof(szProcName) - 1] = '\0';
        szProcVersion[sizeof(szProcVersion) - 1] = '\0';

        /* ��ȡ���̵��������� */
        lRet = config_hb_get_start_cmd(ulRows, szProcCmd, sizeof(szProcCmd));
        if(0 > lRet)
        {
            mon_trace(MON_TRACE_PROCESS, LOG_LEVEL_ERROR, "Get Start Command FAIL.");
            config_hb_deinit();
            return DOS_FAIL;
        }
        szProcCmd[sizeof(szProcCmd) - 1] = '\0';

        for(ulCols = 0; ulCols < g_ulPidCnt; ulCols++)
        {
            S8 * pszPtr = mon_str_get_name(g_pastProc[ulCols]->szProcName);

            if (DOS_ADDR_INVALID(pszPtr))
            {
                DOS_ASSERT(0);
                break;
            }

            if(0 == dos_strcmp(szProcName, "monitord"))
            {
                /* �����ص�����minitord���̣���ô��Ϊ������ */
                bHasStarted = DOS_TRUE;
                break;
            }
            if(0 == dos_strcmp("monitord", pszPtr))
            {
                /* �����ǰ���̲���minitord���̣�����monitord���̵ĶԱ�ֱ������ */
                continue;
            }
            if(0 == dos_strcmp(pszPtr, szProcName))
            {
            /* �����ҵ���˵��szProcName������ */
                bHasStarted = DOS_TRUE;
                break;
            }
        }

        if(DOS_FALSE == bHasStarted)
        {
            mon_trace(MON_TRACE_PROCESS, LOG_LEVEL_DEBUG, "Process %s lost.", szProcName);

            lRet = mon_system(szProcCmd);
            if(lRet < 0)
            {
                mon_trace(MON_TRACE_PROCESS,LOG_LEVEL_EMERG, "Restart Process %s FAIL.", szProcName);
                return DOS_FAIL;
            }
        }
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

    for (ulRows = 0; ulRows < g_ulPidCnt; ulRows++)
    {
        if(DOS_ADDR_INVALID(g_pastProc[ulRows]))
        {
            DOS_ASSERT(0);
            return DOS_FAIL;
        }

        if(DOS_FALSE == mon_is_pid_valid(g_pastProc[ulRows]->ulProcId))
        {
            DOS_ASSERT(0);
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

    for (ulRows = 0; ulRows < g_ulPidCnt; ulRows++)
    {
        if(DOS_ADDR_INVALID(g_pastProc[ulRows]))
        {
            DOS_ASSERT(0);
            return DOS_FAIL;
        }

        if(DOS_FALSE == mon_is_pid_valid(g_pastProc[ulRows]->ulProcId))
        {
            mon_trace(MON_TRACE_PROCESS, LOG_LEVEL_ERROR, "Pid %u does not exist.", g_pastProc[ulRows]->ulProcId);
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

