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
#include "sc_bs.h"


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

            sc_trace_scb(pstSCB, "Get call source and dest. Customer: %u, Source: %d, Dest: %d", pstSCB->ulCustomerID, ulCallSrc, ulCallDst);

            /* ���浽scb�У�������õ� */
            pstSCB->stCall.ulCallSrc = ulCallSrc;
            pstSCB->stCall.ulCallDst = ulCallDst;

            /* ���ֺ��� */
            if (SC_DIRECTION_SIP == ulCallSrc && SC_DIRECTION_PSTN == ulCallDst)
            {
                pstSCB->stCall.stSCBTag.usStatus = SC_CALL_AUTH;

                if (DOS_ADDR_VALID(pstSCB->stCall.pstAgentCalling)
                    && DOS_ADDR_VALID(pstSCB->stCall.pstAgentCalling->pstAgentInfo))
                {
                    sc_agent_stat(SC_AGENT_STAT_CALL, pstSCB->stCall.pstAgentCalling->pstAgentInfo, 0, 0);
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
                    sc_agent_stat(SC_AGENT_STAT_CALL, pstSCB->stCall.pstAgentCalling->pstAgentInfo, 0, 0);
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
    SC_LEG_CB                  *pstCalleeLegCB = NULL;
    U32                         ulRet = DOS_FAIL;

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

    switch (pstSCB->stCall.stSCBTag.usStatus)
    {
        case SC_CALL_AUTH:
            //pstSCB->stCall.stSCBTag.usStatus = SC_CALL_EXEC;
            if (pstAuthRsp->ucBalanceWarning)
            {
                /* TODO */
                return sc_req_play_sound(pstSCB->ulSCBNo, pstSCB->stCall.ulCallingLegNo, SC_SND_LOW_BALANCE, 1, 0, 0);
            }

            if (SC_DIRECTION_PSTN == pstSCB->stCall.ulCallSrc && SC_DIRECTION_SIP == pstSCB->stCall.ulCallDst)
            {
                /* ��ֺ��� */
                ulRet = sc_incoming_call_proc(pstSCB, pstLegCB);
            }
            else
            {
                ulRet = sc_outgoing_call_process(pstSCB, pstLegCB);
            }
            break;
         case SC_CALL_AUTH2:
            /* ���б��� */
            pstSCB->stCall.stSCBTag.usStatus = SC_CALL_EXEC;

            pstCalleeLegCB = sc_lcb_get(pstSCB->stCall.ulCalleeLegNo);
            if (DOS_ADDR_INVALID(pstCalleeLegCB))
            {
                return DOS_FAIL;
            }
            ulRet = sc_make_call2pstn(pstSCB, pstCalleeLegCB);
            break;
         default:
            break;
    }

    return ulRet;
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

    sc_trace_scb(pstSCB, "process alerting msg. calling leg: %u, callee leg: %u, status : %u"
                        , pstSCB->stCall.ulCallingLegNo, pstSCB->stCall.ulCalleeLegNo, pstSCB->stCall.stSCBTag.usStatus);

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
        case SC_CALL_AUTH2:
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
    SC_LEG_CB             *pstCallingLegCB = NULL;
    SC_LEG_CB             *pstRecordLegCB = NULL;
    SC_MSG_EVT_ANSWER_ST  *pstEvtAnswer   = NULL;
    SC_MSG_CMD_RECORD_ST  stRecordRsp;

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
        case SC_CALL_AUTH2:
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

                /* �ж��Ƿ���Ҫ¼�� */
                if (pstCalleeLegCB->stRecord.bValid)
                {
                    pstRecordLegCB = pstCalleeLegCB;
                }
                else
                {
                    pstCallingLegCB = sc_lcb_get(pstSCB->stCall.ulCallingLegNo);
                    if (DOS_ADDR_VALID(pstCallingLegCB) && pstCallingLegCB->stRecord.bValid)
                    {
                        pstRecordLegCB = pstCallingLegCB;
                    }
                }

                if (DOS_ADDR_VALID(pstRecordLegCB))
                {
                    stRecordRsp.stMsgTag.ulMsgType = SC_CMD_RECORD;
                    stRecordRsp.stMsgTag.ulSCBNo = pstSCB->ulSCBNo;
                    stRecordRsp.stMsgTag.usInterErr = 0;
                    stRecordRsp.ulSCBNo = pstSCB->ulSCBNo;
                    stRecordRsp.ulLegNo = pstRecordLegCB->ulCBNo;

                    if (sc_send_cmd_record(&stRecordRsp.stMsgTag) != DOS_SUCC)
                    {
                        sc_log(SC_LOG_SET_FLAG(LOG_LEVEL_INFO, SC_MOD_EVENT, SC_LOG_DISIST), "Send record cmd FAIL! SCBNo : %u", pstSCB->ulSCBNo);
                    }
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
    SC_MSG_EVT_HUNGUP_ST *pstHungup     = NULL;
    SC_LEG_CB            *pstCallee     = NULL;
    SC_LEG_CB            *pstCalling    = NULL;
    SC_LEG_CB            *pstHungupLeg  = NULL;
    SC_LEG_CB            *pstOtherLeg   = NULL;
    SC_AGENT_NODE_ST     *pstAgentCall  = NULL;
    S32                  i              = 0;
    S32                  lRes           = DOS_FAIL;

    pstHungup = (SC_MSG_EVT_HUNGUP_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstHungup) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Leg %u has hungup. Legs:%u-%u, status : %u", pstHungup->ulLegNo, pstSCB->stCall.ulCalleeLegNo, pstSCB->stCall.ulCallingLegNo, pstSCB->stCall.stSCBTag.usStatus);

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
                if (sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stCall.ulCallingLegNo, CC_ERR_NORMAL_CLEAR) != DOS_SUCC)
                {
                    sc_lcb_free(pstCalling);
                    pstCalling = NULL;

                    sc_scb_free(pstSCB);
                    pstSCB = NULL;
                }
            }

            pstSCB->stCall.stSCBTag.usStatus = SC_CALL_RELEASE;
            break;

        case SC_CALL_ALERTING:
        case SC_CALL_ACTIVE:
            pstCallee = sc_lcb_get(pstSCB->stCall.ulCalleeLegNo);
            pstCalling = sc_lcb_get(pstSCB->stCall.ulCallingLegNo);
            if (DOS_ADDR_INVALID(pstCallee)
                || DOS_ADDR_INVALID(pstCalling))
            {
                /* TODO ������ */
            }

            if (pstSCB->stCall.ulCallingLegNo == pstHungup->ulLegNo)
            {
                pstCallee->stCall.stTimeInfo.ulByeTime = pstCalling->stCall.stTimeInfo.ulByeTime;
            }

            /* ���ɻ��� */
            if (sc_scb_is_exit_service(pstSCB, BS_SERV_RECORDING))
            {
                if (pstCallee->stRecord.bValid)
                {
                    sc_send_billing_stop2bs_record(pstSCB, pstCallee);
                }
                else
                {
                    sc_send_billing_stop2bs_record(pstSCB, pstCalling);
                }

                sc_scb_remove_service(pstSCB, BS_SERV_RECORDING);
            }

            sc_send_billing_stop2bs(pstSCB, pstCallee, NULL);

            /* �����˵������leg��OK */
            /*
              * ��Ҫ�����Ƿ�ǩ�����⣬�����/����LEG����ǩ�ˣ���Ҫ����SCB��������LEG�ҵ��µ�SCB��
              * ���򣬽���Ҫ��ǩ��LEG��Ϊ��ǰҵ����ƿ������LEG���Ҷ�����һ��LEG
              * ������Ҫ����ͻ����
              */
            /* release ʱ���϶�����һ��leg hungup�ˣ����ڵ�leg��Ҫ�ͷŵ����ж���һ���ǲ�����ϯ��ǩ�����������Ҫ�Ҷ� */

            if (pstSCB->stCall.ulCalleeLegNo == pstHungup->ulLegNo)
            {
                pstHungupLeg = pstCallee;
                pstOtherLeg = pstCalling;
                pstAgentCall = pstSCB->stCall.pstAgentCalling;
            }
            else
            {
                pstHungupLeg = pstCalling;
                pstOtherLeg = pstCallee;
                pstAgentCall = pstSCB->stCall.pstAgentCallee;
            }

            /* �ж��Ƿ���Ҫ���У��ͻ���ǡ�1���ǿͻ�һ���ȹҶϵ�(���������У��ͻ�ֻ����PSTN����ϯֻ����SIP) */
            if ((pstHungupLeg->stCall.ucPeerType == SC_LEG_PEER_INBOUND
                || pstHungupLeg->stCall.ucPeerType == SC_LEG_PEER_OUTBOUND)
                && DOS_ADDR_VALID(pstAgentCall)
                && DOS_ADDR_VALID(pstAgentCall->pstAgentInfo)
                && pstAgentCall->pstAgentInfo->ucProcesingTime != 0)
            {
                /* �ͻ���� */
                pstSCB->stMarkCustom.stSCBTag.bValid = DOS_TRUE;
                pstSCB->stMarkCustom.ulLegNo = pstOtherLeg->ulCBNo;
                pstSCB->stMarkCustom.pstAgentCall = pstAgentCall;
                pstSCB->ulCurrentSrv++;
                pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stMarkCustom.stSCBTag;

                if (pstOtherLeg->ulIndSCBNo == U32_BUTT)
                {
                    /* �ǳ�ǩʱ��Ҫ����ϯ��Ӧ��leg�Ľ���ʱ�䣬��ֵ����ʼʱ�䣬����ǻ���ʱʹ�� */
                    pstOtherLeg->stCall.stTimeInfo.ulStartTime = pstOtherLeg->stCall.stTimeInfo.ulByeTime;
                    for (i=0; i<SC_MAX_SERVICE_TYPE; i++)
                    {
                        pstSCB->aucServType[i] = 0;
                    }

                    if (pstOtherLeg->stCall.ucPeerType == SC_LEG_PEER_INBOUND)
                    {
                        sc_scb_set_service(pstSCB, BS_SERV_INBAND_CALL);
                    }
                    else if(pstOtherLeg->stCall.ucPeerType == SC_LEG_PEER_OUTBOUND)
                    {
                        sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL);
                    }
                    else
                    {
                        sc_scb_set_service(pstSCB, BS_SERV_INTER_CALL);
                    }

                    /* ���ͻ��ĺ����Ϊ���к��� */
                    dos_strcpy(pstOtherLeg->stCall.stNumInfo.szOriginalCalling, pstAgentCall->pstAgentInfo->szLastCustomerNum);
                }

                /* �޸���ϯ״̬Ϊ proc������ ��Ǳ����� */
                if (pstSCB->stMarkCustom.pstAgentCall->pstAgentInfo->ucStatus != SC_ACD_OFFLINE)
                {
                    sc_agent_set_proc(pstAgentCall->pstAgentInfo, OPERATING_TYPE_PHONE);
                }
                sc_req_play_sound(pstSCB->ulSCBNo, pstOtherLeg->ulCBNo, SC_SND_CALL_OVER, 1, 0, 0);
                pstSCB->stMarkCustom.stSCBTag.usStatus = SC_MAKR_CUSTOM_PROC;

                /* ������ʱ�� */
                lRes = dos_tmr_start(&pstSCB->stMarkCustom.stTmrHandle, pstAgentCall->pstAgentInfo->ucProcesingTime * 1000, sc_agent_mark_custom_callback, (U64)pstOtherLeg->ulCBNo, TIMER_NORMAL_NO_LOOP);
                if (lRes < 0)
                {
                    DOS_ASSERT(0);
                    pstSCB->stMarkCustom.stTmrHandle = NULL;
                }

                sc_lcb_free(pstHungupLeg);
                pstHungupLeg = NULL;

                if (pstSCB->stCall.ulCalleeLegNo == pstHungup->ulLegNo)
                {
                    pstSCB->stCall.ulCalleeLegNo = U32_BUTT;
                }
                else
                {
                    pstSCB->stCall.ulCallingLegNo = U32_BUTT;
                }

                pstSCB->stCall.stSCBTag.usStatus = SC_CALL_PROCESS;

                break;
            }

            /* ����Ҫ�ͻ���ǣ�ֱ�ӹҶ���һ��leg */
            sc_lcb_free(pstHungupLeg);
            pstHungupLeg = NULL;

            if (pstSCB->stCall.ulCalleeLegNo == pstHungup->ulLegNo)
            {
                pstSCB->stCall.ulCalleeLegNo = U32_BUTT;
            }
            else
            {
                pstSCB->stCall.ulCallingLegNo = U32_BUTT;
            }

            /* �޸���ϯ��״̬ */
            if (DOS_ADDR_VALID(pstSCB->stCall.pstAgentCallee)
                && DOS_ADDR_VALID(pstSCB->stCall.pstAgentCallee->pstAgentInfo)
                && pstSCB->stCall.pstAgentCallee->pstAgentInfo->ucStatus != SC_ACD_OFFLINE)
            {
                sc_agent_set_idle(pstSCB->stCall.pstAgentCallee->pstAgentInfo, OPERATING_TYPE_PHONE);
            }

            if (DOS_ADDR_VALID(pstSCB->stCall.pstAgentCalling)
                && DOS_ADDR_VALID(pstSCB->stCall.pstAgentCalling->pstAgentInfo)
                && pstSCB->stCall.pstAgentCalling->pstAgentInfo->ucStatus != SC_ACD_OFFLINE)
            {
                sc_agent_set_idle(pstSCB->stCall.pstAgentCalling->pstAgentInfo, OPERATING_TYPE_PHONE);
            }

            if (pstOtherLeg->ulIndSCBNo != U32_BUTT)
            {
                /* ��ǩ���������� */
                pstOtherLeg->ulSCBNo = U32_BUTT;
                sc_req_play_sound(pstOtherLeg->ulIndSCBNo, pstOtherLeg->ulCBNo, SC_SND_MUSIC_SIGNIN, 1, 0, 0);
                /* �ͷŵ� SCB */
                sc_scb_free(pstSCB);
                pstSCB = NULL;
            }
            else
            {
                sc_req_hungup(pstSCB->ulSCBNo, pstOtherLeg->ulCBNo, CC_ERR_NORMAL_CLEAR);
                pstSCB->stCall.stSCBTag.usStatus = SC_CALL_RELEASE;
            }

            break;

        case SC_CALL_PROCESS:
            /* ����LEG���Ҷ��� */
            pstCallee = sc_lcb_get(pstSCB->stCall.ulCalleeLegNo);
            if (DOS_ADDR_VALID(pstCallee))
            {
                sc_lcb_free(pstCallee);
                pstCallee = NULL;
            }

            pstCalling = sc_lcb_get(pstSCB->stCall.ulCallingLegNo);
            if (DOS_ADDR_VALID(pstCalling))
            {
                sc_lcb_free(pstCalling);
                pstCalling = NULL;
            }

            sc_scb_free(pstSCB);
            pstSCB = NULL;
            break;

        case SC_CALL_RELEASE:
            pstCallee = sc_lcb_get(pstSCB->stCall.ulCalleeLegNo);
            if (DOS_ADDR_VALID(pstCallee))
            {
                sc_lcb_free(pstCallee);
                pstCallee = NULL;
            }

            pstCalling = sc_lcb_get(pstSCB->stCall.ulCallingLegNo);
            if (DOS_ADDR_VALID(pstCalling))
            {
                sc_lcb_free(pstCalling);
                pstCalling = NULL;
            }

            sc_scb_free(pstSCB);
            pstSCB = NULL;
            break;
        default:
            DOS_ASSERT(0);
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

    sc_trace_scb(pstSCB, "Processing the call playback stop msg. status: %u", pstSCB->stCall.stSCBTag.usStatus);

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

