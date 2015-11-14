/*
 *            (C) Copyright 2015, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  �ļ���: mod_dipcc_mon.c
 *
 *  ����ʱ��: 2015��07��24��11:38
 *  ��    ��: bubble
 *  ��    ��: ��Դ���ģ�����������
 *  �޸���ʷ:
 */


#ifdef __cplusplus
extern "C"{
#endif

#include <dos.h>

#if (INCLUDE_BH_SERVER)
#if INCLUDE_RES_MONITOR

#include <pthread.h>
#include <netdb.h>
#include "mon_def.h"
#include "mon_monitor_and_handle.h"

#define DEFAULT_MAIL_PORT 25

pthread_t g_pMonthr, g_pHndthr;
S32  g_lMailSockfd = U32_BUTT;
U32  mon_cli_conn_init();

/**
 * ����:���ɲ���ʼ��������Դ
 * ��������
 *   �޲���
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
U32 mon_init()
{
    U32 ulRet = 0;

    ulRet = mon_res_alloc();
    if(DOS_SUCC != ulRet)
    {
        mon_trace(MON_TRACE_PUB, LOG_LEVEL_ERROR, "Alloc resource FAIL.");
        return DOS_FAIL;
    }

    mon_trace(MON_TRACE_PUB, LOG_LEVEL_DEBUG, "Alloc resource SUCC.");
    return DOS_SUCC;
}

U32  mon_cli_conn_init()
{
    S32  lRet = U32_BUTT;
    struct hostent *pstHost;
    S8     szMailServ[64] = {0};
    struct sockaddr_in stServerAddr;

    /* ��ȡmail server */
    lRet = config_hb_get_mail_server(szMailServ, sizeof(szMailServ));
    if (lRet < 0)
    {
        mon_trace(MON_TRACE_NOTIFY,  LOG_LEVEL_ERROR, "Get default SMTP Server FAIL.");
        return DOS_FAIL;
    }
    mon_trace(MON_TRACE_NOTIFY, LOG_LEVEL_DEBUG, "Mail Server:%s, Port:%u", szMailServ, DEFAULT_MAIL_PORT);

    /* ��ȡ����IP��ַ */
    pstHost = gethostbyname(szMailServ);
    if (!pstHost)
    {
        mon_trace(MON_TRACE_NOTIFY,  LOG_LEVEL_ERROR, "Get Host By Name FAIL.");
        return DOS_FAIL;
    }
    /* �����׽��� */
    g_lMailSockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (g_lMailSockfd < 0)
    {
        mon_trace(MON_TRACE_NOTIFY,  LOG_LEVEL_ERROR, "Create Socket error.");
        return DOS_FAIL;
    }
    mon_trace(MON_TRACE_NOTIFY, LOG_LEVEL_DEBUG, "Create Socket SUCC.(g_lSockfd:%d)", g_lMailSockfd);
    /* ������������ */
    bzero(&stServerAddr, sizeof(stServerAddr));
    stServerAddr.sin_family = AF_INET;
    stServerAddr.sin_port = htons(DEFAULT_MAIL_PORT);
    stServerAddr.sin_addr = *((struct in_addr *)pstHost->h_addr);

    /* �ͻ��������������� */
    lRet = connect(g_lMailSockfd, (struct sockaddr *)(&stServerAddr), sizeof(struct sockaddr));
    if (lRet < 0)
    {
        mon_trace(MON_TRACE_NOTIFY,  LOG_LEVEL_ERROR, "Mail Server Connect error. errno:%u, cause:%s", errno, strerror(errno));
        return DOS_FAIL;
    }
    mon_trace(MON_TRACE_NOTIFY, LOG_LEVEL_DEBUG, "Mail Server Connect SUCC.");
    return DOS_SUCC;
}

/**
 * ����: ������������
 * ��������
 *   �޲���
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
U32 mon_start()
{
    S32 lRet = 0;

    lRet = pthread_create(&g_pHndthr, NULL, mon_warning_handle, NULL);
    if (lRet != 0)
    {
        mon_trace(MON_TRACE_PUB, LOG_LEVEL_ERROR, "Create warning handle thread FAIL.");
        return DOS_FAIL;
    }

    lRet = pthread_create(&g_pMonthr, NULL, mon_res_monitor, NULL);
    if (lRet != 0)
    {
        mon_trace(MON_TRACE_PUB, LOG_LEVEL_ERROR, "Create resource monitor thread FAIL.");
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/**
 * ����:����ֹͣ���ͷ�������Դ
 * ��������
 *   �޲���
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
U32 mon_stop()
{
    U32 ulRet = 0;
    ulRet = mon_res_destroy();
    if(DOS_SUCC != ulRet)
    {
        mon_trace(MON_TRACE_PUB, LOG_LEVEL_ERROR, "Destroy Resource FAIL.");
        return DOS_FAIL;
    }
    return DOS_SUCC;
}

#endif //#if INCLUDE_RES_MONITOR
#endif //#if (INCLUDE_BH_SERVER)

#ifdef __cplusplus
}
#endif

