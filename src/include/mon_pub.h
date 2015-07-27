/*
  *        (C)Copyright 2014,dipcc.CO.,LTD.
  *            ALL RIGHTS RESERVED
  *
  *        mon_pub.h
  *        Created on: 2014-12-29
  *        Author: bubble
  *        Todo: Monitoring CPU resources
  *        History:
  */

#ifndef _MON_PUB_H__
#define _MON_PUB_H__


typedef enum tagMonNotifyLevel
{
    MON_NOTIFY_LEVEL_EMERG = 0,  /* �����澯 */
    MON_NOTIFY_LEVEL_CRITI,      /* ��Ҫ�澯 */
    MON_NOTIFY_LEVEL_MINOR,      /* ��Ҫ�澯 */
    MON_NOTIFY_LEVEL_HINT,       /* ��ʾ�澯 */

    MON_NOTIFY_LEVEL_BUTT = 31   /* ��Ч�澯 */
}MON_NOTIFY_LEVEL_EN;

/* �ر�ע��: ��ö�ٵĶ���˳������ȫ�ֱ���
 * m_pstMsgMapCN�Լ�m_pstMsgMapEN
 * ��˳����ȫ��Ӧ����������
 */
typedef enum tagMonNotifyType
{
    MON_NOTIFY_TYPE_LACK_FEE = 0,  /* �˻����� */
    MON_MOTIFY_TYPE_LACK_GW,       /* �м̲����� */
    MON_NOTIFY_TYPE_LACK_ROUTE,    /* ȱ��·�� */

    MON_NOTIFY_TYPE_BUTT = 31     /* ��Чֵ */
}MON_NOTIFY_TYPE_EN;

typedef enum tagMonNofityMeans
{
    MON_NOTIFY_MEANS_EMAIL = 0,   /* �ʼ� */
    MON_NOTIFY_MEANS_SMS,         /* ���� */
    MON_NOTIFY_MEANS_AUDIO        /* ���� */
}MON_NOTIFY_MEANS_EN;


#endif //_MON_PUB_H__