U32 sc_call_queue_leave(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_LEAVE_CALLQUE_ST *pstEvtCall = NULL;
    U32                 ulRet               = DOS_FAIL;
    SC_LEG_CB           *pstCallingLegCB    = NULL;
    U32                 ulErrCode           = CC_ERR_NO_REASON;

    pstEvtCall = (SC_MSG_EVT_LEAVE_CALLQUE_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstEvtCall) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing call queue event. status : %u", pstSCB->stCall.stSCBTag.usStatus);

    switch (pstSCB->stCall.stSCBTag.usStatus)
    {
        case SC_CALL_AUTH:
            if (SC_LEAVE_CALL_QUE_TIMEOUT == pstMsg->usInterErr)
            {
                /* ������г�ʱ */
            }
            else if (SC_LEAVE_CALL_QUE_SUCC == pstMsg->usInterErr)
            {
                if (DOS_ADDR_INVALID(pstEvtCall->pstAgentNode))
                {
                    /* ���� */
                }
                else
                {
                    pstCallingLegCB = sc_lcb_get(pstSCB->stCall.ulCallingLegNo);
                    /* ������ϯ */
                    ulRet = sc_agent_call_by_id(pstSCB, pstCallingLegCB, pstEvtCall->pstAgentNode->pstAgentInfo->ulAgentID, &ulErrCode);
                }
            }
        default:
            break;

     }

    sc_trace_scb(pstSCB, "Proccessed call queue event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    if (ulRet != DOS_SUCC)
    {
        /* TODO ʧ�ܵĴ��� */
    }

    return ulRet;

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
            stPlaybackRsp.enType = SC_CND_PLAYBACK_SYSTEM;
            stPlaybackRsp.ulTotalAudioCnt = 0;
            stPlaybackRsp.blNeedDTMF = DOS_FALSE;

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

U32 sc_interception_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32  ulRet = DOS_FAIL;
    SC_LEG_CB *pstLCB = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing interception auth event.");

    pstLCB = sc_lcb_get(pstSCB->stInterception.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (pstSCB->stInterception.stSCBTag.usStatus)
    {
        case SC_INTERCEPTION_IDEL:
            ulRet = DOS_SUCC;
            break;

        case SC_INTERCEPTION_AUTH:
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
                    goto proc_finishe;
                    break;
            }

            pstSCB->stInterception.stSCBTag.usStatus = SC_INTERCEPTION_EXEC;
            break;

        case SC_INTERCEPTION_EXEC:
        case SC_INTERCEPTION_PROC:
        case SC_INTERCEPTION_ALERTING:
        case SC_INTERCEPTION_ACTIVE:
        case SC_INTERCEPTION_RELEASE:
            ulRet = DOS_SUCC;
            break;

        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard auth event.");
            ulRet = DOS_SUCC;
            break;
    }

proc_finishe:
    sc_trace_scb(pstSCB, "Proccessed interception call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");
    if (ulRet != DOS_SUCC)
    {
        if (DOS_ADDR_VALID(pstLCB))
        {
            sc_lcb_free(pstLCB);
            pstLCB = NULL;
        }
    }

    return ulRet;
}

U32 sc_interception_setup(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32  ulRet = DOS_FAIL;
    SC_LEG_CB    *pstLCB = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing interception call setup event event.");

    pstLCB = sc_lcb_get(pstSCB->stInterception.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_trace_scb(pstSCB, "There is no calling leg.");

        goto proc_finishe;
    }

    switch (pstSCB->stInterception.stSCBTag.usStatus)
    {
        case SC_INTERCEPTION_IDEL:
        case SC_INTERCEPTION_AUTH:
            /* δ��֤ͨ���������ҶϺ��� */
            goto proc_finishe;
            break;

        case SC_INTERCEPTION_EXEC:
        case SC_INTERCEPTION_PROC:
        case SC_INTERCEPTION_ALERTING:
            /* Ǩ��״̬��proc */
            pstSCB->stInterception.stSCBTag.usStatus = SC_INTERCEPTION_PROC;
            ulRet = DOS_SUCC;
            break;

        case SC_INTERCEPTION_ACTIVE:
            ulRet = DOS_SUCC;
            break;

        case SC_INTERCEPTION_RELEASE:
            ulRet = DOS_SUCC;
            break;

        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard call setup event.");
            ulRet = DOS_SUCC;
            break;
    }

proc_finishe:
    sc_trace_scb(pstSCB, "Proccessed interception call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");
    if (ulRet != DOS_SUCC)
    {
        if (DOS_ADDR_VALID(pstLCB))
        {
            sc_lcb_free(pstLCB);
            pstLCB = NULL;
        }
    }

    return ulRet;

}

U32 sc_interception_ringing(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB  *pstLCB = NULL;
    U32        ulRet   = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing interception ringing event.");

    pstLCB = sc_lcb_get(pstSCB->stInterception.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is leg for interception.");
        goto proc_finishe;
    }

    switch (pstSCB->stInterception.stSCBTag.usStatus)
    {
        case SC_INTERCEPTION_IDEL:
            break;

        case SC_INTERCEPTION_AUTH:
            break;

        case SC_INTERCEPTION_EXEC:
        case SC_INTERCEPTION_PROC:
            pstSCB->stInterception.stSCBTag.usStatus = SC_INTERCEPTION_ALERTING;
            ulRet = DOS_SUCC;
            break;

        case SC_INTERCEPTION_ALERTING:
            break;

        case SC_INTERCEPTION_ACTIVE:
            break;

        case SC_INTERCEPTION_RELEASE:
            break;
    }

proc_finishe:
    sc_trace_scb(pstSCB, "Proccessed interception call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");
    if (ulRet != DOS_SUCC)
    {
        if (DOS_ADDR_VALID(pstLCB))
        {
            sc_lcb_free(pstLCB);
            pstLCB = NULL;
        }
    }

    return ulRet;
}

U32 sc_interception_answer(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB  *pstLCB      = NULL;
    SC_LEG_CB  *pstAgentLCB = NULL;
    U32        ulRet        = DOS_FAIL;
    SC_MSG_CMD_MUX_ST stInterceptRsp;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing interception answer event.");

    pstLCB = sc_lcb_get(pstSCB->stInterception.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is leg for interception.");
        goto proc_finishe;
    }

    pstAgentLCB = sc_lcb_get(pstSCB->stInterception.ulAgentLegNo);
    if (DOS_ADDR_INVALID(pstAgentLCB))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is leg for interception.");
        goto proc_finishe;
    }

    switch (pstSCB->stInterception.stSCBTag.usStatus)
    {
        case SC_INTERCEPTION_IDEL:
            break;

        case SC_INTERCEPTION_AUTH:
            break;

        case SC_INTERCEPTION_EXEC:
        case SC_INTERCEPTION_PROC:
        case SC_INTERCEPTION_ALERTING:
            pstSCB->stInterception.stSCBTag.usStatus = SC_INTERCEPTION_ACTIVE;

            stInterceptRsp.stMsgTag.ulMsgType = SC_CMD_MUX;
            stInterceptRsp.stMsgTag.ulSCBNo = pstSCB->ulSCBNo;
            stInterceptRsp.stMsgTag.usInterErr = 0;

            stInterceptRsp.ulMode = SC_MUX_CMD_INTERCEPT;
            stInterceptRsp.ulSCBNo = pstSCB->ulSCBNo;
            stInterceptRsp.ulLegNo = pstLCB->ulCBNo;
            stInterceptRsp.ulAgentLegNo = pstAgentLCB->ulCBNo;

            if (sc_send_cmd_mux(&stInterceptRsp.stMsgTag) != DOS_SUCC)
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Playback request send fail.");
                goto proc_finishe;
            }

            ulRet = DOS_SUCC;

            break;

        case SC_INTERCEPTION_ACTIVE:
            break;

        case SC_INTERCEPTION_RELEASE:
            break;
    }

