/*
 *            (C) Copyright 2014, ����Ѷͨ . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  config_hb_srv.c
 *
 *  Created on: 2015-12-24
 *      Author: Lion
 *        Todo: ͳһ�����������ʹ�����Լ�ά��
 *     History:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* include dos public header files */
#include <dos.h>

/* include provate header file */
#include "config_api.h"

#if (INCLUDE_XML_CONFIG)

#if INCLUDE_MONITOR

/* provate macro */
#define WARNING_CONFIG_FILE_PATH "/ipcc/etc/pub"

#define WARNING_CONFIG_FILE_NAME "warn-config.xml"


/* global variable */
/* ȫ�����þ�� */
static mxml_node_t *g_pstWaringCfg = NULL;


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
S32 config_warn_init()
{
    S8 szBuffFilename[256];

    dos_printf("%s", "Start init hb-srv config.");

    snprintf(szBuffFilename, sizeof(szBuffFilename)
                , "%s/%s", WARNING_CONFIG_FILE_PATH, WARNING_CONFIG_FILE_NAME);

    if (access(szBuffFilename, R_OK|W_OK) < 0)
    {
        dos_printf("Cannon find the warn-config.xml file. (%s)", szBuffFilename);
        dos_printf("%s", "Cannon find the warn-config.xml. Config init failed.");
        DOS_ASSERT(0);
        return -1;
    }

    dos_printf("Use file %s as hb server config file.", szBuffFilename);

    g_pstWaringCfg = _config_init(szBuffFilename);
    if (!g_pstWaringCfg)
    {
        dos_printf("%s", "Init warn-config.xml config fail.");
        DOS_ASSERT(0);
        return -1;
    }
    dos_printf("%s", "warn-config config init successfully.");

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
S32 config_warn_deinit()
{
    S32 lRet = 0;

    lRet = _config_deinit(g_pstWaringCfg);
    g_pstWaringCfg = NULL;

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
S32 config_warn_save()
{
    S8 szBuffFilename[256];

    dos_printf("%s", "Start save hb server config.");

    snprintf(szBuffFilename, sizeof(szBuffFilename)
                , "%s/%s", WARNING_CONFIG_FILE_PATH, WARNING_CONFIG_FILE_NAME);

    if (access(szBuffFilename, R_OK|W_OK) < 0)
    {
        dos_printf("Cannon find the warn-config.xml file in service etc. (%s)", szBuffFilename);
        dos_printf("%s", "Cannon find the warn-config.xml. save failed.");
        DOS_ASSERT(0);
        return -1;
    }

    dos_printf("Save process list to file %s.", szBuffFilename);

    if (_config_save(g_pstWaringCfg, szBuffFilename) < 0)
    {
        DOS_ASSERT(0);
        return -1;
    }

    return 0;
}

/**
 *  ����: S32 config_hb_get_mail_server(S8 *pszServer, U32 ulLen)
 *  ����:
 *      S8 *pszServer  SMTP����������
 *      U32 ulLen      ����
 *  ����: ��ȡSMTP����������
 *  ����: �ɹ�����0�����򷵻�-1
 */
S32 config_warn_get_mail_server(S8 *pszServer, U32 ulLen)
{
    S8  szNodePath[] = "config/mail";
    S8  *pszSMTP = NULL;

    if (DOS_ADDR_INVALID(g_pstWaringCfg)
        || DOS_ADDR_INVALID(pszServer))
    {
        dos_printf("%s", ".Init warn-config.xml config fail!");
        DOS_ASSERT(0);
        return -1;
    }
    pszSMTP = _config_get_param(g_pstWaringCfg, szNodePath, "server", pszServer, ulLen);
    if (!pszSMTP)
    {
        dos_printf("%s", "Get SMTP Server FAIL.");
        DOS_ASSERT(0);
        return -1;
    }
    return 0;
}

/**
 *  ����: S32 config_hb_get_auth_username(S8 *pszServer, U32 ulLen)
 *  ����:
 *      S8 *pszServer  ��֤�û���
 *      U32 ulLen      �û�������
 *  ����: ��ȡSMTPĬ���˻�����֤��
 *  ����: �ɹ�����0�����򷵻�-1
 */
S32 config_warn_get_auth_username(S8 *pszUsername, U32 ulLen)
{
    S8  szNodePath[] = "config/mail";
    S8  *szServer = NULL;

    if(!g_pstWaringCfg)
    {
        dos_printf("%s", ".Init warn-config.xml config fail!");
        DOS_ASSERT(0);
        return -1;
    }
    szServer = _config_get_param(g_pstWaringCfg, szNodePath, "username", pszUsername, ulLen);
    if (!szServer)
    {
        dos_printf("%s", "Get Auth Username FAIL.");
        DOS_ASSERT(0);
        return -1;
    }
    return 0;
}

/**
 *  ����: S32 config_hb_get_auth_passwd(S8 *pszPasswd, U32 ulLen)
 *  ����:
 *      S8 *pszPasswd  ��֤����
 *      U32 ulLen      ���볤��
 *  ����: ��ȡSMTPĬ���˻�����֤����
 *  ����: �ɹ�����0�����򷵻�-1
 */
S32 config_warn_get_auth_passwd(S8 *pszPasswd, U32 ulLen)
{
    S8  szNodePath[] = "config/mail";
    S8  *szServer = NULL;

    if(!g_pstWaringCfg)
    {
        dos_printf("%s", ".Init warn-config.xml config fail!");
        DOS_ASSERT(0);
        return -1;
    }

    szServer = _config_get_param(g_pstWaringCfg, szNodePath, "password", pszPasswd, ulLen);
    if (!szServer)
    {
        dos_printf("%s", "Get Auth Username FAIL.");
        DOS_ASSERT(0);
        return -1;
    }
    return 0;
}

U32 config_warn_get_mail_port()
{
    S8 *pszValue;
    S8 szBuff[32] = { 0 };
    U16 usPort = 0;
    S32 lPort = 0;

    if(!g_pstWaringCfg)
    {
        dos_printf("%s", ".Init warn-config.xml config fail!");
        DOS_ASSERT(0);
        return -1;
    }

    pszValue = _config_get_param(g_pstWaringCfg, "config/mail", "port", szBuff, sizeof(szBuff));
    if (!pszValue)
    {
        szBuff[0] = '\0';
    }

    usPort = (U16)dos_atol(szBuff, &lPort);
    if (0 != usPort || usPort >= U16_BUTT)
    {
        usPort = DOS_MON_MAIL_PORT;
    }
    else
    {
        usPort = (U16)lPort;
    }

    return usPort;
}

#endif //end  #if (INCLUDE_MONITOR)

#endif //end  #if (INCLUDE_XML_CONFIG)

#ifdef __cplusplus
}
#endif /* __cplusplus */

