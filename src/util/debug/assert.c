/**            (C) Copyright 2014, ����Ѷͨ . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  assert.c
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: ʵ�ֶ�����Ϣ��¼��ع���
 *     History:
 */

 #ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>
#include <pthread.h>
#include <time.h>

/*
 * �����ļ�����󳤶�
 **/
#define DOS_ASSERT_FILENAME_LEN       64

/**
 * ����hash���С
 */
#define DOS_ASSERT_HASH_TABLES_SIZE   256

/**
 * �ض���ϵͳ��������
 */
typedef struct tm TIME_ST;
typedef time_t    TIME_T_ST;


/**
 * ����洢assert�Ľڵ�
 */
typedef struct tagDosAssert
{
    HASH_NODE_S stNode;

    TIME_T_ST     stFirstTime;                             /* ��һ�γ��ֵ�ʱ�� */
    TIME_T_ST     stLastTime;                              /* ���һ�γ��ֵ�ʱ�� */

    S8      szFilename[DOS_ASSERT_FILENAME_LEN];     /* �ļ��� */
    U32     ulLine;                                  /* �к� */

    U32     ulTimes;                                 /* ���ִ��� */
}DOS_ASSERT_NODE_ST;

/**
 * ����hashͰ
 */
static HASH_TABLE_S     *g_pstHashAssert = NULL;

/**
 * �����߳�ͬ���Ļ�����
 */
static pthread_mutex_t  g_mutexAssertInfo = PTHREAD_MUTEX_INITIALIZER;

#ifdef INCLUDE_CC_SC
extern U32 sc_log_digest_print(S8 *pszFormat, ...);
#endif

/**
 * ����: static U32 _assert_string_hash(S8 *pszString, U32 *pulIndex)
 * ����: �����ַ�������hashֵ
 * ����:
 *      S8 *pszString: �ַ���
 *      U32 *pulIndex:���ؽ��
 * ����ֵ:�ɹ�����DOS_SUCC,ʧ�ܷ���DOA_FAIL
 */
static U32 _assert_string_hash(S8 *pszString, U32 *pulIndex)
{
    U32  ulHashVal;
    S8  *pszStr;
    U32  i;

    pszStr = pszString;
    if (NULL == pszString || NULL == pulIndex)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulHashVal = 0;

    for (i = 0; i < dos_strlen(pszStr); i ++)
    {
        ulHashVal += (ulHashVal << 5) + (U8)pszStr[i];
    }

    *pulIndex = ulHashVal % DOS_ASSERT_HASH_TABLES_SIZE;

    return DOS_SUCC;
}


/**
 * ����: static S32 _assert_find_node(VOID *pSymName, HASH_NODE_S *pNode)
 * ����: hash��ص��������ڲ��ҽڵ�ʱ�����ȽϽڵ��Ƿ���ͬ
 * ����:
 *      VOID *pSymName : ���ҵĹؼ���
 *      HASH_NODE_S *pNode : ��Ҫ�ȽϵĽڵ�
 * ����ֵ:�ɹ�����DOS_SUCC,ʧ�ܷ���DOA_FAIL
 */
static S32 _assert_find_node(VOID *pSymName, HASH_NODE_S *pNode)
{
    DOS_ASSERT_NODE_ST *pstAssertInfoNode = (DOS_ASSERT_NODE_ST *)pNode;
    S8 szBuff[256] = { 0 };

    dos_snprintf(szBuff, sizeof(szBuff), "%s:%d", pstAssertInfoNode->szFilename, pstAssertInfoNode->ulLine);

    if (0 == dos_strncmp(szBuff, (S8 *)pSymName, sizeof(szBuff)))
    {
        return 0;
    }

    return -1;
}


/**
 * ����: static VOID _assert_print(HASH_NODE_S *pNode, U32 ulIndex)
 * ����: ����hash��ʱ�Ļص�������������ӡ�����Ϣ
 * ����:
 *      HASH_NODE_S *pNode : ��Ҫ����Ľڵ�
 *      U32 ulIndex        : ���Ӳ���
 * ����ֵ: VOID
 */
