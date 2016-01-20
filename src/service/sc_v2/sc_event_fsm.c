/**
 * @file : sc_event_fsm.c
 *
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 * ҵ�����ģ�����ҵ��״̬��ʵ��
 *
 * @date: 2016��1��13��
 * @arthur: Larry
 */
#ifdef __cplusplus
extern "C" {
#endif /* End of __cplusplus */

#include <dos.h>
#include "sc_def.h"
#include "sc_pub.h"
#include "bs_pub.h"
#include "sc_res.h"
#include "sc_hint.h"
#include "sc_debug.h"
#include "sc_pub.h"


U32 sc_call_access_code(SC_SRV_CB *pstSCB, S8 *pszAccessCode)
{
    return DOS_SUCC;
}


/**
 * ��������ҵ���к��н�����Ϣ
 *
 * @param SC_MSG_TAG_ST *pstMsg
 * @param SC_SRV_CB *pstSCB
 *
 * @return �ɹ�����DOS_SUCC,ʧ�ܷ���DOS_FAIL
 */
U32 sc_call_setup(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB   *pstCallingLegCB = NULL;
    SC_MSG_EVT_CALL_ST  *pstCallSetup;
    U32         ulCallSrc        = U32_BUTT;
    U32         ulCallDst        = U32_BUTT;
    U32         ulRet            = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstCallSetup = (SC_MSG_EVT_CALL_ST*)pstMsg;

    sc_trace_scb(pstSCB, "Processing the call setup msg. status: %u", pstSCB->stCall.stSCBTag.usStatus);

    switch (pstSCB->stCall.stSCBTag.usStatus)
    {
        case SC_CALL_IDEL:
            break;

        case SC_CALL_PORC:
            /*  ����ط����Ǻ��� */
            pstCallingLegCB = sc_lcb_get(pstSCB->stCall.ulCallingLegNo);
            if (DOS_ADDR_INVALID(pstCallingLegCB))
            {
                sc_trace_scb(pstSCB, "Cannot find the LCB. (%s)", pstSCB->stCall.ulCallingLegNo);

                goto proc_fail;
            }

            /* ����ǽ�����ҵ����Ҫ���������ҵ�� */
            if ('*' == pstCallingLegCB->stCall.stNumInfo.szOriginalCallee[0])
            {
                ulRet = sc_call_access_code(pstSCB, pstCallingLegCB->stCall.stNumInfo.szOriginalCallee);
                goto proc_finished;
            }

            ulCallSrc = sc_leg_get_source(pstSCB, pstCallingLegCB);
            /* һ��Ҫͨ�����аѿͻ���Ϣ�����������������ڽ�����  */
            if (U32_BUTT == pstSCB->ulCustomerID)
            {
                ulRet = sc_req_hungup_with_sound(U32_BUTT, pstSCB->stCall.ulCallingLegNo, CC_ERR_SC_NO_SERV_RIGHTS);
                goto proc_finished;
            }

            ulCallDst = sc_leg_get_destination(pstSCB, pstCallingLegCB);

            sc_trace_scb(pstSCB, "Get call source and dest. Customer: %u Source: %d, Dest: %d", pstSCB->ulCustomerID, ulCallSrc, ulCallDst);

            /* ���ֺ��� */
            if (SC_DIRECTION_SIP == ulCallSrc && SC_DIRECTION_PSTN == ulCallDst)
            {
                pstSCB->stCall.stSCBTag.usStatus = SC_CALL_AUTH;

                if (DOS_ADDR_VALID(pstSCB->stCall.pstAgentCalling))
                {
                    sc_acd_agent_stat(SC_AGENT_STAT_CALL, pstSCB->stCall.pstAgentCalling->pstAgentInfo, 0, 0);
                }

                sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL);

                ulRet = sc_send_usr_auth2bs(pstSCB, pstCallingLegCB);
                goto proc_finished;
            }
            /* ���� */
            else if (SC_DIRECTION_PSTN == ulCallSrc && SC_DIRECTION_SIP == ulCallDst)
            {
                pstSCB->stCall.stSCBTag.usStatus = SC_CALL_AUTH;

                if (DOS_ADDR_VALID(pstSCB->stCall.pstAgentCalling))
                {
                    sc_acd_agent_stat(SC_AGENT_STAT_CALL, pstSCB->stCall.pstAgentCalling->pstAgentInfo, 0, 0);
                }

                sc_scb_set_service(pstSCB, BS_SERV_INBAND_CALL);

                ulRet = sc_send_usr_auth2bs(pstSCB, pstCallingLegCB);
                goto proc_finished;
            }
            /* �ڲ����� */
            else if (SC_DIRECTION_SIP == ulCallSrc && SC_DIRECTION_SIP == ulCallDst)
            {
                pstSCB->stCall.stSCBTag.usStatus = SC_CALL_EXEC;

                sc_scb_set_service(pstSCB, BS_SERV_INTER_CALL);

                ulRet = sc_internal_call_process(pstSCB, pstCallingLegCB);
                goto proc_finished;
            }
            else
            {
                pstSCB->stCall.stSCBTag.usStatus = SC_CALL_RELEASE;

                ulRet = sc_req_hungup_with_sound(U32_BUTT, pstSCB->stCall.ulCallingLegNo, CC_ERR_SC_NO_SERV_RIGHTS);
                goto proc_finished;
            }
            break;

        case SC_CALL_AUTH:
            break;
        case SC_CALL_EXEC:
            /* ����ط�Ӧ���Ǳ��е�LEG������, ����һ�±��е�LEG */
            pstSCB->stCall.ulCalleeLegNo = pstCallSetup->ulLegNo;
            break;

        case SC_CALL_ALERTING:
            break;

        case SC_CALL_ACTIVE:
        case SC_CALL_PROCESS:
        case SC_CALL_RELEASE:
            ulRet = DOS_SUCC;
            break;

        default:
            ulRet = DOS_FAIL;
            sc_trace_scb(pstSCB, "Invalid status.%u", pstSCB->stCall.stSCBTag.usStatus);
            break;
    }

proc_finished:
    return DOS_SUCC;

proc_fail:
    sc_scb_free(pstSCB);
    pstSCB = NULL;
    return DOS_FAIL;
}

