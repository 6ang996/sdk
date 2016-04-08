/*
 *            (C) Copyright 2014, ����Ѷͨ . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  task_lib.c
 *
 *  Created on: 2014-11-6
 *      Author: Larry
 *        Todo: ��װϵͳ��صĺ�������߿�һ������
 *     History:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>

S8 *dos_get_localtime(U32 ulTimestamp, S8 *pszBuffer, S32 lLength)
{
    S32 MON1[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};    /* ƽ�� */
    S32 MON2[12] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};    /* ���� */
    static const S32 FOURYEARS = (366 + 365 +365 +365);                 /* ÿ������������� */
    static const S32 DAYMS = 24*3600;                                   /* ÿ������� */
    S32 lDaysTotal = 0;                                                 /* ������ */
    S32 lYear4 = 0;                                                     /* �õ���1970�����������ڣ�4�꣩�Ĵ��� */
    S32 lDayRemain = 0;                                                 /* �õ�����һ�����ڵ����� */
    S32 lSecondRemain = 0;                                              /* ����һ��ʣ������� */
    S32 lYear = 0, lMonth = 0, lDays = 0, lHour = 0, lMin = 0, lSec = 0;
    BOOL bLeapYear = DOS_FALSE;
    S32 *pMonths = NULL;
    S32 i = 0;
    S32 lTemp = 0;

    if (DOS_ADDR_INVALID(pszBuffer) || lLength < TIME_STR_LEN)
    {
        DOS_ASSERT(0);
        return NULL;
    }

    /* ��ȡ����ʱ�� */
    ulTimestamp     += 8 * 3600;
    lDaysTotal      = ulTimestamp / DAYMS;
    lYear4          = lDaysTotal / FOURYEARS;
    lDayRemain      = lDaysTotal % FOURYEARS;
    lYear           = 1970 + lYear4 * 4;
    lSecondRemain   = ulTimestamp % DAYMS;

    if (lDayRemain < 365)                               /* һ�������ڣ���һ�� */
    {
        /* ƽ�� */
    }
    else if (lDayRemain < (365+365))                    /* һ�������ڣ��ڶ��� */
    {
        /* ƽ�� */
        lYear += 1;
        lDayRemain -= 365;
    }
    else if (lDayRemain < (365+365+365))                /* һ�������ڣ������� */
    {
        /* ƽ�� */
        lYear += 2;
        lDayRemain -= (365+365);
    }
    else                                                /* һ�������ڣ������꣬��һ�������� */
    {
        /* ���� */
        lYear += 3;
        lDayRemain -= (365+365+365);
        bLeapYear = DOS_TRUE;
    }

    pMonths = bLeapYear ? MON2 : MON1;

    for (i=0; i<12; ++i)
    {
        lTemp = lDayRemain - pMonths[i];
        if (lTemp <= 0)
        {
            lMonth = i + 1;
            if (lTemp == 0) /* ��ʾ�պ�������µ����һ�죬��ô������������µ��������� */
            {
                lDays = pMonths[i];
            }
            else
            {
                lDays = lDayRemain;
            }
            break;
        }

        lDayRemain = lTemp;
    }

    lHour = lSecondRemain / 3600;
    lMin = (lSecondRemain % 3600) / 60;
    lSec = (lSecondRemain % 3600) % 60;

    dos_snprintf(pszBuffer, lLength, "%04d-%02d-%02d %02d:%02d:%02d", lYear, lMonth, lDays, lHour, lMin, lSec);

    return pszBuffer;
}

