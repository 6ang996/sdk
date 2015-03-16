/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  文件名：sc_debug.c
 *
 *  创建时间: 2014年12月16日15:16:28
 *  作    者: Larry
 *  描    述: 业务控制模块调试函数
 *  修改历史:
 */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* include public header files */
#include <dos.h>
#include <esl.h>
#include <hash/hash.h>

/* include private header files */
#include "sc_httpd.h"
#include "sc_http_api.h"
#include "sc_def.h"
#include "sc_debug.h"
#include "sc_acd_def.h"
#include "sc_acd.h"
#include "sc_ep.h"

/* define marcos */

/* define enums */

/* define structs */

/* declare global params */
U32       g_ulSCLogLevel        = LOG_LEVEL_DEBUG;       /* 日志级别 */

U32       g_ulTaskTraceAll      = 0;       /* 是否跟踪所有任务 */

U32       g_ulCallTraceAll      = 1;     /* 跟踪所有的呼叫 */

#ifdef DEBUG_VERSION
U32       g_ulTraceFlags        = 0xFFFFFFFF;
#else
U32       g_ulTraceFlags        = 0;
#endif

extern SC_HTTPD_CB_ST        *g_pstHTTPDList[SC_MAX_HTTPD_NUM];
extern SC_HTTP_CLIENT_CB_S   *g_pstHTTPClientList[SC_MAX_HTTP_CLIENT_NUM];
extern HASH_TABLE_S          *g_pstAgentList;
extern SC_TASK_MNGT_ST       *g_pstTaskMngtInfo;
extern HASH_TABLE_S          *g_pstGroupList;
extern HASH_TABLE_S          *g_pstAgentList;
extern HASH_TABLE_S          *g_pstHashGW;
HASH_TABLE_S                 *g_pstHashGWGrp;

/* declare functions */
extern SC_TASK_CB_ST *sc_tcb_get_by_id(U32 ulTCBNo);

/**
 * 函数: VOID sc_debug_show_httpd(U32 ulIndex)
 * 功能: 打印http server的信息
 * 参数:
 *  U32 ulIndex  telnet客户端索引
 */
VOID sc_show_httpd(U32 ulIndex, U32 ulID)
{
    U32 ulHttpdIndex, ulActive;
    S8 szCmdBuff[1024];
    S8 szIPAddr[32];

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nHTTP Server List for WEB CMD:");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n%10s%32s%16s%10s%10s", "Index", "IP Address", "Port", "Req Cnt", "Status");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n--------------------------------------------------------------------------------");
    cli_out_string(ulIndex, szCmdBuff);

    for (ulActive=0, ulHttpdIndex=0; ulHttpdIndex<SC_MAX_HTTPD_NUM; ulHttpdIndex++)
    {
        if (!g_pstHTTPDList[ulHttpdIndex])
        {
            continue;
        }

        if (U32_BUTT != ulID && ulID != ulHttpdIndex)
        {
            continue;
        }

        if (!dos_ipaddrtostr(g_pstHTTPDList[ulHttpdIndex]->aulIPAddr[0], szIPAddr, sizeof(szIPAddr)))
        {
            szIPAddr[0] = '\0';
        }

        dos_snprintf(szCmdBuff, sizeof(szCmdBuff)
                        , "\r\n%10d%32s%16d%10d%10s"
                        , g_pstHTTPDList[ulHttpdIndex]->ulIndex
                        , '\0' == szIPAddr[0] ? "NULL" : szIPAddr
                        , g_pstHTTPDList[ulHttpdIndex]->usPort
                        , g_pstHTTPDList[ulHttpdIndex]->ulReqCnt
                        , g_pstHTTPDList[ulHttpdIndex]->ulStatus ? "Active" : "Inactive");
        cli_out_string(ulIndex, szCmdBuff);
    }
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n--------------------------------------------------------------------------------");
    cli_out_string(ulIndex, szCmdBuff);

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nTotal: %d, Active: %d.\r\n", ulHttpdIndex, ulActive);
    cli_out_string(ulIndex, szCmdBuff);
}

/**
 * 函数: VOID sc_debug_show_http(U32 ulIndex)
 * 功能: 打印http 客户端的信息
 * 参数:
 *  U32 ulIndex  telnet客户端索引
 */
VOID sc_show_http(U32 ulIndex, U32 ulID)
{
    S8 szCmdBuff[1024];
    U32 ulHttpClientIndex = 0;

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nHTTP Client List for WEB CMD:");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n%16s%16s%16s", "Index", "Valid", "Server Index");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n------------------------------------------------------------");
    cli_out_string(ulIndex, szCmdBuff);

    for (ulHttpClientIndex=0; ulHttpClientIndex<SC_MAX_HTTP_CLIENT_NUM; ulHttpClientIndex++)
    {
        if (U32_BUTT != ulID && ulID != ulHttpClientIndex)
        {
            continue;
        }

        dos_snprintf(szCmdBuff, sizeof(szCmdBuff)
                        , "\r\n%16d%16d%16d"
                        , g_pstHTTPClientList[ulHttpClientIndex]->ulIndex
                        , g_pstHTTPClientList[ulHttpClientIndex]->ulValid
                        , g_pstHTTPClientList[ulHttpClientIndex]->ulCurrentSrv);
        cli_out_string(ulIndex, szCmdBuff);
    }

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n------------------------------------------------------------");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nTotal: %d.\r\n", ulHttpClientIndex);
    cli_out_string(ulIndex, szCmdBuff);
}

/**
 * 函数: VOID sc_debug_show_http(U32 ulIndex)
 * 功能: 打印scb的概要信息
 * 参数:
 *  U32 ulIndex  telnet客户端索引
 */
VOID sc_show_scb(U32 ulIndex, U32 ulID)
{

}

static S8* sc_debug_make_weeks(U32 ulWeekMask, S8 *pszWeeks, U32 ulLength)
{
    U32 ulCurrLen = 0;

    if (DOS_ADDR_INVALID(pszWeeks) || 0 == ulLength)
    {
        return "\0";
    }

    if (ulWeekMask & 0x00000001)
    {
        ulCurrLen = dos_snprintf(pszWeeks + ulCurrLen, ulLength - ulCurrLen, "Sun. ");
    }

    if (ulWeekMask & 0x00000002)
    {
        ulCurrLen = dos_snprintf(pszWeeks + ulCurrLen, ulLength - ulCurrLen, "Mon. ");
    }

    if (ulWeekMask & 0x00000004)
    {
        ulCurrLen = dos_snprintf(pszWeeks + ulCurrLen, ulLength - ulCurrLen, "Tus. ");
    }

    if (ulWeekMask & 0x00000008)
    {
        ulCurrLen = dos_snprintf(pszWeeks + ulCurrLen, ulLength - ulCurrLen, "Wen. ");
    }

    if (ulWeekMask & 0x00000010)
    {
        ulCurrLen = dos_snprintf(pszWeeks + ulCurrLen, ulLength - ulCurrLen, "Thur. ");
    }

    if (ulWeekMask & 0x00000020)
    {
        ulCurrLen = dos_snprintf(pszWeeks + ulCurrLen, ulLength - ulCurrLen, "Fri. ");
    }

    if (ulWeekMask & 0x00000040)
    {
        ulCurrLen = dos_snprintf(pszWeeks + ulCurrLen, ulLength - ulCurrLen, "Sat. ");
    }

    return pszWeeks;
}

