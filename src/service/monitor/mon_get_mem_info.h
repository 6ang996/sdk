/*
  *        (C)Copyright 2014,dipcc.CO.,LTD.
  *            ALL RIGHTS RESERVED
  *
  *        mem_info.h
  *        Created on:2014-11-29
  *            Author:bubble
  *              Todo:Monitoring memory resources
  *           History:
  */

#ifndef _MON_GET_MEM_INFO_H__
#define _MON_GET_MEM_INFO_H__

#if INCLUDE_RES_MONITOR

#include <dos/dos_types.h>

#define MEMBER_COUNT    6     //��Ҫͳ�Ƶĳ�Ա����
#define MAX_BUFF_LENGTH 1024  //��������С

typedef struct tagMonSysMemData
{
    U32  ulPhysicalMemTotalBytes; //�����ڴ��ܴ�СKBytes
    U32  ulPhysicalMemFreeBytes;  //���������ڴ��СKBytes
    U32  ulCached;
    U32  ulBuffers;
    U32  ulSwapTotalBytes;        //Swap����������С
    U32  ulSwapFreeBytes;         //Swap���з�����С
    U32  ulSwapUsageRate;         //Swap��������ռ����
    U32  ulPhysicalMemUsageRate;  //�����ڴ�ʹ����
}MON_SYS_MEM_DATA_S;

U32  mon_mem_malloc();
U32  mon_mem_free();
U32  mon_read_mem_file();
U32  mon_get_mem_data();
U32  mon_handle_mem_warning();

#endif // #if INCLUDE_RES_MONITOR
#endif //end _MON_GET_MEM_INFO_H__

