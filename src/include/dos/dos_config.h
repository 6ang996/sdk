/*
 *            (C) Copyright 2014, ����Ѷͨ . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  dos_config.h
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: ͳһ�����������ʹ�����Լ�ά��
 *     History:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>

#if (INCLUDE_XML_CONFIG)

/**
 * ������U32 config_hh_get_send_interval()
 * ���ܣ���ȡ�������ͼ��
 * ������
 * ����ֵ���ɹ������������.ʧ�ܷ���0
 */
U32 config_hh_get_send_interval();


/**
 * ������U32 config_hb_get_max_fail_cnt()
 * ���ܣ���ȡ�������ʧ�ܴ���
 * ������
 * ����ֵ���ɹ��������ʧ�ܴ���.ʧ�ܷ���0
 */
U32 config_hb_get_max_fail_cnt();

/**
 * ������S32 config_hb_get_treatment()
 * ���ܣ���ȡ������ʱ�Ĵ���ʽ
 * ������
 * ����ֵ���ɹ����ش���ʽ���.ʧ�ܷ��أ�1
 */
S32 config_hb_get_treatment();

/**
 * ������S8* config_get_service_root(S8 *pszBuff, U32 ulLen)
 * ���ܣ���ȡ�������ĸ�Ŀ¼
 * ������
 * 		S8 *pszBuff�� ����
 * 		U32 ulLen�����泤��
 * ����ֵ���ɹ�����buff��ָ�룬ʧ��ʱ����null
 */
S8* config_get_service_root(S8 *pszBuff, U32 ulLen);

/**
 * ������U32 config_get_db_host(S8 *pszBuff, U32 ulLen);
 * ���ܣ���ȡ���ݿ�������
 * ������
 * 		S8 *pszBuff�� ����
 * 		U32 ulLen�����泤��
 * ����ֵ���ɹ�����0.ʧ�ܷ��أ�1
 */
U32 config_get_db_host(S8 *pszBuff, U32 ulLen);

/**
 * ������U32 config_get_db_port();
 * ���ܣ���ȡ���ݿ�˿ں�
 * ������
 * ����ֵ���ɹ�����0.ʧ�ܷ��أ�1
 */
U32 config_get_db_port();

/**
 * ������U32 config_get_db_user(S8 *pszBuff, U32 ulLen);
 * ���ܣ���ȡ���ݿ�������
 * ������
 * 		S8 *pszBuff�� ����
 * 		U32 ulLen�����泤��
 * ����ֵ���ɹ�����0.ʧ�ܷ��أ�1
 */
U32 config_get_db_user(S8 *pszBuff, U32 ulLen);


/**
 * ������U32 config_get_syssrc_db_user(S8 *pszBuff, U32 ulLen);
 * ���ܣ���ȡ��Դ������ݿ�������
 * ������
 * 		S8 *pszBuff�� ����
 * 		U32 ulLen�����泤��
 * ����ֵ���ɹ�����0.ʧ�ܷ��أ�1
 */
U32 config_get_syssrc_db_user(S8 *pszBuff, U32 ulLen);


/**
 * ������U32 config_get_db_password(S8 *pszBuff, U32 ulLen);
 * ���ܣ���ȡccsys���ݿ�����
 * ������
 * 		S8 *pszBuff�� ����
 * 		U32 ulLen�����泤��
 * ����ֵ���ɹ�����0.ʧ�ܷ��أ�1
 */
U32 config_get_db_password(S8 *pszBuff, U32 ulLen);

/**
 * ������U32 config_get_syssrc_db_password(S8 *pszBuff, U32 ulLen);
 * ���ܣ���ȡ��Դ������ݿ�����
 * ������
 * 		S8 *pszBuff�� ����
 * 		U32 ulLen�����泤��
 * ����ֵ���ɹ�����0.ʧ�ܷ��أ�1
 */
U32 config_get_syssrc_db_password(S8 *pszBuff, U32 ulLen);


/**
 * ������U32 config_get_db_dbname(S8 *pszBuff, U32 ulLen);
 * ���ܣ���ȡccsys���ݿ���
 * ������
 * 		S8 *pszBuff�� ����
 * 		U32 ulLen�����泤��
 * ����ֵ���ɹ�����0.ʧ�ܷ��أ�1
 */
U32 config_get_db_dbname(S8 *pszBuff, U32 ulLen);

/**
 * ������U32 config_get_syssrc_db_dbname(S8 *pszBuff, U32 ulLen);
 * ���ܣ���ȡϵͳ��Դ���ݿ���
 * ������
 * 		S8 *pszBuff�� ����
 * 		U32 ulLen�����泤��
 * ����ֵ���ɹ�����0.ʧ�ܷ��أ�1
 */
U32 config_get_syssrc_db_dbname(S8 *pszBuff, U32 ulLen);

