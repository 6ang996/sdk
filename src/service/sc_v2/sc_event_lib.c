/**
 * @file : sc_event_lib.c
 *
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 * ʵ��ҵ�����ģ�鹫������
 *
 *
 * @date: 2016��1��18��
 * @arthur: Larry
 */


#ifdef __cplusplus
extern "C" {
#endif /* End of __cplusplus */

#include <dos.h>
#include "sc_def.h"
#include "sc_debug.h"
#include "sc_http_api.h"
#include "bs_pub.h"
#include "sc_pub.h"
#include "sc_res.h"
#include "sc_hint.h"

/**
 * �����ڲ����У�����һ�����ⲿ�ĺ��У��� @a pstLegCB Ϊ���У��򱻽з������
 *
 * @param pstMsg
 * @param pstLegCB
 *
 * @return �ɹ�����DOS_SUCC,ʧ�ܷ���DOS_FAIL
 */
U32 sc_outgoing_call_process(SC_SRV_CB *pstSCB, SC_LEG_CB *pstCallingLegCB)
{
    SC_MSG_CMD_CALL_ST          stCallMsg;
    SC_LEG_CB                   *pstCalleeLegCB = NULL;
    SC_AGENT_INFO_ST            *pstAgentInfo   = NULL;
    U32                         ulRet = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstSCB) || DOS_ADDR_INVALID(pstCallingLegCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    stCallMsg.stMsgTag.ulMsgType = SC_CMD_CALL;
    stCallMsg.stMsgTag.ulSCBNo = pstSCB->ulSCBNo;
    stCallMsg.stMsgTag.usInterErr = 0;
    stCallMsg.ulSCBNo = pstSCB->ulSCBNo;

    if (DOS_ADDR_VALID(pstSCB->stCall.pstAgentCalling))
    {
        pstAgentInfo = pstSCB->stCall.pstAgentCalling->pstAgentInfo;
    }

    pstCalleeLegCB = sc_lcb_alloc();
    if (DOS_ADDR_INVALID(pstCalleeLegCB))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Alloc SCB fail.");

        return sc_req_hungup_with_sound(pstSCB->ulSCBNo, pstCallingLegCB->ulCBNo, CC_ERR_SC_SYSTEM_BUSY);
    }

    stCallMsg.ulLCBNo = pstCalleeLegCB->ulCBNo;

    pstCalleeLegCB->stCall.bValid = DOS_TRUE;
    pstCalleeLegCB->stCall.ucStatus = SC_LEG_INIT;
    pstCalleeLegCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND;
    pstCalleeLegCB->ulSCBNo = pstSCB->ulSCBNo;
    if (DOS_ADDR_VALID(pstAgentInfo))
    {
        pstCalleeLegCB->stRecord.bValid = pstAgentInfo->bRecord;
    }

    pstSCB->stCall.ulCalleeLegNo = pstCalleeLegCB->ulCBNo;

    /* ά��һ�����к��룬���к�������к������л�� */
    if (DOS_ADDR_VALID(pstAgentInfo))
    {
        ulRet = sc_caller_setting_select_number(pstSCB->ulCustomerID, pstAgentInfo->ulAgentID, SC_SRC_CALLER_TYPE_AGENT, pstCallingLegCB->stCall.stNumInfo.szOriginalCalling, SC_NUM_LENGTH);
    }
    else
    {
        ulRet = sc_caller_setting_select_number(pstSCB->ulCustomerID, 0, SC_SRC_CALLER_TYPE_ALL, pstCallingLegCB->stCall.stNumInfo.szOriginalCalling, SC_NUM_LENGTH);
    }

    if (ulRet != DOS_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_HTTP_API), "Agent signin customID(%u) get caller number", pstSCB->ulCustomerID);
        sc_lcb_free(pstCalleeLegCB);

        return sc_req_hungup_with_sound(pstSCB->ulSCBNo, pstCallingLegCB->ulCBNo, CC_ERR_SC_CALLER_NUMBER_ILLEGAL);;
    }

    dos_snprintf(pstCallingLegCB->stCall.stNumInfo.szRealCalling, sizeof(pstCallingLegCB->stCall.stNumInfo.szRealCalling), pstCallingLegCB->stCall.stNumInfo.szOriginalCalling);
    dos_snprintf(pstCallingLegCB->stCall.stNumInfo.szRealCallee, sizeof(pstCallingLegCB->stCall.stNumInfo.szRealCallee), pstCallingLegCB->stCall.stNumInfo.szOriginalCallee);

    /* ��LEG����һ�º��� */
    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCallee), pstCallingLegCB->stCall.stNumInfo.szRealCallee);
    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szOriginalCalling, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCalling), pstCallingLegCB->stCall.stNumInfo.szRealCalling);

    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szRealCallee, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCallee), pstCallingLegCB->stCall.stNumInfo.szRealCallee);
    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szRealCalling, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCalling), pstCallingLegCB->stCall.stNumInfo.szRealCalling);

    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szCallee, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCallee), pstCallingLegCB->stCall.stNumInfo.szRealCallee);
    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szCalling, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCalling), pstCallingLegCB->stCall.stNumInfo.szRealCalling);

    /* ���ֺ�����Ҫ��·�� */
    pstSCB->stCall.ulRouteID = sc_route_search(pstSCB, pstCalleeLegCB->stCall.stNumInfo.szRealCalling, pstCalleeLegCB->stCall.stNumInfo.szRealCallee);
    if (U32_BUTT == pstSCB->stCall.ulRouteID)
    {
        sc_trace_scb(pstSCB, "no route to pstn.");

        sc_lcb_free(pstCalleeLegCB);

        return sc_req_hungup_with_sound(pstSCB->ulSCBNo, pstCallingLegCB->ulCBNo, CC_ERR_SC_NO_ROUTE);
    }

    /* ���û���ҵ��м̣���Ҫ�ܾ����� */
    pstCalleeLegCB->stCall.ulTrunkCnt = sc_route_get_trunks(pstSCB->stCall.ulRouteID, pstCalleeLegCB->stCall.aulTrunkList, SC_MAX_TRUCK_NUM);
    if (0 == pstCalleeLegCB->stCall.ulTrunkCnt)
    {
        sc_trace_scb(pstSCB, "no trunk to pstn. route id:%u", pstSCB->stCall.ulRouteID);

        sc_lcb_free(pstCalleeLegCB);

        return sc_req_hungup_with_sound(pstSCB->ulSCBNo, pstCallingLegCB->ulCBNo, CC_ERR_SC_NO_TRUNK);
    }

    ulRet = sc_send_cmd_new_call(&stCallMsg.stMsgTag);
    if (ulRet == DOS_SUCC)
    {
        pstSCB->stCall.stSCBTag.usStatus = SC_CALL_EXEC;

        if (DOS_ADDR_VALID(pstAgentInfo))
        {
            /* �޸���ϯ�Ĺ���״̬ */
            pthread_mutex_lock(&pstAgentInfo->mutexLock);
            sc_agent_set_ringback(pstAgentInfo);
            pstAgentInfo->ulLegNo = pstSCB->stCall.ulCallingLegNo;
            dos_snprintf(pstAgentInfo->szLastCustomerNum, SC_NUM_LENGTH, "%s", pstCallingLegCB->stCall.stNumInfo.szOriginalCallee);
            pthread_mutex_unlock(&pstAgentInfo->mutexLock);
            /* �������� */
            sc_agent_call_notify(pstAgentInfo, pstCallingLegCB->stCall.stNumInfo.szOriginalCallee);
        }
    }

    return ulRet;
}

/**
 * ���н���������һ���ڲ���SIP�˻����� @a pstLegCB Ϊ���У��򱻽з������
 *
 * @param pstMsg
 * @param pstLegCB
 *
 * @return �ɹ�����DOS_SUCC,ʧ�ܷ���DOS_FAIL
 */
