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

#define MAX_PID_VALUE             65535
#define MIN_PID_VALUE             0

#define MAX_TOKEN_LEN             256 //��󳤶�
#define MAX_TOKEN_CNT             16

#include <dos/dos_types.h>
#include "mon_notification.h"


enum MON_WARNING_LEVEL_E
{
    MON_WARNING_EMERG = 0,     //�����澯
    MON_WARNING_IMPORTANT,     //��Ҫ�澯
    MON_WARNING_SECONDARY,     //��Ҫ�澯
    MON_WARNING_PROMPT,        //��ʾ

    MON_WARNING_BUTT = 32
};

typedef struct tagWarningMsg
{
    U32   ulNo;              //�澯���
    BOOL  bExcep;            //�Ƿ�����״̬
    U32   ulWarningLevel;    //�澯����
    S8    szWarningDesc[32]; //�澯����
    S8    szNormalDesc[32];  //��������
}MON_WARNING_MSG_S;


typedef struct tagMonContact
{
    S8  szEmail[32];
    S8  szTelNo[16];
    S8  szContact[16];
}MON_CONTACT_ST;

U32  mon_get_contact(U32 ulCustomerID, U32 ulRoleID, MON_CONTACT_ST *pstContact);
S32  mon_get_contact_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames);
U32  mon_get_balance(U32 ulCustomerID, U64* puLBalance);
S32  mon_get_balance_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames);
U32  mon_get_level(U32 ulNotifyType);
U32  mon_init_str_array();
U32  mon_deinit_str_array();
BOOL mon_is_substr(S8 * pszSrc, S8 * pszDest);
BOOL mon_is_ended_with(S8 * pszSentence, const S8 * pszStr);
BOOL mon_is_suffix_true(S8 * pszFile, const S8 * pszSuffix);
U32  mon_first_int_from_str(S8 * pszStr);
U32  mon_analyse_by_reg_expr(S8* pszStr, S8* pszRegExpr, S8* pszRsltList[], U32 ulLen);
U32  mon_generate_warning_id(U32 ulResType, U32 ulNo, U32 ulErrType);
S8 * mon_str_get_name(S8 * pszCmd);


#endif //#if INCLUDE_RES_MONITOR
#endif // end of _MONITOR_H__

