/*
 *            (C) Copyright 2014, ����Ѷͨ . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  dos_string.c
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: ���峣���ַ�����������
 *     History:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>


/**
 * ������S8* dos_strcpy(S8 *dst, const S8 *src)
 * ���ܣ���src�ַ���copy��dts��ָ���buf��
 * ������
 */
DLLEXPORT S8* dos_strcpy(S8 *dst, const S8 *src)
{
    if (DOS_ADDR_INVALID(dst) || DOS_ADDR_INVALID(src))
    {
        DOS_ASSERT(0);
        return NULL;
    }

    return strcpy(dst, src);
}


/**
 * ������S8* dos_strcpy(S8 *dst, const S8 *src)
 * ���ܣ���src�ַ���copy��dts��ָ���buf�У����copylmaxlen���ַ�
 * ������
 */
DLLEXPORT S8* dos_strncpy(S8 *dst, const S8 *src, S32 lMaxLen)
{
    if (DOS_ADDR_INVALID(dst) || DOS_ADDR_INVALID(src))
    {
        DOS_ASSERT(0);
        return NULL;
    }

    return strncpy(dst, src, lMaxLen);
}


/**
 * ������U32 dos_strlen(const S8 *str)
 * ���ܣ������ַ�������
 * ������
 *  ʧ�ܷ���0���ɹ������ַ�������
 */
DLLEXPORT U32 dos_strlen(const S8 *str)
{
    U32 ulCnt;

    if (DOS_ADDR_VALID(str))
    {
        ulCnt = 0;
        while (*str)
        {
            ulCnt++;
            str++;
        }

        return ulCnt;

    }

    return 0;
}


/**
 * ������S8 *dos_strcat(S8 *dest, const S8 *src)
 * ���ܣ��ַ���ƴ�ӹ���
 * ������
 *      S8 *dest�� Ŀ���ַ���
 *      const S8 *src��Դ�ַ�����
 *  ʧ�ܷ���null���ɹ�����Ŀ���ַ������ֵ�ַ
 */
DLLEXPORT S8 *dos_strcat(S8 *dest, const S8 *src)
{
    if (DOS_ADDR_VALID(dest) && DOS_ADDR_VALID(src))
    {
        return strcat(dest, src);
    }

    return NULL;
}


/**
 * ������S8 *dos_strchr(const S8 *str, S32 i)
 * ���ܣ���str�в����ַ������ַ�i
 * ������
 *      const S8 *str��Դ�ַ���
 *      S32 i���ַ�
 * ����ֵ��
 *      �ɹ������ַ�i���str�׵�ַƫ�Ƶ�ַ��ʧ�ܷ���null
 */
DLLEXPORT S8 *dos_strchr(const S8 *str, S32 i)
{
    if (i >= U8_BUTT)
    {
        DOS_ASSERT(0);
        return NULL;
    }

    if (DOS_ADDR_VALID(str))
    {
        return strchr(str, i);
    }

    return NULL;
}


/**
 * ������S8* dos_strstr(S8 * source,  S8 *key)
 * ���ܣ����ַ���srouce�в����ַ���key
 * ������
 *      S8 * source��Դ�ַ���
 *      S8 *key���ؼ���
 * ����ֵ�����û���ҵ�������null������ҵ�����key���source�׵�ַ��ƫ��
 */
DLLEXPORT S8* dos_strstr(S8 * source, S8 *key)
{
       char *pCh = key;
       char *pSch = NULL, *pSource = NULL;

       if (DOS_ADDR_INVALID(source) || DOS_ADDR_INVALID(key))
       {
           DOS_ASSERT(0);
           return NULL;
       }

       pSource = source;
       do
       {
           pSch = dos_strchr(pSource, *pCh);
           if (pSch)
           {
                if (dos_strncmp(key, pSch, dos_strlen(key)) == 0)
                {
                    break;
                }
                else
                {
                    pSource = pSch+1;
                }
           }
       }while (pSch);

       return pSch;
}


/**
 * ������S32 dos_strcmp (const S8 *s1, const S8 *s2)
 * ���ܣ��ַ����Ƚ�
 * ������
 * ����ֵ��
 */