proc_finishe:
    sc_trace_scb(pstSCB, "Proccessed interception call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");
    if (ulRet != DOS_SUCC)
    {
        if (DOS_ADDR_VALID(pstLCB))
        {
            sc_lcb_free(pstLCB);
            pstLCB = NULL;
        }
    }

    return ulRet;
}

U32 sc_interception_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB  *pstLCB      = NULL;
    U32        ulRet        = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing interception release event.");

    pstLCB = sc_lcb_get(pstSCB->stInterception.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is leg for interception.");
        return DOS_FAIL;
    }

    switch (pstSCB->stInterception.stSCBTag.usStatus)
    {
        case SC_INTERCEPTION_IDEL:
            break;

        case SC_INTERCEPTION_AUTH:
            break;

        case SC_INTERCEPTION_EXEC:
        case SC_INTERCEPTION_PROC:
        case SC_INTERCEPTION_ALERTING:
            break;

        case SC_INTERCEPTION_ACTIVE:
        case SC_INTERCEPTION_RELEASE:
            /* ���ͻ��� */
            if (ulRet != DOS_SUCC)
            {
                if (DOS_ADDR_VALID(pstLCB))
                {
                    sc_lcb_free(pstLCB);
                    pstLCB = NULL;
                }
            }
            pstSCB->stInterception.stSCBTag.bValid = DOS_FALSE;
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed interception call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return ulRet;

}

U32 sc_whisper_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32  ulRet = DOS_FAIL;
    SC_LEG_CB *pstLCB = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing interception auth event.");

    pstLCB = sc_lcb_get(pstSCB->stWhispered.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (pstSCB->stWhispered.stSCBTag.usStatus)
    {
        case SC_WHISPER_IDEL:
            ulRet = DOS_SUCC;
            break;

        case SC_WHISPER_AUTH:
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
                    goto proc_finishe;
                    break;
            }

            pstSCB->stWhispered.stSCBTag.usStatus = SC_WHISPER_EXEC;
            break;

        case SC_WHISPER_EXEC:
        case SC_WHISPER_PROC:
        case SC_WHISPER_ALERTING:
        case SC_WHISPER_ACTIVE:
        case SC_WHISPER_RELEASE:
            ulRet = DOS_SUCC;
            break;

        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard auth event.");
            ulRet = DOS_SUCC;
            break;
    }

proc_finishe:
    sc_trace_scb(pstSCB, "Proccessed interception call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");
    if (ulRet != DOS_SUCC)
    {
        if (DOS_ADDR_VALID(pstLCB))
        {
            sc_lcb_free(pstLCB);
            pstLCB = NULL;
        }
    }

    return ulRet;
}

U32 sc_whisper_setup(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32  ulRet = DOS_FAIL;
    SC_LEG_CB    *pstLCB = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Proccessing interception call setup event event.");

    pstLCB = sc_lcb_get(pstSCB->stWhispered.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_trace_scb(pstSCB, "There is no calling leg.");

        goto proc_finishe;
    }

    switch (pstSCB->stWhispered.stSCBTag.usStatus)
    {
        case SC_WHISPER_IDEL:
        case SC_WHISPER_AUTH:
            /* δ��֤ͨ���������ҶϺ��� */
            goto proc_finishe;
            break;

        case SC_WHISPER_EXEC:
        case SC_WHISPER_PROC:
        case SC_WHISPER_ALERTING:
            /* Ǩ��״̬��proc */
            pstSCB->stWhispered.stSCBTag.usStatus = SC_WHISPER_PROC;
            ulRet = DOS_SUCC;
            break;

        case SC_WHISPER_ACTIVE:
            ulRet = DOS_SUCC;
            break;

        case SC_WHISPER_RELEASE:
            ulRet = DOS_SUCC;
            break;

        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard call setup event.");
            ulRet = DOS_SUCC;
            break;
    }

proc_finishe:
    sc_trace_scb(pstSCB, "Proccessed interception call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");
    if (ulRet != DOS_SUCC)
    {
        if (DOS_ADDR_VALID(pstLCB))
        {
            sc_lcb_free(pstLCB);
            pstLCB = NULL;
        }
    }

    return ulRet;

}

