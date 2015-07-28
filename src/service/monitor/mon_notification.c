#ifdef __cplusplus
extern "C"{
#endif

#include <dos.h>

#if (INCLUDE_BH_SERVER)
#if INCLUDE_RES_MONITOR

#include "mon_notification.h"
#include "mon_def.h"

/* �������İ�������ʽ�����ݵ�ӳ�� */
MON_MSG_MAP_ST m_pstMsgMapCN[] = {
            {"lack_fee",   "����", "�𾴵��û�%s����ֹ%04u-%02u-%02u %02u:%02u:02u,������Ϊ%s,���ֵ. ������:%x"},
            {"lack_gw",    "�м̲���", "�𾴵��û�%s,,�����м�����Ϊ%u,�������м�,����Ӱ��ҵ��,������:%x"},
            {"lack_route", "·�ɲ���", "�𾴵��û�%s,����·������Ϊ%u��������·������Ӱ��ҵ��,������:%x"}
        };

/* ����Ӣ�İ�������ʽ�����ݵ�ӳ��� */
MON_MSG_MAP_ST m_pstMsgMapEN[] = {
        {"lack_fee",   "No enough balance",  "Dear %s, by the time %04u-%02u-%02u %02u:%02u:02u,your balance is %.2f,please recharge, thank you! errno:%x."},
        {"lack_gw",    "No enough gateway",  "Dear %s,you have %u gateway(s) in total,please buy new ones to avoid your services being affacted,thank you! errno:%x."},
        {"lack_route", "No enough route",    "Dear %s,you have %u route(s) in total,please buy new ones to avoid your services being affacted,thank you! errno:%x."}
     };

/* �澯�ֶ�����Ϊӳ��� */
MON_NOTIFY_MEANS_CB_ST m_pstNotifyMeansCB[] = {
        {MON_NOTIFY_MEANS_EMAIL, mon_send_email},
        {MON_NOTIFY_MEANS_SMS,   mon_send_sms},
        {MON_NOTIFY_MEANS_AUDIO, mon_send_audio}
     };


/**
 * ����: U32 mon_send_sms(S8 * pszMsg, S8 * pszTelNo)
 * ����:
 *     S8*  pszMsg    ��������
 *     S8*  pszTitle
 *     S8*  pszTelNo  ���ն��ŵĺ���
 * ����: ���Ͷ���
 */
U32 mon_send_sms(S8 * pszMsg, S8* pszTitle,S8 * pszTelNo)
{
    return DOS_SUCC;
}


/**
 * ����: U32 mon_send_audio(S8 * pszMsg, S8 * pszWechatNo)
 * ����:
 *     S8 * pszMsg       ��������
 *     S8 * pszWechatNo  ΢�ź���
 * ����:
 *   �ɹ�����DOS_SUCC,��֮����DOS_FAIL
 */
U32 mon_send_audio(S8 * pszMsg, S8* pszTitle, S8 * pszTelNo)
{
    return DOS_SUCC;
}


/**
 * ����:U32 mon_send_email(S8* pszMsg, S8* pszTitle,S8* pszEmailAddress)
 * ����:
 *     S8 * pszMsg          �ʼ�����
 *     S8 * pszTitle        �ʼ�����
 *     S8 * pszEmailAddress �ʼ�Ŀ�ĵ�ַ
 * ����:
 *   �ɹ�����DOS_SUCC,��֮����DOS_FAIL
 */
U32 mon_send_email(S8* pszMsg, S8* pszTitle,S8* pszEmailAddress)
{
    S8 szBuffCmd[1024] = {0};
    FILE * fp = NULL;

    if (DOS_ADDR_INVALID(pszMsg)
        || DOS_ADDR_INVALID(pszTitle)
        || DOS_ADDR_INVALID(pszEmailAddress))
    {
        DOS_ASSERT(0);
        mon_trace(MON_TRACE_NOTIFY, LOG_LEVEL_ERROR, "Msg:%p; Title:%p; Email Address:%p.", pszMsg, pszTitle, pszEmailAddress);
        return DOS_FAIL;
    }

    dos_snprintf(szBuffCmd, sizeof(szBuffCmd), "echo %s | mail -s %s %s", pszMsg, pszTitle, pszEmailAddress);
    fp = popen(szBuffCmd, "r");
    if (DOS_ADDR_INVALID(fp))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pclose(fp);
    fp = NULL;
    return DOS_SUCC;
}

#endif //#if INCLUDE_RES_MONITOR
#endif //#if (INCLUDE_BH_SERVER)

#ifdef __cplusplus
}
#endif

