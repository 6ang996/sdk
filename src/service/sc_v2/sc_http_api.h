/**
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  �ļ���: sc_http_api.h
 *
 *  ����ʱ��: 2014��12��16��10:15:56
 *  ��    ��: Larry
 *  ��    ��: ҵ�����ģ�飬Ⱥ���������ⲿ�ṩ�Ľӿں���
 *  �޸���ʷ:
 */

#ifndef __SC_HTTP_API_H__
#define __SC_HTTP_API_H__

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>

/* include public header files */
/* define marcos */
#define SC_API_PARAMS_MAX_NUM          24
#define SC_HTTP_REQ_MAX_LEN            64

/* HTTP��Ӧ���ݴ����� */
enum SC_HTTP_ERRNO
{
    SC_HTTP_ERRNO_SUCC = 0,                      /* �޴��� */
    SC_HTTP_ERRNO_INVALID_USR = 0xF0000001,      /* ����У��ʧ�ܣ����Ϸ����û� */
    SC_HTTP_ERRNO_INVALID_DATA,                  /* ����У��ʧ�ܣ����Ϸ������� */
    SC_HTTP_ERRNO_INVALID_TASK_STATUS,           /* ����У��ʧ�ܣ����Ϸ������� */
    SC_HTTP_ERRNO_INVALID_REQUEST,               /* ����У��ʧ�ܣ����Ϸ������� */
    SC_HTTP_ERRNO_INVALID_CMD,                   /* ����У��ʧ�ܣ����Ϸ��������� */
    SC_HTTP_ERRNO_INVALID_PARAM,                 /* ����У��ʧ�ܣ����Ϸ��Ĳ��� */
    SC_HTTP_ERRNO_CMD_EXEC_FAIL,                 /* ����ִ��ʧ�� */
    SC_HTTP_ERRNO_SERVER_ERROR,                  /* ����ִ��ʧ�� */
    SC_HTTP_ERRNO_SERVER_NOT_READY,

    SC_HTTP_ERRNO_BUTT
};


/* define enums */
enum tagAPICMDList
{
    SC_API_CMD_RELOAD                   = 0,      /* API�����֣�reload�����ļ� */
    SC_API_CMD_TASK_CTRL,                         /* API�����֣������������ */
    SC_API_CMD_CALL_CTRL,                         /* API�����֣����и�Ԥ */
    SC_API_CMD_NUM_VERIFY,                        /* API�����֣�������� */
    // ------------------------------------------------------------
    SC_API_CMD_GATEWAY,                           /* API�����֣����ؿ��� */
    //------------------------------------------------------------

    SC_API_CMD_BUTT
};

enum tagAPICMDActionList
{
    SC_API_CMD_ACTION_FORCE             = 0,      /* API����reload����ֵ��ǿ��ˢ��ָ����XML���� */
    SC_API_CMD_ACTION_FORCE_ALL,                  /* API����reload����ֵ��ǿ��ˢ���������� */
    SC_API_CMD_ACTION_CONDITIONAL,                /* API����reload����ֵ����������ˢ�£���Ӱ��ҵ�� */

    SC_API_CMD_ACTION_ADD,                        /* API����task ctrl����ֵ���������� */
    SC_API_CMD_ACTION_DELETE,                     /* API����task ctrl����ֵ��ֹͣ���� */
    SC_API_CMD_ACTION_UPDATE,
    SC_API_CMD_ACTION_START,                      /* API����task ctrl����ֵ���������� */
    SC_API_CMD_ACTION_STOP,                       /* API����task ctrl����ֵ��ֹͣ���� */
    SC_API_CMD_ACTION_CONTINUE,                   /* API����task ctrl����ֵ�������������� */
    SC_API_CMD_ACTION_PAUSE,                      /* API����task ctrl����ֵ����ͣ���� */
    SC_API_CMD_ACTION_STATUS,

    SC_API_CMD_ACTION_MONITORING,                 /* API����call ctrl����ֵ���������� */
    SC_API_CMD_ACTION_HUNGUP,                     /* API����call ctrl����ֵ���ҶϺ��� */

    SC_API_CMD_ACTION_SIGNIN,                     /* ��ϯǩ�룬��½ */
    SC_API_CMD_ACTION_SIGNOFF,                    /* ��ϯǩ�����ǳ� */
    SC_API_CMD_ACTION_ONLINE,                     /* ��ϯ���ߣ����� */
    SC_API_CMD_ACTION_OFFLINE,                    /* ��ϯ���ߣ���æ */

