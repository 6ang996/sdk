/*
  *      (C)Copyright 2014,dipcc,CO.,LTD.
  *            ALL RIGHTS RESERVED
  *
  *       monitor.h
  *       Created on:2014-12-09
  *           Author:bubble
  *             Todo:define message queue operations
  *          History:
  */


#ifndef _MON_WARNING_MSG_QUEUE_H__
#define _MON_WARNING_MSG_QUEUE_H__

#if INCLUDE_RES_MONITOR  


#include <dos/dos_types.h>

typedef struct tagMessage
{
   U32     ulWarningId; //�澯id
   U32     ulMsgLen;    //��Ϣ����
   VOID *  msg;        //��Ϣ����
   struct  tagMessage * next; //��̽��
   struct  tagMessage * prior;//ǰ���ڵ�
}MON_MSG_S;

typedef struct tagMsgQueue
{
   MON_MSG_S * pstHead;//���еĶ�ͷָ��
   MON_MSG_S * pstRear;//���еĶ�βָ��
   U32 ulQueueLength;   //���еĳ���
}MON_MSG_QUEUE_S;


U32  mon_init_warning_msg_queue();
U32  mon_warning_msg_en_queue(MON_MSG_S * pstMsg);
U32  mon_warning_msg_de_queue();
BOOL mon_is_warning_msg_queue_empty();
MON_MSG_QUEUE_S * mon_get_warning_msg_queue();
U32  mon_destroy_warning_msg_queue();

#endif //#if INCLUDE_RES_MONITOR  
#endif //end _MON_WARNING_MSG_QUEUE_H__

