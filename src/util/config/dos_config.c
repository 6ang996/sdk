/*
 *            (C) Copyright 2014, ����Ѷͨ . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  dos_config.c
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: ͳһ�����������ʹ�����Լ�ά��
 *     History:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* include provate header file */
#include "config_api.h"

#if (INCLUDE_XML_CONFIG)

/* provate macro */
#ifdef ARM_VERSION
    #define GLB_CONFIG_FILE_PATH1 "/flash/apps/ptc/etc"
#else
    #define GLB_CONFIG_FILE_PATH1 "/etc"
#endif

#define GLB_CONFIG_FILE_PATH2 "../etc"

#define GLB_CONFIG_FILE_NAME "global.xml"


/* global variable */
/* ȫ�����þ�� */
static mxml_node_t *g_pstGlobalCfg;

/**
 * ������S8* config_get_service_root(S8 *pszBuff, U32 ulLen)
 * ���ܣ���ȡ�������ĸ�Ŀ¼
 * ������
 *      S8 *pszBuff�� ����
 *      U32 ulLen�����泤��
 * ����ֵ���ɹ�����buff��ָ�룬ʧ��ʱ����null
 */
S8* config_get_service_root(S8 *pszBuff, U32 ulLen)
{
    S8 *pszValue = NULL;

    if (!pszBuff || ulLen < 0)
    {
        DOS_ASSERT(0);
        pszBuff[0] = '\0';
        return NULL;
    }

    pszValue = _config_get_param(g_pstGlobalCfg, "config/service/path", "service_root", pszBuff, ulLen);
    if (!pszValue || *pszValue == '\0')
    {
        pszBuff[0] = '\0';
        return NULL;
    }

    return pszBuff;
}

/**
 * ������U32 config_get_mysql_host(S8 *pszBuff, U32 ulLen)
 * ���ܣ���ȡ���ݿ�������
 * ������
 *      S8 *pszBuff�� ����
 *      U32 ulLen�����泤��
 * ����ֵ���ɹ�����0.ʧ�ܷ��أ�1
 */
U32 config_get_db_host(S8 *pszBuff, U32 ulLen)
{
    S8 *pszValue = NULL;

    if (!pszBuff || ulLen < 0)
    {
        DOS_ASSERT(0);
        pszBuff[0] = '\0';
        return -1;
    }

    pszValue = _config_get_param(g_pstGlobalCfg, "config/mysql", "host", pszBuff, ulLen);
    if (!pszValue)
    {
        pszBuff[0] = '\0';
        return -1;
    }

    return 0;
}

/**
 * ������U32 config_get_mysql_host()
 * ���ܣ���ȡ���ݿ�˿ں�
 * ������
 * ����ֵ���ɹ�����0.ʧ�ܷ��أ�1
 */
U32 config_get_db_port()
{
    S8 *pszValue = NULL;
    S8 szBuff[32] = { 0 };
    U16 usDBPort = 0;
    S32 lPort = 0;

    pszValue = _config_get_param(g_pstGlobalCfg, "config/mysql", "port", szBuff, sizeof(szBuff));
    if (!pszValue)
    {
        szBuff[0] = '\0';
    }

    usDBPort = (U16)dos_atol(szBuff, &lPort);
    if (0 == usDBPort || usDBPort >= U16_BUTT)
    {
        usDBPort = 3306;
    }
    else
    {
        usDBPort = (U16)lPort;
    }

    return usDBPort;
}

/**
 * ������U32 config_get_mysql_user(S8 *pszBuff, U32 ulLen)
 * ���ܣ���ȡccsys���ݿ��û���
 * ������
 *      S8 *pszBuff�� ����
 *      U32 ulLen�����泤��
 * ����ֵ���ɹ�����0.ʧ�ܷ��أ�1
 */