U32 sc_internal_call_process(SC_SRV_CB *pstSCB, SC_LEG_CB *pstLegCB)
{
    SC_AGENT_NODE_ST *pstAgent    = NULL;
    SC_MSG_CMD_CALL_ST          stCallMsg;
    SC_SRV_CB                  *pstSCBAgent = NULL;
    SC_LEG_CB                  *pstLegCBAgent = NULL;
    SC_LEG_CB                  *pstCalleeLeg = NULL;
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

    pstAgent = sc_agent_get_by_sip_acc(pstLegCB->stCall.stNumInfo.szRealCallee);
    if (DOS_ADDR_INVALID(pstAgent)
        || DOS_ADDR_INVALID(pstAgent->pstAgentInfo))
    {
        goto processing;
    }

    if (pstAgent->pstAgentInfo->ulLegNo < SC_LEG_CB_SIZE)
    {
        pstLegCBAgent = sc_lcb_get(pstAgent->pstAgentInfo->ulLegNo);
        if (DOS_ADDR_INVALID(pstLegCBAgent) || pstLegCBAgent->stCall.ucLocalMode != SC_LEG_LOCAL_SIGNIN)
        {
            goto processing;
        }

        pstSCBAgent = sc_scb_get(pstLegCBAgent->ulSCBNo);
        if (DOS_ADDR_INVALID(pstSCBAgent))
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

    if (DOS_ADDR_VALID(pstLegCBAgent))
    {
        pstSCBAgent = sc_scb_get(pstLegCBAgent->ulIndSCBNo);
        if (DOS_ADDR_VALID(pstSCBAgent))
        {
            pstSCB->stCall.stSCBTag.usStatus = SC_CALL_TONE;

            pstLegCBAgent->ulSCBNo = pstSCB->ulSCBNo;
            pstSCB->stCall.ulCalleeLegNo = pstLegCBAgent->ulCBNo;
            pstSCB->stCall.pstAgentCallee = pstSCBAgent->stSigin.pstAgentNode;
            /* ��ϯ��ǩ */
            sc_req_playback_stop(pstSCB->ulSCBNo, pstLegCBAgent->ulCBNo);
            sc_req_play_sound(pstSCB->ulSCBNo, pstSCBAgent->stSigin.ulLegNo, SC_SND_INCOMING_CALL_TIP, 1, 0, 0);

            /* �Ż����������� */
            sc_req_ringback(pstSCB->ulSCBNo, pstSCB->stCall.ulCallingLegNo, DOS_TRUE);

            if (DOS_ADDR_VALID(pstSCB->stCall.pstAgentCallee)
                && DOS_ADDR_VALID(pstSCB->stCall.pstAgentCallee->pstAgentInfo))
            {
                sc_agent_set_ringing(pstSCB->stCall.pstAgentCallee->pstAgentInfo);
            }

            return DOS_SUCC;
        }
    }

    pstCalleeLeg = sc_lcb_alloc();
    if (DOS_ADDR_INVALID(pstCalleeLeg))
    {
        return sc_req_hungup_with_sound(pstSCB->ulSCBNo, pstLegCB->ulCBNo, CC_ERR_SC_SYSTEM_BUSY);
    }

    pstCalleeLeg->ulSCBNo = pstSCB->ulSCBNo;
    pstCalleeLeg->stCall.bValid = DOS_TRUE;
    pstCalleeLeg->stCall.ucStatus = SC_LEG_INIT;
    pstCalleeLeg->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND_INTERNAL;
    pstCalleeLeg->stCall.ulCodecCnt = 0;
    pstCalleeLeg->stCall.ulTrunkCnt = 0;
    pstCalleeLeg->stCall.szEIXAddr[0] = '\0';

    dos_snprintf(pstCalleeLeg->stCall.stNumInfo.szOriginalCallee, sizeof(pstCalleeLeg->stCall.stNumInfo.szCallee), pstLegCB->stCall.stNumInfo.szRealCallee);
    dos_snprintf(pstCalleeLeg->stCall.stNumInfo.szOriginalCalling, sizeof(pstCalleeLeg->stCall.stNumInfo.szCalling), pstLegCB->stCall.stNumInfo.szRealCalling);

    dos_snprintf(pstCalleeLeg->stCall.stNumInfo.szRealCallee, sizeof(pstCalleeLeg->stCall.stNumInfo.szCallee), pstLegCB->stCall.stNumInfo.szRealCallee);
    dos_snprintf(pstCalleeLeg->stCall.stNumInfo.szRealCalling, sizeof(pstCalleeLeg->stCall.stNumInfo.szCalling), pstLegCB->stCall.stNumInfo.szRealCalling);

    dos_snprintf(pstCalleeLeg->stCall.stNumInfo.szCallee, sizeof(pstCalleeLeg->stCall.stNumInfo.szCallee), pstLegCB->stCall.stNumInfo.szRealCallee);
    dos_snprintf(pstCalleeLeg->stCall.stNumInfo.szCalling, sizeof(pstCalleeLeg->stCall.stNumInfo.szCalling), pstLegCB->stCall.stNumInfo.szRealCalling);

    stCallMsg.stMsgTag.ulMsgType = SC_CMD_CALL;
    stCallMsg.stMsgTag.ulSCBNo = pstSCB->ulSCBNo;
    stCallMsg.stMsgTag.usInterErr = 0;
    stCallMsg.ulSCBNo = pstSCB->ulSCBNo;
    stCallMsg.ulLCBNo = pstCalleeLeg->ulCBNo;

    if (sc_send_cmd_new_call(&stCallMsg.stMsgTag) != DOS_SUCC)
    {
        sc_lcb_free(pstCalleeLeg);
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Send new call request fail.");
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

static U32 sc_incoming_call_sip_proc(SC_SRV_CB *pstSCB, SC_LEG_CB *pstCallingLegCB, U32 ulSipID, U32 *pulErrCode)
{
    S8    szCallee[32] = { 0, };
    SC_AGENT_NODE_ST *pstAgentNode = NULL;
    SC_LEG_CB *pstCalleeLegCB = NULL;
    SC_MSG_CMD_CALL_ST      stCallMsg;
    SC_SRV_CB *pstIndSCB = NULL;

    if (DOS_SUCC != sc_sip_account_get_by_id(ulSipID, szCallee, sizeof(szCallee)))
    {
        DOS_ASSERT(0);

        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "DID number %s seems donot bind a SIP User ID, Reject Call.", pstCallingLegCB->stCall.stNumInfo.szOriginalCallee);
        *pulErrCode = CC_ERR_SC_CALLEE_NUMBER_ILLEGAL;

        return DOS_FAIL;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Start find agent by userid(%s)", szCallee);
    pstAgentNode = sc_agent_get_by_sip_acc(szCallee);
    if (DOS_ADDR_VALID(pstAgentNode))
    {
        pstSCB->stCall.pstAgentCallee = pstAgentNode;
        /* ������ϯ���ж��Ƿ�¼������ϯ�Ƿ�ǩ����ϯ���� */
        if (pstAgentNode->pstAgentInfo->bRecord)
        {
            pstCallingLegCB->stRecord.bValid = DOS_TRUE;
        }

        dos_snprintf(pstAgentNode->pstAgentInfo->szLastCustomerNum, SC_NUM_LENGTH, "%s", pstCallingLegCB->stCall.stNumInfo.szOriginalCalling);

        /* �������� */
        sc_agent_call_notify(pstAgentNode->pstAgentInfo, pstCallingLegCB->stCall.stNumInfo.szOriginalCalling);

        if (pstAgentNode->pstAgentInfo->bConnected
            && pstAgentNode->pstAgentInfo->ucBindType == AGENT_BIND_SIP)
        {
            pstCalleeLegCB = sc_lcb_get(pstAgentNode->pstAgentInfo->ulLegNo);
            if (DOS_ADDR_VALID(pstCalleeLegCB))
            {
                pstSCB->stCall.ulCalleeLegNo = pstAgentNode->pstAgentInfo->ulLegNo;
                pstCalleeLegCB->ulSCBNo = pstSCB->ulSCBNo;
                sc_req_playback_stop(pstSCB->ulSCBNo, pstCalleeLegCB->ulCBNo);
                /* ����ʾ������ϯ */
                pstIndSCB = sc_scb_get(pstCalleeLegCB->ulIndSCBNo);
                if (DOS_ADDR_VALID(pstIndSCB))
                {
                    sc_req_play_sound(pstSCB->ulSCBNo, pstIndSCB->stSigin.ulLegNo, SC_SND_INCOMING_CALL_TIP, 1, 0, 0);
                }

                /* �Ż����������� */
                pstSCB->stCall.stSCBTag.usStatus = SC_CALL_TONE;

                return DOS_SUCC;
            }
        }
    }

    /* ����һ���µ�leg��������� */
    stCallMsg.stMsgTag.ulMsgType = SC_CMD_CALL;
    stCallMsg.stMsgTag.ulSCBNo = pstSCB->ulSCBNo;
    stCallMsg.stMsgTag.usInterErr = 0;
    stCallMsg.ulSCBNo = pstSCB->ulSCBNo;

    pstCalleeLegCB = sc_lcb_alloc();
    if (DOS_ADDR_INVALID(pstCalleeLegCB))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Alloc SCB fail.");

        return sc_req_hungup_with_sound(pstSCB->ulSCBNo, pstCallingLegCB->ulCBNo, CC_ERR_SC_SYSTEM_BUSY);
    }

    stCallMsg.ulLCBNo = pstCalleeLegCB->ulCBNo;

    pstCalleeLegCB->stCall.bValid = DOS_TRUE;
    pstCalleeLegCB->stCall.ucStatus = SC_LEG_INIT;
    pstCalleeLegCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND_INTERNAL;
    pstCalleeLegCB->ulSCBNo = pstSCB->ulSCBNo;

    pstSCB->stCall.ulCalleeLegNo = pstCalleeLegCB->ulCBNo;

    /* ά��һ�����к��� */
    dos_snprintf(pstCallingLegCB->stCall.stNumInfo.szRealCalling, sizeof(pstCallingLegCB->stCall.stNumInfo.szRealCalling), pstCallingLegCB->stCall.stNumInfo.szOriginalCalling);
    dos_snprintf(pstCallingLegCB->stCall.stNumInfo.szRealCallee, sizeof(pstCallingLegCB->stCall.stNumInfo.szRealCallee), pstCallingLegCB->stCall.stNumInfo.szOriginalCallee);

    /* ��LEG����һ�º��� */
    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCallee), pstCallingLegCB->stCall.stNumInfo.szOriginalCallee);
    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szOriginalCalling, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCalling), pstCallingLegCB->stCall.stNumInfo.szOriginalCalling);

    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szRealCallee, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCallee), szCallee);
    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szRealCalling, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCalling), pstCallingLegCB->stCall.stNumInfo.szRealCalling);

    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szCallee, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCallee), szCallee);
    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szCalling, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCalling), pstCallingLegCB->stCall.stNumInfo.szRealCalling);

    if (sc_send_cmd_new_call(&stCallMsg.stMsgTag) != DOS_SUCC)
    {
        *pulErrCode = CC_ERR_SC_MESSAGE_SENT_ERR;

        goto proc_fail;
    }

    pstSCB->stCall.stSCBTag.usStatus = SC_CALL_EXEC;

    return DOS_SUCC;

proc_fail:

    return DOS_FAIL;
}