VOID sc_show_task_list(U32 ulIndex, U32 ulCustomID)
{
    S8 szCmdBuff[1024] = { 0, };
    U32 ulTaskIndex, ulTotal;
    SC_TASK_CB_ST *pstTCB;

    if (U32_BUTT == ulCustomID)
    {
        cli_out_string(ulIndex, "\r\nCli Show Task List: ");
    }
    else
    {
        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nCli Show Task List for Customer %u: ", ulCustomID);

        cli_out_string(ulIndex, szCmdBuff);
    }

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n--------------------------------------------------------------------------------");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n%6s%7s%9s%6s%10s%10s%10s%10", "No.", "Status", "Priority", "Trace", "ID", "Custom ID", "Agent Cnt", "Caller Cnt");
    cli_out_string(ulIndex, szCmdBuff);

    for (ulTaskIndex=0,ulTotal=0; ulTaskIndex<SC_MAX_TASK_NUM; ulTaskIndex++)
    {
        pstTCB = &g_pstTaskMngtInfo->pstTaskList[ulTaskIndex];
        if (DOS_ADDR_INVALID(pstTCB) || !pstTCB->ucValid)
        {
            continue;
        }

        if (!pstTCB->ucValid)
        {
            continue;
        }

        if (U32_BUTT != ulCustomID && ulCustomID != pstTCB->ulCustomID)
        {
            continue;
        }

        dos_snprintf(szCmdBuff, sizeof(szCmdBuff)
                        , "\r\n%6d%7d%9d%-4s|%1s%10s%10s%10s%10"
                        , pstTCB->usTCBNo
                        , pstTCB->ucTaskStatus
                        , pstTCB->ucPriority
                        , pstTCB->bTraceON ? "Y" : "N"
                        , pstTCB->bTraceCallON ? "Y" : "N"
                        , pstTCB->ulTaskID
                        , pstTCB->ulCustomID
                        , pstTCB->usSiteCount
                        , pstTCB->usCallerCount);
        cli_out_string(ulIndex, szCmdBuff);
        ulTotal++;
    }

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n--------------------------------------------------------------------------------");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nTotal : %s\r\n",ulTotal);
    cli_out_string(ulIndex, szCmdBuff);
}

VOID sc_show_task(U32 ulIndex, U32 ulTaskID, U32 ulCustomID)
{
    S8 szCmdBuff[1024] = { 0 };
    S8 szWeeks[64] = { 0 };
    SC_TASK_CB_ST *pstTCB;

    /* 如果没有指定task id，或者指定了customer id，就需要使用列表的形式显示任务概要 */
    if (U32_BUTT == ulTaskID || U32_BUTT != ulCustomID)
    {
        sc_show_task_list(ulIndex, ulCustomID);
        return ;
    }

    pstTCB = sc_tcb_find_by_taskid(ulTaskID);
    if (DOS_ADDR_INVALID(pstTCB))
    {
        cli_out_string(ulIndex, "\r\nError:"
                                "\r\n    Invalid task ID"
                                "\r\n    Task with the ID is not valid. "
                                "\r\n    Please use \"sc show task custom id \" to ckeck a valid task\r\n");
        return;
    }

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff)
                , "\r\nTask Detail: "
                  "\r\n---------------Detail Information---------------");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff)
                , "\r\n                   ID : %d"
                  "\r\n               Status : %d"
                  "\r\n             Priority : %d"
                  "\r\n  Voice File Play Cnt : %d"
                  "\r\n      Voice File Path : %s"
                  "\r\n                Trace : %d"
                  "\r\n           Trace Call : %d"
                  "\r\n             ID in DB : %d"
                  "\r\n          Customer ID : %d"
                  "\r\n  Current Concurrency : %d"
                  "\r\n          Agent Count : %d"
                  "\r\n       Agent Queue ID : %d"
                  "\r\nTime Period 1 Weekday : %s"
                  "\r\n        Time Period 1 : %d:%d - %d:%d"
                  "\r\nTime Period 2 Weekday : %s"
                  "\r\n        Time Period 2 : %d:%d - %d:%d"
                  "\r\nTime Period 3 Weekday : %s"
                  "\r\n        Time Period 3 : %d:%d - %d:%d"
                  "\r\nTime Period 4 Weekday : %s"
                  "\r\n        Time Period 4 : %d:%d - %d:%d"
                , pstTCB->ulTaskID
                , pstTCB->ucTaskStatus
                , pstTCB->ucPriority
                , pstTCB->ucAudioPlayCnt
                , pstTCB->szAudioFileLen
                , pstTCB->bTraceON
                , pstTCB->bTraceCallON
                , pstTCB->ulTaskID
                , pstTCB->ulCustomID
                , pstTCB->ulCurrentConcurrency
                , pstTCB->usSiteCount
                , pstTCB->ulAgentQueueID
                , sc_debug_make_weeks(pstTCB->astPeriod[0].ucWeekMask, szWeeks, sizeof(szWeeks))
                , pstTCB->astPeriod[0].ucHourBegin
                , pstTCB->astPeriod[0].ucMinuteBegin
                , pstTCB->astPeriod[0].ucHourEnd
                , pstTCB->astPeriod[0].ucMinuteEnd
                , sc_debug_make_weeks(pstTCB->astPeriod[1].ucWeekMask, szWeeks, sizeof(szWeeks))
                , pstTCB->astPeriod[1].ucHourBegin
                , pstTCB->astPeriod[1].ucMinuteBegin
                , pstTCB->astPeriod[1].ucHourEnd
                , pstTCB->astPeriod[1].ucMinuteEnd
                , sc_debug_make_weeks(pstTCB->astPeriod[2].ucWeekMask, szWeeks, sizeof(szWeeks))
                , pstTCB->astPeriod[2].ucHourBegin
                , pstTCB->astPeriod[2].ucMinuteBegin
                , pstTCB->astPeriod[2].ucHourEnd
                , pstTCB->astPeriod[2].ucMinuteEnd
                , sc_debug_make_weeks(pstTCB->astPeriod[3].ucWeekMask, szWeeks, sizeof(szWeeks))
                , pstTCB->astPeriod[3].ucHourBegin
                , pstTCB->astPeriod[3].ucMinuteBegin
                , pstTCB->astPeriod[3].ucHourEnd
                , pstTCB->astPeriod[3].ucMinuteEnd);
    cli_out_string(ulIndex, szCmdBuff);

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff),
                  "\r\n----------------Stat Information----------------");
    cli_out_string(ulIndex, szCmdBuff);

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff)
                , "\r\n       Total Call(s) : %d"
                  "\r\n         Call Failed : %d"
                  "\r\n      Call Connected : %d"
                , pstTCB->ulTotalCall
                , pstTCB->ulCallFailed
                , pstTCB->ulCallConnected);
    cli_out_string(ulIndex, szCmdBuff);

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff),
                  "\r\n------------------------------------------------");
    cli_out_string(ulIndex, szCmdBuff);

}

