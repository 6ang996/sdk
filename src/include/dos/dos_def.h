/*
 *            (C) Copyright 2014, ����Ѷͨ . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  sys_def.h
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: ����Dos��ز���
 *     History:
 */

#ifndef __DOSDEF_H__
#define __DOSDEF_H__

/* �������ʱ������ */
#define TIMER_MAX_NUMBER 1024

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* ������������Ϣ */
S8 *dos_get_process_name();
S8 *dos_get_process_version();
S8 *dos_get_pid_file_path(S8 *pszBuff, S32 lMaxLen);
S8 *dos_get_sys_root_path();
S32 dos_set_process_name(S8 *pszName);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