U32 sc_agent_call_by_id(SC_SRV_CB *pstSCB, SC_LEG_CB *pstCallingLegCB, U32 ulAgentID, U32 *pulErrCode)
{
    SC_AGENT_NODE_ST    *pstAgentNode   = NULL;
    SC_LEG_CB           *pstCalleeLegCB = NULL;
    SC_SRV_CB           *pstIndSCB      = NULL;
    SC_MSG_CMD_CALL_ST stCallMsg;
    S8 szCallee[SC_NUM_LENGTH] = {0,};

    if (DOS_ADDR_INVALID(pstSCB)
        || DOS_ADDR_INVALID(pstCallingLegCB))
    {
        return DOS_FAIL;
    }

    pstAgentNode = sc_agent_get_by_id(ulAgentID);
    if (DOS_ADDR_INVALID(pstAgentNode))
    {
        /* û���ҵ���ϯ */
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_ACD), "Not found agnet by id %u", ulAgentID);

        return DOS_FAIL;
    }

    /* �ж���ϯ��״̬ */
    if (pstAgentNode->pstAgentInfo->ucWorkStatus != SC_ACD_WORK_IDEL
        || pstAgentNode->pstAgentInfo->ucServStatus != SC_ACD_SERV_IDEL)
    {
        /* ��������� */
        if (pstAgentNode->pstAgentInfo->ucWorkStatus != SC_ACD_WORK_IDEL)
        {
            *pulErrCode = CC_ERR_SC_USER_HAS_BEEN_LEFT;
        }
        else
        {
            *pulErrCode = CC_ERR_SC_USER_BUSY;
        }

        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_ACD), "Call agnet FAIL. Agent (%u) work_status : %d, serv_status : %d"
            , pstAgentNode->pstAgentInfo->ulAgentID, pstAgentNode->pstAgentInfo->ucWorkStatus, pstAgentNode->pstAgentInfo->ucServStatus);

        return DOS_FAIL;
    }

    /* �Ƿ���Ҫ¼�� */
    if (pstAgentNode->pstAgentInfo->bRecord)
    {
        pstCallingLegCB->stRecord.bValid = DOS_TRUE;
        pstCallingLegCB->stRecord.usStatus = SC_SU_RECORD_INIT;
    }

    dos_snprintf(pstAgentNode->pstAgentInfo->szLastCustomerNum, SC_NUM_LENGTH, "%s", pstCallingLegCB->stCall.stNumInfo.szOriginalCalling);

    /* �������� */
    sc_agent_call_notify(pstAgentNode->pstAgentInfo, pstCallingLegCB->stCall.stNumInfo.szOriginalCalling);

    pstSCB->stCall.pstAgentCallee = pstAgentNode;

    /* �ж���ϯ�Ƿ�ǩ */
    if (pstAgentNode->pstAgentInfo->bConnected)
    {
        pstCalleeLegCB = sc_lcb_get(pstAgentNode->pstAgentInfo->ulLegNo);
        if (DOS_ADDR_VALID(pstCalleeLegCB)
            && pstCalleeLegCB->ulIndSCBNo != U32_BUTT)
        {
            /* ��ǩ */
            pstSCB->stCall.ulCalleeLegNo = pstAgentNode->pstAgentInfo->ulLegNo;
            pstCalleeLegCB->ulSCBNo = pstSCB->ulSCBNo;
            sc_req_playback_stop(pstSCB->ulSCBNo, pstCalleeLegCB->ulCBNo);
            pstIndSCB = sc_scb_get(pstCalleeLegCB->ulIndSCBNo);
            if (DOS_ADDR_VALID(pstIndSCB))
            {
                /* ����ʾ������ϯ */
                sc_req_play_sound(pstSCB->ulSCBNo, pstIndSCB->stSigin.ulLegNo, SC_SND_INCOMING_CALL_TIP, 1, 0, 0);
            }

            /* �Ż����������� */
            sc_req_ringback(pstSCB->ulSCBNo, pstSCB->stCall.ulCallingLegNo, DOS_TRUE);

            pstSCB->stCall.stSCBTag.usStatus = SC_CALL_TONE;

            /* �޸���ϯ��״̬����æ */
            sc_agent_set_ringing(pstAgentNode->pstAgentInfo);

            return DOS_SUCC;
        }
    }

    /* ����һ���µ�leg��������� */
    pstCalleeLegCB = sc_lcb_alloc();
    if (DOS_ADDR_INVALID(pstCalleeLegCB))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_ACD), "Alloc SCB fail.");
        *pulErrCode = CC_ERR_SC_SYSTEM_BUSY;

        return DOS_FAIL;
    }

    pstCalleeLegCB->stCall.bValid = DOS_TRUE;
    pstCalleeLegCB->stCall.ucStatus = SC_LEG_INIT;
    pstCalleeLegCB->ulSCBNo = pstSCB->ulSCBNo;

    pstSCB->stCall.ulCalleeLegNo = pstCalleeLegCB->ulCBNo;

    switch (pstAgentNode->pstAgentInfo->ucBindType)
    {
        case AGENT_BIND_SIP:
            dos_snprintf(szCallee, sizeof(szCallee), pstAgentNode->pstAgentInfo->szUserID);
            pstCalleeLegCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND_INTERNAL;
            break;

        case AGENT_BIND_TELE:
            dos_snprintf(szCallee, sizeof(szCallee), pstAgentNode->pstAgentInfo->szTelePhone);
            pstCalleeLegCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND;
            if (sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL))
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Add outbound service fail.");
                *pulErrCode = CC_ERR_SC_SYSTEM_ABNORMAL;
                goto process_fail;
            }
            break;

        case AGENT_BIND_MOBILE:
            dos_snprintf(szCallee, sizeof(szCallee), pstAgentNode->pstAgentInfo->szMobile);
            pstCalleeLegCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND;
            if (sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL))
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Add outbound service fail.");
                *pulErrCode = CC_ERR_SC_SYSTEM_ABNORMAL;
                goto process_fail;
            }
            break;

        case AGENT_BIND_TT_NUMBER:
            dos_snprintf(szCallee, sizeof(szCallee), pstAgentNode->pstAgentInfo->szTTNumber);
            pstCalleeLegCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND_TT;
            break;

        default:
            break;
    }

    /* ά��һ�����к��� */
    dos_snprintf(pstCallingLegCB->stCall.stNumInfo.szRealCalling, sizeof(pstCallingLegCB->stCall.stNumInfo.szRealCalling), pstCallingLegCB->stCall.stNumInfo.szOriginalCalling);
    dos_snprintf(pstCallingLegCB->stCall.stNumInfo.szRealCallee, sizeof(pstCallingLegCB->stCall.stNumInfo.szRealCallee), szCallee);

    /* ��LEG����һ�º��� */
    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCallee), pstCallingLegCB->stCall.stNumInfo.szOriginalCallee);
    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szOriginalCalling, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCalling), pstCallingLegCB->stCall.stNumInfo.szOriginalCalling);

    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szRealCallee, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCallee), szCallee);
    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szRealCalling, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCalling), pstCallingLegCB->stCall.stNumInfo.szRealCalling);

    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szCallee, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCallee), szCallee);
    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szCalling, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCalling), pstCallingLegCB->stCall.stNumInfo.szRealCalling);

    /* �޸���ϯ״̬ */
    sc_agent_set_ringing(pstAgentNode->pstAgentInfo);

    if (pstCalleeLegCB->stCall.ucPeerType == SC_LEG_PEER_OUTBOUND)
    {
        /* ������Ҫȥ��֤ */
        if (sc_send_usr_auth2bs(pstSCB, pstCalleeLegCB) != DOS_SUCC)
        {
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Send auth fail.");

            goto process_fail;
        }
        pstSCB->stCall.stSCBTag.usStatus = SC_CALL_AUTH2;

        return DOS_SUCC;
    }

    stCallMsg.stMsgTag.ulMsgType = SC_CMD_CALL;
    stCallMsg.stMsgTag.ulSCBNo = pstSCB->ulSCBNo;
    stCallMsg.stMsgTag.usInterErr = 0;
    stCallMsg.ulSCBNo = pstSCB->ulSCBNo;
    stCallMsg.ulLCBNo = pstCalleeLegCB->ulCBNo;

    if (sc_send_cmd_new_call(&stCallMsg.stMsgTag) != DOS_SUCC)
    {
        *pulErrCode = CC_ERR_SC_MESSAGE_SENT_ERR;

        return DOS_FAIL;
    }

    pstSCB->stCall.stSCBTag.usStatus = SC_CALL_EXEC;

    return DOS_SUCC;

process_fail:
    /* �ڵ��ú����ĵط�����ʧ�ܴ��� */
    return DOS_FAIL;
}

U32 sc_agent_auto_callback(SC_SRV_CB *pstSCB, SC_AGENT_NODE_ST *pstAgentNode)
{
    SC_LEG_CB *pstCallingLegCB = NULL;
    SC_LEG_CB *pstCalleeLegCB = NULL;
    SC_MSG_CMD_CALL_ST      stCallMsg;
    U32 ulErrCode = CC_ERR_NO_REASON;
    S8 szCallee[SC_NUM_LENGTH] = {0,};
    SC_SRV_CB *pstIndSCB = NULL;

    if (DOS_ADDR_INVALID(pstSCB)
        || DOS_ADDR_INVALID(pstAgentNode))
    {
        return DOS_FAIL;
    }

    pstCallingLegCB = sc_lcb_get(pstSCB->stAutoCall.ulCallingLegNo);
    if (DOS_ADDR_INVALID(pstCallingLegCB))
    {
        return DOS_FAIL;
    }

    /* �ж��޸���ϯ��״̬ */
    pstSCB->stAutoCall.ulAgentID = pstAgentNode->pstAgentInfo->ulAgentID;
    pstAgentNode->pstAgentInfo->bSelected = DOS_FALSE;
    sc_agent_set_ringing(pstAgentNode->pstAgentInfo);

    dos_snprintf(pstAgentNode->pstAgentInfo->szLastCustomerNum, SC_NUM_LENGTH, "%s", pstCallingLegCB->stCall.stNumInfo.szOriginalCallee);

    /* ��ϯ���� */
    sc_agent_call_notify(pstAgentNode->pstAgentInfo, pstCallingLegCB->stCall.stNumInfo.szOriginalCallee);

    /* �Ƿ���Ҫ¼�� */
    if (pstAgentNode->pstAgentInfo->bRecord)
    {
        pstCallingLegCB->stRecord.bValid = DOS_TRUE;
    }

    /* �ж���ϯ�Ƿ�ǩ */
    if (pstAgentNode->pstAgentInfo->bConnected)
    {
        pstCalleeLegCB = sc_lcb_get(pstAgentNode->pstAgentInfo->ulLegNo);
        if (DOS_ADDR_VALID(pstCalleeLegCB)
            && pstCalleeLegCB->ulIndSCBNo != U32_BUTT)
        {
            pstSCB->stAutoCall.ulCalleeLegNo = pstAgentNode->pstAgentInfo->ulLegNo;
            pstCalleeLegCB->ulSCBNo = pstSCB->ulSCBNo;
            /* ����ʾ������ϯ */
            sc_req_playback_stop(pstSCB->ulSCBNo, pstCalleeLegCB->ulCBNo);
            pstIndSCB = sc_scb_get(pstCalleeLegCB->ulIndSCBNo);
            if (DOS_ADDR_VALID(pstIndSCB))
            {
                sc_req_play_sound(pstIndSCB->ulSCBNo, pstIndSCB->stSigin.ulLegNo, SC_SND_INCOMING_CALL_TIP, 1, 0, 0);
            }

            pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_TONE;

            return DOS_SUCC;
        }
    }
    /* ����һ���µ�leg��������� */
    stCallMsg.stMsgTag.ulMsgType = SC_CMD_CALL;
    stCallMsg.stMsgTag.ulSCBNo = pstSCB->ulSCBNo;
    stCallMsg.stMsgTag.usInterErr = 0;
    stCallMsg.ulSCBNo = pstSCB->ulSCBNo;

    pstCalleeLegCB = sc_lcb_alloc();
    if (DOS_ADDR_INVALID(pstCalleeLegCB))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Alloc SCB fail.");
        ulErrCode = CC_ERR_SC_SYSTEM_BUSY;

        goto process_fail;
    }

    stCallMsg.ulLCBNo = pstCalleeLegCB->ulCBNo;

    pstCalleeLegCB->stCall.bValid = DOS_TRUE;
    pstCalleeLegCB->stCall.ucStatus = SC_LEG_INIT;
    //pstCalleeLegCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND_INTERNAL;
    pstCalleeLegCB->ulSCBNo = pstSCB->ulSCBNo;

    pstSCB->stAutoCall.ulCalleeLegNo = pstCalleeLegCB->ulCBNo;

    switch (pstAgentNode->pstAgentInfo->ucBindType)
    {
        case AGENT_BIND_SIP:
            dos_snprintf(szCallee, sizeof(szCallee), pstAgentNode->pstAgentInfo->szUserID);
            pstCalleeLegCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND_INTERNAL;
            break;

        case AGENT_BIND_TELE:
            dos_snprintf(szCallee, sizeof(szCallee), pstAgentNode->pstAgentInfo->szTelePhone);
            pstCalleeLegCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND;
            if (sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL))
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Add outbound service fail.");
                ulErrCode = CC_ERR_SC_SYSTEM_ABNORMAL;
                goto process_fail;
            }
            break;

        case AGENT_BIND_MOBILE:
            dos_snprintf(szCallee, sizeof(szCallee), pstAgentNode->pstAgentInfo->szMobile);
            pstCalleeLegCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND;
            if (sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL))
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Add outbound service fail.");
                ulErrCode = CC_ERR_SC_SYSTEM_ABNORMAL;
                goto process_fail;
            }
            break;

        case AGENT_BIND_TT_NUMBER:
            dos_snprintf(szCallee, sizeof(szCallee), pstAgentNode->pstAgentInfo->szTTNumber);
            pstCalleeLegCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND_TT;
            break;

        default:
            break;
    }

    /* ��LEG����һ�º��� */
    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCallee), szCallee);
    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szOriginalCalling, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCalling), pstCallingLegCB->stCall.stNumInfo.szOriginalCallee);

    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szRealCallee, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCallee), szCallee);
    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szRealCalling, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCalling), pstCallingLegCB->stCall.stNumInfo.szRealCallee);

    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szCallee, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCallee), szCallee);
    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szCalling, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCalling), pstCallingLegCB->stCall.stNumInfo.szRealCallee);

    if (pstCalleeLegCB->stCall.ucPeerType == SC_LEG_PEER_OUTBOUND)
    {
        /* ������Ҫȥ��֤ */
        if (sc_send_usr_auth2bs(pstSCB, pstCalleeLegCB) != DOS_SUCC)
        {
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Send auth fail.");

            goto process_fail;
        }
        pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_AUTH2;

        return DOS_SUCC;
    }

    if (sc_send_cmd_new_call(&stCallMsg.stMsgTag) != DOS_SUCC)
    {
        ulErrCode = CC_ERR_SC_MESSAGE_SENT_ERR;

        return DOS_FAIL;
    }

    pstSCB->stAutoCall.stSCBTag.usStatus = SC_AUTO_CALL_EXEC2;

    return DOS_SUCC;

