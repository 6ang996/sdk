/**
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  �ļ���: sc_acd.h
 *
 *  ����ʱ��: 2014��12��16��10:15:56
 *  ��    ��: Larry
 *  ��    ��: ACDģ��˽��ͷ�ļ�
 *  �޸���ʷ:
 */


#ifndef __SC_ACD_H__
#define __SC_ACD_H__

#define SC_ACD_GROUP_NAME_LEN         24

#define SC_ACD_HASH_SIZE              128

#define SC_ACD_SITE_IS_USEABLE(pstSiteDesc)        \
    DOS_ADDR_VALID(pstSiteDesc)                    \
    && (SC_ACD_IDEL == pstSiteDesc->usStatus       \
    || SC_ACD_CONNECTED == pstSiteDesc->usStatus)


typedef struct tagACDQueueNode{
    HASH_NODE_S           stHashLink;       /* hash��ʹ�ýڵ� */

    U32                   ulID;             /* ��ǰ�ڵ��ڵ�ǰ��������ı�� */

    SC_ACD_SITE_DESC_ST   *pstSiteInfo;     /* ��ϯ��Ϣ */
}SC_ACD_SITE_HASH_NODE_ST;

typedef struct tagACDQueryMngtNode
{
    HASH_NODE_S        stHashLink;               /* hash��ʹ�ýڵ� */

    U16       usID;                                /* ��ǰ��ϯ���� */
    U16       usCount;                             /* ��ǰ��ϯ����ϯ���� */
    U16       usLastUsedSite;                      /* ��һ�νӵ绰����ϯ��� */
    U8        ucACDPolicy;                         /* ����з������ */
    U8        ucWaitingDelete;                     /* �Ƿ�ȴ�ɾ�� */

    U32       ulCustomID;                          /* ��ǰ��ϯ������ */
    U32       ulGroupID;                           /* ��ϯ�������ݿ��еı�� */
    S8        szGroupName[SC_ACD_GROUP_NAME_LEN];  /* ��ϯ���� */

    HASH_TABLE_S     *pstSiteQueue;                /* ��ϯhash�� */
    pthread_mutex_t  mutexSiteQueue;
}SC_ACD_GRP_HASH_NODE_ST;

typedef struct tagACDFindSiteParam
{
    U32                 ulPolocy;
    U32                 ulLastSieUsed;
    U32                 ulResult;

    SC_ACD_SITE_DESC_ST *pstResult;
}SC_ACD_FIND_SITE_PARAM_ST;

VOID sc_acd_trace(U32 ulLevel, S8 *pszFormat, ...);

#define sc_acd_trace_debug(format, args...) \
        sc_acd_trace(LOG_LEVEL_DEBUG, (format), ##args)

#define sc_acd_trace_info(format, args...) \
        sc_acd_trace(LOG_LEVEL_INFO, (format), ##args)

#define sc_acd_trace_notice(format, args...) \
        sc_acd_trace(LOG_LEVEL_NOTIC, (format), ##args)

#define sc_acd_trace_warning(format, args...) \
        sc_acd_trace(LOG_LEVEL_WARNING, (format), ##args)

#define sc_acd_trace_error(format, args...) \
        sc_acd_trace(LOG_LEVEL_ERROR, (format), ##args)

#define sc_acd_trace_cirt(format, args...) \
        sc_acd_trace(LOG_LEVEL_CIRT, (format), ##args)

#define sc_acd_trace_alert(format, args...) \
        sc_acd_trace(LOG_LEVEL_ALERT, (format), ##args)

#define sc_acd_trace_emerg(format, args...) \
        sc_acd_trace(LOG_LEVEL_EMERG, (format), ##args)

#endif