VOID sc_show_agent_group_detail(U32 ulIndex, U32 ulID)
{
    SC_ACD_GRP_HASH_NODE_ST      *pstAgentGrouop = NULL;
    SC_ACD_AGENT_QUEUE_NODE_ST   *pstAgentQueueNode = NULL;
    HASH_NODE_S  *pstHashNode = NULL;
    DLL_NODE_S   *pstDLLNode  = NULL;
    U32 ulHashIndex = 0;
    S8  szCmdBuff[1024] = { 0 };

    if (U32_BUTT == ulID)
    {
        return;
    }

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nThe detail info for the Agent Group %s:", ulID);
    cli_out_string(ulIndex, szCmdBuff);

    sc_acd_hash_func4grp(ulID, &ulHashIndex);
    pstHashNode = hash_find_node(g_pstGroupList, ulHashIndex, &ulID, sc_acd_grp_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n\r\n\tERROR: Cannot find the agent group with id %s!", ulID);
        cli_out_string(ulIndex, szCmdBuff);
        return;
    }

    pstAgentGrouop = pstHashNode->pHandle;

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n----------Group Parameters----------");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n           Index:%d", pstAgentGrouop->ulGroupID);
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n            Name:%d", pstAgentGrouop->ulGroupID);
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n     Customer ID:%d", pstAgentGrouop->ulCustomID);
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n     Agent Count:%d", pstAgentGrouop->usCount);
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n      ACD Policy:%d", pstAgentGrouop->ucACDPolicy);
    cli_out_string(ulIndex, szCmdBuff);

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n      ACD Policy:%d", pstAgentGrouop->ucACDPolicy);
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n------------------------------------");

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n\r\nGroup Members");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n----------------------------------------------------------------------------------------------------------------");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff)
                    , "\r\n%6s%10s%10s%10d%10s%10s%7s%6s%7s%12s%12s%12s"
                    , "Index", "Status", "ID", "Customer" "Group1", "Group2", "Record", "Trace", "Leader", "SIP Acc" "szExtension", "Emp NO.");
    cli_out_string(ulIndex, szCmdBuff);

    DLL_Scan(&pstAgentGrouop->stAgentList, pstDLLNode, DLL_NODE_S *)
    {
        if (DOS_ADDR_INVALID(pstDLLNode)
            || DOS_ADDR_INVALID(pstDLLNode->pHandle))
        {
            continue;
        }

        pstAgentQueueNode = pstDLLNode->pHandle;
        if (DOS_ADDR_INVALID(pstAgentQueueNode->pstAgentInfo))
        {
            continue;
        }

        dos_snprintf(szCmdBuff, sizeof(szCmdBuff)
                    , "\r\n%6d%10d%10d%10d%10d%7s%6s%7s%12s%12s%12s"
                    , pstAgentQueueNode->ulID
                    , pstAgentQueueNode->pstAgentInfo->usStatus
                    , pstAgentQueueNode->pstAgentInfo->ulSiteID
                    , pstAgentQueueNode->pstAgentInfo->ulCustomerID
                    , pstAgentQueueNode->pstAgentInfo->aulGroupID[0]
                    , pstAgentQueueNode->pstAgentInfo->aulGroupID[1]
                    , pstAgentQueueNode->pstAgentInfo->bRecord ? "Y" : "N"
                    , pstAgentQueueNode->pstAgentInfo->bTraceON ? "Y" : "N"
                    , pstAgentQueueNode->pstAgentInfo->bGroupHeader ? "Y" : "N"
                    , pstAgentQueueNode->pstAgentInfo->szUserID
                    , pstAgentQueueNode->pstAgentInfo->szExtension
                    , pstAgentQueueNode->pstAgentInfo->szEmpNo);
        cli_out_string(ulIndex, szCmdBuff);
    }

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n----------------------------------------------------------------------------------------------------------------");
    cli_out_string(ulIndex, szCmdBuff);
}

VOID sc_show_agent_group(U32 ulIndex, U32 ulCustomID, U32 ulGroupID)
{
    SC_ACD_GRP_HASH_NODE_ST   *pstAgentGrouop = NULL;
    HASH_NODE_S  *pstHashNode = NULL;
    U32 ulHashIndex, ulTotal = 0;
    S8  szCmdBuff[1024] = { 0 };

    if (U32_BUTT != ulGroupID)
    {
        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nList the agents group with the id %d:", ulGroupID);
    }
    else if (U32_BUTT != ulCustomID)
    {
        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nList the agents group own by Customer %d: ", ulCustomID);
    }
    else
    {
        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nList the agents List: ");
    }

    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n----------------------------------------------------------------------------------------------------------------");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff)
                    , "\r\n%6s%10s%10s%10d%10s%16s"
                    , "#", "Index", "Customer", "Agent Cnt" "ACD Policy", "Name");
    cli_out_string(ulIndex, szCmdBuff);

    HASH_Scan_Table(g_pstGroupList, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstGroupList, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            if (DOS_ADDR_INVALID(pstHashNode)
                || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }

            pstAgentGrouop = (SC_ACD_GRP_HASH_NODE_ST *)pstHashNode->pHandle;
            if (DOS_ADDR_INVALID(pstAgentGrouop))
            {
                continue;
            }

            if (U32_BUTT != ulCustomID && pstAgentGrouop->ulCustomID != ulCustomID)
            {
                continue;
            }

            if (U32_BUTT != ulGroupID && pstAgentGrouop->ulGroupID != ulGroupID)
            {
                continue;
            }

            dos_snprintf(szCmdBuff, sizeof(szCmdBuff)
                            , "\r\n%6d%10d%10d%10d%10d%16s"
                            , "#", "Index", "Customer", "Agent Cnt" "ACD Policy", "Name"
                            , pstAgentGrouop->usID
                            , pstAgentGrouop->ulGroupID
                            , pstAgentGrouop->ulCustomID
                            , pstAgentGrouop->usCount
                            , pstAgentGrouop->ucACDPolicy
                            , pstAgentGrouop->szGroupName);
            cli_out_string(ulIndex, szCmdBuff);

            ulTotal++;
        }
    }
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n------------------------------------------------------------------------------------------------------");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nTotal : %d\r\n", ulTotal);
    cli_out_string(ulIndex, szCmdBuff);

}

