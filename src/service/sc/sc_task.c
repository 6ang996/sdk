/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  �ļ�����sc_task.c
 *
 *  ����ʱ��: 2014��12��16��10:23:53
 *  ��    ��: Larry
 *  ��    ��: ÿһ��Ⱥ�������ʵ��
 *  �޸���ʷ:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* include public header files */
#include <dos.h>
#include <esl.h>

/* include private header files */
#include "sc_def.h"
#include "sc_debug.h"
#include "sc_acd_def.h"
#include "bs_pub.h"

/* define marcos */

/* define enums */

/* define structs */

extern U32                g_ulMaxConcurrency4Task;



/* declare functions */
inline U32 sc_random(U32 ulMax)
{
    if (!ulMax)
    {
        return U32_BUTT;
    }

    srand((unsigned)time( NULL ));

    return rand() % ulMax;
}

/*
 * ����: SC_TEL_NUM_QUERY_NODE_ST *sc_task_get_callee(SC_TASK_CB_ST *pstTCB)
 * ����: ��ȡ���к���
 * ����:
 *      SC_TASK_CB_ST *pstTCB: ������ƿ�
 * ����ֵ: �ɹ����ر��к�����ƿ�ָ��(�Ѿ��������ˣ�����ʹ����֮��Ҫ�ͷ���Դ)�����򷵻�NULL
 * ���øú���֮����������˺Ϸ�ֵ����Ҫ�ͷŸ���Դ
 */
SC_TEL_NUM_QUERY_NODE_ST *sc_task_get_callee(SC_TASK_CB_ST *pstTCB)
{
    SC_TEL_NUM_QUERY_NODE_ST *pstCallee = NULL;
    list_t                   *pstList = NULL;
    U32                      ulCount = 0;

    SC_TRACE_IN((U64)pstTCB, 0, 0, 0);
    if (DOS_ADDR_INVALID(pstTCB))
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return NULL;
    }

    if (dos_list_is_empty(&pstTCB->stCalleeNumQuery))
    {
        ulCount = sc_task_load_callee(pstTCB);
        sc_logr_info(NULL, SC_TASK, "Load callee number for task %d. Load result : %d", pstTCB->ulTaskID, ulCount);
    }

    if (dos_list_is_empty(&pstTCB->stCalleeNumQuery))
    {
        pstTCB->ucTaskStatus = SC_TASK_STOP;

        sc_logr_info(NULL, SC_TASK, "Task %d has finished. or there is no callees.", pstTCB->ulTaskID);

        return NULL;
    }

    while (1)
    {
        if (dos_list_is_empty(&pstTCB->stCalleeNumQuery))
        {
            break;
        }

        pstList = dos_list_fetch(&pstTCB->stCalleeNumQuery);
        if (!pstList)
        {
            continue;
        }

        pstCallee = dos_list_entry(pstList, SC_TEL_NUM_QUERY_NODE_ST, stLink);
        if (!pstCallee)
        {
            continue;
        }

        break;
    }

    pstCallee->stLink.next = NULL;
    pstCallee->stLink.prev = NULL;

    sc_logr_info(NULL, SC_TASK, "Select callee %s for new call.", pstCallee->szNumber);

    return pstCallee;
}


