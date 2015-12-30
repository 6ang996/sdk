/*
 *            (C) Copyright 2014, ����Ѷͨ . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  memory.h
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: �ڴ����ģ����ض���
 *     History:
 */

#ifndef __MEMORY_H__
#define __MEMORY_H__

#if INCLUDE_MEMORY_MNGT

/* ����ħ���� */
#define MEM_CCB_MEGIC           0x002B3CF4
#define MEM_CCB_FREE_MEGIC      0x00EB3CF4

/* �����ڴ����� */
#define MEM_TYPE_STATIC   0x01000000
#define MEM_TYPE_DYNANIC  0x00000000

/* ��ش������ */
#define MEM_SET_MAGIC(p)                             \
do {                                                 \
    (p)->ulMemDesc = (p)->ulMemDesc | MEM_CCB_MEGIC; \
}while(0)

#define MEM_SET_TYPE(p, type)                      \
do {                                               \
    (p)->ulMemDesc = (p)->ulMemDesc | (type);      \
}while(0)

#define MEM_SET_FREE_MAGIC(p)                             \
do {                                                 \
    (p)->ulMemDesc = (p)->ulMemDesc | MEM_CCB_FREE_MEGIC; \
}while(0)


#define MEM_CHECK_MAGIC(p) (MEM_CCB_MEGIC == (p)->ulMemDesc)
#define MEM_CHECK_FREE_MAGIC(p) (MEM_CCB_FREE_MEGIC == (p)->ulMemDesc)

typedef struct tagMemInfoNode{
    HASH_NODE_S stNode;   /* hash��ڵ�, �뱣�ָó�ԱΪ��һ��Ԫ�� */
    S8 szFileName[48];    /* �����ڴ���õ����ڵ��ļ� */
    U32 ulLine;           /* �����ڴ���õ����ڵ����� */

    U32 ulRef;            /* ��ǰ����֮��û���ͷŵ��ڴ���� */
    U32 ulTotalSize;      /* ��ǰ�ڴ����������ڴ��С�ܺ�, ��ǰ��û���ͷŵ��ڴ� */
}MEM_INFO_NODE_ST;

typedef struct tagMemCCB{
    MEM_INFO_NODE_ST  *pstRefer;   /* ָ���ļ����������ڵ� */
    U32               ulMemDesc;   /* ��һ�ֽ�ָ�����ڴ��type, �����ֽڴ洢ħ���� */
    U32               ulSize;      /* �����ڴ�Ĵ�С */
}MEM_CCB_ST;

typedef struct tagGetMemInfoParam
{
    S8      *szInfoBuff;
    U32     ulBuffSize;
    U32     ulBuffLen;

}GET_MEM_INFO_PARAM_ST;

#endif

#endif
