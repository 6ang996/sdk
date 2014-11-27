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

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* Ĭ�����ѭ������ 10485759�� */
#define DOS_DEFAULT_MAX_LOOP       0x9FFFFFL

/* ��ʼ��һ��ѭ������ */
#define DOS_LOOP_DETECT_INIT(__var, __limit)  \
    long __##__var = __limit;

/* ���ѭ�� */
#define DOS_LOOP_DETECT(__var)        \
(__##__var)--;                        \
if ((__##__var) <= 0)                 \
{                                     \
    logr_error("Endless loop detected. File:%s, Line:%d.", dos_get_filename(__FILE__), __LINE__); \
    break;                            \
}


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DOSDEF_H__ */