U32 config_get_db_user(S8 *pszBuff, U32 ulLen)
{
    S8 *pszValue = NULL;

    if (!pszBuff || ulLen < 0)
    {
        DOS_ASSERT(0);
        pszBuff[0] = '\0';
        return -1;
    }

    pszValue = _config_get_param(g_pstGlobalCfg, "config/mysql", "username", pszBuff, ulLen);
    if (!pszValue)
    {
        pszBuff[0] = '\0';
        return -1;
    }

    return 0;
}


/**
 * ������U32 config_get_syssrc_db_user(S8 *pszBuff, U32 ulLen)
 * ���ܣ���ȡ��Դ������ݿ��û���
 * ������
 *      S8 *pszBuff�� ����
 *      U32 ulLen�����泤��
 * ����ֵ���ɹ�����0.ʧ�ܷ��أ�1
 */

U32 config_get_syssrc_db_user(S8 *pszBuff, U32 ulLen)
{
    S8 *pszValue = NULL;

    if (!pszBuff || ulLen < 0)
    {
        DOS_ASSERT(0);
        pszBuff[0] = '\0';
        return -1;
    }

    pszValue = _config_get_param(g_pstGlobalCfg, "config/mysql", "src_username", pszBuff, ulLen);
    if (!pszValue)
    {
        pszBuff[0] = '\0';
        return -1;
    }

    return 0;
}


/**
 * ������U32 config_get_db_password(S8 *pszBuff, U32 ulLen)
 * ���ܣ���ȡccsys���ݿ�����
 * ������
 *      S8 *pszBuff�� ����
 *      U32 ulLen�����泤��
 * ����ֵ���ɹ�����0.ʧ�ܷ��أ�1
 */
U32 config_get_db_password(S8 *pszBuff, U32 ulLen)
{
    S8 *pszValue = NULL;

    if (!pszBuff || ulLen < 0)
    {
        DOS_ASSERT(0);
        pszBuff[0] = '\0';
        return -1;
    }

    pszValue = _config_get_param(g_pstGlobalCfg, "config/mysql", "password", pszBuff, ulLen);
    if (!pszValue)
    {
        pszBuff[0] = '\0';
        return -1;
    }

    return 0;
}


/**
 * ������U32 config_get_syssrc_db_password(S8 *pszBuff, U32 ulLen)
 * ���ܣ���ȡ��Դ������ݿ�����
 * ������
 *      S8 *pszBuff�� ����
 *      U32 ulLen�����泤��
 * ����ֵ���ɹ�����0.ʧ�ܷ��أ�1
 */
U32 config_get_syssrc_db_password(S8 *pszBuff, U32 ulLen)
{
    S8 *pszValue = NULL;

    if (!pszBuff || ulLen < 0)
    {
        DOS_ASSERT(0);
        pszBuff[0] = '\0';
        return -1;
    }

    pszValue = _config_get_param(g_pstGlobalCfg, "config/mysql", "src_password", pszBuff, ulLen);

    if (!pszValue)
    {
        pszBuff[0] = '\0';
        return -1;
    }

    return 0;
}

/**
 * ������U32 config_get_db_dbname(S8 *pszBuff, U32 ulLen)
 * ���ܣ���ȡccsys���ݿ���
 * ������
 *      S8 *pszBuff�� ����
 *      U32 ulLen�����泤��
 * ����ֵ���ɹ�����0.ʧ�ܷ��أ�1
 */
U32 config_get_db_dbname(S8 *pszBuff, U32 ulLen)
{
    S8 *pszValue = NULL;

    if (!pszBuff || ulLen < 0)
    {
        DOS_ASSERT(0);
        pszBuff[0] = '\0';
        return -1;
    }

    pszValue = _config_get_param(g_pstGlobalCfg, "config/mysql", "dbname", pszBuff, ulLen);
    if (!pszValue)
    {
        pszBuff[0] = '\0';
        return -1;
    }

    return 0;
}