U32 sc_call_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_AUTH_RESULT_ST  *pstAuthRsp;
    SC_LEG_CB                  *pstLegCB = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstAuthRsp = (SC_MSG_EVT_AUTH_RESULT_ST *)pstMsg;
    pstLegCB = sc_lcb_get(pstSCB->stCall.ulCallingLegNo);
    if (DOS_ADDR_INVALID(pstLegCB))
    {
        sc_scb_free(pstSCB);

        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    if (pstAuthRsp->stMsgTag.usInterErr != BS_ERR_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Release call with error code %u", pstAuthRsp->stMsgTag.usInterErr);
        /* ע��ͨ��ƫ�������ҵ�CCͳһ����Ĵ����� */
        sc_req_hungup_with_sound(pstSCB->ulSCBNo, pstSCB->stCall.ulCallingLegNo, CC_ERR_BS_HEAD + pstAuthRsp->stMsgTag.usInterErr);
        return DOS_SUCC;
    }

    pstSCB->stCall.stSCBTag.usStatus = SC_CALL_EXEC;

    if (pstAuthRsp->ucBalanceWarning)
    {
        return sc_req_play_sound(pstSCB->ulSCBNo, pstSCB->stCall.ulCallingLegNo, SC_SND_LOW_BALANCE, 1, 0, 0);
    }

    return sc_outgoing_call_process(pstSCB, pstLegCB);
}


U32 sc_call_ringing(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_RINGING_ST *pstEvent = NULL;
    SC_LEG_CB             *pstCalleeLegCB = NULL;
    SC_LEG_CB             *pstCallingLegCB = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstEvent = (SC_MSG_EVT_RINGING_ST *)pstMsg;

    sc_trace_scb(pstSCB, "process alerting msg. calling leg: %u, callee leg: %u"
                        , pstSCB->stCall.ulCallingLegNo, pstSCB->stCall.ulCalleeLegNo);

    pstCalleeLegCB = sc_lcb_get(pstSCB->stCall.ulCalleeLegNo);
    pstCallingLegCB = sc_lcb_get(pstSCB->stCall.ulCallingLegNo);
    if (DOS_ADDR_INVALID(pstCalleeLegCB) || DOS_ADDR_INVALID(pstCallingLegCB))
    {
        sc_trace_scb(pstSCB, "alerting with only one leg.");
        return DOS_SUCC;
    }

    switch (pstSCB->stCall.stSCBTag.usStatus)
    {
        case SC_CALL_IDEL:
        case SC_CALL_PORC:
        case SC_CALL_AUTH:
        case SC_CALL_EXEC:
            pstSCB->stCall.stSCBTag.usStatus = SC_CALL_ALERTING;

            if (pstEvent->ulLegNo == pstSCB->stCall.ulCalleeLegNo)
            {
                if (pstEvent->ulWithMedia)
                {
                    pstCalleeLegCB = sc_lcb_get(pstSCB->stCall.ulCalleeLegNo);
                    if (DOS_ADDR_VALID(pstCalleeLegCB))
                    {
                        pstCalleeLegCB->stCall.bEarlyMedia = DOS_TRUE;

                        if (sc_req_bridge_call(pstSCB->ulSCBNo, pstSCB->stCall.ulCalleeLegNo, pstSCB->stCall.ulCallingLegNo) != DOS_SUCC)
                        {
                            sc_trace_scb(pstSCB, "Bridge call when early media fail.");
                            goto proc_fail;
                        }
                    }
                }
                else
                {
                    if (sc_req_ringback(pstSCB->ulSCBNo
                                            , pstSCB->stCall.ulCallingLegNo
                                            , pstCallingLegCB->stCall.bEarlyMedia) != DOS_SUCC)
                    {
                        sc_trace_scb(pstSCB, "Send ringback request fail.");
                    }
                }
            }

            break;

        case SC_CALL_ALERTING:
            /* ����LEG״̬�任 */
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_EVENT), "Calling has been ringback.");
            break;

        case SC_CALL_ACTIVE:
        case SC_CALL_PROCESS:
        case SC_CALL_RELEASE:
            break;
    }

    return DOS_SUCC;

proc_fail:
    /* ����ط����ùҶϺ��У�����ý������飬��Ӱ��� */
    return DOS_FAIL;
}

U32 sc_call_answer(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB             *pstCalleeLegCB = NULL;
    SC_MSG_EVT_ANSWER_ST  *pstEvtAnswer   = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (pstSCB->stCall.stSCBTag.usStatus)
    {
        case SC_CALL_IDEL:
        case SC_CALL_PORC:
        case SC_CALL_AUTH:
        case SC_CALL_EXEC:
        case SC_CALL_ALERTING:
            pstEvtAnswer = (SC_MSG_EVT_ANSWER_ST *)pstMsg;

            pstSCB->stCall.ulCalleeLegNo = pstEvtAnswer->ulLegNo;

            /* Ӧ������ */
            sc_req_answer_call(pstSCB->ulSCBNo, pstSCB->stCall.ulCallingLegNo);

            /* ���������� */
            pstCalleeLegCB = sc_lcb_get(pstEvtAnswer->ulLegNo);
            if (DOS_ADDR_VALID(pstCalleeLegCB)
                && !pstCalleeLegCB->stCall.bEarlyMedia)
            {
                if (sc_req_bridge_call(pstSCB->ulSCBNo, pstSCB->stCall.ulCalleeLegNo, pstSCB->stCall.ulCallingLegNo) != DOS_SUCC)
                {
                    sc_trace_scb(pstSCB, "Bridge call when early media fail.");
                    goto proc_fail;
                }
            }

            pstSCB->stCall.stSCBTag.usStatus = SC_CALL_ACTIVE;
            break;

        case SC_CALL_ACTIVE:
        case SC_CALL_PROCESS:
        case SC_CALL_RELEASE:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_EVENT), "Calling has been answered");
            break;
    }

    return DOS_SUCC;