U32 sc_whisper_ringing(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB  *pstLCB = NULL;
    U32        ulRet   = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing interception ringing event.");

    pstLCB = sc_lcb_get(pstSCB->stWhispered.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is leg for interception.");
        goto proc_finishe;
    }

    switch (pstSCB->stWhispered.stSCBTag.usStatus)
    {
        case SC_WHISPER_IDEL:
            break;

        case SC_WHISPER_AUTH:
            break;

        case SC_WHISPER_EXEC:
        case SC_WHISPER_PROC:
            pstSCB->stWhispered.stSCBTag.usStatus = SC_WHISPER_ALERTING;
            ulRet = DOS_SUCC;
            break;

        case SC_WHISPER_ALERTING:
            break;

        case SC_WHISPER_ACTIVE:
            break;

        case SC_WHISPER_RELEASE:
            break;
    }

proc_finishe:
    sc_trace_scb(pstSCB, "Proccessed interception call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");
    if (ulRet != DOS_SUCC)
    {
        if (DOS_ADDR_VALID(pstLCB))
        {
            sc_lcb_free(pstLCB);
            pstLCB = NULL;
        }
    }

    return ulRet;
}

U32 sc_whisper_answer(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB  *pstLCB      = NULL;
    SC_LEG_CB  *pstAgentLCB = NULL;
    U32        ulRet        = DOS_FAIL;
    SC_MSG_CMD_MUX_ST stInterceptRsp;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing interception answer event.");

    pstLCB = sc_lcb_get(pstSCB->stWhispered.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is leg for interception.");
        goto proc_finishe;
    }

    pstAgentLCB = sc_lcb_get(pstSCB->stWhispered.ulAgentLegNo);
    if (DOS_ADDR_INVALID(pstAgentLCB))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is leg for interception.");
        goto proc_finishe;
    }

    switch (pstSCB->stWhispered.stSCBTag.usStatus)
    {
        case SC_WHISPER_IDEL:
            break;

        case SC_WHISPER_AUTH:
            break;

        case SC_WHISPER_EXEC:
        case SC_WHISPER_PROC:
        case SC_WHISPER_ALERTING:
            pstSCB->stWhispered.stSCBTag.usStatus = SC_WHISPER_ACTIVE;

            stInterceptRsp.stMsgTag.ulMsgType = SC_CMD_MUX;
            stInterceptRsp.stMsgTag.ulSCBNo = pstSCB->ulSCBNo;
            stInterceptRsp.stMsgTag.usInterErr = 0;

            stInterceptRsp.ulMode = SC_MUX_CMD_WHISPER;
            stInterceptRsp.ulSCBNo = pstSCB->ulSCBNo;
            stInterceptRsp.ulLegNo = pstLCB->ulCBNo;
            stInterceptRsp.ulAgentLegNo = pstAgentLCB->ulCBNo;

            if (sc_send_cmd_mux(&stInterceptRsp.stMsgTag) != DOS_SUCC)
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Playback request send fail.");
                goto proc_finishe;
            }

            ulRet = DOS_SUCC;

            break;

        case SC_WHISPER_ACTIVE:
            break;

        case SC_WHISPER_RELEASE:
            break;
    }

proc_finishe:
    sc_trace_scb(pstSCB, "Proccessed interception call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");
    if (ulRet != DOS_SUCC)
    {
        if (DOS_ADDR_VALID(pstLCB))
        {
            sc_lcb_free(pstLCB);
            pstLCB = NULL;
        }
    }

    return ulRet;
}

U32 sc_whisper_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB  *pstLCB      = NULL;
    U32        ulRet        = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing interception release event.");

    pstLCB = sc_lcb_get(pstSCB->stWhispered.ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is leg for interception.");
        return DOS_FAIL;
    }

    switch (pstSCB->stWhispered.stSCBTag.usStatus)
    {
        case SC_WHISPER_IDEL:
            break;

        case SC_WHISPER_AUTH:
            break;

        case SC_WHISPER_EXEC:
        case SC_WHISPER_PROC:
        case SC_WHISPER_ALERTING:
            break;

        case SC_WHISPER_ACTIVE:
        case SC_WHISPER_RELEASE:
            /* ���ͻ��� */
            if (ulRet != DOS_SUCC)
            {
                if (DOS_ADDR_VALID(pstLCB))
                {
                    sc_lcb_free(pstLCB);
                    pstLCB = NULL;
                }
            }
            pstSCB->stWhispered.stSCBTag.bValid = DOS_FALSE;
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed interception call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return ulRet;

}

U32 sc_auto_call_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_AUTH_RESULT_ST  *pstAuthRsp;
    SC_LEG_CB                  *pstLegCB = NULL;
    SC_LEG_CB                  *pstCalleeLegCB = NULL;
    U32                         ulRet = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing auto call auth event. status : %u", pstSCB->stAutoCall.stSCBTag.usStatus);

    pstAuthRsp = (SC_MSG_EVT_AUTH_RESULT_ST *)pstMsg;

    if (pstAuthRsp->stMsgTag.usInterErr != BS_ERR_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Release call with error code %u", pstAuthRsp->stMsgTag.usInterErr);
        /* ע��ͨ��ƫ�������ҵ�CCͳһ����Ĵ����� */

        return DOS_SUCC;
    }

    switch (pstSCB->stAutoCall.stSCBTag.usStatus)
    {
        case SC_AUTO_CALL_AUTH:
            pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_EXEC;
            pstLegCB = sc_lcb_get(pstSCB->stAutoCall.ulCallingLegNo);
            if (DOS_ADDR_INVALID(pstLegCB))
            {
                sc_scb_free(pstSCB);

                DOS_ASSERT(0);

                return DOS_FAIL;
            }
            ulRet = sc_make_call2pstn(pstSCB, pstLegCB);
            break;
        case SC_AUTO_CALL_AUTH2:
            /* ������ϯʱ�����е���֤��������ϯ */
            pstCalleeLegCB = sc_lcb_get(pstSCB->stAutoCall.ulCalleeLegNo);
            if (DOS_ADDR_INVALID(pstCalleeLegCB))
            {
                /* TODO */
                return DOS_FAIL;
            }
            pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_EXEC2;
            ulRet = sc_make_call2pstn(pstSCB, pstCalleeLegCB);
            break;

        default:
            break;
    }

    return ulRet;
}

U32 sc_auto_call_setup(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32  ulRet = DOS_FAIL;
    SC_LEG_CB    *pstLCB = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing auto call stup event. status : %u", pstSCB->stAutoCall.stSCBTag.usStatus);

    pstLCB = sc_lcb_get(pstSCB->stAutoCall.ulCallingLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_trace_scb(pstSCB, "There is no calling leg.");

        goto proc_finishe;
    }

    switch (pstSCB->stAutoCall.stSCBTag.usStatus)
    {
        case SC_AUTO_CALL_IDEL:
        case SC_AUTO_CALL_AUTH:
            /* δ��֤ͨ���������ҶϺ��� */
            goto proc_finishe;
            break;

        case SC_AUTO_CALL_EXEC:
        case SC_AUTO_CALL_PORC:
        case SC_AUTO_CALL_ALERTING:
            /* Ǩ��״̬��proc */
            pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_PORC;
            ulRet = DOS_SUCC;
            break;

        case SC_AUTO_CALL_ACTIVE:
        case SC_AUTO_CALL_AFTER_KEY:
            ulRet = DOS_SUCC;
            break;
        case SC_AUTO_CALL_EXEC2:
            /* ��ϯ��leg���� */
            pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_PORC2;
            ulRet = DOS_SUCC;
            break;
        case SC_AUTO_CALL_CONNECTED:
        case SC_AUTO_CALL_PROCESS:
            ulRet = DOS_SUCC;
            break;
        case SC_AUTO_CALL_RELEASE:
            ulRet = DOS_SUCC;
            break;

        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard call setup event.");
            ulRet = DOS_SUCC;
            break;
    }

proc_finishe:
    sc_trace_scb(pstSCB, "Proccessed auto call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");
    if (ulRet != DOS_SUCC)
    {
        if (DOS_ADDR_VALID(pstLCB))
        {
            sc_lcb_free(pstLCB);
            pstLCB = NULL;
        }
    }

    return ulRet;
}

