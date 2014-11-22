/*
 *            (C) Copyright 2014, ����Ѷͨ . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  task.h
 *
 *  Created on: 2014-11-6
 *      Author: Larry
 *        Todo: �̹߳�����ض���
 *     History:
 */

#ifndef __TASK_H___
#define __TASK_H___

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

typedef enum tagThreadStatus{
    THREAD_STATUS_UNINIT = 0,
    THREAD_STATUS_INITED,
    THREAD_STATUS_RUNING,
    THREAD_STATUS_WAITING_EXIT,
}THREAD_STATUS_EN;

typedef enum tagThreadPriority{
    THREAD_PRIORITY_HEIGHT  = 0,
    THREAD_PRIORITY_MID,
    THREAD_PRIORITY_LOW,
    THREAD_PRIORITY_DEFAULT
}THREAD_PRIORITY_EN;

typedef struct tagThreadMngtCB{
    /* �̵߳ĳ�ʼ������ �� û�п��ÿ�*/
    U32 *(*init)();

    /* �̵߳������������ڸú����е���pthread_create�������Դ����߳̄1�5 */
    U32 *(*start)();

    /* �̵߳�ֹͣ������ û�п��ÿ� */
    U32 *(*stop)();

    /* �����̵߳�joinģʽ�� û�п��ÿ� */
    U32 *(*join)();

    /* �����̵߳ı��� */
    S8 *pszThreadName;
}THREAD_MNGT_CB;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