proc_fail:
    /* �Ҷ����� */

    return DOS_FAIL;
}

U32 sc_call_bridge(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    /* DO nothing */
    return DOS_SUCC;
}

U32 sc_call_unbridge(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    /* DO nothing */
    return DOS_SUCC;
}

U32 sc_call_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_HUNGUP_ST *pstHungup = NULL;
    SC_LEG_CB            *pstCallee = NULL;
    SC_LEG_CB            *pstCalling = NULL;

    pstHungup = (SC_MSG_EVT_HUNGUP_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstHungup) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Leg %u has hungup. Legs:%u-%u", pstHungup->ulLegNo, pstSCB->stCall.ulCalleeLegNo, pstSCB->stCall.ulCallingLegNo);

    switch (pstSCB->stCall.stSCBTag.usStatus)
    {
        case SC_CALL_IDEL:
            break;

        case SC_CALL_PORC:
            break;

        case SC_CALL_AUTH:
            break;

        case SC_CALL_EXEC:
            /* ����ط�Ӧ���Ǳ����쳣�� */

            pstCalling = sc_lcb_get(pstSCB->stCall.ulCallingLegNo);
            if (DOS_ADDR_VALID(pstCalling))
            {
                if (sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stCall.ulCallingLegNo, CC_ERR_NORMAL_CLEAR))
                {
                    sc_lcb_free(pstCalling);
                    pstCalling = NULL;

                    sc_scb_free(pstSCB);
                    pstSCB = NULL;
                }
            }

            pstSCB->stCall.stSCBTag.usStatus = SC_CALL_PROCESS;
            break;

        case SC_CALL_ALERTING:
        case SC_CALL_ACTIVE:
            pstCallee = sc_lcb_get(pstSCB->stCall.ulCalleeLegNo);
            pstCalling = sc_lcb_get(pstSCB->stCall.ulCallingLegNo);
            if (DOS_ADDR_INVALID(pstCallee) || DOS_ADDR_INVALID(pstCalling))
            {
                /* ���ͻ������ͷ�ҵ����ƿ� */

                if (pstCallee)
                {
                    sc_lcb_free(pstCallee);
                }

                if (pstCalling)
                {
                    sc_lcb_free(pstCalling);
                }

                sc_scb_free(pstSCB);
                return DOS_SUCC;
            }

            /* �����˵������leg��OK */
            /*
              * ��Ҫ�����Ƿ�ǩ�����⣬�����/����LEG����ǩ�ˣ���Ҫ����SCB��������LEG�ҵ��µ�SCB��
              * ���򣬽���Ҫ��ǩ��LEG��Ϊ��ǰҵ����ƿ������LEG���Ҷ�����һ��LEG
              * ������Ҫ����ͻ����
              */
            if (pstSCB->stCall.ulCalleeLegNo == pstHungup->ulLegNo)
            {
                sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stCall.ulCallingLegNo, CC_ERR_NORMAL_CLEAR);
            }
            else
            {
                sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stCall.ulCalleeLegNo, CC_ERR_NORMAL_CLEAR);
            }

            pstSCB->stCall.stSCBTag.usStatus = SC_CALL_PROCESS;
            break;

        case SC_CALL_PROCESS:
            /* ����LEG���Ҷ��� */
            sc_scb_free(pstSCB);
            pstSCB = NULL;
            break;

        case SC_CALL_RELEASE:
            break;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_DEBUG, SC_MOD_EVENT), "Leg %u has hunguped. ", pstHungup->ulLegNo);

    return DOS_SUCC;
}

U32 sc_call_hold(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_HOLD_ST  *pstHold = NULL;

    pstHold = (SC_MSG_EVT_HOLD_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstHold) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (pstHold->blIsHold)
    {
        /* ����Ǳ�HOLD�ģ���Ҫ����HOLDҵ��Ŷ */
        pstSCB->stHold.stSCBTag.bValid = DOS_TRUE;
        pstSCB->stHold.stSCBTag.bWaitingExit = DOS_FALSE;
        pstSCB->stHold.stSCBTag.usStatus = SC_HOLD_ACTIVE;
        pstSCB->stHold.ulCallLegNo = pstHold->ulLegNo;

        pstSCB->ulCurrentSrv++;
        pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stHold.stSCBTag;

        /* ��HOLD�� ���Ų����� */
        /* ��HOLD�Է� ���ź��б����� */
    }
    else
    {
        /* ����Ǳ�UNHOLD�ģ��Ѿ�û��HOLDҵ���ˣ�����������оͺ� */
        if (sc_req_bridge_call(pstSCB->ulSCBNo, pstSCB->stCall.ulCalleeLegNo, pstSCB->stCall.ulCallingLegNo) != DOS_SUCC)
        {
            sc_trace_scb(pstSCB, "Bridge call when early media fail.");

            sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stCall.ulCalleeLegNo, CC_ERR_SC_SERV_NOT_EXIST);
            sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stCall.ulCallingLegNo, CC_ERR_SC_SERV_NOT_EXIST);
            return DOS_FAIL;
        }
    }

    return DOS_SUCC;
}

