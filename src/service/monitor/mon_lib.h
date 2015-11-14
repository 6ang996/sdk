/*
 *      (C)Copyright 2014,dipcc,CO.,LTD.
 *            ALL RIGHTS RESERVED
 *
 *       monitor.h
 *       Created on:2014-11-29
 *           Author:bubble
 *             Todo:define common simple operations
 *          History:
 */

#ifndef _MONITOR_H__
#define _MONITOR_H__

#if INCLUDE_RES_MONITOR

#define CPU_RES                   0x000000f1 //CPU��Դ
#define MEM_RES                   0x000000f2 //�ڴ���Դ
#define DISK_RES                  0x000000f3 //������Դ
#define NET_RES                   0x000000f4 //������Դ
#define PROC_RES                  0x000000f5 //������Դ

#define RES_LACK                  0x00000001 //��Դȱ��
#define TEMP_HIGH                 0x00000002 //�¶ȹ���

/* ��ɫ���� */
#define ROLE_MANAGER              15  /* ����Ա */
#define ROLE_DIRECTOR             1   /* ���� */
#define ROLE_OPERATOR             0   /* ����Ա */
#define ROLE_FINANCE              2   /* ���� */

#define MAX_PID_VALUE             65535
#define MIN_PID_VALUE             0

#define MAX_TOKEN_LEN             256 //��󳤶�
#define MAX_TOKEN_CNT             16

#include <dos/dos_types.h>
#include "mon_notification.h"
#include "../../util/heartbeat/heartbeat.h"

#define GENERATE_WARNING_MSG(pstMsg, ulIndex, ulNo) \
    pstMsg->ulWarningId = ulNo; \
    pstMsg->ulMsgLen = dos_strlen(g_pstWarningMsg[ulIndex].szWarningDesc); \
    pstMsg->msg = (VOID *)g_pstWarningMsg[ulIndex].szWarningDesc ; \
    g_pstWarningMsg[ulIndex].bExcep = DOS_TRUE

#define GENERATE_NORMAL_MSG(pstMsg, ulIndex, ulNo) \
    pstMsg->ulWarningId = ulNo; \
    pstMsg->ulMsgLen = dos_strlen(g_pstWarningMsg[ulIndex].szNormalDesc); \
    pstMsg->msg = (VOID *)g_pstWarningMsg[ulIndex].szNormalDesc; \
    g_pstWarningMsg[ulIndex].bExcep = DOS_FALSE

enum MON_WARNING_LEVEL_E
{
    MON_WARNING_EMERG = 0,     //�����澯
    MON_WARNING_IMPORTANT,     //��Ҫ�澯
    MON_WARNING_SECONDARY,     //��Ҫ�澯
    MON_WARNING_PROMPT,        //��ʾ

    MON_WARNING_BUTT = 32
};

U32  mon_get_sp_email(MON_NOTIFY_MSG_ST *pstMsg);
U32  mon_get_level(U32 ulNotifyType);
U32  mon_init_str_array();
U32  mon_deinit_str_array();
BOOL mon_is_suffix_true(S8 * pszFile, const S8 * pszSuffix);
U32  mon_first_int_from_str(S8 * pszStr);
U32  mon_analyse_by_reg_expr(S8* pszStr, S8* pszRegExpr, S8* pszRsltList[], U32 ulLen);
U32  mon_generate_warning_id(U32 ulResType, U32 ulNo, U32 ulErrType);
U32  mon_get_msg_index(U32 ulNo);


#endif //#if INCLUDE_RES_MONITOR
#endif // end of _MONITOR_H__

