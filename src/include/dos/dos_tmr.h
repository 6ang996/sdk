/*
 *            (C) Copyright 2014, ����Ѷͨ . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  dos_tmr.h
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: DOS��ʱ��ģ����ṩ�Ĺ���ͷ�ļ�
 *     History:
 */

#ifndef __DOS_TMR_H__
#define __DOS_TMR_H__

#if INCLUDE_SERVICE_TIMER

typedef enum tagTimerType{
    TIMER_NORMAL_NO_LOOP = 0,
    TIMER_NORMAL_LOOP,

    TIMER_NORMAL_BUTT,
}TIME_TYPE_E;

typedef enum tagTimerStatus{
    TIMER_STATUS_WAITING_ADD = 0,   /* �ڵȴ���Ӷ������� */
    TIMER_STATUS_WAITING_DEL,       /* �ڹ��������У��ȴ���ɾ�� */
    TIMER_STATUS_WAITING_RESET,     /* �ڹ��������У��ȴ��ָ� */
    TIMER_STATUS_WORKING,           /* �ȴ���ʱ */
    TIMER_STATUS_TIMEOUT,           /* ��ʱ */

    TIMER_STATUS_BUTT,
}TIMER_STATUS_E;

typedef struct tagTimerHandle{
    U64          ulData;           /* ��ʱ������ TIMER_STATUS_E*/
    U32          ulTmrStatus;      /* ��ʱ��״̬ refer to  */
    TIME_TYPE_E  eType;            /* ��ʱ������ refer to TIME_TYPE_E */
    S64          lInterval;        /* ��ʱ����ʱ��� */
    VOID         (*func)(U64 );    /* ��ʱ���ص����� */

    struct tagTimerHandle **hTmr;  /* handle���ڵ�ַ */
}TIMER_HANDLE_ST;

#define DOS_TMR_ST TIMER_HANDLE_ST*

S32 tmr_task_init();
S32 tmr_task_start();
S32 tmr_task_stop();
S32 dos_tmr_start(DOS_TMR_ST *hTmrHandle, U32 ulInterval, VOID (*func)(U64 ), U64 uLParam, U32 ulType);
S32 dos_tmr_stop(DOS_TMR_ST *hTmrHandle);

#endif

#endif

