/*
 *            (C) Copyright 2014, ����Ѷͨ . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  endian.c
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: �������������������໥ת���ĺ���
 *     History:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>

/**
 * ������static inline BOOL test_id_big_endian()
 * ���ܣ��ж������Ƿ�Ϊ�����
 * ������
 * ����ֵ������Ǵ���򷵻��棬���򷵻ؼ�
 */
static inline BOOL test_id_big_endian()
{
    S32 lTest = 1;

    if(((char*)&lTest)[sizeof(S32) - 1] ==1)
    {
        return DOS_TRUE;
    }
    else
    {
        return DOS_FALSE;
    }
}

/**
 * ������U32 dos_htonl(U32 ulSrc)
 * ���ܣ�������ת������
 * ������
 * 		U32 ulSrc����Ҫת��������
 * ����ֵ����������������
 */
DLLEXPORT U32 dos_htonl(U32 ulSrc)
{
	U32 ulTmp;

	if (test_id_big_endian())
	{
		return ulSrc;
	}
	else
	{
		ulTmp = ((ulSrc & 0xFF000000) >> 24) |  ((ulSrc & 0x00FF0000) >> 8) |
                ((ulSrc & 0x0000FF00) << 8 ) |  ((ulSrc & 0x000000FF) << 24);

		return ulTmp;
	}
}

/**
 * ������U32 dos_htonl(U32 ulSrc)
 * ���ܣ�������ת������
 * ������
 * 		U32 ulSrc����Ҫת��������
 * ����ֵ����������������
 */
DLLEXPORT U32 dos_ntohl(U32 ulSrc)
{
	return dos_htonl(ulSrc);
}


/**
 * ������U16 dos_htonl(U16 usSrc)
 * ���ܣ�������ת������
 * ������
 * 		U16 usSrc����Ҫת��������
 * ����ֵ����������������
 */
DLLEXPORT U16 dos_htons(U16 usSrc)
{
	U16 usTmp;

	if (test_id_big_endian())
	{
		return usSrc;
	}
	else
	{
		usTmp = ((usSrc & 0xFF00)>>8) |  ((usSrc & 0x00FF)<<8);

		return usTmp;
	}
}

/**
 * ������U16 dos_htonl(U16 usSrc)
 * ���ܣ�������ת������
 * ������
 * 		U16 usSrc����Ҫת��������
 * ����ֵ����������������
 */
DLLEXPORT U16 dos_ntohs(U16 usSrc)
{
	return dos_htons(usSrc);
}


#ifdef __cplusplus
}
#endif /* __cplusplus */

