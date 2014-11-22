/**            (C) Copyright 2014, ����Ѷͨ . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  config_api.c
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: ʵ�ֲ���xml��ʽ�����ļ���api������mini��xml���װ
 *              ��api��װΪ�̰߳�ȫ�����ⲿֱ��ʹ��.
 *              �ⲿû��һ��xml�ĵ���Ҫ����dos���ڵ㡣
 *     History:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */
#include <dos.h>
#include <pthread.h>
#include "mxml.h"

#if (INCLUDE_XML_CONFIG)

static pthread_mutex_t g_mutexXMLLock = PTHREAD_MUTEX_INITIALIZER;

/**
 * ������mxml_node_t * _config_init(S8 *pszFile)
 * ���ܣ���ʼ��xml�����ļ�ģ�飬��ʼ��xmldos��
 * ������
 *      S8 *pszFile �����ļ��ļ���
 * ����ֵ���ɹ�����dom��ָ�룬ʧ�ܷ���null
 *  */
mxml_node_t * _config_init(S8 *pszFile)
{
    FILE *fp;
    mxml_node_t *pstXMLRoot;

    if (!pszFile || '\0' == pszFile[0])
    {
        dos_printf("%s", "Please special the configuration file.");
        return NULL;
    }

    fp = fopen(pszFile, "r");
    if (!fp)
    {
        dos_printf("%s", "Open configuration file fail.");
        return NULL;
    }

    pthread_mutex_lock(&g_mutexXMLLock);
    pstXMLRoot = mxmlLoadFile(NULL, fp, MXML_TEXT_CALLBACK);
    pthread_mutex_unlock(&g_mutexXMLLock);

    fclose(fp);

    return pstXMLRoot;
}

/**
 * ������U32 _config_deinit(mxml_node_t *pstXMLRoot)
 * ������
 *      mxml_node_t *pstXMLRoot����Ҫ�����ٵ�xml��
 * ���ܣ�����xml��
 * ����ֵ���ɹ�����0��ʧ�ܷ��أ�1
 */
U32 _config_deinit(mxml_node_t *pstXMLRoot)
{
    pthread_mutex_lock(&g_mutexXMLLock);
    if (pstXMLRoot)
    {
        mxmlDelete(pstXMLRoot);
        pstXMLRoot = NULL;
    }
    pthread_mutex_unlock(&g_mutexXMLLock);

    return 0;
}

/**
 * ������U32 _config_save(mxml_node_t *pstXMLRoot, S8 *pszFile)
 * ���ܣ���pstXMLRootָ����xml dom�����浽pszFileָ�����ļ����ļ���
 * ������
 *      mxml_node_t *pstXMLRoot����Ҫ����Ķ���
 *      S8 *pszFile���ļ���
 * ����ֵ���ɹ�����0��ʧ�ܷ��أ�1
 */
U32 _config_save(mxml_node_t *pstXMLRoot, S8 *pszFile)
{
    FILE *fp;

    if (!pszFile || '\0' == pszFile[0])
    {
        dos_printf("%s", "Please special the configuration file.");
        return -1;
    }

    pthread_mutex_lock(&g_mutexXMLLock);
    if (!pstXMLRoot)
    {
        dos_printf("%s", "The XML dom is empty.");
        pthread_mutex_unlock(&g_mutexXMLLock);
        return -1;
    }

    fp = fopen(pszFile, "w");
    if (!fp)
    {
        dos_printf("%s", "Open configuration file fail.");
        pthread_mutex_unlock(&g_mutexXMLLock);
        return -1;
    }

    mxmlSaveFile(pstXMLRoot, fp, NULL);
    pthread_mutex_unlock(&g_mutexXMLLock);

    fclose(fp);

    return 0;
}

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
S8  *_config_get_param(mxml_node_t *pstXMLRoot, S8 *path, S8 *name, S8 *szBuff, U32 ulLen)
{
    mxml_node_t *node;
    const S8 *pValue;

    if (!path || '\0' == path[0])
    {
        dos_printf("%s", "Please special the path.");
        goto error1;
    }

    if (!name || '\0' == name[0])
    {
        dos_printf("%s", "Please special the node name.");
        goto error1;
    }

    if (!szBuff || 0 == ulLen)
    {
        dos_printf("%s", "Please special the node name.");
        goto error1;
    }

    /* ����·�� */
    pthread_mutex_lock(&g_mutexXMLLock);

    if (!pstXMLRoot)
    {
        dos_printf("%s", "Please special the dom root.");
        goto error2;
    }

    node = mxmlFindPath(pstXMLRoot, path);
    if (!node)
    {
        dos_printf("Cannot read the xml root element for the path:%s.", path);
        goto error2;
    }

    /* ����·���µĽڵ� */
    node = mxmlFindElement(node, pstXMLRoot, "param", "name", name, MXML_DESCEND);
    if (!node)
    {
        dos_printf("Cannot read the xml node which has a name %s.", name);
        goto error2;
    }

    /* ��ȡֵ */
    pValue = mxmlElementGetAttr(node, "value");
    /* ����ȷʵû��ֵ��Ҳ��Ҫ���سɹ� */
    if (!pValue || '\0' == pValue[0])
    {
        szBuff[0] = '\0';
        goto success;
    }
    if (strlen(pValue) > ulLen)
    {
        szBuff[0] = '\0';
        goto error2;
    }

    strncpy(szBuff, pValue, ulLen);
    szBuff[ulLen -1] = '\0';

success:
    pthread_mutex_unlock(&g_mutexXMLLock);
    return szBuff;

error2:
    pthread_mutex_unlock(&g_mutexXMLLock);
error1:
    return NULL;
}

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
U32 _config_set_param(mxml_node_t *pstXMLRoot, S8 *path, S8 *name, S8 *value)
{
    mxml_node_t *node;

    if (!pstXMLRoot)
    {
        dos_printf("%s", "Please special the dom tree.");
        return -1;
    }

    if (!path || '\0' == path[0])
    {
        dos_printf("%s", "Please special the path.");
        return -1;
    }

    if (!name || '\0' == name[0])
    {
        dos_printf("%s", "Please special the node name.");
        return -1;
    }

    if (!value)
    {
        dos_printf("%s", "Please special the node value.");
        return -1;
    }

    pthread_mutex_lock(&g_mutexXMLLock);
    node = mxmlFindPath(pstXMLRoot, path);
    if (!node)
    {
        dos_printf("Cannot read the xml root element for the path:%d.", path);
        pthread_mutex_unlock(&g_mutexXMLLock);
        return -1;
    }

    /* ��һ�����·���£����������Ƿ���ڣ���������ھ�Ҫ����һ����������ھ�ֱ������ֵ */
    node = mxmlFindElement(node, pstXMLRoot, "param", "name", name, MXML_DESCEND);
    if (!node)
    {
        node = mxmlNewElement(node, "param");
        mxmlElementSetAttr(node, "name", name);
        mxmlElementSetAttr(node, "value", value);
    }
    else
    {
        mxmlElementSetAttr(node, "value", value);
    }
    pthread_mutex_unlock(&g_mutexXMLLock);

    return 0;
}

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