static VOID _assert_print(HASH_NODE_S *pNode, VOID *pulIndex)
{
    DOS_ASSERT_NODE_ST *pstAssertInfoNode = (DOS_ASSERT_NODE_ST *)pNode;
    U32   ulIndex;
    S8 szBuff[512];
    U32 ulLen;
    TIME_ST *pstTimeFirst, *pstTimeLast;
    S8 szTime1[32] = { 0 }, szTime2[32] = { 0 };

    if (!pNode || !pulIndex)
    {
        return;
    }

    ulIndex = *(U32 *)pulIndex;

    pstTimeFirst = localtime(&pstAssertInfoNode->stFirstTime);
    pstTimeLast = localtime(&pstAssertInfoNode->stLastTime);

    strftime(szTime1, sizeof(szTime1), "%Y-%m-%d %H:%M:%S", pstTimeFirst);
    strftime(szTime2, sizeof(szTime2), "%Y-%m-%d %H:%M:%S", pstTimeLast);

    ulLen = dos_snprintf(szBuff, sizeof(szBuff)
            , "Assert happened %4d times, first time: %s, last time: %s. File:%s, line:%d.\r\n"
            , pstAssertInfoNode->ulTimes
            , szTime1
            , szTime2
            , pstAssertInfoNode->szFilename
            , pstAssertInfoNode->ulLine);
    if (ulLen < sizeof(szBuff))
    {
        szBuff[ulLen] = '\0';
    }
    else
    {
        szBuff[sizeof(szBuff) - 1] = '\0';
    }

    cli_out_string(ulIndex, szBuff);

}

/**
 * ����: static VOID _assert_print(HASH_NODE_S *pNode, U32 ulIndex)
 * ����: ����hash��ʱ�Ļص�������������ӡ�����Ϣ
 * ����:
 *      HASH_NODE_S *pNode : ��Ҫ����Ľڵ�
 *      U32 ulIndex        : ���Ӳ���
 * ����ֵ: VOID
 */
static VOID _assert_record(HASH_NODE_S *pNode, VOID *pParam)
{
    DOS_ASSERT_NODE_ST *pstAssertInfoNode = (DOS_ASSERT_NODE_ST *)pNode;
    S8 szBuff[512];
    U32 ulLen;
    TIME_ST *pstTimeFirst, *pstTimeLast;
    S8 szTime1[32] = { 0 }, szTime2[32] = { 0 };

    if (!pNode)
    {
        return;
    }

    pstTimeFirst = localtime(&pstAssertInfoNode->stFirstTime);
    pstTimeLast = localtime(&pstAssertInfoNode->stLastTime);

    strftime(szTime1, sizeof(szTime1), "%Y-%m-%d %H:%M:%S", pstTimeFirst);
    strftime(szTime2, sizeof(szTime2), "%Y-%m-%d %H:%M:%S", pstTimeLast);

    ulLen = dos_snprintf(szBuff, sizeof(szBuff)
            , "Assert happened %4d times, first time: %s, last time: %s. File:%s, line:%d.\r\n"
            , pstAssertInfoNode->ulTimes
            , szTime1
            , szTime2
            , pstAssertInfoNode->szFilename
            , pstAssertInfoNode->ulLine);
    if (ulLen < sizeof(szBuff))
    {
        szBuff[ulLen] = '\0';
    }
    else
    {
        szBuff[sizeof(szBuff) - 1] = '\0';
    }

    dos_syslog(LOG_LEVEL_ERROR, szBuff);
}



/**
 * ����: S32 dos_assert_print(U32 ulIndex, S32 argc, S8 **argv)
 * ����: �����лص�������������ӡ�����Ϣ
 * ����:
 *      U32 ulIndex, : �����пͻ�������
 *      S32 argc, S8 **argv : ��������
 * ����ֵ: �ɹ�����0 ʧ�ܷ���-1
 */
S32 dos_assert_print(U32 ulIndex, S32 argc, S8 **argv)
{
    cli_out_string(ulIndex, "Assert Info since the service start:\r\n");

    pthread_mutex_lock(&g_mutexAssertInfo);
    hash_walk_table(g_pstHashAssert,  (VOID *)&ulIndex, _assert_print);
    pthread_mutex_unlock(&g_mutexAssertInfo);

    return 0;
}