VOID sc_show_agent(U32 ulIndex, U32 ulID, U32 ulCustomID, U32 ulGroupID)
{
    U32 ulHashIndex, i, blNeddPrint, ulTotal = 0;
    S8  szCmdBuff[1024] = { 0 };
    SC_ACD_AGENT_QUEUE_NODE_ST   *pstAgentQueueNode = NULL;
    HASH_NODE_S  *pstHashNode = NULL;

    if (U32_BUTT != ulGroupID)
    {
        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nList the agents in the Group %d:", ulGroupID);
    }
    else if (U32_BUTT != ulCustomID)
    {
        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nList the agents own by Customer %d: ", ulCustomID);
    }
    else if (U32_BUTT != ulID)
    {
        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nList the agents %d: ", ulID);
    }
    else
    {
        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nList the agents List");
    }

    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n----------------------------------------------------------------------------------------------------------------");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff)
                    , "\r\n%6s%10s%10s%10d%10s%10s%7s%6s%7s%12s%12s%12s"
                    , "Index", "Status", "ID", "Customer" "Group1", "Group2", "Record", "Trace", "Leader", "SIP Acc" "szExtension", "Emp NO.");
    cli_out_string(ulIndex, szCmdBuff);


    HASH_Scan_Table(g_pstAgentList, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstAgentList, ulHashIndex, pstHashNode, HASH_NODE_S*)
        {
            if (DOS_ADDR_INVALID(pstHashNode)
                || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }

            pstAgentQueueNode = (SC_ACD_AGENT_QUEUE_NODE_ST   *)pstHashNode->pHandle;
            if (DOS_ADDR_INVALID(pstAgentQueueNode->pstAgentInfo))
            {
                continue;
            }

            blNeddPrint = DOS_FALSE;

            if (U32_BUTT != ulGroupID)
            {
                blNeddPrint = DOS_FALSE;
                for (i=0; i<MAX_GROUP_PER_SITE; i++)
                {
                    if (pstAgentQueueNode->pstAgentInfo->aulGroupID[i] == ulGroupID)
                    {
                        blNeddPrint = DOS_TRUE;
                        break;
                    }
                }
            }
            else if (U32_BUTT != ulCustomID)
            {
                if (ulCustomID == pstAgentQueueNode->pstAgentInfo->ulCustomerID)
                {
                    blNeddPrint = DOS_TRUE;
                }
            }
            else if (U32_BUTT != ulID)
            {
                if (ulID == pstAgentQueueNode->pstAgentInfo->ulSiteID)
                {
                    blNeddPrint = DOS_TRUE;
                }
            }
            else
            {
                blNeddPrint = DOS_TRUE;
            }

            if (!blNeddPrint)
            {
                continue;
            }

            dos_snprintf(szCmdBuff, sizeof(szCmdBuff)
                        , "\r\n%6d%10d%10d%10d%10d%7s%6s%7s%12s%12s%12s"
                        , pstAgentQueueNode->ulID
                        , pstAgentQueueNode->pstAgentInfo->usStatus
                        , pstAgentQueueNode->pstAgentInfo->ulSiteID
                        , pstAgentQueueNode->pstAgentInfo->ulCustomerID
                        , pstAgentQueueNode->pstAgentInfo->aulGroupID[0]
                        , pstAgentQueueNode->pstAgentInfo->aulGroupID[1]
                        , pstAgentQueueNode->pstAgentInfo->bRecord ? "Y" : "N"
                        , pstAgentQueueNode->pstAgentInfo->bTraceON ? "Y" : "N"
                        , pstAgentQueueNode->pstAgentInfo->bGroupHeader ? "Y" : "N"
                        , pstAgentQueueNode->pstAgentInfo->szUserID
                        , pstAgentQueueNode->pstAgentInfo->szExtension
                        , pstAgentQueueNode->pstAgentInfo->szEmpNo);
            cli_out_string(ulIndex, szCmdBuff);


            ulTotal++;
        }
    }
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n------------------------------------------------------------------------------------------------------");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nTotal : %d\r\n", ulTotal);
    cli_out_string(ulIndex, szCmdBuff);

    return;
}

VOID sc_show_caller_for_task(U32 ulIndex, U32 ulTaskID)
{
    S8 szCmdBuff[1024] = { 0 };
    U32 i=0, ulTotal;
    SC_TASK_CB_ST *pstTCB;

    pstTCB = sc_tcb_find_by_taskid(ulTaskID);
    if (DOS_ADDR_INVALID(pstTCB))
    {
        cli_out_string(ulIndex, "\r\nError:"
                                "\r\n    Invalid task ID"
                                "\r\n    Task with the ID is not valid. "
                                "\r\n    Please use \"sc show task custom id \" to ckeck a valid task\r\n");
        return;
    }

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nList the Caller(s) in the Task %d", ulTaskID);
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n----------------------------------------");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n%6s%6s%10s%16s", "No.", "Trace", "ID", "Number");
    cli_out_string(ulIndex, szCmdBuff);

    for (i=0, ulTotal=0; i<SC_MAX_CALLER_NUM; i++)
    {
        if (DOS_ADDR_INVALID(pstTCB->pstCallerNumQuery)
            || !pstTCB->pstCallerNumQuery[i].bValid)
        {
            continue;
        }

        dos_snprintf(szCmdBuff, sizeof(szCmdBuff)
                        , "\r\n%6d%6s%10d%16s"
                        , pstTCB->pstCallerNumQuery[i].usNo
                        , pstTCB->pstCallerNumQuery[i].bTraceON ? "Y" : "N"
                        , pstTCB->pstCallerNumQuery[i].ulIndexInDB
                        , pstTCB->pstCallerNumQuery[i].szNumber);
        cli_out_string(ulIndex, szCmdBuff);
        ulTotal++;
    }

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n----------------------------------------");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nTotal : %s\r\n",ulTotal);
    cli_out_string(ulIndex, szCmdBuff);
}

VOID sc_show_callee_for_task(U32 ulIndex, U32 ulTaskID)
{
/*
    S8 szCmdBuff[1024] = { 0 };
    S8 szWeeks[64] = { 0 };
    SC_TASK_CB_ST *pstTCB;

    pstTCB = sc_tcb_find_by_taskid(ulTaskID);
    if (DOS_ADDR_INVALID(pstTCB))
    {
        cli_out_string(ulIndex, "\r\nError:"
                                "\r\n    Invalid task ID"
                                "\r\n    Task with the ID is not valid. "
                                "\r\n    Please use \"sc show task custom id \" to ckeck a valid task\r\n");
        return;
    }

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nList the Callee(s) in the Task %d", ulTaskID);
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n----------------------------------------");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n%6s%6s%10s%16s", "No.", "Trace", "ID", "Number");
    cli_out_string(ulIndex, szCmdBuff);
*/
}

VOID sc_show_scb_detail(U32 ulIndex, U32 ulSCBID)
{
    SC_SCB_ST *pstSCB = NULL;
    S8 szCmdBuff[1024] = { 0 };

    if (ulSCBID >= SC_MAX_SCB_NUM
        || !g_pstTaskMngtInfo->pstCallSCBList[ulSCBID].bValid)
    {
        cli_out_string(ulIndex, "\r\nError:"
                        "\r\n    Invalid SCB ID\r\n");
        return;
    }

    pstSCB = &g_pstTaskMngtInfo->pstCallSCBList[ulSCBID];
    cli_out_string(ulIndex, "\r\nList the SCB Information:");

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff)
                   , "\r\n             ID : %d"
                   , "\r\n          Trace : %d"
                   , "\r\n         TCB ID : %d"
                   , "\r\n       Agent ID : %d"
                   , "\r\n         Status : %d"
                   , "\r\n   Service Type : %d, %d, %d, %d"
                   , "\r\n      Custom ID : %s"
                   , "\r\n        Task ID : %d"
                   , "\r\n       Trunk ID : %d"
                   , "\r\n     Hold Count : %d"
                   , "\r\nHold Time Count : %d"
                   , "\r\n  Caller Number : %s"
                   , "\r\n  Callee Number : %s"
                   , "\r\n            ANI : %s"
                   , "\r\n    Site Number : %s"
                   , "\r\n           UUID : %s\r\n"
                   , pstSCB->usSCBNo
                   , pstSCB->bTraceNo ? "Yes" : "No"
                   , pstSCB->usTCBNo
                   , pstSCB->usSiteNo
                   , pstSCB->ucStatus
                   , pstSCB->aucServiceType[0]
                   , pstSCB->aucServiceType[1]
                   , pstSCB->aucServiceType[2]
                   , pstSCB->aucServiceType[3]
                   , pstSCB->ulCustomID
                   , pstSCB->ulTaskID
                   , pstSCB->ulTrunkID
                   , pstSCB->usHoldCnt
                   , pstSCB->usHoldTotalTime
                   , pstSCB->szCallerNum
                   , pstSCB->szCalleeNum
                   , pstSCB->szANINum
                   , pstSCB->szSiteNum
                   , pstSCB->szUUID);
    cli_out_string(ulIndex, szCmdBuff);
}