process_fail:

    sc_req_hungup_with_sound(pstSCB->ulSCBNo, pstCallingLegCB->ulCBNo, ulErrCode);
    return DOS_FAIL;
}

U32 sc_demo_task_callback(SC_SRV_CB *pstSCB, SC_AGENT_NODE_ST *pstAgentNode)
{
    SC_LEG_CB *pstCallingLegCB = NULL;
    SC_LEG_CB *pstCalleeLegCB = NULL;
    SC_MSG_CMD_CALL_ST      stCallMsg;
    U32 ulErrCode = CC_ERR_NO_REASON;
    S8 szCallee[SC_NUM_LENGTH] = {0,};
    SC_SRV_CB *pstIndSCB = NULL;

    if (DOS_ADDR_INVALID(pstSCB)
        || DOS_ADDR_INVALID(pstAgentNode))
    {
        return DOS_FAIL;
    }

    pstCallingLegCB = sc_lcb_get(pstSCB->stDemoTask.ulCallingLegNo);
    if (DOS_ADDR_INVALID(pstCallingLegCB))
    {
        return DOS_FAIL;
    }

    /* �ж��޸���ϯ��״̬ */
    pstSCB->stDemoTask.ulAgentID = pstAgentNode->pstAgentInfo->ulAgentID;
    pstAgentNode->pstAgentInfo->bSelected = DOS_FALSE;
    sc_agent_set_ringing(pstAgentNode->pstAgentInfo);

    dos_snprintf(pstAgentNode->pstAgentInfo->szLastCustomerNum, SC_NUM_LENGTH, "%s", pstCallingLegCB->stCall.stNumInfo.szOriginalCallee);

    /* ��ϯ���� */
    sc_agent_call_notify(pstAgentNode->pstAgentInfo, pstCallingLegCB->stCall.stNumInfo.szOriginalCallee);

    /* �Ƿ���Ҫ¼�� */
    if (pstAgentNode->pstAgentInfo->bRecord)
    {
        pstCallingLegCB->stRecord.bValid = DOS_TRUE;
    }

    /* �ж���ϯ�Ƿ�ǩ */
    if (pstAgentNode->pstAgentInfo->bConnected)
    {
        pstCalleeLegCB = sc_lcb_get(pstAgentNode->pstAgentInfo->ulLegNo);
        if (DOS_ADDR_VALID(pstCalleeLegCB)
            && pstCalleeLegCB->ulIndSCBNo != U32_BUTT)
        {
            pstSCB->stDemoTask.ulCalleeLegNo = pstAgentNode->pstAgentInfo->ulLegNo;
            pstCalleeLegCB->ulSCBNo = pstSCB->ulSCBNo;
            /* ����ʾ������ϯ */
            sc_req_playback_stop(pstSCB->ulSCBNo, pstCalleeLegCB->ulCBNo);
            pstIndSCB = sc_scb_get(pstCalleeLegCB->ulIndSCBNo);
            if (DOS_ADDR_VALID(pstIndSCB))
            {
                sc_req_play_sound(pstIndSCB->ulSCBNo, pstIndSCB->stSigin.ulLegNo, SC_SND_INCOMING_CALL_TIP, 1, 0, 0);
            }

            pstSCB->stDemoTask.stSCBTag.usStatus = SC_AUTO_CALL_TONE;

            return DOS_SUCC;
        }
    }
    /* ����һ���µ�leg��������� */
    stCallMsg.stMsgTag.ulMsgType = SC_CMD_CALL;
    stCallMsg.stMsgTag.ulSCBNo = pstSCB->ulSCBNo;
    stCallMsg.stMsgTag.usInterErr = 0;
    stCallMsg.ulSCBNo = pstSCB->ulSCBNo;

    pstCalleeLegCB = sc_lcb_alloc();
    if (DOS_ADDR_INVALID(pstCalleeLegCB))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Alloc SCB fail.");
        ulErrCode = CC_ERR_SC_SYSTEM_BUSY;

        goto process_fail;
    }

    stCallMsg.ulLCBNo = pstCalleeLegCB->ulCBNo;

    pstCalleeLegCB->stCall.bValid = DOS_TRUE;
    pstCalleeLegCB->stCall.ucStatus = SC_LEG_INIT;
    //pstCalleeLegCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND_INTERNAL;
    pstCalleeLegCB->ulSCBNo = pstSCB->ulSCBNo;

    pstSCB->stDemoTask.ulCalleeLegNo = pstCalleeLegCB->ulCBNo;

    switch (pstAgentNode->pstAgentInfo->ucBindType)
    {
        case AGENT_BIND_SIP:
            dos_snprintf(szCallee, sizeof(szCallee), pstAgentNode->pstAgentInfo->szUserID);
            pstCalleeLegCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND_INTERNAL;
            break;

        case AGENT_BIND_TELE:
            dos_snprintf(szCallee, sizeof(szCallee), pstAgentNode->pstAgentInfo->szTelePhone);
            pstCalleeLegCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND;
            if (sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL))
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Add outbound service fail.");
                ulErrCode = CC_ERR_SC_SYSTEM_ABNORMAL;
                goto process_fail;
            }
            break;

        case AGENT_BIND_MOBILE:
            dos_snprintf(szCallee, sizeof(szCallee), pstAgentNode->pstAgentInfo->szMobile);
            pstCalleeLegCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND;
            if (sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL))
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Add outbound service fail.");
                ulErrCode = CC_ERR_SC_SYSTEM_ABNORMAL;
                goto process_fail;
            }
            break;

        case AGENT_BIND_TT_NUMBER:
            dos_snprintf(szCallee, sizeof(szCallee), pstAgentNode->pstAgentInfo->szTTNumber);
            pstCalleeLegCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND_TT;
            break;

        default:
            break;
    }

    /* ��LEG����һ�º��� */
    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCallee), szCallee);
    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szOriginalCalling, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCalling), pstCallingLegCB->stCall.stNumInfo.szOriginalCallee);

    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szRealCallee, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCallee), szCallee);
    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szRealCalling, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCalling), pstCallingLegCB->stCall.stNumInfo.szRealCallee);

    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szCallee, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCallee), szCallee);
    dos_snprintf(pstCalleeLegCB->stCall.stNumInfo.szCalling, sizeof(pstCalleeLegCB->stCall.stNumInfo.szCalling), pstCallingLegCB->stCall.stNumInfo.szRealCallee);

    if (pstCalleeLegCB->stCall.ucPeerType == SC_LEG_PEER_OUTBOUND)
    {
        /* ������Ҫȥ��֤ */
        if (sc_send_usr_auth2bs(pstSCB, pstCalleeLegCB) != DOS_SUCC)
        {
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Send auth fail.");

            goto process_fail;
        }
        pstSCB->stDemoTask.stSCBTag.usStatus = SC_AUTO_CALL_AUTH2;

        return DOS_SUCC;
    }

    if (sc_send_cmd_new_call(&stCallMsg.stMsgTag) != DOS_SUCC)
    {
        ulErrCode = CC_ERR_SC_MESSAGE_SENT_ERR;

        return DOS_FAIL;
    }

    pstSCB->stDemoTask.stSCBTag.usStatus = SC_AUTO_CALL_EXEC2;

    return DOS_SUCC;

process_fail:

    sc_req_hungup_with_sound(pstSCB->ulSCBNo, pstCallingLegCB->ulCBNo, ulErrCode);
    return DOS_FAIL;
}


/**
 * ����: U32 sc_ep_incoming_call_proc(esl_handle_t *pstHandle, esl_event_t *pstEvent, SC_SCB_ST *pstSCB)
 * ����: ������PSTN���뵽SIP��ĺ���
 * ����:
 *      esl_event_t *pstEvent   : ESL �¼�
 *      esl_handle_t *pstHandle : �������ݵ�handle
 *      SC_SCB_ST *pstSCB       : ҵ����ƿ�
 * ����ֵ: �ɹ�����DOS_SUCC,ʧ�ܷ���DOS_FAIL
 */
