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
DLLEXPORT VOID dos_task_delay(U32 ulMSec)
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
DLLEXPORT void dos_clean_watchdog()
{

}

DLLEXPORT VOID dos_random_init()
{
    srand((unsigned)time( NULL ));
}

DLLEXPORT U32 dos_random(U32 ulMax)
{
    return rand() % ulMax;
}

DLLEXPORT U32 dos_random_range(U32 ulMin, U32 ulMax)
{
    U32 ulRand = 0;
    S32 i;

    if (ulMin >= ulMax)
    {
        DOS_ASSERT(0);
        return U32_BUTT;
    }

    i = 1000;
    while (i-- > 0)
    {
        ulRand = rand() % ulMax;

        if (ulRand >= ulMin && ulRand < ulMax)
        {
            break;
        }
    }

    if (ulRand < ulMin || ulRand >= ulMax)
    {
        DOS_ASSERT(0);
        return U32_BUTT;
    }

    return ulRand;
}



#ifdef __cplusplus
}
#endif /* __cplusplus */


