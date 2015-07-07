/*
  *      (C)Copyright 2014,dipcc,CO.,LTD.
  *            ALL RIGHTS RESERVED
  *
  *       monitor.h
  *       Created on:2014-12-06
  *       Author:bubble
  *       Todo:monitor and handle warning message
  *       History:
  */

#ifndef _MON_MONITOR_AND_HANDLE_H__
#define _MON_MONITOR_AND_HANDLE_H__

#if INCLUDE_RES_MONITOR

#include <dos/dos_types.h>

#define SQL_CMD_MAX_LENGTH 1024
#define MAX_BUFF_LENGTH    1024

typedef struct tagThreshold
{
    U32 ulMemThreshold;       //ϵͳ�ڴ����ռ����
    U32 ulCPUThreshold;       //ϵͳCPUƽ�����ռ����
    U32 ul5sCPUThreshold;     //5s CPUƽ�����ռ����
    U32 ul1minCPUThreshold;   //1min CPUƽ�����ռ����
    U32 ul10minCPUThreshold;  //10min CPUƽ�����ռ����
    U32 ulPartitionThreshold; //�������ƽ��ռ����
    U32 ulDiskThreshold;      //�������ƽ��ռ����
    U32 ulMaxBandWidth;       //���������� 90Mbps
    U32 ulProcMemThreshold;   //����ռ���ڴ����ٷֱ�
}MON_THRESHOLD_S;

VOID * mon_res_monitor(VOID *p);
VOID * mon_warning_handle(VOID *p);
U32    mon_res_alloc();
U32    mon_res_destroy();

#endif //#if INCLUDE_RES_MONITOR
#endif //end _MON_MONITOR_AND_HANDLE_H__