/**
 * ������U32 config_get_syssrc_db_dbname(S8 *pszBuff, U32 ulLen)
 * ���ܣ���ȡϵͳ��Դ���ݿ���
 * ������
 *      S8 *pszBuff�� ����
 *      U32 ulLen�����泤��
 * ����ֵ���ɹ�����0.ʧ�ܷ��أ�1
 */
U32 config_get_syssrc_db_dbname(S8 *pszBuff, U32 ulLen)
{
    S8  *pszValue = NULL;


    if (!pszBuff || ulLen < 0)
    {
        DOS_ASSERT(0);
        pszBuff[0] = '\0';
        return -1;
    }

    pszValue = _config_get_param(g_pstGlobalCfg, "config/mysql", "src_dbname", pszBuff, ulLen);


    if (!pszValue)
    {
        pszBuff[0] = '\0';
        return -1;
    }

    return 0;
}

/**
 * ������U32 config_get_py_path(S8 *pszBuff, U32 ulLen)
 * ���ܣ���ȡPython�ű�·��
 * ������
 *      S8 *pszBuff�� ����
 *      U32 ulLen�����泤��
 * ����ֵ���ɹ�����0.ʧ�ܷ��أ�1
 */
#if INCLUDE_SERVICE_PYTHON
U32 config_get_py_path(S8 *pszBuff, U32 ulLen)
{
    S8 *pszValue = NULL;

    if (!pszBuff || ulLen < 0)
    {
        DOS_ASSERT(0);
        pszBuff[0] = '\0';
        return -1;
    }

    pszValue = _config_get_param(g_pstGlobalCfg, "config/service/path/freeswitch", "fs_scripts_path", pszBuff, ulLen);
    if (!pszValue)
    {
        pszBuff[0] = '\0';
        return -1;
    }

    return 0;
}
#endif

/**
 * ������U32 config_get_mysqlsock_path(S8 *pszBuff, U32 ulLen)
 * ���ܣ���ȡMySQL��sock�ļ�·��
 * ������
 *      S8 *pszBuff�� ����
 *      U32 ulLen�����泤��
 * ����ֵ���ɹ�����0.ʧ�ܷ��أ�1
 */
U32 config_get_mysqlsock_path(S8 *pszBuff, U32 ulLen)
{
    S8 *pszValue = NULL;

    if (DOS_ADDR_INVALID(pszBuff)
        || ulLen < 0)
    {
        DOS_ASSERT(0);
        pszBuff[0] = '\0';
        return -1;
    }

    pszValue = _config_get_param(g_pstGlobalCfg, "config/mysql", "sockpath", pszBuff, ulLen);
    if (!pszValue)
    {
        pszBuff[0] = '\0';
        return -1;
    }

    return 0;
}

