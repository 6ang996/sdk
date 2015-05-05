#ifdef __cplusplus
extern "C" {
#endif

#include <dos.h>

#if (INCLUDE_BH_SERVER)
#if INCLUDE_RES_MONITOR
#if INCLUDE_PROC_MONITOR

#include <dirent.h>
#include "mon_get_proc_info.h"
#include "mon_lib.h"


extern S8 g_szMonProcessInfo[MAX_PROC_CNT * MAX_BUFF_LENGTH];
extern MON_PROC_STATUS_S * g_pastProc[MAX_PROC_CNT];
extern U32 g_ulPidCnt; //ʵ�����еĽ��̸���

static BOOL  mon_is_pid_valid(U32 ulPid);
static U32   mon_get_cpu_mem_time_info(U32 ulPid, MON_PROC_STATUS_S * pstProc);
static U32   mon_get_openfile_count(U32 ulPid);
static U32   mon_get_threads_count(U32 ulPid);
static U32   mon_get_proc_pid_list();
static U32   mon_kill_process(U32 ulPid);
static U32   mon_check_all_process();

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
   MON_PROC_STATUS_S * pstProc;
   
   pstProc = (MON_PROC_STATUS_S *)dos_dmem_alloc(MAX_PROC_CNT * sizeof(MON_PROC_STATUS_S));
   if(DOS_ADDR_INVALID(pstProc))
   {
      logr_cirt("%s:Line %u:mon_proc_malloc|alloc memory failure, pstProc is %p!"
                , dos_get_filename(__FILE__), __LINE__, pstProc);
      return DOS_FAIL;
   }
   memset(pstProc, 0, MAX_PROC_CNT * sizeof(MON_PROC_STATUS_S));
   
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
      logr_cirt("%s:Line %u:mon_proc_free|free memory failure,pstProc is %p!"
                , dos_get_filename(__FILE__), __LINE__ , pstProc);
      return DOS_FAIL;
   }
   
   dos_dmem_free(pstProc);
   for(ulRows = 0; ulRows < MAX_PROC_CNT; ulRows++)
   {
      g_pastProc[ulRows] = NULL;
   }
   
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
      logr_emerg("%s:Line %u:mon_is_pid_valid|pid %u is invalid!",
                 dos_get_filename(__FILE__), __LINE__, ulPid);
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
{//δ���
   S8  szPsCmd[MAX_CMD_LENGTH] = {0};
   S8  szProcFile[] = "procinfo"; 
   FILE * fp;
   S8* pszAnalyseRslt[12] = {0};
   U32 ulRet = 0;

   if(DOS_FALSE == mon_is_pid_valid(ulPid))
   {
      logr_error("%s:Line %u:mon_get_cpu_mem_time_info|get cpu & mem & time information failure,pid %u is invalid!"     
                    , dos_get_filename(__FILE__), __LINE__, ulPid);
      return DOS_FAIL;
   }
   
   if (DOS_ADDR_INVALID(pstProc))
   {
      logr_cirt("%s:line %u:mon_get_cpu_mem_time_info|get cpu & mem & time information|failure,pstProc is %p!"
                , dos_get_filename(__FILE__), __LINE__, pstProc);
      return DOS_FAIL;
   }

   dos_snprintf(szPsCmd, MAX_CMD_LENGTH, "ps aux | grep %u > %s", ulPid, szProcFile);
   szPsCmd[dos_strlen(szPsCmd)] = '\0';
   system(szPsCmd);

   fp = fopen(szProcFile, "r");
   if (DOS_ADDR_INVALID(fp))
   {
      logr_cirt("%s:line %u:mon_get_cpu_mem_time_info|get cpu & mem & time information|failure,file \'%s\' fp is %p!"
                , dos_get_filename(__FILE__), __LINE__, szProcFile, fp);
      return DOS_FAIL;
   }

   fseek(fp, 0, SEEK_SET);
   while (!feof(fp))
   {  
      S8 szLine[MAX_BUFF_LENGTH] = {0};
      U32 ulData = 0;
      S32 lRet  = 0;
      
      if (NULL != (fgets(szLine, MAX_BUFF_LENGTH, fp)))
      {
          ulRet = mon_analyse_by_reg_expr(szLine, " \t\n", pszAnalyseRslt, sizeof(pszAnalyseRslt)/sizeof(pszAnalyseRslt[0]));
          if(DOS_SUCC != ulRet)
          {
             logr_error("%s:Line %u:mon_get_cpu_mem_time_info|analyse sentence failure!"
                        , dos_get_filename(__FILE__), __LINE__);
             goto failure;
          }
          /* ����Ȼ���ڶ����ǽ���id�����������ڴ�ƽ��ռ���ʣ
           * ��������cpu��ƽ��ռ���ʣ���ʮ����cpu��ǰ����ʱ��
           */
          lRet = dos_atoul(pszAnalyseRslt[1], &ulData);
          if(0 != lRet)
          {
             logr_error("%s:Line %u:mon_get_cpu_mem_time_info|dos_atol failure!"
                        , dos_get_filename(__FILE__), __LINE__);
             goto failure;
          }
          if(ulPid != ulData)
          {
             continue;
          }
          pstProc->fCPURate = atof(pszAnalyseRslt[2]);
          pstProc->fMemoryRate = atof(pszAnalyseRslt[3]);
          dos_strcpy(pstProc->szProcCPUTime, pszAnalyseRslt[8]);
          pstProc->szProcCPUTime[dos_strlen(pszAnalyseRslt[8])] = '\0';
          break;
      }
   }
   goto success;

success:
   fclose(fp);
   fp = NULL;
   unlink(szProcFile);  
   return DOS_SUCC;
failure:
   fclose(fp);
   fp = NULL;
   unlink(szProcFile);  
   return DOS_FAIL;
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
   S8  szLine[MAX_BUFF_LENGTH] = {0};
   S8  szFileName[] = "filecount";
   U32 ulCount = 0;
   FILE * fp;

   if(DOS_FALSE == mon_is_pid_valid(ulPid))
   {
      logr_error("%s:Line %u:mon_get_openfile_count|get open file count failure,pid %u is invalid!"
                    , dos_get_filename(__FILE__), __LINE__, ulPid);
      return DOS_FAIL;
   }
   /**
    *  ʹ������: lsof -p pid | wc -l ����ͳ�Ƴ���ǰ���̴��˶����ļ�
    */
   dos_snprintf(szLsofCmd, MAX_CMD_LENGTH, "lsof -p %u | wc -l > %s", ulPid, szFileName);
   system(szLsofCmd);

   fp = fopen(szFileName, "r");
   fseek(fp, 0, SEEK_SET);
   if (DOS_ADDR_INVALID(fp))
   {
      logr_cirt("%s:line %u:mon_get_openfile_count|file \'%s\' open failure,fp is %p!"
                , dos_get_filename(__FILE__), __LINE__, szFileName, fp);
      return DOS_FAIL;
   }

   if (NULL != (fgets(szLine, MAX_BUFF_LENGTH, fp)))
   {
      S32 lRet = 0;
      
      lRet = dos_atoul(szLine, &ulCount);
      if(0 != lRet)
      {
         logr_error("%s:Line %u:mon_get_openfile_count|dos_atol failure,lRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, lRet);
         goto failure;
      }
   }
   goto success;

success:
   fclose(fp);
   fp = NULL;
   unlink(szFileName);
   return ulCount;
failure:
    fclose(fp);
    fp = NULL;
    unlink(szFileName);
    return DOS_FAIL;
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
      logr_error("%s:Line %u:mon_get_db_conn_count|get database connections count failure,process pid %d is invalid!", 
                    dos_get_filename(__FILE__), __LINE__, ulPid);
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
      logr_error("%s:Line %u:mon_get_threads_count|get threads count failure,pid %u is invalid!"
                    , dos_get_filename(__FILE__), __LINE__, ulPid);
      return DOS_FAIL;
   }
   
   dos_snprintf(szPidFile, MAX_CMD_LENGTH, "/proc/%u/status", ulPid);

   fp = fopen(szPidFile, "r");
   if (DOS_ADDR_INVALID(fp))
   {
      logr_cirt("%s:line %u:mon_get_threads_count|file \'%s\' open failure,fp is %p!"
                , dos_get_filename(__FILE__), __LINE__, szPidFile, fp);
      return DOS_FAIL;
   }

   fseek(fp, 0, SEEK_SET);
   while (!feof(fp))
   {
      memset(szLine, 0, MAX_BUFF_LENGTH * sizeof(S8));
      if (NULL != (fgets(szLine, MAX_BUFF_LENGTH, fp)))
      {
         /* Threads������ߵ��Ǹ����־��ǵ�ǰ���̵��߳����� */
         if (0 == (dos_strncmp(szLine, "Threads", dos_strlen("Threads"))))
         {
            U32 ulData = 0;
            U32 ulRet = 0;
            S32 lRet = 0;
            ulRet = mon_analyse_by_reg_expr(szLine, " \t\n", pszAnalyseRslt, sizeof(pszAnalyseRslt)/sizeof(pszAnalyseRslt[0]));
            if(DOS_SUCC != ulRet)
            {
                logr_error("%s:Line %u:mon_get_threads_count|analyse sentence failure"
                            , dos_get_filename(__FILE__), __LINE__);
                goto failure;
            }
            
            lRet = dos_atoul(pszAnalyseRslt[1], &ulData);
            if(0 != lRet)
            {
               logr_error("%s:Line %u:mon_get_threads_count|dos_atol failure"
                            , dos_get_filename(__FILE__), __LINE__);
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
   S8 szCommPidDir[] = "/tcom/var/run/pid/";
   /* ���ݿ��pid�ļ�����Ŀ¼ */
   S8 szDBPidDir[] = "/var/dipcc/";
   g_ulPidCnt = 0;
   FILE * fp = NULL;
   
   pstDir = opendir(szCommPidDir);
   if (DOS_ADDR_INVALID(pstDir))
   {
      logr_cirt("%s:Line %u:mon_get_proc_pid_list|dir \'%s\' access failed,psrDir is %p!"
                , dos_get_filename(__FILE__), __LINE__, szCommPidDir, pstDir);
      return DOS_FAIL;
   }

   while (NULL != (pstEntry = readdir(pstDir)))
   {  
      /*�����ǰ�ļ�����ͨ�ļ�(Ϊ�˹��˵�"."��".."Ŀ¼)��������pid��׺������Ϊ�ǽ���id�ļ�*/
      if (DT_REG == pstEntry->d_type && DOS_TRUE == mon_is_suffix_true(pstEntry->d_name, "pid"))//�������ͨ�ļ�
      {
         S8     szProcAllName[64] = {0};
         S8 *   pTemp;
         S8     szLine[8] = {0};
         S8     szAbsFilePath[64] = {0};

         dos_strcat(szAbsFilePath, szCommPidDir);
         dos_strcat(szAbsFilePath, pstEntry->d_name);
         
         fp = fopen(szAbsFilePath, "r");
      
         if (DOS_ADDR_INVALID(fp))
         {
            logr_cirt("%s:Line %d:mon_get_proc_pid_list|file \'%s\' access failed,fp is %p!"
                        , dos_get_filename(__FILE__), __LINE__, szAbsFilePath, fp);
            closedir(pstDir);
            return DOS_FAIL;
         }

         fseek(fp, 0, SEEK_SET);
         if (NULL != fgets(szLine, sizeof(szLine), fp))
         {
            S32 lRet = 0;
            if(DOS_ADDR_INVALID(g_pastProc[g_ulPidCnt]))
            {
               logr_cirt("%s:Line %d:mon_get_proc_pid_list|get proc pid list failure,g_pastProc[%d] is %p!"
                            , dos_get_filename(__FILE__),  __LINE__, g_ulPidCnt, g_pastProc[g_ulPidCnt]);
               goto failure;
            }
         
            lRet = dos_atoul(szLine, &(g_pastProc[g_ulPidCnt]->ulProcId));
            if(0 != lRet)
            {
               logr_error("%s:Line %u:mon_get_proc_pid_list|dos_atol failure,lRet is %d!"
                            , dos_get_filename(__FILE__), __LINE__, lRet);
               goto failure;
            }

            if(mon_is_proc_dead(g_pastProc[g_ulPidCnt]->ulProcId))
            {//���˵������ڵĽ���
               logr_error("%s:Line %u:mon_get_proc_pid_list|the pid %u is invalid or not exist!"
                            , dos_get_filename(__FILE__), __LINE__, g_pastProc[g_ulPidCnt]->ulProcId);
               fclose(fp);
               fp = NULL;
               unlink(szAbsFilePath);
               continue;
            }
            
            pTemp = mon_get_proc_name_by_id(g_pastProc[g_ulPidCnt]->ulProcId, szProcAllName);
            if(DOS_ADDR_INVALID(pTemp))
            {
               logr_error("%s:Line %u:mon_get_proc_pid_list|get proc name by pid failure, pTemp is %p!"
                            , dos_get_filename(__FILE__), __LINE__, pTemp);
               goto failure;
            }
            
            dos_strcpy(g_pastProc[g_ulPidCnt]->szProcName, pTemp);
            (g_pastProc[g_ulPidCnt]->szProcName)[dos_strlen(pTemp)] = '\0';
            ++g_ulPidCnt;
         }
         else
         {
            fclose(fp);
            unlink(szAbsFilePath);
            fp = NULL;
         }
         fclose(fp);
         fp = NULL;
      } 
   }
   closedir(pstDir);

   pstDir = opendir(szDBPidDir);
   if (DOS_ADDR_INVALID(pstDir))
   {
      logr_cirt("%s:Line %u:mon_get_proc_pid_list|dir \'%s\' access failed,pstDir is %p!"
                , dos_get_filename(__FILE__), __LINE__, szDBPidDir, pstDir);
      return DOS_FAIL;
   }

   while (NULL != (pstEntry = readdir(pstDir)))
   {
      if (DT_REG == pstEntry->d_type && DOS_TRUE == mon_is_suffix_true(pstEntry->d_name, "pid"))
      {
         S8  szLine[8] = {0};
         S8  szProcAllName[64] = {0};
         S8 *pTemp;
         S8  szAbsFilePath[64] = {0};
         
         dos_strcat(szAbsFilePath, szDBPidDir);
         dos_strcat(szAbsFilePath, pstEntry->d_name);
   
         fp = fopen(szAbsFilePath, "r");      
         if (DOS_ADDR_INVALID(fp))
         {
            logr_cirt("%s:Line %u:mon_get_proc_pid_list|file \'%s\' access failure,fp is %p!"
                        , dos_get_filename(__FILE__), __LINE__, szAbsFilePath, fp);
            closedir(pstDir);
            return DOS_FAIL;
         }

         fseek(fp, 0, SEEK_SET);
         if (NULL != (fgets(szLine, sizeof(szLine), fp)))
         {
            S32 lRet = 0;
            if(DOS_ADDR_INVALID(g_pastProc[g_ulPidCnt]))
            {
                logr_cirt("%s:Line %u:mon_get_proc_pid_list|get proc pid list failure,g_pastProc[%u] is %p!"
                            , dos_get_filename(__FILE__), __LINE__ ,g_ulPidCnt, g_pastProc[g_ulPidCnt]);
                goto failure;
            }
            
            lRet = dos_atoul(szLine, &(g_pastProc[g_ulPidCnt]->ulProcId));
            if(0 != lRet)
            {
               logr_error("%s:Line %u:mon_get_proc_pid_list|dos_atol afilure, lRet is %d!"
                            , dos_get_filename(__FILE__), __LINE__, lRet);
               goto failure;
            }

            if(mon_is_proc_dead(g_pastProc[g_ulPidCnt]->ulProcId))
            {//���˵������ڵĽ���
               logr_error("%s:Line %u:mon_get_proc_pid_list|pid %u is invalid or not exist!"
                            , dos_get_filename(__FILE__), __LINE__
                            , g_pastProc[g_ulPidCnt]->ulProcId);
               fclose(fp);
               fp = NULL;
               unlink(szAbsFilePath);
               continue;
            }
            
            pTemp = mon_get_proc_name_by_id(g_pastProc[g_ulPidCnt]->ulProcId, szProcAllName);
            if(DOS_ADDR_INVALID(pTemp))
            {
               logr_error("%s:Line %u:mon_get_proc_pid_list|get proc name by id failure,pTemp is %p!"
                            , dos_get_filename(__FILE__), __LINE__, pTemp);
               goto failure;
            }
            
            dos_strcpy(g_pastProc[g_ulPidCnt]->szProcName, pTemp);
            (g_pastProc[g_ulPidCnt]->szProcName)[dos_strlen(pTemp)] = '\0';
            ++g_ulPidCnt;
         }
         else
         {
            fclose(fp);
            unlink(szAbsFilePath);
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

   ulRet = mon_get_proc_pid_list();
   if(DOS_SUCC != ulRet)
   {
      logr_error("%s:Line %u:mon_get_process_data|get proc pid list failure,ulRet == %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
      return DOS_FAIL;
   }
   
   for (ulRows = 0; ulRows < g_ulPidCnt; ulRows++)
   {
      if(DOS_ADDR_INVALID(g_pastProc[ulRows]))
      {
          logr_cirt("%s:Line %u:mon_get_process_data|get proc data failure,g_pastProc[%u] is %p!"
                    , dos_get_filename(__FILE__), __LINE__, ulRows, g_pastProc[ulRows]);
         return DOS_FAIL;
      }
      
      ulRet = mon_get_cpu_mem_time_info(g_pastProc[ulRows]->ulProcId, g_pastProc[ulRows]);
      if(DOS_SUCC != ulRet)
      {
         logr_error("%s:Line %u:mon_get_process_data|get mem & cpu & time failure,ulRet == %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
         return DOS_FAIL;
      }

      ulRet = mon_get_openfile_count(g_pastProc[ulRows]->ulProcId);
      if(DOS_FAIL == ulRet)
      {
         logr_error("%s:Line %u:mon_get_process_data|get open file count failure,ulRet == %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
         return DOS_FAIL;
      }
      g_pastProc[ulRows]->ulOpenFileCnt = ulRet;
      
      ulRet = mon_get_db_conn_count(g_pastProc[ulRows]->ulProcId);
      if(DOS_FAIL == ulRet)
      {
         logr_error("%s:Line %u:mon_get_process_data|get database connection failure,ulRet == %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
         return DOS_FAIL;
      }
      g_pastProc[ulRows]->ulDBConnCnt = ulRet;

      ulRet = mon_get_threads_count(g_pastProc[ulRows]->ulProcId);
      if(DOS_FAIL == ulRet)
      {
         logr_error("%s:Line %u:mon_get_process_data|get threads count failure,ulRet == %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
         return DOS_FAIL;
      }
      g_pastProc[ulRows]->ulThreadsCnt = ulRet;
   }

   return DOS_SUCC;
}

/**
 * ����:ɱ������
 * ��������
 *   ����1: S32 lPid  ����id
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
static U32  mon_kill_process(U32 ulPid)
{
   S8 szKillCmd[MAX_CMD_LENGTH] = {0};

   /* ʹ��"kill -9 ����id"��ʽɱ�����  */
   dos_snprintf(szKillCmd, MAX_CMD_LENGTH, "kill -9 %u", ulPid);
   system(szKillCmd);

   if (DOS_TRUE == mon_is_proc_dead(ulPid))
   {
      logr_info("%s:Line %u:mon_kill_process|kill process success,pid %u is killed!"
                , dos_get_filename(__FILE__), __LINE__, ulPid);
      return DOS_SUCC;
   }

   return DOS_FAIL;
}

/**
 * ����:�����û�е��ߵĽ��̣����������������֮
 * ��������
 *   �޲���
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
static U32  mon_check_all_process()
{
   S32 lRet = 0;
   U32 ulRows = 0;
   S8  szProcName[32] = {0};
   S8  szProcVersion[16] = {0};
   S8  szProcCmd[1024] = {0};
   U32 ulCfgProcCnt = 0;
   
   /* ��ȡ��ǰ���ý������� */
   ulCfgProcCnt = config_hb_get_process_cfg_cnt();
   if(0 > ulCfgProcCnt)
   {
      logr_error("%s:Line %u:mon_check_all_process|get config process count failure,ulCfgProcCnt is %u!"
                   , dos_get_filename(__FILE__), __LINE__, ulCfgProcCnt);
      config_hb_deinit();
      return DOS_FAIL;
   }

   /*
    *  ������õĽ��̸���С�ڻ��ߵ��ڼ�ص��Ľ��̸�����
    *  ��ô��Ϊ���еļ�ؽ��̶��Ѿ�����
    */
   if(ulCfgProcCnt <= g_ulPidCnt - 1)
   {
      logr_info("%s:Line %u:mon_check_all_process|no process lost!"
                 , dos_get_filename(__FILE__), __LINE__);
      return DOS_SUCC;
   }
      
   for (ulRows = 0; ulRows < ulCfgProcCnt; ulRows++)
   {
      U32 ulCols = 0;
      /* Ĭ��δ���� */
      S32 bHasStarted = DOS_FALSE;

      memset(szProcName, 0, sizeof(szProcName));
      memset(szProcVersion, 0, sizeof(szProcVersion));
      /* ��ȡ�������ͽ��̰汾�� */
      lRet = config_hb_get_process_list(ulRows, szProcName, sizeof(szProcName)
                 , szProcVersion, sizeof(szProcVersion));
      if(lRet < 0)
      {
         logr_error("%s:Line %u:mon_check_all_process|get process list failure!"
                     , dos_get_filename(__FILE__), __LINE__);
         config_hb_deinit();
         return DOS_FAIL;
      }

      memset(szProcCmd, 0, sizeof(szProcCmd));
      /* ��ȡ���̵��������� */
      lRet = config_hb_get_start_cmd(ulRows, szProcCmd, sizeof(szProcCmd));
      if(0 > lRet)
      {
         logr_error("%s:Line %u:mon_check_all_process|get start command failure!"
                     , dos_get_filename(__FILE__), __LINE__);
         config_hb_deinit();
         return DOS_FAIL;
      }

      for(ulCols = 0; ulCols < g_ulPidCnt; ulCols++)
      {
         S8 * pszPtr = mon_str_get_name(g_pastProc[ulCols]->szProcName);

         if (DOS_ADDR_INVALID(pszPtr))
         {
             logr_error("%s:Line %u: pszStr == %p", dos_get_filename(__FILE__), __LINE__, pszPtr);
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
         logr_info("%s:Line %u: Process \"%s\" lost,it will be restarted."
                    , dos_get_filename(__FILE__), __LINE__, g_pastProc[ulCols]->szProcName);
         lRet = system(szProcCmd);
         if(0 > lRet)
         {
            logr_error("%s:Line %u:mon_check_all_process|start process \'%s\' FAIL."
                         , dos_get_filename(__FILE__), __LINE__, szProcName);
            return DOS_FAIL;
         }
      }
   }
   return DOS_SUCC;
}


/**
 * ����:ɱ�����б���ؽ���
 * ��������
 *   �޲���
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
U32 mon_kill_all_monitor_process()
{
    U32 ulRows = 0;
    U32 ulRet = 0;
    U32 ulPid = 0;

    ulPid = getpid();
    for(ulRows = 0; ulRows < g_ulPidCnt; ulRows++)
    {
       if(ulPid == g_pastProc[ulRows]->ulProcId)
       {
          continue;
       }
       ulRet = mon_kill_process(g_pastProc[ulRows]->ulProcId);
       if(DOS_SUCC != ulRet)
       {
          logr_error("%s:Line %u:mon_kill_all_monitor_process|kill all monitor process failure,ulRet is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
          return DOS_FAIL;
       }
    }

    return DOS_SUCC;
}


/**
 * ����:���������
 * ��������
 *   �޲���
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
U32 mon_restart_computer()
{
    system("/sbin/reboot");
    return DOS_SUCC;
}


/**
 * ����:���ݽ���id��ý�����
 * ��������
 *   ����1:S32 lPid  ����id
 *   ����2:S8 * pszPidName   ������
 * ����ֵ��
 *   �ɹ��򷵻ؽ�������ʧ���򷵻�NULL
 */
S8 * mon_get_proc_name_by_id(U32 ulPid, S8 * pszPidName)
{
   S8   szPsCmd[64] = {0};
   S8   szMonFile[] = "monstring";
   S8   szLine[1024] = {0};
   S8*  pTokenPtr = NULL;
   FILE * fp = NULL;

   if(ulPid > MAX_PID_VALUE || ulPid <= MIN_PID_VALUE)
   {
      logr_emerg("%s:Line %u:mon_get_proc_name_by_id|pid %d is invalid!"
                    , dos_get_filename(__FILE__), __LINE__, ulPid);
      return NULL;
   }

   if(DOS_ADDR_INVALID(pszPidName))
   {
      logr_warning("%s:Line %u:mon_get_proc_name_by_id|pszPidName is %p!"
                    , dos_get_filename(__FILE__), __LINE__, pszPidName);
      return NULL;
   }

   dos_snprintf(szPsCmd, sizeof(szPsCmd), "ps -ef | grep %u > %s"
                ,ulPid, szMonFile);
   system(szPsCmd);

   fp = fopen(szMonFile, "r");
   if (DOS_ADDR_INVALID(fp))
   {
      logr_cirt("%s:Line %u:mon_get_proc_name_by_id|file \'%s\' open failed,fp is %p!"
                , dos_get_filename(__FILE__), __LINE__, szMonFile, fp);
      return NULL;
   }

   fseek(fp, 0, SEEK_SET);

   while(!feof(fp))
   {
      U32 ulCols = 0;
      if(NULL != fgets(szLine, sizeof(szLine), fp))
      {
         if(ulPid != mon_first_int_from_str(szLine))
         {
            continue;
         }

         pTokenPtr = strtok(szLine, " \t\n");
         while(DOS_ADDR_VALID(pTokenPtr))
         {
            pTokenPtr = strtok(NULL, " \t\n");
            if(5 == ulCols)
            {
               break;
            }
            ++ulCols;
         }

         pTokenPtr = strtok(NULL, "\t");
         *(pTokenPtr + dos_strlen(pTokenPtr) - 1) = '\0';

         fclose(fp);
         fp = NULL;
         unlink(szMonFile);

         return pTokenPtr;
      }
   }

   fclose(fp);
   fp = NULL;
   unlink(szMonFile);

   return NULL;
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
   S8 szPsCmd[MAX_CMD_LENGTH] = {0};
   S8 szPsFile[] = "procfile";
   S8 szLine[MAX_BUFF_LENGTH] = {0};
   FILE * fp;

   if(DOS_FALSE == mon_is_pid_valid(ulPid))
   {
       logr_error("%s:Line %u:mon_is_proc_dead|pid %d is invalid or not exist,ulPid = %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulPid);
       return DOS_TRUE;
   }

   dos_snprintf(szPsCmd, MAX_CMD_LENGTH, "ps aux | grep %u > %s", ulPid, szPsFile);
   system(szPsCmd);

   fp = fopen(szPsFile, "r");
   if (DOS_ADDR_INVALID(fp))
   {
      logr_cirt("%s:Line %u:mon_is_proc_dead|file \'%s\' open failed,fp is %p!"
                , dos_get_filename(__FILE__), __LINE__, szPsFile, fp);
      return DOS_FAIL;
   }

   fseek(fp, 0, SEEK_SET);
   while (!feof(fp))
   {
      if ((fgets(szLine, MAX_BUFF_LENGTH, fp)) != NULL)
      {
         if (ulPid == mon_first_int_from_str(szLine))
         {
            fclose(fp);
            fp = NULL;
            unlink(szPsFile);
            logr_warning("%s:Line %u:mon_is_proc_dead|pid %u is running"
                         , dos_get_filename(__FILE__), __LINE__, ulPid);
            return DOS_FALSE;
         }
      }
   }

   fclose(fp);
   fp = NULL;
   unlink(szPsFile);
   logr_info("%s:Line %u:mon_is_proc_dead|pid %d is dead!"
                , dos_get_filename(__FILE__), __LINE__
                , ulPid);
   return DOS_TRUE;
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
         logr_cirt("%s:Line %u:mon_get_all_proc_total_cpu_rate|get all proc total CPU rate failure,g_pastProc[%u] is %p!"
                    , dos_get_filename(__FILE__), __LINE__, ulRows, g_pastProc[ulRows]);
         return DOS_FAIL;
      }
      
      if(DOS_FALSE == mon_is_pid_valid(g_pastProc[ulRows]->ulProcId))
      {
         logr_error("%s:Line %u:mon_get_all_proc_total_cpu_rate|proc pid %u is not exist or invalid!"
                    , dos_get_filename(__FILE__), __LINE__, g_pastProc[ulRows]->ulProcId);
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
 *   �ɹ��򷵻����ڴ�ռ���ʣ�ʧ�ܷ���-1
 */
U32  mon_get_proc_total_mem_rate()
{
   F64 fTotalMemRate = 0.0;
   U32 ulRows = 0;

   for (ulRows = 0; ulRows < g_ulPidCnt; ulRows++)
   {
      if(DOS_ADDR_INVALID(g_pastProc[ulRows]))
      {
         logr_error("%s:Line %u:mon_get_all_proc_total_mem_rate|get total proc memory failure,g_pastProc[%u] is %p!"
                    , dos_get_filename(__FILE__), __LINE__, ulRows, g_pastProc[ulRows]);
         return DOS_FAIL;
      }
      
      if(DOS_FALSE == mon_is_pid_valid(g_pastProc[ulRows]->ulProcId))
      {
         logr_error("%s:Line %u:mon_get_all_proc_total_mem_rate|proc pid %u is invalid!"
                    , dos_get_filename(__FILE__), __LINE__, g_pastProc[ulRows]->ulProcId);
         continue;
      }
      fTotalMemRate += g_pastProc[ulRows]->fMemoryRate;
   }

   return (U32)(fTotalMemRate + 0.5);
}

/**
 * ����:��ȡ���м�ؽ�����Ϣ�ĸ�ʽ����Ϣ�ַ���
 * ��������
 *   �޲���
 * ����ֵ��
 *   �ɹ��򷵻�DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
U32  mon_get_process_formatted_info()
{
   U32 ulRows = 0;
   S8  szTempBuff[MAX_BUFF_LENGTH] = {0};
   U32 ulMemRate = 0;
   U32 ulCPURate = 0;
   U32 ulRet = 0;

   memset(g_szMonProcessInfo, 0, MAX_PROC_CNT * MAX_BUFF_LENGTH);

   ulRet = mon_check_all_process();
   if(DOS_SUCC != ulRet)
   {
      logr_error("%s:Line %u:mon_get_process_formatted_info|check all process failure, ulRet is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
      return DOS_FAIL;
   }

   for (ulRows = 0; ulRows < g_ulPidCnt; ulRows++)
   {
      S8 szTempInfo[MAX_BUFF_LENGTH] = {0};

      if(DOS_ADDR_INVALID(g_pastProc[ulRows]))
      {
         logr_error("%s:Line %u:mon_get_process_formatted_info|get proc formatted information failure,g_pastProc[%] is %p!"
                    , dos_get_filename(__FILE__), __LINE__, ulRows, g_pastProc[ulRows]);
         return DOS_FAIL;
      }

      if(DOS_FALSE == mon_is_pid_valid(g_pastProc[ulRows]->ulProcId))
      {
         logr_error("%s:Line %u:mon_get_process_formatted_info|proc pid %u is not exist or invalid!"
                    , dos_get_filename(__FILE__), __LINE__, g_pastProc[ulRows]->ulProcId);
         continue;
      }

      ulMemRate = mon_get_proc_total_mem_rate();
      if(DOS_FAIL == ulMemRate)
      {
         logr_error("%s:Line %u:mon_get_process_formatted_info|get all proc total memory rate failure,lMemRate is %u%%!"
                    , dos_get_filename(__FILE__), __LINE__, ulMemRate);
         return DOS_FAIL;
      }

      ulCPURate = mon_get_proc_total_cpu_rate();
      if(DOS_FAIL == ulCPURate)
      {
         logr_error("%s:Line %u:mon_get_process_formatted_info|get all proc total CPU rate failure,lCPURate is %d%%!"
                    , dos_get_filename(__FILE__), __LINE__, ulCPURate);
         return DOS_FAIL;
      }
      
      dos_snprintf(szTempInfo, MAX_BUFF_LENGTH,
          "Process Name:%s\n" \
          "Process ID:%u\n" \
          "Memory Usage Rate:%.1f%%\n" \
          "CPU Usage Rate:%.1f%%\n" \
          "Total CPU Time:%s\n" \
          "Open file count:%u\n" \
          "Database connections count:%u\n" \
          "Threads count:%u\n",
          g_pastProc[ulRows]->szProcName,
          g_pastProc[ulRows]->ulProcId,
          g_pastProc[ulRows]->fMemoryRate,
          g_pastProc[ulRows]->fCPURate,
          g_pastProc[ulRows]->szProcCPUTime,
          g_pastProc[ulRows]->ulOpenFileCnt,
          g_pastProc[ulRows]->ulDBConnCnt,
          g_pastProc[ulRows]->ulThreadsCnt);
       dos_strcat(g_szMonProcessInfo, szTempInfo);
       dos_strcat(g_szMonProcessInfo, "\n");
   }
   
   dos_snprintf(szTempBuff, MAX_BUFF_LENGTH,
          "Total Memory Usage Rate:%u%%\nTotal CPU Usage Rate:%u%%"
          , ulMemRate, ulCPURate);
   dos_strcat(g_szMonProcessInfo, szTempBuff);
   
   return DOS_SUCC;
}

#endif //end #if INCLUDE_PROC_MONITOR
#endif //end #if INCLUDE_RES_MONITOR
#endif //end #if (INCLUDE_BH_SERVER)

#ifdef __cplusplus
}
#endif

