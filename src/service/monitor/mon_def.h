#ifndef _MON_DEF_H_
#define _MON_DEF_H_

#include "mon_lib.h"


enum tagMonSubMod
{
    MON_TRACE_MEM = 0,      /* �ڴ���Դģ�� */
    MON_TRACE_CPU,          /* CPU��Դģ�� */
    MON_TRACE_DISK,         /* Ӳ����Դģ�� */
    MON_TRACE_NET,          /* ������Դģ�� */
    MON_TRACE_PROCESS,      /* ������Դģ�� */
    MON_TRACE_MH,           /* ����봦��ģ�� */
    MON_TRACE_PUB,          /* ��Դ��ع���ģ�� */
    MON_TRACE_LIB,          /* ��Դ��ؿ�ģ�� */
    MON_TRACE_NOTIFY,       /* �澯֪ͨģ�� */
    MON_TRACE_WARNING_MSG,  /* �澯��Ϣģ�� */

    MON_TRACE_BUTT = 32 /* ��Чģ�� */
};

enum tagSysRestartType
{
    MON_SYS_RESTART_IMMEDIATELY = 0,  /* ϵͳ�������� */
    MON_SYS_RESTART_FIXED,            /* ϵͳ��ʱ���� */
    MON_SYS_RESTART_LATER,            /* �Ժ�����(û��ҵ��ʱ) */

    MON_SYS_RESTART_BUTT = 16
}MON_SYS_RESTART_TYPE;

U32 mon_restart_system(U32 ulStyle, U32 ulTimeStamp);
U32 mon_system(S8 *pszCmd);
VOID mon_show_cpu(U32 ulIndex);
VOID mon_show_mem(U32 ulIndex);
VOID mon_show_disk(U32 ulIndex);
VOID mon_show_netcard(U32 ulIndex);
VOID mon_show_process(U32 ulIndex);

VOID mon_trace(U32 ulTraceTarget, U8 ucTraceLevel, const S8 * szFormat, ...);

U32 mon_init_notify_list();

#endif
