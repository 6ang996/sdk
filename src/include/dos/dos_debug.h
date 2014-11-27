/*
 *            (C) Copyright 2014, ����Ѷͨ . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  dos_debug.h
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: DOS����ģ���ṩ�Ĺ�����������
 *     History:
 */

#ifndef __DOS_DEBUG_H__
#define __DOS_DEBUG_H__

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */


#define dos_printf(format, args...)  dos_vprintf(dos_get_filename(__FILE__), __LINE__, (format), ##args)

VOID dos_vprintf(const S8 *pszFile, S32 lLine, const S8 *pszFormat, ...);
VOID dos_syslog(S32 lLevel, S8 *pszLog);

#if INCLUDE_EXCEPTION_CATCH
extern VOID dos_signal_handle_reg();
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif

