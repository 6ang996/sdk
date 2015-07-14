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

/**
 * ������config_hb_get_process_cfg_cnt
 * ���ܣ���ȡ���õĽ�������
 * ������
 * ����ֵ���ɹ�����0��ʧ�ܷ��أ�1
 *
 * ˵�����ú���������֮ǰ���ᱣ�浱ǰ���õ��ļ�����������и��ģ�����ǰ����
 */
S32 config_hb_get_process_cfg_cnt()
{
   S32 lCfgProcCnt = 0;
   S8  szCfgProcCnt[4] = {0};
   S8* pszCfgProcCnt = NULL;
   S8  szBuff[32] = {0};
   S32 lRet = 0;

   if(!g_pstHBSrvCfg)
   {
      dos_printf("%s", "Init hb-srv.xml config fail.");
      DOS_ASSERT(0);
      return -1;
   }
   dos_strncpy(szBuff, "config/process", dos_strlen("config/process"));
   szBuff[dos_strlen("config/process")] = '\0';

   pszCfgProcCnt = _config_get_param(g_pstHBSrvCfg, szBuff, "count"
                    , szCfgProcCnt, sizeof(szCfgProcCnt));
   if(!pszCfgProcCnt)
   {
      dos_printf("%s", "get configured process count failure.");
      DOS_ASSERT(0);
      return -1;
   }

   lRet = dos_atol(pszCfgProcCnt, &lCfgProcCnt);
   if(0 != lRet)
   {
      dos_printf("%s","dos_atol failure!");
      DOS_ASSERT(0);
      return -1;
   }

   return lCfgProcCnt;
}


/**
 * ������config_hb_get_start_cmd
 * ���ܣ���ȡ���̵���������
 * ������
 * ����ֵ���ɹ�����0��ʧ�ܷ��أ�1
 *
 * ˵�����ú���������֮ǰ���ᱣ�浱ǰ���õ��ļ�����������и��ģ�����ǰ����
 */
S32 config_hb_get_start_cmd(U32 ulIndex, S8 *pszCmd, U32 ulLen)
{
   S8  szProcCmd[1024] = {0};
   S8  szNodePath[32] = {0};
   S8* pszValue = NULL;

   if(!g_pstHBSrvCfg)
   {
      dos_printf("%s", "get start process command failed!");
      DOS_ASSERT(0);
      return -1;
   }
   dos_snprintf(szNodePath, sizeof(szNodePath), "config/process/%u", ulIndex);

   pszValue = _config_get_param(g_pstHBSrvCfg, szNodePath, "startcmd", szProcCmd, sizeof(szProcCmd));
   if(!pszCmd && *pszCmd == '\0')
   {
      dos_printf("%s", "get start process command failed!");
      DOS_ASSERT(0);
      return -1;
   }
   dos_strncpy(pszCmd, pszValue, dos_strlen(pszValue));
   pszCmd[dos_strlen(pszValue)] = '\0';
   return 0;
}

/**
 * ������config_hb_threshold_mem
 * ���ܣ���ȡ�ڴ�ռ���ʵķ�ֵ
 * ������
 * ����ֵ���ɹ�����0��ʧ�ܷ��أ�1
 *
 * ˵�����ú���������֮ǰ���ᱣ�浱ǰ���õ��ļ�����������и��ģ�����ǰ����
 */
S32 config_hb_threshold_mem(U32* pulMem)
{
   S8  szNodePath[] = "config/threshold/mem";
   S8  szMemThreshold[4] = {0};
   S8* pszMemThreshold = NULL;
   S32 lRet = 0;

   if(!g_pstHBSrvCfg)
   {
      dos_printf("%s", "Init hb-srv.xml config fail.");
      DOS_ASSERT(0);
      return -1;
   }

   pszMemThreshold = _config_get_param(g_pstHBSrvCfg, szNodePath, "rate", szMemThreshold, sizeof(szMemThreshold));
   if(!pszMemThreshold || '\0' == pszMemThreshold[0])
   {
      dos_printf("%s", "get memory threshold value failed.");
      DOS_ASSERT(0);
      return -1;
   }

   lRet = dos_atoul(pszMemThreshold, pulMem);
   if(0 > lRet)
   {
      dos_printf("%s", "dos_atol failed.");
      DOS_ASSERT(0);
      return -1;
   }

   return 0;
}


