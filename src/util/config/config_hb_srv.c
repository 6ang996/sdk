/*
 *            (C) Copyright 2014, ����Ѷͨ . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  config_hb_srv.c
 *
 *  Created on: 2014-11-14
 *      Author: Larry
 *        Todo: ͳһ�����������ʹ�����Լ�ά��
 *     History:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* include dos public header files */
#include <dos.h>

#if INCLUDE_BH_SERVER
/* include provate header file */
#include "config_api.h"

#if (INCLUDE_XML_CONFIG)

/* provate macro */
#define HB_CONFIG_FILE_PATH1 "/etc"
#define HB_CONFIG_FILE_PATH2 "../etc"

#define HB_CONFIG_FILE_NAME "hb-srv.xml"


/* global variable */
/* ȫ�����þ�� */
static mxml_node_t *g_pstHBSrvCfg = NULL;


/**
 * ������config_hb_get_process_list
 * ���ܣ���ȡָ��index��·���µĽ�����Ϣ
 * ������
 *      U32 ulIndex��xml��process��·�����
 *      S8 *pszName�������������Ž������Ļ���
 *      U32 ulNameLen����Ž���������ĳ���
 *      S8 *pszVersion�������������Ž��̰汾��
 *      U32 ulVerLen����Ž��̰汾�Ż���ĳ���
 * ����ֵ��
 *      �ɹ�����0��ʧ�ܷ��أ�1
 */
S32 config_hb_get_process_list(U32 ulIndex, S8 *pszName, U32 ulNameLen, S8 *pszVersion, U32 ulVerLen)
{
    S8 szXmlPath[256] = { 0 };
    S8 szProcessName[DOS_MAX_PROCESS_NAME_LEN] = { 0 };
    S8 szProcessVersion[DOS_MAX_PROCESS_VER_LEN] = { 0 };
    S8 *pszValue = NULL;

    if (!pszName || 0 == ulNameLen)
    {
        DOS_ASSERT(0);
        return -1;
    }

    if (!pszVersion || 0 == ulVerLen)
    {
        DOS_ASSERT(0);
        return -1;
    }

    if (!g_pstHBSrvCfg)
    {
        DOS_ASSERT(0);
        return -1;
    }

    snprintf(szXmlPath, sizeof(szXmlPath), "config/process/%d", ulIndex);
    pszValue = _config_get_param(g_pstHBSrvCfg, szXmlPath, "name", szProcessName, sizeof(szProcessName));
    if (!pszValue || '\0' == pszValue[0])
    {
        return -1;
    }
    pszValue = _config_get_param(g_pstHBSrvCfg, szXmlPath, "version", szProcessVersion, sizeof(szProcessVersion));

    dos_strncpy(pszName, szProcessName, ulNameLen);
    pszName[ulNameLen - 1] = '\0';

    if (!pszValue || '\0' == pszValue[0])
    {
        dos_strncpy(pszVersion, szProcessVersion, ulVerLen);
        pszVersion[ulVerLen - 1] = '\0';
    }
    else
    {
        pszVersion[0] = '\0';
    }

    return 0;
}


/**
 * ������S32 config_hb_init()
 * ���ܣ� ��ʼ������ģ�������
 * ������
 * ����ֵ���ɹ�����0��ʧ�ܷ��أ�1
 *
 * ˵�����ú�����ʼ������ģ�顣�����ļ�������·��˳��Ϊ��
 *      1.ϵͳ�����ļ�Ŀ¼
 *      2."../etc/" �ѵ�ǰĿ¼����binĿ¼���ϲ�Ŀ¼Ϊ��ǰ�����Ŀ¼���ڷ����Ŀ¼�������
 */
S32 config_hb_init()
{
    S8 szBuffFilename[256];

    dos_printf("%s", "Start init hb-srv config.");

    snprintf(szBuffFilename, sizeof(szBuffFilename)
                , "%s/%s", HB_CONFIG_FILE_PATH1, HB_CONFIG_FILE_NAME);

    if (access(szBuffFilename, R_OK|W_OK) < 0)
    {
        dos_printf("Cannon find the hb-srv.xml file in system etc. (%s)", szBuffFilename);

        snprintf(szBuffFilename, sizeof(szBuffFilename)
                        , "%s/%s", HB_CONFIG_FILE_PATH2, HB_CONFIG_FILE_NAME);
        if (access(szBuffFilename, R_OK|W_OK) < 0)
        {
            dos_printf("Cannon find the hb-srv.xml file in service etc. (%s)", szBuffFilename);
            dos_printf("%s", "Cannon find the hb-srv.xml. Config init failed.");
            DOS_ASSERT(0);
            return -1;
        }
    }

    dos_printf("Use file %s as hb server config file.", szBuffFilename);

    g_pstHBSrvCfg = _config_init(szBuffFilename);
    if (!g_pstHBSrvCfg)
    {
        dos_printf("%s", "Init hb-srv.xml config fail.");
        DOS_ASSERT(0);
        return -1;
    }
    dos_printf("%s", "hb-srv config init successfully.");

    return 0;
}

/**
 * ������S32 config_hb_deinit()
 * ���ܣ� ��������ģ������
 * ������
 * ����ֵ���ɹ�����0��ʧ�ܷ��أ�1
 *
 * ˵�����ú���������֮ǰ���ᱣ�浱ǰ���õ��ļ�����������и��ģ�����ǰ����
 */
S32 config_hb_deinit()
{
    S32 lRet = 0;

    lRet = _config_deinit(g_pstHBSrvCfg);
    g_pstHBSrvCfg = NULL;

    return lRet;
}

/**
 * ������S32 config_hb_save()
 * ���ܣ���������ģ�������ļ�
 * ������
 * ����ֵ���ɹ�����0��ʧ�ܷ��أ�1
 *
 * ˵�����ú���������֮ǰ���ᱣ�浱ǰ���õ��ļ�����������и��ģ�����ǰ����
 */
S32 config_hb_save()
{
    S8 szBuffFilename[256];

    dos_printf("%s", "Start save hb server config.");

    snprintf(szBuffFilename, sizeof(szBuffFilename)
                , "%s/%s", HB_CONFIG_FILE_PATH1, HB_CONFIG_FILE_NAME);

    if (access(szBuffFilename, R_OK|W_OK) < 0)
    {
        dos_printf("Cannon find the hb-srv.xml file in system etc. (%s)", szBuffFilename);

        snprintf(szBuffFilename, sizeof(szBuffFilename)
                        , "%s/%s", HB_CONFIG_FILE_PATH2, HB_CONFIG_FILE_NAME);
        if (access(szBuffFilename, R_OK|W_OK) < 0)
        {
            dos_printf("Cannon find the hb-srv.xml file in service etc. (%s)", szBuffFilename);
            dos_printf("%s", "Cannon find the hb-srv.xml. save failed.");
            DOS_ASSERT(0);
            return -1;
        }
    }

    dos_printf("Save process list to file %s.", szBuffFilename);

    if (_config_save(g_pstHBSrvCfg, szBuffFilename) < 0)
    {
        DOS_ASSERT(0);
        return -1;
    }

    return 0;
}

#endif

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

