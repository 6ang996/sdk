
/**
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  �ļ���: sc_sysstat.h
 *
 *  ����ʱ��: 2015��6��29��17:28:16
 *  ��    ��: Larry
 *  ��    ��: ��ȡϵͳ��Ϣ���ļ�
 *  �޸���ʷ:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#if (DOS_INCLUDE_SYS_STAT)

U32 dos_sysstat_cpu_start();
U32 dos_get_cpu_idel_percentage();

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