U32 sc_call_dtmf(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_DTMF_ST    *pstDTMF = NULL;
    SC_LEG_CB             *pstLCB =  NULL;

    pstDTMF = (SC_MSG_EVT_DTMF_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstDTMF) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstLCB = sc_lcb_get(pstDTMF->ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* �������й��̣�DTMFֻ���ǿͻ���ǣ�����û��ҵ����, ����ֻ�е�ǰLEGΪ��ϯʱ����Ҫ */
    if (pstDTMF->ulLegNo == pstSCB->stCall.ulCalleeLegNo)
    {
        if (DOS_ADDR_INVALID(pstSCB->stCall.pstAgentCallee))
        {
            return DOS_SUCC;
        }
    }

    if (pstDTMF->ulLegNo == pstSCB->stCall.ulCalleeLegNo)
    {
        if (DOS_ADDR_INVALID(pstSCB->stCall.pstAgentCallee))
        {
            return DOS_SUCC;
        }
    }

    /* ����ͻ���� */
    if (dos_strnlen(pstLCB->stCall.stNumInfo.szDial, SC_NUM_LENGTH) == 3)
    {
        /* ��һλ��*, �ڶ�λ��һ������, ����λΪ*����# */
        if ('*' == pstLCB->stCall.stNumInfo.szDial[0]
            && (pstLCB->stCall.stNumInfo.szDial[1] >= '0'  && pstLCB->stCall.stNumInfo.szDial[1] < '9')
            && ('*' == pstLCB->stCall.stNumInfo.szDial[2]  || '#' == pstLCB->stCall.stNumInfo.szDial[2]))
        {
            /* ��¼�ͻ���� */
            sc_trace_scb(pstSCB, "Mark custom. %d", pstLCB->stCall.stNumInfo.szDial[1]);
            return DOS_SUCC;
        }
    }

    /* ��ϯǿ�ƹҶκ��� */
    if (dos_strnicmp(pstLCB->stCall.stNumInfo.szDial, "##", SC_NUM_LENGTH) == 0
        || dos_strnicmp(pstLCB->stCall.stNumInfo.szDial, "**", SC_NUM_LENGTH) == 0)
    {
        /* �Ҷ϶Զ� */
        if (pstLCB->ulCBNo == pstSCB->stCall.ulCalleeLegNo)
        {
            return sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stCall.ulCallingLegNo, CC_ERR_NORMAL_CLEAR);
        }
        else
        {
            return sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stCall.ulCalleeLegNo, CC_ERR_NORMAL_CLEAR);
        }
    }

    return DOS_SUCC;
}

U32 sc_call_record_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    return DOS_SUCC;
}

