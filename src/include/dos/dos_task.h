/*
 *            (C) Copyright 2014, ����Ѷͨ . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  dos_task.h
 *
 *  Created on: 2014-11-6
 *      Author: Larry
 *        Todo: ��ʱ�̹߳�����غ���
 *     History:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

S8 *dos_get_localtime(U32 ulTimestamp, S8 *pszBuffer, S32 lLength);
VOID dos_task_delay(U32 ulMSec);
void dos_clean_watchdog();
DLLEXPORT VOID dos_random_init();
DLLEXPORT U32 dos_random(U32 ulMax);
DLLEXPORT U32 dos_random_range(U32 ulMin, U32 ulMax);

#ifdef __cplusplus
}
#endif /* __cplusplus */