/**
 * ����: S32 dos_assert_record()
 * ����: ��������Ϣ��¼���ļ���
 * ����:
 * ����ֵ: �ɹ�����0 ʧ�ܷ���-1
 */
S32 dos_assert_record()
{
    dos_syslog(LOG_LEVEL_ERROR, "Assert Info since the service start:\r\n");

    pthread_mutex_lock(&g_mutexAssertInfo);
    hash_walk_table(g_pstHashAssert,  NULL, _assert_record);
    pthread_mutex_unlock(&g_mutexAssertInfo);

    return 0;
}


/**
 * ����: VOID dos_assert(const S8 *pszFileName, const U32 ulLine, const U32 param)
 * ����: ��¼����
 * ����:
 *      const S8 *pszFileName, const U32 ulLine, const U32 param �� ����������Ϣ
 * ����ֵ: VOID
 */
DLLEXPORT VOID dos_assert(const S8 *pszFunctionName, const S8 *pszFileName, const U32 ulLine, const U32 param)
{
    U32 ulHashIndex;
    S8 szFileLine[256] = { 0, };
    DOS_ASSERT_NODE_ST *pstFileDescNode = NULL;

#ifdef INCLUDE_CC_SC
    sc_log_digest_print("Assert happened: func:%s,file=%s,line=%d, param:%d"
        , pszFunctionName, pszFileName, ulLine , param);
#endif

    /* ����hash��key */
    dos_snprintf(szFileLine, sizeof(szFileLine), "%s:%d", pszFileName, ulLine);
    _assert_string_hash(szFileLine, &ulHashIndex);
    //dos_printf("Assert info. Fileline:%s, hash index:%d", szFileLine, ulHashIndex);

    pthread_mutex_lock(&g_mutexAssertInfo);
    pstFileDescNode = (DOS_ASSERT_NODE_ST *)hash_find_node(g_pstHashAssert, ulHashIndex, szFileLine, _assert_find_node);
    if (!pstFileDescNode)
    {
        pstFileDescNode = (DOS_ASSERT_NODE_ST *)dos_dmem_alloc(sizeof(DOS_ASSERT_NODE_ST));
        if (!pstFileDescNode)
        {
            DOS_ASSERT(0);
            pthread_mutex_unlock(&g_mutexAssertInfo);
            return;
        }

        strncpy(pstFileDescNode->szFilename, pszFileName, sizeof(pstFileDescNode->szFilename));
        pstFileDescNode->szFilename[sizeof(pstFileDescNode->szFilename) - 1] = '\0';
        pstFileDescNode->ulLine = ulLine;
        pstFileDescNode->ulTimes = 0;
        time(&pstFileDescNode->stFirstTime);
        time(&pstFileDescNode->stLastTime);
        hash_add_node(g_pstHashAssert, (HASH_NODE_S *)pstFileDescNode, ulHashIndex, NULL);

        //dos_printf("New assert ecord node for fileline:%s, %x", szFileLine, pstFileDescNode);
    }

    pstFileDescNode->ulTimes++;
    time(&pstFileDescNode->stLastTime);
    pthread_mutex_unlock(&g_mutexAssertInfo);
}


/**
 * ����: S32 dos_assert_init()
 * ����: ��ʼ����¼����ģ�飬ֻҪ�ǳ�ʼ��hash��
 * ����:
 *      Nan
 * ����ֵ: �ɹ�����0 ʧ�ܷ���-1
 */
S32 dos_assert_init()
{
    pthread_mutex_lock(&g_mutexAssertInfo);
    g_pstHashAssert = hash_create_table(DOS_ASSERT_HASH_TABLES_SIZE, NULL);
    if (!g_pstHashAssert)
    {
        pthread_mutex_unlock(&g_mutexAssertInfo);
        return -1;
    }
    pthread_mutex_unlock(&g_mutexAssertInfo);

    return 0;

}

#ifdef __cplusplus
}
#endif /* __cplusplus */