/**
 * ������config_hb_threshold_cpu
 * ���ܣ���ȡcpu�ĸ�ռ���ʵķ�ֵ
 * ������
 * ����ֵ���ɹ�����0��ʧ�ܷ��أ�1
 *
 * ˵�����ú���������֮ǰ���ᱣ�浱ǰ���õ��ļ�����������и��ģ�����ǰ����
 */
S32 config_hb_threshold_cpu(U32* pulAvg, U32* pul5sAvg, U32 *pul1minAvg, U32 *pul10minAvg)
{
   S8  szAvgCPURateThreshold[4] = {0};
   S8  sz5sAvgCPURateThreshold[4] = {0};
   S8  sz1minAvgCPURateThreshold[4] = {0};
   S8  sz10minAvgCPURateThreshold[4] = {0};
   S8  szNodePath[] = "config/threshold/cpu";

   S8* pszAvgCPURateThreshold = NULL;
   S8* psz5sAvgCPURateThreshold = NULL;
   S8* psz1minAvgCPURateThreshold = NULL;
   S8* psz10minAvgCPURateThreshold = NULL;

   S32 lRet = 0;

   if(!g_pstHBSrvCfg)
   {
      dos_printf("%s", "Init hb-srv.xml config fail.");
      DOS_ASSERT(0);
      return -1;
   }

   pszAvgCPURateThreshold = _config_get_param(g_pstHBSrvCfg, szNodePath, "avgRate"
                            , szAvgCPURateThreshold, sizeof(szAvgCPURateThreshold));
   if(!pszAvgCPURateThreshold)
   {
      dos_printf("%s", "Init hb-srv.xml config fail.");
      DOS_ASSERT(0);
      return -1;
   }

   psz5sAvgCPURateThreshold = _config_get_param(g_pstHBSrvCfg, szNodePath, "5sAvgRate"
                                , sz5sAvgCPURateThreshold, sizeof(sz5sAvgCPURateThreshold));
   if(!psz5sAvgCPURateThreshold)
   {
      dos_printf("%s", "Init hb-srv.xml config fail.");
      DOS_ASSERT(0);
      return -1;
   }

   psz1minAvgCPURateThreshold = _config_get_param(g_pstHBSrvCfg, szNodePath, "1minAvgRate"
                                , sz1minAvgCPURateThreshold, sizeof(sz1minAvgCPURateThreshold));
   if(!psz1minAvgCPURateThreshold)
   {
      dos_printf("%s", "Init hb-srv.xml config fail.");
      DOS_ASSERT(0);
      return -1;
   }

   psz10minAvgCPURateThreshold = _config_get_param(g_pstHBSrvCfg, szNodePath, "10minAvgRate"
                                    , sz10minAvgCPURateThreshold, sizeof(sz10minAvgCPURateThreshold));
   if(!psz10minAvgCPURateThreshold)
   {
      dos_printf("%s", "Init hb-srv.xml config fail.");
      DOS_ASSERT(0);
      return -1;
   }

   lRet = dos_atoul(pszAvgCPURateThreshold, pulAvg);
   if(0 > lRet)
   {
      dos_printf("%s", "dos_atol failed.");
      DOS_ASSERT(0);
      return -1;
   }

   lRet = dos_atoul(psz5sAvgCPURateThreshold, pul5sAvg);
   if(0 > lRet)
   {
      dos_printf("%s", "dos_atol failed.");
      DOS_ASSERT(0);
      return -1;
   }

   lRet = dos_atoul(psz1minAvgCPURateThreshold, pul1minAvg);
   if(0 > lRet)
   {
      dos_printf("%s", "dos_atol failed.");
      DOS_ASSERT(0);
      return -1;
   }

   lRet = dos_atoul(psz10minAvgCPURateThreshold, pul10minAvg);
   if(0 > lRet)
   {
      dos_printf("%s", "dos_atol failed.");
      DOS_ASSERT(0);
      return -1;
   }

   return 0;
}

/**
 * ������config_hb_threshold_disk
 * ���ܣ���ȡӲ��ռ���ʵķ�ֵ
 * ������
 * ����ֵ���ɹ�����0��ʧ�ܷ��أ�1
 *
 * ˵�����ú���������֮ǰ���ᱣ�浱ǰ���õ��ļ�����������и��ģ�����ǰ����
 */
