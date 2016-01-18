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
    MON_TRACE_MAIL,         /* �ʼ�ģ�� */
    MON_TRACE_CONFIG,       /* �����ļ�ģ�� */

    MON_TRACE_BUTT = 32 /* ��Чģ�� */
};

typedef struct tagThreshold
{
    U32 ulMemThreshold;       //ϵͳ�ڴ����ռ����
    U32 ulCPUThreshold;       //ϵͳCPUƽ�����ռ����
    U32 ul5sCPUThreshold;     //5s CPUƽ�����ռ����
    U32 ul1minCPUThreshold;   //1min CPUƽ�����ռ����
    U32 ul10minCPUThreshold;  //10min CPUƽ�����ռ����
    U32 ulPartitionThreshold; //�������ƽ��ռ����
    U32 ulDiskThreshold;      //�������ƽ��ռ����
    U32 ulMaxBandWidth;       //���������� 90Mbps
    U32 ulProcMemThreshold;   //����ռ���ڴ����ٷֱ�
}MON_THRESHOLD_S;

typedef struct tagWarningMsg
{
    U32   ulNo;              //�澯���
    BOOL  bExcep;            //�Ƿ�����״̬
    U32   ulWarningLevel;    //�澯����
    S8    szWarningDesc[128]; //�澯����
    S8    szNormalDesc[32];  //��������
}MON_WARNING_MSG_S;

U32 mon_system(S8 *pszCmd);
VOID mon_show_cpu(U32 ulIndex);
VOID mon_show_mem(U32 ulIndex);
VOID mon_show_disk(U32 ulIndex);
VOID mon_show_netcard(U32 ulIndex);
VOID mon_show_process(U32 ulIndex);
VOID mon_trace(U32 ulTraceTarget, U8 ucTraceLevel, const S8 * szFormat, ...);

U32 mon_init_notify_list();

#endif
