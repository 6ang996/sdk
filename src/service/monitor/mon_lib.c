#ifdef __cplusplus
extern "C"{
#endif

#include <dos.h>

#if (INCLUDE_BH_SERVER)
#if INCLUDE_RES_MONITOR

#include "mon_lib.h"
#include "mon_def.h"

#ifndef MAX_PID_VALUE
#define MAX_PID_VALUE 65535
#define MIN_PID_VALUE 0
#endif

extern S8 * g_pszAnalyseList;
extern DLL_S *g_pstNotifyList;
extern DB_HANDLE_ST *g_pstDBHandle;
extern MON_MSG_MAP_ST *m_pstMsgMapCN;
extern MON_MSG_MAP_ST *m_pstMsgMapEN;

/**
 * ����:��ʼ��֪ͨ��Ϣ����
 * ��������
 *   �޲���
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
U32 mon_init_notify_list()
{
    if (DOS_ADDR_INVALID(g_pstNotifyList))
    {
        g_pstNotifyList = (DLL_S *)dos_dmem_alloc(sizeof(DLL_S));
        if (DOS_ADDR_INVALID(g_pstNotifyList))
        {
            mon_trace(MON_TRACE_NOTIFY, LOG_LEVEL_ERROR, "Alloc Memory FAIL.");
            DOS_ASSERT(0);
            return DOS_FAIL;
        }
        DLL_Init(g_pstNotifyList);

        return DOS_SUCC;
    }
    mon_trace(MON_TRACE_NOTIFY, LOG_LEVEL_DEBUG, "You don\'t need to initialize notification msg list.");
    return DOS_SUCC;
}

/**
 * ����: U32 mon_get_level(U32 ulNotifyType)
 * ����: ��ȡ�澯��Ӧ����ͼ���
 * ��������
 *   �޲���
 * ����ֵ��
 *   �ɹ�������ͼ���ʧ�ܷ���DOS_FAIL
 */
U32 mon_get_level(U32 ulNotifyType)
{
    S8 *pszDesc = NULL;
    S8 szLevel[16] = {0};
    U32 ulLevel;

    pszDesc = m_pstMsgMapCN[ulNotifyType].pszName;
    if (config_hb_get_level(pszDesc, szLevel, sizeof(szLevel)) < 0)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (0 == dos_strcmp(szLevel, "urgent"))
    {
        ulLevel = MON_NOTIFY_LEVEL_EMERG;
    }
    else if (0 == dos_strcmp(szLevel, "important"))
    {
        ulLevel = MON_NOTIFY_LEVEL_CRITI;
    }
    else if (0 == dos_strcmp(szLevel, "minor"))
    {
        ulLevel = MON_NOTIFY_LEVEL_MINOR;
    }
    else if (0 == dos_strcmp(szLevel, "hint"))
    {
        ulLevel = MON_NOTIFY_LEVEL_HINT;
    }
    else
    {
        ulLevel = MON_NOTIFY_LEVEL_BUTT;
    }

    return ulLevel;
}

/**
 *  ������U32 mon_notify_customer(MON_NOTIFY_MSG_ST *pstMsg)
 *  ���ܣ����ⲿ�澯��Ϣ���͸��ͻ�
 *  ������
 *      MON_NOTIFY_MSG_ST *pstMsg  ֪ͨ��Ϣ
 *  ����ֵ�� �ɹ�����DOS_SUCC,ʧ�ܷ���DOS_FAIL
 */
U32 mon_notify_customer(MON_NOTIFY_MSG_ST *pstMsg)
{
    U32 ulLevel = U32_BUTT, ulRet = U32_BUTT, ulType = U32_BUTT;
    S8  szBuff[256] = {0}, szQuery[1024]={0};
    U64 uLBalance = U64_BUTT;
    time_t ulTime = 0;
    struct tm* pstCurTime;
    MON_CONTACT_ST stContact = {{0}};

    if (DOS_ADDR_INVALID(pstMsg))
    {
        DOS_ASSERT(0);
        mon_trace(MON_TRACE_NOTIFY, LOG_LEVEL_ERROR, "pstMsg:%p", pstMsg);
        return DOS_FAIL;
    }

    /* ��ȡ����Ϣ�ĸ澯���� */
    ulLevel = mon_get_level(pstMsg->ulWarningID & 0xFF);
    if (MON_NOTIFY_LEVEL_BUTT == ulLevel)
    {
        ulLevel = MON_NOTIFY_LEVEL_EMERG;
    }

    /* ��ȡ�澯���� */
    ulType = pstMsg->ulWarningID & 0xFF;

    /* ��ȡ��Ϣ����ʱ�� */
    ulTime = pstMsg->ulCurTime;
    pstCurTime = localtime(&ulTime);

    switch (ulType)
    {
        case MON_NOTIFY_TYPE_LACK_FEE:
            /* ��ȡ��ϵ��ʽ */
            ulRet = mon_get_contact(pstMsg->ulDestCustomerID, pstMsg->ulDestRoleID, &stContact);
            if (DOS_SUCC != ulRet)
            {
                DOS_ASSERT(0);
                mon_trace(MON_TRACE_NOTIFY, LOG_LEVEL_ERROR, "Get contact FAIL.ulRet:%u", ulRet);
                return DOS_FAIL;
            }
            /* ��ȡ�����Ϣ */
            ulRet = mon_get_balance(pstMsg->ulDestCustomerID, &uLBalance);
            if (DOS_SUCC != ulRet)
            {
                DOS_ASSERT(0);
                mon_trace(MON_TRACE_NOTIFY, LOG_LEVEL_ERROR, "Get balance FAIL.ulRet:%u", ulRet);
            }

            dos_snprintf(szBuff, sizeof(szBuff), m_pstMsgMapEN[ulType].pszDesc
                            , stContact.szContact, pstCurTime->tm_year + 1900, pstCurTime->tm_mon + 1
                            , pstCurTime->tm_mday, pstCurTime->tm_hour, pstCurTime->tm_min
                            , pstCurTime->tm_sec, uLBalance / 10000.0, pstMsg->ulWarningID);
            break;
        case MON_MOTIFY_TYPE_LACK_GW:
            break;
        case MON_NOTIFY_TYPE_LACK_ROUTE:
            break;
        default:
            break;
    }

    dos_snprintf(szQuery, sizeof(szQuery)
                    , "INSERT INTO"\
                      "     tbl_hb_warning(warning_id, seq, ctime, level, title, content, dest_customer_id, dest_role_id)"\
                      "     VALUES(%u,%u,\'%04u-%02u-%02u %02u:%02u:%02u\',%u,%s,%s,%u,%u);"
                    , pstMsg->ulWarningID
                    , pstMsg->ulSeq
                    , pstCurTime->tm_year + 1900
                    , pstCurTime->tm_mon + 1
                    , pstCurTime->tm_mday
                    , pstCurTime->tm_hour
                    , pstCurTime->tm_min
                    , pstCurTime->tm_sec
                    , pstMsg->ulLevel
                    , m_pstMsgMapEN[ulType].pszTitle
                    , m_pstMsgMapEN[ulType].pszDesc
                    , pstMsg->ulDestCustomerID
                    , pstMsg->ulDestRoleID);
    if (db_query(g_pstDBHandle, szQuery, NULL, NULL, NULL) != DB_ERR_SUCC)
    {
        DOS_ASSERT(0);
        mon_trace(MON_TRACE_NOTIFY, LOG_LEVEL_ERROR , "Add Msg to DB FAIL. SQL:%s", szQuery);
        return DOS_FAIL;
    }

    if (pstMsg->ulLevel <= ulLevel)
    {
        ulRet = mon_send_email(szBuff, m_pstMsgMapEN[ulType].pszTitle, stContact.szEmail);
        if (ulRet != DOS_SUCC)
        {
            mon_trace(MON_TRACE_NOTIFY, LOG_LEVEL_ERROR, "Send Email FAIL. ulRet:%u", ulRet);
            DOS_ASSERT(0);
            return DOS_FAIL;
        }

        ulRet = mon_send_sms(szBuff, m_pstMsgMapEN[ulType].pszTitle, stContact.szTelNo);
        if (ulRet != DOS_SUCC)
        {
            mon_trace(MON_TRACE_NOTIFY, LOG_LEVEL_ERROR, "Send SMS FAIL. ulRet:%u", ulRet);
            DOS_ASSERT(0);
            return DOS_FAIL;
        }
    }

    return DOS_SUCC;
}


/**
 *  ������U32 mon_get_contact(U32 ulCustomerID, U32 ulRoleID, MON_CONTACT_ST *pstContact)
 *  ���ܣ����ݿͻ�id��ͻ���ɫid��ȡ��ϵ����Ϣ
 *  ������
 *      U32 ulCustomerID           �ͻ�id
 *      U32 ulRoleID               �ͻ���ɫid
 *      MON_CONTACT_ST *pstContact �����������ʾ��ϵ�������Ϣ
 *  ����ֵ�� �ɹ�����DOS_SUCC,ʧ�ܷ���DOS_FAIL
 */
U32 mon_get_contact(U32 ulCustomerID, U32 ulRoleID, MON_CONTACT_ST *pstContact)
{
    S8  szQuery[1024] = {0};
    S32 lRet = 0;

    if (DOS_ADDR_INVALID(pstContact))
    {
        DOS_ASSERT(0);
        mon_trace(MON_TRACE_NOTIFY, LOG_LEVEL_ERROR, "Get Contact FAIL. CustomerID:%u; RoleID:%u; pstContact:%p."
                    , ulCustomerID, ulRoleID, pstContact);
        return DOS_FAIL;
    }

    dos_snprintf(szQuery, sizeof(szQuery)
                    , "SELECT contact,tbl_number,email FROM tbl_contact WHERE id=%d AND customer_id=%d;"
                    , ulRoleID
                    , ulCustomerID);

    lRet = db_query(g_pstDBHandle, szQuery, mon_get_contact_cb, (VOID *)pstContact, NULL);
    if (lRet != DB_ERR_SUCC)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    return DOS_SUCC;
}

/**
 *  ������S32 mon_get_contact_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
 *  ���ܣ���ȡ��ϵ�������Ϣ�ص�����
 *  ������
 *  ����ֵ�� �ɹ�����DOS_SUCC,ʧ�ܷ���DOS_FAIL
 */
S32 mon_get_contact_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    MON_CONTACT_ST *pstContact = NULL;

    if (DOS_ADDR_INVALID(aszNames)
        || DOS_ADDR_INVALID(aszValues)
        || DOS_ADDR_INVALID(pArg))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstContact = (MON_CONTACT_ST *)pArg;

    dos_snprintf(pstContact->szContact, sizeof(pstContact->szContact), "%s", aszValues[0]);
    dos_snprintf(pstContact->szTelNo, sizeof(pstContact->szTelNo), "%s", aszValues[1]);
    dos_snprintf(pstContact->szEmail, sizeof(pstContact->szEmail), "%s", aszValues[2]);

    return DOS_SUCC;
}

