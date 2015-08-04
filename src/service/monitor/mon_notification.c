#ifdef __cplusplus
extern "C"{
#endif

#include <dos.h>

#if (INCLUDE_BH_SERVER)
#if INCLUDE_RES_MONITOR

#include "mon_notification.h"
#include "mon_def.h"
#include "../../util/heartbeat/heartbeat.h"

/* Ŀǰ��Ϊ�����ʼ��ķ���������Ӫ�� */
#define MSGBODY "From:%s\r\n" \
                "To:%s\r\n" \
                "Content-type: text/html;charset=gb2312\r\n" \
                "Subject: %s\r\n" \
                "<p>%s</p>"

extern MON_MSG_MAP_ST m_pstMsgMap;
extern U32 mon_get_sp_email(S8 *pszEmail);

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
    S8 szSPEmail[32] = {0}, szEmailMsg[1024] = {0}, szCmdBuff[1024] = {0}, szFile[] = "mail.txt";
    U32 ulRet = 0;

    ulRet = mon_get_sp_email(szSPEmail);
    if (DOS_SUCC != ulRet)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* ��ʽ���ʼ� */
    dos_snprintf(szEmailMsg, sizeof(szEmailMsg), MSGBODY, szSPEmail, pszEmailAddress, pszTitle, pszMsg);
    /* ��ʽ������ */
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), " echo \"%s\" > %s", szEmailMsg, szFile);
    system(szCmdBuff);

    dos_memzero(szCmdBuff, sizeof(szCmdBuff));
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "sendmail -t %s < %s", pszEmailAddress, szFile);
    system(szCmdBuff);

    unlink(szFile);

    return DOS_SUCC;
}

#endif //#if INCLUDE_RES_MONITOR
#endif //#if (INCLUDE_BH_SERVER)

#ifdef __cplusplus
}
#endif