    //---------------------------------------------------
    SC_API_CMD_ACTION_GATEWAY_ADD,                       /* ����һ������ */
    SC_API_CMD_ACTION_GATEWAY_DELETE,                    /* ɾ��һ������ */
    SC_API_CMD_ACTION_GATEWAY_UPDATE,                    /* ���ظ���     */
    SC_API_CMD_ACTION_GATEWAY_SYNC,                      /* ��������ǿ��ͬ�� */

    SC_API_CMD_ACTION_SIP_ADD,                           /* ����һ��sip�˻� */
    SC_API_CMD_ACTION_SIP_DELETE,                        /* ɾ��һ��sip�˻� */
    SC_API_CMD_ACTION_SIP_UPDATE,                        /* ����һ��sip�˻� */

    SC_API_CMD_ACTION_ROUTE_ADD,                         /* ����һ��·�� */
    SC_API_CMD_ACTION_ROUTE_DELETE,                      /* ɾ��һ��·�� */
    SC_API_CMD_ACTION_ROUTE_UPDATE,                      /* ·�ɸ��� */
    SC_API_CMD_ACTION_ROUTE_SYNC,                        /* ·��ǿ������ͬ�� */

    SC_API_CMD_ACTION_GW_GROUP_ADD,                      /* ����һ�������� */
    SC_API_CMD_ACTION_GW_GROUP_DELETE,                   /* ɾ��һ�������� */
    SC_API_CMD_ACTION_GW_GROUP_UPDATE,

    SC_API_CMD_ACTION_DID_ADD,                           /* ���һ��did */
    SC_API_CMD_ACTION_DID_DELETE,                        /* ɾ��һ��did */
    SC_API_CMD_ACTION_DID_UPDATE,                        /* ����һ��did */

    SC_API_CMD_ACTION_BLACK_ADD,                         /* ��Ӻ����� */
    SC_API_CMD_ACTION_BLACK_DELETE,                      /* ɾ�������� */
    SC_API_CMD_ACTION_BLACK_UPDATE,                      /* ���º����� */

    SC_API_CMD_ACTION_CALLER_ADD,                        /* ������� */
    SC_API_CMD_ACTION_CALLER_DELETE,                     /* ɾ������ */
    SC_API_CMD_ACTION_CALLER_UPDATE,                     /* ���и��� */

    SC_API_CMD_ACTION_CALLER_GRP_ADD,                    /* ������к����� */
    SC_API_CMD_ACTION_CALLER_GRP_DELETE,                 /* ɾ�����к����� */
    SC_API_CMD_ACTION_CALLER_GRP_UPDATE,                 /* �������к����� */

    SC_API_CMD_ACTION_CALLER_SET_ADD,                    /* �������к����趨 */
    SC_API_CMD_ACTION_CALLER_SET_DELETE,                 /* ɾ�����к����趨 */
    SC_API_CMD_ACTION_CALLER_SET_UPDATE,                 /* �������к����趨 */

    SC_API_CMD_ACTION_TRANSFORM_ADD,                     /* ���Ӻ���任 */
    SC_API_CMD_ACTION_TRANSFORM_DELETE,                  /* ɾ������任 */
    SC_API_CMD_ACTION_TRANSFORM_UPDATE,                  /* ���º���任 */

    SC_API_CMD_ACTION_DEMO_TASK,                         /* Ⱥ��������ʾ */
    SC_API_CMD_ACTION_DEMO_PREVIEW,                      /* Ԥ�������ʾ */

    SC_API_CMD_ACTION_SWITCHBOARD_ADD,
    SC_API_CMD_ACTION_SWITCHBOARD_UPDATE,
    SC_API_CMD_ACTION_SWITCHBOARD_DELETE,

    SC_API_CMD_ACTION_KEYMAP_ADD,
    SC_API_CMD_ACTION_KEYMAP_UPDATE,
    SC_API_CMD_ACTION_KEYMAP_DELETE,

    SC_API_CMD_ACTION_PERIOD_ADD,
    SC_API_CMD_ACTION_PERIOD_UPDATE,
    SC_API_CMD_ACTION_PERIOD_DELETE,

    SC_API_CMD_ACTION_BUTT
};

enum tagAPICallCtrlCmd
{
    SC_API_MAKE_CALL                 = 0, /* ������� */
    SC_API_HANGUP_CALL,                   /* �ҶϺ��� */
    SC_API_RECORD,                        /* ����¼�� */
    SC_API_WHISPERS,                      /* ���� */
    SC_API_INTERCEPT,                     /* ���� */
    SC_API_TRANSFOR_ATTAND,               /* attend transfer */
    SC_API_TRANSFOR_BLIND,                /* blind transfer */
    SC_API_CONFERENCE,                    /* ���� */
    SC_API_HOLD,                          /* ���б��� */
    SC_API_UNHOLD,                        /* ȡ�����б��� */