/**
 * ������U32 config_get_py_path(S8 *pszBuff, U32 ulLen);
 * ���ܣ���ȡPython�ű�·��
 * ������
 * 		S8 *pszBuff�� ����
 * 		U32 ulLen�����泤��
 * ����ֵ���ɹ�����0.ʧ�ܷ��أ�1
 */
U32 config_get_py_path(S8 *pszBuff, U32 ulLen);

/**
 * ������
 * U32 config_get_mysqlsock_path(S8 *pszBuff, U32 ulLen);
 * ���ܣ���ȡMySQL���ݿ�sock�ļ�·��
 * ������
 * 		S8 *pszBuff�� ����
 * 		U32 ulLen�����泤��
 * ����ֵ���ɹ�����0.ʧ�ܷ��أ�1
 */
U32 config_get_mysqlsock_path(S8 *pszBuff, U32 ulLen);

/**
 * ������U32 config_get_syssrc_writeDB(S8 *pszBuff, U32 ulLen)
 * ���ܣ���ȡ��Դ���ģ���Ƿ�д���ݿ�
 * ������
 *      S8 *pszBuff�� ����
 *      U32 ulLen�����泤��
 * ����ֵ���ɹ�����0.ʧ�ܷ��أ�1
 */
U32 config_get_syssrc_writeDB(S8 *pszBuff, U32 ulLen);

/**
 * ������U32 config_init()
 * ���ܣ� ��ʼ������ģ��
 * ������
 * ����ֵ���ɹ�����0��ʧ�ܷ��أ�1
 *
 * ˵�����ú�����ʼ������ģ�顣�����ļ�������·��˳��Ϊ��
 * 		1.ϵͳ�����ļ�Ŀ¼
 * 		2."../etc/" �ѵ�ǰĿ¼����binĿ¼���ϲ�Ŀ¼Ϊ��ǰ�����Ŀ¼���ڷ����Ŀ¼�������
 */
U32 config_init();

/**
 * ������U32 config_deinit()
 * ���ܣ� ��������ģ��
 * ������
 * ����ֵ���ɹ�����0��ʧ�ܷ��أ�1
 *
 * ˵�����ú���������֮ǰ���ᱣ�浱ǰ���õ��ļ�����������и��ģ�����ǰ����
 */
U32 config_deinit();

S32 config_save();

#if INCLUDE_BH_SERVER

/**
 * ������config_hb_get_process_list
 * ���ܣ���ȡָ��index��·���µĽ�����Ϣ
 * ������
 * 		U32 ulIndex��xml��process��·�����
 * 		S8 *pszName�������������Ž������Ļ���
 * 		U32 ulNameLen����Ž���������ĳ���
 * 		S8 *pszVersion�������������Ž��̰汾��
 * 		U32 ulVerLen����Ž��̰汾�Ż���ĳ���
 * ����ֵ��
 * 		�ɹ�����0��ʧ�ܷ��أ�1
 */
S32 config_hb_get_process_list(U32 ulIndex, S8 *pszName, U32 ulNameLen, S8 *pszVersion, U32 ulVerLen);

/**
 * ������S32 config_hb_init()
 * ���ܣ� ��ʼ������ģ�������
 * ������
 * ����ֵ���ɹ�����0��ʧ�ܷ��أ�1
 *
 * ˵�����ú�����ʼ������ģ�顣�����ļ�������·��˳��Ϊ��
 * 		1.ϵͳ�����ļ�Ŀ¼
 * 		2."../etc/" �ѵ�ǰĿ¼����binĿ¼���ϲ�Ŀ¼Ϊ��ǰ�����Ŀ¼���ڷ����Ŀ¼�������
 */
S32 config_hb_init();

/**
 * ������S32 config_hb_deinit()
 * ���ܣ� ��������ģ������
 * ������
 * ����ֵ���ɹ�����0��ʧ�ܷ��أ�1
 *
 * ˵�����ú���������֮ǰ���ᱣ�浱ǰ���õ��ļ�����������и��ģ�����ǰ����
 */
S32 config_hb_deinit();

/**
 * ������S32 config_hb_save()
 * ���ܣ���������ģ�������ļ�
 * ������
 * ����ֵ���ɹ�����0��ʧ�ܷ��أ�1
 *
 * ˵�����ú���������֮ǰ���ᱣ�浱ǰ���õ��ļ�����������и��ģ�����ǰ����
 */
S32 config_hb_save();

/**
 * ������S32 config_hb_get_process_cfg_cnt();
 * ���ܣ���ȡ���õĽ��̸���
 * ������
 * ����ֵ���ɹ����ý��̵ĸ�����ʧ�ܷ��أ�1
 *
 * ˵�����ú���������֮ǰ���ᱣ�浱ǰ���õ��ļ�����������и��ģ�����ǰ����
 */
