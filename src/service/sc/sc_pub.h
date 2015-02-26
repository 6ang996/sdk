/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  �ļ���: sc_pub.h
 *
 *  ����ʱ��: 2014��12��25��17:35:03
 *  ��    ��: Larry
 *  ��    ��: ����ҵ�����ģ��Ĺ�������
 *  �޸���ʷ:
 */

#ifndef __SC_PUB_H__
#define __SC_PUB_H__

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* include public header files */
#include <dos.h>
#include <pthread.h>
#include <semaphore.h>


/* include private header files */

/* define marcos */
/* ����HTTP������������ */
#define SC_MAX_HTTPD_NUM               1


/* ���������16���ͻ�������HTTP������ */
#define SC_MAX_HTTP_CLIENT_NUM         16

/* ����HTTP API������������ */
#define SC_API_PARAMS_MAX_NUM          24

/* ����HTTP�������У��ļ�������󳤶� */
#define SC_HTTP_REQ_MAX_LEN            64

/* ����ESLģ���������󳤶� */
#define SC_ESL_CMD_BUFF_LEN            1024

/* �������ģ�飬���������� */
#define SC_DEFAULT_PLAYCNT             3

/* �������ģ�飬������˽�����ʱʱ�� */
#define SC_MAX_TIMEOUT4NOANSWER        10

/* �绰���볤�� */
#define SC_TEL_NUMBER_LENGTH           24

/* ���ų������ֵ */
#define SC_EMP_NUMBER_LENGTH           8

/* ÿ���������ʱ��������ڵ� */
#define SC_MAX_PERIOD_NUM              4

/* UUID ��󳤶� */
#define SC_MAX_UUID_LENGTH             40

/* �����ļ������� */
#define SC_MAX_AUDIO_FILENAME_LEN      128

/* �������еı��� */
#define SC_MAX_CALL_MULTIPLE           3

#define SC_MAX_SRV_TYPE_PRE_LEG        4

/* ���������� */
#define SC_MAX_CALL                    3000

/* ��������� */
#define SC_MAX_TASK_NUM                1024

/* �������� */
#define SC_MAX_SCB_NUM                 SC_MAX_CALL*2

/* SCB hash������� */
#define SC_MAX_SCB_HASH_NUM            4096

/* ÿ���������ϯ����� */
#define SC_MAX_SITE_NUM                1024

/* ÿ���������к���������� */
#define SC_MAX_CALLER_NUM              1024

/* ÿ�����񱻽к�����С����(һ��LOAD�����������ʵ�ʻ����ʵ���������) */
#define SC_MIN_CALLEE_NUM              65535

/* ����һ�����������Գ���ʱ����12Сʱ(43200s) */
#define SC_MAX_CALL_DURATION           43200

/* ����ͬһ�����뱻�ظ����е����ʱ������4Сʱ */
#define SC_MAX_CALL_INTERCAL           60 * 4

#define SC_CALL_THRESHOLD_VAL0         40         /* ��ֵ0���ٷֱ� ����Ҫ�������з�����*/
#define SC_CALL_THRESHOLD_VAL1         80         /* ��ֵ1���ٷֱ� ����Ҫ�������з�����*/

#define SC_CALL_INTERVAL_VAL0          300        /* ��ǰ������/��������*100 < ��ֵ0��300����һ�� */
#define SC_CALL_INTERVAL_VAL1          500        /* ��ǰ������/��������*100 < ��ֵ1��500����һ�� */
#define SC_CALL_INTERVAL_VAL2          1000       /* ��ǰ������/��������*100 > ��ֵ1��1000����һ�� */

#define SC_MEM_THRESHOLD_VAL0          90         /* ϵͳ״̬��ֵ���ڴ�ռ���ʷ�ֵ0 */
#define SC_MEM_THRESHOLD_VAL1          95         /* ϵͳ״̬��ֵ���ڴ�ռ���ʷ�ֵ0 */
#define SC_CPU_THRESHOLD_VAL0          90         /* ϵͳ״̬��ֵ��CPUռ���ʷ�ֵ0 */
#define SC_CPU_THRESHOLD_VAL1          95         /* ϵͳ״̬��ֵ��CPUռ���ʷ�ֵ0 */

/* define enums */

/* define structs */

/* declare functions */
#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __SC_HTTPD_PUB_H__ */


