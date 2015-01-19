/*
  *        (C)Copyright 2014,dipcc.CO.,LTD.
  *            ALL RIGHTS RESERVED
  *
  *        network_info.h
  *        Created on:2014-12-04
  *            Author:bubble
  *              Todo:Monitoring process resources
  *           History:
  */
 
#ifndef _MON_GET_NET_INFO_H__
#define _MON_GET_NET_INFO_H__

#if INCLUDE_RES_MONITOR  

#include <dos/dos_types.h>

#define MAX_PID_LENGTH   8
#define MAX_NETCARD_CNT  8
#define MAX_CMD_LENGTH   64
#define MAX_BUFF_LENGTH  1024

typedef struct tagNetcardParam //������Ϣ
{
   S8  szNetDevName[32];    //�����豸����
   S8  szMacAddress[32];    //�豸MAC��ַ
   S8  szIPAddress[32];     //����IPv4��ַ
   S8  szBroadIPAddress[32];//�㲥IP��ַ
   S8  szNetMask[32];       //��������
   S32 lRWSpeed;            //���ݴ����������
}MON_NET_CARD_PARAM_S;


S32  mon_netcard_malloc();
S32  mon_netcard_free();
BOOL mon_is_netcard_connected(const S8 * pszNetCard);
S32  mon_get_max_trans_speed(const S8 * pszDevName);
S32  mon_get_netcard_data();
S32  mon_netcard_formatted_info();


#endif //#if INCLUDE_RES_MONITOR  
#endif // end of _MON_GET_NET_INFO_H__