U32 sc_call_playback_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB              *pstCallingLegCB = NULL;
    SC_MSG_EVT_PLAYBACK_ST *pstCallSetup    = NULL;
    U32                    ulRet            = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstCallingLegCB = sc_lcb_get(pstSCB->stCall.ulCallingLegNo);
    if (DOS_ADDR_INVALID(pstCallingLegCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstCallSetup = (SC_MSG_EVT_PLAYBACK_ST*)pstMsg;

    sc_trace_scb(pstSCB, "Processing the playback stop msg. status: %u", pstSCB->stCall.stSCBTag.usStatus);

    switch (pstSCB->stCall.stSCBTag.usStatus)
    {
        case SC_CALL_IDEL:
            break;

        case SC_CALL_PORC:
            break;

        case SC_CALL_AUTH:
            break;

        case SC_CALL_EXEC:
            ulRet = sc_outgoing_call_process(pstSCB, pstCallingLegCB);
            break;

        case SC_CALL_ALERTING:
            break;

        case SC_CALL_ACTIVE:
            break;
        case SC_CALL_PROCESS:
            break;

        case SC_CALL_RELEASE:
            break;

        default:
            ulRet = DOS_FAIL;
            sc_trace_scb(pstSCB, "Invalid status.%u", pstSCB->stCall.stSCBTag.usStatus);
            break;
    }

    return DOS_SUCC;
}

U32 sc_preview_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32  ulRet = DOS_FAIL;
    SC_LEG_CB *pstLCB = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing Preview auth event.");

    pstLCB = sc_lcb_get(pstSCB->stPreviewCall.ulCallingLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (pstSCB->stPreviewCall.stSCBTag.usStatus)
    {
        case SC_PREVIEW_CALL_IDEL:
            ulRet = DOS_SUCC;
            break;

        case SC_PREVIEW_CALL_AUTH:
            switch (pstLCB->stCall.ucPeerType)
            {
                case SC_LEG_PEER_OUTBOUND:
                    ulRet = sc_make_call2pstn(pstSCB, pstLCB);
                    break;

                case SC_LEG_PEER_OUTBOUND_TT:
                    ulRet = sc_make_call2eix(pstSCB, pstLCB);
                    break;

                case SC_LEG_PEER_OUTBOUND_INTERNAL:
                    ulRet = sc_make_call2sip(pstSCB, pstLCB);
                    break;

                default:
                    sc_trace_scb(pstSCB, "Invalid perr type. %u", pstLCB->stCall.ucPeerType);
                    goto process_fail;
                    break;
            }

            pstSCB->stPreviewCall.stSCBTag.usStatus = SC_PREVIEW_CALL_EXEC;
            break;

        case SC_PREVIEW_CALL_EXEC:
        case SC_PREVIEW_CALL_PORC:
        case SC_PREVIEW_CALL_ALERTING:
        case SC_PREVIEW_CALL_ACTIVE:
        case SC_PREVIEW_CALL_CONNECTING:
        case SC_PREVIEW_CALL_ALERTING2:
        case SC_PREVIEW_CALL_CONNECTED:
        case SC_PREVIEW_CALL_PROCESS:
            ulRet = DOS_SUCC;
            break;

        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard auth event.");
            ulRet = DOS_SUCC;
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed Preview auth event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return DOS_SUCC;

process_fail:
    if (DOS_ADDR_VALID(pstLCB))
    {
        sc_lcb_free(pstLCB);
        pstLCB = NULL;
    }

    if (DOS_ADDR_VALID(pstSCB))
    {
        sc_scb_free(pstSCB);
        pstSCB = NULL;
    }

    return DOS_FAIL;
}

U32 sc_preview_setup(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32  ulRet = DOS_FAIL;
    SC_LEG_CB    *pstCallingCB = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing preview call setup event event.");

    pstCallingCB = sc_lcb_get(pstSCB->stPreviewCall.ulCallingLegNo);
    if (DOS_ADDR_INVALID(pstCallingCB))
    {
        sc_trace_scb(pstSCB, "There is no calling leg.");

        goto fail_proc;
    }

    switch (pstSCB->stPreviewCall.stSCBTag.usStatus)
    {
        case SC_PREVIEW_CALL_IDEL:
        case SC_PREVIEW_CALL_AUTH:
            /* δ��֤ͨ���������ҶϺ��� */
            goto unauth_proc;
            break;

        case SC_PREVIEW_CALL_EXEC:
        case SC_PREVIEW_CALL_PORC:
        case SC_PREVIEW_CALL_ALERTING:
            /* Ǩ��״̬��proc */
            pstSCB->stPreviewCall.stSCBTag.usStatus = SC_PREVIEW_CALL_PORC;
            ulRet = DOS_SUCC;
            break;

        case SC_PREVIEW_CALL_ACTIVE:
            pstSCB->stPreviewCall.stSCBTag.usStatus = SC_PREVIEW_CALL_CONNECTING;
            ulRet = DOS_SUCC;
            break;

        case SC_PREVIEW_CALL_CONNECTING:
        case SC_PREVIEW_CALL_ALERTING2:
        case SC_PREVIEW_CALL_CONNECTED:
        case SC_PREVIEW_CALL_PROCESS:
            ulRet = DOS_SUCC;
            break;

        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard call setup event.");
            ulRet = DOS_SUCC;
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed preview call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return DOS_SUCC;

unauth_proc:
    return DOS_FAIL;

fail_proc:
    return DOS_FAIL;

}


U32 sc_preview_answer(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32  ulRet = DOS_FAIL;
    SC_LEG_CB    *pstCallingCB = NULL;
    SC_LEG_CB    *pstCalleeCB  = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing preview call setup event event.");

    pstCallingCB = sc_lcb_get(pstSCB->stPreviewCall.ulCallingLegNo);
    if (DOS_ADDR_INVALID(pstCallingCB))
    {
        sc_trace_scb(pstSCB, "There is no calling leg.");

        goto fail_proc;
    }

    switch (pstSCB->stPreviewCall.stSCBTag.usStatus)
    {
        case SC_PREVIEW_CALL_IDEL:
        case SC_PREVIEW_CALL_AUTH:
            ulRet = DOS_FAIL;
            goto unauth_proc;
            break;

        case SC_PREVIEW_CALL_EXEC:
        case SC_PREVIEW_CALL_PORC:
        case SC_PREVIEW_CALL_ALERTING:
            /* ��ϯ��֮ͨ��Ĵ��� */
            /* 1. ����PSTN�ĺ��� */
            /* 2. Ǩ��״̬��CONNTECTING */
            pstCalleeCB = sc_lcb_alloc();
            if (DOS_ADDR_INVALID(pstCalleeCB))
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Alloc lcb fail");
                goto fail_proc;
            }

            pstCalleeCB->stCall.bValid = DOS_TRUE;
            pstCalleeCB->stCall.ucStatus = SC_LEG_INIT;
            pstCalleeCB->ulSCBNo = pstSCB->ulSCBNo;
            pstSCB->stPreviewCall.ulCalleeLegNo = pstCalleeCB->ulCBNo;

            dos_snprintf(pstCalleeCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstCalleeCB->stCall.stNumInfo.szOriginalCallee), pstCallingCB->stCall.stNumInfo.szOriginalCalling);
            dos_snprintf(pstCalleeCB->stCall.stNumInfo.szOriginalCalling, sizeof(pstCalleeCB->stCall.stNumInfo.szOriginalCalling), pstCallingCB->stCall.stNumInfo.szOriginalCallee);

            /* @TODO ͨ�������趨ѡ�����к��룬���õ� pstCalleeCB->stCall.stNumInfo.szRealCalling */
            dos_snprintf(pstCalleeCB->stCall.stNumInfo.szRealCalling, sizeof(pstCalleeCB->stCall.stNumInfo.szRealCalling), pstCallingCB->stCall.stNumInfo.szOriginalCalling);

            if (sc_make_call2pstn(pstSCB, pstCalleeCB) != DOS_SUCC)
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Make call to pstn fail.");
                goto fail_proc;
            }

            pstSCB->stPreviewCall.stSCBTag.usStatus = SC_PREVIEW_CALL_CONNECTING;
            break;

        case SC_PREVIEW_CALL_ACTIVE:
            ulRet = DOS_SUCC;
            break;

        case SC_PREVIEW_CALL_CONNECTING:
        case SC_PREVIEW_CALL_ALERTING2:
            if (sc_req_bridge_call(pstSCB->ulSCBNo, pstSCB->stPreviewCall.ulCalleeLegNo, pstSCB->stPreviewCall.ulCallingLegNo) != DOS_SUCC)
            {
                sc_trace_scb(pstSCB, "Bridge call when early media fail.");
                goto fail_proc;
            }

            pstSCB->stPreviewCall.stSCBTag.usStatus = SC_PREVIEW_CALL_CONNECTED;
            break;

        case SC_PREVIEW_CALL_CONNECTED:
            ulRet = DOS_SUCC;
            break;

        case SC_PREVIEW_CALL_PROCESS:
            /* ����ǩ֮�ڵ�һ������ */
            break;

        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard call setup event.");
            ulRet = DOS_SUCC;
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed preview call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return DOS_SUCC;

unauth_proc:
    return DOS_FAIL;

fail_proc:
    return DOS_FAIL;
}

U32 sc_preview_ringing(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32  ulRet = DOS_FAIL;
    SC_MSG_EVT_RINGING_ST   *pstRinging;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstRinging = (SC_MSG_EVT_RINGING_ST*)pstMsg;

    sc_trace_scb(pstSCB, "Proccessing preview call setup event event.");

    switch (pstSCB->stPreviewCall.stSCBTag.usStatus)
    {
        case SC_PREVIEW_CALL_IDEL:
        case SC_PREVIEW_CALL_AUTH:
            /* δ��֤ͨ���������ҶϺ��� */
            ulRet = DOS_FAIL;
            goto unauth_proc;
            break;

        case SC_PREVIEW_CALL_EXEC:
        case SC_PREVIEW_CALL_PORC:
            /* Ǩ�Ƶ�alerting״̬ */
            if (pstRinging->ulWithMedia)
            {
                sc_req_ringback(pstSCB->ulSCBNo, pstSCB->stPreviewCall.ulCallingLegNo, DOS_TRUE);
            }

            pstSCB->stPreviewCall.stSCBTag.usStatus = SC_PREVIEW_CALL_ALERTING;

            ulRet = DOS_SUCC;
            break;

        case SC_PREVIEW_CALL_ALERTING:
            break;

        case SC_PREVIEW_CALL_ACTIVE:
            ulRet = DOS_SUCC;
            break;

        case SC_PREVIEW_CALL_CONNECTING:
            /* Ǩ�Ƶ�alerting״̬ */
            /* �����ý����Ҫbridge���У�����������Ż����� */
            if (pstRinging->ulWithMedia)
            {
                if (sc_req_bridge_call(pstSCB->ulSCBNo, pstSCB->stPreviewCall.ulCalleeLegNo, pstSCB->stPreviewCall.ulCallingLegNo) != DOS_SUCC)
                {
                    sc_trace_scb(pstSCB, "Bridge call when early media fail.");
                    goto fail_proc;
                }
            }

            pstSCB->stPreviewCall.stSCBTag.usStatus = SC_PREVIEW_CALL_ALERTING2;
            break;

        case SC_PREVIEW_CALL_ALERTING2:
            /* �������ý��״̬Ǩ�Ƶ���ý�壬��Ҫ�����зŻ��� */
            /* �������ý��״̬Ǩ�Ƶ���ý�壬�ŽӺ��� */
            break;

        case SC_PREVIEW_CALL_CONNECTED:
            ulRet = DOS_SUCC;
            break;

        case SC_PREVIEW_CALL_PROCESS:
            ulRet = DOS_SUCC;
            break;

        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard call setup event.");
            ulRet = DOS_SUCC;
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed preview call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return DOS_SUCC;
unauth_proc:
    return DOS_FAIL;

fail_proc:
    return DOS_FAIL;

}

U32 sc_preview_hold(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    return DOS_SUCC;
}

U32 sc_preview_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32  ulRet = DOS_FAIL;
    SC_LEG_CB     *pstCallingCB = NULL;
    SC_LEG_CB     *pstCalleeCB  = NULL;
    SC_MSG_EVT_HUNGUP_ST  *pstHungup = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstHungup = (SC_MSG_EVT_HUNGUP_ST *)pstMsg;

    sc_trace_scb(pstSCB, "Proccessing preview call hungup event.");

    switch (pstSCB->stPreviewCall.stSCBTag.usStatus)
    {
        case SC_PREVIEW_CALL_IDEL:
        case SC_PREVIEW_CALL_AUTH:
        case SC_PREVIEW_CALL_EXEC:
        case SC_PREVIEW_CALL_PORC:
        case SC_PREVIEW_CALL_ALERTING:
        case SC_PREVIEW_CALL_ACTIVE:
            /* ���ʱ��Ҷ�ֻ������ϯ��LEG������Դ���� */
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Hungup with agent not connected.");

            pstCallingCB = sc_lcb_get(pstSCB->stPreviewCall.ulCallingLegNo);
            if (pstCallingCB)
            {
                sc_lcb_free(pstCallingCB);
            }

            sc_scb_free(pstSCB);
            break;

        case SC_PREVIEW_CALL_CONNECTING:
        case SC_PREVIEW_CALL_ALERTING2:
            /* ���ʱ��Ҷϣ���������ϯҲ���ܿͻ�������ǿͻ���Ҫע��LEG��״̬ */
            break;

        case SC_PREVIEW_CALL_CONNECTED:
            /* ���ʱ��Ҷϣ����������ͷŵĽ��࣬������ͺ� */
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Hungup with agent connected.");


            pstCallingCB = sc_lcb_get(pstSCB->stPreviewCall.ulCallingLegNo);
            pstCalleeCB = sc_lcb_get(pstSCB->stPreviewCall.ulCalleeLegNo);
            if (pstSCB->stPreviewCall.ulCalleeLegNo == pstHungup->ulLegNo)
            {
                if (sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stPreviewCall.ulCallingLegNo, CC_ERR_NORMAL_CLEAR) != DOS_SUCC)
                {
                    DOS_ASSERT(0);
                }

                sc_lcb_free(pstCalleeCB);
                pstCallingCB = NULL;
            }
            else
            {
                if (sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stPreviewCall.ulCalleeLegNo, CC_ERR_NORMAL_CLEAR) != DOS_SUCC)
                {
                    DOS_ASSERT(0);
                }

                sc_lcb_free(pstCallingCB);
                pstCalleeCB = NULL;
            }

            pstSCB->stPreviewCall.stSCBTag.usStatus = SC_PREVIEW_CALL_PROCESS;
            break;

        case SC_PREVIEW_CALL_PROCESS:
            /* ��ϯ�������ˣ��Ҷ� */
            break;

        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard call hungup event.");
            ulRet = DOS_SUCC;
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed preview call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return DOS_SUCC;

}