U32 sc_incoming_call_proc(SC_SRV_CB *pstSCB, SC_LEG_CB *pstCallingLegCB)
{
    U32   ulCustomerID = U32_BUTT;
    U32   ulBindType = U32_BUTT;
    U32   ulBindID = U32_BUTT;
    U32   ulErrCode = CC_ERR_NO_REASON;
    U32  ulRet = DOS_FAIL;

    if (DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);

        goto proc_finished;
    }

    ulCustomerID = pstSCB->ulCustomerID;
    if (U32_BUTT != ulCustomerID)
    {
        if (sc_did_bind_info_get(pstCallingLegCB->stCall.stNumInfo.szOriginalCallee, &ulBindType, &ulBindID) != DOS_SUCC
            || ulBindType >=SC_DID_BIND_TYPE_BUTT
            || U32_BUTT == ulBindID)
        {
            DOS_ASSERT(0);

            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Cannot get the bind info for the DID number %s, Reject Call.", pstCallingLegCB->stCall.stNumInfo.szOriginalCallee);
            ulErrCode = CC_ERR_SC_CALLEE_NUMBER_ILLEGAL;
            goto proc_finished;
        }

        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Process Incoming Call, DID Number %s. Bind Type: %u, Bind ID: %u"
                        , pstCallingLegCB->stCall.stNumInfo.szOriginalCallee
                        , ulBindType
                        , ulBindID);

        switch (ulBindType)
        {
            case SC_DID_BIND_TYPE_SIP:
                /* ��sip�ֻ�ʱ�����ж���ϯ��״̬, ���ı���ϯ��״̬, ֻ���ж�һ���Ƿ���Ҫ¼�� */
                ulRet = sc_incoming_call_sip_proc(pstSCB, pstCallingLegCB, ulBindID, &ulErrCode);
                break;

            case SC_DID_BIND_TYPE_QUEUE:
                /* ����������ж���ҵ����ƿ� */
                pstSCB->stIncomingQueue.stSCBTag.bValid = DOS_TRUE;
                pstSCB->ulCurrentSrv++;
                pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stIncomingQueue.stSCBTag;
                pstSCB->stIncomingQueue.ulEnqueuTime = time(NULL);
                pstSCB->stIncomingQueue.ulLegNo = pstSCB->stCall.ulCallingLegNo;
                pstSCB->stIncomingQueue.stSCBTag.usStatus = SC_INQUEUE_IDEL;
                if (sc_cwq_add_call(pstSCB, ulBindID, pstCallingLegCB->stCall.stNumInfo.szRealCallee) != DOS_SUCC)
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
            case SC_DID_BIND_TYPE_AGENT:
                /* ������ϯ */
                ulRet = sc_agent_call_by_id(pstSCB, pstCallingLegCB, ulBindID, &ulErrCode);
                break;

            default:
                DOS_ASSERT(0);
                ulErrCode = CC_ERR_SC_CONFIG_ERR;
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "DID number %s has bind an error number, Reject Call.", pstCallingLegCB->stCall.stNumInfo.szOriginalCallee);
                goto proc_finished;
        }
    }
    else
    {
        DOS_ASSERT(0);
        ulErrCode = CC_ERR_SC_CALLEE_NUMBER_ILLEGAL;
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Destination is not a DID number, Reject Call. Destination: %s", pstCallingLegCB->stCall.stNumInfo.szOriginalCallee);
        ulRet = DOS_FAIL;

    }