S32 config_hb_get_process_cfg_cnt();

/**
 * ������S32 config_hb_get_start_cmd(U32 ulIndex, S8 *pszCmd, U32 ulLen);
 * ���ܣ�����������̵�����
 * ������
 * ����ֵ���ɹ�����0��ʧ�ܷ���-1
 *
 * ˵�����ú���������֮ǰ���ᱣ�浱ǰ���õ��ļ�����������и��ģ�����ǰ����
 */
S32 config_hb_get_start_cmd(U32 ulIndex, S8 *pszCmd, U32 ulLen);



/**
 * ������S32 config_threshold_mem(S32* plMem);
 * ���ܣ���ȡ�ڴ�ռ���ʷ�ֵ
 * ������
 * ����ֵ���ɹ�����0��ʧ�ܷ���-1
 *
 * ˵�����ú���������֮ǰ���ᱣ�浱ǰ���õ��ļ�����������и��ģ�����ǰ����
 */
S32 config_hb_threshold_mem(U32* pulMem);


/**
 * ������S32 config_threshold_mem(S32* plMem);
 * ���ܣ���ȡCPUռ���ʵķ�ֵ
 * ������
 * ����ֵ���ɹ�����0��ʧ�ܷ���-1
 *
 * ˵�����ú���������֮ǰ���ᱣ�浱ǰ���õ��ļ�����������и��ģ�����ǰ����
 */
S32 config_hb_threshold_cpu(U32* pulAvg, U32* pul5sAvg, U32 *pul1minAvg, U32 *pul10minAvg);


/**
 * ������S32 config_threshold_disk(S32 *plPartition, S32* plDisk);
 * ���ܣ���ȡ����ռ���ʵķ�ֵ
 * ������
 * ����ֵ���ɹ�����0��ʧ�ܷ���-1
 *
 * ˵�����ú���������֮ǰ���ᱣ�浱ǰ���õ��ļ�����������и��ģ�����ǰ����
 */
S32 config_hb_threshold_disk(U32 *pulPartition, U32* pulDisk);

/**
 * ������S32 config_hb_threshold_bandwidth(U32* pulBandWidth);
 * ���ܣ���ȡ�������ռ�ô���
 * ������
 * ����ֵ���ɹ�����0��ʧ�ܷ���-1
 *
 * ˵�����ú���������֮ǰ���ᱣ�浱ǰ���õ��ļ�����������и��ģ�����ǰ����
 */
S32 config_hb_threshold_bandwidth(U32* pulBandWidth);

/**
 * ������S32 config_hb_threshold_proc(S32* plMem);
 * ���ܣ���ȡ������Դռ���ʵķ�ֵ
 * ������
 * ����ֵ���ɹ�����0��ʧ�ܷ���-1
 *
 * ˵�����ú���������֮ǰ���ᱣ�浱ǰ���õ��ļ�����������и��ģ�����ǰ����
 */
S32 config_hb_threshold_proc(U32* pulMem);

/**
 * ������U32 config_get_shortcut_cmd(U32 ulNo, S8 *pszCtrlCmd, U32 ulLen);
 * ���ܣ���ȡctrl_panelģ���ݼ�֧�ֵ�����
 * ������
 * ����ֵ���ɹ�����0��ʧ�ܷ���-1
 *
 * ˵�����ú���������֮ǰ���ᱣ�浱ǰ���õ��ļ�����������и��ģ�����ǰ����
 */
U32 config_get_shortcut_cmd(U32 ulNo, S8 *pszCtrlCmd, U32 ulLen);

#endif //end INCLUDE_BH_SERVER

#ifdef INCLUDE_PTS

U32 config_get_pts_port1();
U32 config_get_pts_port2();
U32 config_get_web_server_port();
U32 config_get_pts_proxy_port();
U32 config_get_pts_telnet_server_port();
//U32 config_get_pts_domain(S8 *pszBuff, U32 ulLen);

#endif // end INCLUDE_PTS

#ifdef INCLUDE_PTC

U32 config_get_pts_major_domain(S8 *pszBuff, U32 ulLen);
U32 config_get_pts_minor_domain(S8 *pszBuff, U32 ulLen);
U32 config_get_pts_major_port();
U32 config_get_pts_minor_port();
U32 config_get_ptc_name(S8 *pszBuff, U32 ulLen);
U32 config_set_pts_major_domain(S8 *pszBuff);
U32 config_set_pts_minor_domain(S8 *pszBuff);
U32 config_set_pts_major_port(S8 *pszBuff);
U32 config_set_pts_minor_port(S8 *pszBuff);

#endif //end INCLUDE_PTC

#endif  //end INCLUDE_XML_CONFIG

#ifdef __cplusplus
}
#endif /* __cplusplus */

