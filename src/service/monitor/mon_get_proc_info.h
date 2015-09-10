/*
  *        (C)Copyright 2014,dipcc.CO.,LTD.
  *            ALL RIGHTS RESERVED
  *
  *        process_info.h
  *        Created on:2014-12-03
  *            Author:bubble
  *              Todo:Monitoring process resources
  *           History:
  */


#ifndef _MON_GET_PROC_INFO_H__
#define _MON_GET_PROC_INFO_H__

#if INCLUDE_RES_MONITOR


#include <dos/dos_types.h>

#define STRING_MAX_LEN   8
#define MAX_PROC_CNT     16
#define MAX_CMD_LENGTH   64
#define MAX_BUFF_LENGTH  1024
#define MAX_PID_VALUE    65535
#define MIN_PID_VALUE    0


typedef struct tagMonProcStatus
{
    U32 ulProcId;           //����id
    S8  szProcName[64];    //������
    F64 fMemoryRate;       //�ڴ�ռ����
    F64 fCPURate;          //CPUƽ��ռ����
    S8  szProcCPUTime[32]; //CPU����ʱ��
    U32 ulOpenFileCnt;      //�򿪵��ļ�����
    U32 ulDBConnCnt;        //���ݿ�������
    U32 ulThreadsCnt;       //�߳���
}MON_PROC_STATUS_S;


U32  mon_proc_malloc();
U32  mon_proc_free();
U32  mon_get_process_data();
U32  mon_check_all_process();
U32  mon_restart_computer();
U32  mon_get_proc_name_by_id(U32 ulPid, S8 * pszPidName, U32 ulLen);
BOOL mon_is_proc_dead(U32 ulPid);
U32  mon_get_proc_total_cpu_rate();
U32  mon_get_proc_total_mem_rate();

#endif //#if INCLUDE_RES_MONITOR
#endif //end _MON_GET_PROC_INFO_H__

