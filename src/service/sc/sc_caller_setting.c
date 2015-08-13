#include <dos.h>
#include <esl.h>
#include "sc_def.h"
#include "sc_ep.h"

/* ����������������� */
#define MAX_WALK_TIMES   20

extern HASH_TABLE_S *g_pstHashCallerSetting;
extern HASH_TABLE_S *g_pstHashCaller;
extern HASH_TABLE_S *g_pstHashCallerGrp;
extern HASH_TABLE_S *g_pstHashDIDNum;

/**
 *  ����:U32  sc_caller_setting_select_number(U32 ulCustomerID, U32 ulSrcID, U32 ulSrcType, U32 ulPolicy, S8 *pszNumber)
 *  ����: ���ݺ���Դ�Ͳ���ѡ�����к���
 *  ����:  U32 ulCustomerID   �ͻ�id���������
 *         U32 ulSrcID        ����Դid, �������
 *         U32 ulSrcType      ����Դ����, �������
 *         U32 ulPolicy       ����ѡ�����,�������
 *         S8 *pszNumber      ���к��뻺�棬�������
 *  ����ֵ: �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 ***/
U32  sc_caller_setting_select_number(U32 ulCustomerID, U32 ulSrcID, U32 ulSrcType, U32 ulPolicy, S8 *pszNumber)
{
    U32 ulDstID = 0, ulDstType = 0, ulRet = 0;

    if (DOS_ADDR_INVALID(pszNumber))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* ���ݺ���Դ��ȡ����Ŀ�� */
    ulRet = sc_get_dst_by_src(ulCustomerID, ulSrcID, ulSrcType, &ulDstID, &ulDstType);
    if (DOS_SUCC != ulRet)
    {
        /* δ�ҵ������Դ��ƥ��ĺ���Ŀ�꣬�ҵ�ǰ����Դ�󶨵�DID���� */
        switch (ulSrcType)
        {
            /* �������Դ����ϯ����ôӦ��ȥ������ϯ�󶨵�DID���� */
            case SC_SRC_CALLER_TYPE_AGENT:
                break;
            /* �������Դ����ϯ�飬������ϯ������ѡһ����ϯ�󶨵�DID���� */
            case SC_SRC_CALLER_TYPE_AGENTGRP:
                break;
            /* ��������еģ��ڵ�ǰ�ͻ��µ�DID��������ѡһ������ */
            case SC_SRC_CALLER_TYPE_ALL:
                {
                    /* ֻ�Ǵ���׮������δʵ�� */
                    ulRet = sc_select_did_random(ulCustomerID, pszNumber);
                    if (DOS_SUCC != ulRet)
                    {
                        DOS_ASSERT(0);
                        return DOS_FAIL;
                    }
                    else
                    {
                        return DOS_SUCC;
                    }
                }
            default:
                break;
        }
        return DOS_SUCC;
    }
    switch (ulDstType)
    {
        case SC_DST_CALLER_TYPE_CFG:
            {
                /* ֱ�Ӳ������и�󡻺�� */
                ulRet = sc_get_number_by_callerid(ulCustomerID, ulDstID, pszNumber);
                if (DOS_SUCC != ulRet)
                {
                    DOS_ASSERT(0);
                    return DOS_FAIL;
                }
                else
                {
                    /* ���ҵ�ǰ��ϯ������ϯ��󶨵�DID */
                    return DOS_SUCC;
                }
            }
        case SC_DST_CALLER_TYPE_DID:
            {
                /* ����did����id��ȡ���к��뻺�� */
                ulRet = sc_get_number_by_didid(ulDstID, pszNumber);
                if (DOS_SUCC != ulRet)
                {
                    DOS_ASSERT(0);
                    return DOS_FAIL;
                }
                else
                {
                    return DOS_SUCC;
                }
            }
        case SC_DST_CALLER_TYPE_CALLERGRP:
            {
                switch (ulPolicy)
                {
                    case SC_CALLER_POLICY_IN_ORDER:
                        {
                            ulRet = sc_select_number_in_order(ulCustomerID, ulDstID, pszNumber);
                            if (DOS_SUCC != ulRet)
                            {
                                DOS_ASSERT(0);
                                return DOS_FAIL;
                            }
                            else
                            {
                                return DOS_SUCC;
                            }
                        }
                    case SC_CALLER_POLICY_RANDOM:
                        {
                            ulRet = sc_select_number_random(ulCustomerID, ulDstID, pszNumber);
                            if (DOS_SUCC != ulRet)
                            {
                                DOS_ASSERT(0);
                                return DOS_FAIL;
                            }
                            else
                            {
                                return DOS_SUCC;
                            }
                        }
                    default:
                        break;
                }
                break;
            }
        default:
            break;
    }
    return DOS_SUCC;
}

