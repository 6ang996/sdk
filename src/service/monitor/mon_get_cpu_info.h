/*
  *        (C)Copyright 2014,dipcc.CO.,LTD.
  *            ALL RIGHTS RESERVED
  *
  *        cpu_info.h
  *        Created on:2014-12-01
  *            Author:bubble
  *              Todo:Monitoring CPU resources
  *           History:
  */


#ifndef _MON_GET_CPU_INFO_H__
#define _MON_GET_CPU_INFO_H__

#if INCLUDE_RES_MONITOR

#include <dos/dos_types.h>

#define TEN_MINUTE_SAMPLE_ARRAY  120 //10min���ڵ����
#define MAX_BUFF_LENGTH          1024
#define PENULTIMATE              119 //�����ڶ�
#define COUNTDOWN_XII            108 //������ʮ��


typedef struct tagSysMonCPUTime
{
   U32  ulUser;         //�û�̬CPUʱ��
   U32  ulNice;         //niceΪ���Ľ���ռ��CPUʱ��
   U32  ulSystem;       //����ʱ��
   U32  ulIdle;         //CPU���еȴ�ʱ��
   U32  ulIowait;       //Ӳ��IO�ȴ�ʱ��
   U32  ulHardirq;      //Ӳ�ж�ʱ��
   U32  ulSoftirq;      //���ж�ʱ��
   U32  ulStealstolen;  //��������ϵͳ�������ʱ��
   U32  ulGuest;        //�ں�̬��ϵͳ����cpuռ��ʱ��
   struct tagSysMonCPUTime * next;  //ָ����һ�ڵ��ָ��
   struct tagSysMonCPUTime * prior; //ָ��ǰһ���ڵ�ָ��
}MON_SYS_CPU_TIME_S;

typedef struct tagMonCPURslt
{
  U32 ulCPUUsageRate;     //CPU����ƽ��ռ����
  U32 ulCPU5sUsageRate;   //5s��CPUƽ��ռ����
  U32 ulCPU1minUsageRate; //1min��CPUƽ��ռ����
  U32 ulCPU10minUsageRate;//10min��CPUƽ��ռ����
}MON_CPU_RSLT_S;



U32  mon_cpu_rslt_malloc();
U32  mon_cpu_rslt_free();
U32  mon_init_cpu_queue();
U32  mon_get_cpu_rslt_data();
U32  mon_cpu_queue_destroy();

#endif // end #if INCLUDE_RES_MONITOR
#endif // end of _MON_GET_CPU_INFO_H__

