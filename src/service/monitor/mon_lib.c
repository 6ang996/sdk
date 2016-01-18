#ifdef __cplusplus
extern "C"{
#endif

#include <dos.h>
#include <iconv.h>

#if (INCLUDE_BH_SERVER)
#if INCLUDE_RES_MONITOR

#include <iconv.h>
#include "mon_lib.h"
#include "mon_def.h"
#include "mon_mail.h"

#ifndef MAX_PID_VALUE
#define MAX_PID_VALUE 65535
#define MIN_PID_VALUE 0

#endif

/* ˵��: �����Ķ���Ҳ����������¸�ʽ��3��%s��һ��%u, һ��%08x����һ��%s��ʽ��ʱ�䣬��2��%s��ʽ��MON_NOTIFY_MSG_ST�е�szContent��Ա����3��%s��ʽ��Notes��%u��ʽ������������Ŀͻ�id��
         %08x��ʽ�������룬����˳�������ϸ�˳�� */
#define NO_ENOUGH_BALANCE_CN "<h1>�𾴵��û������ã�</h1><br/><p style=\"align:2em\">��ֹ%s��������Ϊ%s%s��Ϊ�˲�Ӱ�����ҵ���뾡���ֵ��лл��(CNo:%u, No:%08x)</p>"
#define NO_ENOUGH_BALANCE_EN "<h1>Dear IPCC User:</h1><br/><p style=\"align:2em\">By the time %s,your balance is %s%s,in order to avoid your services being interrupted,please recharge as soon as possible,thank you!(CNo:%u, No:%08x)</p>"

/* �ú�����������Ϣ����������������������޸� */
#define MAX_EXCEPT_CNT  4

extern S8 * g_pszAnalyseList;
extern DLL_S *g_pstNotifyList;
extern DB_HANDLE_ST *g_pstCCDBHandle;

/* ��Ϣӳ�� */
MON_MSG_MAP_ST m_pstMsgMap[][MAX_EXCEPT_CNT] = {
    {
        {"lack_fee",   "����", NO_ENOUGH_BALANCE_CN},
        /* szDesc��Ա������Ҫʱ�����ٶ������ݣ�Ϊ�˼��ݣ����øĸ�ʽ��� */
        {"lack_gw" ,   "�м̲���", "%s%s%s%u%08x"},
        {"lack_route", "·�ɲ���", "%s%s%s%u%08x"}
    },  /* �����0��Ϊ�������� */
    {
        {"lack_fee",   "No enough balance", NO_ENOUGH_BALANCE_EN},
        {"lack_gw",    "No enough gateway", "%s%s%s%u%08x"},
        {"lack_route", "No enough route", "%s%s%s%u%08x"}
    }   /* �����1������ʽӢ�� */
};

U32 mon_system();
BOOL mon_is_email(S8* pszAddress);

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

    pszDesc = m_pstMsgMap[MON_NOTIFY_LANG_CN][ulNotifyType].szName;
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
    U32 ulLevel = U32_BUTT,ulType = U32_BUTT;
    S8  szBuff[512] = {0}, szTime[32] = {0};
    S32 lLang = U32_BUTT;

    time_t ulTime = 0;
    struct tm* pstCurTime;

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

    /* ��ȡ��ǰ�����Ի��� */
    lLang = config_hb_get_lang();
    if (lLang < 0)
    {
        /* ������ö�ȡʧ�ܣ���Ĭ��Ϊ���� */
        lLang = MON_NOTIFY_LANG_CN;
    }
    /* ��ʽ��ʱ���ַ��� */
    dos_snprintf(szTime, sizeof(szTime), "%04u-%02u-%02u %02u:%02u:%02u"
                    , pstCurTime->tm_year + 1900
                    , pstCurTime->tm_mon + 1
                    , pstCurTime->tm_mday
                    , pstCurTime->tm_hour
                    , pstCurTime->tm_min
                    , pstCurTime->tm_sec);

    /* ��ʽ����Ϣ */
    dos_snprintf(szBuff, sizeof(szBuff), m_pstMsgMap[lLang][ulType].szDesc, szTime, pstMsg->szContent, pstMsg->szNotes, pstMsg->ulCustomerID, pstMsg->ulWarningID);

    if (DOS_SUCC != mon_send_email(pstMsg->ulCustomerID, m_pstMsgMap[lLang][ulType].szTitle, szBuff, MON_WARNING_LEVEL_MAJOR, MON_WARNING_TYPE_MAIL))
    {
        mon_trace(MON_TRACE_NOTIFY, LOG_LEVEL_ERROR, "Send mail FAIL.");
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

    if (dos_is_digit_str(aszValues[0]) < 0)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    if (DOS_TRUE != mon_is_email(aszValues[1]))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    dos_snprintf(pstContact->szTelNo, sizeof(pstContact->szTelNo), "%s", aszValues[0]);
    dos_snprintf(pstContact->szEmail, sizeof(pstContact->szEmail), "%s", aszValues[1]);

    return DOS_SUCC;
}

/**
 *  ������U32  mon_get_sp_email(MON_NOTIFY_MSG_ST *pstMsg)
 *  ���ܣ���ȡ��ǰ��Ӫ���ʼ���ַ����ϵ��ʽ
 *  ������
 *      S8 *pszEmail   ���������Ϊ��ȡ���ʼ���ַ
 *  ����ֵ�� �ɹ�����DOS_SUCC,ʧ�ܷ���DOS_FAIL
 */
U32  mon_get_sp_email(MON_NOTIFY_MSG_ST *pstMsg)
{
    S8  szQuery[256] = {0};

    /* �Ȳ��ҹ���Ա */
    dos_snprintf(szQuery, sizeof(szQuery), "SELECT tel_number,email FROM tbl_contact WHERE role=%u AND customer_id=%u LIMIT 0, 1;", ROLE_MANAGER, pstMsg->ulCustomerID);
    db_query(g_pstCCDBHandle, szQuery, mon_get_contact_cb, (VOID *)&pstMsg->stContact, NULL);
    if ('\0' != pstMsg->stContact.szEmail[0])
    {
        return DOS_SUCC;
    }
    /* �ٲ��Ҿ��� */
    dos_snprintf(szQuery, sizeof(szQuery), "SELECT tel_number,email FROM tbl_contact WHERE role=%u AND customer_id=%u LIMIT 0, 1;", ROLE_DIRECTOR, pstMsg->ulCustomerID);
    db_query(g_pstCCDBHandle, szQuery, mon_get_contact_cb, (VOID *)&pstMsg->stContact, NULL);
    if ('\0' != pstMsg->stContact.szEmail[0])
    {
        return DOS_SUCC;
    }

    /* �ٲ��Ҳ���Ա */
    dos_snprintf(szQuery, sizeof(szQuery), "SELECT tel_number,email FROM tbl_contact WHERE role=%u AND customer_id=%u LIMIT 0, 1;", ROLE_OPERATOR, pstMsg->ulCustomerID);
    db_query(g_pstCCDBHandle, szQuery, mon_get_contact_cb, (VOID *)&pstMsg->stContact, NULL);
    if ('\0' != pstMsg->stContact.szEmail[0])
    {
        return DOS_SUCC;
    }
    /* �ٲ��Ҳ��� */
    dos_snprintf(szQuery, sizeof(szQuery), "SELECT tel_number,email FROM tbl_contact WHERE role=%u AND customer_id=%u LIMIT 0, 1;", ROLE_FINANCE, pstMsg->ulCustomerID);
    db_query(g_pstCCDBHandle, szQuery, mon_get_contact_cb, (VOID *)&pstMsg->stContact, NULL);
    if ('\0' != pstMsg->stContact.szEmail[0])
    {
        return DOS_SUCC;
    }
    DOS_ASSERT(0);
    mon_trace(MON_TRACE_NOTIFY, LOG_LEVEL_ERROR, "Get contact FAIL.");
    return DOS_FAIL;
}

U32 mon_system(S8 *pszCmd)
{
    S32  lStatus = U32_BUTT;

    if (DOS_ADDR_INVALID(pszCmd)
        || '\0' == *pszCmd)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    lStatus = system(pszCmd);
    if (lStatus < 0)
    {
        mon_trace(MON_TRACE_LIB, LOG_LEVEL_ERROR, "Command \'%s\' execute FAIL. errno:%d, cause:%s", pszCmd, errno, strerror(errno));
        return DOS_FAIL;
    }

    if (WIFEXITED(lStatus))
    {
        mon_trace(MON_TRACE_LIB, LOG_LEVEL_DEBUG, "Command:%s ==> Normal termination,exit status = %d.", pszCmd, WEXITSTATUS(lStatus));
    }
    else if (WIFSIGNALED(lStatus))
    {
        mon_trace(MON_TRACE_LIB, LOG_LEVEL_DEBUG, "Command:%s ==> Abnormal termination,signal number = %d.", pszCmd, WTERMSIG(lStatus));
    }
    else if (WIFSTOPPED(lStatus))
    {
        mon_trace(MON_TRACE_LIB, LOG_LEVEL_DEBUG, "Command:%s ==> Process stoped,signal number = %d.", pszCmd, WIFSTOPPED(lStatus));
    }
    else
    {
        mon_trace(MON_TRACE_LIB, LOG_LEVEL_DEBUG, "Command:%s ==> Another Cause, system returned Code:%d.", pszCmd, lStatus);
    }

    return DOS_SUCC;
}

/**
 *  ������BOOL mon_is_email(S8* pszAddress)
 *  ���ܣ��ж��Ƿ��ǵ����ʼ���ַ
 *  ������
 *  ����ֵ�� �ɹ�����DOS_SUCC,ʧ�ܷ���DOS_FAIL
 *     Ŀǰ���ıȽϼ򵥣��Ժ����Ż�
 */
BOOL mon_is_email(S8* pszAddress)
{
    if (DOS_ADDR_INVALID(pszAddress)
        || '\0' == pszAddress[0])
    {
        DOS_ASSERT(0);
        return DOS_FALSE;
    }
    if (DOS_FALSE == dos_strstr(pszAddress, "@")
        || DOS_FALSE == dos_strstr(pszAddress, "."))
    {
        DOS_ASSERT(0);
        return DOS_FALSE;
    }
    return DOS_TRUE;
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

#endif //#if INCLUDE_RES_MONITOR
#endif //#if (INCLUDE_BH_SERVER)

#ifdef __cplusplus
}
#endif