/**
 * ����: S32 sc_generate_random(S32 lUp, S32 lDown)
 * ����: ����һ�������x. ����lUpΪ�Ͻ磬lDownΪ�½磬x<=lUp && x>=lDown��������ֵ��ʱ������⴫��������lUpһ������lDown
 * ����: S32 lUp �Ͻ�
 *       S32 lDown �½�
 * ����ֵ: ����һ������lUp��lDown֮���������������߽�ֵlUp��lDown
 **/
S32 sc_generate_random(S32 lUp, S32 lDown)
{
    S32  lDiff = 0;

    srand(time(NULL));
    lDiff = lUp > lDown?(lUp-lDown):(lDown-lUp);

    return rand() % (lDiff + 1) + (lUp > lDown?lDown:lUp);
}

/**
 * ����: U32 sc_get_dst_by_src(U32 ulCustomerID, U32 ulSrcID, U32 ulSrcType, U32 ulDstID, U32 ulDstType)
 * ����: ���ݺ���Դ��ú���Ŀ��
 * ����: U32 ulCustomerID  �ͻ�id
 *       U32 ulSrcID       ����Դid�� �������
 *       U32 ulSrcType     ����Դ����, �������
 *       U32* pulDstID     ����Ŀ��id���������
 *       U32* pulDstType   ����Ŀ�����ͣ��������
 * ����ֵ: ����һ������lUp��lDown֮��������
 **/
U32 sc_get_dst_by_src(U32 ulCustomerID, U32 ulSrcID, U32 ulSrcType, U32* pulDstID, U32* pulDstType)
{
    HASH_NODE_S *pstHashNode = NULL;
    U32 ulHashIndex = U32_BUTT;
    SC_CALLER_SETTING_ST *pstSetting = NULL;

    if (DOS_ADDR_INVALID(pulDstID)
        || DOS_ADDR_INVALID(pulDstType))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulHashIndex = sc_ep_caller_setting_hash_func(ulCustomerID);
    pstHashNode = hash_find_node(g_pstHashCallerSetting, ulHashIndex, &ulCustomerID, sc_ep_caller_setting_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        ||DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        return DOS_FAIL;
    }
    else
    {
        pstSetting = (SC_CALLER_SETTING_ST *)pstHashNode->pHandle;
        *pulDstID = pstSetting->ulDstID;
        *pulDstType = pstSetting->ulDstType;
        return DOS_SUCC;
    }
}

/**
 * ����: U32 sc_get_number_by_callerid(U32 ulCustomerID, U32 ulCallerID, S8 *pszNumber)
 * ����: ����caller id��ȡ���к��뻺��
 * ����: U32 ulCustomerID    �ͻ�id���������
 *       U32 ulCallerID      ���к���id�� �������
 *       S8 *pszNumber       ���к��뻺�棬�������
 * ����ֵ: �ɹ�����DOS_SUCC,���򷵻�DOS_SUCC
 **/