U32 sc_preview_dtmf(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    /* ������ֶ��β��� */
    return DOS_SUCC;
}

U32 sc_preview_record_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    /* ����¼������ */
    return DOS_SUCC;
}

U32 sc_preview_playback_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    /* ����������� */
    return DOS_SUCC;
}

U32 sc_voice_verify_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB  *pstLCB = NULL;
    U32        ulRet = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing auth rsp event for voice verify.");

    pstLCB = sc_lcb_get(pstSCB->stVoiceVerify.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is leg for voice verify.");
        goto proc_finishe;
    }

    switch (pstSCB->stVoiceVerify.stSCBTag.usStatus)
    {
        case SC_VOICE_VERIFY_INIT:
            break;

        case SC_VOICE_VERIFY_AUTH:
            ulRet = sc_make_call2pstn(pstSCB, pstLCB);
            if (ulRet != DOS_SUCC)
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Make call for voice verify fail.");
                goto proc_finishe;
            }

            pstSCB->stVoiceVerify.stSCBTag.usStatus = SC_VOICE_VERIFY_EXEC;
            break;

        case SC_VOICE_VERIFY_EXEC:
            break;

        case SC_VOICE_VERIFY_PROC:
            break;

        case SC_VOICE_VERIFY_ALERTING:
            break;

        case SC_VOICE_VERIFY_ACTIVE:
            break;

        case SC_VOICE_VERIFY_RELEASE:
            break;
    }