DLLEXPORT S32 dos_strcmp(const S8 *s1, const S8 *s2)
{

    if (DOS_ADDR_VALID(s1) && DOS_ADDR_VALID(s2))
    {
        return strcmp(s1, s2);
    }
    else
    {
        DOS_ASSERT(0);
    }

    return -1;/*not equal*/
}


/**
 * ������S32 dos_stricmp (const S8 *s1, const S8 *s2)
 * ���ܣ��ַ����Ƚ�,�����ַ��Ĵ�Сд
 * ������
 * ����ֵ��
 */
DLLEXPORT S32 dos_stricmp(const S8 *s1, const S8 *s2)
{
    S8 cCh1, cCh2;

    if (DOS_ADDR_VALID(s1) && DOS_ADDR_VALID(s2))
    {
        if (s1 == s2)
        {
            return 0;
        }

        for (;;)
        {
            cCh1 = *s1;
            cCh2 = *s2;

            if (cCh1 >= 'A' && cCh1 <= 'Z')
            {
                 cCh1 += 'a' - 'A';
            }
            if (cCh2 >= 'A' && cCh2 <= 'Z')
            {
                 cCh2 += 'a' - 'A';
            }

            if (cCh1 != cCh2)
            {
                 return (S32)(cCh1 - cCh2);
            }
            if (!cCh1)
            {
                 return 0;
            }

            s1++;
            s2++;
        }
    }
    else
    {
        DOS_ASSERT(0);
    }

    return -1;/*not equal*/
}


/**
 * ������S32 dos_strncmp (const S8 *s1, const S8 *s2, U32 n)
 * ���ܣ��Ƚ�s1��s2��ǰ��n���ַ�
 * ������
 * ����ֵ��
 */
DLLEXPORT S32 dos_strncmp(const S8 *s1, const S8 *s2, U32 n)
{
    if (DOS_ADDR_VALID(s1) && DOS_ADDR_VALID(s2))
    {
        return strncmp(s1, s2, n);
    }

    return -1;/*not equal*/
}

DLLEXPORT S32 dos_strlastindexof(const S8 *s, const S8 ch, U32 n)
{
    U32 i, len;

    if (DOS_ADDR_INVALID(s))
    {
        return -1;
    }

    i = n;
    len = dos_strlen(s);
    if (len < n)
    {
        i = len;
    }

    for (; i>=0; i--)
    {
        if (ch == s[i])
        {
            return i;
        }
    }

    return -1;
}


/**
 * ������S32 dos_strnicmp(const S8 *s1, const S8 *s2, U32 n)
 * ���ܣ��Ƚ�s1��s2��ǰ��n���ַ������Դ�Сд
 * ������
 * ����ֵ��
 */
DLLEXPORT S32 dos_strnicmp(const S8 *s1, const S8 *s2, U32 n)
{
    S8  cCh1, cCh2;

    if (DOS_ADDR_VALID(s1) && DOS_ADDR_VALID(s2))
    {
        if ((s1 == s2) || (n == 0))
        {
            return 0;
        }

        while (n)
        {
            cCh1 = *s1;
            cCh2 = *s2;

            if (cCh1 >= 'A' && cCh1 <= 'Z')
            {
                cCh1 += 'a' - 'A';
            }

            if (cCh2 >= 'A' && cCh2 <= 'Z')
            {
                cCh2 += 'a' - 'A';
            }
            if (cCh1 != cCh2)
            {
                return (S32)(cCh1 - cCh2);
            }
            if (!cCh1)
            {
                return 0;
            }

            s1++;
            s2++;
            n--;
        }

        return 0;
    }

    return -1;
}


/**
 * ������S32 dos_strnlen(const S8 * str, S32 count)
 * ���ܣ������ַ������ȣ�������count���ֽ�
 * ������
 * ����ֵ��
 */
DLLEXPORT S32 dos_strnlen(const S8 *s, S32 count)
{
    const S8 *sc;

    if(DOS_ADDR_VALID(s))
    {
        for (sc = s; count-- && *sc != '\0'; ++sc)
        /* nothing */;

        return (S32)(sc - s);
    }

   return 0;
}


