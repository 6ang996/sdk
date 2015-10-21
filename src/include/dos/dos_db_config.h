#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#ifndef __DOS_DB_CONFIG_H__
#define __DOS_DB_CONFIG_H__

#if INCLUDE_DB_CONFIG

enum {
    LANG_TYPE_CN   = 0,
    LANG_TYPE_EN,
    LANG_TYPE_BUTT
}SC_LANG_TYPE;

enum {
    PARAM_LANG_TYPE                = 1,
    PARAM_WEB_PORT                 = 2,
    PARAM_AUDIO_KEY                = 3,
    PARAM_TOTAL_TIME               = 4,
    PARAM_SIP_REG_INTERVAL         = 5,
    PARAM_RTP_START_PORT           = 6,
    PARAM_RTP_END_PORT             = 7,
    PARAM_ENABLE_NAT               = 8,
    PARAM_SIP_CODEC                = 9,
    PARAM_RAS_ACCESS_IP            = 10,
    PARAM_RAS_ACCESS_PORT          = 11,
    PARAM_CALL_RATE                = 12,
    PARAM_RESTART_ONTIME           = 13,
    PARAM_RESTART_ONTIME_ENABLE    = 14,
    PARAM_RESTART_CYCLE            = 15,
    PARAM_RESTART_CYCLE_ENABLE     = 16,

    PARAM_BUTT,
};

/**
 * ����: dos_db_config_get_param
 * ����: ��ȡ���ݿ����
 * ����:
 *       U32 ulIndex     : �������
 *       S8 *pszBuffer   : ����������洢��������
 *       U32 ulBufferLen : ����������洢�������泤��
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 *
 * �ú�����ʵʱ�����ݿ��ж�ȡ���ݣ����Ƶ��ʹ�ã��뽫�������棬�����Ч��
 */
U32 dos_db_config_get_param(U32 ulIndex, S8 *pszBuffer, U32 ulBufferLen);

/**
 * ����: dos_db_config_init
 * ����: ��ʼ����ģ��
 * ����:
 * ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 *
 * �ú�����ʹ��xml���û�ȡ���ݿ������ļ�����ˣ�����xml�����ļ���ʼ�����֮���ٵ��øú���
 */
U32 dos_db_config_init();


#endif /* end of INCLUDE_DB_CONFIG */

#endif /* end of __DOS_DB_CONFIG_H__ */

#ifdef __cplusplus
}
#endif /* __cplusplus */