proc_finishe:

    if (ulRet != DOS_SUCC)
    {
        if (pstLCB)
        {
            sc_lcb_free(pstLCB);
            pstLCB = NULL;
        }
    }

    sc_trace_scb(pstSCB, "Processed auth rsp event for voice verify. Ret: %s", ulRet != DOS_SUCC ? "FAIL" : "succ");


    if (ulRet != DOS_SUCC)
    {
        if (pstSCB)
        {
            sc_scb_free(pstSCB);
            pstSCB = NULL;
        }
    }

    return ulRet;
}

U32 sc_voice_verify_setup(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB  *pstLCB = NULL;
    U32        ulRet = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing call setup event for voice verify.");

    pstLCB = sc_lcb_get(pstSCB->stVoiceVerify.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is leg for voice verify.");
        goto proc_finishe;
    }

    switch (pstSCB->stVoiceVerify.stSCBTag.usStatus)
    {
        case SC_VOICE_VERIFY_INIT:
            break;

        case SC_VOICE_VERIFY_AUTH:
            break;

        case SC_VOICE_VERIFY_EXEC:
            pstSCB->stVoiceVerify.stSCBTag.usStatus = SC_VOICE_VERIFY_PROC;
            ulRet = DOS_SUCC;
            break;

        case SC_VOICE_VERIFY_PROC:
            break;

        case SC_VOICE_VERIFY_ALERTING:
            break;

        case SC_VOICE_VERIFY_ACTIVE:
            break;

        case SC_VOICE_VERIFY_RELEASE:
            break;
    }


proc_finishe:
    sc_trace_scb(pstSCB, "Processed call setup event for voice verify. Ret: %s", (ulRet == DOS_SUCC) ? "succ" : "FAIL");

    if (ulRet != DOS_SUCC)
    {
        if (DOS_ADDR_VALID(pstLCB) && pstLCB->stCall.bValid && pstLCB->stCall.ucStatus >= SC_LEG_PROC)
        {
            sc_req_hungup(pstSCB->ulSCBNo, pstLCB->ulCBNo, CC_ERR_SC_FORBIDDEN);
        }
        else
        {
            if (DOS_ADDR_VALID(pstLCB))
            {
                sc_lcb_free(pstLCB);
                pstLCB = NULL;
            }

            if (DOS_ADDR_VALID(pstSCB))
            {
                sc_scb_free(pstSCB);
                pstSCB = NULL;
            }
        }
    }

    return ulRet;
}

U32 sc_voice_verify_ringing(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB  *pstLCB = NULL;
    U32        ulRet   = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing call ringing event for voice verify.");

    pstLCB = sc_lcb_get(pstSCB->stVoiceVerify.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is leg for voice verify.");
        goto proc_finishe;
    }

    switch (pstSCB->stVoiceVerify.stSCBTag.usStatus)
    {
        case SC_VOICE_VERIFY_INIT:
            break;

        case SC_VOICE_VERIFY_AUTH:
            break;

        case SC_VOICE_VERIFY_EXEC:
        case SC_VOICE_VERIFY_PROC:
            pstSCB->stVoiceVerify.stSCBTag.usStatus = SC_VOICE_VERIFY_ALERTING;
            ulRet = DOS_SUCC;
            break;

        case SC_VOICE_VERIFY_ALERTING:
            break;

        case SC_VOICE_VERIFY_ACTIVE:
            break;

        case SC_VOICE_VERIFY_RELEASE:
            break;
    }

proc_finishe:
    sc_trace_scb(pstSCB, "Processed call ringing event for voice verify.");

    if (ulRet != DOS_SUCC)
    {
        if (DOS_ADDR_VALID(pstLCB) && pstLCB->stCall.bValid && pstLCB->stCall.ucStatus >= SC_LEG_PROC)
        {
            sc_req_hungup(pstSCB->ulSCBNo, pstLCB->ulCBNo, CC_ERR_SC_FORBIDDEN);
        }
        else
        {
            if (DOS_ADDR_VALID(pstLCB))
            {
                sc_lcb_free(pstLCB);
                pstLCB = NULL;
            }

            if (DOS_ADDR_VALID(pstSCB))
            {
                sc_scb_free(pstSCB);
                pstSCB = NULL;
            }
        }
    }

    return ulRet;
}

