/*
 *            (C) Copyright 2014, ����Ѷͨ . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  memory.c
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: �򵥵�ʵ���ڴ����
 *     History:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* include dos header files */
#include <dos.h>
#include <hash/hash.h>
#include "memory.h"

/* include system header files */
#include <pthread.h>

#if INCLUDE_MEMORY_MNGT

/* ����hash�����Ԫ�ظ��� */
#define HASH_MAX_ELEMENT_NUM  DOS_MEM_MNGT_HASH_SIZE

/* �ڴ����������� */
static HASH_TABLE_S     *g_pstHashMemMngtTable;
static pthread_mutex_t  g_mutexMemMngtTable = PTHREAD_MUTEX_INITIALIZER;


static U32 _mem_string_hash(S8 *pszString, U32 *pulIndex);
static S32 _mem_find_node(VOID *pSymName, HASH_NODE_S *pNode);

/*
 * ����: _mem_mngt_init()
 * ����: ��ʼ���ڴ����ģ��
 * ����:
 * ����ֵ: �ɹ�����0�� ʧ�ܷ��أ�1
 * ע��: ʹ�øú���������ڴ�
 */
U32 _mem_mngt_init()
{
    pthread_mutex_lock(&g_mutexMemMngtTable);
    g_pstHashMemMngtTable = hash_create_table(HASH_MAX_ELEMENT_NUM, NULL);
    if (!g_pstHashMemMngtTable)
    {
        pthread_mutex_unlock(&g_mutexMemMngtTable);
        return -1;
    }
    pthread_mutex_unlock(&g_mutexMemMngtTable);

    return 0;
}

/*
 * ����: _mem_alloc(S8 *pszFileName, U32 ulLine, U32 ulSize, U32 ulFlag)
 * ����: �����ڴ�
 * ����:
 *       S8 *pszFileName : �ڴ����� �ļ���
 *       U32 ulLine : �ڴ����� ����
 *       U32 ulSize : �����ڴ�Ĵ�С
 *       U32 ulFlag : �Ƿ�᳤��פ���ڴ�
 * ����ֵ: NULL
 * ע��: ʹ�øú���������ڴ�
 */
DLLEXPORT VOID * _mem_alloc(S8 *pszFileName, U32 ulLine, U32 ulSize, U32 ulFlag)
{
    S8 szFileLine[256];
    struct tagMemInfoNode *pstFileDescNode;
    VOID *ptr;
    MEM_CCB_ST stMemCCB;
    U32 ulHashIndex;

    /* ����hash��key */
    snprintf(szFileLine, sizeof(szFileLine), "%s:%d", pszFileName, ulLine);
    _mem_string_hash(szFileLine, &ulHashIndex);
    //dos_printf("Fileline:%s, hash index:%d", szFileLine, ulHashIndex);

    pthread_mutex_lock(&g_mutexMemMngtTable);
    /* ȥhash���������ڴ�������û�з�����ڴ� */
    pstFileDescNode = (MEM_INFO_NODE_ST *)hash_find_node(g_pstHashMemMngtTable, ulHashIndex, szFileLine, _mem_find_node);
    if (!pstFileDescNode)
    {
        /* �ڴ�����û�з��������Ҫ�½� */
        pstFileDescNode = (MEM_INFO_NODE_ST *)malloc(sizeof(MEM_INFO_NODE_ST));
        if (!pstFileDescNode)
        {
            pthread_mutex_unlock(&g_mutexMemMngtTable);
            DOS_ASSERT(0);
            return NULL;
        }

        strncpy(pstFileDescNode->szFileName, pszFileName, sizeof(pstFileDescNode->szFileName));
        pstFileDescNode->szFileName[sizeof(pstFileDescNode->szFileName) - 1] = '\0';
        pstFileDescNode->ulLine = ulLine;
        pstFileDescNode->ulRef = 0;
        pstFileDescNode->ulTotalSize = 0;
        //hash_set(g_pstHashFile2Ref, szFileLine, pstFileDescNode);
        hash_add_node(g_pstHashMemMngtTable, (HASH_NODE_S *)pstFileDescNode, ulHashIndex, NULL);

        //dos_printf("Create new node for fileline:%s", szFileLine);
    }
    pthread_mutex_unlock(&g_mutexMemMngtTable);

    /* �����ڴ�����Ҫʹ�õ��ڴ� */
    ptr = malloc(ulSize + sizeof(MEM_CCB_ST));
    if (!ptr)
    {
        DOS_ASSERT(0);
        return NULL;
    }
    //dos_printf("Alloc memory:%p, length:%d", ptr, ulSize + sizeof(MEM_CCB_ST));

    /* �����ڴ�����������Ϣ */
    pthread_mutex_lock(&g_mutexMemMngtTable);
    pstFileDescNode->ulTotalSize += ulSize;
    pstFileDescNode->ulRef++;
    pthread_mutex_unlock(&g_mutexMemMngtTable);

    /* �����ڴ���ƿ죬�洢���������ڴ��ͷ�� */
    memset((VOID *)&stMemCCB, 0, sizeof(stMemCCB));
    MEM_SET_MAGIC(&stMemCCB);
    if (ulFlag)
    {
        MEM_SET_TYPE(&stMemCCB, MEM_TYPE_DYNANIC);
    }
    else
    {
        MEM_SET_TYPE(&stMemCCB, MEM_TYPE_STATIC);
    }
    stMemCCB.pstRefer = pstFileDescNode;

    memcpy((VOID *)ptr, (VOID *)&stMemCCB, sizeof(stMemCCB));

    /* ����ʱ��Ҫ�ѿ��ƿ���û�ʹ�� */
    return (VOID *)((U8 *)ptr + sizeof(MEM_CCB_ST));
}