/*
 * ����: U32 sc_task_make_call(SC_TASK_CB_ST *pstTCB)
 * ����: ����ҵ����ƿ飬����������ӵ�������ģ�飬�ȴ�����
 * ����:
 *      SC_TASK_CB_ST *pstTCB: ������ƿ�
 * ����ֵ: �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
U32 sc_task_make_call(SC_TASK_CB_ST *pstTCB)
{
    SC_SCB_ST                 *pstSCB    = NULL;
    SC_TEL_NUM_QUERY_NODE_ST  *pstCallee = NULL;
    //SC_CALLER_QUERY_NODE_ST   *pstCaller = NULL;
    S8  szCaller[SC_TEL_NUMBER_LENGTH]   = {0};

    SC_TRACE_IN((U64)pstTCB, 0, 0, 0);

    if (DOS_ADDR_INVALID(pstTCB))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pstCallee = sc_task_get_callee(pstTCB);
    if (!pstCallee)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    /* ֻҪȡ���˱��к��룬��Ӧ�ü�һ */
    pstTCB->ulCalledCount++;

    if (sc_get_number_by_callergrp(pstTCB->ulCallerGrpID, szCaller, SC_TEL_NUMBER_LENGTH) != DOS_SUCC)
    {
        DOS_ASSERT(0);
        sc_logr_info(pstSCB, SC_TASK, "Get caller from caller group(%u) FAIL.",pstTCB->ulCallerGrpID);
        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    sc_logr_debug(pstSCB, SC_TASK, "Get caller(%s) from caller group(%u)", szCaller, pstTCB->ulCallerGrpID);

#if 0
    pstCaller = sc_task_get_caller(pstTCB);
    if (!pstCaller)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }
#endif

    pstSCB = sc_scb_alloc();
    if (!pstSCB)
    {
        sc_logr_notice(pstSCB, SC_TASK, "Make call for task %u fail. Alloc SCB fail.", pstTCB->ulTaskID);
        goto fail;
    }

    SC_SCB_SET_STATUS(pstSCB, SC_SCB_INIT);

    if (pstTCB->bTraceCallON || pstSCB->bTraceNo
        || pstCallee->ucTraceON)
    {
        pstSCB->bTraceNo  = 1;
    }
    pstSCB->ulCustomID = pstTCB->ulCustomID;
    pstSCB->ulTaskID = pstTCB->ulTaskID;
    pstSCB->usTCBNo = pstTCB->usTCBNo;
    pstSCB->usSiteNo = U16_BUTT;
    pstSCB->ulTrunkID = U32_BUTT;
    pstSCB->ulCallDuration = 0;
    pstSCB->ucLegRole = SC_CALLER;

    dos_strncpy(pstSCB->szCallerNum, szCaller, SC_TEL_NUMBER_LENGTH);
    pstSCB->szCallerNum[SC_TEL_NUMBER_LENGTH - 1] = '\0';
    dos_strncpy(pstSCB->szCalleeNum, pstCallee->szNumber, SC_TEL_NUMBER_LENGTH);
    pstSCB->szCalleeNum[SC_TEL_NUMBER_LENGTH - 1] = '\0';
    pstSCB->szSiteNum[0] = '\0';
    pstSCB->szUUID[0] = '\0';

    SC_SCB_SET_SERVICE(pstSCB, SC_SERV_AUTO_DIALING);
    SC_SCB_SET_SERVICE(pstSCB, SC_SERV_OUTBOUND_CALL);
    SC_SCB_SET_SERVICE(pstSCB, SC_SERV_EXTERNAL_CALL);

    if (!sc_ep_black_list_check(pstSCB->ulCustomID, pstSCB->szCalleeNum))
    {
        DOS_ASSERT(0);
        sc_logr_info(pstSCB, SC_ESL, "Cannot make call for number %s which is in black list.", pstSCB->szCalleeNum);
        goto fail;
    }

    if (sc_send_usr_auth2bs(pstSCB) != DOS_SUCC)
    {
        sc_call_trace(pstSCB, "Make call failed.");

        //sc_task_callee_set_recall(pstTCB, pstCallee->ulIndex);
        goto fail;
    }

    SC_SCB_SET_STATUS(pstSCB, SC_SCB_AUTH);

    sc_call_trace(pstSCB, "Make call for task %u successfully.", pstTCB->ulTaskID);

    if (pstCallee)
    {
        dos_dmem_free(pstCallee);
        pstCallee = NULL;
    }

    return DOS_SUCC;

fail:
    if (pstCallee)
    {
        dos_dmem_free(pstCallee);
        pstCallee = NULL;
    }

    if (pstSCB)
    {
        DOS_ASSERT(0);
        sc_scb_free(pstSCB);
        pstSCB = NULL;
    }

    return DOS_FAIL;
}

/*
 * ����: VOID *sc_task_runtime(VOID *ptr)
 * ����: ��������������߳�������
 * ����:
 */