struct tm *dos_get_localtime_struct(U32 ulTimestamp, struct tm *pstTime)
{
    S32 MON1[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};    /* ƽ�� */
    S32 MON2[12] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};    /* ���� */
    static const S32 FOURYEARS = (366 + 365 +365 +365);                 /* ÿ������������� */
    static const S32 DAYMS = 24*3600;                                   /* ÿ������� */
    S32 lDaysTotal = 0;                                                 /* ������ */
    S32 lYear4 = 0;                                                     /* �õ���1970�����������ڣ�4�꣩�Ĵ��� */
    S32 lDayRemain = 0;                                                 /* �õ�����һ�����ڵ����� */
    S32 lSecondRemain = 0;                                              /* ����һ��ʣ������� */
    S32 lYear = 0, lMonth = 0, lDays = 0, lHour = 0, lMin = 0, lSec = 0;
    BOOL bLeapYear = DOS_FALSE;
    S32 *pMonths = NULL;
    S32 i = 0;
    S32 lTemp = 0;

    if (DOS_ADDR_INVALID(pstTime))
    {
        DOS_ASSERT(0);
        return NULL;
    }

    /* ��ȡ����ʱ�� */
    ulTimestamp     += 8 * 3600;
    lDaysTotal      = ulTimestamp / DAYMS;
    lYear4          = lDaysTotal / FOURYEARS;
    lDayRemain      = lDaysTotal % FOURYEARS;
    lYear           = 1970 + lYear4 * 4;
    lSecondRemain   = ulTimestamp % DAYMS;

    if (lDayRemain < 365)                               /* һ�������ڣ���һ�� */
    {
        /* ƽ�� */
    }
    else if (lDayRemain < (365+365))                    /* һ�������ڣ��ڶ��� */
    {
        /* ƽ�� */
        lYear += 1;
        lDayRemain -= 365;
    }
    else if (lDayRemain < (365+365+365))                /* һ�������ڣ������� */
    {
        /* ƽ�� */
        lYear += 2;
        lDayRemain -= (365+365);
    }
    else                                                /* һ�������ڣ������꣬��һ�������� */
    {
        /* ���� */
        lYear += 3;
        lDayRemain -= (365+365+365);
        bLeapYear = DOS_TRUE;
    }

    pMonths = bLeapYear ? MON2 : MON1;

    for (i=0; i<12; ++i)
    {
        lTemp = lDayRemain - pMonths[i];
        if (lTemp <= 0)
        {
            lMonth = i + 1;
            if (lTemp == 0) /* ��ʾ�պ�������µ����һ�죬��ô������������µ��������� */
            {
                lDays = pMonths[i];
            }
            else
            {
                lDays = lDayRemain;
            }
            break;
        }

        lDayRemain = lTemp;
    }

    if (lSecondRemain)
    {
        /* ��һ�� */
        lDaysTotal++;
    }
    pstTime->tm_wday = (lDaysTotal + 3) % 7;

    lHour = lSecondRemain / 3600;
    lMin = (lSecondRemain % 3600) / 60;
    lSec = (lSecondRemain % 3600) % 60;

    pstTime->tm_year = lYear - 1900;
    pstTime->tm_mon = lMonth - 1;
    pstTime->tm_mday = lDays;
    pstTime->tm_hour = lHour;
    pstTime->tm_min = lMin;
    pstTime->tm_sec = lSec;

    return pstTime;
}

/**
 * ����: void dos_task_delay(U32 ulMsSec)
 * ����: ʹĳһ���߳�˯�� ulMsSec����֮����ִ��
 * ����: U32 ulMSEC˯�ߵĺ�����
 * ����ֵ: void
 */
DLLEXPORT VOID dos_task_delay(U32 ulMSec)
{
    if (0 == ulMSec)
    {
        return;
    }

    usleep(ulMSec * 1000);
}

/**
 * ����: void dos_clean_watchdog()
 * ����: ������Ź�
 * ����: NULL
 * ����ֵ: void
 */
DLLEXPORT void dos_clean_watchdog()
{

}

DLLEXPORT VOID dos_random_init()
{
    srand((unsigned)time( NULL ));
}

DLLEXPORT U32 dos_random(U32 ulMax)
{
    return rand() % ulMax;
}

DLLEXPORT U32 dos_random_range(U32 ulMin, U32 ulMax)
{
    U32 ulRand = 0;
    S32 i;

    if (ulMin >= ulMax)
    {
        DOS_ASSERT(0);
        return U32_BUTT;
    }

    i = 1000;
    while (i-- > 0)
    {
        ulRand = rand() % ulMax;

        if (ulRand >= ulMin && ulRand < ulMax)
        {
            break;
        }
    }

    if (ulRand < ulMin || ulRand >= ulMax)
    {
        DOS_ASSERT(0);
        return U32_BUTT;
    }

    return ulRand;
}



#ifdef __cplusplus
}
#endif /* __cplusplus */


