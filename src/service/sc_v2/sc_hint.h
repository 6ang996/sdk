/**
 * @file : sc_hint.h
 *
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 * ҵ�����ģ�������ع��ܶ���
 *
 *
 * @date: 2016��1��9��
 * @arthur: Larry
 */

#ifdef __cplusplus
extern "C" {
#endif /* End of __cplusplus */

#ifndef __SC_HINT_H__
#define __SC_HINT_H__

#define SC_HINT_FILE_PATH       "/usr/local/freeswitch/sounds/okcc"


#define SC_HINT_LANG_CN         1

typedef enum tagSoundType{
    SC_SND_CALL_OVER         = 0, /**< �����ʾ�� */
    SC_SND_INCOMING_CALL_TIP,     /**< ��ǩʱ��������ʾ�� */
    SC_SND_LOW_BALANCE,           /**< �������Ѳ��� */
    SC_SND_YOUR_BALANCE,          /**< ������Ϊ */
    SC_SND_MUSIC_HOLD,            /**< ������ */
    SC_SND_MUSIC_SIGNIN,          /**< ��ǩ���� */
    SC_SND_NETWORK_FAULT,         /**< ������� */
    SC_SND_NO_PERMISSION,         /**< ��ҵ��Ȩ�� */
    SC_SND_SET_SUCC,              /**< ���óɹ� */
    SC_SND_SET_FAIL,              /**< ����ʧ�� */
    SC_SND_OPT_SUCC,              /**< �������� */
    SC_SND_OPT_FAIL,              /**< ����ʧ�� */
    SC_SND_SYS_MAINTAIN,          /**< ϵͳά�� */
    SC_SND_TMP_UNAVAILABLE,       /**< ����������û���ʱ�޷���ͨ�����Ժ��ٲ� */
    SC_SND_USER_BUSY,             /**< ����������û�æ�����Ժ��ٲ� */
    SC_SND_USER_LINE_FAULT,       /**< �û���æ */
    SC_SND_USER_NOT_FOUND,        /**< ����������û�æ�����ڣ����֤���ٲ� */
    SC_SND_CONNECTING,            /**< ����Ϊ���ͨ */
    SC_SND_1_YAO,
    SC_SND_0,
    SC_SND_1,
    SC_SND_2,
    SC_SND_3,
    SC_SND_4,
    SC_SND_5,
    SC_SND_6,
    SC_SND_7,
    SC_SND_8,
    SC_SND_9,

    SC_SND_BUTT
}SC_SOUND_TYPE_EN;

typedef enum tagToneType{
    SC_TONE_RINGBACK,
    SC_TONE_DIAL,
    SC_TONE_RING,

    SC_TONE_BUTT
}SC_TONE_TYPE_EN;

typedef struct tagSCHintDesc{
    U32 ulIndex;
    S8  *pszName;
}SC_HINT_DESC_ST;


S8 *sc_hint_get_name(U32 ulSndInd);
S8 *sc_hine_get_tone(U32 ulSNDInd);

#endif

#ifdef __cplusplus
}
#endif /* End of __cplusplus */