VOID *sc_task_runtime(VOID *ptr)
{
    SC_TEL_NUM_QUERY_NODE_ST *pstCallee = NULL;
    SC_TASK_CB_ST   *pstTCB        = NULL;
    list_t          *pstList       = NULL;
    U32             ulTaskInterval = 0;
    S32             lResult        = 0;
    U32             ulMinInterval  = 0;

    if (!ptr)
    {
        sc_logr_error(NULL, SC_TASK, "%s", "Fail to start the thread for task, invalid parameter");
        pthread_exit(0);
    }

    pstTCB = (SC_TASK_CB_ST *)ptr;
    if (DOS_ADDR_INVALID(pstTCB))
    {
        DOS_ASSERT(0);

        sc_logr_error(NULL, SC_TASK, "%s", "Start task without pointer a TCB.");
        return NULL;
    }

    pstTCB->ucTaskStatus = SC_TASK_WORKING;

    if (sc_check_server_ctrl(pstTCB->ulCustomID
                                , BS_SERV_AUTO_DIALING
                                , SC_SRV_CTRL_ATTR_TASK_MODE
                                , pstTCB->ucMode
                                , SC_SRV_CTRL_ATTR_INVLID
                                , U32_BUTT))
    {
        DOS_ASSERT(0);
        sc_logr_notice(NULL, SC_TASK, "Service not allow.(TaskID:%u) ", pstTCB->ulTaskID);

        goto finished;
    }

    /* ����һ����ʱ�������Ѿ����й��ĺ�����������ʱд�����ݿ��� */
    lResult = dos_tmr_start(&pstTCB->pstTmrHandle
                            , SC_TASK_UPDATE_DB_TIMER * 1000
                            , sc_task_update_calledcnt
                            , (U64)pstTCB
                            , TIMER_NORMAL_LOOP);
    if (lResult < 0)
    {
        sc_logr_error(NULL, SC_TASK, "Start timer update task(%u) calledcnt FAIL", pstTCB->ulTaskID);
    }

    /*
       ����ط���ŵ���˼��: �������ʱ��Ϊ5000ms����Ҫ����5000ms�ڲ�������ϯ��Ҫ�ĺ���ȫ����������
       �����Ҫ����һ��������е�ʱ����
       ���Ǻ��м����С��20ms
       ����1000ms���һ��
       ����ǲ���Ҫ��ϯ�ĺ��У�����20CPS�����ȷ���(��ȻҪ������Ͳ�����)
    */
    if (pstTCB->usSiteCount * pstTCB->ulCallRate)
    {
        ulMinInterval = 5000 / pstTCB->usSiteCount * pstTCB->ulCallRate;
        ulMinInterval = (ulMinInterval < 20) ? 20 : (ulMinInterval > 1000) ? 1000 : ulMinInterval;
    }
    else
    {
        ulMinInterval = 1000 / 20;
    }

    sc_logr_info(NULL, SC_TASK, "Start run task(%u), Min interval: %ums", pstTCB->ulTaskID, ulMinInterval);

    while (1)
    {
        if (0 == ulTaskInterval)
        {
            ulTaskInterval = ulMinInterval;
        }
        dos_task_delay(ulTaskInterval);
        ulTaskInterval = 0;

        /* ���ݵ�ǰ��������ȷ��������еļ���������ǰ�����Ѿ���������״̬����Ҫǿ�Ƶ������ */
        if (!sc_task_check_can_call(pstTCB))
        {
            /* ���ܻ�ǳ��죬�Ͳ�Ҫ��ӡ�� */
            /*sc_logr_debug(NULL, SC_TASK, "Cannot make call for reach the max concurrency. Task : %u.", pstTCB->ulTaskID);*/
            continue;
        }

        /* �����ͣ�˾ͼ����ȴ� */
        if (SC_TASK_PAUSED == pstTCB->ucTaskStatus)
        {
            sc_logr_debug(NULL, SC_TASK, "Cannot make call for paused status. Task : %u.", pstTCB->ulTaskID);
            ulTaskInterval = 20000;
            continue;
        }

        /* �����ֹͣ�ˣ��ͼ�⻹��û�к��У�����к��У��͵ȴ����ȴ�û�к���ʱ�˳����� */
        if (SC_TASK_STOP == pstTCB->ucTaskStatus)
        {
            if (pstTCB->ulCurrentConcurrency != 0)
            {
                sc_logr_debug(NULL, SC_TASK, "Cannot make call for stoped status. Task : %u, CurrentConcurrency : %u.", pstTCB->ulTaskID, pstTCB->ulCurrentConcurrency);
                ulTaskInterval = 20000;
                continue;
            }

            /* ��������ˣ��˳���ѭ�� */
            break;
        }

        /* ��鵱ǰ�Ƿ��������ʱ��� */
        if (sc_task_check_can_call_by_time(pstTCB) != DOS_TRUE)
        {
            sc_logr_debug(NULL, SC_TASK, "Cannot make call for invalid time period. Task : %u. %d", pstTCB->ulTaskID, pstTCB->usTCBNo);
            ulTaskInterval = 20000;
            continue;
        }

        /* ��⵱ʱ�����Ƿ���Է������ */
        if (sc_task_check_can_call_by_status(pstTCB) != DOS_TRUE)
        {
            sc_logr_debug(NULL, SC_TASK, "Cannot make call for system busy. Task : %u.", pstTCB->ulTaskID);
            continue;
        }
#if 1
        /* ������� */
        if (sc_task_make_call(pstTCB))
        {
            sc_logr_debug(NULL, SC_TASK, "%s", "Make call fail.");
        }
#endif
    }

finished:
    sc_logr_info(NULL, SC_TASK, "Task %d finished.", pstTCB->ulTaskID);

    /* �ͷ������Դ */
    if (DOS_ADDR_VALID(pstTCB->pstTmrHandle))
    {
        dos_tmr_stop(&pstTCB->pstTmrHandle);
        pstTCB->pstTmrHandle = NULL;
    }

    while (1)
    {
        if (dos_list_is_empty(&pstTCB->stCalleeNumQuery))
        {
            break;
        }

        pstList = dos_list_fetch(&pstTCB->stCalleeNumQuery);
        if (DOS_ADDR_INVALID(pstList))
        {
            break;
        }

        pstCallee = dos_list_entry(pstList, SC_TEL_NUM_QUERY_NODE_ST, stLink);
        if (DOS_ADDR_INVALID(pstCallee))
        {
            continue;
        }

        dos_dmem_free(pstCallee);
        pstCallee = NULL;
    }

#if 0
    if (pstTCB->pstCallerNumQuery)
    {
        dos_dmem_free(pstTCB->pstCallerNumQuery);
        pstTCB->pstCallerNumQuery = NULL;
    }
#endif

    pthread_mutex_destroy(&pstTCB->mutexTaskList);

    /* Ⱥ����������󣬽����еı��к�����������Ϊ���к���������� */
    pstTCB->bThreadRunning = DOS_FALSE;
    pstTCB->ulCalledCount = pstTCB->ulCalleeCountTotal;
    sc_task_update_calledcnt((U64)pstTCB);
    sc_task_save_status(pstTCB->ulTaskID, SC_TASK_STATUS_DB_STOP, NULL);

    sc_tcb_free(pstTCB);
    pstTCB = NULL;

    return NULL;
}

