/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  �ļ���: sc_api_process.c
 *
 *  ����ʱ��: 2014��12��16��10:18:20
 *  ��    ��: Larry
 *  ��    ��: ����HTTP���󣬲���HTTP��������Я��������ַ�����������ģ��
 *  �޸���ʷ:
 */
#if 0
#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */
#endif

/* include public header files */
#include <dos.h>
#include <pthread.h>
#include <semaphore.h>


/* include private header files */
#include "sc_pub.h"
#include "sc_task_pub.h"
#include "sc_debug.h"
#include "sc_httpd_pub.h"
#include "sc_http_api.h"
#include "sc_acd_pub.h"

extern U32 g_ulTaskTraceAll;

U32 sc_http_api_reload_xml(SC_HTTP_CLIENT_CB_S *pstClient);
U32 sc_http_api_task_ctrl(SC_HTTP_CLIENT_CB_S *pstClient);
U32 sc_http_api_num_verify(SC_HTTP_CLIENT_CB_S *pstClient);
U32 sc_http_api_call_ctrl(SC_HTTP_CLIENT_CB_S *pstClient);
U32 sc_http_api_agent_action(SC_HTTP_CLIENT_CB_S *pstClient);


/* global parameters */
SC_HTTP_REQ_REG_TABLE_SC g_pstHttpReqRegTable[] =
{
    {"reload",                   sc_http_api_reload_xml},
    {"task",                     sc_http_api_task_ctrl},
    {"verify",                   sc_http_api_num_verify},
    {"callctrl",                 sc_http_api_call_ctrl},
    {"agent-action",             sc_http_api_agent_action},

    {"",                         NULL}
};


/* declare functions */
/**
 * ����: S8 *sc_http_api_get_value(list_t *pstParamList, S8 *pszKey)
 * ����:
 *      ��http������ƿ�pstParamList�л�ȡ��Ӧ�ֶ�pszKey��ֵ
 * ����:
 *      list_t *pstParamList: �����б�
 *      S8 *pszKey: ������
 * ����ֵ: ����ҵ���ز������ͷ��ظò�������ָ�룬���򷵻�NULL
 */
static S8 *sc_http_api_get_value(list_t *pstParamList, S8 *pszKey)
{
    SC_API_PARAMS_ST *pstParamListNode = NULL;
    list_t           *pstLineNode = pstParamList;

    SC_TRACE_IN((U64)pstParamList, (U64)pszKey, 0, 0);

    if (!pstParamList || !pszKey)
    {
        DOS_ASSERT(0);
        SC_TRACE_OUT();
        return NULL;
    }

    while (1)
    {
        if (dos_list_is_empty(pstParamList))
        {
            break;
        }

        pstLineNode = dos_list_work(pstParamList, pstLineNode);
        if (!pstLineNode)
        {
            break;
        }

        pstParamListNode = dos_list_entry(pstLineNode, SC_API_PARAMS_ST, stList);

        if (pstParamListNode
            && 0 == dos_strcmp(pstParamListNode->pszStringName, pszKey))
        {
            SC_TRACE_OUT();
            return pstParamListNode->pszStringVal;
        }
    }

    SC_TRACE_OUT();
    return NULL;
}