U32 sc_get_number_by_callerid(U32 ulCustomerID, U32 ulCallerID, S8 *pszNumber)
{
    U32 ulHashIndex = U32_BUTT;
    HASH_NODE_S  *pstHashNode = NULL;
    SC_CALLER_QUERY_NODE_ST *pstCaller = NULL;

    if (DOS_ADDR_INVALID(pszNumber))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulHashIndex = sc_ep_caller_hash_func(ulCustomerID);
    pstHashNode = hash_find_node(g_pstHashCaller, ulHashIndex, (VOID *)&ulCustomerID, sc_ep_caller_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    pstCaller = (SC_CALLER_QUERY_NODE_ST *)pstHashNode->pHandle;
    dos_memcpy(pszNumber, pstCaller->szNumber, sizeof(pstCaller->szNumber));

    return DOS_SUCC;
}

/**
 * ����: U32 sc_get_number_by_didid(U32 ulDidID, S8* pszNumber)
 * ����: ����did����id��ȡdid���뻺��
 * ����: U32 ulDidID     �����������������
 *       S8* pszNumber   ���뻺�棬�������
 * ����ֵ: �ɹ�����DOS_SUCC,���򷵻�DOS_SUCC
 **/
U32 sc_get_number_by_didid(U32 ulDidID, S8* pszNumber)
{
    SC_DID_NODE_ST *pstDid = NULL;
    U32  ulHashIndex = U32_BUTT;
    HASH_NODE_S *pstHashNode = NULL;
    BOOL bFound = DOS_FALSE;

    if (DOS_ADDR_INVALID(pszNumber))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    HASH_Scan_Table(g_pstHashDIDNum, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashDIDNum, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            if (DOS_ADDR_INVALID(pstHashNode)
                || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }

            pstDid = (SC_DID_NODE_ST *)pstHashNode->pHandle;
            if (pstDid->ulDIDID != ulDidID)
            {
                continue;
            }
            dos_memcpy(pszNumber, pstDid->szDIDNum, sizeof(pstDid->szDIDNum));
            bFound = DOS_TRUE;
        }
    }
    if (DOS_FALSE == bFound)
    {
        return DOS_FAIL;
    }
    return DOS_SUCC;
}

/**
 * ����: U32 sc_select_number_in_order(U32 ulCustomerID, U32 ulGrpID, S8 *pszNumber)
 * ����: ����ѭ��˳����򷵻�һ������
 * ����: U32 ulCustomerID  �ͻ�id
 *       U32 ulGrpID       ������id
 *       S8 *pszNumber     ���뻺�棬 �������
 * ����ֵ: �ɹ�����DOS_SUCC,���򷵻�DOS_SUCC
 **/