proc_finished:

    if (ulRet != DOS_SUCC)
    {
        /* �����з���ʾ���Ҷ� */
        if (pstSCB)
        {
            sc_req_hungup(pstSCB->ulSCBNo, pstSCB->stCall.ulCallingLegNo,ulErrCode);
        }

        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/**
 * ���� @a pstLCB �е���Ϣ������һ����PSTN�ĺ���
 *
 * @param pstSCB
 * @param pstLCB
 *
 * @return �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 *
 * @note ����ú����г����쳣��@a pstSCB �� @a pstLCB��ָ��Ŀ��ƿ鲻�ᱻ�ͷţ����õĵط����������
 */
U32 sc_make_call2pstn(SC_SRV_CB *pstSCB, SC_LEG_CB *pstLCB)
{
    SC_MSG_CMD_CALL_ST          stCallMsg;

    if (DOS_ADDR_INVALID(pstSCB) || DOS_ADDR_INVALID(pstLCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* @TODO ������任���������COPY�� pstLCB->stCall.stNumInfo.szRealCalling �� pstLCB->stCall.stNumInfo.szRealCalling*/
    dos_snprintf(pstLCB->stCall.stNumInfo.szRealCalling, sizeof(pstLCB->stCall.stNumInfo.szRealCalling), pstLCB->stCall.stNumInfo.szOriginalCalling);
    dos_snprintf(pstLCB->stCall.stNumInfo.szRealCallee, sizeof(pstLCB->stCall.stNumInfo.szRealCallee), pstLCB->stCall.stNumInfo.szOriginalCallee);

    /* ���ֺ�����Ҫ��·�� */
    pstSCB->stCall.ulRouteID = sc_route_search(pstSCB, pstLCB->stCall.stNumInfo.szRealCalling, pstLCB->stCall.stNumInfo.szRealCallee);
    if (U32_BUTT == pstSCB->stCall.ulRouteID)
    {
        sc_trace_scb(pstSCB, "no route to pstn.");

        pstLCB->stCall.ulCause = CC_ERR_SC_NO_ROUTE;
        return DOS_FAIL;
    }

    /* ���û���ҵ��м̣���Ҫ�ܾ����� */
    pstLCB->stCall.ulTrunkCnt = sc_route_get_trunks(pstSCB->stCall.ulRouteID, pstLCB->stCall.aulTrunkList, SC_MAX_TRUCK_NUM);
    if (0 == pstLCB->stCall.ulTrunkCnt)
    {
        sc_trace_scb(pstSCB, "no trunk to pstn. route id:%u", pstSCB->stCall.ulRouteID);

        pstLCB->stCall.ulCause = CC_ERR_SC_NO_TRUNK;
        return DOS_FAIL;
    }

    /* @TODO ������任���������COPY�� pstLCB->stCall.stNumInfo.szCalling �� pstLCB->stCall.stNumInfo.szCallee*/
    dos_snprintf(pstLCB->stCall.stNumInfo.szCallee, sizeof(pstLCB->stCall.stNumInfo.szRealCallee), pstLCB->stCall.stNumInfo.szOriginalCallee);
    dos_snprintf(pstLCB->stCall.stNumInfo.szCalling, sizeof(pstLCB->stCall.stNumInfo.szRealCallee), pstLCB->stCall.stNumInfo.szOriginalCalling);

    dos_snprintf(pstLCB->stCall.stNumInfo.szCID, sizeof(pstLCB->stCall.stNumInfo.szCID), pstLCB->stCall.stNumInfo.szOriginalCallee);

    stCallMsg.stMsgTag.ulMsgType = SC_CMD_CALL;
    stCallMsg.stMsgTag.ulSCBNo   = pstSCB->ulSCBNo;
    stCallMsg.ulLCBNo = pstLCB->ulCBNo;
    stCallMsg.ulSCBNo = pstSCB->ulSCBNo;

    if (sc_send_cmd_new_call(&stCallMsg.stMsgTag) != DOS_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Send new call request fail.");
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/**
 * ���� @a pstLCB �е���Ϣ������һ����TT�ŵĺ���
 *
 * @param pstSCB
 * @param pstLCB
 *
 * @return �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 *
 * @note ����ú����г����쳣��@a pstSCB �� @a pstLCB��ָ��Ŀ��ƿ鲻�ᱻ�ͷţ����õĵط����������
 */
U32 sc_make_call2eix(SC_SRV_CB *pstSCB, SC_LEG_CB *pstLCB)
{
    SC_MSG_CMD_CALL_ST          stCallMsg;

    if (DOS_ADDR_INVALID(pstSCB) || DOS_ADDR_INVALID(pstLCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    stCallMsg.stMsgTag.ulMsgType = SC_CMD_CALL;
    stCallMsg.stMsgTag.ulSCBNo   = pstSCB->ulSCBNo;
    stCallMsg.ulLCBNo = pstLCB->ulCBNo;
    stCallMsg.ulSCBNo = pstLCB->ulSCBNo;

    if (sc_eix_dev_get_by_tt(pstLCB->stCall.stNumInfo.szRealCallee, pstLCB->stCall.szEIXAddr, sizeof(pstLCB->stCall.szEIXAddr)) != DOS_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Get EIX ip fail by the TT number. %s", pstLCB->stCall.stNumInfo.szRealCallee);
        return DOS_FAIL;
    }

    dos_snprintf(pstLCB->stCall.stNumInfo.szRealCallee, sizeof(pstLCB->stCall.stNumInfo.szRealCallee), pstLCB->stCall.stNumInfo.szOriginalCallee);
    dos_snprintf(pstLCB->stCall.stNumInfo.szRealCalling, sizeof(pstLCB->stCall.stNumInfo.szRealCalling), pstLCB->stCall.stNumInfo.szOriginalCalling);
    dos_snprintf(pstLCB->stCall.stNumInfo.szCallee, sizeof(pstLCB->stCall.stNumInfo.szRealCallee), pstLCB->stCall.stNumInfo.szOriginalCallee);
    dos_snprintf(pstLCB->stCall.stNumInfo.szCalling, sizeof(pstLCB->stCall.stNumInfo.szRealCallee), pstLCB->stCall.stNumInfo.szOriginalCalling);

    dos_snprintf(pstLCB->stCall.stNumInfo.szCID, sizeof(pstLCB->stCall.stNumInfo.szCID), pstLCB->stCall.stNumInfo.szOriginalCallee);

    /* ����EIXǿ��ʹ��G723��G729 */
    pstLCB->stCall.aucCodecList[0] = PT_G723;
    pstLCB->stCall.aucCodecList[1] = PT_G729;
    pstLCB->stCall.ulCodecCnt = 2;

    if (sc_send_cmd_new_call(&stCallMsg.stMsgTag) != DOS_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Send new call request fail.");
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/**
 * ���� @a pstLCB �е���Ϣ������һ����SIP�˻��ĺ���
 *
 * @param pstSCB
 * @param pstLCB
 *
 * @return �ɹ�����DOS_SUCC�����򷵻�DOS_FAIL
 *
 * @note ����ú����г����쳣��@a pstSCB �� @a pstLCB��ָ��Ŀ��ƿ鲻�ᱻ�ͷţ����õĵط����������
 */
U32 sc_make_call2sip(SC_SRV_CB *pstSCB, SC_LEG_CB *pstLCB)
{
    SC_MSG_CMD_CALL_ST          stCallMsg;

    if (DOS_ADDR_INVALID(pstSCB) || DOS_ADDR_INVALID(pstLCB))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    stCallMsg.stMsgTag.ulMsgType = SC_CMD_CALL;
    stCallMsg.stMsgTag.ulSCBNo   = pstSCB->ulSCBNo;
    stCallMsg.ulLCBNo = pstLCB->ulCBNo;
    stCallMsg.ulSCBNo = pstLCB->ulSCBNo;

    dos_snprintf(pstLCB->stCall.stNumInfo.szRealCallee, sizeof(pstLCB->stCall.stNumInfo.szRealCallee), pstLCB->stCall.stNumInfo.szOriginalCallee);
    dos_snprintf(pstLCB->stCall.stNumInfo.szRealCalling, sizeof(pstLCB->stCall.stNumInfo.szRealCalling), pstLCB->stCall.stNumInfo.szOriginalCalling);
    dos_snprintf(pstLCB->stCall.stNumInfo.szCallee, sizeof(pstLCB->stCall.stNumInfo.szRealCallee), pstLCB->stCall.stNumInfo.szOriginalCallee);
    dos_snprintf(pstLCB->stCall.stNumInfo.szCalling, sizeof(pstLCB->stCall.stNumInfo.szRealCallee), pstLCB->stCall.stNumInfo.szOriginalCalling);

    dos_snprintf(pstLCB->stCall.stNumInfo.szCID, sizeof(pstLCB->stCall.stNumInfo.szCID), pstLCB->stCall.stNumInfo.szOriginalCallee);

    if (sc_send_cmd_new_call(&stCallMsg.stMsgTag) != DOS_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Send new call request fail.");
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

U32 sc_voice_verify_proc(U32 ulCustomer, S8 *pszNumber, S8 *pszPassword, U32 ulPlayCnt)
{
    SC_SRV_CB    *pstSCB = NULL;
    SC_LEG_CB    *pstLCB = NULL;

    if (DOS_ADDR_INVALID(pszNumber) || '\0' == pszNumber[0])
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Empty number for voice verify");
        goto fail_proc;
    }

    if (DOS_ADDR_INVALID(pszPassword) || '\0' == pszPassword[0])
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Empty verify code for voice verify");
        goto fail_proc;
    }

    pstSCB = sc_scb_alloc();
    if (DOS_ADDR_INVALID(pstSCB))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Alloc SCB fail");
        goto fail_proc;
    }

    pstLCB = sc_lcb_alloc();
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Alloc LCB fail");
        goto fail_proc;
    }

    pstSCB->stVoiceVerify.stSCBTag.bValid = DOS_TRUE;
    pstSCB->stVoiceVerify.stSCBTag.usStatus = SC_VOICE_VERIFY_INIT;
    pstSCB->stVoiceVerify.ulLegNo = pstLCB->ulCBNo;
    pstSCB->stVoiceVerify.ulTipsHitNo1 = SC_SND_YOUR_CODE_IS;
    pstSCB->stVoiceVerify.ulTipsHitNo2 = SC_SND_BUTT;
    dos_snprintf(pstSCB->stVoiceVerify.szVerifyCode, sizeof(pstSCB->stVoiceVerify.szVerifyCode), pszPassword);
    pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stVoiceVerify.stSCBTag;
    if (sc_scb_set_service(pstSCB, BS_SERV_VERIFY) != DOS_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Add voice verify service to scb fail.");
        goto fail_proc;
    }

    pstLCB->ulSCBNo = pstSCB->ulSCBNo;
    pstLCB->stCall.bValid = DOS_TRUE;

    if (sc_caller_setting_select_number(ulCustomer, 0, SC_SRC_CALLER_TYPE_ALL
                        , pstLCB->stCall.stNumInfo.szOriginalCalling, SC_NUM_LENGTH) != DOS_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is no caller for number verify.");
        goto fail_proc;
    }

    dos_snprintf(pstLCB->stCall.stNumInfo.szOriginalCallee, SC_NUM_LENGTH, pszNumber);

    if (sc_send_usr_auth2bs(pstSCB, pstLCB) != DOS_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Send auth request fail.");
        goto fail_proc;
    }

    pstSCB->stVoiceVerify.stSCBTag.usStatus = SC_VOICE_VERIFY_AUTH;

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Send auth request for number verify succ. Customer: %u, Code: %s, Callee: %s", ulCustomer, pszPassword, pszNumber);
    return DOS_SUCC;

fail_proc:
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

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Send auth request for number verify FAIL. Customer: %u, Code: %s, Callee: %s", ulCustomer, pszPassword, pszNumber);

    return DOS_FAIL;
}

U32 sc_call_ctrl_call_agent(U32 ulCurrentAgent, SC_AGENT_NODE_ST  *pstAgentNode)
{
    return DOS_SUCC;
}

U32 sc_call_ctrl_call_sip(U32 ulAgent, S8 *pszSipNumber)
{

    return DOS_SUCC;
}

U32 sc_call_ctrl_call_out(U32 ulAgent, U32 ulTaskID, S8 *pszNumber)
{
    SC_AGENT_NODE_ST *pstAgentNode = NULL;
    SC_SRV_CB        *pstSCB       = NULL;
    SC_LEG_CB        *pstLCB       = NULL;
    SC_LEG_CB        *pstAgentLCB  = NULL;
    U32              ulRet         = DOS_FAIL;

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Request call out. Agent: %u, Task: %u, Number: %u", ulAgent, ulTaskID, NULL == pszNumber ? "NULL" : pszNumber);

    if (DOS_ADDR_INVALID(pszNumber) || '\0' == pszNumber[0])
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Request call out. Number is empty");
        return DOS_FAIL;
    }

    pstAgentNode = sc_agent_get_by_id(ulAgent);
    if (DOS_ADDR_INVALID(pstAgentNode) || DOS_ADDR_INVALID(pstAgentNode->pstAgentInfo))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Cannot found the agent %u", ulAgent);
        return DOS_FAIL;
    }

    pstAgentLCB = sc_lcb_get(pstAgentNode->pstAgentInfo->ulLegNo);
    if (DOS_ADDR_VALID(pstAgentLCB)
        && (!pstAgentNode->pstAgentInfo->bNeedConnected
            || !pstAgentNode->pstAgentInfo->bConnected
            || pstAgentLCB->ulIndSCBNo == U32_BUTT))
    {
        /* ��ϯ���ǳ�ǩ�������Ѿ�����һ��legͨ�� */
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "The agent %u is not sigin, but have a leg(%u)", ulAgent, pstAgentNode->pstAgentInfo->ulLegNo);
        return DOS_FAIL;
    }

    pstLCB = sc_lcb_alloc();
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Alloc lcb fail");
        return DOS_FAIL;
    }

    pstSCB = sc_scb_alloc();
    if (DOS_ADDR_INVALID(pstSCB))
    {
        sc_lcb_free(pstLCB);
        pstLCB = NULL;

        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Alloc scb fail");
        return DOS_FAIL;
    }

    pstLCB->stCall.bValid = DOS_SUCC;
    pstLCB->stCall.ucStatus = SC_LEG_INIT;

    pstSCB->ulCustomerID = pstAgentNode->pstAgentInfo->ulCustomerID;
    pstSCB->stPreviewCall.stSCBTag.bValid = DOS_TRUE;
    pstSCB->stPreviewCall.stSCBTag.usStatus = SC_PREVIEW_CALL_IDEL;
    pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stPreviewCall.stSCBTag;
    if (sc_scb_set_service(pstSCB, BS_SERV_PREVIEW_DIALING) != DOS_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Set service fail.");

        goto process_fail;
    }

    /* ��ǩ */
    if (DOS_ADDR_VALID(pstAgentLCB))
    {
        /* �����к������л�ȡ���к��� */
        ulRet = sc_caller_setting_select_number(pstSCB->ulCustomerID, ulAgent, SC_SRC_CALLER_TYPE_AGENT, pstLCB->stCall.stNumInfo.szOriginalCalling, SC_NUM_LENGTH);
        if (ulRet != DOS_SUCC)
        {
            sc_lcb_free(pstLCB);
            pstLCB = NULL;
            sc_scb_free(pstSCB);
            pstSCB = NULL;
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Get caller fail by agent(%u).", ulAgent);

            goto process_fail;
        }

        dos_snprintf(pstLCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstLCB->stCall.stNumInfo.szOriginalCallee), pszNumber);
        dos_snprintf(pstAgentNode->pstAgentInfo->szLastCustomerNum, SC_NUM_LENGTH, "%s", pszNumber);

        pstSCB->stPreviewCall.ulCallingLegNo = pstAgentLCB->ulCBNo;
        pstSCB->stPreviewCall.ulCalleeLegNo = pstLCB->ulCBNo;
        pstSCB->stPreviewCall.ulAgentID = pstAgentNode->pstAgentInfo->ulAgentID;

        pstAgentLCB->ulSCBNo = pstSCB->ulSCBNo;
        pstLCB->ulSCBNo = pstSCB->ulSCBNo;

        /* �ж��Ƿ�¼�� */
        if (pstAgentNode->pstAgentInfo->bRecord)
        {
            pstLCB->stRecord.bValid = DOS_TRUE;
        }

        pstSCB->stPreviewCall.stSCBTag.usStatus = SC_PREVIEW_CALL_AUTH;

        if (sc_send_usr_auth2bs(pstSCB, pstLCB) != DOS_SUCC)
        {
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Send auth fail.");

            goto process_fail;
        }

        /* �޸���ϯ��״̬ */
        sc_agent_set_ringback(pstAgentNode->pstAgentInfo);

        /* ��ϯ���� */
        sc_agent_call_notify(pstAgentNode->pstAgentInfo, pszNumber);

        return DOS_SUCC;
    }

    switch (pstAgentNode->pstAgentInfo->ucBindType)
    {
        case AGENT_BIND_SIP:
            dos_snprintf(pstLCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstLCB->stCall.stNumInfo.szOriginalCallee), pstAgentNode->pstAgentInfo->szUserID);
            pstLCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND_INTERNAL;
            break;

        case AGENT_BIND_TELE:
            dos_snprintf(pstLCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstLCB->stCall.stNumInfo.szOriginalCallee), pstAgentNode->pstAgentInfo->szTelePhone);
            pstLCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND;
            if (sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL))
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Add outbound service fail.");

                goto process_fail;
            }
            break;

        case AGENT_BIND_MOBILE:
            dos_snprintf(pstLCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstLCB->stCall.stNumInfo.szOriginalCallee), pstAgentNode->pstAgentInfo->szMobile);
            pstLCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND;
            if (sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL))
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Add outbound service fail.");

                goto process_fail;
            }
            break;

        case AGENT_BIND_TT_NUMBER:
            dos_snprintf(pstLCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstLCB->stCall.stNumInfo.szOriginalCallee), pstAgentNode->pstAgentInfo->szTTNumber);
            pstLCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND_TT;
            break;

        default:
            break;
    }

    /* �����к������л�ȡ���к��� */
    ulRet = sc_caller_setting_select_number(pstSCB->ulCustomerID, ulAgent, SC_SRC_CALLER_TYPE_AGENT, pstLCB->stCall.stNumInfo.szOriginalCalling, SC_NUM_LENGTH);
    if (ulRet != DOS_SUCC)
    {
        sc_lcb_free(pstLCB);
        pstLCB = NULL;
        sc_scb_free(pstSCB);
        pstSCB = NULL;
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Get caller fail by agent(%u).", ulAgent);

        goto process_fail;
    }

    dos_snprintf(pstAgentNode->pstAgentInfo->szLastCustomerNum, SC_NUM_LENGTH, "%s", pszNumber);

    /* �ж��Ƿ�¼�� */
    if (pstAgentNode->pstAgentInfo->bRecord)
    {
        pstLCB->stRecord.bValid = DOS_TRUE;
    }

    pstSCB->stPreviewCall.ulAgentID = pstAgentNode->pstAgentInfo->ulAgentID;
    pstSCB->stPreviewCall.ulCallingLegNo = pstLCB->ulCBNo;
    pstSCB->stPreviewCall.stSCBTag.usStatus = SC_PREVIEW_CALL_AUTH;
    pstLCB->ulSCBNo = pstSCB->ulSCBNo;
    pstAgentNode->pstAgentInfo->ulLegNo = pstLCB->ulCBNo;

    if (sc_send_usr_auth2bs(pstSCB, pstLCB) != DOS_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Send auth fail.");

        goto process_fail;
    }

    /* �޸���ϯ��״̬����æ */
    sc_agent_set_ringing(pstAgentNode->pstAgentInfo);

    /* ��ϯ���� */
    sc_agent_call_notify(pstAgentNode->pstAgentInfo, pszNumber);

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Request call out. send auth succ.");

    return DOS_SUCC;