VOID sc_show_gateway(U32 ulIndex, U32 ulID)
{
    SC_GW_NODE_ST        *pstGWNode     = NULL;
    HASH_NODE_S          *pstHashNode   = NULL;
    S8 szCmdBuff[1024] = { 0 };
    U32 ulHashIndex;

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nList all the gateway:");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n------------------------------------------------");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n%12s%36s", "Index", "Domain");
    cli_out_string(ulIndex, szCmdBuff);

    HASH_Scan_Table(g_pstHashGW, ulHashIndex)
    {
        HASH_Scan_Bucket(g_pstHashGW, ulHashIndex, pstHashNode, HASH_NODE_S *)
        {
            if (DOS_ADDR_INVALID(pstHashNode)
                || DOS_ADDR_INVALID(pstHashNode->pHandle))
            {
                continue;
            }

            pstGWNode = pstHashNode->pHandle;

            if (U32_BUTT != ulID && ulID != pstGWNode->ulGWID)
            {
                continue;
            }

            dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n%12d%36s", pstGWNode->ulGWID, pstGWNode->szGWDomain);
            cli_out_string(ulIndex, szCmdBuff);
        }
    }
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n------------------------------------------------");
}

VOID sc_show_gateway_grp(U32 ulIndex, U32 ulID)
{
    SC_GW_GRP_NODE_ST    *pstGWGrpNode  = NULL;
    SC_GW_NODE_ST        *pstGWNode     = NULL;
    HASH_NODE_S          *pstHashNode   = NULL;
    DLL_NODE_S           *pstDLLNode    = NULL;
    S8 szCmdBuff[1024] = { 0 };
    U32 ulHashIndex;

    ulHashIndex = sc_ep_gw_grp_hash_func(ulID);
    pstHashNode = hash_find_node(g_pstHashGWGrp, ulHashIndex, &ulID, sc_ep_gw_grp_hash_find);
    if (DOS_ADDR_INVALID(pstHashNode)
        || DOS_ADDR_INVALID(pstHashNode->pHandle))
    {
        cli_out_string(ulIndex, "\r\n\tERROR: Invalid gateway group ID while show the gateway group(s).\r\n");
        return;
    }
    pstGWGrpNode = pstHashNode->pHandle;

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\nList the gateway in the gateway group %d:", ulID);
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n------------------------------------------------");
    cli_out_string(ulIndex, szCmdBuff);
    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n%12s%36s", "Index", "Domain");
    cli_out_string(ulIndex, szCmdBuff);

    DLL_Scan(&pstGWGrpNode->stGWList, pstDLLNode, DLL_NODE_S *)
    {
        if (DOS_ADDR_INVALID(pstDLLNode)
            || DOS_ADDR_INVALID(pstDLLNode->pHandle))
        {
            continue;
        }

        pstGWNode = pstDLLNode->pHandle;

        dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n%12d%36s", pstGWNode->ulGWID, pstGWNode->szGWDomain);
        cli_out_string(ulIndex, szCmdBuff);
    }

    dos_snprintf(szCmdBuff, sizeof(szCmdBuff), "\r\n------------------------------------------------");
    cli_out_string(ulIndex, szCmdBuff);
}

S32 sc_debug_call(U32 ulTraceFlag, S8 *pszCaller, S8 *pszCallee)
{
    return 0;
}