/*
 * ����: _mem_free(VOID *p)
 * ����: �ͷ�ָ��p��ָ��ĵ�ַ�ռ�
 * ����: VOID *P The memory's pointer.
 * ����ֵ: NULL
 * ע��: ʹ�øú���������ڴ�
 */
DLLEXPORT VOID _mem_free(VOID *p)
{
    MEM_INFO_NODE_ST *pstFileDescNode = NULL;
    MEM_CCB_ST *pstMemCCB;
    VOID *ptr;
    S8 szBuffer[64] = { 0 };
    U32 ulLen, ulLenUsed;

    if (!p)
    {
        DOS_ASSERT(0);
        return;
    }

    /* �ҵ��ڴ���ƿ� */
    ptr = (VOID *)((U8 *)p - sizeof(MEM_CCB_ST));
    pstMemCCB = ptr;
    pstFileDescNode = pstMemCCB->pstRefer;

    //dos_printf("Free memory:%p", ptr);
    /* ͨ��ħ�����жϣ��ڴ��Ƿ��Ѿ��ͷ� */
    if (MEM_CHECK_FREE_MAGIC(pstMemCCB))
    {
        dos_snprintf(szBuffer, sizeof(szBuffer), "Pointer %p has already free!", p);
        dos_syslog(LOG_LEVEL_ERROR, szBuffer);
        return;
    }

    /* ����ħ���ֲ���ȷ�������֮��ʹ��ϵͳ�����ͷ� */
    if (!MEM_CHECK_MAGIC(pstMemCCB))
    {
        pstMemCCB->ulMemDesc = 0;
        DOS_ASSERT(0);

        /* ���ش��󣬴�ӡһ�� */
        for (ulLen=0,ulLenUsed=0; ulLen<64; ulLen++)
        {
            ulLenUsed += dos_snprintf(szBuffer+ulLenUsed, sizeof(ulLenUsed) - ulLenUsed, "%02X ", *((U8 *)(ptr + ulLen)));

            if (ulLen != 0 && !(ulLen % 16))
            {
                dos_syslog(LOG_LEVEL_DEBUG, szBuffer);

                ulLenUsed = 0;
            }
        }
    }

    /* �����ڴ�����������Ϣ��ȷ��� */
    /* �ͷ��ڴ� */
    pthread_mutex_lock(&g_mutexMemMngtTable);
    if (0 == pstFileDescNode->ulRef)
    {
        //DOS_ASSERT(0);
    }
    else
    {
        pstFileDescNode->ulRef--;
    }

    if (pstFileDescNode->ulTotalSize < pstMemCCB->ulSize)
    {
        //DOS_ASSERT(0);
    }
    else
    {
        pstFileDescNode->ulTotalSize -= pstMemCCB->ulSize;
    }
    pthread_mutex_unlock(&g_mutexMemMngtTable);
    free(ptr);
    /* �޸�ħ���� */
    MEM_SET_FREE_MAGIC(pstMemCCB);
}


/* private functions */
/*
 * ������static U32 _mem_string_hash(S8 *pszString, U32 *pulIndex)
 * ���ܣ������ַ���pszstring��hashֵ
 * ������
 *      S8 *pszString����Ҫ����hashֵ���ַ���
 *      U32 *pulIndex���������������֮��Ľ��
 * ����ֵ���ɹ�����0���ǲ����ظ�ֵ
 *  */
static U32 _mem_string_hash(S8 *pszString, U32 *pulIndex)
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

    *pulIndex = ulHashVal % HASH_MAX_ELEMENT_NUM;

    return DOS_SUCC;
}

/*
 * ������static S32 _mem_find_node(VOID *pSymName, HASH_NODE_S *pNode)
 * ���ܣ�hash�ڲ���ʱ�ԱȵĻص�����
 * ������
 *      VOID *pSymName������ʱ������ַ���key
 *      HASH_NODE_S *pNode����ǰҪ�ԱȵĽڵ�ָ��
 * ����ֵ���ɹ�����0���ǲ����ظ�ֵ
 *  */
