/*
  *      (C)Copyright 2014,dipcc,CO.,LTD.
  *            ALL RIGHTS RESERVED
  *
  *       notification.h
  *       Created on:2014-12-07
  *           Author:bubble
  *             Todo:define notification method
  *          History:
  */
#include <dos/dos_types.h>
#include "mon_pub.h"

#ifndef _NOTIFICATION_H__
#define _NOTIFICATION_H__

/* �ýṹ�����������Ϣ
 * �ر�ע��:*
 *     Ϊ�˽��ⲿ�ĸ澯��Ϣ��ϵͳ��ظ澯��Ϣ��������
 *     �����ulWarningID������0x07000000��MON_NOTIFY_TYPE_EN
 *     �е�ĳһ��ֵ��λ������Ľ��.
 *     ��Ϣ�����кſɲ������ֵ��������server��ͳһ��ά��
 */
typedef struct tagMonNotifyMsg
{
    U32     ulWarningID;         /* �澯ID */
    U32     ulSeq;               /* ��Ϣ�����к� */
    U32     ulLevel;             /* �澯���� */
    U32     ulDestCustomerID;    /* Ŀ�Ŀͻ�ID */
    U32     ulDestRoleID;        /* Ŀ�Ľ�ɫID */
    time_t  ulCurTime;           /* ��Ϣ����ʱ�� */
    S8      szContact[32];       /* ��ϵ��ʽ */
    S8      szContent[256];      /* ��Ϣ���� */
}MON_NOTIFY_MSG_ST;


typedef struct tagMonMsgMap
{
    S8   *pszName;   /* ���������� */
    S8   *pszTitle;  /* �澯���� */
    S8   *pszDesc;   /* �澯���� */
}MON_MSG_MAP_ST;


typedef struct tagMonNotifyMeansCB
{
    MON_NOTIFY_MEANS_EN  ulMeans;         /* �澯��ʽ */
    U32 (*callback)(S8*, S8*, S8*);       /* �ص����� */
}MON_NOTIFY_MEANS_CB_ST;


#if INCLUDE_RES_MONITOR

U32 mon_send_sms(S8 * pszMsg, S8* pszTitle,S8 * pszTelNo);
U32 mon_send_audio(S8 * pszMsg, S8* pszTitle, S8 * pszTelNo);
U32 mon_send_email(S8* pszMsg, S8* pszTitle, S8* pszEmailAddress);

#endif //#if INCLUDE_RES_MONITOR
#endif // end _NOTIFICATION_H__