U32 sc_select_number_in_order(U32 ulCustomerID, U32 ulGrpID, S8 *pszNumber)
{
    U32  ulHashIndex = U32_BUTT, ulNewNo = U32_BUTT, ulCount = 0, ulTempNo = 0;
    HASH_NODE_S *pstHashNode = NULL;
    DLL_NODE_S *pstNode = NULL;
    SC_CALLER_GRP_NODE_ST *pstCallerGrp = NULL;
    SC_CALLER_QUERY_NODE_ST *pstCaller = NULL;
    BOOL bFound = DOS_FALSE;

    if (DOS_ADDR_INVALID(pszNumber))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulHashIndex = sc_ep_caller_grp_hash_func(ulCustomerID);
    pstHashNode = hash_find_node(g_pstHashCallerGrp, ulHashIndex, (VOID *)&ulCustomerID, sc_ep_caller_grp_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstCallerGrp = (SC_CALLER_GRP_NODE_ST *)pstHashNode->pHandle;
    /* ���֮ǰ��δ�ù�(��ʼ��״̬��0)������֮ǰ���һ������������һ���ڵ㣬���ͷ��ʼ�� */
    if (0 == pstCallerGrp->ulLastNo || pstCallerGrp->stCallerList.ulCount == pstCallerGrp->ulLastNo)
    {
        ulNewNo = 1;
    }
    else
    {
        /* �����º������һ������ */
        ulNewNo = pstCallerGrp->ulLastNo + 1;
    }

    ulTempNo = ulNewNo;

    DLL_Scan(&pstCallerGrp->stCallerList, pstNode, DLL_NODE_S *)
    {
        ++ulCount;
        if (ulCount == ulNewNo)
        {
            /* ����ҵ��˸ýڵ㣬��Ϊ�գ������������һ�����õļ��� */
            if (DOS_ADDR_INVALID(pstNode)
                || DOS_ADDR_INVALID(pstHashNode))
            {
                /* �����ˣ���δ�ҵ����õģ���ʱ�˳����Ա������ͷ����ǰ�ڵ����Ѱ�� */
                if (ulNewNo == pstCallerGrp->stCallerList.ulCount)
                {
                    break;
                }
                else
                {
                    ulNewNo++;
                }
                continue;
            }
            else
            {
                pstCaller = (SC_CALLER_QUERY_NODE_ST *)pstNode->pHandle;
                /* �����ݸ��Ƶ����뻺�� */
                dos_memcpy(pszNumber, pstCaller->szNumber, sizeof(pstCaller->szNumber));
                bFound = DOS_TRUE;
                /* ͬʱ�������º��б�� */
                pstCallerGrp->ulLastNo = ulNewNo;
                break;
            }
        }
    }

    if (DOS_FALSE == bFound)
    {
        ulCount = 0;
        /* �����û���ҵ�����ô������ͷ��ʼ����ǰ�ڵ���� */
        DLL_Scan(&pstCallerGrp->stCallerList, pstNode, DLL_NODE_S *)
        {
            ++ulCount;
            if (DOS_ADDR_INVALID(pstNode)
                || DOS_ADDR_INVALID(pstNode->pHandle))
            {
                /* ��û�ҵ�����˵�����ڴ��в����ڸýڵ� */
                if (ulCount == ulTempNo)
                {
                    DOS_ASSERT(0);
                    return DOS_FAIL;
                }
            }
            else
            {
                pstCaller = (SC_CALLER_QUERY_NODE_ST *)pstNode->pHandle;
                dos_memcpy(pszNumber, pstCaller->szNumber, sizeof(pstCaller->szNumber));
                pstCallerGrp->ulLastNo = ulCount;
                return DOS_SUCC;
            }
        }
    }
    else
    {
        return DOS_SUCC;
    }
    return DOS_FAIL;
}

/**
 * ����: U32 sc_select_number_random(U32 ulCustomerID, U32 ulGrpID, S8 *pszNumber)
 * ����: ����������򷵻�һ������
 * ����: U32 ulCustomerID  �ͻ�id
 *       U32 ulGrpID       ������id
 *       S8 *pszNumber     ���뻺�棬 �������
 * ����ֵ: �ɹ�����DOS_SUCC,���򷵻�DOS_SUCC
 **/
U32 sc_select_number_random(U32 ulCustomerID, U32 ulGrpID, S8 *pszNumber)
{
    S32  lNum = U32_BUTT, lLoop = U32_BUTT;
    BOOL bFound = DOS_FALSE;
    HASH_NODE_S *pstHashNode = NULL;
    DLL_NODE_S  *pstNode = NULL;
    SC_CALLER_QUERY_NODE_ST *pstCaller= NULL;
    SC_CALLER_GRP_NODE_ST *pstCallerGrp = NULL;
    U32  ulHashIndex = U32_BUTT, ulCount = 0;

    if (DOS_ADDR_INVALID(pszNumber))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    ulHashIndex = sc_ep_caller_grp_hash_func(ulCustomerID);
    pstHashNode = hash_find_node(g_pstHashCallerGrp, ulHashIndex, (VOID *)&ulCustomerID, sc_ep_caller_grp_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstCallerGrp = (SC_CALLER_GRP_NODE_ST *)pstHashNode->pHandle;

    /* ������MAX_WALK_TIMES�Σ������û�ҵ����ʵ����к��룬����Ϊ����ʧ�ܣ���ֹ���޴������ѭ�� */
    for (lLoop = 0; lLoop < MAX_WALK_TIMES; lLoop++)
    {
        lNum = sc_generate_random(1, pstCallerGrp->stCallerList.ulCount);

        DLL_Scan(&pstCallerGrp->stCallerList, pstNode, DLL_NODE_S *)
        {
            ++ulCount;
            if (ulCount == lNum)
            {
                /* ����ڵ�Ϊ�գ���������������� */
                if (DOS_ADDR_INVALID(pstNode)
                    || DOS_ADDR_INVALID(pstNode->pHandle))
                {
                    break;
                }
                else
                {
                    pstCaller = (SC_CALLER_QUERY_NODE_ST *)pstNode->pHandle;
                    dos_memcpy(pszNumber, pstCaller->szNumber, sizeof(pstCaller->szNumber));
                    pstCallerGrp->ulLastNo = (U32)lNum;
                    bFound = DOS_TRUE;
                }
            }
        }
        if (DOS_FALSE == bFound)
        {
            /* �����δ�ҵ����������һ��ѭ������ */
            continue;
        }
    }
    if (DOS_FALSE == bFound)
    {
        return DOS_FAIL;
    }
    else
    {
        return DOS_SUCC;
    }
}


/**
 * ����: U32 sc_select_did_random(U32 ulCustomerID, S8 *pszNumber)
 * ����: ����������򷵻�һ��DID����
 * ����: U32 ulCustomerID  �ͻ�id
 *       S8 *pszNumber     ���뻺�棬 �������
 * ����ֵ: �ɹ�����DOS_SUCC,���򷵻�DOS_SUCC
 **/
U32 sc_select_did_random(U32 ulCustomerID, S8 *pszNumber)
{
    return DOS_SUCC;
}