static S32 _mem_find_node(VOID *pSymName, HASH_NODE_S *pNode)
{
    MEM_INFO_NODE_ST *pstMemInfoNode = (MEM_INFO_NODE_ST *)pNode;
    S8 szBuff[256] = { 0 };

    snprintf(szBuff, sizeof(szBuff), "%s:%d", pstMemInfoNode->szFileName, pstMemInfoNode->ulLine);

    if (0 == dos_strncmp(szBuff, (S8 *)pSymName, sizeof(szBuff)))
    {
        return 0;
    }

    return -1;
}

/**
 * ������VOID mem_printf(HASH_NODE_S *pNode, U32 ulIndex)
 * ���ܣ�����hash������ӡ����
 * ������
 *      HASH_NODE_S *pNode���ڴ������ڵ�
 *      U32 ulIndex���ͻ�������
 * ����ֵ��
 */
VOID mem_printf(HASH_NODE_S *pNode, VOID *pulIndex)
{
    MEM_INFO_NODE_ST *pstMemInfoNode = (MEM_INFO_NODE_ST *)pNode;
    S8 szBuff[512];
    U32 ulLen;
    U32 ulIndex;

    ulIndex = *(U32 *)pulIndex;

    ulLen = snprintf(szBuff, sizeof(szBuff)
            , "%-40s%6u%6u\r\n"
            , pstMemInfoNode->szFileName
            , pstMemInfoNode->ulLine
            , pstMemInfoNode->ulRef);
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

VOID mem_printf_save(HASH_NODE_S *pNode, VOID *arg)
{
    MEM_INFO_NODE_ST *pstMemInfoNode = (MEM_INFO_NODE_ST *)pNode;
    GET_MEM_INFO_PARAM_ST *pstParam = (GET_MEM_INFO_PARAM_ST *)arg;
    S8 *szInfoBuff = pstParam->szInfoBuff;
    U32 ulBuffSize = pstParam->ulBuffSize;
    U32 ulBuffLen  = pstParam->ulBuffLen;
    U32 ulLen = 0;

    if (NULL == szInfoBuff || ulBuffSize <= ulBuffLen)
    {
        return;
    }

    ulLen = dos_snprintf(szInfoBuff+ulBuffLen, ulBuffSize-ulBuffLen
            , "%-40s%6u%6u\r\n"
            , pstMemInfoNode->szFileName
            , pstMemInfoNode->ulLine
            , pstMemInfoNode->ulRef);
    if (ulLen >= ulBuffSize-ulBuffLen)
    {
        pstParam->ulBuffLen = ulBuffSize;
        return;
    }

    pstParam->ulBuffLen = ulBuffLen + ulLen;
}

/**
 * ������S32 cli_cmd_mem(U32 ulIndex, S32 argc, S8 **argv)
 * ���ܣ������лص���������ӡ�ڴ��������
 * ������
 * ����ֵ��
 */
S32 cli_cmd_mem(U32 ulIndex, S32 argc, S8 **argv)
{
    S8 szTitle[512];
    U32 ulLen;

    ulLen = snprintf(szTitle, sizeof(szTitle), "\r\n%-40s%6s%6s\r\n", "File Name", "Line", "Refer");
    if (ulLen < sizeof(szTitle))
    {
        szTitle[ulLen] = '\0';
        ulLen++;
    }
    else
    {
        szTitle[sizeof(szTitle) - 1] = '\0';
    }

    cli_out_string(ulIndex, "Memory Info:\r\n");
    cli_out_string(ulIndex, szTitle);

    pthread_mutex_lock(&g_mutexMemMngtTable);
    hash_walk_table(g_pstHashMemMngtTable,  (VOID *)&ulIndex, mem_printf);
    pthread_mutex_unlock(&g_mutexMemMngtTable);
    return 0;
}


S32 cli_cmd_get_mem_info(S8 *szInfoBuff, U32 ulBuffSize)
{
    if (NULL == szInfoBuff)
    {
        return DOS_FAIL;
    }

    U32 ulLen = 0;
    GET_MEM_INFO_PARAM_ST stParam;

    ulLen = dos_snprintf(szInfoBuff, ulBuffSize, "Memory Info:\r\n\r\n%-40s%6s%6s\r\n", "File Name", "Line", "Refer");
    if (ulLen >= ulBuffSize)
    {
        return DOS_SUCC;
    }

    stParam.szInfoBuff = szInfoBuff;
    stParam.ulBuffSize = ulBuffSize;
    stParam.ulBuffLen  = ulLen;

    pthread_mutex_lock(&g_mutexMemMngtTable);
    hash_walk_table(g_pstHashMemMngtTable,  (VOID *)&stParam, mem_printf_save);
    pthread_mutex_unlock(&g_mutexMemMngtTable);

    return DOS_SUCC;

}


#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
