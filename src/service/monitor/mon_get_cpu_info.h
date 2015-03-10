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
   S32  lUser;         //�û�̬CPUʱ��
   S32  lNice;         //niceΪ���Ľ���ռ��CPUʱ�� 
   S32  lSystem;       //����ʱ��
   S32  lIdle;         //CPU���еȴ�ʱ��
   S32  lIowait;       //Ӳ��IO�ȴ�ʱ��
   S32  lHardirq;      //Ӳ�ж�ʱ��
   S32  lSoftirq;      //���ж�ʱ��
   S32  lStealstolen;  //��������ϵͳ�������ʱ��
   S32  lGuest;        //�ں�̬��ϵͳ����cpuռ��ʱ��
   struct tagSysMonCPUTime * next;  //ָ����һ�ڵ��ָ��
   struct tagSysMonCPUTime * prior; //ָ��ǰһ���ڵ�ָ��
}MON_SYS_CPU_TIME_S;

typedef struct tagMonCPURslt
{
  S32 lCPUUsageRate;     //CPU����ƽ��ռ����
  S32 lCPU5sUsageRate;   //5s��CPUƽ��ռ����
  S32 lCPU1minUsageRate; //1min��CPUƽ��ռ����
  S32 lCPU10minUsageRate;//10min��CPUƽ��ռ����
}MON_CPU_RSLT_S;



S32  mon_cpu_rslt_malloc();
S32  mon_cpu_rslt_free();
S32  mon_init_cpu_queue();
S32  mon_get_cpu_rslt_data();
S32  mon_get_cpu_rslt_formatted_info();
S32  mon_cpu_queue_destroy();

#endif // end #if INCLUDE_RES_MONITOR  
#endif // end of _MON_GET_CPU_INFO_H__

