
#ifndef __LICENSE_H__
#define __LICENSE_H__

/*
 * ����: U32 licc_init();
 * ����: ��ʼ��license�ͻ���
 * ����:
 * ����ֵ: �ɷ���DOS_SUCC��ʧ�ܷ���DOS_FAIL
 *   !!!!!!�������ʧ�ܽ�����ֹӦ�ó���ִ��
 **/
extern U32 licc_init();

/*
 * ����: licc_get_limitation
 * ����: ��ȡLICENSE��ĳ��ģ�������
 * ����:
 *     U32 ulModIndex      : ģ����
 *     U32 *pulLimitation  : �����������ģ�������
 * ����ֵ: �ɷ���DOS_SUCC��ʧ�ܷ���DOS_FAIL
 **/
extern U32 licc_get_limitation(U32 ulModIndex, U32 *pulLimitation);

/*
 * ����: licc_get_default_limitation
 * ����: ��ȡLICENSE��ĳ��ģ���Ĭ������
 * ����:
 *     U32 ulModIndex      : ģ����
 *     U32 *pulLimitation  : �����������ģ���Ĭ������
 * ����ֵ: �ɷ���DOS_SUCC��ʧ�ܷ���DOS_FAIL
 **/
extern U32 licc_get_default_limitation(U32 ulModIndex, U32 *pulLimitation);

/*
 * ����: licc_get_expire_check
 * ����: ��ȡLICENSE��ĳ��ģ���ʱ������
 * ����:
 *     U32 ulModIndex      : ģ����
 * ����ֵ: ���Licenseû�й��ڣ�����DOS_SUCC�����򷵻�DOS_FAIL
 **/
extern U32 licc_get_expire_check(U32 ulModIndex);

/*
 * ����: licc_get_machine
 * ����: ��ȡ������
 * ����:
 *     S8 *pszMachine, �������������������
 *     S32 *plLength, �����������, ���뻺����󳤶ȣ����������볤��
 * ����ֵ: �ɷ���DOS_SUCC��ʧ�ܷ���DOS_FAIL
 **/
extern U32 licc_get_machine(S8 *pszMachine, S32 *plLength);

/*
 * ����: lic_get_licc_version
 * ����: ��ȡlicense�ͻ��˰汾��
 * ����:
 *     U32 *pulVersion, ����������洢ת��Ϊ�����İ汾��
 * ����ֵ: �ɷ���DOS_SUCC��ʧ�ܷ���DOS_FAIL
 **/
U32 lic_get_licc_version(U32 *pulVersion);

/*
 * ����: lic_get_licc_version_str
 * ����: ��ȡlicense�ͻ��˰汾��
 * ����:
 * ����ֵ: �ɷ����ַ�����ʽ�İ汾��
 **/
S8* lic_get_licc_version_str();



#endif /* __LICENSE_H__ */