/*
 * ����: U32 sc_task_start(SC_TASK_CB_ST *pstTCB)
 * ����: �������л�����
 * ����:
 *      SC_TASK_CB_ST *pstTCB: ������ƿ�
 * �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 */
U32 sc_task_start(SC_TASK_CB_ST *pstTCB)
{
    SC_TRACE_IN((U64)pstTCB, 0, 0, 0);

    if (!pstTCB
        || !pstTCB->ucValid)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    if (pstTCB->bThreadRunning)
    {
        sc_logr_notice(NULL, SC_TASK, "Task %u already running.", pstTCB->ulTaskID);
    }
    else
    {
        if (pthread_create(&pstTCB->pthID, NULL, sc_task_runtime, pstTCB) < 0)
        {
            DOS_ASSERT(0);

            pstTCB->bThreadRunning = DOS_FALSE;

            sc_logr_notice(NULL, SC_TASK, "Start task %d faild", pstTCB->ulTaskID);

            SC_TRACE_OUT();
            return DOS_FAIL;
        }

        pstTCB->bThreadRunning = DOS_TRUE;
    }

    sc_task_save_status(pstTCB->ulTaskID, SC_TASK_STATUS_DB_START, NULL);

    sc_logr_notice(NULL, SC_TASK, "Start task %d finished.", pstTCB->ulTaskID);

    return DOS_SUCC;
}