U32 sc_auto_call_ringing(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32        ulRet   = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing auto call ringing event. status : %u", pstSCB->stAutoCall.stSCBTag.usStatus);

    switch (pstSCB->stAutoCall.stSCBTag.usStatus)
    {
        case SC_AUTO_CALL_IDEL:
        case SC_AUTO_CALL_AUTH:
            /* δ��֤ͨ���������ҶϺ��� */
            goto proc_finishe;
            break;

        case SC_AUTO_CALL_EXEC:
        case SC_AUTO_CALL_PORC:
        case SC_AUTO_CALL_ALERTING:
            pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_ALERTING;
            ulRet = DOS_SUCC;
            break;

        case SC_AUTO_CALL_ACTIVE:
            ulRet = DOS_SUCC;
            break;
        case SC_AUTO_CALL_AFTER_KEY:
            ulRet = DOS_SUCC;
            break;
        case SC_AUTO_CALL_PORC2:
            /* ��ϯ���� */
            if (sc_req_bridge_call(pstSCB->ulSCBNo, pstSCB->stAutoCall.ulCalleeLegNo, pstSCB->stAutoCall.ulCallingLegNo) != DOS_SUCC)
            {
                sc_trace_scb(pstSCB, "Bridge call when early media fail.");
                ulRet = DOS_FAIL;
                goto proc_finishe;
            }

            pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_CONNECTED;
            break;
        case SC_AUTO_CALL_CONNECTED:
        case SC_AUTO_CALL_PROCESS:
            ulRet = DOS_SUCC;
            break;
        case SC_AUTO_CALL_RELEASE:
            ulRet = DOS_SUCC;
            break;

        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard call setup event.");
            ulRet = DOS_SUCC;
            break;
    }

proc_finishe:
    sc_trace_scb(pstSCB, "Proccessed auto call ringing event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return ulRet;
}


U32 sc_auto_call_answer(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    U32          ulRet       = DOS_FAIL;
    U32          ulTaskMode  = U32_BUTT;
    SC_LEG_CB    *pstLCB     = NULL;
    SC_MSG_CMD_PLAYBACK_ST  stPlaybackRsp;
    U32          ulErrCode   = CC_ERR_NO_REASON;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing auto call answer event. status : %u", pstSCB->stAutoCall.stSCBTag.usStatus);

    pstLCB = sc_lcb_get(pstSCB->stAutoCall.ulCallingLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_trace_scb(pstSCB, "There is no calling leg.");

        goto proc_finishe;
    }

    switch (pstSCB->stAutoCall.stSCBTag.usStatus)
    {
        case SC_AUTO_CALL_IDEL:
        case SC_AUTO_CALL_AUTH:
            /* δ��֤ͨ���������ҶϺ��� */
            goto proc_finishe;
            break;

        case SC_AUTO_CALL_EXEC:
        case SC_AUTO_CALL_PORC:
        case SC_AUTO_CALL_ALERTING:
            /* �������� */
            ulTaskMode = sc_task_get_mode(pstSCB->stAutoCall.ulTcbID);
            if (ulTaskMode >= SC_TASK_MODE_BUTT)
            {
                DOS_ASSERT(0);

                ulErrCode = CC_ERR_SC_CONFIG_ERR;
                ulRet = DOS_FAIL;
                goto proc_finishe;
            }

            switch (ulTaskMode)
            {
                /* ��Ҫ�����ģ�ͳһ�ȷ������ڷ����������봦��������� */
                case SC_TASK_MODE_KEY4AGENT:
                case SC_TASK_MODE_KEY4AGENT1:
                case SC_TASK_MODE_AUDIO_ONLY:
                case SC_TASK_MODE_AGENT_AFTER_AUDIO:
                    stPlaybackRsp.stMsgTag.ulMsgType = SC_CMD_PLAYBACK;
                    stPlaybackRsp.stMsgTag.ulSCBNo = pstSCB->ulSCBNo;
                    stPlaybackRsp.stMsgTag.usInterErr = 0;
                    stPlaybackRsp.ulMode = 0;
                    stPlaybackRsp.ulSCBNo = pstSCB->ulSCBNo;
                    stPlaybackRsp.ulLegNo = pstLCB->ulCBNo;
                    stPlaybackRsp.ulLoopCnt = sc_task_get_playcnt(pstSCB->stAutoCall.ulTcbID);
                    stPlaybackRsp.ulInterval = 0;
                    stPlaybackRsp.ulSilence  = 0;
                    stPlaybackRsp.enType = SC_CND_PLAYBACK_FILE;
                    stPlaybackRsp.blNeedDTMF = DOS_TRUE;
                    stPlaybackRsp.ulTotalAudioCnt++;
                    if (sc_task_get_audio_file(pstSCB->stAutoCall.ulTcbID) == NULL)
                    {
                        ulErrCode = CC_ERR_SC_SYSTEM_ABNORMAL;
                        ulRet = DOS_FAIL;
                        goto proc_finishe;
                    }

                    dos_strncpy(stPlaybackRsp.szAudioFile, sc_task_get_audio_file(pstSCB->stAutoCall.ulTcbID), SC_MAX_AUDIO_FILENAME_LEN-1);
                    stPlaybackRsp.szAudioFile[SC_MAX_AUDIO_FILENAME_LEN - 1] = '\0';

                    if (sc_send_cmd_playback(&stPlaybackRsp.stMsgTag) != DOS_SUCC)
                    {
                        ulErrCode = CC_ERR_SC_SYSTEM_ABNORMAL;
                        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Playback request send fail.");
                        goto proc_finishe;
                    }

                    pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_ACTIVE;
                    ulRet = DOS_SUCC;
                    break;

                /* ֱ�ӽ�ͨ��ϯ */
                case SC_TASK_MODE_DIRECT4AGETN:
                    pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_AFTER_KEY;
                    /* ����������ж���ҵ����ƿ� */
                    pstSCB->stIncomingQueue.stSCBTag.bValid = DOS_TRUE;
                    pstSCB->ulCurrentSrv++;
                    pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stIncomingQueue.stSCBTag;
                    pstSCB->stIncomingQueue.ulEnqueuTime = time(NULL);
                    pstSCB->stIncomingQueue.ulLegNo = pstSCB->stAutoCall.ulCallingLegNo;
                    pstSCB->stIncomingQueue.stSCBTag.usStatus = SC_INQUEUE_IDEL;
                    if (sc_cwq_add_call(pstSCB, sc_task_get_agent_queue(pstSCB->stAutoCall.ulTcbID), pstLCB->stCall.stNumInfo.szRealCallee) != DOS_SUCC)
                    {
                        /* �������ʧ�� */
                        DOS_ASSERT(0);
                        ulRet = DOS_FAIL;
                    }
                    else
                    {
                        /* ������ʾ�ͻ��ȴ� */
                        pstSCB->stIncomingQueue.stSCBTag.usStatus = SC_INQUEUE_ACTIVE;
                        sc_req_play_sound(pstSCB->ulSCBNo, pstSCB->stIncomingQueue.ulLegNo, SC_SND_CONNECTING, 1, 0, 0);
                        ulRet = DOS_SUCC;
                    }

                    break;
                default:
                    DOS_ASSERT(0);
                    ulRet = DOS_FAIL;
                    ulErrCode = CC_ERR_SC_CONFIG_ERR;
                    goto proc_finishe;
            }

            break;
        case SC_AUTO_CALL_ACTIVE:
        case SC_AUTO_CALL_AFTER_KEY:
            /* TODO */
        case SC_AUTO_CALL_CONNECTED:
        case SC_AUTO_CALL_PROCESS:
            ulRet = DOS_SUCC;
            break;
        case SC_AUTO_CALL_RELEASE:
            ulRet = DOS_SUCC;
            break;

        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard call setup event.");
            ulRet = DOS_SUCC;
            break;
    }

proc_finishe:
    sc_trace_scb(pstSCB, "Proccessed auto call answer event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    if (ulRet != DOS_SUCC)
    {
        /* TODO ʧ�ܵĴ��� */
    }

    return ulRet;
}

U32 sc_auto_call_palayback_end(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    /* �ж�һ��Ⱥ�������ģʽ������Ǻ��к�ת��ϯ����ת��ϯ������ͨ������ */
    SC_LEG_CB              *pstLCB          = NULL;
    SC_MSG_EVT_PLAYBACK_ST *pstRlayback     = NULL;
    U32                    ulTaskMode       = U32_BUTT;
    U32                    ulErrCode        = CC_ERR_NO_REASON;
    U32                    ulRet            = DOS_SUCC;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "process the auto call playback stop msg. status: %u", pstSCB->stCall.stSCBTag.usStatus);

    pstRlayback = (SC_MSG_EVT_PLAYBACK_ST *)pstMsg;

    pstLCB = sc_lcb_get(pstSCB->stAutoCall.ulCallingLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_trace_scb(pstSCB, "There is no calling leg.");

        goto proc_finishe;
    }

    if (pstLCB->stPlayback.usStatus == SC_SU_PLAYBACK_INIT)
    {
        /* ������ silence_stream ��ʱ��stop�¼�������Ҫ���� ����������⣬������playstop�¼�û���ϴ� */
        ulRet = DOS_SUCC;

        goto proc_finishe;
    }

    switch (pstSCB->stAutoCall.stSCBTag.usStatus)
    {
        case SC_AUTO_CALL_ACTIVE:
            ulTaskMode = sc_task_get_mode(pstSCB->stAutoCall.ulTcbID);
            if (ulTaskMode >= SC_TASK_MODE_BUTT)
            {
                DOS_ASSERT(0);

                ulErrCode = CC_ERR_SC_CONFIG_ERR;
                ulRet = DOS_FAIL;
                goto proc_finishe;
            }

            switch (ulTaskMode)
            {
                /* ��Ҫ�����ģ�ͳһ�ȷ������ڷ����������봦��������� */
                case SC_TASK_MODE_AGENT_AFTER_AUDIO:
                    /* ת��ϯ */
                    pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_AFTER_KEY;
                    /* ����������ж���ҵ����ƿ� */
                    pstSCB->stIncomingQueue.stSCBTag.bValid = DOS_TRUE;
                    pstSCB->ulCurrentSrv++;
                    pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stIncomingQueue.stSCBTag;
                    pstSCB->stIncomingQueue.ulEnqueuTime = time(NULL);
                    pstSCB->stIncomingQueue.ulLegNo = pstSCB->stAutoCall.ulCallingLegNo;
                    pstSCB->stIncomingQueue.stSCBTag.usStatus = SC_INQUEUE_IDEL;
                    if (sc_cwq_add_call(pstSCB, sc_task_get_agent_queue(pstSCB->stAutoCall.ulTcbID), pstLCB->stCall.stNumInfo.szRealCallee) != DOS_SUCC)
                    {
                        /* �������ʧ�� */
                        DOS_ASSERT(0);
                        ulRet = DOS_FAIL;
                    }
                    else
                    {
                        /* ������ʾ�ͻ��ȴ� */
                        pstSCB->stIncomingQueue.stSCBTag.usStatus = SC_INQUEUE_ACTIVE;
                        sc_req_play_sound(pstSCB->ulSCBNo, pstSCB->stIncomingQueue.ulLegNo, SC_SND_CONNECTING, 1, 0, 0);
                        ulRet = DOS_SUCC;
                    }

                    break;
                case SC_TASK_MODE_AUDIO_ONLY:
                    /* �ҶϿͻ� */
                    pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_RELEASE;
                    if (sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stAutoCall.ulCallingLegNo, CC_ERR_NORMAL_CLEAR) != DOS_SUCC)
                    {

                    }

                    break;
                default:
                    break;
            }
    }

proc_finishe:

    sc_trace_scb(pstSCB, "Proccessed auto call playk stop event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return DOS_SUCC;
}

U32 sc_auto_call_queue_leave(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_LEAVE_CALLQUE_ST *pstEvtCall = NULL;
    U32                   ulRet         = DOS_FAIL;

    pstEvtCall = (SC_MSG_EVT_LEAVE_CALLQUE_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstEvtCall) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing auto call queue event. status : %u", pstSCB->stAutoCall.stSCBTag.usStatus);

    switch (pstSCB->stAutoCall.stSCBTag.usStatus)
    {
        case SC_AUTO_CALL_AFTER_KEY:
            if (SC_LEAVE_CALL_QUE_TIMEOUT == pstMsg->usInterErr)
            {
                /* ������г�ʱ */
            }
            else if (SC_LEAVE_CALL_QUE_SUCC == pstMsg->usInterErr)
            {
                if (DOS_ADDR_INVALID(pstEvtCall->pstAgentNode))
                {
                    /* ���� */
                }
                else
                {
                    /* ������ϯ */
                    sc_agent_auto_callback(pstSCB, pstEvtCall->pstAgentNode);
                }
            }
        default:
            break;

     }

    sc_trace_scb(pstSCB, "Proccessed auto call answer event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    if (ulRet != DOS_SUCC)
    {
        /* TODO ʧ�ܵĴ��� */
    }

    return ulRet;

}

U32 sc_auto_call_dtmf(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_DTMF_ST    *pstDTMF      = NULL;
    SC_LEG_CB             *pstLCB       =  NULL;
    U32                   ulTaskMode    = U32_BUTT;
    S32                   lKey          = 0;

    pstDTMF = (SC_MSG_EVT_DTMF_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstDTMF) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing auto call dtmf event. status : %u", pstSCB->stAutoCall.stSCBTag.usStatus);

    pstLCB = sc_lcb_get(pstDTMF->ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    lKey = pstDTMF->cDTMFVal - '0';

    switch (pstSCB->stAutoCall.stSCBTag.usStatus)
    {
        case SC_AUTO_CALL_ACTIVE:
            ulTaskMode = sc_task_get_mode(pstSCB->stAutoCall.ulTcbID);
            if (ulTaskMode >= SC_TASK_MODE_BUTT)
            {
                /* TODO */
                DOS_ASSERT(0);
                //ulErrCode = CC_ERR_SC_CONFIG_ERR;
                return DOS_FAIL;
            }

            switch (ulTaskMode)
            {
                case SC_TASK_MODE_KEY4AGENT1:
                    if (lKey != 0)
                    {
                        break;
                    }
                case SC_TASK_MODE_KEY4AGENT:
                    /* ת��ϯ */
                    pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_AFTER_KEY;
                    sc_cwq_add_call(pstSCB, sc_task_get_agent_queue(pstSCB->stAutoCall.ulTcbID), pstLCB->stCall.stNumInfo.szCallee);
                    break;

                default:
                    break;
            }
            break;
         default:
            break;
    }


    return DOS_SUCC;
}

U32 sc_auto_call_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
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

    sc_trace_scb(pstSCB, "Proccessing auto call hungup event. status : %u", pstSCB->stAutoCall.stSCBTag.usStatus);

    switch (pstSCB->stAutoCall.stSCBTag.usStatus)
    {
        case SC_AUTO_CALL_IDEL:
        case SC_AUTO_CALL_AUTH:
        case SC_AUTO_CALL_EXEC:
        case SC_AUTO_CALL_PORC:
        case SC_AUTO_CALL_ALERTING:
        case SC_AUTO_CALL_ACTIVE:
            /* ���ʱ��Ҷ�ֻ���ǿͻ���LEG������Դ���� */
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Hungup with agent not connected.");

            pstCallingCB = sc_lcb_get(pstSCB->stAutoCall.ulCallingLegNo);
            if (pstCallingCB)
            {
                sc_lcb_free(pstCallingCB);
            }

            sc_scb_free(pstSCB);
            break;

        case SC_AUTO_CALL_AFTER_KEY:
        case SC_AUTO_CALL_AUTH2:
        case SC_AUTO_CALL_EXEC2:
        case SC_AUTO_CALL_PORC2:
        case SC_AUTO_CALL_ALERTING2:
            /* ���ʱ��Ҷϣ���������ϯҲ���ܿͻ�������ǿͻ���Ҫע��LEG��״̬ */
            break;
        case SC_AUTO_CALL_CONNECTED:
            /* ���ʱ��Ҷϣ����������ͷŵĽ��࣬������ͺã��ж���ϯ��״̬ */
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Hungup with agent connected.");
            pstCallingCB = sc_lcb_get(pstSCB->stAutoCall.ulCallingLegNo);
            pstCalleeCB = sc_lcb_get(pstSCB->stAutoCall.ulCalleeLegNo);
            if (pstSCB->stAutoCall.ulCalleeLegNo == pstHungup->ulLegNo)
            {
                if (sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stAutoCall.ulCallingLegNo, CC_ERR_NORMAL_CLEAR) != DOS_SUCC)
                {
                    DOS_ASSERT(0);
                }

                sc_lcb_free(pstCalleeCB);
                pstCallingCB = NULL;
            }
            else
            {
                if (pstCalleeCB->ulIndSCBNo != U32_BUTT)
                {
                    /* TODO ��ǩ���ж�һ���Ƿ���Ҫ��ǿͻ���Ҫע��scb���ͷ� */
                }
                else
                {
                    if (sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stAutoCall.ulCalleeLegNo, CC_ERR_NORMAL_CLEAR) != DOS_SUCC)
                    {
                        DOS_ASSERT(0);
                    }
                }
                sc_lcb_free(pstCallingCB);
                pstCalleeCB = NULL;
            }

            pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_PROCESS;
            break;

        case SC_AUTO_CALL_PROCESS:
            /* ��ϯ�������ˣ��Ҷ� */
            pstCalleeCB = sc_lcb_get(pstSCB->stCall.ulCalleeLegNo);
            if (DOS_ADDR_VALID(pstCalleeCB))
            {
                sc_lcb_free(pstCalleeCB);
                pstCalleeCB = NULL;
            }

            pstCallingCB = sc_lcb_get(pstSCB->stCall.ulCallingLegNo);
            if (DOS_ADDR_VALID(pstCallingCB))
            {
                sc_lcb_free(pstCallingCB);
                pstCallingCB = NULL;
            }

            sc_scb_free(pstSCB);
            pstSCB = NULL;
            break;
        case SC_AUTO_CALL_RELEASE:
            pstCalleeCB = sc_lcb_get(pstSCB->stCall.ulCalleeLegNo);
            if (DOS_ADDR_VALID(pstCalleeCB))
            {
                sc_lcb_free(pstCalleeCB);
                pstCalleeCB = NULL;
            }

            pstCallingCB = sc_lcb_get(pstSCB->stCall.ulCallingLegNo);
            if (DOS_ADDR_VALID(pstCallingCB))
            {
                sc_lcb_free(pstCallingCB);
                pstCallingCB = NULL;
            }

            sc_scb_free(pstSCB);
            pstSCB = NULL;

            break;
        default:
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Discard call hungup event.");
            ulRet = DOS_SUCC;
            break;
    }

    sc_trace_scb(pstSCB, "Proccessed auto call setup event. Result: %s", (DOS_SUCC == ulRet) ? "succ" : "FAIL");

    return DOS_SUCC;
}


U32 sc_sigin_auth_rsp(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_AUTH_RESULT_ST  *pstAuthRsp;
    SC_LEG_CB                  *pstLegCB = NULL;
    U32                         ulRet = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    pstAuthRsp = (SC_MSG_EVT_AUTH_RESULT_ST *)pstMsg;

    sc_trace_scb(pstSCB, "Processing the sigin auth msg. status: %u", pstSCB->stSigin.stSCBTag.usStatus);

    if (!pstSCB->stSigin.stSCBTag.bValid)
    {
        /* û������ ��ǩҵ�� */
        sc_scb_free(pstSCB);

        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Scb(%u) not have sigin server.", pstSCB->ulSCBNo);
        return DOS_FAIL;
    }

    pstLegCB = sc_lcb_get(pstSCB->stSigin.ulLegNo);
    if (DOS_ADDR_INVALID(pstLegCB))
    {
        sc_scb_free(pstSCB);

        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (pstAuthRsp->stMsgTag.usInterErr != BS_ERR_SUCC)
    {
        sc_scb_free(pstSCB);

        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Release call with error code %u", pstAuthRsp->stMsgTag.usInterErr);
        /* ע��ͨ��ƫ�������ҵ�CCͳһ����Ĵ����� */

        return DOS_FAIL;
    }

    switch (pstSCB->stSigin.stSCBTag.usStatus)
    {
        case SC_SIGIN_AUTH:
            /* ������� */
            pstSCB->stSigin.stSCBTag.usStatus = SC_SIGIN_EXEC;
            ulRet = sc_make_call2pstn(pstSCB, pstLegCB);
            break;
         default:
            break;
    }

    return ulRet;
}

U32 sc_sigin_setup(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB   *pstLegCB = NULL;
    SC_MSG_EVT_CALL_ST  *pstCallSetup;
    U32         ulRet            = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstCallSetup = (SC_MSG_EVT_CALL_ST*)pstMsg;

    sc_trace_scb(pstSCB, "Processing the sigin setup msg. status: %u", pstSCB->stSigin.stSCBTag.usStatus);

    pstLegCB = sc_lcb_get(pstSCB->stSigin.ulLegNo);
    if (DOS_ADDR_INVALID(pstLegCB))
    {
        sc_scb_free(pstSCB);

        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (pstSCB->stSigin.stSCBTag.usStatus)
    {
        case SC_SIGIN_EXEC:
            /* ������� */
            pstSCB->stSigin.stSCBTag.usStatus = SC_SIGIN_PORC;
            ulRet = DOS_SUCC;
            break;
         default:
            break;
    }

    return ulRet;
}

U32 sc_sigin_ringing(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_RINGING_ST *pstEvent = NULL;
    SC_LEG_CB             *pstLegCB = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstEvent = (SC_MSG_EVT_RINGING_ST *)pstMsg;

    sc_trace_scb(pstSCB, "process the sigin alerting msg. status: %u", pstSCB->stSigin.stSCBTag.usStatus);

    pstLegCB = sc_lcb_get(pstSCB->stSigin.ulLegNo);
    if (DOS_ADDR_INVALID(pstLegCB))
    {
        sc_scb_free(pstSCB);

        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    pstSCB->stSigin.pstAgentNode->pstAgentInfo->ulLegNo = pstSCB->stSigin.ulLegNo;

    switch (pstSCB->stSigin.stSCBTag.usStatus)
    {
        case SC_SIGIN_IDEL:
        case SC_SIGIN_AUTH:
        case SC_SIGIN_PORC:
        case SC_SIGIN_EXEC:
            pstSCB->stSigin.stSCBTag.usStatus = SC_SIGIN_ALERTING;
            break;

        case SC_SIGIN_ALERTING:
        case SC_SIGIN_ACTIVE:
        case SC_SIGIN_RELEASE:
            break;
    }

    return DOS_SUCC;
}

U32 sc_sigin_answer(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_ANSWER_ST  *pstEvtAnswer   = NULL;
    SC_LEG_CB             *pstLegCB = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "process the sigin answer msg. status: %u", pstSCB->stSigin.stSCBTag.usStatus);

    pstEvtAnswer = (SC_MSG_EVT_ANSWER_ST *)pstMsg;

    pstLegCB = sc_lcb_get(pstSCB->stSigin.ulLegNo);
    if (DOS_ADDR_INVALID(pstLegCB))
    {
        sc_scb_free(pstSCB);

        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (pstSCB->stSigin.stSCBTag.usStatus)
    {
        case SC_SIGIN_ALERTING:
            pstSCB->stSigin.stSCBTag.usStatus = SC_SIGIN_ACTIVE;

            if (DOS_ADDR_VALID(pstSCB->stSigin.pstAgentNode))
            {
                pstSCB->stSigin.pstAgentNode->pstAgentInfo->bConnected = DOS_TRUE;
            }
            /* �ų�ǩ�� */
            sc_req_play_sound(pstSCB->ulSCBNo, pstSCB->stSigin.ulLegNo, SC_SND_MUSIC_SIGNIN, 1, 0, 0);

            break;
        default:
            break;
    }

    return DOS_SUCC;
}


U32 sc_sigin_playback_stop(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB              *pstLegCB        = NULL;
    SC_MSG_EVT_PLAYBACK_ST *pstRlayback     = NULL;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "process the sigin playback stop msg. status: %u", pstSCB->stSigin.stSCBTag.usStatus);

    pstRlayback = (SC_MSG_EVT_PLAYBACK_ST *)pstMsg;

    pstLegCB = sc_lcb_get(pstSCB->stSigin.ulLegNo);
    if (DOS_ADDR_INVALID(pstLegCB))
    {
        sc_scb_free(pstSCB);

        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (pstSCB->stSigin.stSCBTag.usStatus)
    {
        case SC_SIGIN_ACTIVE:
            if (DOS_ADDR_INVALID(pstSCB->stSigin.pstAgentNode))
            {
                break;
            }

            if (pstSCB->stSigin.pstAgentNode->pstAgentInfo->bNeedConnected)
            {
                if (pstLegCB->stPlayback.usStatus == SC_SU_PLAYBACK_INIT)
                {
                    /* �ų�ǩ�� */
                    sc_req_play_sound(pstSCB->ulSCBNo, pstSCB->stSigin.ulLegNo, SC_SND_MUSIC_SIGNIN, 1, 0, 0);
                }
            }
            else
            {

            }

            break;
        default:
            break;
    }

    return DOS_SUCC;
}

U32 sc_sigin_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB              *pstLegCB        = NULL;
    SC_MSG_EVT_PLAYBACK_ST *pstRlayback     = NULL;
    U32                     ulRet           = DOS_FAIL;
    SC_MSG_CMD_CALL_ST stCallMsg;

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "process the sigin playback msg. status: %u", pstSCB->stSigin.stSCBTag.usStatus);

    pstRlayback = (SC_MSG_EVT_PLAYBACK_ST *)pstMsg;

    pstLegCB = sc_lcb_get(pstSCB->stSigin.ulLegNo);
    if (DOS_ADDR_INVALID(pstLegCB))
    {
        sc_scb_free(pstSCB);

        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (DOS_ADDR_INVALID(pstSCB->stSigin.pstAgentNode))
    {
        sc_scb_free(pstSCB);
        sc_lcb_free(pstLegCB);

        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (pstSCB->stSigin.stSCBTag.usStatus)
    {
        case SC_SIGIN_ACTIVE:
            if (pstSCB->stSigin.pstAgentNode->pstAgentInfo->bNeedConnected)
            {
                /* ��Ҫ���º�����ϯ�����г�ǩ */
                pstLegCB->stPlayback.usStatus = SC_SU_PLAYBACK_INIT;
                pstSCB->stSigin.pstAgentNode->pstAgentInfo->bConnected = DOS_FALSE;
                if (pstLegCB->stCall.ucPeerType == SC_LEG_PEER_OUTBOUND)
                {
                    /* ��Ҫ��֤ */
                    pstSCB->stSigin.stSCBTag.usStatus = SC_SIGIN_AUTH;
                    ulRet = sc_send_usr_auth2bs(pstSCB, pstLegCB);
                    if (ulRet != DOS_SUCC)
                    {
                        sc_scb_free(pstSCB);
                        sc_lcb_free(pstLegCB);

                        return DOS_FAIL;
                    }
                }
                else
                {
                    /* ������� */
                    /* ����һ�º��� */
                    dos_snprintf(pstLegCB->stCall.stNumInfo.szRealCallee, sizeof(pstLegCB->stCall.stNumInfo.szCallee), pstLegCB->stCall.stNumInfo.szOriginalCallee);
                    dos_snprintf(pstLegCB->stCall.stNumInfo.szRealCalling, sizeof(pstLegCB->stCall.stNumInfo.szCalling), pstLegCB->stCall.stNumInfo.szOriginalCalling);

                    dos_snprintf(pstLegCB->stCall.stNumInfo.szCallee, sizeof(pstLegCB->stCall.stNumInfo.szCallee), pstLegCB->stCall.stNumInfo.szOriginalCallee);
                    dos_snprintf(pstLegCB->stCall.stNumInfo.szCalling, sizeof(pstLegCB->stCall.stNumInfo.szCalling), pstLegCB->stCall.stNumInfo.szOriginalCalling);

                    pstSCB->stSigin.stSCBTag.usStatus = SC_SIGIN_EXEC;

                    stCallMsg.stMsgTag.ulMsgType = SC_CMD_CALL;
                    stCallMsg.stMsgTag.ulSCBNo = pstSCB->ulSCBNo;
                    stCallMsg.stMsgTag.usInterErr = 0;
                    stCallMsg.ulSCBNo = pstSCB->ulSCBNo;
                    stCallMsg.ulLCBNo = pstLegCB->ulCBNo;

                    if (sc_send_cmd_new_call(&stCallMsg.stMsgTag) != DOS_SUCC)
                    {
                        sc_scb_free(pstSCB);
                        sc_lcb_free(pstLegCB);

                        return DOS_FAIL;
                    }
                }
            }
            else
            {
                /* �ͷ� */
                pstSCB->stSigin.pstAgentNode->pstAgentInfo->bConnected = DOS_FALSE;
                sc_scb_free(pstSCB);
                sc_lcb_free(pstLegCB);
            }
            break;
        default:
            /* �ͷ� */
            pstSCB->stSigin.pstAgentNode->pstAgentInfo->bConnected = DOS_FALSE;
            pstSCB->stSigin.pstAgentNode->pstAgentInfo->bNeedConnected = DOS_FALSE;
            sc_scb_free(pstSCB);
            sc_lcb_free(pstLegCB);
            break;
    }

    return DOS_SUCC;
}

U32 sc_incoming_queue_leave(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_LEAVE_CALLQUE_ST *pstEvtCall = NULL;

    pstEvtCall = (SC_MSG_EVT_LEAVE_CALLQUE_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstEvtCall) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing leave call queue event. status : %u", pstSCB->stIncomingQueue.stSCBTag.usStatus);

    switch (pstSCB->stIncomingQueue.stSCBTag.usStatus)
    {
        case SC_INQUEUE_ACTIVE:
            {
                pstSCB->stIncomingQueue.stSCBTag.bWaitingExit = DOS_TRUE;

            }
            break;
        default:
            break;
    }


    return DOS_SUCC;
}

U32 sc_mark_custom_dtmf(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_MSG_EVT_DTMF_ST    *pstDTMF      = NULL;
    SC_LEG_CB             *pstLCB       =  NULL;
    U32                   ulRet         = DOS_SUCC;
    U32                   ulKey         = 0;

    pstDTMF = (SC_MSG_EVT_DTMF_ST *)pstMsg;
    if (DOS_ADDR_INVALID(pstDTMF) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    sc_trace_scb(pstSCB, "Processing mark custom dtmf event. status : %u", pstSCB->stMarkCustom.stSCBTag.usStatus);

    pstLCB = sc_lcb_get(pstDTMF->ulLegNo);
    if (DOS_ADDR_INVALID(pstLCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    switch (pstSCB->stMarkCustom.stSCBTag.usStatus)
    {
        case SC_MAKR_CUSTOM_PROC:
            if (pstSCB->stMarkCustom.szDialCache[0] == '\0'
                && pstDTMF->cDTMFVal != '*')
            {
                /* ��һ���ַ����� * ���� #�����ñ��� */
                break;
            }

            /* ���浽������ */
            dos_snprintf(pstSCB->stMarkCustom.szDialCache+dos_strlen(pstSCB->stMarkCustom.szDialCache), SC_MAX_ACCESS_CODE_LENGTH-dos_strlen(pstSCB->stMarkCustom.szDialCache), "%c", pstDTMF->cDTMFVal);

            /* ���Ϊ * ���� #���ж��Ƿ����  [*|#]D[*|#] */
            if ((pstDTMF->cDTMFVal == '*' || pstDTMF->cDTMFVal == '#')
                    && dos_strlen(pstSCB->stMarkCustom.szDialCache) > 1)
            {
                if (pstSCB->stMarkCustom.szDialCache[0] == '*'
                    && pstSCB->stMarkCustom.szDialCache[3]  == '\0'
                    && 1 == dos_sscanf(pstSCB->stMarkCustom.szDialCache+1, "%u", &ulKey)
                    && ulKey <= 9)
                {
                    /* �ͻ���� */
                    if (DOS_ADDR_VALID(pstSCB->stMarkCustom.pstAgentCall)
                         && DOS_ADDR_VALID(pstSCB->stMarkCustom.pstAgentCall->pstAgentInfo))
                    {
                        sc_agent_marker_update_req(pstSCB->ulCustomerID, pstSCB->stMarkCustom.pstAgentCall->pstAgentInfo->ulAgentID, ulKey, pstSCB->stMarkCustom.pstAgentCall->pstAgentInfo->szLastCustomerNum);
                    }

                    /* ֹͣ��ʱ�� */
                    if (DOS_ADDR_VALID(pstSCB->stMarkCustom.stTmrHandle))
                    {
                        dos_tmr_stop(&pstSCB->stMarkCustom.stTmrHandle);
                        pstSCB->stMarkCustom.stTmrHandle = NULL;
                    }

                    /* ֹͣ���� */
                    sc_req_playback_stop(pstSCB->ulSCBNo, pstLCB->ulCBNo);

                    /* �ж���ϯ�Ƿ��ǳ�ǩ�����������Ҷϵ绰 */
                    if (pstSCB->stMarkCustom.pstAgentCall->pstAgentInfo->ucStatus != SC_ACD_OFFLINE)
                    {
                        sc_agent_set_idle(pstSCB->stMarkCustom.pstAgentCall->pstAgentInfo, OPERATING_TYPE_PHONE);
                    }

                    if (pstLCB->ulIndSCBNo != U32_BUTT)
                    {
                        /* ��ǩ���������� */
                        pstLCB->ulSCBNo = U32_BUTT;
                        sc_req_play_sound(pstLCB->ulIndSCBNo, pstLCB->ulCBNo, SC_SND_MUSIC_SIGNIN, 1, 0, 0);
                        /* �ͷŵ� SCB */
                        sc_scb_free(pstSCB);
                        pstSCB = NULL;
                    }
                    else
                    {
                        sc_req_hungup(pstSCB->ulSCBNo, pstLCB->ulCBNo, CC_ERR_NORMAL_CLEAR);
                        pstSCB->stMarkCustom.stSCBTag.usStatus = SC_MAKR_CUSTOM_ACTIVE;
                    }
                }
                else
                {
                    /* ��ʽ������ջ��� */
                    pstSCB->stMarkCustom.szDialCache[0] = '\0';
                }

            }

            break;
         default:
            break;
    }

    return ulRet;
}

U32 sc_mark_custom_release(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{
    SC_LEG_CB            *pstMarkLeg    = NULL;

    sc_trace_scb(pstSCB, "Processing mark custom realse event. status : %u", pstSCB->stMarkCustom.stSCBTag.usStatus);

    switch (pstSCB->stMarkCustom.stSCBTag.usStatus)
    {
        case SC_MAKR_CUSTOM_ACTIVE:
            /* ��Ҫ���ɿͻ���ǵĻ�����leg������ҵ�����ͷ� */
            pstMarkLeg = sc_lcb_get(pstSCB->stMarkCustom.ulLegNo);
            if (DOS_ADDR_VALID(pstMarkLeg))
            {
                /* ���к����ΪΪ �ͻ���� */
                if (pstSCB->stMarkCustom.szDialCache[0] == '*'
                    && pstSCB->stMarkCustom.szDialCache[1] >= '0'
                    && pstSCB->stMarkCustom.szDialCache[1] <= '9'
                    && (pstSCB->stMarkCustom.szDialCache[2] == '*' || pstSCB->stMarkCustom.szDialCache[2] == '#')
                    && pstSCB->stMarkCustom.szDialCache[3] == '\0')
                {
                    dos_strcpy(pstMarkLeg->stCall.stNumInfo.szOriginalCallee, pstSCB->stMarkCustom.szDialCache);
                }
                else
                {
                    dos_strcpy(pstMarkLeg->stCall.stNumInfo.szOriginalCallee, "*#");
                }

                sc_send_billing_stop2bs(pstSCB, pstMarkLeg, NULL);
            }
            pstSCB->stMarkCustom.stSCBTag.bWaitingExit = DOS_TRUE;
            break;
        default:
            break;
    }

    return DOS_SUCC;
}


#ifdef __cplusplus
}
#endif /* End of __cplusplus */