/**
 * ������S32 dos_strndup(const S8 * str, S32 count)
 * ���ܣ�dump s��ָ����ַ��������count���ֽ�
 * ������
 * ����ֵ��
 */
DLLEXPORT S8 *dos_strndup(const S8 *s, S32 count)
{
    S8 *pStr;

    pStr = (S8 *)dos_dmem_alloc(count);
    if (!pStr)
    {
        DOS_ASSERT(0);
        return NULL;
    }

    if (DOS_ADDR_VALID(s))
    {
        dos_strncpy(pStr, s, count);
        pStr[count - 1] ='\0';
    }

    return pStr;
}

/**
 * ������S8 dos_toupper(S8 ch)
 * ���ܣ����ַ�ת���ɴ�д��ĸ
 * ������
 * ����ֵ��
 */
DLLEXPORT S8 dos_toupper(S8 ch)
{
    if (ch >= 'a' && ch <= 'z')
    {
        return (S8)(ch + 'A' - 'a');
    }

    return ch;
}


/**
 * ������S8 dos_tolower(S8 ch)
 * ���ܣ����ַ�ת����Сд��ĸ
 * ������
 * ����ֵ��
 */
DLLEXPORT S8 dos_tolower(S8 ch)
{
    if (ch >= 'A' && ch <= 'Z')
    {
        return (S8)(ch + 'a' - 'A');
    }

    return ch;
}


/**
 * ������VOID dos_uppercase (S8 *str)
 * ���ܣ����ַ��������е�Сд�ַ�ת���ɴ�д
 * ������
 * ����ֵ��
 */
DLLEXPORT VOID dos_uppercase(S8 *str)
{
    if (DOS_ADDR_INVALID(str))
    {
        DOS_ASSERT(0);
        return;
    }

    while (*str)
    {
        if (*str >= 'a' && *str <= 'z')
        {
            *str = (S8)((*str) + 'A' - 'a');
        }
        str++;
    }
}


/**
 * ������VOID dos_lowercase (S8 *str)
 * ���ܣ����ַ��������еĴ�д�ַ�ת����Сд
 * ������
 * ����ֵ��
 */
DLLEXPORT VOID dos_lowercase(S8 *str)
{
    if (DOS_ADDR_INVALID(str))
    {
        DOS_ASSERT(0);
        return;
    }

    while (*str)
    {
        if (*str >= 'A' && *str <= 'Z')
        {
            *str = (S8)((*str) + 'a' - 'A');
        }
        str++;
    }
}


/**
 *����: S32 dos_atouc(const S8 *szStr, U8 *pnVal)
 *����:���ַ���ת����8λ�޷�������
 *����:
 *����ֵ:�ɹ�����0��ʧ�ܷ���-1
 */
DLLEXPORT S32 dos_atouc(const S8 *szStr, U8 *pucVal)
{
    S32  lTmp;
    if (DOS_ADDR_VALID(szStr) && DOS_ADDR_VALID(pucVal))
    {
        if (dos_sscanf(szStr, "%d", &lTmp) < 1)
        {
            *pucVal = 0;
            return -1;
        }

        *pucVal = lTmp;
        return 0;
    }
    return -1;
}



/**
 * ������S32 dos_atol(const S8 *szStr, S32 *pnVal)
 * ���ܣ����ַ���ת����32λ�з�������
 * ������
 * ����ֵ���ɹ�����0��ʧ�ܷ��أ�1
 */
DLLEXPORT S32 dos_atol(const S8 *szStr, S32 *pnVal)
{
    if (DOS_ADDR_VALID(szStr) && DOS_ADDR_VALID(pnVal))
    {
        if (dos_sscanf(szStr, "%d", pnVal) < 1)
        {
            *pnVal = 0;
            return -1;
        }
        return 0;
    }
    return -1;
}


/**
 * ������S32 dos_atoul (const S8 *szStr, U32 *pulVal)
 * ���ܣ����ַ���ת����32λ�޷�������
 * ������
 * ����ֵ���ɹ����أ�0��ʧ�ܷ��أ�1
 */
