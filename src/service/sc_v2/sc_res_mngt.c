/**
 * @file : sc_res_mngt.c
 *
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 * ������Դ����
 *
 * @date: 2016��1��14��
 * @arthur: Larry
 */

#ifdef __cplusplus
extern "C" {
#endif /* End of __cplusplus */

#include <dos.h>
#include "sc_def.h"
#include "sc_res.h"
#include "sc_pub.h"
#include "sc_debug.h"


/** SIP�˻�HASH�� REFER TO SC_USER_ID_NODE_ST */
HASH_TABLE_S             *g_pstHashSIPUserID  = NULL;
pthread_mutex_t          g_mutexHashSIPUserID = PTHREAD_MUTEX_INITIALIZER;

/** DID����hash�� REFER TO SC_DID_NODE_ST */
HASH_TABLE_S             *g_pstHashDIDNum  = NULL;
pthread_mutex_t          g_mutexHashDIDNum = PTHREAD_MUTEX_INITIALIZER;

/* ·���������� REFER TO SC_ROUTE_NODE_ST */
DLL_S                    g_stRouteList;
pthread_mutex_t          g_mutexRouteList = PTHREAD_MUTEX_INITIALIZER;

/* ��ҵ���� */
DLL_S                    g_stCustomerList;
pthread_mutex_t          g_mutexCustomerList = PTHREAD_MUTEX_INITIALIZER;

/* �����б� refer to SC_GW_NODE_ST (ʹ��HASH) */
HASH_TABLE_S             *g_pstHashGW = NULL;
pthread_mutex_t          g_mutexHashGW = PTHREAD_MUTEX_INITIALIZER;

/* �������б� refer to SC_GW_GRP_NODE_ST (ʹ��HASH) */
HASH_TABLE_S             *g_pstHashGWGrp = NULL;
pthread_mutex_t          g_mutexHashGWGrp = PTHREAD_MUTEX_INITIALIZER;

/* ����任�������� REFER TO SC_NUM_TRANSFORM_NODE_ST */
DLL_S                    g_stNumTransformList;
pthread_mutex_t          g_mutexNumTransformList = PTHREAD_MUTEX_INITIALIZER;

/* ������HASH�� */
HASH_TABLE_S             *g_pstHashBlackList  = NULL;
pthread_mutex_t          g_mutexHashBlackList = PTHREAD_MUTEX_INITIALIZER;

HASH_TABLE_S             *g_pstHashCaller = NULL;
pthread_mutex_t          g_mutexHashCaller = PTHREAD_MUTEX_INITIALIZER;

/* ���к������б�����SC_CALLER_GRP_NODE_ST */
HASH_TABLE_S             *g_pstHashCallerGrp = NULL;
pthread_mutex_t          g_mutexHashCallerGrp = PTHREAD_MUTEX_INITIALIZER;

/* TT�ź�EIX��Ӧ��ϵ�� */
HASH_TABLE_S             *g_pstHashTTNumber = NULL;
pthread_mutex_t          g_mutexHashTTNumber = PTHREAD_MUTEX_INITIALIZER;

/* ���к�������hash�� refer to SC_NUMBER_LMT_NODE_ST */
HASH_TABLE_S             *g_pstHashNumberlmt = NULL;
pthread_mutex_t          g_mutexHashNumberlmt = PTHREAD_MUTEX_INITIALIZER;

/* ҵ����� */
HASH_TABLE_S             *g_pstHashServCtrl = NULL;
pthread_mutex_t          g_mutexHashServCtrl = PTHREAD_MUTEX_INITIALIZER;

/* ���к����趨�б�����SC_CALLER_SETTING_ST */
HASH_TABLE_S             *g_pstHashCallerSetting = NULL;
pthread_mutex_t          g_mutexHashCallerSetting = PTHREAD_MUTEX_INITIALIZER;

U32 sc_res_init()
{
    DLL_Init(&g_stRouteList);
    DLL_Init(&g_stCustomerList);
    DLL_Init(&g_stNumTransformList);

    g_pstHashGWGrp = hash_create_table(SC_GW_GRP_HASH_SIZE, NULL);
    g_pstHashGW = hash_create_table(SC_GW_HASH_SIZE, NULL);
    g_pstHashDIDNum = hash_create_table(SC_DID_HASH_SIZE, NULL);
    g_pstHashSIPUserID = hash_create_table(SC_SIP_ACCOUNT_HASH_SIZE, NULL);
    g_pstHashBlackList = hash_create_table(SC_BLACK_LIST_HASH_SIZE, NULL);
    g_pstHashTTNumber = hash_create_table(SC_EIX_DEV_HASH_SIZE, NULL);
    g_pstHashCaller = hash_create_table(SC_CALLER_HASH_SIZE, NULL);
    g_pstHashCallerGrp = hash_create_table(SC_CALLER_GRP_HASH_SIZE, NULL);
    g_pstHashCallerSetting = hash_create_table(SC_CALLER_SETTING_HASH_SIZE, NULL);
    g_pstHashServCtrl = hash_create_table(SC_SERV_CTRL_HASH_SIZE, NULL);
    if (DOS_ADDR_INVALID(g_pstHashGW)
        || DOS_ADDR_INVALID(g_pstHashGWGrp)
        || DOS_ADDR_INVALID(g_pstHashDIDNum)
        || DOS_ADDR_INVALID(g_pstHashSIPUserID)
        || DOS_ADDR_INVALID(g_pstHashBlackList)
        || DOS_ADDR_INVALID(g_pstHashCaller)
        || DOS_ADDR_INVALID(g_pstHashCallerGrp)
        || DOS_ADDR_INVALID(g_pstHashTTNumber)
        || DOS_ADDR_INVALID(g_pstHashServCtrl))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_RES), "Alloc memort for res hash table fail");

        return DOS_FAIL;
    }

    /* �����������˳���ܸ��� */
    sc_gateway_load(SC_INVALID_INDEX);
    sc_gateway_group_load(SC_INVALID_INDEX);
    sc_gateway_relationship_load();

    sc_route_load(SC_INVALID_INDEX);
    sc_transform_load(SC_INVALID_INDEX);
    sc_customer_load(SC_INVALID_INDEX);
    sc_did_load(SC_INVALID_INDEX);
    sc_sip_account_load(SC_INVALID_INDEX);
    sc_black_list_load(SC_INVALID_INDEX);
    sc_eix_dev_load(SC_INVALID_INDEX);

    /* ��������ļ���˳��ͬ��������,ͬʱ���뱣֤֮ǰ�Ѿ�����DID����(������ҵ���߼�) */
    sc_caller_load(SC_INVALID_INDEX);
    sc_caller_group_load(SC_INVALID_INDEX);
    sc_caller_relationship_load();

    sc_caller_setting_load(SC_INVALID_INDEX);
    sc_serv_ctrl_load(SC_INVALID_INDEX);

    return DOS_SUCC;
}

U32 sc_res_start()
{
    return DOS_SUCC;
}

U32 sc_res_stop()
{
    return DOS_SUCC;
}



#ifdef __cplusplus
}
#endif /* End of __cplusplus */