process_fail:
    if (DOS_ADDR_VALID(pstSCB))
    {
        sc_scb_free(pstSCB);
        pstSCB = NULL;
    }

    if (DOS_ADDR_VALID(pstLCB))
    {
        sc_lcb_free(pstLCB);
        pstLCB = DOS_SUCC;
    }

    return DOS_FAIL;
}

U32 sc_call_ctrl_transfer(U32 ulAgent, U32 ulAgentCalled, BOOL bIsAttend)
{
    return DOS_SUCC;
}

U32 sc_call_ctrl_hold(U32 ulAgent, BOOL isHold)
{
    return DOS_SUCC;
}

U32 sc_call_ctrl_unhold(U32 ulAgent)
{
    return DOS_SUCC;
}

U32 sc_call_ctrl_hangup(U32 ulAgent)
{
    return DOS_SUCC;
}

U32 sc_call_ctrl_hangup_all(U32 ulAgent)
{
    return DOS_SUCC;
}

U32 sc_call_ctrl_intercept(U32 ulTaskID, U32 ulAgent, U32 ulCustomerID, U32 ulType, S8 *pszCallee)
{
    SC_AGENT_NODE_ST *pstAgentNode = NULL;
    SC_LEG_CB        *pstLCB       = NULL;
    SC_SRV_CB        *pstSCB       = NULL;
    SC_LEG_CB        *pstLCBAgent  = NULL;

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Request intercept. Agent: %u, Task: %u, Number: %s", ulAgent, ulTaskID, NULL == pszCallee ? "NULL" : pszCallee);

    if (DOS_ADDR_INVALID(pszCallee))
    {
        return DOS_FAIL;
    }

    pstAgentNode = sc_agent_get_by_id(ulAgent);
    if (DOS_ADDR_INVALID(pstAgentNode))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Cannot found the agent %u", ulAgent);
        return DOS_FAIL;
    }

    if (DOS_ADDR_INVALID(pstAgentNode->pstAgentInfo))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Agent CB Error %u", ulAgent);
        return DOS_FAIL;
    }

    if (pstAgentNode->pstAgentInfo->ulLegNo >= SC_LEG_CB_SIZE)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Agent not in calling %u", ulAgent);
        return DOS_FAIL;
    }

    pstLCBAgent = sc_lcb_get(pstAgentNode->pstAgentInfo->ulLegNo);
    if (DOS_ADDR_INVALID(pstLCBAgent))
    {
        return DOS_FAIL;
    }

    pstSCB = sc_scb_get(pstLCBAgent->ulSCBNo);
    if (DOS_ADDR_INVALID(pstSCB))
    {
        return DOS_FAIL;
    }

    pstLCB = sc_lcb_alloc();
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Alloc lcb fail");
        return DOS_FAIL;
    }

    pstSCB->stInterception.stSCBTag.bValid = DOS_TRUE;
    pstSCB->stInterception.stSCBTag.usStatus = SC_INTERCEPTION_IDEL;
    pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stInterception.stSCBTag;

    pstLCB->stCall.bValid = DOS_SUCC;
    pstLCB->stCall.ucStatus = SC_LEG_INIT;

/*
    if (!sc_ep_black_list_check(ulCustomerID, pszCallee))
    {
        DOS_ASSERT(0);

        sc_logr_info(pstSCB, SC_ESL, "Cannot make call. Callee in blocak list. (%s)", pszCallee);

        goto process_fail;
    }
*/
    /* ���к��룬 �����к������л�ȡ */
    if (sc_caller_setting_select_number(ulCustomerID, 0, SC_SRC_CALLER_TYPE_ALL
                        , pstLCB->stCall.stNumInfo.szOriginalCalling, SC_NUM_LENGTH) != DOS_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "There is no caller for number verify.");

        goto process_fail;
    }

    /* ���к��� */
    dos_snprintf(pstLCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstLCB->stCall.stNumInfo.szOriginalCallee), pszCallee);
    switch (ulType)
    {
        case AGENT_BIND_SIP:
            pstLCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND_INTERNAL;
            break;

        case AGENT_BIND_TELE:
            pstLCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND;
            if (sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL))
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Add outbound service fail.");

                goto process_fail;
            }
            break;

        case AGENT_BIND_MOBILE:
            pstLCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND;
            if (sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL))
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Add outbound service fail.");

                goto process_fail;
            }
            break;

        case AGENT_BIND_TT_NUMBER:
            pstLCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND_TT;
            break;

        default:
            break;
    }

    pstSCB->stInterception.ulLegNo = pstLCB->ulCBNo;
    pstSCB->stInterception.ulAgentLegNo = pstLCBAgent->ulCBNo;
    pstSCB->stInterception.stSCBTag.usStatus = SC_INTERCEPTION_AUTH;
    pstLCB->ulSCBNo = pstSCB->ulSCBNo;

    if (sc_send_usr_auth2bs(pstSCB, pstLCB) != DOS_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Send auth fail.");

        goto process_fail;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Request call out. send auth succ.");

    return DOS_SUCC;

process_fail:

    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_lcb_free(pstLCB);
        pstLCB = NULL;
    }

    return DOS_FAIL;
}


U32 sc_call_ctrl_proc(U32 ulAction, U32 ulTaskID, U32 ulAgent, U32 ulCustomerID, U32 ulType, S8 *pszCallee, U32 ulFlag, U32 ulCalleeAgentID)
{
    U32 ulRet = DOS_FAIL;

    if (ulAction >= SC_API_CALLCTRL_BUTT)
    {
        DOS_ASSERT(0);

        goto proc_end;
    }

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_EVENT), "Start process call ctrl msg. Action: %u, Agent: %u, Customer: %u, Task: %u, Caller: %s"
                    , ulAction, ulAgent, ulCustomerID, ulTaskID, pszCallee ? pszCallee : "");

    switch (ulAction)
    {
        case SC_API_MAKE_CALL:
        case SC_API_TRANSFOR_ATTAND:
        case SC_API_TRANSFOR_BLIND:
        case SC_API_CONFERENCE:
        case SC_API_HOLD:
        case SC_API_UNHOLD:
            ulRet = DOS_SUCC;
            break;

        case SC_API_HANGUP_CALL:
            ulRet = sc_call_ctrl_hangup_all(ulAgent);
            break;

        case SC_API_RECORD:
#if 0
            /* ������ϯ */
            if (sc_acd_get_agent_by_id(&stAgentInfo, ulAgent) != DOS_SUCC)
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Cannot hangup call for agent with id %u. Agent not found..", ulAgent);
                goto proc_fail;
            }

            if (stAgentInfo.usSCBNo >= SC_MAX_SCB_NUM)
            {
                DOS_ASSERT(0);

                sc_logr_info(pstSCB, SC_ESL, "Cannot hangup call for agent with id %u. Agent handle a invalid SCB No(%u).", ulAgent, stAgentInfo.usSCBNo);
                goto proc_fail;
            }

            pstSCB = sc_scb_get(stAgentInfo.usSCBNo);
            if (DOS_ADDR_INVALID(pstSCB) || !pstSCB->bValid)
            {
                DOS_ASSERT(0);

                sc_logr_info(pstSCB, SC_ESL, "Cannot hangup call for agent with id %u. Agent handle a SCB(%u) is invalid.", ulAgent, stAgentInfo.usSCBNo);
                goto proc_fail;
            }

            if ('\0' == pstSCB->szUUID[0])
            {
                DOS_ASSERT(0);

                sc_logr_info(pstSCB, SC_ESL, "Cannot hangup call for agent with id %u. Agent handle a SCB(%u) without UUID.", ulAgent, stAgentInfo.usSCBNo);
                goto proc_fail;
            }

            if (sc_ep_record(pstSCB) != DOS_SUCC)
            {
                goto proc_fail;
            }
#endif
            break;

        case SC_API_WHISPERS:
            ulRet = DOS_SUCC;
            break;
        case SC_API_INTERCEPT:
            ulRet = sc_call_ctrl_intercept(ulTaskID, ulAgent, ulCustomerID, ulType, pszCallee);
            break;
        default:
            ulRet = DOS_FAIL;
    }

proc_end:
    if (ulRet != DOS_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Process call ctrl msg FAIL. Action: %u, Agent: %u, Customer: %u, Task: %u, Caller: %s"
                    , ulAction, ulAgent, ulCustomerID, ulTaskID, pszCallee);
    }
    else
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_INFO, SC_MOD_EVENT), "Finished to process call ctrl msg. Action: %u, Agent: %u, Customer: %u, Task: %u, Caller: %s"
                    , ulAction, ulAgent, ulCustomerID, ulTaskID, pszCallee);
    }

    return ulRet;
}