U32 config_get_shortcut_cmd(U32 ulNo, S8 *pszCtrlCmd, U32 ulLen)
{
    S8 *pszCmd = NULL;
    S8 szNum[4] = {0};

    /* ������ת��Ϊ�ַ��� */
    if (dos_ltoa(ulNo, szNum, sizeof(szNum)) < 0)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    szNum[sizeof(szNum) - 1] = '\0';
    pszCmd = _config_get_param(g_pstGlobalCfg, "config/shortcut", szNum, pszCtrlCmd, ulLen);
    if (!pszCmd)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/**
 * ������U32 config_hh_get_send_interval()
 * ���ܣ���ȡ�������ͼ��
 * ������
 * ����ֵ���ɹ������������.ʧ�ܷ���0
 */
U32 config_hh_get_send_interval()
{
    S8 *pszValue;
    S8 szBuff[32];
    S32 lInterval;

    pszValue = _config_get_param(g_pstGlobalCfg, "config/heartbeat", "interval", szBuff, sizeof(szBuff));
    if (!pszValue)
    {
        return 0;
    }

    lInterval = atoi(pszValue);
    if (lInterval < 0)
    {
        return 0;
    }

    return (U32)lInterval;
}

/**
 * ������U32 config_hb_get_max_fail_cnt()
 * ���ܣ���ȡ�������ʧ�ܴ���
 * ������
 * ����ֵ���ɹ��������ʧ�ܴ���.ʧ�ܷ���0
 */
U32 config_hb_get_max_fail_cnt()
{
    S8 *pszValue;
    S8 szBuff[32];
    S32 lInterval;

    pszValue = _config_get_param(g_pstGlobalCfg, "config/heartbeat", "max_fail_cnt", szBuff, sizeof(szBuff));
    if (!pszValue)
    {
        return 0;
    }

    lInterval = atoi(pszValue);
    if (lInterval < 0)
    {
        return 0;
    }

    return (U32)lInterval;
}

/**
 * ������S32 config_hb_get_treatment()
 * ���ܣ���ȡ������ʱ�Ĵ���ʽ
 * ������
 * ����ֵ���ɹ����ش���ʽ���.ʧ�ܷ��أ�1
 */
S32 config_hb_get_treatment()
{
    S8 *pszValue;
    S8 szBuff[32];
    S32 lInterval;

    pszValue = _config_get_param(g_pstGlobalCfg, "config/heartbeat", "max_fail_cnt", szBuff, sizeof(szBuff));
    if (!pszValue)
    {
        return -1;
    }

    lInterval = atoi(pszValue);
    if (lInterval < 0)
    {
        return -1;
    }

    return lInterval;
}

/**
 * ������U32 config_init()
 * ���ܣ� ��ʼ������ģ��
 * ������
 * ����ֵ���ɹ�����0��ʧ�ܷ��أ�1
 *
 * ˵�����ú�����ʼ������ģ�顣�����ļ�������·��˳��Ϊ��
 *      1.ϵͳ�����ļ�Ŀ¼
 *      2."../etc/" �ѵ�ǰĿ¼����binĿ¼���ϲ�Ŀ¼Ϊ��ǰ�����Ŀ¼���ڷ����Ŀ¼�������
 */
U32 config_init()
{
    S8 szBuffFilename[256];

    dos_printf("%s", "Start init config.");

    snprintf(szBuffFilename, sizeof(szBuffFilename)
                , "%s/%s", GLB_CONFIG_FILE_PATH1, GLB_CONFIG_FILE_NAME);

    if (access(szBuffFilename, R_OK|W_OK) < 0)
    {
        dos_printf("Cannon find the global config file in system etc. (%s)", szBuffFilename);

        snprintf(szBuffFilename, sizeof(szBuffFilename)
                        , "%s/%s", GLB_CONFIG_FILE_PATH2, GLB_CONFIG_FILE_NAME);
        if (access(szBuffFilename, R_OK|W_OK) < 0)
        {
            dos_printf("Cannon find the global config file in service etc. (%s)", szBuffFilename);
            dos_printf("%s", "Cannon find the global config. Config init failed.");
            DOS_ASSERT(0);
            return -1;
        }
    }

    dos_printf("Use file %s as global config file.", szBuffFilename);

    g_pstGlobalCfg = _config_init(szBuffFilename);
    if (!g_pstGlobalCfg)
    {
        dos_printf("%s", "Init global config fail.");
        DOS_ASSERT(0);
        return -1;
    }
    dos_printf("%s", "Config init successfully.");

    return 0;
}

/**
 * ������U32 config_deinit()
 * ���ܣ� ��������ģ��
 * ������
 * ����ֵ���ɹ�����0��ʧ�ܷ��أ�1
 *
 * ˵�����ú���������֮ǰ���ᱣ�浱ǰ���õ��ļ�����������и��ģ�����ǰ����
 */
U32 config_deinit()
{
    S32 lRet = 0;

    lRet = _config_deinit(g_pstGlobalCfg);
    g_pstGlobalCfg = NULL;

    return lRet;
}

S32 config_save()
{
     S8 szBuffFilename[256];

     snprintf(szBuffFilename, sizeof(szBuffFilename)
                , "%s/%s", GLB_CONFIG_FILE_PATH1, GLB_CONFIG_FILE_NAME);

    if (access(szBuffFilename, R_OK|W_OK) < 0)
    {
        dos_printf("Cannon find the global config file in system etc. (%s)", szBuffFilename);

        snprintf(szBuffFilename, sizeof(szBuffFilename)
                        , "%s/%s", GLB_CONFIG_FILE_PATH2, GLB_CONFIG_FILE_NAME);
        if (access(szBuffFilename, R_OK|W_OK) < 0)
        {
            dos_printf("Cannon find the global config file in service etc. (%s)", szBuffFilename);
            dos_printf("%s", "Cannon find the global config. Config init failed.");
            DOS_ASSERT(0);
            return -1;
        }
    }

    if (_config_save(g_pstGlobalCfg, szBuffFilename) < 0)
    {
        DOS_ASSERT(0);
        return -1;
    }

    return 0;
}

/* pts���ýӿ� */
#ifdef INCLUDE_PTS

/**
 * ������U32 config_get_pts_port()
 * ���ܣ���ȡpts�Ķ˿�
 * ������
 * ����ֵ���ɹ�����0.ʧ�ܷ��أ�1
 */
U32 config_get_pts_port1()
{
    S8 *pszValue;
    S8 szBuff[32] = { 0 };
    U16 usDBPort = 0;
    S32 lPort = 0;

    pszValue = _config_get_param(g_pstGlobalCfg, "config/pts", "pts_port1", szBuff, sizeof(szBuff));
    if (!pszValue)
    {
        szBuff[0] = '\0';
    }

    usDBPort = (U16)dos_atol(szBuff, &lPort);
    if (0 != usDBPort || usDBPort >= U16_BUTT)
    {
        usDBPort = DOS_PTS_SERVER_PORT;
    }
    else
    {
        usDBPort = (U16)lPort;
    }

    return usDBPort;
}

U32 config_get_pts_port2()
{
    S8 *pszValue;
    S8 szBuff[32] = { 0 };
    U16 usDBPort = 0;
    S32 lPort = 0;

    pszValue = _config_get_param(g_pstGlobalCfg, "config/pts", "pts_port2", szBuff, sizeof(szBuff));
    if (!pszValue)
    {
        szBuff[0] = '\0';
    }

    usDBPort = (U16)dos_atol(szBuff, &lPort);
    if (0 != usDBPort || usDBPort >= U16_BUTT)
    {
        usDBPort = DOS_PTS_SERVER_PORT;
    }
    else
    {
        usDBPort = (U16)lPort;
    }

    return usDBPort;
}


/**
 * ������U32 config_get_goahead_server_port()
 * ���ܣ���ȡpts��web server�Ķ˿�
 * ������
 * ����ֵ���ɹ�����0.ʧ�ܷ��أ�1
 */
U32 config_get_web_server_port()
{
    S8 *pszValue;
    S8 szBuff[32] = { 0 };
    U16 usDBPort = 0;
    S32 lPort = 0;

    pszValue = _config_get_param(g_pstGlobalCfg, "config/pts", "web_server_port", szBuff, sizeof(szBuff));
    if (!pszValue)
    {
        szBuff[0] = '\0';
    }

    usDBPort = (U16)dos_atol(szBuff, &lPort);
    if (0 != usDBPort || usDBPort >= U16_BUTT)
    {
        usDBPort = DOS_PTS_WEB_SERVER_PORT;
    }
    else
    {
        usDBPort = (U16)lPort;
    }

    return usDBPort;
}

/**
 * ������U32 config_get_pts_proxy_port()
 * ���ܣ���ȡpts��web server�Ķ˿�
 * ������
 * ����ֵ���ɹ�����0.ʧ�ܷ��أ�1
 */
U32 config_get_pts_proxy_port()
{
    S8 *pszValue;
    S8 szBuff[32] = { 0 };
    U16 usDBPort = 0;
    S32 lPort = 0;

    pszValue = _config_get_param(g_pstGlobalCfg, "config/pts", "proxy_port", szBuff, sizeof(szBuff));
    if (!pszValue)
    {
        szBuff[0] = '\0';
    }

    usDBPort = (U16)dos_atol(szBuff, &lPort);
    if (0 != usDBPort || usDBPort >= U16_BUTT)
    {
        usDBPort = DOS_PTS_PROXY_PORT;
    }
    else
    {
        usDBPort = (U16)lPort;
    }

    return usDBPort;
}

/**
 * ������U32 config_get_pts_telnet_server_port()
 * ���ܣ���ȡpts��telnet server�Ķ˿�
 * ������
 * ����ֵ���ɹ�����0.ʧ�ܷ��أ�1
 */
U32 config_get_pts_telnet_server_port()
{
    S8 *pszValue;
    S8 szBuff[32] = { 0 };
    U16 usDBPort = 0;
    S32 lPort = 0;

    pszValue = _config_get_param(g_pstGlobalCfg, "config/pts", "telnet_server_port", szBuff, sizeof(szBuff));
    if (!pszValue)
    {
        szBuff[0] = '\0';
    }

    usDBPort = (U16)dos_atol(szBuff, &lPort);
    if (0 != usDBPort || usDBPort >= U16_BUTT)
    {
        usDBPort = DOS_PTS_TELNETD_LINSTEN_PORT;
    }
    else
    {
        usDBPort = (U16)lPort;
    }

    return usDBPort;
}

/**
 * ������U32 config_get_pts_domain_dbname(S8 *pszBuff, U32 ulLen)
 * ���ܣ���ȡpts������
 * ������
 *      S8 *pszBuff�� ����
 *      U32 ulLen�����泤��
 * ����ֵ���ɹ�����0.ʧ�ܷ��أ�1
 */
/*U32 config_get_pts_domain(S8 *pszBuff, U32 ulLen)
{
    S8 *pszValue;

    if (!pszBuff || ulLen < 0)
    {
        DOS_ASSERT(0);
        pszBuff[0] = '\0';
        return DOS_FAIL;
    }

    pszValue = _config_get_param(g_pstGlobalCfg, "config/pts", "pts_domain", pszBuff, ulLen);
    if (!pszValue)
    {
        pszBuff[0] = '\0';
        return DOS_FAIL;
    }

    return DOS_SUCC;
}*/

#endif

/* ptc���ýӿ� */
#ifdef INCLUDE_PTC

/**
 * ������U32 config_get_pts_domain_dbname(S8 *pszBuff, U32 ulLen)
 * ���ܣ���ȡpts������
 * ������
 *      S8 *pszBuff�� ����
 *      U32 ulLen�����泤��
 * ����ֵ���ɹ�����0.ʧ�ܷ��أ�1
 */
U32 config_get_pts_major_domain(S8 *pszBuff, U32 ulLen)
{
    S8 *pszValue;

    if (!pszBuff || ulLen < 0)
    {
        DOS_ASSERT(0);
        pszBuff[0] = '\0';
        return DOS_FAIL;
    }

    pszValue = _config_get_param(g_pstGlobalCfg, "config/ptc", "pts_major_domain", pszBuff, ulLen);
    if (!pszValue)
    {
        pszBuff[0] = '\0';
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

U32 config_set_pts_major_domain(S8 *pszBuff)
{
    if (!pszBuff)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    S32 lResult = 0;

    lResult = _config_set_param(g_pstGlobalCfg, "config/ptc", "pts_major_domain", pszBuff);
    if (lResult < 0)
    {
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

U32 config_get_pts_minor_domain(S8 *pszBuff, U32 ulLen)
{
    S8 *pszValue;

    if (!pszBuff || ulLen < 0)
    {
        DOS_ASSERT(0);
        pszBuff[0] = '\0';
        return DOS_FAIL;
    }

    pszValue = _config_get_param(g_pstGlobalCfg, "config/ptc", "pts_minor_domain", pszBuff, ulLen);
    if (!pszValue)
    {
        pszBuff[0] = '\0';
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

U32 config_set_pts_minor_domain(S8 *pszBuff)
{
    if (!pszBuff)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    S32 lResult = 0;

    lResult = _config_set_param(g_pstGlobalCfg, "config/ptc", "pts_minor_domain", pszBuff);
    if (lResult < 0)
    {
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/**
 * ������U32 config_get_pts_port()
 * ���ܣ���ȡpts�Ķ˿�
 * ������
 * ����ֵ��
 */
U32 config_get_pts_major_port()
{
    S8 *pszValue;
    S8 szBuff[32] = { 0 };
    U16 usDBPort = 0;
    S32 lPort = 0;

    pszValue = _config_get_param(g_pstGlobalCfg, "config/ptc", "pts_major_port", szBuff, sizeof(szBuff));
    if (!pszValue)
    {
        szBuff[0] = '\0';
    }

    usDBPort = (U16)dos_atol(szBuff, &lPort);
    if (0 != usDBPort || usDBPort >= U16_BUTT)
    {
        usDBPort = DOS_PTS_SERVER_PORT;
    }
    else
    {
        usDBPort = (U16)lPort;
    }

    return usDBPort;
}

U32 config_set_pts_major_port(S8 *pszBuff)
{
    if (!pszBuff)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    S32 lResult = 0;

    lResult = _config_set_param(g_pstGlobalCfg, "config/ptc", "pts_major_port", pszBuff);
    if (lResult < 0)
    {
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/**
 * ������U32 config_get_pts_port()
 * ���ܣ���ȡpts�Ķ˿�
 * ������
 * ����ֵ��
 */
U32 config_get_pts_minor_port()
{
    S8 *pszValue;
    S8 szBuff[32] = { 0 };
    U16 usDBPort = 0;
    S32 lPort = 0;

    pszValue = _config_get_param(g_pstGlobalCfg, "config/ptc", "pts_minor_port", szBuff, sizeof(szBuff));
    if (!pszValue)
    {
        szBuff[0] = '\0';
    }

    usDBPort = (U16)dos_atol(szBuff, &lPort);
    if (0 != usDBPort || usDBPort >= U16_BUTT)
    {
        usDBPort = DOS_PTS_SERVER_PORT;
    }
    else
    {
        usDBPort = (U16)lPort;
    }

    return usDBPort;
}

U32 config_set_pts_minor_port(S8 *pszBuff)
{
    if (!pszBuff)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    S32 lResult = 0;

    lResult = _config_set_param(g_pstGlobalCfg, "config/ptc", "pts_minor_port", pszBuff);
    if (lResult < 0)
    {
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/**
 * ������U32 config_get_ptc_name(S8 *pszBuff, U32 ulLen)
 * ���ܣ���ȡptc��ID
 * ������
 *      S8 *pszBuff�� ����
 *      U32 ulLen�����泤��
 * ����ֵ���ɹ�����0.ʧ�ܷ��أ�1
 */
U32 config_get_ptc_name(S8 *pszBuff, U32 ulLen)
{
    S8 *pszValue;

    if (!pszBuff || ulLen < 0)
    {
        DOS_ASSERT(0);
        pszBuff[0] = '\0';
        return DOS_FAIL;
    }
    pszValue = _config_get_param(g_pstGlobalCfg, "config/ptc", "ptc_name", pszBuff, ulLen);
    if (!pszValue)
    {
        pszBuff[0] = '\0';
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

#endif

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