S32 cli_cc_trace(U32 ulIndex, S32 argc, S8 **argv)
{
    U32 ulSubMod = SC_SUB_MOD_BUTT;
    U32 ulSubMod1 = SC_SUB_MOD_BUTT;
    U32 ulTraceAll = DOS_FALSE;
    U32 ulID = 0;

    if (argc < 4)
    {
        return -1;
    }

    if (dos_strnicmp(argv[2], "scb", dos_strlen("scb")) == 0)
    {
        if (5 == argc)
        {
            if (dos_strnicmp(argv[3], "all", dos_strlen("all")) == 0
                || dos_atoul(argv[3], &ulID) == 0)
            {
                if (dos_strnicmp(argv[3], "off", dos_strlen("off")) == 0)
                {}
                else if (dos_strnicmp(argv[3], "on", dos_strlen("on")) == 0)
                {}
                else
                {
                    return -1;
                }
            }
            else
            {
                return -1;
            }
        }
        else
        {
            return -1;
        }

        return 0;
    }
    else if (dos_strnicmp(argv[2], "task", dos_strlen("debug")) == 0)
    {
        if (5 == argc)
        {
            if (dos_strnicmp(argv[3], "all", dos_strlen("all")) == 0)
            {
                if (dos_strnicmp(argv[3], "off", dos_strlen("off")) == 0)
                {
                    g_ulTaskTraceAll = 0;
                }
                else if (dos_strnicmp(argv[3], "on", dos_strlen("on")) == 0)
                {
                    g_ulTaskTraceAll = 1;
                }
                else
                {
                    return -1;
                }
            }
            else if (dos_atoul(argv[3], &ulID) == 0)
            {
                SC_TASK_CB_ST *pstTCB = NULL;

                pstTCB = sc_tcb_find_by_taskid(ulID);
                if (DOS_ADDR_INVALID(pstTCB))
                {
                    return -1;
                }

                if (dos_strnicmp(argv[3], "off", dos_strlen("off")) == 0)
                {
                    pstTCB->bTraceON = DOS_FALSE;
                }
                else if (dos_strnicmp(argv[3], "on", dos_strlen("on")) == 0)
                {
                    pstTCB->bTraceON = DOS_TRUE;
                }
                else
                {
                    return -1;
                }
            }
            else
            {
                return -1;
            }
        }
        else
        {
            return -1;
        }

        return 0;
    }
    else if (dos_strnicmp(argv[2], "call", dos_strlen("call")) == 0)
    {
        if (6 == argc)
        {
            if (dos_strnicmp(argv[3], "callee", dos_strlen("callee")) == 0)
            {
                if (dos_strnicmp(argv[5], "off", dos_strlen("off")) == 0)
                {
                    sc_debug_call(DOS_FALSE, NULL, argv[4]);
                }
                else if (dos_strnicmp(argv[5], "on", dos_strlen("on")) == 0)
                {
                    sc_debug_call(DOS_TRUE, NULL, argv[4]);
                }
                else
                {
                    return -1;
                }
            }
            else if (dos_strnicmp(argv[3], "caller", dos_strlen("caller")) == 0)
            {
                if (dos_strnicmp(argv[5], "off", dos_strlen("off")) == 0)
                {
                    sc_debug_call(DOS_FALSE, argv[4], NULL);
                }
                else if (dos_strnicmp(argv[5], "on", dos_strlen("on")) == 0)
                {
                    sc_debug_call(DOS_TRUE, argv[4], NULL);
                }
                else
                {
                    return -1;
                }
            }
            else
            {
                return -1;
            }
        }
        else if (8 == argc)
        {
            S8 *pszCaller = NULL, *pszCallee = NULL;

            if (dos_strnicmp(argv[3], "caller", dos_strlen("caller")) == 0)
            {
                pszCaller = argv[4];
            }
            else if (dos_strnicmp(argv[3], "callee", dos_strlen("callee")) == 0)
            {
                pszCallee = argv[4];
            }
            else
            {
                return -1;
            }

            if (dos_strnicmp(argv[5], "caller", dos_strlen("caller")) == 0)
            {
                pszCaller = argv[6];
            }
            else if (dos_strnicmp(argv[5], "callee", dos_strlen("callee")) == 0)
            {
                pszCallee = argv[6];
            }
            else
            {
                return -1;
            }

            if (dos_strnicmp(argv[5], "off", dos_strlen("off")) == 0)
            {
                sc_debug_call(DOS_FALSE, pszCaller, pszCallee);
            }
            else if (dos_strnicmp(argv[5], "off", dos_strlen("off")) == 0)
            {
                sc_debug_call(DOS_TRUE, pszCaller, pszCallee);
            }
        }
        else
        {
            return -1;
        }

        return 0;
    }
    else
    {
        if (dos_strnicmp(argv[2], "func", dos_strlen("func")) == 0)
        {
            ulSubMod = SC_FUNC;
        }
        else if (dos_strnicmp(argv[2], "http", dos_strlen("http")) == 0)
        {
            ulSubMod = SC_HTTPD;
        }
        else if (dos_strnicmp(argv[2], "api", dos_strlen("api")) == 0)
        {
            ulSubMod = SC_HTTP_API;
        }
        else if (dos_strnicmp(argv[2], "acd", dos_strlen("acd")) == 0)
        {
            ulSubMod = SC_ACD;
        }
        else if (dos_strnicmp(argv[2], "task", dos_strlen("task")) == 0)
        {
            ulSubMod = SC_TASK;
            ulSubMod1 = SC_TASK_MNGT;
        }
        else if (dos_strnicmp(argv[2], "dialer", dos_strlen("dialer")) == 0)
        {
            ulSubMod = SC_DIALER;
        }
        else if (dos_strnicmp(argv[2], "esl", dos_strlen("esl")) == 0)
        {
            ulSubMod = SC_ESL;
        }
        else if (dos_strnicmp(argv[2], "bss", dos_strlen("bss")) == 0)
        {
            ulSubMod = SC_BS;
        }
        else if (dos_strnicmp(argv[2], "all", dos_strlen("bss")) == 0)
        {
            ulTraceAll = DOS_TRUE;
        }

        if (!ulTraceAll && SC_SUB_MOD_BUTT == ulSubMod)
        {
            return -1;
        }

        if (dos_strnicmp(argv[3], "off", dos_strlen("off")) == 0)
        {
            if (ulTraceAll)
            {
                g_ulTraceFlags = 0;
            }
            else
            {
                if (SC_SUB_MOD_BUTT != ulSubMod)
                {
                    SC_NODEBUG_SUBMOD(g_ulTraceFlags, ulSubMod);
                }

                if (SC_SUB_MOD_BUTT != ulSubMod1)
                {
                    SC_NODEBUG_SUBMOD(g_ulTraceFlags, ulSubMod1);
                }
            }
        }
        else if (dos_strnicmp(argv[3], "on", dos_strlen("on")) == 0)
        {
            if (ulTraceAll)
            {
                g_ulTraceFlags = 0xFFFFFFFF;
            }
            else
            {
                if (SC_SUB_MOD_BUTT != ulSubMod)
                {
                    SC_DEBUG_SUBMOD(g_ulTraceFlags, ulSubMod);
                }

                if (SC_SUB_MOD_BUTT != ulSubMod1)
                {
                    SC_DEBUG_SUBMOD(g_ulTraceFlags, ulSubMod1);
                }
            }
        }
        else
        {
            return -1;
        }
    }

    return 0;
}

