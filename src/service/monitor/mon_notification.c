#ifdef __cplusplus
extern "C"{
#endif

#include <dos.h>

#if (INCLUDE_BH_SERVER)
#if INCLUDE_RES_MONITOR

#include "mon_notification.h"

/**
 * ����:���Ͷ���֪ͨ�ͻ�
 * ��������
 *     ����1:S8 * pszMsg   ��Ҫ���͵Ķ�������
 *     ����2:S8 * pszTelNo �绰����
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
U32 mon_send_shortmsg(S8 * pszMsg, S8 * pszTelNo)
{
   /* Ŀǰ�������û�з�װ�ã�������ʵ�� */
   return DOS_SUCC;
}

/**
 * ����:����绰��ʽ֪ͨ�û�
 * ��������
 *     ����1:S8 * pszMsg    ��Ҫ����绰���ı�����
 *     ����2:S8 * pszTelNo  �绰����
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
U32 mon_dial_telephone(S8 * pszMsg, S8 * pszTelNo)
{
   /* Ŀǰ�������û�з�װ�ã�������ʵ�� */
   return DOS_SUCC;
}

/**
 * ����:����������ʽ֪ͨ�û�
 * ��������
 *     ����1:S8 * pszMsg       ��Ҫ�����������ı�������
 *     ����2:S8 * pszWechatNo  ΢���˺�
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
U32 mon_send_audio(S8 * pszMsg, S8 * pszWechatNo)
{
   /* Ŀǰ�������û�з�װ�ã�������ʵ�� */
   return DOS_SUCC;
}

/**
 * ����:����Web�澯��ʽ֪ͨ�û�
 * ��������
 *     ����1:S8 * pszMsg  ��Ҫ���͵�Web�澯����
 *     ����2:S8 * pszURL  Web��URL
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
U32 mon_send_web(S8 * pszMsg, S8 * pszURL)
{
   /* Ŀǰ�������û�з�װ�ã�������ʵ�� */
   return DOS_SUCC;
}

/**
 * ����:�����ʼ���ʽ֪ͨ�û�
 * ��������
 *     ����1:S8 * pszMsg          ��Ҫ���͵ĵ����ʼ�����
 *     ����2:S8 * pszEmailAddress �����ʼ���ַ
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
U32 mon_send_email(S8 * pszMsg, S8 * pszEmailAddress)
{
   /* Ŀǰ�������û�з�װ�ã�������ʵ�� */
   return DOS_SUCC;
}

#endif //#if INCLUDE_RES_MONITOR
#endif //#if (INCLUDE_BH_SERVER)

#ifdef __cplusplus
}
#endif

