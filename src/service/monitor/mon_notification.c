#ifdef __cplusplus
extern "C"{
#endif

#include <dos.h>

#if (INCLUDE_BH_SERVER)
#if INCLUDE_RES_MONITOR

#include "mon_notification.h"

/**
 * ����: U32 mon_send_shortmsg(S8 * pszMsg, S8 * pszTelNo)
 * ����:
 *     S8*  pszMsg    ��������
 *     S8*  pszTelNo  ���ն��ŵĺ���
 * ����: ���Ͷ���
 */
U32 mon_send_shortmsg(S8 * pszMsg, S8 * pszTelNo)
{
    return DOS_SUCC;
}

/**
 * ����: U32 mon_dial_telephone(S8 * pszMsg, S8 * pszTelNo)
 * ����:
 *     S8* pszMsg    �绰����
 *     S8* pszTelNo  �绰����
 * ����:
 *     �ɹ�����DOS_SUCC,��֮����DOS_FAIL
 */
U32 mon_dial_telephone(S8 * pszMsg, S8 * pszTelNo)
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
U32 mon_send_audio(S8 * pszMsg, S8 * pszWechatNo)
{
    return DOS_SUCC;
}

/**
 * ����:U32 mon_send_web(S8 * pszMsg, S8 * pszURL)
 * ����:
 *     S8 * pszMsg  ��Ϣ����
 *     S8 * pszURL  Web��URL
 * ����:
 *   �ɹ�����DOS_SUCC,��֮����DOS_FAIL
 */
U32 mon_send_web(S8 * pszMsg, S8 * pszURL)
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

