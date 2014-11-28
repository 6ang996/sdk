/*
 *            (C) Copyright 2014, ����Ѷͨ . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  task_lib.c
 *
 *  Created on: 2014-11-6
 *      Author: Larry
 *        Todo: ��װϵͳ��صĺ�������߿�һ������
 *     History:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>

/**
 * ����: void dos_task_delay(U32 ulMsSec)
 * ����: ʹĳһ���߳�˯�� ulMsSec����֮����ִ��
 * ����: U32 ulMSEC˯�ߵĺ�����
 * ����ֵ: void
 */
VOID dos_task_delay(U32 ulMSec)
{
    if (0 == ulMSec)
    {
        return;
    }

    usleep(ulMSec * 1000);
}

/**
 * ����: void dos_clean_watchdog()
 * ����: ������Ź�
 * ����: NULL
 * ����ֵ: void
 */
void dos_clean_watchdog()
{

}

#ifdef __cplusplus
}
#endif /* __cplusplus */