DLLEXPORT S32 dos_atoul(const S8 *szStr, U32 *pulVal)
{
    U32 nVal;

    if (DOS_ADDR_VALID(szStr) && DOS_ADDR_VALID(pulVal))
    {
        if (dos_sscanf(szStr, "%u", &nVal) < 1)
        {
            *pulVal = 0;
            return -1;
        }

        *pulVal = nVal;
        return 0;
    }

    return -1;
}

/**
 * ������S32 dos_atoull(const S8 *szStr, U64 *pulVal)
 * ���ܣ����ַ���ת����32λ�޷�������
 * ������
 * ����ֵ���ɹ����أ�0��ʧ�ܷ��أ�1
 */
DLLEXPORT S32 dos_atoull(const S8 *szStr, U64 *pulVal)
{
    U64 nVal;

    if (DOS_ADDR_VALID(szStr) && DOS_ADDR_VALID(pulVal))
    {
        if (dos_sscanf(szStr, "%lu", &nVal) < 1)
        {
            *pulVal = 0;
            return -1;
        }

        *pulVal = nVal;
        return 0;
    }

    return -1;
}

/**
 * ������S32 dos_atoll(const S8 *szStr, S64 *pulVal)
 * ���ܣ����ַ���ת����32λ�޷�������
 * ������
 * ����ֵ���ɹ����أ�0��ʧ�ܷ��أ�1
 */
DLLEXPORT S32 dos_atoll(const S8 *szStr, S64 *pulVal)
{
    S64 nVal;

    if (DOS_ADDR_VALID(szStr) && DOS_ADDR_VALID(pulVal))
    {
        if (dos_sscanf(szStr, "%ld", &nVal) < 1)
        {
            *pulVal = 0;
            return -1;
        }

        *pulVal = nVal;
        return 0;
    }

    return -1;
}


/**
 * ������S32 dos_atolx (const S8 *szStr, S32 *pnVal)
 * ���ܣ���16���Ƹ�ʽ�������ַ���ת����32λ�޷�������
 * ������
 * ����ֵ���ɹ����أ�0��ʧ�ܷ��أ�1
 */
DLLEXPORT S32 dos_atolx (const S8 *szStr, S32 *pnVal)
{
    if (DOS_ADDR_VALID(szStr) && DOS_ADDR_VALID(pnVal))
    {
        if (dos_sscanf(szStr, "%x", pnVal) < 1 &&
            dos_sscanf(szStr, "%X", pnVal) < 1)
        {
            *pnVal = 0;
            return -1;
        }

        return 0;
    }

    return -1;
}


/**
 * ������S32 dos_atoulx(const S8 *szStr, U32 *pulVal)
 * ���ܣ���16���Ƹ�ʽ�������ַ���ת����32λ�޷�������
 * ������
 * ����ֵ���ɹ����أ�0��ʧ�ܷ��أ�1
 */
DLLEXPORT S32 dos_atoulx(const S8 *szStr, U32 *pulVal)
{
    U32 nVal;

    if (DOS_ADDR_VALID(szStr) && DOS_ADDR_VALID(pulVal))
    {
        if (dos_sscanf(szStr, "%x", &nVal) < 1 &&
            dos_sscanf(szStr, "%X", &nVal) < 1)
        {
            *pulVal = 0;
            return -1;
        }

        *pulVal = nVal;
        return 0;
    }

    return -1;
}


/**
 * ������S32 dos_ltoa(S32 nVal, S8 *szStr)
 * ���ܣ�������ת�����ַ���
 * ������
 * ����ֵ���ɹ����أ�0��ʧ�ܷ��أ�1
 */
DLLEXPORT S32 dos_ltoa(S32 nVal, S8 *szStr, U32 ulStrLen)
{
    if (DOS_ADDR_VALID(szStr) && ulStrLen > 0)
    {
        if (dos_snprintf(szStr, ulStrLen, "%d", nVal) < 1)
        {
            return -1;
        }
        return 0;
    }
    return -1;
}


/**
 * ������S32 dos_ltoax(S32 nVal, S8 *szStr)
 * ���ܣ���32λ�з�������ת����16���Ƶĸ�ʽ���ַ���
 * ������
 * ����ֵ���ɹ����أ�0��ʧ�ܷ��أ�1
 */