S32 cli_cc_show(U32 ulIndex, S32 argc, S8 **argv)
{
    U32 ulID;

    if (argc < 3)
    {
        return -1;
    }

    if (dos_strnicmp(argv[2], "httpd", dos_strlen("httpd")) == 0)
    {
        if (3 == argc)
        {
            sc_show_httpd(ulIndex, U32_BUTT);
        }
        else if (4 == argc)
        {
            if (dos_atoul(argv[3], &ulID) == 0)
            {
                sc_show_httpd(ulIndex, ulID);
            }
            else
            {
                cli_out_string(ulIndex, "\r\n\tERROR: Invalid HTTPD ID while show the HTTPDS.\r\n");
                return -1;
            }
        }
    }
    else if (dos_strnicmp(argv[2], "http", dos_strlen("http")) == 0)
    {
        if (3 == argc)
        {
            sc_show_http(ulIndex, U32_BUTT);
        }
        else if (4 == argc)
        {
            if (dos_atoul(argv[3], &ulID) == 0)
            {
                sc_show_http(ulIndex, ulID);
            }
            else
            {
                cli_out_string(ulIndex, "\r\n\tERROR: Invalid HTTP Clinet ID while show the HTTPS.\r\n");
                return -1;
            }
        }
    }
    else if (dos_strnicmp(argv[2], "gateway", dos_strlen("gateway")) == 0)
    {
        if (3 == argc)
        {
            sc_show_gateway(ulIndex, U32_BUTT);
        }
        else if (4 == argc)
        {
            if (dos_atoul(argv[3], &ulID) == 0)
            {
                sc_show_gateway(ulIndex, ulID);
            }
            else
            {
                cli_out_string(ulIndex, "\r\n\tERRNO: Invalid gateway ID while show the gateway(s).\r\n");
                return -1;
            }
        }
    }
    else if (dos_strnicmp(argv[2], "gwgrp", dos_strlen("gwgrp")) == 0)
    {
        if (4 == argc)
        {
            if (dos_atoul(argv[3], &ulID) == 0)
            {
                sc_show_gateway_grp(ulIndex, ulID);
            }
            else
            {
                cli_out_string(ulIndex, "\r\n\tERROR: Invalid gateway group ID while show the gateway group(s).\r\n");
                return -1;
            }
        }
        else
        {
            return -1;
        }
    }
    else if (dos_strnicmp(argv[2], "scb", dos_strlen("scb")) == 0)
    {
        if (3 == argc)
        {
            sc_show_scb(ulIndex, U32_BUTT);
        }
        else if (4 == argc)
        {
            if (dos_atoul(argv[3], &ulID) == 0)
            {
                sc_show_scb(ulIndex, ulID);
            }
            else
            {
                cli_out_string(ulIndex, "\r\n\tERROR: Invalid SCB ID while show the SCB.\r\n");
                return -1;
            }
        }
    }
    else if (dos_strnicmp(argv[2], "task", dos_strlen("task")) == 0)
    {
        if (3 == argc)
        {
            sc_show_task(ulIndex, U32_BUTT, U32_BUTT);
        }
        else if (4 == argc)
        {
            if (dos_atoul(argv[3], &ulID) == 0)
            {
                sc_show_task(ulIndex, ulID, U32_BUTT);
            }
            else
            {
                cli_out_string(ulIndex, "\r\n\tERROR: Invalid Task ID while show the Task(s).\r\n");
                return -1;
            }
        }
        else if (5 == argc)
        {
            if (dos_atoul(argv[4], &ulID) == 0)
            {
                cli_out_string(ulIndex, "\r\n\tERROR: Invalid Customer ID while show the Task(s).\r\n");
                return -1;
            }

            if (dos_strnicmp(argv[2], "custom", dos_strlen("custom")) == 0)
            {
                sc_show_task(ulIndex, U32_BUTT, ulID);
            }
            else
            {
                return -1;
            }
        }
        else
        {
            return -1;
        }
    }
    else if (dos_strnicmp(argv[2], "caller", dos_strlen("caller")) == 0)
    {
        if (4 == argc)
        {
            if (dos_atoul(argv[3], &ulID) == 0)
            {
                sc_show_caller_for_task(ulIndex, ulID);
            }
            else
            {
                cli_out_string(ulIndex, "\r\n\tERROR: Invalid Task ID while show the Caller(s).\r\n");
                return -1;
            }
        }
        else
        {
            return -1;
        }
    }
    else if (dos_strnicmp(argv[2], "callee", dos_strlen("callee")) == 0)
    {
        if (4 == argc)
        {
            if (dos_atoul(argv[3], &ulID) == 0)
            {
                sc_show_callee_for_task(ulIndex, ulID);
            }
            else
            {
                cli_out_string(ulIndex, "\r\n\tERROR: Invalid Task ID while show the Callee(s).\r\n");
                return -1;
            }
        }
        else
        {
            return -1;
        }
    }
    else if (dos_strnicmp(argv[2], "agent", dos_strlen("agent")) == 0)
    {
        if (3 == argc)
        {
            sc_show_agent(ulIndex, U32_BUTT, U32_BUTT, U32_BUTT);
        }
        else if (4 == argc)
        {
            if (dos_atoul(argv[3], &ulID) == 0)
            {
                sc_show_agent(ulIndex, ulID, U32_BUTT, U32_BUTT);
            }
            else
            {
                cli_out_string(ulIndex, "\r\n\tERROR: Invalid Agent Group ID while show the Agent Group(s).\r\n");
                return -1;
            }
        }
        else if (6 == argc)
        {
            if (dos_atoul(argv[5], &ulID) < 0)
            {
                cli_out_string(ulIndex, "\r\n\tERROR: Invalid Agent Group ID while show the Agent Group(s).\r\n");
                return -1;
            }

            if (dos_strnicmp(argv[4], "custom", dos_strlen("custom")) == 0)
            {
                sc_show_agent(ulIndex, U32_BUTT, ulID, U32_BUTT);
            }
            else if (dos_strnicmp(argv[4], "group", dos_strlen("group")) == 0)
            {
                sc_show_agent(ulIndex, U32_BUTT, U32_BUTT, ulID);
            }
            else
            {
                return -1;
            }
        }
        else
        {
            return -1;
        }
    }
    else if (dos_strnicmp(argv[2], "agentgrp", dos_strlen("agentgrp")) == 0)
    {
        if (3 == argc)
        {
            sc_show_agent_group(ulIndex, U32_BUTT, U32_BUTT);
        }
        else if (4 == argc)
        {
            if (dos_atoul(argv[3], &ulID) == 0)
            {
                sc_show_agent_group_detail(ulIndex, ulID);
            }
            else
            {
                cli_out_string(ulIndex, "\r\n\tERROR: Invalid Agent Group ID while show the Agent Group(s).\r\n");
                return -1;
            }
        }
        else if (6 == argc)
        {
            if (dos_atoul(argv[5], &ulID) < 0)
            {
                cli_out_string(ulIndex, "\r\n\tERROR: Invalid Agent Group ID while show the Agent Group(s).\r\n");
                return -1;
            }

            if (dos_strnicmp(argv[4], "custom", dos_strlen("custom")) == 0)
            {
                sc_show_agent_group(ulIndex, ulID, U32_BUTT);
            }
            else if (dos_strnicmp(argv[4], "group", dos_strlen("group")) == 0)
            {
                sc_show_agent_group(ulIndex, U32_BUTT, ulID);
            }
            else
            {
                return -1;
            }
        }
        else
        {
            return -1;
        }
    }

    return 0;
}

S32 cli_cc_debug(U32 ulIndex, S32 argc, S8 **argv)
{
    U32 ulLogLevel;

    if (argc < 3)
    {
        return -1;
    }

    if (dos_strnicmp(argv[2], "debug", dos_strlen("debug")) == 0)
    {
        ulLogLevel = LOG_LEVEL_DEBUG;
    }
    else if (dos_strnicmp(argv[2], "info", dos_strlen("info")) == 0)
    {
        ulLogLevel = LOG_LEVEL_INFO;
    }
    else if (dos_strnicmp(argv[2], "notice", dos_strlen("notice")) == 0)
    {
        ulLogLevel = LOG_LEVEL_NOTIC;
    }
    else if (dos_strnicmp(argv[2], "warning", dos_strlen("warning")) == 0)
    {
        ulLogLevel = LOG_LEVEL_WARNING;
    }
    else if (dos_strnicmp(argv[2], "error", dos_strlen("error")) == 0)
    {
        ulLogLevel = LOG_LEVEL_ERROR;
    }
    else if (dos_strnicmp(argv[2], "cirt", dos_strlen("cirt")) == 0)
    {
        ulLogLevel = LOG_LEVEL_CIRT;
    }
    else if (dos_strnicmp(argv[2], "alert", dos_strlen("alert")) == 0)
    {
        ulLogLevel = LOG_LEVEL_ALERT;
    }
    else if (dos_strnicmp(argv[2], "emerg", dos_strlen("emerg")) == 0)
    {
        ulLogLevel = LOG_LEVEL_EMERG;
    }

    if (LOG_LEVEL_INVAILD == ulLogLevel)
    {
        return -1;
    }

    g_ulSCLogLevel = ulLogLevel;

    return 0;
}

