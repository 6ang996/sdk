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
        sc_logr_info(SC_TASK, "Load callee number for task %d. Load result : %d", pstTCB->ulTaskID, ulCount);
    }

    if (dos_list_is_empty(&pstTCB->stCalleeNumQuery))
    {
        pstTCB->ucTaskStatus = SC_TASK_STOP;

        sc_logr_info(SC_TASK, "Task %d has finished. or there is no callees.", pstTCB->ulTaskID);

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

    sc_logr_info(SC_TASK, "Select callee %s for new call.", pstCallee->szNumber);

    return pstCallee;
}

/*
 * ����: SC_CALLER_QUERY_NODE_ST *sc_task_get_caller(SC_TASK_CB_ST *pstTCB)
 * ����: ��ȡ���к��룬�����к����б������漴ѡ��һ��
 * ����:
 *      SC_TASK_CB_ST *pstTCB: ������ƿ�
 * ����ֵ: �ɹ��������к�����ƿ�ָ�룬���򷵻�NULL
 */
SC_CALLER_QUERY_NODE_ST *sc_task_get_caller(SC_TASK_CB_ST *pstTCB)
{
    U32                      ulCallerIndex = 0;
    S32                      lMaxSelectTime  = 0;
    SC_CALLER_QUERY_NODE_ST  *pstCaller = NULL;

    SC_TRACE_IN((U64)pstTCB, 0, 0, 0);
    if (DOS_ADDR_INVALID(pstTCB))
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return NULL;
    }

    while (1)
    {
        lMaxSelectTime++;
        if (lMaxSelectTime > pstTCB->usCallerCount)
        {
            DOS_ASSERT(0);
            SC_TRACE_OUT();
            break;
        }

        ulCallerIndex = sc_random((U32)pstTCB->usCallerCount);
        if (ulCallerIndex >= SC_MAX_CALLER_NUM)
        {
            DOS_ASSERT(0);
            SC_TRACE_OUT();
            break;
        }

        if (!pstTCB->pstCallerNumQuery[ulCallerIndex].bValid)
        {
            continue;
        }

        pstCaller = &pstTCB->pstCallerNumQuery[ulCallerIndex];
        break;
    }

    if (pstCaller)
    {
        sc_logr_info(SC_TASK, "Select caller %s for new call", pstCaller->szNumber);
    }
    else
    {
        sc_logr_info(SC_TASK, "%s", "There is no caller for new call");
    }

    SC_TRACE_OUT();
    return pstCaller;
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
    SC_CALLER_QUERY_NODE_ST   *pstCaller = NULL;

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

    pstCaller = sc_task_get_caller(pstTCB);
    if (!pstCaller)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    pstSCB = sc_scb_alloc();
    if (!pstSCB)
    {
        sc_logr_notice(SC_TASK, "Make call for task %u fail. Alloc SCB fail.", pstTCB->ulTaskID);
        goto fail;
    }

    SC_SCB_SET_STATUS(pstSCB, SC_SCB_INIT);

    if (pstTCB->bTraceCallON || pstSCB->bTraceNo
        || pstCallee->ucTraceON || pstCaller->bTraceON)
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

    dos_strncpy(pstSCB->szCallerNum, pstCaller->szNumber, SC_TEL_NUMBER_LENGTH);
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
        sc_logr_info(SC_ESL, "Cannot make call for number %s which is in black list.", pstSCB->szCalleeNum);
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
    SC_TEL_NUM_QUERY_NODE_ST *pstCallee;
    SC_TASK_CB_ST   *pstTCB;
    list_t          *pstList;
    U32             ulTaskInterval = 0;

    if (!ptr)
    {
        sc_logr_error(SC_TASK, "%s", "Fail to start the thread for task, invalid parameter");
        pthread_exit(0);
    }

    pstTCB = (SC_TASK_CB_ST *)ptr;
    if (DOS_ADDR_INVALID(pstTCB))
    {
        DOS_ASSERT(0);

        sc_logr_error(SC_TASK, "%s", "Start task without pointer a TCB.");
        return NULL;
    }

    pstTCB->ucTaskStatus = SC_TASK_WORKING;
    while (1)
    {
        if (0 == ulTaskInterval)
        {
            ulTaskInterval = sc_task_get_call_interval(pstTCB);
        }
        usleep(ulTaskInterval * 1000);
        ulTaskInterval = 0;

        /* ���ݵ�ǰ��������ȷ��������еļ���������ǰ�����Ѿ���������״̬����Ҫǿ�Ƶ������ */
        if (pstTCB->ulCurrentConcurrency >= pstTCB->ulMaxConcurrency)
        {
            sc_logr_info(SC_TASK, "Cannot make call for reach the max concurrency. Task : %u.", pstTCB->ulTaskID);
            continue;
        }

        /* �����ͣ�˾ͼ����ȴ� */
        if (SC_TASK_PAUSED == pstTCB->ucTaskStatus)
        {
            sc_logr_info(SC_TASK, "Cannot make call for puased status. Task : %u.", pstTCB->ulTaskID);
            ulTaskInterval = 1000;
            continue;
        }

        /* �����ֹͣ�ˣ��ͼ�⻹��û�к��У�����к��У��͵ȴ����ȴ�û�к���ʱ�˳����� */
        if (SC_TASK_STOP == pstTCB->ucTaskStatus)
        {
            if (pstTCB->ulCurrentConcurrency != 0)
            {
                sc_logr_info(SC_TASK, "Cannot make call for stop status. Task : %u.", pstTCB->ulTaskID);
                ulTaskInterval = 1000;
                continue;
            }

            /* ��������ˣ��˳���ѭ�� */
            break;
        }

        /* ��鵱ǰ�Ƿ��������ʱ��� */
        if (sc_task_check_can_call_by_time(pstTCB) != DOS_TRUE)
        {
            sc_logr_info(SC_TASK, "Cannot make call for invalid time period. Task : %u.", pstTCB->ulTaskID);
            ulTaskInterval = 1000;
            continue;
        }

        /* ��⵱ʱ�����Ƿ���Է������ */
        if (sc_task_check_can_call_by_status(pstTCB) != DOS_TRUE)
        {
            sc_logr_info(SC_TASK, "Cannot make call for system busy. Task : %u.", pstTCB->ulTaskID);
            continue;
        }

        /* ������� */
        if (sc_task_make_call(pstTCB))
        {
            sc_logr_info(SC_TASK, "%s", "Make call fail.");
        }
    }

    sc_update_task_status(pstTCB->ulTaskID, pstTCB->ucTaskStatus);

    sc_logr_info(SC_TASK, "Task %d finished.", pstTCB->ulTaskID);

    /* �ͷ������Դ */
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

    if (pstTCB->pstCallerNumQuery)
    {
        dos_dmem_free(pstTCB->pstCallerNumQuery);
        pstTCB->pstCallerNumQuery = NULL;
    }
    pthread_mutex_destroy(&pstTCB->mutexTaskList);

    sc_task_save_status(pstTCB->ulTaskID, SC_TASK_STATUS_DB_STOP, NULL);

    sc_tcb_free(pstTCB);
    pstTCB = NULL;

    return NULL;
}