U32 sc_voice_verify_answer(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB  *pstLCB = NULL;
    U32        ulRet   = DOS_FAIL;
    U32        ulLoop  = DOS_FAIL;
    SC_MSG_CMD_PLAYBACK_ST  stPlaybackRsp;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing call answer event for voice verify.");

    pstLCB = sc_lcb_get(pstSCB->stVoiceVerify.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is leg for voice verify.");
        goto proc_finishe;
    }

    switch (pstSCB->stVoiceVerify.stSCBTag.usStatus)
    {
        case SC_VOICE_VERIFY_INIT:
            break;

        case SC_VOICE_VERIFY_AUTH:
            break;

        case SC_VOICE_VERIFY_EXEC:
        case SC_VOICE_VERIFY_PROC:
        case SC_VOICE_VERIFY_ALERTING:
            pstSCB->stVoiceVerify.stSCBTag.usStatus = SC_VOICE_VERIFY_ACTIVE;

            stPlaybackRsp.stMsgTag.ulMsgType = SC_CMD_PLAYBACK;
            stPlaybackRsp.stMsgTag.ulSCBNo = pstSCB->ulSCBNo;
            stPlaybackRsp.stMsgTag.usInterErr = 0;
            stPlaybackRsp.ulMode = 0;
            stPlaybackRsp.ulSCBNo = pstSCB->ulSCBNo;
            stPlaybackRsp.ulLegNo = pstLCB->ulCBNo;
            stPlaybackRsp.ulLoopCnt = SC_NUM_VERIFY_TIME;
            stPlaybackRsp.ulInterval = 0;
            stPlaybackRsp.ulSilence  = 0;
            stPlaybackRsp.blTone = DOS_FALSE;
            stPlaybackRsp.ulTotalAudioCnt = 0;

            stPlaybackRsp.aulAudioList[0] = pstSCB->stVoiceVerify.ulTipsHitNo1;
            stPlaybackRsp.ulTotalAudioCnt++;
            for (ulLoop=0; ulLoop<SC_MAX_AUDIO_NUM; ulLoop++)
            {
                if ('\0' == pstSCB->stVoiceVerify.szVerifyCode[ulLoop])
                {
                    break;
                }

                switch (pstSCB->stVoiceVerify.szVerifyCode[ulLoop])
                {
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                        stPlaybackRsp.aulAudioList[stPlaybackRsp.ulTotalAudioCnt] = SC_SND_0 + (pstSCB->stVoiceVerify.szVerifyCode[ulLoop] - '0');
                        stPlaybackRsp.ulTotalAudioCnt++;
                        break;
                    default:
                        DOS_ASSERT(0);
                        break;
                }
            }

            if (sc_send_cmd_playback(&stPlaybackRsp.stMsgTag) != DOS_SUCC)
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Playback request send fail.");
                goto proc_finishe;
            }

            ulRet = DOS_SUCC;

            break;

        case SC_VOICE_VERIFY_ACTIVE:
            break;

        case SC_VOICE_VERIFY_RELEASE:
            break;
    }

proc_finishe:
    sc_trace_scb(pstSCB, "Processed call answer event for voice verify.");

    if (ulRet != DOS_SUCC)
    {
        if (DOS_ADDR_VALID(pstLCB) && pstLCB->stCall.bValid && pstLCB->stCall.ucStatus >= SC_LEG_PROC)
        {
            sc_req_hungup(pstSCB->ulSCBNo, pstLCB->ulCBNo, CC_ERR_SC_FORBIDDEN);
        }
        else
        {
            if (DOS_ADDR_VALID(pstLCB))
            {
                sc_lcb_free(pstLCB);
                pstLCB = NULL;
            }

            if (DOS_ADDR_VALID(pstSCB))
            {
                sc_scb_free(pstSCB);
                pstSCB = NULL;
            }
        }
    }

    return ulRet;
}

U32 sc_voice_verify_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB  *pstLCB = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing call release event for voice verify.");

    pstLCB = sc_lcb_get(pstSCB->stVoiceVerify.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_scb_free(pstSCB);
        pstSCB = NULL;

        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is leg for voice verify.");
        return DOS_FAIL;
    }

    switch (pstSCB->stVoiceVerify.stSCBTag.usStatus)
    {
        case SC_VOICE_VERIFY_INIT:
        case SC_VOICE_VERIFY_AUTH:
        case SC_VOICE_VERIFY_EXEC:
        case SC_VOICE_VERIFY_PROC:
        case SC_VOICE_VERIFY_ALERTING:
            break;

        case SC_VOICE_VERIFY_ACTIVE:
        case SC_VOICE_VERIFY_RELEASE:
            /* ���ͻ��� */
            if (DOS_ADDR_VALID(pstSCB))
            {
                sc_scb_free(pstSCB);
                pstSCB = NULL;
            }

            if (DOS_ADDR_VALID(pstLCB))
            {
                sc_lcb_free(pstLCB);
                pstLCB = NULL;
            }
            break;
    }

    sc_trace_scb(pstSCB, "Processed call release event for voice verify.");

    return DOS_SUCC;
}

U32 sc_voice_verify_playback_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB  *pstLCB = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing call release event for voice verify.");

    pstLCB = sc_lcb_get(pstSCB->stVoiceVerify.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_scb_free(pstSCB);
        pstSCB = NULL;

        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is leg for voice verify.");
        return DOS_FAIL;
    }

    switch (pstSCB->stVoiceVerify.stSCBTag.usStatus)
    {
        case SC_VOICE_VERIFY_INIT:
        case SC_VOICE_VERIFY_AUTH:
        case SC_VOICE_VERIFY_EXEC:
        case SC_VOICE_VERIFY_PROC:
        case SC_VOICE_VERIFY_ALERTING:
            break;

        case SC_VOICE_VERIFY_ACTIVE:
            sc_req_hungup(pstSCB->ulSCBNo, pstLCB->ulCBNo, CC_ERR_NORMAL_CLEAR);
            break;
        case SC_VOICE_VERIFY_RELEASE:
            /* ���ͻ��� */
            if (DOS_ADDR_VALID(pstSCB))
            {
                sc_scb_free(pstSCB);
                pstSCB = NULL;
            }

            if (DOS_ADDR_VALID(pstLCB))
            {
                sc_lcb_free(pstLCB);
                pstLCB = NULL;
            }
            break;
    }

    sc_trace_scb(pstSCB, "Processed call release event for voice verify.");

    return DOS_SUCC;
}


#ifdef __cplusplus
}
#endif /* End of __cplusplus */


