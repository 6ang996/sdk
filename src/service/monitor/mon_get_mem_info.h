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
	S32  lPhysicalMemTotalBytes; //�����ڴ��ܴ�СKBytes 
    S32  lPhysicalMemFreeBytes;  //���������ڴ��СKBytes
    S32  lBuffers;               //Buffers��С
    S32  lCached;                //Cached��С
	S32  lSwapTotalBytes;        //Swap����������С
    S32  lSwapFreeBytes;         //Swap���з�����С
    S32  lPhysicalMemUsageRate;  //�����ڴ�ʹ����
    S32  lSwapUsageRate;         //Swap��������ռ����
}MON_SYS_MEM_DATA_S;


S32  mon_mem_malloc();
S32  mon_mem_free();
S32  mon_read_mem_file();
S32  mon_get_mem_data();
S32  mon_get_mem_formatted_info();

#endif // #if INCLUDE_RES_MONITOR  
#endif //end _MON_GET_MEM_INFO_H__