DLLEXPORT S32 dos_ltoax(S32 nVal, S8 *szStr, U32 ulStrLen)
{
    if (DOS_ADDR_VALID(szStr))
    {
        if (dos_snprintf(szStr, ulStrLen, "%x", nVal) < 1)
        {
            return -1;
        }
        return 0;
    }

    return -1;
}


/**
 * ������S32 dos_ultoax (U32 ulVal, S8 *szStr)
 * ���ܣ���32λ�޷�������ת����16���Ƶĸ�ʽ���ַ���
 * ������
 * ����ֵ���ɹ����أ�0��ʧ�ܷ��أ�1
 */
DLLEXPORT S32 dos_ultoax (U32 ulVal, S8 *szStr, U32 ulStrLen)
{
    if (DOS_ADDR_VALID(szStr))
    {
        if (dos_snprintf(szStr, ulStrLen, "%x", ulVal) < 1)
        {
            return -1;
        }

        return 0;
    }

    return -1;
}


/**
 * ������S8  *dos_ipaddrtostr(U32 ulAddr, S8 *str)
 * ���ܣ���ulAddr����ʾ��ip��ַת���ɵ�ָ�ʽ��IP��ַ
 * ������
 * ����ֵ���ɹ�����STR�׵�ַ��ʧ�ܷ���null
 */
DLLEXPORT S8  *dos_ipaddrtostr(U32 ulAddr, S8 *str, U32 ulStrLen)
{
    if (DOS_ADDR_VALID(str))
    {
        dos_snprintf(str, ulStrLen, "%u.%u.%u.%u"
                        , ulAddr & 0xff
                        , (ulAddr >> 8) & 0xff
                        , (ulAddr >> 16) & 0xff
                        , ulAddr >> 24);

        return str;
    }

    return NULL;
}


/**
 * ������S32 dos_is_digit_str(S8 *str)
 * ���ܣ��ж��ַ����Ƿ��������ַ���
 * ������
 * ����ֵ���ɹ�����0��ʧ�ܷ���-1
 */
DLLEXPORT S32 dos_is_digit_str(S8 *str)
{
    S32 i = 0;

    if (DOS_ADDR_VALID(str))
    {
        while ('\0' != str[i])
        {
            if (str[i] < '0' || str[i] > '9')
            {
                return -1;
            }

            i++;
        }

        return 0;
    }

    return -1;
}

/**
 * ������S32 dos_is_printable_str(S8* str)
 * ���ܣ��ж��ַ����Ƿ�Ϊ�ɴ�ӡ�ַ���
 *       ����ַ���������һ���ɴ�ӡ�ַ�(�ո����)������Ϊ�ǿɴ�ӡ�ַ���
 * ������
 * ����ֵ���ɹ�����0��ʧ�ܷ���-1
 */
DLLEXPORT S32 dos_is_printable_str(S8* str)
{
    S8 *pszPtr = str;
    S32 lFlag = -1;

    while ('\0' != *pszPtr)
    {
        if (*pszPtr >= '!')
        {
            lFlag = 0;
            break;
        }
        ++pszPtr;
    }

    return lFlag;
}

/**
 * ������S32 dos_strtoipaddr(S8 *szStr, U32 *pulIpAddr)
 * ���ܣ�����ָ�ʽ���ַ���IP��ַת��Ϊ�ֽ�������ʽ��IP��ַ
 * ������
 * ����ֵ���ɹ�����STR�׵�ַ��ʧ�ܷ���null
 */
DLLEXPORT S32 dos_strtoipaddr(S8 *szStr, U32 *pulIpAddr)
{
    U32 a, b, c, d;

    if (DOS_ADDR_VALID(pulIpAddr))
    {
        if (dos_sscanf(szStr, "%d.%d.%d.%d", &a, &b, &c, &d) == 4)
        {
            if (a<256 && b<256 && c<256 && d<256)
            {
                *pulIpAddr = (a<<24) | (b<<16) | (c<<8) | d;
                return 0;
            }
        }
    }

    return -1;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