S32 config_hb_threshold_disk(U32 *pulPartition, U32* pulDisk)
{
   S8  szNodePath[] = "config/threshold/disk";
   S8  szPartitionRate[4] = {0};
   S8  szDiskRate[4] = {0};

   S8* pszPartitionRate = NULL;
   S8* pszDiskRate = NULL;

   S32 lRet = 0;

   if(!g_pstHBSrvCfg)
   {
      dos_printf("%s", "Init hb-srv.xml config fail.");
      DOS_ASSERT(0);
      return -1;
   }

   pszPartitionRate = _config_get_param(g_pstHBSrvCfg, szNodePath, "partitionRate"
                        , szPartitionRate, sizeof(szPartitionRate));
   if(!pszPartitionRate)
   {
      dos_printf("%s", "Init hb-srv.xml config fail.");
      DOS_ASSERT(0);
      return -1;
   }

   pszDiskRate = _config_get_param(g_pstHBSrvCfg, szNodePath, "diskRate"
                    , szDiskRate, sizeof(szDiskRate));
   if(!pszDiskRate)
   {
      dos_printf("%s", "Init hb-srv.xml config fail.");
      DOS_ASSERT(0);
      return -1;
   }

   lRet = dos_atoul(pszPartitionRate, pulPartition);
   if(0 > lRet)
   {
      dos_printf("%s", "dos_atol failed.");
      DOS_ASSERT(0);
      return -1;
   }

   lRet = dos_atoul(pszDiskRate, pulDisk);
   if(0 > lRet)
   {
      dos_printf("%s", "dos_atol failed.");
      DOS_ASSERT(0);
      return -1;
   }

   return 0;
}

/**
 * ������config_hb_threshold_disk
 * ���ܣ���ȡ�����ڴ���CPUռ���ʷ�ֵ
 * ������
 * ����ֵ���ɹ�����0��ʧ�ܷ��أ�1
 *
 * ˵�����ú���������֮ǰ���ᱣ�浱ǰ���õ��ļ�����������и��ģ�����ǰ����
 */
S32 config_hb_threshold_proc(U32* pulMem)
{
   S8  szNodePath[] = "config/threshold/proc";
   S8  szProcMemThres[4] = {0};

   S8* pszProcMemThres = NULL;

   S32 lRet = 0;

   if(!g_pstHBSrvCfg)
   {
      dos_printf("%s", ".Init hb-srv.xml config fail!");
      DOS_ASSERT(0);
      return -1;
   }

   pszProcMemThres = _config_get_param(g_pstHBSrvCfg, szNodePath, "memRate"
                        , szProcMemThres, sizeof(szProcMemThres));
   if(!pszProcMemThres)
   {
      dos_printf("%s", ".Init hb-srv.xml config fail!");
      DOS_ASSERT(0);
      return -1;
   }

   lRet = dos_atoul(pszProcMemThres, pulMem);
   if(0 > lRet)
   {
      dos_printf("%s", "dos_atoul failed!");
      DOS_ASSERT(0);
      return -1;
   }

   return 0;
}

/**
 * ������config_hb_threshold_bandwidth
 * ���ܣ���ȡ����ռ��������
 * ������
 * ����ֵ���ɹ�����0��ʧ�ܷ��أ�1
 *
 * ˵�����ú���������֮ǰ���ᱣ�浱ǰ���õ��ļ�����������и��ģ�����ǰ����
 */
S32 config_hb_threshold_bandwidth(U32* pulBandWidth)
{
    S8 szNodePath[] = "config/threshold/net";
    S8 szMaxBandWidth[8] = {0};

    S8* pszMaxBandWidth = NULL;
    S32 lRet = 0;

    if(!g_pstHBSrvCfg)
    {
        dos_printf("%s", ".Init hb-srv.xml config fail!");
        DOS_ASSERT(0);
        return -1;
    }

    pszMaxBandWidth = _config_get_param(g_pstHBSrvCfg, szNodePath, "maxBandWidth"
                        , szMaxBandWidth, sizeof(szMaxBandWidth));
    if (DOS_ADDR_INVALID(pszMaxBandWidth))
    {
        dos_printf("%s", ".Init hb-srv.xml config fail!");
        DOS_ASSERT(0);
        return -1;
    }

    lRet = dos_atoul(pszMaxBandWidth, pulBandWidth);
    if(0 > lRet)
    {
        dos_printf("%s", "dos_atoul failed!");
        DOS_ASSERT(0);
        return -1;
    }

    return 0;
}


#endif //end  #if (INCLUDE_XML_CONFIG)

#endif //end  #if INCLUDE_BH_SERVER

#ifdef __cplusplus
}
#endif /* __cplusplus */

