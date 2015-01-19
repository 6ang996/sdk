/*
  *        (C)Copyright 2014,dipcc.CO.,LTD.
  *            ALL RIGHTS RESERVED
  *
  *        disk_info.h
  *        Created on:2014-12-01
  *            Author:bubble
  *              Todo:Monitoring disk resources
  *           History:
  */


#ifndef _MON_GET_DISK_INFO_H__
#define _MON_GET_DISK_INFO_H__

#if INCLUDE_RES_MONITOR  

#include <dos/dos_types.h>

#define MAX_PARTITION_COUNT   16 //������������
#define MAX_PARTITION_LENGTH  64
#define MAX_RAID_SERIAL_LEN   255
#define MAX_CMD_LENGTH        64
#define MAX_BUFF_LENGTH       1024


typedef struct tagMonSysPartData
{
    S8   szPartitionName[MAX_PARTITION_LENGTH];  //������
    S32  lPartitionAvailBytes; //���������ֽ���
    S32  lPartitionUsageRate;  //����ʹ����
    S8   szDiskSerialNo[MAX_PARTITION_LENGTH]; //���ڴ������к�
}MON_SYS_PART_DATA_S;

S32 mon_disk_malloc();
S32  mon_disk_free();
S32  mon_get_total_disk_usage_rate();
S32 mon_get_partition_data();
S32  mon_get_partition_formatted_info();
S32  mon_get_total_disk_kbytes();

#endif //#if INCLUDE_RES_MONITOR  
#endif //end of _MON_GET_DISK_INFO_H__

