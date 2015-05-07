/*
 *            (C) Copyright 2014, ����Ѷͨ . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  syscfg.c
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: ������Ϣ��غ���
 *     History:
 */


#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */
    
#include <dos.h>

#define MAX_PROCESS_NAME_LENGTH   48

#define MAX_PATH_LENGTH           256

/* ��ʱģ�������Ϣ */
/* ������ */
static S8 g_pszProcessName[MAX_PROCESS_NAME_LENGTH] = { 0 };

/* ���̰汾 */
static S8 *g_pszProcessVersion = "1.1";

/* ����Ĭ��ϵͳ��Ŀ¼ */
static S8 *g_pszDefaultSysRootPath = "/ipcc";

/* ��������ļ���ϵͳ��Ŀ¼ */
static S8 g_szSysRootPath[MAX_PATH_LENGTH] = {0, };

/**
 * Function: S8 *dos_get_process_name()
 *     Todo: ��ȡ�ý�������
 *   Return: ���ظý�������
 */
S8 *dos_get_process_name()
{
    return g_pszProcessName;
}

/**
 * Function: S8 *dos_get_process_version()
 *     Todo: ��ȡ�ý��̰汾��
 *   Return: ���ظý��̰汾��
 */
S8 *dos_get_process_version()
{
    return g_pszProcessVersion;
}

/*
 *  ������S8 *dos_get_pid_file_pah(S8 *pszBuff, S32 lMaxLen)
 *  ���ܣ���ȡpid�ļ���·��
 *  ������
 *      S8 *pszBuff  ���ݻ���
 *      S32 lMaxLen  ����ĳ���
 *  ����ֵ:
 *      ���һ�������������pid�ļ�·��copy�����棬���ҷ��ػ�����׵�ַ��
 *      ����쳣�����ط�NULL, �����е����ݽ������������
 */
S8 *dos_get_pid_file_path(S8 *pszBuff, S32 lMaxLen)
{
    S32 lRet = 0;
    S8 szSysRootPath[256];

    if (!pszBuff || lMaxLen <= 0)
    {
        DOS_ASSERT(0);
        return 0;
    }

    if (config_get_service_root(szSysRootPath, sizeof(szSysRootPath)))
    {
        lRet = snprintf(pszBuff, lMaxLen, "%s/var/run/pid", szSysRootPath);
    }
    else
    {
        lRet = snprintf(pszBuff, lMaxLen, "%s/var/run/pid", g_pszDefaultSysRootPath);
    }

    if (lRet >= lMaxLen)
    {
        DOS_ASSERT(0);
        return NULL;
    }

    return pszBuff;
}

/**
 * ������S8 *dos_get_sys_root_path()
 * ���ܣ���ȡϵͳ��Ŀ¼
 * ������
 * ����ֵ�����������ļ��еĸ�Ŀ¼������ϵͳģ���Ŀ¼
 */
S8 *dos_get_sys_root_path()
{
    S8 szSysRootPath[256];
    S8 *pszPath;

    if ('\0' != g_szSysRootPath[0])
    {
        return g_szSysRootPath;
    }
    else
    {
        pszPath = config_get_service_root(szSysRootPath, sizeof(szSysRootPath));
        if (pszPath)
        {
            dos_strncpy(g_szSysRootPath, szSysRootPath, sizeof(g_szSysRootPath));
            g_szSysRootPath[MAX_PATH_LENGTH - 1] = '\0';

            return g_szSysRootPath;
        }
        else
        {
            return g_pszDefaultSysRootPath;
        }
    }
}

/**
 * ������U32 dos_set_process_name(S8 *pszName)
 * ���ܣ����õ�ǰ���̵�����
 * ������
 *      S8 *pszName : ����ʱ�Ľ����������ܻ����ȫ·����
 * ����ֵ
 *      �ɹ�����0��ʧ�ܷ��أ�1
 */
S32 dos_set_process_name(S8 *pszName)
{
    S8 *pSprit = NULL;
    S8 *pNext = NULL;

    if (!pszName || '\0' == pszName)
    {
        DOS_ASSERT(0);
        return -1;
    }

    /* ���ܴ���·��������ֻ��Ҫ���һ�� */
    pSprit = pszName;
    pNext = dos_strchr(pSprit, '/');
    while(pNext)
    {
        pSprit = pNext + 1;
        if ('\0' == pSprit[0])
        {
            break;
        }

        pNext = dos_strchr(pSprit, '/');
    }

    /* �洢һ�� */
    if ('\0' == pSprit[0])
    {
        DOS_ASSERT(0);
        g_pszProcessName[0] = '\0';
    }
    else
    {
        dos_strncpy(g_pszProcessName, pSprit, sizeof(g_pszProcessName));
        g_pszProcessName[sizeof(g_pszProcessName) - 1] = '\0';
    }

    return 0;
}

/**
 * ������S8 *dos_get_filename(S8* path)
 * ���ܣ�����·�����ļ�������ɲ���·�����ļ���
 * ������
 *      S8 *pszName : ����ʱ�Ľ����������ܻ����ȫ·����
 * ����ֵ
 *      �ɹ����ļ����0��ʧ�ܷ���NULL
 */
DLLEXPORT S8 *dos_get_filename(const S8* path)
{
    const S8 *pSprit = NULL;
    const S8 *pNext = NULL;

    if (!path || '\0' == path)
    {
        DOS_ASSERT(0);
        return NULL;
    }

    /* ���ܴ���·��������ֻ��Ҫ���һ�� */
    pSprit = path;
    pNext = dos_strchr(pSprit, '/');
    if (!pNext)
    {
        return (S8 *)path;
    }

    while(pNext)
    {
        pSprit = pNext + 1;
        if ('\0' == pSprit[0])
        {
            break;
        }

        pNext = dos_strchr(pSprit, '/');
    }

    if ('\0' == pSprit[0])
    {
        return NULL;
    }
    else
    {
        return (char *)pSprit;
    }
}


#ifdef __cplusplus
}
#endif /* __cplusplus */