U32 sc_http_api_reload_xml(SC_HTTP_CLIENT_CB_S *pstClient)
{
    U32 ulCustomID, ulAction, ulGatewayID, ulDialplan;
    S8  *pszCustomID, *pszAction, *pszGatewayID, *pszDialplan;;

    pszCustomID = sc_http_api_get_value(&pstClient->stParamList, "userid");
    if (!pszCustomID)
    {
        ulCustomID = U32_BUTT;

        goto invalid_params;
    }
    else
    {
        if (dos_atoul(pszCustomID, &ulCustomID) < 0)
        {
            ulCustomID = U32_BUTT;
        }
    }

    pszAction = sc_http_api_get_value(&pstClient->stParamList, "action");
    if (!pszAction)
    {
        ulAction = SC_API_CMD_ACTION_BUTT;
    }
    else
    {
        ulAction = SC_API_CMD_ACTION_BUTT;

        if (0 == dos_strncmp(pszAction, "pause", dos_strlen("pause")))
        {
            ulAction = SC_API_CMD_ACTION_PAUSE;
        }
        else if (0 == dos_strncmp(pszAction, "monitoring", dos_strlen("monitoring")))
        {
            ulAction = SC_API_CMD_ACTION_MONITORING;
        }
        else if (0 == dos_strncmp(pszAction, "hungup", dos_strlen("hungup")))
        {
            ulAction = SC_API_CMD_ACTION_HUNGUP;
        }
    }

    pszGatewayID = sc_http_api_get_value(&pstClient->stParamList, "gateway");
    if (!pszGatewayID)
    {
        ulGatewayID = U32_BUTT;
    }
    else
    {
        if (dos_atoul(pszGatewayID, &ulGatewayID) < 0)
        {
            ulGatewayID = U32_BUTT;
        }
    }

    pszDialplan = sc_http_api_get_value(&pstClient->stParamList, "dialplan");
    if (!pszDialplan)
    {
        ulDialplan = U32_BUTT;
    }
    else
    {
        if (dos_atoul(pszDialplan, &ulDialplan) < 0)
        {
            ulDialplan = U32_BUTT;
        }
    }

    return DOS_SUCC;

invalid_params:
    pstClient->ulResponseCode = 200;
    pstClient->ulErrCode = SC_HTTP_ERRNO_INVALID_PARAM;

    SC_TRACE_OUT();
    return DOS_FAIL;

}

U32 sc_http_api_task_ctrl(SC_HTTP_CLIENT_CB_S *pstClient)
{
    S8 *pszCustomID;
    S8 *pszTaskID;
    S8 *pszAction;
    U32 ulCustomID, ulTaskID, ulAction;
    SC_TASK_CTRL_CMD_ST *pstCMD = NULL;

    if (DOS_ADDR_INVALID(pstClient))
    {
        DOS_ASSERT(0);

        return SC_HTTP_ERRNO_INVALID_REQUEST;
    }

    SC_TRACE_IN(pstClient, 0, 0, 0);
/*
    pszCMD = sc_http_api_get_value(&pstClient->stParamList, "cmd");
    if (!pszCMD || '\0' == pszCMD[0])
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }
    if (dos_strcmp(pszCMD, "task") != 0)
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }
*/

    pszCustomID = sc_http_api_get_value(&pstClient->stParamList, "userid");
    if (!pszCustomID || '\0' == pszCustomID[0])
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }
    if (dos_atoul(pszCustomID, &ulCustomID) < 0)
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }

    pszTaskID = sc_http_api_get_value(&pstClient->stParamList, "task");
    if (!pszTaskID || '\0' == pszTaskID[0])
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }
    if (dos_atoul(pszTaskID, &ulTaskID) < 0)
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }

    pszAction = sc_http_api_get_value(&pstClient->stParamList, "action");
    if (!pszAction || '\0' == pszAction[0])
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }
    if (dos_strcmp(pszAction, "start") == 0)
    {
        ulAction = SC_API_CMD_ACTION_START;
    }
    else if (dos_strcmp(pszAction, "stop") == 0)
    {
        ulAction = SC_API_CMD_ACTION_STOP;
    }
    else if (dos_strcmp(pszAction, "pause") == 0)
    {
        ulAction = SC_API_CMD_ACTION_PAUSE;
    }
    else if (dos_strcmp(pszAction, "continue") == 0)
    {
        ulAction = SC_API_CMD_ACTION_CONTINUE;
    }
    else
    {
        DOS_ASSERT(0);
        goto invalid_params;
    }

    /* ��ʼ�������� */
    pstCMD = (SC_TASK_CTRL_CMD_ST *)dos_dmem_alloc(sizeof(SC_TASK_CTRL_CMD_ST));
    if (!pstCMD)
    {
        DOS_ASSERT(0);

        goto exec_fail;
    }
    dos_memzero(pstCMD, sizeof(SC_TASK_CTRL_CMD_ST));

    pstCMD->ulCustomID = ulCustomID;
    pstCMD->ulTaskID = ulTaskID;
    pstCMD->ulCMD = SC_API_CMD_TASK_CTRL;
    pstCMD->ulAction = ulAction;
    pstCMD->ulCMDErrCode = U32_BUTT;
    sem_init(&pstCMD->semCMDExecNotify, 0, 0);

    if (sc_task_cmd_queue_add(pstCMD) != DOS_SUCC)
    {
        DOS_ASSERT(0);

        goto exec_fail;
    }

    /* TODO: ��Ҫ���õȴ�ʱ�䣬�����ʱ���Ͳ�Ҫ�ٵȴ��� */
    sem_wait(&pstCMD->semCMDExecNotify);
