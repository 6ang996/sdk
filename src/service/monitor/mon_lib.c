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
 *   �ɹ��򷵻ظ澯id��ʧ���򷵻�(U32)0xff
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
    return (ulResType << 24) | (ulNo << 16 ) | (ulErrType & 0xffffffff);
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