/*
 * ����: U32 sc_task_init(SC_TASK_CB_ST *pstTCB)
 * ����: ��ʼ����������
 * ����:
 *      SC_TASK_CB_ST *pstTCB: ������ƿ�
 * �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 */
U32 sc_task_init(SC_TASK_CB_ST *pstTCB)
{
    U32       ulIndex;
    U32       lRet;
    SC_TRACE_IN((U64)pstTCB, 0, 0, 0);

    if (!pstTCB)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    /* ��������Դ */
    pstTCB->pstCallerNumQuery = (SC_CALLER_QUERY_NODE_ST *)dos_dmem_alloc(sizeof(SC_CALLER_QUERY_NODE_ST) * SC_MAX_CALLER_NUM);
    if (!pstTCB->pstCallerNumQuery )
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    dos_memzero(pstTCB->pstCallerNumQuery, sizeof(SC_CALLER_QUERY_NODE_ST) * SC_MAX_CALLER_NUM);
    for (ulIndex=0; ulIndex<SC_MAX_CALLER_NUM; ulIndex++)
    {
        pstTCB->pstCallerNumQuery[ulIndex].usNo = ulIndex;
        pstTCB->pstCallerNumQuery[ulIndex].bValid = 0;
        pstTCB->pstCallerNumQuery[ulIndex].bTraceON = 0;
        pstTCB->pstCallerNumQuery[ulIndex].szNumber[0] = '\0';
        pstTCB->pstCallerNumQuery[ulIndex].ulIndexInDB = U32_BUTT;
    }

    pstTCB->ulLastCalleeIndex = 0;

    lRet = sc_task_load_callee(pstTCB);
    if (lRet != DOS_SUCC)
    {
        DOS_ASSERT(0);
        sc_logr_error(SC_TASK, "Load callee for task %d failed, Or there in no callee number.", pstTCB->ulTaskID);

        goto init_fail;
    }
    sc_logr_info(SC_TASK, "Task %d has been loaded %d callee(s).", pstTCB->ulTaskID, pstTCB->ulCalleeCount);

    lRet = sc_task_load_caller(pstTCB);
    if (lRet != DOS_SUCC)
    {
        DOS_ASSERT(0);
        sc_logr_error(SC_TASK, "Load caller for task %d failed, Or there in no caller number.", pstTCB->ulTaskID);

        goto init_fail;
    }
    sc_logr_info(SC_TASK, "Task %d has been loaded %d caller(s).", pstTCB->ulTaskID, pstTCB->usCallerCount);

    if (sc_task_load_period(pstTCB) != DOS_SUCC)
    {
        DOS_ASSERT(0);
        sc_logr_error(SC_TASK, "Load period for task %d failed.", pstTCB->ulTaskID);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    if (sc_task_load_audio(pstTCB) != DOS_SUCC)
    {
        DOS_ASSERT(0);
        sc_logr_error(SC_TASK, "Load audio file for task %d FAILED.", pstTCB->ulTaskID);

        goto init_fail;
    }

    if (sc_task_load_agent_info(pstTCB) != DOS_SUCC)
    {
        DOS_ASSERT(0);
        sc_logr_error(SC_TASK, "Load agent info for task %d FAILED.", pstTCB->ulTaskID);

        goto init_fail;
    }

    if (sc_task_load_other_info(pstTCB) != DOS_SUCC)
    {
        DOS_ASSERT(0);
        sc_logr_error(SC_TASK, "Load agent info for task %d FAILED.", pstTCB->ulTaskID);

        goto init_fail;
    }

    if (SC_TASK_MODE_AUDIO_ONLY == pstTCB->ucMode)
    {
        pstTCB->ulMaxConcurrency = g_ulMaxConcurrency4Task;
    }
    else
    {
        pstTCB->ulMaxConcurrency = pstTCB->usSiteCount * SC_MAX_CALL_MULTIPLE;
    }



    sc_logr_notice(SC_TASK, "Load data for task %d finished.", pstTCB->ulTaskID);

    SC_TRACE_OUT();
    return DOS_SUCC;

 init_fail:
    if (pstTCB->pstCallerNumQuery)
    {
        dos_dmem_free(pstTCB->pstCallerNumQuery);
        pstTCB->pstCallerNumQuery = NULL;
    }

    SC_TRACE_OUT();
    return DOS_FAIL;
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

    if (pthread_create(&pstTCB->pthID, NULL, sc_task_runtime, pstTCB) < 0)
    {
        DOS_ASSERT(0);

        sc_logr_notice(SC_TASK, "Start task %d faild", pstTCB->ulTaskID);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    sc_task_save_status(pstTCB->ulTaskID, SC_TASK_STATUS_DB_START, NULL);

    sc_logr_notice(SC_TASK, "Start task %d finished.", pstTCB->ulTaskID);

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
        || pstTCB->ucTaskStatus != SC_TASK_WORKING)
    {
        DOS_ASSERT(0);

        sc_logr_info(SC_TASK, "Cannot stop the task. TCB Valid:%d, TCB Status: %d", pstTCB->ucValid, pstTCB->ucTaskStatus);

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

        sc_logr_info(SC_TASK, "Cannot stop the task. TCB Valid:%d, TCB Status: %d", pstTCB->ucValid, pstTCB->ucTaskStatus);

        SC_TRACE_OUT();
        return DOS_FAIL;
    }

    sc_task_save_status(pstTCB->ulTaskID, SC_TASK_STATUS_DB_CONTINUE, NULL);

    pthread_mutex_lock(&pstTCB->mutexTaskList);
    pstTCB->ucTaskStatus = SC_TASK_WORKING;
    pthread_mutex_unlock(&pstTCB->mutexTaskList);

    return DOS_SUCC;
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

        sc_logr_info(SC_TASK, "Cannot stop the task. TCB Valid:%d, TCB Status: %d", pstTCB->ucValid, pstTCB->ucTaskStatus);

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


