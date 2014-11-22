/**            (C) Copyright 2014, ����Ѷͨ . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  config_api.h
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: config api���ṩ�Ľӿں�������
 *     History:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>
#include "mxml.h"

#if (INCLUDE_XML_CONFIG)

/* ������mxml_node_t * _config_init(S8 *pszFile)
 * ���ܣ���ʼ��xml�����ļ�ģ�飬��ʼ��xmldos��
 * ������
 *      S8 *pszFile �����ļ��ļ���
 * ����ֵ���ɹ�����dom��ָ�룬ʧ�ܷ���null
 *  */
mxml_node_t * _config_init(S8 *pszFile);

/**
 * ������U32 config_deinit(mxml_node_t *pstXMLRoot)
 * ������
 *      mxml_node_t *pstXMLRoot����Ҫ�����ٵ�xml��
 * ���ܣ�����xml��
 * ����ֵ���ɹ�����0��ʧ�ܷ��أ�1
 */
U32 _config_deinit(mxml_node_t *pstXMLRoot);

/**
 * ������U32 _config_save(mxml_node_t *pstXMLRoot, S8 *pszFile)
 * ���ܣ���pstXMLRootָ����xml dom�����浽pszFileָ�����ļ����ļ���
 * ������
 *      mxml_node_t *pstXMLRoot����Ҫ����Ķ���
 *      S8 *pszFile���ļ���
 * ����ֵ���ɹ�����0��ʧ�ܷ��أ�1
 */
U32 _config_save(mxml_node_t *pstXMLRoot, S8 *pszFile);

/**
 *  ������S8  *_config_get_param(mxml_node_t *pstXMLRoot, S8 *path, S8 *name, S8 *szBuff, U32 ulLen)
 *  ���ܣ���ȡpstXMLRootָ����dos����path·���£�������Ϊname��������ֵ
 *  ������
 *      mxml_node_t *pstXMLRoot�� ��Ҫ���ҵ�dom��
 *      S8 *path�� ��Ҫ���ҵ�·��
 *      S8 *name������������
 *      S8 *szBuff���������������ֵbuff
 *      U32 ulLen��buff�ĶԴ򳤶�
 *  ����ֵ���ɹ�ʱ��value copy��buff�У����ҷ���buff����ָ�룻ʧ��ʱ����null
 */
S8  *_config_get_param(mxml_node_t *pstXMLRoot, S8 *path, S8 *name, S8 *szBuff, U32 ulLen);

/**
 *  ������U32 _config_set_param(mxml_node_t *pstXMLRoot, S8 *path, S8 *name, S8 *value)
 *  ���ܣ�����pstXMLRootָ����dos����path·���£�������Ϊname��������ֵ
 *  ������
 *      mxml_node_t *pstXMLRoot�� ��Ҫ���ҵ�dom��
 *      S8 *path�� ��Ҫ���ҵ�·��
 *      S8 *name������������
 *      S8 *value��������ֵ
 *  ����ֵ���ط��������ֵ
 *
 *  ע�⣺�����Ҫ���õ���������ڣ��ýӿں���������ָ����Ŀ¼�´����ò�����������ֵ
 */
U32 _config_set_param(mxml_node_t *pstXMLRoot, S8 *path, S8 *name, S8 *value);

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