#if 0
    if (U32_BUTT == pstCMD->ulCMDErrCode)
    {
        DOS_ASSERT(0);

        goto exec_fail;
    }
#endif
    pstClient->ulResponseCode = 200;
    pstClient->ulErrCode = pstCMD->ulCMDErrCode;

    if (sc_task_cmd_queue_del(pstCMD) == DOS_SUCC)
    {
        if (pstCMD->pszErrMSG)
        {
            dos_dmem_free(pstCMD->pszErrMSG);
        }
        pstCMD->pszErrMSG = NULL;

        dos_dmem_free(pstCMD);
        pstCMD = NULL;
    }
    else
    {
        pstClient->ulResponseCode = 200;
        pstClient->ulErrCode = SC_HTTP_ERRNO_CMD_EXEC_FAIL;
        DOS_ASSERT(0);
    }

    SC_TRACE_OUT();
    return DOS_TRUE;

invalid_params:
    pstClient->ulResponseCode = 200;
    pstClient->ulErrCode = SC_HTTP_ERRNO_INVALID_PARAM;

    SC_TRACE_OUT();
    return DOS_FAIL;

exec_fail:
    pstClient->ulResponseCode = 200;
    pstClient->ulErrCode = SC_HTTP_ERRNO_CMD_EXEC_FAIL;

    if (pstCMD->pszErrMSG)
    {
        dos_dmem_free(pstCMD->pszErrMSG);
    }
    pstCMD->pszErrMSG = NULL;

    dos_dmem_free(pstCMD);
    pstCMD = NULL;

    SC_TRACE_OUT();
    return DOS_FAIL;
}

U32 sc_http_api_num_verify(SC_HTTP_CLIENT_CB_S *pstClient)
{
    return DOS_FAIL;
}

U32 sc_http_api_call_ctrl(SC_HTTP_CLIENT_CB_S *pstClient)
{
    return DOS_FAIL;
}

U32 sc_http_api_agent_action(SC_HTTP_CLIENT_CB_S *pstClient)
{
    S8 *pszGrpID      = NULL;
    S8 *pszAgentID    = NULL;
    S8 *pszExtension  = NULL;
    S8 *pszAction     = NULL;
    U32 ulGrpID = U32_BUTT, ulAgentID = U32_BUTT, ulAction = U32_BUTT;

    if (DOS_ADDR_INVALID(pstClient))
    {
        DOS_ASSERT(0);

        return SC_HTTP_ERRNO_INVALID_REQUEST;
    }

    SC_TRACE_IN(pstClient, 0, 0, 0);


    pszGrpID = sc_http_api_get_value(&pstClient->stParamList, "grpid");
    pszAgentID = sc_http_api_get_value(&pstClient->stParamList, "agentid");
    pszExtension = sc_http_api_get_value(&pstClient->stParamList, "extension");
    pszAction = sc_http_api_get_value(&pstClient->stParamList, "action");
    if (DOS_ADDR_INVALID(pszGrpID)
        || DOS_ADDR_INVALID(pszAgentID)
        || DOS_ADDR_INVALID(pszExtension)
        || DOS_ADDR_INVALID(pszAction))
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_REQUEST;
    }

    if (dos_atoul(pszGrpID, &ulGrpID) < 0
        || dos_atoul(pszAgentID, &ulAgentID) < 0)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_REQUEST;
    }

    if (dos_strncmp(pszAction, "signin",sizeof("signin")) == 0)
    {
        ulAction = SC_ACD_SITE_ACTION_SIGNIN;
    }
    else if (dos_strncmp(pszAction, "signout",sizeof("signout")) == 0)
    {
        ulAction = SC_ACD_SITE_ACTION_SIGNOUT;
    }
    else if (dos_strncmp(pszAction, "online",sizeof("online")) == 0)
    {
        ulAction = SC_ACD_SITE_ACTION_ONLINE;
    }
    else if (dos_strncmp(pszAction, "offline",sizeof("offline")) == 0)
    {
        ulAction = SC_ACD_SITE_ACTION_OFFLINE;
    }
    else if (dos_strncmp(pszAction, "update",sizeof("update")) == 0)
    {
        ulAction = SC_ACD_SITE_ACTION_UPDATE;
    }
    else if (dos_strncmp(pszAction, "add",sizeof("add")) == 0)
    {
        ulAction = SC_ACD_SITE_ACTION_ADD;
    }
    else if (dos_strncmp(pszAction, "delete",sizeof("delete")) == 0)
    {
        ulAction = SC_ACD_SITE_ACTION_DELETE;
    }
    else
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_INVALID_REQUEST;
    }

    if (sc_acd_http_req_proc(ulAction, ulGrpID, ulAgentID, pszExtension) != DOS_SUCC)
    {
        DOS_ASSERT(0);
        return SC_HTTP_ERRNO_CMD_EXEC_FAIL;
    }

    return SC_HTTP_ERRNO_SUCC;
}