    SC_API_CALLCTRL_BUTT
}SC_API_CALLCTRL_CMD_EN;

enum tagAPIParams
{
    SC_API_VERSION                      = 0,      /* API������API�汾�� */
    SC_API_PARAM_CMD,                             /* API���������� */
    SC_API_PARAM_ACTION,                          /* API�����������ֵ */
    SC_API_PARAM_CUSTOM_ID,                       /* API�������û�ID */
    SC_API_PARAM_TASK_ID,                         /* API����������ID */
    SC_API_PARAM_UUID,                            /* API����������Ψһ��ʾ�� */
    SC_API_PARAM_NUM,                             /* API����������˵ĺ��� */
    SC_API_PARAM_EXTRACODE,                       /* API������������˸����� */

    SC_API_PARAM_BUTT
};

/* define structs */
typedef struct tagSCAPIParams
{
    S8    *pszStringName;                       /* �ַ���ֵ */
    S8    *pszStringVal;                       /* �ַ���ֵ */

    list_t  stList;
}SC_API_PARAMS_ST;

typedef struct tagHttpDataBuf
{
    S8          *pszBuff;                       /* ��������ָ�� */
    U32         ulLength;                       /* ���泤�� */
}SC_DATA_BUF_ST;


typedef struct tagHttpClientCB
{
    U32               ulValid;                  /* �Ƿ���� */
    U32               ulIndex;                  /* ��ǰ��� */
    U32               ulCurrentSrv;             /* ��ǰ��������� */

    S32               lSock;                    /* ��ǰ���ӵ�socket */

    U32               ulResponseCode;           /* HTTP��Ӧ�� */
    U32               ulErrCode;                /* HTTP��Ӧ�� */

    SC_DATA_BUF_ST    stDataBuff;

    list_t  stParamList;                        /* ��������б� */
    list_t  stResponseDataList;   /* ������ڵ�  refer to RESULT_DATA_NODE_ST */
}SC_HTTP_CLIENT_CB_S;


typedef U32 (*http_req_handle)(list_t *pstArgv);

typedef struct tagHttpRequestProcess
{
    S8              *pszRequest;
    http_req_handle callback;
}SC_HTTP_REQ_REG_TABLE_SC;

/* declare functions */
http_req_handle sc_http_api_find(S8 *pszName);
U32 sc_http_api_process(SC_HTTP_CLIENT_CB_S *pstClient);
U32 sc_http_api_reload_xml(list_t *pstArgv);
U32 sc_http_api_task_ctrl(list_t *pstArgv);
U32 sc_http_api_gateway_action(list_t *pstArgv);
U32 sc_http_api_sip_action(list_t *pstArgv);
U32 sc_http_api_num_verify(list_t *pstArgv);
U32 sc_http_api_call_ctrl(list_t *pstArgv);
U32 sc_http_api_agent_action(list_t *pstArgv);
U32 sc_http_api_agent_grp(list_t *pstArgv);
U32 sc_http_api_route_action(list_t *pstArgv);
U32 sc_http_api_gw_group_action(list_t *pstArgv);
U32 sc_http_api_did_action(list_t *pstArgv);
U32 sc_http_api_switchboard_action(list_t *pstArgv);
U32 sc_http_api_period_action(list_t *pstArgv);
U32 sc_http_api_keymap_action(list_t *pstArgv);
U32 sc_http_api_sys_stat_sync();

U32 sc_http_api_black_action(list_t *pstArgv);
U32 sc_http_api_caller_action(list_t *pstArgv);
U32 sc_http_api_callergrp_action(list_t *pstArgv);
U32 sc_http_api_callerset_action(list_t *pstArgv);
U32 sc_http_api_eix_action(list_t *pstArgv);
U32 sc_http_api_numlmt_action(list_t *pstArgv);
U32 sc_http_api_numtransform_action(list_t *pstArgv);
U32 sc_http_api_customer_action(list_t *pstArgv);
U32 sc_http_api_demo_action(list_t *pstArgv);
U32 sc_agent_group_http_update_proc(U32 ulAction, U32 ulGrpID);
U32 sc_http_api_agent_call_ctrl(list_t *pstArgv);
U32 sc_http_api_agent(list_t *pstArgv);
U32 sc_http_api_serv_ctrl_action(list_t *pstArgv);
U32 sc_http_api_stat_syn(list_t *pstArgv);
U32 sc_task_mngt_cmd_proc(U32 ulAction, U32 ulCustomerID, U32 ulTaskID);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __SC_TASK_PUB_H__ */