S32 cli_cc_process(U32 ulIndex, S32 argc, S8 **argv)
{
    if (DOS_ADDR_INVALID(argv))
    {
        goto cc_usage;
    }

    if (argc < 2)
    {
        goto cc_usage;
    }

    if (dos_strnicmp(argv[1], "debug", dos_strlen("debug")) == 0)
    {
        if (cli_cc_debug(ulIndex, argc, argv) < 0)
        {
            goto cc_usage;
        }
        else
        {
            cli_out_string(ulIndex, "Set debug level successfully.\r\n");
        }
    }
    else if (dos_strnicmp(argv[1], "show", dos_strlen("show")) == 0)
    {
        if (cli_cc_show(ulIndex, argc, argv) < 0)
        {
            goto cc_usage;
        }
    }
    else if (dos_strnicmp(argv[1], "trace", dos_strlen("trace")) == 0)
    {
        if (cli_cc_trace(ulIndex, argc, argv) < 0)
        {
            goto cc_usage;
        }
    }
    else
    {
        goto cc_usage;
    }

    return 0;

cc_usage:


    cli_out_string(ulIndex, "\r\n");
    cli_out_string(ulIndex, "cc show gwgrp id\r\n");
    cli_out_string(ulIndex, "cc show httpd|http|gateway|gwgrp|scb [id]\r\n");
    cli_out_string(ulIndex, "cc show task [custom] id\r\n");
    cli_out_string(ulIndex, "cc show caller|callee taskid\r\n");
    cli_out_string(ulIndex, "cc show agent|agentgrp [custom|group] id\r\n");
    cli_out_string(ulIndex, "cc debug debug|info|notice|warning|error|cirt|alert|emerg\r\n");
    cli_out_string(ulIndex, "cc trace func|http|api|acd|task|dialer|esl|bss|all on|off\r\n");
    cli_out_string(ulIndex, "cc trace scb scbid|all on|off\r\n");
    cli_out_string(ulIndex, "cc trace task taskid|all on|off\r\n");
    cli_out_string(ulIndex, "cc trace call <callee num> <caller num> on|off\r\n\r\n");

    return 0;
}


VOID sc_debug(U32 ulSubMod, U32 ulLevel, const S8* szFormat, ...)
{
    va_list         Arg;
    U32             ulTraceTagLen;
    S8              szTraceStr[1024];
    BOOL            bIsOutput = DOS_FALSE;

    if (ulLevel >= LOG_LEVEL_INVAILD)
    {
        return;
    }

    /* warning级别以上强制输出 */
    if (ulLevel <= LOG_LEVEL_WARNING)
    {
        bIsOutput = DOS_TRUE;
    }

    if (!bIsOutput
        && ulSubMod >= SC_SUB_MOD_BUTT)
    {
        bIsOutput = DOS_TRUE;
    }

    if (!bIsOutput
        && SC_CHECK_SUBMOD(g_ulTraceFlags, ulSubMod))
    {
        bIsOutput = DOS_TRUE;
    }

    if (!bIsOutput
        && ulLevel <= g_ulSCLogLevel)
    {
        bIsOutput = DOS_TRUE;
    }

    if(!bIsOutput)
    {
        return;
    }

    switch(ulSubMod)
    {
        case SC_FUNC:
            dos_snprintf(szTraceStr, sizeof(szTraceStr), "SC_FNC:");
            break;
        case SC_HTTPD:
            dos_snprintf(szTraceStr, sizeof(szTraceStr), "SC_HTD:");
            break;
        case SC_HTTP_API:
            dos_snprintf(szTraceStr, sizeof(szTraceStr), "SC_API:");
            break;
        case SC_ACD:
            dos_snprintf(szTraceStr, sizeof(szTraceStr), "SC_ACD:");
            break;
        case SC_TASK_MNGT:
            dos_snprintf(szTraceStr, sizeof(szTraceStr), "SC_TSK:");
            break;
        case SC_TASK:
            dos_snprintf(szTraceStr, sizeof(szTraceStr), "SC_TSK:");
            break;
        case SC_DIALER:
            dos_snprintf(szTraceStr, sizeof(szTraceStr), "SC_DLR:");
            break;
        case SC_ESL:
            dos_snprintf(szTraceStr, sizeof(szTraceStr), "SC_ESL:");
            break;
        case SC_BS:
            dos_snprintf(szTraceStr, sizeof(szTraceStr), "SC_BSS:");
            break;
        default:
            dos_snprintf(szTraceStr, sizeof(szTraceStr), "SC:");
            break;
    }

    ulTraceTagLen = dos_strlen(szTraceStr);

    va_start(Arg, szFormat);
    vsnprintf(szTraceStr + ulTraceTagLen, sizeof(szTraceStr) - ulTraceTagLen, szFormat, Arg);
    va_end(Arg);
    szTraceStr[sizeof(szTraceStr) -1] = '\0';

    //printf("%s\r\n", szTraceStr);
    dos_log(ulLevel, LOG_TYPE_RUNINFO, szTraceStr);
}


VOID sc_call_trace(SC_SCB_ST *pstSCB, const S8 *szFormat, ...)
{
    SC_TASK_CB_ST                 *pstTCB      = NULL;
    SC_CALLER_QUERY_NODE_ST       *pstCaller   = NULL;
    va_list argptr;
    char szBuf[1024];

    SC_TRACE_IN((U64)pstSCB, 0, 0, 0);

    if (!pstSCB)
    {
        SC_TRACE_OUT();
        return;
    }

    if (pstSCB->usTCBNo <= SC_MAX_TASK_NUM)
    {
        pstTCB = sc_tcb_get_by_id(pstSCB->usTCBNo);
    }

    if (g_ulCallTraceAll)
    {
        goto trace;
    }

    if (pstTCB && pstTCB->bTraceCallON)
    {
        goto trace;
    }

    if (pstCaller && pstCaller->bTraceON)
    {
        goto trace;
    }

    return;

trace:
    va_start(argptr, szFormat);
    vsnprintf(szBuf, sizeof(szBuf), szFormat, argptr);
    va_end(argptr);
    /*
     *格式: <SCB:No, status, token, caller, callee, uuid, trunk><TCB:No, status, ID, Custom,><CALLER:No, num,><SITE:No, status, SIP, id, externsion>
     */
    sc_logr_debug(SC_ESL, "Call Trace:%s\r\n"
                  "\t[SCB Info]: No:%05d, status: %d, caller: %s, callee: %s, uuid:%s\r\n"
                  "\t[TCB Info]: No:%05d, status: %d, Task ID: %d, Custom ID: %d\r\n"
                  , szBuf
                  , pstSCB->usSCBNo
                  , pstSCB->bValid
                  , ('\0' == pstSCB->szCallerNum[0]) ? "NULL" : pstSCB->szCallerNum
                  , ('\0' == pstSCB->szCalleeNum[0]) ? "NULL" : pstSCB->szCalleeNum
                  , ('\0' == pstSCB->szUUID[0]) ? "NULL" : pstSCB->szUUID
                  , pstTCB ? pstTCB->usTCBNo : -1
                  , pstTCB ? pstTCB->ucValid : -1
                  , pstTCB ? pstTCB->ulTaskID : -1
                  , pstTCB ? pstTCB->ulCustomID : -1);
}

VOID sc_task_trace(SC_TASK_CB_ST *pstTCB, const S8* szFormat, ...)
{
    SC_TRACE_IN((U64)pstTCB, 0, 0, 0);

    if (!pstTCB)
    {
        SC_TRACE_OUT();
        return;
    }

    if (!pstTCB->bTraceON && !g_ulTaskTraceAll)
    {
        SC_TRACE_OUT();
        return;
    }

    /*
     * 格式: <任务控制块号，任务ID, 用户ID, 状态>
     **/
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