/*
 * ����: U32 sc_task_stop(SC_TASK_CB_ST *pstTCB)
 * ����: ֹͣ���л�����
 * ����:
 *      SC_TASK_CB_ST *pstTCB: ������ƿ�
 * �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 */
U32 sc_task_stop(SC_TASK_CB_ST *pstTCB)
{
    SC_TRACE_IN((U64)pstTCB, 0, 0, 0);
    if (!pstTCB)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    if (!pstTCB->ucValid
        /*|| pstTCB->ucTaskStatus != SC_TASK_WORKING*/)
    {
        DOS_ASSERT(0);

        sc_logr_info(NULL, SC_TASK, "Cannot stop the task. TCB Valid:%d, TCB Status: %d", pstTCB->ucValid, pstTCB->ucTaskStatus);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    sc_task_save_status(pstTCB->ulTaskID, SC_TASK_STATUS_DB_STOP, NULL);

    pthread_mutex_lock(&pstTCB->mutexTaskList);
    pstTCB->ucTaskStatus = SC_TASK_STOP;
    pthread_mutex_unlock(&pstTCB->mutexTaskList);

    return DOS_SUCC;
}

/*
 * ����: U32 sc_task_continue(SC_TASK_CB_ST *pstTCB)
 * ����: �ָ����л�����
 * ����:
 *      SC_TASK_CB_ST *pstTCB: ������ƿ�
 * �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 */
U32 sc_task_continue(SC_TASK_CB_ST *pstTCB)
{
    SC_TRACE_IN((U64)pstTCB, 0, 0, 0);
    if (!pstTCB)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    if (!pstTCB->ucValid
        || pstTCB->ucTaskStatus != SC_TASK_PAUSED)
    {
        DOS_ASSERT(0);

        sc_logr_info(NULL, SC_TASK, "Cannot continue the task. TCB Valid:%d, TCB Status: %d", pstTCB->ucValid, pstTCB->ucTaskStatus);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pthread_mutex_lock(&pstTCB->mutexTaskList);
    pstTCB->ucTaskStatus = SC_TASK_WORKING;
    pthread_mutex_unlock(&pstTCB->mutexTaskList);

    /* ��ʼ���� */
    return sc_task_start(pstTCB);
}

/*
 * ����: U32 sc_task_pause(SC_TASK_CB_ST *pstTCB)
 * ����: ��ͣ���л�����
 * ����:
 *      SC_TASK_CB_ST *pstTCB: ������ƿ�
 * �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 */
U32 sc_task_pause(SC_TASK_CB_ST *pstTCB)
{
    SC_TRACE_IN((U64)pstTCB, 0, 0, 0);
    if (!pstTCB)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    if (!pstTCB->ucValid
        || pstTCB->ucTaskStatus != SC_TASK_WORKING)
    {
        DOS_ASSERT(0);

        sc_logr_info(NULL, SC_TASK, "Cannot stop the task. TCB Valid:%d, TCB Status: %d", pstTCB->ucValid, pstTCB->ucTaskStatus);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    sc_task_save_status(pstTCB->ulTaskID, SC_TASK_STATUS_DB_PAUSED, NULL);

    pthread_mutex_lock(&pstTCB->mutexTaskList);
    pstTCB->ucTaskStatus = SC_TASK_PAUSED;
    pthread_mutex_unlock(&pstTCB->mutexTaskList);

    return DOS_SUCC;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */


