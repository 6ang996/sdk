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


U32 sc_call2pstn(SC_SRV_CB *pstSCB, SC_LEG_CB *pstCallingLegCB)
{
    SC_MSG_CMD_CALL_ST          stCallMsg;
    U32                         ulRet = DOS_FAIL;

    stCallMsg.stMsgTag.ulMsgType = SC_CMD_CALL;
    stCallMsg.stMsgTag.ulSCBNo = pstSCB->ulSCBNo;
    stCallMsg.stMsgTag.usInterErr = 0;
    stCallMsg.ulPeerType = SC_LEG_PEER_OUTBOUND;
    stCallMsg.ulSCBNo = pstSCB->ulSCBNo;
    stCallMsg.ulCodecCnt = 0;
    stCallMsg.ulTrunkCnt = 0;
    stCallMsg.szEIXAddr[0] = '\0';

    /* ���ֺ�����Ҫ��·�� */
    pstSCB->stCall.ulRouteID = sc_route_search(pstSCB, stCallMsg.stNumInfo.szRealCalling, stCallMsg.stNumInfo.szRealCallee);
    if (U32_BUTT == pstSCB->stCall.ulRouteID)
    {
        sc_trace_scb(pstSCB, "no route to pstn.");

        return sc_req_hungup_with_sound(pstSCB->ulSCBNo, pstCallingLegCB->ulCBNo, CC_ERR_SC_NO_ROUTE);
    }

    /* ���û���ҵ��м̣���Ҫ�ܾ����� */
    stCallMsg.ulTrunkCnt = sc_route_get_trunks(pstSCB->stCall.ulRouteID, stCallMsg.aulTrunkList, SC_MAX_TRUCK_NUM);
    if (0 == stCallMsg.ulTrunkCnt)
    {
        sc_trace_scb(pstSCB, "no trunk to pstn. route id:%u", pstSCB->stCall.ulRouteID);

        return sc_req_hungup_with_sound(pstSCB->ulSCBNo, pstCallingLegCB->ulCBNo, CC_ERR_SC_NO_TRUNK);
    }

    /* ά��һ�����к��� */
    dos_snprintf(pstCallingLegCB->stCall.stNumInfo.szRealCalling
                    , sizeof(pstCallingLegCB->stCall.stNumInfo.szRealCalling)
                    , pstCallingLegCB->stCall.stNumInfo.szOriginalCalling);

    dos_snprintf(pstCallingLegCB->stCall.stNumInfo.szRealCallee
                    , sizeof(pstCallingLegCB->stCall.stNumInfo.szRealCallee)
                    , pstCallingLegCB->stCall.stNumInfo.szOriginalCallee);

    dos_snprintf(stCallMsg.stNumInfo.szOriginalCallee, sizeof(stCallMsg.stNumInfo.szCallee), pstCallingLegCB->stCall.stNumInfo.szRealCallee);
    dos_snprintf(stCallMsg.stNumInfo.szOriginalCalling, sizeof(stCallMsg.stNumInfo.szCalling), pstCallingLegCB->stCall.stNumInfo.szRealCalling);

    dos_snprintf(stCallMsg.stNumInfo.szRealCallee, sizeof(stCallMsg.stNumInfo.szCallee), pstCallingLegCB->stCall.stNumInfo.szRealCallee);
    dos_snprintf(stCallMsg.stNumInfo.szRealCalling, sizeof(stCallMsg.stNumInfo.szCalling), pstCallingLegCB->stCall.stNumInfo.szRealCalling);

    dos_snprintf(stCallMsg.stNumInfo.szCallee, sizeof(stCallMsg.stNumInfo.szCallee), pstCallingLegCB->stCall.stNumInfo.szRealCallee);
    dos_snprintf(stCallMsg.stNumInfo.szCalling, sizeof(stCallMsg.stNumInfo.szCalling), pstCallingLegCB->stCall.stNumInfo.szRealCalling);

    ulRet = sc_send_cmd_new_call(&stCallMsg.stMsgTag);
    if (ulRet == DOS_SUCC)
    {
        pstSCB->stCall.stSCBTag.usStatus = SC_CALL_EXEC;
    }

    return ulRet;
}

U32 sc_call_access_code(SC_SRV_CB *pstSCB, S8 *pszAccessCode)
{
    return DOS_SUCC;
}

/**
 * �����ڲ�����
 *
 * @param SC_MSG_TAG_ST *pstMsg
 * @param SC_SRV_CB *pstSCB
 *
 * @return �ɹ�����DOS_SUCC,ʧ�ܷ���DOS_FAIL
 */
U32 sc_internal_call_process(SC_SRV_CB *pstSCB, SC_LEG_CB *pstLegCB)
{
    SC_ACD_AGENT_QUEUE_NODE_ST *pstAgent    = NULL;
    SC_MSG_CMD_CALL_ST          stCallMsg;
    SC_SRV_CB                  *pstSCBAgent = NULL;
    SC_LEG_CB                  *pstLegCBAgent = NULL;
    U32                        ulCustomerID;

    if (DOS_ADDR_INVALID(pstSCB) || DOS_ADDR_INVALID(pstLegCB))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    /* ���ұ��У���������Ѿ���ǩ�ˣ�ֱ�����Ӿͺã�����ҵ����ƿ�ϲ� */
    /* �����п����Ƿֻ��ţ�SIP�˻� */
    ulCustomerID = sc_sip_account_get_customer(pstLegCB->stCall.stNumInfo.szOriginalCallee);
    if (U32_BUTT == ulCustomerID)
    {
        /* �������ͨ��SIP�˻���ȡ���û�ID����Ҫ�鿴�Ƿ��Ƿֻ��ţ�ͬʱ���ֻ��Ŷ�Ӧ��SIP�˻���Ϊ��ʵ����ʹ�õĺ��� */
        if (sc_sip_account_get_by_extension(pstSCB->ulCustomerID
                        , pstLegCB->stCall.stNumInfo.szOriginalCallee
                        , pstLegCB->stCall.stNumInfo.szRealCallee
                        , sizeof(pstLegCB->stCall.stNumInfo.szRealCallee)) != DOS_SUCC)
        {
            DOS_ASSERT(0);

            return DOS_FAIL;
        }
    }
    else
    {
        dos_snprintf(pstLegCB->stCall.stNumInfo.szRealCallee
                        , sizeof(pstLegCB->stCall.stNumInfo.szRealCallee)
                        , pstLegCB->stCall.stNumInfo.szOriginalCallee);
    }

    /* ά��һ�����к��� */
    dos_snprintf(pstLegCB->stCall.stNumInfo.szRealCalling
                    , sizeof(pstLegCB->stCall.stNumInfo.szRealCalling)
                    , pstLegCB->stCall.stNumInfo.szOriginalCalling);

    pstAgent = sc_get_agent_by_sip_acc(pstLegCB->stCall.stNumInfo.szRealCallee);
    if (DOS_ADDR_INVALID(pstAgent)
        || DOS_ADDR_INVALID(pstAgent->pstAgentInfo))
    {
        goto processing;
    }

    if (pstAgent->pstAgentInfo->ulSCBNo < SC_SCB_SIZE)
    {
        pstSCBAgent = sc_scb_get(pstAgent->pstAgentInfo->ulSCBNo);
        if (DOS_ADDR_INVALID(pstSCBAgent))
        {
            goto processing;
        }

        pstLegCBAgent = sc_lcb_get(pstSCBAgent->stCall.ulCallingLegNo);
        if (DOS_ADDR_INVALID(pstLegCBAgent) || pstLegCBAgent->stCall.ucLocalMode != SC_LEG_LOCAL_SIGNIN)
        {
            goto processing;
        }
    }

processing:
    /* ������ҵ�� */
    if (DOS_ADDR_VALID(pstSCBAgent))
    {
        return sc_req_hungup_with_sound(pstSCB->ulSCBNo, pstLegCB->ulCBNo, CC_ERR_SC_USER_BUSY);
    }

    stCallMsg.stMsgTag.ulMsgType = SC_CMD_CALL;
    stCallMsg.stMsgTag.ulSCBNo = pstSCB->ulSCBNo;
    stCallMsg.stMsgTag.usInterErr = 0;
    stCallMsg.ulPeerType = SC_LEG_PEER_INTERNAL_OUTBOUND;
    stCallMsg.ulSCBNo = pstSCB->ulSCBNo;
    stCallMsg.ulCodecCnt = 0;
    stCallMsg.ulTrunkCnt = 0;
    stCallMsg.szEIXAddr[0] = '\0';

    dos_snprintf(stCallMsg.stNumInfo.szOriginalCallee, sizeof(stCallMsg.stNumInfo.szCallee), pstLegCB->stCall.stNumInfo.szRealCallee);
    dos_snprintf(stCallMsg.stNumInfo.szOriginalCalling, sizeof(stCallMsg.stNumInfo.szCalling), pstLegCB->stCall.stNumInfo.szRealCalling);

    dos_snprintf(stCallMsg.stNumInfo.szRealCallee, sizeof(stCallMsg.stNumInfo.szCallee), pstLegCB->stCall.stNumInfo.szRealCallee);
    dos_snprintf(stCallMsg.stNumInfo.szRealCalling, sizeof(stCallMsg.stNumInfo.szCalling), pstLegCB->stCall.stNumInfo.szRealCalling);

    dos_snprintf(stCallMsg.stNumInfo.szCallee, sizeof(stCallMsg.stNumInfo.szCallee), pstLegCB->stCall.stNumInfo.szRealCallee);
    dos_snprintf(stCallMsg.stNumInfo.szCalling, sizeof(stCallMsg.stNumInfo.szCalling), pstLegCB->stCall.stNumInfo.szRealCalling);

    sc_send_cmd_new_call(&stCallMsg.stMsgTag);

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
        sc_log(LOG_LEVEL_ERROR, "Release call with error code %u", pstAuthRsp->stMsgTag.usInterErr);
        /* ע��ͨ��ƫ�������ҵ�CCͳһ����Ĵ����� */
        sc_req_hungup_with_sound(pstSCB->ulSCBNo, pstSCB->stCall.ulCallingLegNo, CC_ERR_BS_HEAD + pstAuthRsp->stMsgTag.usInterErr);
        return DOS_SUCC;
    }

    pstSCB->stCall.stSCBTag.usStatus = SC_CALL_EXEC;

    //if (pstAuthRsp->ucBalanceWarning)
    {
        return sc_req_play_sound(pstSCB->ulSCBNo, pstSCB->stCall.ulCallingLegNo, SC_SND_LOW_BALANCE, 1, 0, 0);
    }

    return sc_call2pstn(pstSCB, pstLegCB);
}

U32 sc_call_exchange_media(SC_MSG_TAG_ST *pstMsg, SC_SRV_CB *pstSCB)
{

    if (DOS_ADDR_INVALID(pstMsg) || DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    return DOS_SUCC;
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
            sc_log(LOG_LEVEL_INFO, "Calling has been ringback.");
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
            sc_log(LOG_LEVEL_INFO, "Calling has been answered");
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

    sc_log(LOG_LEVEL_DEBUG, "Leg %u has hunguped. ", pstHungup->ulLegNo);

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
            ulRet = sc_call2pstn(pstSCB, pstCallingLegCB);
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



#ifdef __cplusplus
}
#endif /* End of __cplusplus */