U32 sc_demo_task(U32 ulCustomerID, S8 *pszCallee, S8 *pszAgentNum, U32 ulAgentID)
{
    SC_SRV_CB *pstSCB   = NULL;
    SC_LEG_CB *pstLegCB = NULL;
    U32        ulRet    = DOS_FAIL;

    if (DOS_ADDR_INVALID(pszCallee)
        || DOS_ADDR_INVALID(pszAgentNum))
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    /* ����һ��scb��leg */
    pstSCB = sc_scb_alloc();
    if (DOS_ADDR_INVALID(pstSCB))
    {
        DOS_ASSERT(0);
        goto make_call_file;
    }

    pstLegCB = sc_lcb_alloc();
    if (DOS_ADDR_INVALID(pstLegCB))
    {
        DOS_ASSERT(0);
        goto make_call_file;
    }

    pstSCB->stDemoTask.stSCBTag.bValid = DOS_TRUE;
    pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stDemoTask.stSCBTag;
    pstSCB->stDemoTask.ulCallingLegNo = pstLegCB->ulCBNo;
    pstSCB->stDemoTask.ulTaskID = 0;
    pstSCB->stDemoTask.ulTcbID = 0;
    pstSCB->stDemoTask.ulKeyMode = 0;
    pstSCB->stDemoTask.ulAgentID = ulAgentID;
    pstSCB->ulCustomerID = ulCustomerID;

    pstLegCB->stCall.bValid = DOS_TRUE;
    pstLegCB->ulSCBNo = pstSCB->ulSCBNo;

    dos_snprintf(pstLegCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstLegCB->stCall.stNumInfo.szOriginalCallee), pszCallee);
    /* �����к������л�ȡ���к��� */
    ulRet = sc_caller_setting_select_number(pstSCB->ulCustomerID, ulAgentID, SC_SRC_CALLER_TYPE_AGENT, pstLegCB->stCall.stNumInfo.szOriginalCalling, SC_NUM_LENGTH);
    if (ulRet != DOS_SUCC)
    {
        sc_lcb_free(pstLegCB);
        sc_scb_free(pstSCB);
        return DOS_FAIL;
    }

    pstLegCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND;
    sc_scb_set_service(pstSCB, BS_SERV_AUTO_DIALING);

    /* ��֤ */
    pstSCB->stDemoTask.stSCBTag.usStatus = SC_AUTO_CALL_AUTH;
    if (sc_send_usr_auth2bs(pstSCB, pstLegCB) != DOS_SUCC)
    {
        goto make_call_file;
    }

    return DOS_SUCC;

make_call_file:

    return DOS_FAIL;
}

U32 sc_demo_preview(U32 ulCustomerID, S8 *pszCallee, S8 *pszAgentNum, U32 ulAgentID)
{
    SC_AGENT_NODE_ST *pstAgentNode = NULL;
    SC_SRV_CB        *pstSCB       = NULL;
    SC_LEG_CB        *pstLCB       = NULL;
    SC_LEG_CB        *pstAgentLCB  = NULL;
    U32              ulRet         = DOS_FAIL;

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Request call out. Agent: %u, Number: %u", ulAgentID, NULL == pszCallee ? "NULL" : pszCallee);

    if (DOS_ADDR_INVALID(pszCallee) || '\0' == pszCallee[0])
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Request call out. Number is empty");
        return DOS_FAIL;
    }

    pstAgentNode = sc_agent_get_by_id(ulAgentID);
    if (DOS_ADDR_INVALID(pstAgentNode) || DOS_ADDR_INVALID(pstAgentNode->pstAgentInfo))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Cannot found the agent %u", ulAgentID);
        return DOS_FAIL;
    }

    pstAgentLCB = sc_lcb_get(pstAgentNode->pstAgentInfo->ulLegNo);
    if (DOS_ADDR_VALID(pstAgentLCB)
        && (!pstAgentNode->pstAgentInfo->bNeedConnected
            || !pstAgentNode->pstAgentInfo->bConnected
            || pstAgentLCB->ulIndSCBNo == U32_BUTT))
    {
        /* ��ϯ���ǳ�ǩ�������Ѿ�����һ��legͨ�� */
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "The agent %u is not sigin, but have a leg(%u)", ulAgentID, pstAgentNode->pstAgentInfo->ulLegNo);
        return DOS_FAIL;
    }

    pstLCB = sc_lcb_alloc();
    if (DOS_ADDR_INVALID(pstLCB))
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Alloc lcb fail");
        return DOS_FAIL;
    }

    pstSCB = sc_scb_alloc();
    if (DOS_ADDR_INVALID(pstSCB))
    {
        sc_lcb_free(pstLCB);
        pstLCB = NULL;

        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Alloc scb fail");
        return DOS_FAIL;
    }

    pstLCB->stCall.bValid = DOS_SUCC;
    pstLCB->stCall.ucStatus = SC_LEG_INIT;

    pstSCB->ulCustomerID = pstAgentNode->pstAgentInfo->ulCustomerID;
    pstSCB->stPreviewCall.stSCBTag.bValid = DOS_TRUE;
    pstSCB->stPreviewCall.stSCBTag.usStatus = SC_PREVIEW_CALL_IDEL;
    pstSCB->pstServiceList[pstSCB->ulCurrentSrv] = &pstSCB->stPreviewCall.stSCBTag;
    if (sc_scb_set_service(pstSCB, BS_SERV_PREVIEW_DIALING) != DOS_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Set service fail.");

        goto process_fail;
    }

    /* ��ǩ */
    if (DOS_ADDR_VALID(pstAgentLCB))
    {
        /* �����к������л�ȡ���к��� */
        ulRet = sc_caller_setting_select_number(pstSCB->ulCustomerID, ulAgentID, SC_SRC_CALLER_TYPE_AGENT, pstLCB->stCall.stNumInfo.szOriginalCalling, SC_NUM_LENGTH);
        if (ulRet != DOS_SUCC)
        {
            sc_lcb_free(pstLCB);
            pstLCB = NULL;
            sc_scb_free(pstSCB);
            pstSCB = NULL;
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Get caller fail by agent(%u).", ulAgentID);

            goto process_fail;
        }

        dos_snprintf(pstLCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstLCB->stCall.stNumInfo.szOriginalCallee), pszCallee);
        dos_snprintf(pstAgentNode->pstAgentInfo->szLastCustomerNum, SC_NUM_LENGTH, "%s", pszCallee);

        pstSCB->stPreviewCall.ulCallingLegNo = pstAgentLCB->ulCBNo;
        pstSCB->stPreviewCall.ulCalleeLegNo = pstLCB->ulCBNo;
        pstSCB->stPreviewCall.ulAgentID = pstAgentNode->pstAgentInfo->ulAgentID;

        pstAgentLCB->ulSCBNo = pstSCB->ulSCBNo;
        pstLCB->ulSCBNo = pstSCB->ulSCBNo;

        /* �ж��Ƿ�¼�� */
        if (pstAgentNode->pstAgentInfo->bRecord)
        {
            pstLCB->stRecord.bValid = DOS_TRUE;
        }

        pstSCB->stPreviewCall.stSCBTag.usStatus = SC_PREVIEW_CALL_AUTH;

        if (sc_send_usr_auth2bs(pstSCB, pstLCB) != DOS_SUCC)
        {
            sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Send auth fail.");

            goto process_fail;
        }

        /* �޸���ϯ��״̬����æ */
        sc_agent_set_ringback(pstAgentNode->pstAgentInfo);

        /* ��ϯ���� */
        sc_agent_call_notify(pstAgentNode->pstAgentInfo, pszCallee);

        return DOS_SUCC;
    }

    switch (pstAgentNode->pstAgentInfo->ucBindType)
    {
        case AGENT_BIND_SIP:
            dos_snprintf(pstLCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstLCB->stCall.stNumInfo.szOriginalCallee), pstAgentNode->pstAgentInfo->szUserID);
            pstLCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND_INTERNAL;
            break;

        case AGENT_BIND_TELE:
            dos_snprintf(pstLCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstLCB->stCall.stNumInfo.szOriginalCallee), pstAgentNode->pstAgentInfo->szTelePhone);
            pstLCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND;
            if (sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL))
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Add outbound service fail.");

                goto process_fail;
            }
            break;

        case AGENT_BIND_MOBILE:
            dos_snprintf(pstLCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstLCB->stCall.stNumInfo.szOriginalCallee), pstAgentNode->pstAgentInfo->szMobile);
            pstLCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND;
            if (sc_scb_set_service(pstSCB, BS_SERV_OUTBAND_CALL))
            {
                sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Add outbound service fail.");

                goto process_fail;
            }
            break;

        case AGENT_BIND_TT_NUMBER:
            dos_snprintf(pstLCB->stCall.stNumInfo.szOriginalCallee, sizeof(pstLCB->stCall.stNumInfo.szOriginalCallee), pstAgentNode->pstAgentInfo->szTTNumber);
            pstLCB->stCall.ucPeerType = SC_LEG_PEER_OUTBOUND_TT;
            break;

        default:
            break;
    }

    /* �����к������л�ȡ���к��� */
    ulRet = sc_caller_setting_select_number(pstSCB->ulCustomerID, ulAgentID, SC_SRC_CALLER_TYPE_AGENT, pstLCB->stCall.stNumInfo.szOriginalCalling, SC_NUM_LENGTH);
    if (ulRet != DOS_SUCC)
    {
        sc_lcb_free(pstLCB);
        pstLCB = NULL;
        sc_scb_free(pstSCB);
        pstSCB = NULL;
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_WARNING, SC_MOD_EVENT), "Get caller fail by agent(%u).", ulAgentID);

        goto process_fail;
    }

    dos_snprintf(pstAgentNode->pstAgentInfo->szLastCustomerNum, SC_NUM_LENGTH, "%s", pszCallee);

    /* �ж��Ƿ�¼�� */
    if (pstAgentNode->pstAgentInfo->bRecord)
    {
        pstLCB->stRecord.bValid = DOS_TRUE;
    }

    pstSCB->stPreviewCall.ulAgentID = pstAgentNode->pstAgentInfo->ulAgentID;
    pstSCB->stPreviewCall.ulCallingLegNo = pstLCB->ulCBNo;
    pstSCB->stPreviewCall.stSCBTag.usStatus = SC_PREVIEW_CALL_AUTH;
    pstLCB->ulSCBNo = pstSCB->ulSCBNo;
    pstAgentNode->pstAgentInfo->ulLegNo = pstLCB->ulCBNo;

    if (sc_send_usr_auth2bs(pstSCB, pstLCB) != DOS_SUCC)
    {
        sc_log(SC_LOG_SET_MOD(LOG_LEVEL_ERROR, SC_MOD_EVENT), "Send auth fail.");

        goto process_fail;
    }

    /* �޸���ϯ��״̬����æ */
    sc_agent_set_ringing(pstAgentNode->pstAgentInfo);

    /* ��ϯ���� */
    sc_agent_call_notify(pstAgentNode->pstAgentInfo, pszCallee);

    sc_log(SC_LOG_SET_MOD(LOG_LEVEL_NOTIC, SC_MOD_EVENT), "Request call out. send auth succ.");

    return DOS_SUCC;

process_fail:
    if (DOS_ADDR_VALID(pstSCB))
    {
        sc_scb_free(pstSCB);
        pstSCB = NULL;
    }

    if (DOS_ADDR_VALID(pstLCB))
    {
        sc_lcb_free(pstLCB);
        pstLCB = DOS_SUCC;
    }

    return DOS_FAIL;
}

#ifdef __cplusplus
}
#endif /* End of __cplusplus */

