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

extern double ceil(double x);

double dos_ceil(double x)
{
    return ceil(x);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