/**
 *  ������U32 mon_get_balance(U32 ulCustomerID, U64* puLBalance)
 *  ���ܣ����ݿͻ��������Ϣ
 *  ������
 *      U32 ulCustomerID           �ͻ�id
 *      U64* puLBalance            ���������ָ������ָ��
 *  ����ֵ�� �ɹ�����DOS_SUCC,ʧ�ܷ���DOS_FAIL
 */
U32 mon_get_balance(U32 ulCustomerID, U64* puLBalance)
{
    S8  szQuery[1024] = {0};
    S32 lRet = 0;

    dos_snprintf(szQuery, sizeof(szQuery), "SELECT balance FROM tbl_customer WHERE id=%u;", ulCustomerID);

    lRet = db_query(g_pstDBHandle, szQuery, mon_get_balance_cb, puLBalance, NULL);
    if (lRet != DB_ERR_SUCC)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/**
 *  ������S32 mon_get_balance_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
 *  ���ܣ����ݿͻ��������Ϣ�Ļص�����
 *  ������
 *  ����ֵ�� �ɹ�����DOS_SUCC,ʧ�ܷ���DOS_FAIL
 */
S32 mon_get_balance_cb(VOID *pArg, S32 lCount, S8 **aszValues, S8 **aszNames)
{
    U64    *puLBalance = NULL;

    if (DOS_ADDR_INVALID(aszNames)
        || DOS_ADDR_INVALID(aszValues)
        || DOS_ADDR_INVALID(pArg))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    puLBalance = (U64 *)pArg;

    if (dos_atoull(aszValues[0], puLBalance) < 0)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/**
 * ����:Ϊ�ַ��������ڴ�
 * ��������
 *   �޲���
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
U32  mon_init_str_array()
{
   g_pszAnalyseList = (S8 *)dos_dmem_alloc(MAX_TOKEN_CNT * MAX_TOKEN_LEN * sizeof(S8));
   if(DOS_ADDR_INVALID(g_pszAnalyseList))
   {
      DOS_ASSERT(0);
      return DOS_FAIL;
   }
   return DOS_SUCC;
}

/**
 * ����:�ͷ�Ϊ�ַ���������ڴ�
 * ��������
 *   �޲���
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
U32  mon_deinit_str_array()
{
   if(DOS_ADDR_INVALID(g_pszAnalyseList))
   {
      DOS_ASSERT(0);
      return DOS_FAIL;
   }
   dos_dmem_free(g_pszAnalyseList);
   g_pszAnalyseList = NULL;
   return DOS_SUCC;
}

/**
 * ����:�ж�pszDest�Ƿ�ΪpszSrc���Ӵ�
 * ��������
 *   ����1:S8 * pszSrc  Դ�ַ���
 *   ����2:S8 * pszDest ���ַ���
 * ����ֵ��
 *   ���򷵻�DOS_TRUE�����򷵻�DOS_FALSE
 */
BOOL mon_is_substr(S8 * pszSrc, S8 * pszDest)
{
    S8 * pszStr = pszSrc;

    if(DOS_ADDR_INVALID(pszSrc)
        || DOS_ADDR_INVALID(pszDest))
    {
        DOS_ASSERT(0);
        return DOS_FALSE;
    }

   /* ���pszSrc�ĳ���С��pszDest����ôһ�������Ӵ� */
    if (dos_strlen(pszSrc) < dos_strlen(pszDest))
    {
        DOS_ASSERT(0);
        return DOS_FALSE;
    }

    for (;pszStr <= pszSrc + dos_strlen(pszSrc) - dos_strlen(pszDest); ++pszStr)
    {
        if (*pszStr == *pszDest)
        {
            if (dos_strncmp(pszStr, pszDest, dos_strlen(pszDest)) == 0)
            {
                return DOS_TRUE;
            }
        }
    }

    return DOS_FALSE;
}

/**
 * ����:�ж�pszSentence�Ƿ���pszStr��β
 * ��������
 *   ����1:S8 * pszSentence  Դ�ַ���
 *   ����2:S8 * pszStr       ��β�ַ���
 * ����ֵ��
 *   ���򷵻�DOS_TRUE�����򷵻�DOS_FALSE
 */
BOOL mon_is_ended_with(S8 * pszSentence, const S8 * pszStr)
{
    S8 *pszSrc = NULL, *pszTemp = NULL;

    if(DOS_ADDR_INVALID(pszSentence)
        || DOS_ADDR_INVALID(pszStr))
    {
        DOS_ASSERT(0);
        return DOS_FALSE;
    }

    if (dos_strlen(pszSentence) < dos_strlen(pszStr))
    {
        DOS_ASSERT(0);
        return DOS_FALSE;
    }
    pszSrc = pszSentence + dos_strlen(pszSentence) - dos_strlen(pszStr);
    pszTemp = (S8 *)pszStr;

    while (*pszTemp != '\0')
    {
        if (*pszSrc != *pszTemp)
        {
            mon_trace(MON_TRACE_LIB, LOG_LEVEL_ERROR, "\'%s\' is not ended with \'%s\'", pszSrc, pszTemp);
            return DOS_FALSE;
        }
        ++pszSrc;
        ++pszTemp;
    }

   return DOS_TRUE;
}

/**
 * ����:�ж�pszFile�Ƿ���pszSuffixΪ�ļ���׺��
 * ��������
 *   ����1:S8 * pszFile   Դ�ļ���
 *   ����2:S8 * pszSuffix �ļ���׺��
 * ����ֵ��
 *   ���򷵻�DOS_TRUE�����򷵻�DOS_FALSE
 */
BOOL mon_is_suffix_true(S8 * pszFile, const S8 * pszSuffix)
{
    S8 * pszFileSrc = NULL;
    S8 * pszSuffixSrc = NULL;

    if(DOS_ADDR_INVALID(pszFile)
        || DOS_ADDR_INVALID(pszFile))
    {
        DOS_ASSERT(0);
        return DOS_FALSE;
    }

    pszFileSrc = pszFile + dos_strlen(pszFile) -1;
    pszSuffixSrc = (S8 *)pszSuffix + dos_strlen(pszSuffix) - 1;

    for (; pszSuffixSrc >= pszSuffix; pszSuffixSrc--, pszFileSrc--)
    {
        if (*pszFileSrc != *pszSuffixSrc)
        {
            mon_trace(MON_TRACE_LIB, LOG_LEVEL_ERROR, "The suffix of \'%s\' is not \'%s\'.", pszFile, pszSuffix);
            return DOS_FALSE;
        }
    }

    return DOS_TRUE;
}

/**
 * ����:��ȡ�ַ����е�һ������
 * ��������
 *   ����1:S8 * pszStr  ���������ַ������ַ���
 * ����ֵ��
 *   �ɹ��򷵻��ַ����еĵ�һ�����֣�ʧ���򷵻�DOS_FAIL
 */
U32 mon_first_int_from_str(S8 * pszStr)
{
    U32  ulData;
    S8   szTail[1024] = {0};
    S8 * pszSrc = pszStr;
    S8 * pszTemp = NULL;
    S32  lRet = 0;

    if(DOS_ADDR_INVALID(pszStr))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    while (!(*pszSrc >= '0' && *pszSrc <= '9'))
    {
        ++pszSrc;
    }
    pszTemp = pszSrc;
    while (*pszTemp >= '0' && *pszTemp <= '9')
    {
        pszTemp++;
    }
    dos_strncpy(szTail, pszSrc, pszTemp - pszSrc);
    szTail[pszTemp - pszSrc] = '\0';

    lRet = dos_atoul(szTail, &ulData);
    if(0 != lRet)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    return ulData;
}

/**
 * ����:���ַ���pszStr����pszRegExpr����ȥ�ָ���洢��pszRsltList��l
 * ��������
 *   ����1:S8 * pszStr       ���������ַ������ַ���
 *   ����2:S8* pszRegExpr    �ֽ��ַ���
 *   ����3:S8* pszRsltList[] ���ڴ���ַ������׵�ַ
 *   ����4:U32 ulLen         ������󳤶�
 * ����ֵ��
 *   �ɹ��򷵻�DOS_SUCC��ʧ���򷵻�DOS_FAIL
 */
U32  mon_analyse_by_reg_expr(S8* pszStr, S8* pszRegExpr, S8* pszRsltList[], U32 ulLen)
{
    U32 ulRows = 0;
    S8* pszToken = NULL;

    if(DOS_ADDR_INVALID(pszStr))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if(DOS_ADDR_INVALID(pszRegExpr))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

   /* ÿ��ʹ��ǰ��ʼ��Ϊ0 */
    dos_memzero(g_pszAnalyseList, MAX_TOKEN_CNT * MAX_TOKEN_LEN * sizeof(char));

    for(ulRows = 0; ulRows < ulLen; ulRows++)
    {
        /*���ַ����׵�ַ�ֱ����ڷ����ڴ����Ӧλ��*/
        pszRsltList[ulRows] = g_pszAnalyseList + ulRows * MAX_TOKEN_LEN;
    }

    for(ulRows = 0; ulRows < ulLen; ulRows++)
    {
        S8 *pszBuff = NULL;
        if(0 == ulRows)
        {
            pszBuff = pszStr;
        }

        pszToken = strtok(pszBuff, pszRegExpr);
        if(DOS_ADDR_INVALID(pszToken))
        {
            break;
        }
        dos_strncpy(pszRsltList[ulRows], pszToken, dos_strlen(pszToken));
        pszRsltList[ulRows][dos_strlen(pszToken)] = '\0';
    }

    return DOS_SUCC;
}

/**
 * ����:���ɸ澯id
 * ��������
 *   ����1:U32 ulResType     ��Դ����
 *   ����2:U32 ulNo          ��Դ���
 *   ����3:U32 ulErrType     ��������
 * ����ֵ��
 *   �ɹ��򷵻ظ澯id��ʧ���򷵻�(U32)0xff������ϵͳ�澯���λ��1�������澯���λ��0
 */
U32 mon_generate_warning_id(U32 ulResType, U32 ulNo, U32 ulErrType)
{
    if(ulResType >= (U32)0xff || ulNo >= (U32)0xff
        || ulErrType >= (U32)0xff)
    {
        DOS_ASSERT(0);
        return (U32)0xff;
    }
    /* ��1��8λ�洢��Դ���ͣ���2��8λ�洢��Դ��ţ���3��8λ�洢������ */
    return ((ulResType << 24) & 0xff000000) | ((ulNo << 16) & 0xff0000) | ulErrType;
}


/**
 * ����:Ϊ�ַ���ȥͷȥβ��ֻ������򵥵�����
 * ��������
 *   ����1:S8 * pszCmd   ������������
 * ����ֵ��
 *   �ɹ��򷵻�ȥͷȥβ֮��ļ����ƣ�ʧ���򷵻�NULL
 */
S8 * mon_str_get_name(S8 * pszCmd)
{
    S8 * pszPtr = pszCmd;
    if(DOS_ADDR_INVALID(pszPtr))
    {
        DOS_ASSERT(0);
        return NULL;
    }

    /**
      *  �ҵ���һ���ո�ǰ��Ϊ����ľ���·��������Ϊ���������ز���
      */
    while(*pszPtr != ' ' && *pszPtr != '\0' && *pszPtr != '\t')
    {
        ++pszPtr;
    }
    *pszPtr = '\0';

    /*�ҵ����һ��'/'�ַ���'/'��' '֮��Ĳ�����������������*/
    while(*(pszPtr - 1) != '/' && pszPtr != pszCmd)
    {
        --pszPtr;
    }

    pszCmd = pszPtr;

    return pszCmd;
}

#endif //#if INCLUDE_RES_MONITOR
#endif //#if (INCLUDE_BH_SERVER)

#ifdef __cplusplus
}
#endif