U32 sc_http_api_process(SC_HTTP_CLIENT_CB_S *pstClient)
{
    S8        *pStart = NULL, *pEnd = NULL;
    S8        *pszKeyWord[SC_API_PARAMS_MAX_NUM] = { 0 };
    S8        *pWord = NULL, *pValue = NULL;
    S8        szReqBuffer[SC_HTTP_REQ_MAX_LEN] = { 0 };
    S8        szReqLine[1024] = { 0 };
    S32       lKeyCnt = 0, lParamIndex = 0;
    U32       ulIndex, ulRet;
    list_t    *pstParamListNode;
    SC_API_PARAMS_ST *pstParamsList;

    SC_TRACE_IN((U64)pstClient,0,0,0);

    if (!pstClient)
    {
        DOS_ASSERT(0);
        goto cmd_prase_fail1;
    }

    if (!pstClient->ulValid)
    {
        DOS_ASSERT(0);
        goto cmd_prase_fail1;
    }

    if (!pstClient->stDataBuff.ulLength || !pstClient->stDataBuff.pszBuff)
    {
        DOS_ASSERT(0);
        goto cmd_prase_fail1;
    }

    sc_logr_debug("HTTP Request: %s", pstClient->stDataBuff.pszBuff);

    /* ��ȡ������ļ� */
    pStart = dos_strstr(pstClient->stDataBuff.pszBuff, "GET /");
    if (!pStart)
    {
        DOS_ASSERT(0);
        goto cmd_prase_fail1;
    }
    pStart += dos_strlen("GET /");
    pEnd = dos_strstr(pStart, "?");
    if (!pStart || !pEnd
        || pEnd <= pStart)
    {
        DOS_ASSERT(0);
        goto cmd_prase_fail1;
    }
    dos_strncpy(szReqBuffer, pStart, pEnd-pStart);
    szReqBuffer[pEnd - pStart] = '\0';
    if ('\0' == szReqBuffer[0])
    {
        DOS_ASSERT(0);
        goto cmd_prase_fail1;
    }

    /* ��ȡ�����в��� */
    pStart = dos_strstr(pstClient->stDataBuff.pszBuff, "?");
    pEnd = dos_strstr(pstClient->stDataBuff.pszBuff, " HTTP/1.");
    if (!pStart || !pEnd)
    {
        DOS_ASSERT(0);
        goto cmd_prase_fail1;
    }

    /* ��ȡ������� */
    pStart += dos_strlen("?");
    if (pEnd - pStart >= sizeof(szReqLine))
    {
        DOS_ASSERT(0);
        goto cmd_prase_fail1;
    }
    dos_strncpy(szReqLine, pStart, pEnd - pStart);
    szReqLine[pEnd - pStart] = '\0';

    sc_logr_debug("HTTP Request Line: %s?%s", szReqBuffer, szReqLine);

    /* ��ȡ key=value �ַ��� */
    lKeyCnt = 0;
    pWord = strtok(szReqLine, "&");
    while (pWord)
    {
        pszKeyWord[lKeyCnt] = dos_dmem_alloc(dos_strlen(pWord) + 1);
        if (!pszKeyWord[lKeyCnt])
        {
            logr_warning("%s", "Alloc fail.");
            break;
        }

        dos_strcpy(pszKeyWord[lKeyCnt], pWord);
        lKeyCnt++;
        pWord = strtok(NULL, "&");
        if (NULL == pWord)
        {
            break;
        }
    }

    if (lKeyCnt<= 0)
    {
        DOS_ASSERT(0);
        goto cmd_prase_fail1;
    }

    sc_logr_debug("%s", "Start prase the http request.");

    /* ����key=value����������������� */
    dos_list_init(&pstClient->stParamList);
    for (lParamIndex=0; lParamIndex<lKeyCnt; lParamIndex++)
    {
        if(!pszKeyWord[lParamIndex])
        {
            continue;
        }

        sc_logr_debug("Process Token: %s", pszKeyWord[lParamIndex]);

        pWord = dos_strstr(pszKeyWord[lParamIndex], "=");
        pValue = pWord;
        if (!pValue)
        {
            continue;
        }
        pValue++;
        if (!pValue)
        {
            continue;
        }
        *pWord = '\0';

        pstParamsList = (SC_API_PARAMS_ST *)dos_dmem_alloc(sizeof(SC_API_PARAMS_ST));
        if (!pstParamsList)
        {
            DOS_ASSERT(0);
            continue;
        }

        /*
         * pValue ָ����� pszKeyWord[lParamIndex]���ڵ�ַ�ε�ĳ����ַ��
         * ��pszKeyWord[lParamIndex]�Ƕ�̬������ڴ棬��������Ͳ������������ڴ���
         */
        pstParamsList->pszStringName = pszKeyWord[lParamIndex];
        pstParamsList->pszStringVal = pValue;

        dos_list_add_tail(&(pstClient->stParamList), &pstParamsList->stList);
    }

    sc_logr_debug("%s", "Prase the http request finished.");

    ulRet = DOS_FAIL;
    for (ulIndex=0; ulIndex<(sizeof(g_pstHttpReqRegTable)/sizeof(SC_HTTP_REQ_REG_TABLE_SC)); ulIndex++)
    {
        if (g_pstHttpReqRegTable[ulIndex].pszRequest
            && dos_strcmp(g_pstHttpReqRegTable[ulIndex].pszRequest, szReqBuffer) == 0)
        {
            if (g_pstHttpReqRegTable[ulIndex].callback)
            {
                ulRet = g_pstHttpReqRegTable[ulIndex].callback(pstClient);
            }

            break;
        }
    }

    sc_logr_notice("HTTP Request process finished. Return code: %d", ulRet);

    while (1)
    {
        if (dos_list_is_empty(&pstClient->stParamList))
        {
            break;
        }

        pstParamListNode = dos_list_fetch(&pstClient->stParamList);
        if (!pstParamListNode)
        {
            break;
        }

        pstParamsList = dos_list_entry(pstParamListNode, SC_API_PARAMS_ST, stList);
        if (!pstParamsList)
        {
            continue;
        }

        pstParamsList->pszStringName = NULL;
        pstParamsList->pszStringVal = NULL;
        dos_dmem_free(pstParamsList);
        pstParamsList = NULL;
    }

    for (lParamIndex=0; lParamIndex<lKeyCnt; lParamIndex++)
    {
        dos_dmem_free(pszKeyWord[lParamIndex]);
        pszKeyWord[lParamIndex] = NULL;
    }

    if (DOS_SUCC == ulRet)
    {
        pstClient->ulErrCode = SC_HTTP_ERRNO_SUCC;
        pstClient->ulResponseCode = SC_HTTP_OK;
        SC_TRACE_OUT();
        return DOS_SUCC;
    }

cmd_prase_fail1:
    pstClient->ulErrCode = SC_HTTP_ERRNO_INVALID_REQUEST;
    pstClient->ulResponseCode = SC_HTTP_OK;
    SC_TRACE_OUT();
    return DOS_FAIL;
}
#if 0
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
