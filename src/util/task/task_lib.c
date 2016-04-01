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
    S32 lYear, lMonth, lDays, lHour, lMin, lSec;
    S32 lSecondsUsed =0;
    const S32 lSecondsInYear = 365 * 24 * 60 * 60;
    const S32 lsecondsInLeapYear = 366 * 24 * 60 * 60;
    const S32 lMonthDays[] = {31, 0, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    const S32 lSecondsInDay = 24 * 60 * 60;

    if (DOS_ADDR_INVALID(pszBuffer) || lLength < TIME_STR_LEN)
    {
        DOS_ASSERT(0);
        return NULL;
    }

    /* ��ȡ����ʱ�� */
    ulTimestamp += 8 * 60 * 60;

    lYear = 1970;
    while (1)
    {
        if((lYear % 4 == 0 && lYear % 100 != 0) || lYear % 400 == 0)
        {
            if (ulTimestamp - lSecondsUsed < lsecondsInLeapYear)
            {
                break;
            }

            lSecondsUsed += lsecondsInLeapYear;
        }
        else
        {
            if (ulTimestamp - lSecondsUsed < lSecondsInYear)
            {
                break;
            }

            lSecondsUsed += lSecondsInYear;
        }

        lYear++;
    }

    lMonth = 0;
    while (1)
    {
        if (1 == lMonth)
        {
            if((lYear%4 == 0 && lYear % 100 != 0) || lYear % 400 == 0)
            {
                if (ulTimestamp - lSecondsUsed < 29 * lSecondsInDay)
                {
                    break;
                }

                lSecondsUsed += 29 * lSecondsInDay;
            }
            else
            {
                if (ulTimestamp - lSecondsUsed < 28 * lSecondsInDay)
                {
                    break;
                }

                lSecondsUsed += 28 * lSecondsInDay;
            }
        }
        else
        {
            if (ulTimestamp - lSecondsUsed < lMonthDays[lMonth] * lSecondsInDay)
            {
                break;
            }
            lSecondsUsed += lMonthDays[lMonth] * lSecondsInDay;
        }

        lMonth++;
    }

    lMonth++;

    lDays = (ulTimestamp - lSecondsUsed) / (lSecondsInDay);
    lDays++;

    lHour = (ulTimestamp % (lSecondsInDay)) / (60 * 60);
    lMin = (ulTimestamp / 60 ) % 60;
    lSec = ulTimestamp % 60;

    dos_snprintf(pszBuffer, lLength, "%04d-%02d-%02d %02d:%02d:%02d", lYear, lMonth, lDays, lHour, lMin, lSec);

    return pszBuffer;
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


