
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

/*
 * ����: licc_get_license_loaded
 * ����: ��ѯlicense�Ƿ񱻼���
 * ����:
 * ����ֵ: ���license�Ѿ����������ؽ�����DOS_TRIE,���򷵻�DOS_FALSE
 **/
BOOL licc_get_license_loaded();

/*
 * ����: licc_save_customer_id
 * ����: ����ͻ���Ϣ
 * ����:
 *     U8 *aucData  : ���ݻ���
 *     U32 ulLength : ���ݳ���
 * ����ֵ: �ɹ�����DOS_SUCC,���򷵻�DOS_FAIL
 **/
U32 licc_save_customer_id(U8 *aucData, U32 ulLength);

/*
 * ����: licc_load_customer_id
 * ����: ����ͻ���Ϣ
 * ����:
 *     U8 *aucData  : ���ݻ���
 *     U32 ulLength : ���ݳ���
 * ����ֵ: �ɹ�����DOS_SUCC,���򷵻�DOS_FAIL
 **/
U32 licc_load_customer_id(U8 *aucData, U32 ulLength);

/*
 * ����: licc_save_license
 * ����: ����license��Ϣ
 * ����:
 *     U8 *aucData  : ���ݻ���
 *     U32 ulLength : ���ݳ���
 * ����ֵ: �ɹ�����DOS_SUCC,���򷵻�DOS_FAIL
 **/
U32 licc_save_license(U8 *auclicense, U32 ulLength);

/*
 * ����: licc_get_license_version
 * ����: ��ȡlicense�汾��
 * ����:
 *     U32 *pulVersion: license�汾����
 * ����ֵ: �ɹ�����DOS_SUCC,���򷵻�DOS_FAIL
 **/
U32 licc_get_license_version(U32 *pulVersion);

/*
 * ����: licc_get_license_expire
 * ����: ��ȡlicense�汾��
 * ����:
 *     U32 *pulExpire: license�汾����
 * ����ֵ:
 *     !!!!!!!!!!!!!
 *     �����ȡ����ʱ�䣬���ΪU32_BUTT�����Ǵ����ˣ����Ϊ0����˵��û������ʱ�䣬�������ֵ�����ʾ������ʹ�ö�������
 **/
U32 licc_get_license_expire(U32 *pulExpire);

/*
 * ����: licc_get_license_validity
 * ����: ��ȡlicense����ʹ�õ�ʱ��
 * ����:
 *     U32 *pulValidity : �洢license�ܵ���Ч��ʱ��
 *     U32 *pulTimeRemaining : licenseʣ��ʱ����������ڽ�������Ϊ0
 * ����ֵ: ���Licenseû�й��ڣ�����DOS_SUCC�����򷵻�DOS_FAIL
 * ע��:
 *      U32 *pulValidity, U32 *pulTimeRemaining �������������Ϊ��
 **/
extern U32 licc_get_license_validity(U32 *pulValidity, U32 *pulTimeRemaining);

/*
 * ����: licc_get_license_mod_validity
 * ����: ��ȡlicense���ض�ģ�����ʹ�õ�ʱ��
 * ����:
 *     U32 ulModIndex   : ģ����
 *     U32 *pulValidity : �洢ʱ��
 *     U32 *pulTimeRemaining : ʣ��ʱ�䣬������ڽ�������Ϊ0
 * ����ֵ: ���Licenseû�й��ڣ�����DOS_SUCC�����򷵻�DOS_FAIL
 * ע��:
 *      U32 *pulValidity, U32 *pulTimeRemaining �������������Ϊ��
 **/
extern U32 licc_get_license_mod_validity(U32 ulModIndex, U32 *pulValidity, U32 *pulTimeRemaining);

/*
 * ����: licc_set_srv_stat
 * ����: ����ulModuleID��ָ����ģ��ĵ�ͳ��ֵ
 * ����:
 *     U32 ulModuleID:  ģ��ID
 *     U32 ulStat:      ͳ��ֵ
 * ����ֵ:
 *     �ɹ�����SUCC���Ƿ���FAIL
 **/
U32 licc_set_srv_stat(U32 ulModuleID, U32 ulStat);

#endif /* __LICENSE_H__ */

